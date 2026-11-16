#include "html_label.h"

#include "editor/docrender_source_editor.h"
#include "formats/html/dom/html_document_fragment.h"
#include "formats/html/elements/html_input_node.h"
#include "formats/html/css/css_values.h"

#include "formats/html/markup/html_markup.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/h_scroll_bar.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_scroll_bar.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string_view>
#include <utility>

using namespace godot;

using docrender::HTMLInputNode;

namespace {
	constexpr int DRAG_DEADZONE = 6;
	constexpr int HOVER_PASSES = 2;

	std::optional<bool> tab_direction(const InputEventKey& p_key) {
		if (p_key.is_action_pressed("ui_focus_prev", true, true)) {
			return true;
		}
		if (p_key.is_action_pressed("ui_focus_next", true, true)) {
			return false;
		}
		return std::nullopt;
	}

	std::optional<Control::CursorShape> get_css_cursor(std::string_view p_cursor) {
		static constexpr std::pair<std::string_view, Control::CursorShape> shapes[] = {
			{ "default", Control::CURSOR_ARROW },
			{ "pointer", Control::CURSOR_POINTING_HAND },
			{ "text", Control::CURSOR_IBEAM },
			{ "crosshair", Control::CURSOR_CROSS },
			{ "move", Control::CURSOR_MOVE },
			{ "wait", Control::CURSOR_WAIT },
			{ "progress", Control::CURSOR_BUSY },
			{ "help", Control::CURSOR_HELP },
			{ "not-allowed", Control::CURSOR_FORBIDDEN },
			{ "ew-resize", Control::CURSOR_HSIZE },
			{ "ns-resize", Control::CURSOR_VSIZE },
		};
		for (const auto& [keyword, shape] : shapes) {
			if (std::equal(p_cursor.begin(), p_cursor.end(), keyword.begin(), keyword.end(), [](char p_char, char p_lowercase) {
				return p_char == p_lowercase || (p_char >= 'A' && p_char <= 'Z' && p_char - 'A' + 'a' == p_lowercase);
			})) {
				return shape;
			}
		}
		return std::nullopt;
	}
}

void HTMLLabelCanvas::_draw() {
	if (!label) {
		return;
	}
	const Rect2 visible_rect = label->get_visible_rect();
	if (visible_rect.has_area()) {
		const Vector2i selection(std::min(selection_anchor, selection_caret), std::max(selection_anchor, selection_caret));
		const Color color = label->has_theme_color("selection_color", "HTMLLabel")
			? label->get_theme_color("selection_color", "HTMLLabel") : Color(0.25f, 0.55f, 0.95f, 0.5f);
		label->layout->draw_with_highlights(this, Vector2(), visible_rect, selection, nullptr, color, {}, nullptr, visible_rect.position);
		update_ime();
	}
}

bool HTMLLabelCanvas::_has_point(const Vector2& p_point) const {
	return label && label->get_visible_rect().has_point(p_point);
}

String HTMLLabelCanvas::_get_tooltip(const Vector2& p_position) const {
	return !selecting && _has_point(p_position) ? label->layout->get_css_diagnostic_tooltip(p_position) : String();
}

Object* HTMLLabelCanvas::_make_custom_tooltip(const String& p_for_text) const {
	if (p_for_text.is_empty() || !label || !label->is_css_diagnostics_enabled()) {
		return nullptr;
	}
	const Vector2 viewport_size = get_viewport_rect().size;
	const int width = std::max(1, int(std::min<real_t>(380.0, viewport_size.x * 0.8)));
	const int max_height = std::max(1, int(std::min<real_t>(320.0, viewport_size.y * 0.6)));
	const String css = R"CSS(
		body { margin: 0; padding: 8px 12px; background: #23272e; color: #f5f5f5; font-size: 13px; }
		.element { font-weight: bold; padding-bottom: 6px; }
		.diagnostic { border-top: 1px solid #51565f; padding: 6px 0; }
		.kind { color: #ffba65; }
		.source { color: #b8bdc7; font-size: 12px; }
	)CSS";
	const String html = "<!doctype html>" + p_for_text;
	// Size the tooltip from its HTML before Godot shrinks the popup to its minimum size.
	Ref<HTMLLayout> measure;
	measure.instantiate();
	if (measure->parse(html, "res://", css) != OK || measure->layout(Vector2i(width, max_height)) != OK) {
		return nullptr;
	}
	HTMLLabel* panel = memnew(HTMLLabel);
	panel->set_name("CSSDiagnostics");
	panel->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	panel->set_selection_enabled(false);
	panel->set_stylesheet(css);
	panel->set_text(html);
	panel->set_custom_minimum_size(Vector2(width, std::clamp(measure->get_content_size().y, 1, max_height)));
	diagnostic_panel_id = ObjectID(panel->get_instance_id());
	diagnostic_panel_text = p_for_text;
	return panel;
}

void HTMLLabelCanvas::refresh_diagnostic_tooltip(const std::optional<Vector2>& p_point) {
	if (diagnostic_panel_id == ObjectID()) {
		return;
	}
	auto* panel = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(diagnostic_panel_id));
	if (panel && p_point && _get_tooltip(*p_point) == diagnostic_panel_text) {
		return;
	}
	diagnostic_panel_id = ObjectID();
	diagnostic_panel_text = String();
	// Godot owns the popup. Hide it when its contents change without mouse motion.
	if (panel) {
		if (auto* popup = Object::cast_to<Window>(panel->get_parent())) {
			popup->hide();
		}
	}
}

void HTMLLabelCanvas::_gui_input(const Ref<InputEvent>& p_event) {
	if (!label) {
		return;
	}
	const Rect2 visible_rect = label->get_visible_rect();
	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid()) {
		if (dragging_range.is_valid()) {
			const auto range = dragging_range;
			accept_event();
			move_range(range, motion->get_position(), false);
			return;
		}
		const Vector2 point = motion->get_position();
		if (scrollbar_drag) {
			label->layout->drag_scrollbar(*scrollbar_drag, point);
			accept_event();
			return;
		}
		if (textarea_drag) {
			move_textarea_drag(point);
			accept_event();
			return;
		}
		if (dragging_input.is_valid()) {
			set_input_caret(dragging_input, point, true);
			label->request_hover_check();
			accept_event();
			return;
		}
		if (pressed_checkbox.is_valid() && (!visible_rect.has_point(point) ||
			label->layout->get_element_at(point) != pressed_checkbox)) {
			set_element_pseudo(pressed_checkbox, "active", false);
			pressed_checkbox.unref();
		}
		const bool moved = (point - visible_rect.position).distance_squared_to(pressed_position) > DRAG_DEADZONE * DRAG_DEADZONE;
		if (!visible_rect.has_point(point) || (selecting && moved)) {
			cancel_click();
		}
		else {
			if (moved) {
				pressed_link = 0;
				pressed_element.unref();
			}
			if (pressed_button.is_valid() &&
				label->layout->get_button_ancestor(label->layout->get_element_at(point)) != pressed_button) {
				cancel_button_press();
			}
		}
		if (selecting && (moved || selection_dragged)) {
			selection_dragged = true;
			const Vector2 clamped = point.clamp(visible_rect.position, visible_rect.get_end());
			extend_document_selection(clamped);
			set_process_internal(true);
			queue_redraw();
			accept_event();
		}
		label->request_hover_check();
		return;
	}

	const Ref<InputEventPanGesture> pan = p_event;
	const Ref<InputEventMouseButton> button = p_event;
	const auto scroll = [&](const Vector2& position) {
		if (!visible_rect.has_point(position)) return;
		const auto result = label->layout->scroll_overflow_from_input(position, p_event, *label,
			callable_mp(label, &HTMLLabel::on_nested_scroll_advanced));
		if (result == docrender::HTMLScrollInput::ignored) return;
		cancel_click();
		if (selecting && selection_dragged) extend_document_selection(position);
		label->request_hover_check();
		if (result == docrender::HTMLScrollInput::handled) accept_event();
	};
	if (pan.is_valid()) {
		scroll(pan->get_position());
		return;
	}
	if (button.is_null()) return;
	const Vector2 point = button->get_position();
	if (visible_rect.has_point(point) && label->handle_listbox_mouse(label->layout->get_element_at(point), *button.ptr(), point)) {
		cancel_press();
		accept_event();
		return;
	}
	if (button->get_button_index() == MOUSE_BUTTON_LEFT && !button->is_pressed() && dragging_range.is_valid()) {
		const auto range = dragging_range;
		dragging_range.unref();
		accept_event();
		if (!button->is_canceled()) move_range(range, point, true);
		else commit_input(range);
		return;
	}
	if (button->get_button_index() == MOUSE_BUTTON_RIGHT && button->is_pressed() && label->context_menu_enabled &&
		visible_rect.has_point(point) && open_input_menu(label->layout->get_element_at(point), point, true)) {
		accept_event();
		return;
	}
	if (button->get_button_index() != MOUSE_BUTTON_LEFT) {
		scroll(point);
		return;
	}
	if (!button->is_pressed() && scrollbar_drag) {
		if (!button->is_canceled()) {
			label->layout->drag_scrollbar(*scrollbar_drag, point);
			label->layout->finish_scrollbar_drag(*scrollbar_drag);
		}
		scrollbar_drag.reset();
		label->layout->set_scrollbar_pressed(std::nullopt);
		label->request_hover_check();
		accept_event();
		return;
	}
	if (button->is_pressed() && visible_rect.has_point(point)) {
		if (const auto drag = label->layout->scrollbar_at(point)) {
			cancel_press();
			scrollbar_drag = drag;
			label->layout->drag_scrollbar(*drag, point);
			set_default_cursor_shape(CURSOR_ARROW);
			accept_event();
			return;
		}
	}
	if (!button->is_pressed() && textarea_drag) {
		if (!button->is_canceled()) move_textarea_drag(point);
		flush_textarea_resize();
		textarea_drag.reset();
		label->layout->set_scrollbar_pressed(std::nullopt);
		label->request_hover_check();
		accept_event();
		return;
	}
	Ref<HTMLElement> element;
	const HTMLLayout::LinkHit hit = visible_rect.has_point(point)
		? label->layout->find_link_at(point, &element) : HTMLLayout::LinkHit{};
	const Ref<HTMLElement> target_button = label->layout->get_button_ancestor(element);
	const Ref<HTMLElement> form_target = hit.id == 0 ? get_form_click_target(element) : Ref<HTMLElement>();
	const Ref<HTMLInputElement> target_input = element;
	if (!button->is_pressed() && dragging_input.is_valid()) {
		if (!button->is_canceled() && point != pressed_position + visible_rect.position) set_input_caret(dragging_input, point, true);
		dragging_input.unref();
		set_process_internal(false);
		accept_event();
		return;
	}
	if (button->is_pressed()) {
		if (target_input.is_valid() && target_input->get_input()->type() == docrender::HTMLInputNode::Type::RANGE &&
			label->layout->is_focusable(target_input)) {
			cancel_press();
			if (label->focus_element(target_input)) {
				dragging_range = target_input;
				accept_event();
				move_range(target_input, point, false);
			}
			return;
		}
		if (begin_textarea_drag(element, point)) {
			accept_event();
			return;
		}
		cancel_button_press();
		cancel_key_press();
		pressed_position = point - visible_rect.position;
		pressed_link = hit.id;
		pressed_element = element;
		pressed_form_target = form_target;
		selecting = label->selection_enabled && visible_rect.has_point(point);
		selection_dragged = false;
		const Ref<HTMLElement> target_focus = label->layout->get_focus_ancestor(element);
		if (target_focus.is_valid()) label->grab_focus();
		set_focused_element(target_focus);
		const auto control = label->layout->text_control(element);
		if (control) {
			selecting = false;
			pressed_element.unref();
			pressed_link = 0;
			if (target_focus == element) {
				if (control->is_text_control()) {
					dragging_input = element;
					set_input_caret(element, point, button->is_shift_pressed());
					if (control->editor.is_multiline()) set_process_internal(true);
				}
				else if (target_input.is_valid() && target_input->get_input()->is_toggle()) {
					pressed_checkbox = target_input;
					set_element_pseudo(pressed_checkbox, "active", true);
				}
			}
			accept_event();
			return;
		}
		if (label->layout->is_enabled_button(target_button)) {
			pressed_button = target_button;
			set_element_pseudo(pressed_button, "active", true);
		}
		if (selecting) {
			selection_origin = label->layout->selection_point_at(point);
			selecting = !selection_origin.excluded;
			if (selecting) {
				const Vector2i selection = docrender::html_selection_between(selection_origin, selection_origin);
				selection_anchor = selection.x;
				selection_caret = selection.y;
				if (target_button.is_null()) label->grab_focus();
				queue_redraw();
			}
		}
		if (selecting || target_button.is_valid() || form_target.is_valid()) {
			accept_event();
		}
		return;
	}

	if (pressed_checkbox.is_valid()) {
		const Ref<HTMLInputElement> checkbox = pressed_checkbox;
		set_element_pseudo(checkbox, "active", false);
		pressed_checkbox.unref();
		accept_event();
		if (!button->is_canceled() && checkbox == target_input) toggle_checkbox(checkbox);
		return;
	}

	const bool clicked = !selection_dragged && !button->is_canceled() &&
		(point - visible_rect.position).distance_squared_to(pressed_position) <= DRAG_DEADZONE * DRAG_DEADZONE;
	const bool link_clicked = clicked && pressed_link != 0 && pressed_link == hit.id;
	const bool element_clicked = clicked && element.is_valid() && pressed_element == element;
	const uint64_t click_version = element_clicked ? element->adoption_version : 0;
	const bool button_clicked = !selection_dragged && !button->is_canceled() &&
		pressed_button == target_button && label->layout->is_enabled_button(target_button);
	const bool form_clicked = clicked && form_target.is_valid() && pressed_form_target == form_target;
	if (selecting) {
		if (selection_dragged && !button->is_canceled()) {
			extend_document_selection(point.clamp(visible_rect.position, visible_rect.get_end()));
			queue_redraw();
		}
	}
	if (selecting || pressed_button.is_valid() || form_clicked) {
		accept_event();
	}
	const bool was_dragging = selection_dragged;
	cancel_press();
	if (was_dragging) {
		label->request_hover_check();
		callable_mp(label->layout.ptr(), &HTMLLayout::request_auto_content_update).call_deferred();
	}
	const ObjectID label_id(label->get_instance_id());
	if (link_clicked) {
		label->activate_link(hit.uri);
	}
	if (button_clicked) {
		if (auto* target = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(label_id))) {
			target->activate_element(target_button);
		}
	}
	else if (form_clicked) {
		if (auto* target = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(label_id))) target->activate_element(form_target);
	}
	if (element_clicked && element->is_valid() && element->adoption_version == click_version) {
		if (auto* target = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(label_id))) {
			target->emit_signal("element_clicked", element);
		}
	}
}

void HTMLLabelCanvas::extend_document_selection(const Vector2& p_point) {
	const Vector2i selection = docrender::html_selection_between(selection_origin, label->layout->selection_point_at(p_point));
	selection_anchor = selection.x;
	selection_caret = selection.y;
}

void HTMLLabel::_input(const Ref<InputEvent>& p_event) {
	if (open_select.is_valid()) {
		const Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->is_pressed() && button->get_button_index() == MOUSE_BUTTON_LEFT &&
			!Rect2(Vector2(), select_menu->get_size()).has_point(select_menu->get_global_transform_with_canvas().affine_inverse().xform(button->get_position()))) {
			const auto point = get_global_transform_with_canvas().affine_inverse().xform(button->get_position());
			if (get_element_at(point) != open_select) close_select_menu();
		}
	}
	if (!canvas || has_focus() || canvas->tab_elements.empty() || !is_visible_in_tree()) {
		return;
	}
	const Ref<InputEventKey> key = p_event;
	if (key.is_valid()) {
		if (const auto direction = tab_direction(*key.ptr())) {
			// Godot moves focus after _input, and FOCUS_ENTER has no input event.
			canvas->tab_entry_backward = direction;
			callable_mp(canvas, &HTMLLabelCanvas::clear_tab_entry).call_deferred();
		}
	}
}

void HTMLLabel::_gui_input(const Ref<InputEvent>& p_event) {
	const Ref<InputEventKey> key = p_event;
	if (canvas && key.is_valid() && !canvas->handle_dom_key(*key.ptr())) {
		canvas->handle_selection_key(*key.ptr());
	}
}

void HTMLLabelCanvas::handle_selection_key(const InputEventKey& p_key) {
	if (!label->selection_enabled || !p_key.is_pressed()) {
		return;
	}
	if (p_key.is_action("ui_text_select_all", true)) {
		label->select_all();
		label->accept_event();
	}
	else if (p_key.is_action("ui_copy", true)) {
		const String text = label->get_selected_text();
		if (!text.is_empty()) {
			DisplayServer::get_singleton()->clipboard_set(text);
			label->accept_event();
		}
	}
}

void HTMLLabelCanvas::clear_tab_entry() {
	tab_entry_backward.reset();
}

void HTMLLabelCanvas::focus_tab_endpoint(bool p_backward) {
	if (label->has_focus() && focused_element.is_null() && !tab_elements.empty()) {
		set_focused_element(p_backward ? tab_elements.back() : tab_elements.front(), true);
	}
}

bool HTMLLabelCanvas::handle_dom_key(const InputEventKey& p_key) {
	if (const auto direction = tab_direction(p_key); direction && !tab_elements.empty()) {
		label->close_select_menu();
		const bool backwards = *direction;
		auto button = std::find(tab_elements.begin(), tab_elements.end(), focused_element);
		if (button != tab_elements.end()) {
			if (backwards && button != tab_elements.begin()) {
				--button;
			}
			else if (!backwards && std::next(button) != tab_elements.end()) {
				++button;
			}
			else {
				const auto* next = backwards ? label->find_prev_valid_focus() : label->find_next_valid_focus();
				if (next && next != label) {
					return false;
				}
				button = backwards ? std::prev(tab_elements.end()) : tab_elements.begin();
			}
		}
		else {
			button = backwards ? std::prev(tab_elements.end()) : tab_elements.begin();
		}
		accept_event();
		set_focused_element(*button, true);
		return true;
	}

	if (label->handle_select_key(p_key) || handle_input_key(p_key)) return true;
	const Key key = p_key.get_keycode() != KEY_NONE ? p_key.get_keycode() : p_key.get_physical_keycode();
	if ((key == KEY_ENTER || key == KEY_KP_ENTER) && p_key.is_pressed() && !p_key.is_echo() &&
		!p_key.is_alt_pressed() && !p_key.is_ctrl_pressed() && !p_key.is_meta_pressed()) {
		const String uri = label->layout->get_activatable_link_uri(focused_element);
		if (!uri.is_empty()) {
			label->accept_event();
			label->activate_link(uri);
			return true;
		}
	}
	if (key != KEY_ENTER && key != KEY_KP_ENTER && key != KEY_SPACE) {
		return false;
	}
	if (!label->layout->is_enabled_button(focused_element)) {
		cancel_key_press();
		return false;
	}
	if (p_key.is_alt_pressed() || p_key.is_ctrl_pressed() || p_key.is_meta_pressed()) {
		cancel_key_press();
		return false;
	}
		label->accept_event();
		cancel_click();
	if (p_key.is_echo()) {
		return true;
	}
	if (p_key.is_pressed() && activation_key == KEY_NONE && pressed_button.is_null()) {
		activation_key = key;
		set_element_pseudo(focused_element, "active", true);
		if (key != KEY_SPACE) {
			const Ref<HTMLElement> target = focused_element;
			label->activate_element(target);
		}
	}
	else if (!p_key.is_pressed() && activation_key == key) {
		const Ref<HTMLElement> target = focused_element;
		cancel_key_press();
		if (key == KEY_SPACE && !p_key.is_canceled()) {
			label->activate_element(target);
		}
	}
	return true;
}

void HTMLLabelCanvas::set_focused_element(const Ref<HTMLElement>& p_element, bool p_keyboard) {
	if (focused_element != p_element) {
		if (!label->layout->prepare_content_focus(p_element)) return;
		close_input_menu();
		label->close_select_menu();
		const auto previous = focused_element;
		stop_caret_blink();
		close_ime();
		commit_input(previous, true);
		dragging_input.unref();
		cancel_key_press();
		set_element_pseudo(focused_element, "focus", false);
		set_element_pseudo(focused_element, "focus-visible", false);
		focused_element = p_element;
		const auto input = label->layout->text_control(focused_element);
		focused_reset_version = input ? input->editor.get_reset_version() : 0;
		set_element_pseudo(focused_element, "focus", true);
		restart_caret_blink();
		label->layout->repaint_input();
		update_ime();
	}
	set_element_pseudo(focused_element, "focus-visible", p_keyboard);
	label->reveal_focused_element = focused_element.is_valid() && p_keyboard;
	if (label->reveal_focused_element) {
		label->queue_interaction();
	}
}

void HTMLLabelCanvas::set_element_pseudo(const Ref<HTMLElement>& p_button, const char* p_name, bool p_enabled) {
	if (label->layout->set_interaction_pseudo(p_button, p_name, p_enabled)) {
		label->queue_interaction_style_refresh();
	}
}

void HTMLLabelCanvas::update_focus_mode() {
	label->set_focus_mode(!tab_elements.empty() ? FOCUS_ALL :
		(label->selection_enabled || has_focusable_elements ? FOCUS_CLICK : FOCUS_NONE));
	label->set_process_input(!tab_elements.empty());
}

void HTMLLabelCanvas::_notification(int p_what) {
	if (!label) {
		return;
	}
	if (p_what == NOTIFICATION_MOUSE_ENTER || p_what == NOTIFICATION_MOUSE_EXIT) {
		if (p_what == NOTIFICATION_MOUSE_EXIT) {
			cancel_click();
		}
		if (!label->updating_gui_hover) {
			label->request_hover_check();
		}
	}
	else if (p_what == NOTIFICATION_EXIT_TREE) {
		cancel_press();
		set_focused_element({});
		caret_window_active = true;
		tab_elements.clear();
		has_focusable_elements = false;
		clear_tab_entry();
		set_default_cursor_shape(CURSOR_ARROW);
	}
	else if (p_what == NOTIFICATION_INTERNAL_PROCESS && dragging_input.is_valid()) {
		update_input_autoscroll();
	}
	else if (p_what == NOTIFICATION_INTERNAL_PROCESS && selecting && selection_dragged) {
		const Rect2 visible = label->get_visible_rect();
		const Vector2 point = get_local_mouse_position();
		const Vector2 clamped = point.clamp(visible.position, visible.get_end());
		const Vector2 outside = point - clamped;
		if (outside != Vector2()) {
			const Vector2i offset = label->get_scroll_offset();
			const Vector2 step = outside * std::min(1.0, get_process_delta_time() * 12.0);
			label->apply_scroll_offset(offset + Vector2i(step.round()) * label->viewport_input_axes);
			const Vector2i scrolled = label->get_scroll_offset() - offset;
			if (scrolled != Vector2i()) {
				extend_document_selection(clamped + Vector2(scrolled));
				queue_redraw();
			}
		}
	}
}

void HTMLLabelCanvas::cancel_press() {
	if (label && (textarea_drag || scrollbar_drag)) label->layout->set_scrollbar_pressed(std::nullopt);
	textarea_drag.reset();
	scrollbar_drag.reset();
	resize_pending.reset();
	dragged_resize = false;
	dragging_input.unref();
	dragging_range.unref();
	cancel_click();
	cancel_key_press();
	selecting = false;
	selection_dragged = false;
	set_process_internal(false);
}

void HTMLLabelCanvas::cancel_click() {
	if (pressed_checkbox.is_valid()) {
		set_element_pseudo(pressed_checkbox, "active", false);
		pressed_checkbox.unref();
	}
	pressed_link = 0;
	pressed_element.unref();
	pressed_form_target.unref();
	cancel_button_press();
}

void HTMLLabelCanvas::cancel_button_press() {
	if (pressed_button.is_valid()) {
		set_element_pseudo(pressed_button, "active", false);
		pressed_button.unref();
	}
}

void HTMLLabelCanvas::cancel_key_press() {
	if (std::exchange(checkbox_key_pressed, false)) set_element_pseudo(focused_element, "active", false);
	if (activation_key != KEY_NONE) {
		activation_key = KEY_NONE;
		set_element_pseudo(focused_element, "active", false);
	}
}

void HTMLLabelCanvas::update_cursor(const std::optional<Vector2>& p_point) {
	if (!p_point || scrollbar_drag || label->layout->scrollbar_at(*p_point)) {
		set_default_cursor_shape(CURSOR_ARROW);
		return;
	}
	if (const auto resize = textarea_resize_cursor(*p_point)) {
		set_default_cursor_shape(*resize);
		return;
	}
	if (selection_dragged || dragging_input.is_valid()) {
		set_default_cursor_shape(CURSOR_IBEAM);
		return;
	}
	bool over_link = false;
	const auto css_cursor = get_css_cursor(label->layout->get_cursor_at(*p_point, over_link));
	set_default_cursor_shape(css_cursor.value_or(over_link ? CURSOR_POINTING_HAND :
		(label->selection_enabled ? CURSOR_IBEAM : CURSOR_ARROW)));
}

HTMLLabel::HTMLLabel() {
	layout.instantiate();
	layout->uses_document_viewport = true;
	layout->uses_owner_animation_clock = true;
	set_mouse_filter(MOUSE_FILTER_IGNORE);
}

void HTMLLabel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_document", "document"), &HTMLLabel::set_document);
	ClassDB::bind_method(D_METHOD("get_document"), &HTMLLabel::get_document);
	ClassDB::bind_method(D_METHOD("set_text", "text"), &HTMLLabel::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &HTMLLabel::get_text);
	ClassDB::bind_method(D_METHOD("set_base_dir", "base_dir"), &HTMLLabel::set_base_dir);
	ClassDB::bind_method(D_METHOD("get_base_dir"), &HTMLLabel::get_base_dir);
	ClassDB::bind_method(D_METHOD("set_stylesheet", "stylesheet"), &HTMLLabel::set_stylesheet);
	ClassDB::bind_method(D_METHOD("get_stylesheet"), &HTMLLabel::get_stylesheet);
	ClassDB::bind_method(D_METHOD("get_content_size"), &HTMLLabel::get_content_size);
	ClassDB::bind_method(D_METHOD("get_render_error"), &HTMLLabel::get_render_error);
	ClassDB::bind_method(D_METHOD("set_css_diagnostics_enabled", "enabled"), &HTMLLabel::set_css_diagnostics_enabled);
	ClassDB::bind_method(D_METHOD("is_css_diagnostics_enabled"), &HTMLLabel::is_css_diagnostics_enabled);
	ClassDB::bind_method(D_METHOD("get_css_diagnostics"), &HTMLLabel::get_css_diagnostics);
	ClassDB::bind_method(D_METHOD("get_scroll_offset"), &HTMLLabel::get_scroll_offset);
	ClassDB::bind_method(D_METHOD("set_scroll_offset", "offset"), &HTMLLabel::set_scroll_offset);
	ClassDB::bind_method(D_METHOD("scroll_to_anchor", "id", "align"), &HTMLLabel::scroll_to_anchor, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("create_element", "tag_name"), &HTMLLabel::create_element);
	ClassDB::bind_method(D_METHOD("create_document_fragment"), &HTMLLabel::create_document_fragment);
	ClassDB::bind_method(D_METHOD("query_selector", "selector"), &HTMLLabel::query_selector);
	ClassDB::bind_method(D_METHOD("query_selector_all", "selector"), &HTMLLabel::query_selector_all);
	ClassDB::bind_method(D_METHOD("get_element_by_id", "id"), &HTMLLabel::get_element_by_id);
	ClassDB::bind_method(D_METHOD("get_element_at", "position"), &HTMLLabel::get_element_at);
	ClassDB::bind_method(D_METHOD("get_focused_element"), &HTMLLabel::get_focused_element);
	ClassDB::bind_method(D_METHOD("focus_element", "element"), &HTMLLabel::focus_element);
	ClassDB::bind_method(D_METHOD("activate_element", "element"), &HTMLLabel::activate_element);
	ClassDB::bind_method(D_METHOD("submit_form", "form", "submitter"), &HTMLLabel::submit_form, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("scroll_to_element", "element", "align"), &HTMLLabel::scroll_to_element, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("set_selection_enabled", "enabled"), &HTMLLabel::set_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_selection_enabled"), &HTMLLabel::is_selection_enabled);
	ClassDB::bind_method(D_METHOD("set_context_menu_enabled", "enabled"), &HTMLLabel::set_context_menu_enabled);
	ClassDB::bind_method(D_METHOD("is_context_menu_enabled"), &HTMLLabel::is_context_menu_enabled);
	ClassDB::bind_method(D_METHOD("set_animation_process_mode", "mode"), &HTMLLabel::set_animation_process_mode);
	ClassDB::bind_method(D_METHOD("get_animation_process_mode"), &HTMLLabel::get_animation_process_mode);
	ClassDB::bind_method(D_METHOD("advance_animation", "delta"), &HTMLLabel::advance_animation);
	ClassDB::bind_method(D_METHOD("set_caret_blink_enabled", "enabled"), &HTMLLabel::set_caret_blink_enabled);
	ClassDB::bind_method(D_METHOD("is_caret_blink_enabled"), &HTMLLabel::is_caret_blink_enabled);
	ClassDB::bind_method(D_METHOD("set_caret_blink_interval", "interval"), &HTMLLabel::set_caret_blink_interval);
	ClassDB::bind_method(D_METHOD("get_caret_blink_interval"), &HTMLLabel::get_caret_blink_interval);
	ClassDB::bind_method(D_METHOD("select_all"), &HTMLLabel::select_all);
	ClassDB::bind_method(D_METHOD("clear_selection"), &HTMLLabel::clear_selection);
	ClassDB::bind_method(D_METHOD("get_selected_text"), &HTMLLabel::get_selected_text);

	BIND_ENUM_CONSTANT(ANIMATION_PROCESS_IDLE);
	BIND_ENUM_CONSTANT(ANIMATION_PROCESS_PHYSICS);
	BIND_ENUM_CONSTANT(ANIMATION_PROCESS_MANUAL);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_dir", PROPERTY_HINT_DIR), "set_base_dir", "get_base_dir");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "document", PROPERTY_HINT_RESOURCE_TYPE, "HTMLDocument"), "set_document", "get_document");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "stylesheet", PROPERTY_HINT_MULTILINE_TEXT), "set_stylesheet", "get_stylesheet");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "selection_enabled"), "set_selection_enabled", "is_selection_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "context_menu_enabled"), "set_context_menu_enabled", "is_context_menu_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "animation_process_mode", PROPERTY_HINT_ENUM, "Idle,Physics,Manual"),
		"set_animation_process_mode", "get_animation_process_mode");
	ADD_GROUP("Caret", "caret_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "caret_blink"), "set_caret_blink_enabled", "is_caret_blink_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "caret_blink_interval", PROPERTY_HINT_RANGE, "0.1,10,0.01,suffix:s"),
		"set_caret_blink_interval", "get_caret_blink_interval");
	ADD_GROUP("Debug", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "css_diagnostics_enabled"), "set_css_diagnostics_enabled", "is_css_diagnostics_enabled");
	ADD_SIGNAL(MethodInfo("link_pressed", PropertyInfo(Variant::STRING, "uri")));
	ADD_SIGNAL(MethodInfo("element_clicked", PropertyInfo(Variant::OBJECT, "element", PROPERTY_HINT_NONE,
		String(), PROPERTY_USAGE_DEFAULT, HTMLElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("textarea_changed", PropertyInfo(Variant::OBJECT, "element", PROPERTY_HINT_NONE,
		"", PROPERTY_USAGE_DEFAULT, HTMLTextAreaElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("textarea_committed", PropertyInfo(Variant::OBJECT, "element", PROPERTY_HINT_NONE,
		"", PROPERTY_USAGE_DEFAULT, HTMLTextAreaElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("input_changed", PropertyInfo(Variant::OBJECT, "element", PROPERTY_HINT_NONE,
		"", PROPERTY_USAGE_DEFAULT, HTMLInputElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("input_committed", PropertyInfo(Variant::OBJECT, "element", PROPERTY_HINT_NONE,
		"", PROPERTY_USAGE_DEFAULT, HTMLInputElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("button_pressed", PropertyInfo(Variant::OBJECT, "element", PROPERTY_HINT_NONE,
		String(), PROPERTY_USAGE_DEFAULT, HTMLElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("layout_changed"));
	const PropertyInfo form_info(Variant::OBJECT, "form", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, HTMLFormElement::get_class_static());
	ADD_SIGNAL(MethodInfo("form_submitted", form_info, PropertyInfo(Variant::ARRAY, "entries"),
		PropertyInfo(Variant::OBJECT, "submitter", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, HTMLElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("form_validation_failed", form_info,
		PropertyInfo(Variant::OBJECT, "control", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, HTMLElement::get_class_static())));
	ADD_SIGNAL(MethodInfo("form_reset", form_info));
	ADD_SIGNAL(MethodInfo("select_changed",
		PropertyInfo(Variant::OBJECT, "element", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, HTMLSelectElement::get_class_static())));
}

void HTMLLabel::_validate_property(PropertyInfo& p_property) const {
	if (!caret_blink && p_property.name == StringName("caret_blink_interval")) {
		p_property.usage &= ~PROPERTY_USAGE_EDITOR;
	}
}

void HTMLLabel::_draw() {
	const Color background = has_theme_color("background_color", "HTMLLabel")
		? get_theme_color("background_color", "HTMLLabel") : Color(1, 1, 1);
	if (background.a > 0) {
		draw_rect(Rect2(Vector2(), get_size()), background);
	}
}

void HTMLLabel::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE:
		ensure_controls();
		queue_refresh();
		break;
	case NOTIFICATION_RESIZED:
		queue_redraw();
		if (canvas) {
			canvas->cancel_click();
		}
		queue_refresh();
		break;
	case NOTIFICATION_THEME_CHANGED:
		queue_redraw();
		viewport_scrollbar_theme_dirty = true;
		queue_refresh();
		break;
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		queue_refresh();
		break;
	case NOTIFICATION_OS_IME_UPDATE:
		if (canvas) canvas->on_ime_update();
		break;
	case NOTIFICATION_WM_WINDOW_FOCUS_IN:
		if (canvas) {
			canvas->caret_window_active = true;
			canvas->restart_caret_blink();
			canvas->update_ime();
		}
		break;
	case NOTIFICATION_INTERNAL_PROCESS:
		if (animation_process_mode == ANIMATION_PROCESS_IDLE && viewport_size.x > 0) {
			layout->advance_css_animations(get_process_delta_time());
			update_internal_processing();
		}
		if (canvas && canvas->ime_window) canvas->update_ime();
		break;
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS:
		if (animation_process_mode == ANIMATION_PROCESS_PHYSICS && viewport_size.x > 0) {
			layout->advance_css_animations(get_physics_process_delta_time());
			update_internal_processing();
		}
		break;
	case NOTIFICATION_FOCUS_ENTER:
		if (canvas && canvas->tab_entry_backward) {
			canvas->focus_tab_endpoint(*canvas->tab_entry_backward);
			canvas->clear_tab_entry();
		}
		break;
	case NOTIFICATION_FOCUS_EXIT:
		if (canvas) {
			const bool was_dragging = canvas->selection_dragged;
			canvas->cancel_press();
			canvas->set_focused_element({});
			if (was_dragging) {
				request_hover_check();
			}
		}
		break;
	case NOTIFICATION_VISIBILITY_CHANGED:
		if (is_visible_in_tree()) {
			gui_hover_dirty = true;
			request_hover_check();
		}
		else if (canvas) {
			cancel_viewport_scroll();
			cancel_viewport_pan_snap();
			layout->cancel_scroll_activity();
			canvas->cancel_press();
			canvas->set_focused_element({});
			hover_passes_remaining = 0;
			if (layout->update_hover(std::nullopt)) {
				queue_refresh();
			}
			queue_interaction();
		}
		break;
	case NOTIFICATION_EXIT_TREE:
		set_process_internal(false);
		set_physics_process_internal(false);
		dragging_viewport_scrollbar = {};
		cancel_viewport_scroll();
		cancel_viewport_pan_snap();
		layout->cancel_scroll_activity();
		close_select_menu();
		layout->update_hover(std::nullopt);
		hover_passes_remaining = 0;
		hover_reflow = false;
		view_update_pending = false;
		scroll_sort_pending = false;
		layout_signal_pending = false;
		reveal_focused_element = false;
		break;
	case NOTIFICATION_WM_WINDOW_FOCUS_OUT:
		close_select_menu();
		if (canvas) {
			canvas->cancel_press();
			canvas->caret_window_active = false;
			canvas->stop_caret_blink();
			canvas->close_ime();
		}
		break;
	}
}

void HTMLLabel::set_document(const Ref<HTMLDocument>& p_document) {
	if (document == p_document) {
		return;
	}
	if (document.is_valid()) {
		document->disconnect("changed", callable_mp(this, &HTMLLabel::document_changed).bind(document->get_instance_id()));
	}
	document = p_document;
	if (document.is_valid()) {
		DocRenderSourceEditorPlugin::observe_source_document(document);
		document->connect("changed", callable_mp(this, &HTMLLabel::document_changed).bind(document->get_instance_id()));
	}
	source_changed();
}

Ref<HTMLDocument> HTMLLabel::get_document() const {
	return document;
}

void HTMLLabel::set_text(const String& p_text) {
	if (text != p_text) {
		text = p_text;
		if (document.is_null()) {
			source_changed();
		}
	}
}

String HTMLLabel::get_text() const {
	return text;
}

void HTMLLabel::set_base_dir(const String& p_base_dir) {
	if (base_dir != p_base_dir) {
		base_dir = p_base_dir;
		if (document.is_null()) {
			source_changed();
		}
	}
}

String HTMLLabel::get_base_dir() const {
	return base_dir;
}

void HTMLLabel::set_stylesheet(const String& p_stylesheet) {
	if (stylesheet != p_stylesheet) {
		stylesheet = p_stylesheet;
		if (document.is_null()) {
			source_changed();
		}
	}
}

String HTMLLabel::get_stylesheet() const {
	return stylesheet;
}

Vector2i HTMLLabel::get_content_size() const {
	return content_size;
}

String HTMLLabel::get_render_error() const {
	return render_error;
}

void HTMLLabel::set_css_diagnostics_enabled(bool p_enabled) {
	if (layout->is_css_diagnostics_enabled() == p_enabled) {
		return;
	}
	layout->set_css_diagnostics_enabled(p_enabled);
	if (p_enabled) {
		queue_refresh();
	}
	else if (canvas) {
		canvas->queue_redraw();
		canvas->refresh_diagnostic_tooltip(std::nullopt);
	}
}

bool HTMLLabel::is_css_diagnostics_enabled() const {
	return layout->is_css_diagnostics_enabled();
}

TypedArray<Dictionary> HTMLLabel::get_css_diagnostics() const {
	return layout->get_css_diagnostics();
}

Vector2i HTMLLabel::get_scroll_offset() const {
	if (!scroll_container) return {};
	return Vector2i(scroll_container->get_h_scroll(), scroll_container->get_v_scroll()) * viewport_input_axes +
		hidden_scroll_offset;
}

void HTMLLabel::set_scroll_offset(const Vector2i& p_offset) {
	cancel_viewport_scroll();
	cancel_viewport_pan_snap();
	if (!is_inside_tree() || !scroll_container || refresh_queued || pending_scroll_offset) {
		pending_scroll_offset = p_offset;
		pending_scroll_requested = true;
		return;
	}
	const Vector2 limit = Vector2((content_size - viewport_size).max(Vector2i()));
	const auto snapped = layout->snap_viewport(viewport_size, {get_scroll_offset(), p_offset, limit, {1, 1},
		docrender::HTMLSnapMethod::destination});
	apply_scroll_offset(Vector2i(snapped.offset.round()));
}

bool HTMLLabel::scroll_to_anchor(const String& p_id, float p_align) {
	if (!is_inside_tree() || refresh_queued || viewport_size.x <= 0 || viewport_size.y <= 0) {
		return false;
	}
	const auto target = layout->scroll_anchor_into_view(p_id, p_align, *this,
		callable_mp(this, &HTMLLabel::on_nested_scroll_advanced));
	if (!target) return false;
	apply_scroll_target(*target, p_align);
	request_hover_check();
	return true;
}

Ref<HTMLElement> HTMLLabel::create_element(const String& p_tag_name) {
	parse_pending_source();
	return layout->create_element(p_tag_name);
}

Ref<HTMLDocumentFragment> HTMLLabel::create_document_fragment() {
	parse_pending_source();
	return layout->create_document_fragment();
}

Ref<HTMLElement> HTMLLabel::query_selector(const String& p_selector) const {
	return source_dirty ? Ref<HTMLElement>() : layout->query_selector(p_selector);
}

TypedArray<HTMLElement> HTMLLabel::query_selector_all(const String& p_selector) const {
	return source_dirty ? TypedArray<HTMLElement>() : layout->query_selector_all(p_selector);
}

Ref<HTMLElement> HTMLLabel::get_element_by_id(const String& p_id) const {
	return source_dirty ? Ref<HTMLElement>() : layout->get_element_by_id(p_id);
}

Ref<HTMLElement> HTMLLabel::get_element_at(const Vector2& p_position) const {
	if (!is_inside_tree() || refresh_queued || !p_position.is_finite() || !Rect2(Vector2(), get_size()).has_point(p_position)) {
		return {};
	}
	const Vector2 local = (canvas->get_global_transform().affine_inverse() * get_global_transform()).xform(p_position);
	return get_visible_rect().has_point(local) ? layout->get_element_at(local) : Ref<HTMLElement>();
}

Ref<HTMLElement> HTMLLabel::get_focused_element() const {
	return canvas && has_focus() && layout->is_element_connected(canvas->focused_element)
		? canvas->focused_element : Ref<HTMLElement>();
}

bool HTMLLabel::focus_element(const Ref<HTMLElement>& p_element) {
	if (!canvas || !is_visible_in_tree() || !layout->is_focusable(p_element)) return false;
	grab_focus();
	if (!has_focus()) return false;
	canvas->set_focused_element(p_element, true);
	return canvas->focused_element == p_element;
}

bool HTMLLabel::activate_element(const Ref<HTMLElement>& p_element) {
	if (source_dirty || !is_visible_in_tree()) return false;
	const String uri = layout->get_activatable_link_uri(p_element);
	if (!uri.is_empty()) {
		activate_link(uri);
		return true;
	}
	return activate_form_control(p_element);
}

void HTMLLabel::activate_link(const String& p_uri) {
	if (p_uri.begins_with("#")) {
		scroll_to_anchor(docrender::decode_html_fragment(p_uri.substr(1)));
	}
	emit_signal("link_pressed", p_uri);
}

bool HTMLLabel::scroll_to_element(const Ref<HTMLElement>& p_element, float p_align) {
	if (!is_inside_tree() || refresh_queued || viewport_size.x <= 0 || viewport_size.y <= 0) {
		return false;
	}
	const auto target = layout->scroll_element_into_view(p_element, p_align, *this,
		callable_mp(this, &HTMLLabel::on_nested_scroll_advanced));
	if (!target) return false;
	apply_scroll_target(*target, p_align);
	request_hover_check();
	return true;
}

void HTMLLabel::apply_scroll_target(const docrender::HTMLScrollTarget& p_target, std::optional<float> p_align) {
	if (p_target.viewport_fixed) return;
	cancel_viewport_pan_snap();
	const Vector2i current = get_scroll_offset();
	const Rect2 region = layout->viewport_scroll_region(Rect2(current, viewport_size));
	const Vector2 offset = Vector2(current) + docrender::html_scroll_delta(p_target.area, region, p_align);
	const Vector2i max_offset = (content_size - viewport_size).max(Vector2i());
	const auto rounded = p_align ? offset : Vector2(
		offset.x < current.x ? std::floor(offset.x) : std::ceil(offset.x),
		offset.y < current.y ? std::floor(offset.y) : std::ceil(offset.y));
	const auto snap = layout->snap_viewport(viewport_size, {current, rounded, max_offset, {1, 1},
		docrender::HTMLSnapMethod::destination, {p_target.element, p_target.element}});
	const Vector2i destination(layout->viewport_snap_axes() == Vector2i() ? snap.offset : snap.offset.round());
	cancel_viewport_scroll();
	if (destination == current) return;
	if (!layout->viewport_uses_smooth_scroll()) {
		apply_scroll_offset(destination);
		return;
	}
	const auto update = callable_mp(this, &HTMLLabel::advance_viewport_scroll).bind(Vector2(current), Vector2(destination));
	viewport_scroll_tween = HTMLLayout::start_scroll_tween(*this, update);
}

void HTMLLabel::advance_viewport_scroll(double p_fraction, Vector2 p_from, Vector2 p_to) {
	applying_scroll_tween = true;
	apply_scroll_offset(Vector2i(p_from.lerp(p_to, p_fraction).round()));
	applying_scroll_tween = false;
	if (p_fraction >= 1.0) viewport_scroll_tween.unref();
}

void HTMLLabel::cancel_viewport_scroll() {
	if (viewport_scroll_tween.is_valid()) {
		viewport_scroll_tween->kill();
		viewport_scroll_tween.unref();
	}
}

void HTMLLabel::on_nested_scroll_advanced() {
	canvas->cancel_click();
	request_hover_check();
}

void HTMLLabel::set_selection_enabled(bool p_enabled) {
	selection_enabled = p_enabled;
	if (canvas) {
		canvas->update_focus_mode();
		if (!p_enabled) {
			canvas->cancel_press();
			clear_selection();
		}
		request_hover_check();
	}
}

bool HTMLLabel::is_selection_enabled() const {
	return selection_enabled;
}

void HTMLLabel::set_context_menu_enabled(bool p_enabled) {
	context_menu_enabled = p_enabled;
	if (!p_enabled && canvas) canvas->close_input_menu();
}

bool HTMLLabel::is_context_menu_enabled() const {
	return context_menu_enabled;
}

void HTMLLabel::set_animation_process_mode(AnimationProcessMode p_mode) {
	ERR_FAIL_COND(p_mode < ANIMATION_PROCESS_IDLE || p_mode > ANIMATION_PROCESS_MANUAL);
	if (animation_process_mode == p_mode) return;
	animation_process_mode = p_mode;
	update_internal_processing();
}

HTMLLabel::AnimationProcessMode HTMLLabel::get_animation_process_mode() const {
	return animation_process_mode;
}

void HTMLLabel::advance_animation(double p_delta) {
	ERR_FAIL_COND_MSG(!std::isfinite(p_delta) || p_delta < 0.0, "Animation delta must be finite and non-negative.");
	if (!is_inside_tree()) return;
	// Commit pending style changes at the current time before moving the clock.
	flush_queued_refresh();
	if (viewport_size.x <= 0) return;
	layout->advance_css_animations(p_delta);
	flush_queued_refresh();
	update_internal_processing();
}

void HTMLLabel::set_caret_blink_enabled(bool p_enabled) {
	if (caret_blink == p_enabled) return;
	caret_blink = p_enabled;
	if (canvas) canvas->restart_caret_blink();
	notify_property_list_changed();
}

bool HTMLLabel::is_caret_blink_enabled() const {
	return caret_blink;
}

void HTMLLabel::set_caret_blink_interval(double p_interval) {
	ERR_FAIL_COND_MSG(!std::isfinite(p_interval) || p_interval <= 0.0, "Caret blink interval must be a finite positive number.");
	if (caret_blink_interval == p_interval) return;
	caret_blink_interval = p_interval;
	if (canvas && caret_blink) canvas->restart_caret_blink();
}

double HTMLLabel::get_caret_blink_interval() const {
	return caret_blink_interval;
}

void HTMLLabel::select_all() {
	if (selection_enabled && canvas) {
		canvas->selection_anchor = 0;
		canvas->selection_caret = layout->get_plain_text().length();
		canvas->queue_redraw();
		callable_mp(layout.ptr(), &HTMLLayout::request_auto_content_update).call_deferred();
	}
}

void HTMLLabel::clear_selection() {
	if (canvas) {
		const bool changed = canvas->selection_anchor != canvas->selection_caret;
		canvas->selection_anchor = canvas->selection_caret = 0;
		canvas->queue_redraw();
		if (changed) callable_mp(layout.ptr(), &HTMLLayout::request_auto_content_update).call_deferred();
	}
}

String HTMLLabel::get_selected_text() const {
	if (!canvas) {
		return {};
	}
	const int64_t from = std::min(canvas->selection_anchor, canvas->selection_caret);
	const int64_t to = std::max(canvas->selection_anchor, canvas->selection_caret);
	return layout->get_copyable_text(from, to);
}

PackedStringArray HTMLLabel::_get_configuration_warnings() const {
	PackedStringArray warnings;
	if (!render_error.is_empty()) {
		warnings.append(render_error);
	}
	return warnings;
}

void HTMLLabel::ensure_controls() {
	if (scroll_container) {
		return;
	}

	scroll_container = memnew(ScrollContainer);
	scroll_container->set_name("_HTMLScroll");
	scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_NEVER);
	scroll_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_NEVER);
	scroll_container->set_deadzone(DRAG_DEADZONE);
	// Keep the visible rectangle aligned with ScrollContainer's scrollbar page sizes.
	scroll_container->add_theme_constant_override("scrollbar_h_separation", 0);
	scroll_container->add_theme_constant_override("scrollbar_v_separation", 0);
	add_child(scroll_container, false, INTERNAL_MODE_BACK);
	scroll_container->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	scroll_container->connect("resized", callable_mp(this, &HTMLLabel::queue_refresh));
	scroll_container->connect("sort_children", callable_mp(this, &HTMLLabel::on_content_sorted));
	scroll_container->connect("draw", callable_mp(this, &HTMLLabel::draw_viewport_gutters));
	scroll_container->connect("gui_input", callable_mp(this, &HTMLLabel::on_viewport_gui_input));
	scroll_container->connect("scroll_ended", callable_mp(this, &HTMLLabel::settle_viewport_snap).bind(Vector2i(1, 1)));
	scroll_container->get_h_scroll_bar()->connect("gui_input", callable_mp(this, &HTMLLabel::on_viewport_scrollbar_input).bind(0));
	scroll_container->get_v_scroll_bar()->connect("gui_input", callable_mp(this, &HTMLLabel::on_viewport_scrollbar_input).bind(1));
	scroll_container->get_h_scroll_bar()->connect("value_changed", callable_mp(this, &HTMLLabel::on_scroll_changed));
	scroll_container->get_v_scroll_bar()->connect("value_changed", callable_mp(this, &HTMLLabel::on_scroll_changed));

	// The native range represents direct scrolling. Hidden axes can still move the canvas through navigation.
	scroll_content = memnew(Control);
	scroll_content->set_name("_HTMLContent");
	scroll_content->set_mouse_filter(MOUSE_FILTER_IGNORE);
	scroll_content->set_h_size_flags(SIZE_EXPAND_FILL);
	scroll_content->set_v_size_flags(SIZE_EXPAND_FILL);
	scroll_container->add_child(scroll_content, false, INTERNAL_MODE_BACK);

	canvas = memnew(HTMLLabelCanvas);
	canvas->set_name("_HTMLCanvas");
	canvas->label = this;
	canvas->set_mouse_filter(MOUSE_FILTER_PASS);
	canvas->update_focus_mode();
	scroll_content->add_child(canvas, false, INTERNAL_MODE_BACK);
}

void HTMLLabel::queue_refresh() {
	hover_reflow = false;
	hover_passes_remaining = HOVER_PASSES;
	if (canvas && is_inside_tree() && !refresh_queued) {
		refresh_queued = true;
		callable_mp(this, &HTMLLabel::flush_queued_refresh).call_deferred();
	}
}

void HTMLLabel::flush_queued_refresh() {
	if (refresh_queued) refresh();
}

void HTMLLabel::queue_interaction_style_refresh() {
	interaction_styles_dirty = true;
	queue_refresh();
}

void HTMLLabel::source_changed() {
	cancel_viewport_scroll();
	cancel_viewport_pan_snap();
	close_select_menu();
	source_dirty = true;
	layout->clear();
	update_internal_processing();
	pending_scroll_offset = Vector2i();
	pending_scroll_requested = false;
	if (canvas) {
		canvas->refresh_diagnostic_tooltip(std::nullopt);
		canvas->cancel_press();
		canvas->set_focused_element({});
		canvas->tab_elements.clear();
		canvas->has_focusable_elements = false;
		canvas->update_focus_mode();
		clear_selection();
	}
	queue_refresh();
}

void HTMLLabel::document_changed(uint64_t p_document_id) {
	if (document.is_valid() && document->get_instance_id() == p_document_id) {
		source_changed();
	}
}

void HTMLLabel::parse_pending_source() {
	if (!source_dirty) {
		return;
	}
	source_dirty = false;
	layout_updated = true;
	if (document.is_valid()) {
		document->parse_into(*layout.ptr());
		parsed = layout->get_render_error().is_empty();
	}
	else {
		parsed = layout->parse_source(text, base_dir, stylesheet, Dictionary(), false) == OK;
	}
	layout->set_edit_callback(callable_mp(this, &HTMLLabel::on_layout_edited));
	layout->set_transition_callback(callable_mp(this, &HTMLLabel::on_transition_frame));
	layout->set_content_visibility_callbacks(callable_mp(this, &HTMLLabel::on_auto_content_update),
		callable_mp(this, &HTMLLabel::get_content_selection_range));
}

void HTMLLabel::on_auto_content_update() {
	layout_updated = true;
	queue_refresh();
}

void HTMLLabel::on_transition_frame(bool p_size_changed) {
	if (p_size_changed) queue_refresh();
	request_hover_check();
}

void HTMLLabel::update_internal_processing() {
	const bool in_tree = is_inside_tree();
	const bool animate = in_tree && viewport_size.x > 0 && layout->has_active_css_animations();
	const bool track_ime = in_tree && canvas && canvas->ime_window.has_value();
	set_process_internal(track_ime || (animate && animation_process_mode == ANIMATION_PROCESS_IDLE));
	set_physics_process_internal(animate && animation_process_mode == ANIMATION_PROCESS_PHYSICS);
}

Vector2i HTMLLabel::get_content_selection_range() const {
	if (!canvas) return {};
	return Vector2i(std::min(canvas->selection_anchor, canvas->selection_caret), std::max(canvas->selection_anchor, canvas->selection_caret));
}

void HTMLLabel::on_layout_edited() {
	close_select_menu();
	layout_updated = true;
	if (canvas) {
		canvas->close_input_menu();
		// Writing the size during a resize schedules reflow while the handle is held.
		if (!canvas->applying_textarea_resize) {
			canvas->cancel_press();
		}
		clear_selection();
		if (canvas->focused_element.is_valid() && !layout->is_element_connected(canvas->focused_element)) {
			canvas->set_focused_element({});
		}
		else if (const auto input = layout->text_control(canvas->focused_element);
			input && input->editor.get_reset_version() != canvas->focused_reset_version) {
			canvas->focused_reset_version = input->editor.get_reset_version();
			canvas->close_ime();
			canvas->update_ime();
		}
		auto& tabs = canvas->tab_elements;
		tabs.erase(std::remove_if(tabs.begin(), tabs.end(),
			[this](const auto& p_element) { return !layout->is_element_connected(p_element); }), tabs.end());
	}
	queue_refresh();
}

void HTMLLabel::update_viewport_scrollbar_style(const docrender::HTMLScrollbarAppearance& p_style) {
	const bool theme_dirty = std::exchange(viewport_scrollbar_theme_dirty, false);
	if (!theme_dirty && viewport_scrollbar_style == p_style) return;

	viewport_scrollbar_style = p_style;
	const float thickness = p_style.thickness();
	// All three grabber styleboxes have identical margins. Only the paint rectangle changes.
	static constexpr const char* icons[] = { "increment", "increment_highlight", "increment_pressed",
		"decrement", "decrement_highlight", "decrement_pressed" };
	Ref<ImageTexture> empty;
	empty.instantiate();
	for (const bool vertical : { false, true }) {
		ScrollBar* bar = vertical ? static_cast<ScrollBar*>(scroll_container->get_v_scroll_bar())
			: static_cast<ScrollBar*>(scroll_container->get_h_scroll_bar());
		const Side inner = vertical ? SIDE_LEFT : SIDE_TOP;
		const Side outer = vertical ? SIDE_RIGHT : SIDE_BOTTOM;
		const Side start = vertical ? SIDE_TOP : SIDE_LEFT;
		const Side end = vertical ? SIDE_BOTTOM : SIDE_RIGHT;
		const auto make_thumb = [&](bool p_hot, bool p_pressed) {
			const int axis = vertical ? 1 : 0;
			const Rect2 lane(0, 0, vertical ? thickness : 17, vertical ? 17 : thickness);
			const Rect2 thumb = p_style.visible_thumb(lane, axis, p_hot);
			Ref<StyleBoxFlat> style;
			style.instantiate();
			style->set_bg_color(p_style.thumb_color(p_pressed));
			style->set_content_margin(start, 8.0f);
			style->set_content_margin(end, 9.0f);
			style->set_expand_margin(inner, -thumb.position[1 - axis]);
			style->set_expand_margin(outer, thumb.get_end()[1 - axis] - thickness);
			style->set_corner_radius_all(Math::ceil(thumb.size[1 - axis] * 0.5f));
			return style;
		};
		Ref<StyleBoxFlat> track;
		track.instantiate();
		track->set_bg_color(p_style.colors.value_or(docrender::HTMLScrollbarColors{}).track);
		track->set_content_margin(inner, thickness * 0.5f);
		track->set_content_margin(outer, thickness * 0.5f);

		bar->begin_bulk_theme_override();
		bar->add_theme_stylebox_override("scroll", track);
		bar->add_theme_stylebox_override("scroll_focus", track);
		bar->add_theme_stylebox_override("grabber", make_thumb(false, false));
		bar->add_theme_stylebox_override("grabber_highlight", make_thumb(true, false));
		bar->add_theme_stylebox_override("grabber_pressed", make_thumb(true, true));
		for (const char* icon : icons) bar->add_theme_icon_override(icon, empty);
		for (const char* padding : { "padding_left", "padding_right", "padding_top", "padding_bottom" }) {
			bar->add_theme_constant_override(padding, 0);
		}
		bar->end_bulk_theme_override();
	}
}

void HTMLLabel::update_viewport_gutters(const Vector2& p_gutters) {
	if (viewport_panel_gutters == p_gutters) return;
	viewport_panel_gutters = p_gutters;
	if (p_gutters == Vector2() || viewport_panel.is_null()) {
		scroll_container->remove_theme_stylebox_override("panel");
		return;
	}
	const Ref<StyleBox> panel(viewport_panel->duplicate());
	panel->set_content_margin(SIDE_LEFT, viewport_panel->get_margin(SIDE_LEFT) + p_gutters.x);
	panel->set_content_margin(SIDE_RIGHT, viewport_panel->get_margin(SIDE_RIGHT) + p_gutters.y);
	scroll_container->add_theme_stylebox_override("panel", panel);
}

void HTMLLabel::draw_viewport_gutters() {
	if (viewport_painted_gutters == Vector2() || viewport_size == Vector2i()) return;
	const Vector2 translation = scroll_content->get_position() + canvas->get_position();
	const Vector2 origin = translation + Vector2(sorted_scroll_offset);
	layout->draw_viewport_background(*scroll_container, translation,
		Rect2(origin - Vector2(viewport_painted_gutters.x, 0), Vector2(viewport_painted_gutters.x, viewport_size.y)),
		sorted_scroll_offset);
	layout->draw_viewport_background(*scroll_container, translation,
		Rect2(origin + Vector2(viewport_size.x, 0), Vector2(viewport_painted_gutters.y, viewport_size.y)),
		sorted_scroll_offset);
}

void HTMLLabel::refresh() {
	refresh_queued = false;
	if (!is_inside_tree()) {
		return;
	}
	if (!hover_reflow && !interaction_styles_dirty) {
		canvas->cancel_click();
	}
	if (!pending_scroll_offset) {
		pending_scroll_offset = get_scroll_offset();
	}

	parse_pending_source();
	const bool button_style_changed = std::exchange(interaction_styles_dirty, false);
	if (button_style_changed && parsed) {
		layout->mark_styles_pending();
	}
	const bool layout_changed = layout_updated || hover_reflow || button_style_changed;
	layout_updated = false;
	if (viewport_panel.is_null() || viewport_scrollbar_theme_dirty) {
		update_viewport_gutters({});
		viewport_panel = scroll_container->get_theme_stylebox("panel");
	}
	if (!parsed) {
		update_viewport_scrollbar_style({});
		update_view({}, {}, {}, {1, 1}, layout_changed, layout->get_render_error());
		return;
	}

	using namespace litehtml;
	const Vector2 available = scroll_container->get_size() -
		(viewport_panel.is_valid() ? viewport_panel->get_minimum_size() : Vector2());
	const Vector2i media_size(available.floor());
	if (media_size.x <= 0 || media_size.y <= 0) {
		update_view({}, {}, {}, {1, 1}, layout_changed);
		return;
	}
	if (layout->layout(media_size) != OK) {
		update_view({}, {}, {}, {1, 1}, layout_changed, layout->get_render_error());
		return;
	}

	const auto style = layout->get_viewport_style();
	update_viewport_scrollbar_style(style.scrollbar);
	const bool show_bars = style.scrollbar.width != scrollbar_width::none;
	const real_t bar_width = show_bars ? scroll_container->get_v_scroll_bar()->get_combined_minimum_size().x : 0;
	const real_t bar_height = show_bars ? scroll_container->get_h_scroll_bar()->get_combined_minimum_size().y : 0;
	const bool reserve = style.gutter != scrollbar_gutter::automatic;
	const real_t opposite_gutter = style.gutter == scrollbar_gutter::both_edges ? bar_width : 0;
	const Vector2 unconditional_gutters(
		(reserve || style.root_overflow[1] == overflow_scroll ? bar_width : 0) + opposite_gutter,
		style.root_overflow[0] == overflow_scroll ? bar_height : 0);
	const Vector2i unit_size((available - unconditional_gutters).floor());
	const Vector2i input_axes(is_interactive_overflow(style.overflow[0]), is_interactive_overflow(style.overflow[1]));
	const auto visible_bar = [show_bars](overflow p_mode, bool p_overflowing) {
		return show_bars && (p_mode == overflow_scroll || (p_mode == overflow_auto && p_overflowing));
	};
	const auto client_size = [&](bool p_horizontal, bool p_vertical) {
		const Vector2 gutters((reserve || p_vertical ? bar_width : 0) + opposite_gutter, p_horizontal ? bar_height : 0);
		return Vector2i((available - gutters).floor());
	};
	bool horizontal = visible_bar(style.overflow[0], false);
	bool vertical = visible_bar(style.overflow[1], false);
	Vector2i viewport = client_size(horizontal, vertical);

	// Measure with unconditional gutters before testing for bars that depend on overflowing content.
	for (;;) {
		if (viewport.x <= 0 || viewport.y <= 0) {
			update_view({}, {}, {}, {1, 1}, layout_changed);
			return;
		}
		if (layout->layout_viewport(viewport, media_size, unit_size) != OK) {
			update_view({}, {}, {}, {1, 1}, layout_changed, layout->get_render_error());
			return;
		}
		const Vector2i size = layout->get_content_size();
		horizontal = visible_bar(style.overflow[0], horizontal || size.x > viewport.x);
		vertical = visible_bar(style.overflow[1], vertical || size.y > viewport.y);
		const Vector2i next_viewport = client_size(horizontal, vertical);
		if (next_viewport == viewport) {
			const real_t empty_gutter = reserve && !vertical ? bar_width : 0;
			const Vector2 empty_gutters = scroll_container->is_layout_rtl() ?
				Vector2(empty_gutter, opposite_gutter) : Vector2(opposite_gutter, empty_gutter);
			Vector2 painted_gutters = empty_gutters;
			if (vertical) painted_gutters[scroll_container->is_layout_rtl() ? 0 : 1] = bar_width;
			update_view(viewport, size, {horizontal, vertical}, input_axes, layout_changed, {}, empty_gutters, painted_gutters);
			return;
		}
		viewport = next_viewport;
	}
}

void HTMLLabel::update_view(const Vector2i& p_viewport, const Vector2i& p_content,
	const Vector2i& p_visible_bars, const Vector2i& p_input_axes, bool p_layout_changed, const String& p_error,
	const Vector2& p_gutters, const Vector2& p_painted_gutters) {
	view_update_pending = true;
	if (p_viewport == Vector2i()) {
		layout->update_hover(std::nullopt);
		hover_passes_remaining = 0;
		canvas->tab_elements.clear();
		canvas->has_focusable_elements = false;
		canvas->cancel_press();
		canvas->set_focused_element({});
	}
	else {
		canvas->tab_elements = layout->get_tab_elements(canvas->has_focusable_elements);
		if (canvas->focused_element.is_valid() && !layout->is_focusable(canvas->focused_element)) {
			canvas->set_focused_element({});
		}
		if (canvas->pressed_button.is_valid() && !layout->is_enabled_button(canvas->pressed_button)) {
			canvas->cancel_button_press();
		}
		if (canvas->textarea_drag && !layout->get_element_scroll_bounds(canvas->textarea_drag->element)) {
			canvas->cancel_press();
		}
	}
	canvas->update_focus_mode();
	layout_signal_pending |= p_layout_changed || viewport_size != p_viewport ||
		content_size != p_content || render_error != p_error;
	viewport_size = p_viewport;
	update_internal_processing();
	content_size = p_content;
	viewport_input_axes = p_input_axes;
	hidden_scroll_offset = hidden_scroll_offset * (Vector2i(1, 1) - viewport_input_axes);
	const int64_t text_length = layout->get_plain_text().length();
	canvas->selection_anchor = std::min(canvas->selection_anchor, text_length);
	canvas->selection_caret = std::min(canvas->selection_caret, text_length);
	canvas->set_size(Vector2(content_size.max(viewport_size)));
	scroll_content->set_custom_minimum_size(Vector2(content_size * viewport_input_axes));
	update_viewport_gutters(p_gutters);
	if (viewport_painted_gutters != p_painted_gutters) scroll_container->queue_redraw();
	viewport_painted_gutters = p_painted_gutters;
	layout->set_viewport_gutters(p_painted_gutters);
	scroll_container->set_horizontal_scroll_mode(p_visible_bars.x ? ScrollContainer::SCROLL_MODE_SHOW_ALWAYS : ScrollContainer::SCROLL_MODE_SHOW_NEVER);
	scroll_container->set_vertical_scroll_mode(p_visible_bars.y ? ScrollContainer::SCROLL_MODE_SHOW_ALWAYS : ScrollContainer::SCROLL_MODE_SHOW_NEVER);
	// ScrollContainer measures its child in get_minimum_size(), even when its own minimum stays unchanged.
	static_cast<void>(scroll_container->get_minimum_size());
	scroll_container->queue_sort();
	// Apply the requested offset after the native container updates its scrollbar ranges.
	callable_mp(this, &HTMLLabel::apply_pending_scroll).call_deferred();
	if (render_error != p_error) {
		render_error = p_error;
		update_configuration_warnings();
	}
}

Rect2 HTMLLabel::get_visible_rect() const {
	// Scrollbar values may change before the container moves the content canvas.
	return Rect2(Vector2(sorted_scroll_offset), Vector2(viewport_size));
}

void HTMLLabel::apply_scroll_offset(const Vector2i& p_offset) {
	const Vector2i max_offset = (content_size - viewport_size).max(Vector2i());
	const Vector2i offset = p_offset.clamp(Vector2i(), max_offset);
	const Vector2i hidden = offset * (Vector2i(1, 1) - viewport_input_axes);
	if (hidden_scroll_offset != hidden) {
		hidden_scroll_offset = hidden;
		on_scroll_changed(0);
		scroll_container->queue_sort();
	}
	const Vector2i native = offset * viewport_input_axes;
	scroll_container->set_h_scroll(native.x);
	scroll_container->set_v_scroll(native.y);
}

void HTMLLabel::apply_pending_scroll() {
	if (!is_inside_tree() || refresh_queued) {
		return;
	}
	if (pending_scroll_offset) {
		Vector2i offset = *pending_scroll_offset;
		pending_scroll_offset.reset();
		const bool requested = std::exchange(pending_scroll_requested, false);
		if (requested || (viewport_scroll_tween.is_null() && viewport_pan_snap_timer.is_null() && dragging_viewport_scrollbar == Vector2i())) {
			const Vector2 limit = Vector2((content_size - viewport_size).max(Vector2i()));
			const auto method = requested ? docrender::HTMLSnapMethod::destination : docrender::HTMLSnapMethod::relayout;
			offset = Vector2i(layout->snap_viewport(viewport_size, {get_scroll_offset(), offset, limit, {1, 1}, method}).offset.round());
		}
		apply_scroll_offset(offset);
	}
	view_update_pending = false;
	queue_interaction();
}

void HTMLLabel::on_scroll_changed(double) {
	if (!applying_scroll_tween) cancel_viewport_scroll();
	canvas->cancel_click();
	canvas->queue_redraw();
	scroll_sort_pending = true;
	if (!hover_reflow) {
		hover_passes_remaining = HOVER_PASSES;
	}
}

void HTMLLabel::on_viewport_gui_input(const Ref<InputEvent>& p_event) {
	const Vector2 steps = HTMLLayout::scroll_input_steps(p_event);
	if (steps == Vector2()) return;
	cancel_viewport_scroll();
	const Ref<InputEventPanGesture> pan = p_event;
	if (pan.is_null()) cancel_viewport_pan_snap();
	const Vector2 contained = layout->viewport_contained_scroll(steps);
	const Vector2i snap_axes = layout->viewport_snap_axes() * viewport_input_axes;
	const Vector2i handled_axes(
		contained.x != 0 || (snap_axes.x && steps.x != 0),
		contained.y != 0 || (snap_axes.y && steps.y != 0));
	if (handled_axes == Vector2i()) return;

	// Match Godot's wheel and pan step before forwarding any axis that can chain.
	constexpr double SCROLL_PAGE_DIVISOR = 8.0;
	const Vector2 pages(scroll_container->get_h_scroll_bar()->get_page(), scroll_container->get_v_scroll_bar()->get_page());
	const Vector2 current = get_scroll_offset();
	const Vector2 limit = Vector2((content_size - viewport_size).max(Vector2i()));
	const Vector2 natural = current + pages * steps * Vector2(handled_axes * viewport_input_axes) / SCROLL_PAGE_DIVISOR;
	const auto snap = pan.is_valid() ?
		docrender::HTMLSnapResult{natural.clamp(Vector2(), limit), {}} :
		layout->snap_viewport(viewport_size, {current, natural, limit, handled_axes * viewport_input_axes, docrender::HTMLSnapMethod::direction});
	apply_scroll_offset(Vector2i(snap.offset.round()));
	const Vector2 applied = get_scroll_offset();
	Vector2 remaining = steps;
	for (int axis = 0; axis < 2; ++axis) {
		if (contained[axis] != 0 || snap.stopped[axis] || applied[axis] != current[axis]) remaining[axis] = 0;
	}
	const Vector2i pan_axes(snap_axes.x && applied.x != current.x, snap_axes.y && applied.y != current.y);
	if (pan.is_valid() && pan_axes != Vector2i()) {
		viewport_pan_axes = viewport_pan_axes.max(pan_axes);
		if (viewport_pan_snap_timer.is_valid()) viewport_pan_snap_timer->kill();
		viewport_pan_snap_timer = HTMLLayout::start_snap_timer(*this, callable_mp(this, &HTMLLabel::settle_viewport_pan));
	}
	if (!HTMLLayout::forward_scroll_remainder(p_event, remaining)) scroll_container->accept_event();
}

void HTMLLabel::on_viewport_scrollbar_input(const Ref<InputEvent>& p_event, int p_axis) {
	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_null() || button->get_button_index() != MOUSE_BUTTON_LEFT) {
		if (layout->viewport_snap_axes() != Vector2i()) on_viewport_gui_input(p_event);
		return;
	}
	dragging_viewport_scrollbar[p_axis] = button->is_pressed();
	if (button->is_pressed()) {
		cancel_viewport_scroll();
		cancel_viewport_pan_snap();
	}
	else if (!button->is_canceled()) {
		Vector2i axes;
		axes[p_axis] = 1;
		callable_mp(this, &HTMLLabel::settle_viewport_snap).call_deferred(axes);
	}
}

void HTMLLabel::settle_viewport_pan() {
	viewport_pan_snap_timer.unref();
	settle_viewport_snap(std::exchange(viewport_pan_axes, Vector2i()));
}

void HTMLLabel::cancel_viewport_pan_snap() {
	if (viewport_pan_snap_timer.is_valid()) viewport_pan_snap_timer->kill();
	viewport_pan_snap_timer.unref();
	viewport_pan_axes = {};
}

void HTMLLabel::settle_viewport_snap(const Vector2i& p_axes) {
	const Vector2i axes = p_axes * viewport_input_axes;
	if (!is_inside_tree() || layout->viewport_snap_axes() * axes == Vector2i()) return;
	const Vector2 current = get_scroll_offset();
	const Vector2 limit = Vector2((content_size - viewport_size).max(Vector2i()));
	const auto snap = layout->snap_viewport(viewport_size, {current, current, limit, axes,
		docrender::HTMLSnapMethod::destination});
	apply_scroll_offset(Vector2i(snap.offset.round()));
}

void HTMLLabel::on_content_sorted() {
	canvas->set_position(-Vector2(hidden_scroll_offset));
	sorted_scroll_offset = get_scroll_offset();
	layout->set_viewport_scroll_offset(sorted_scroll_offset);
	scroll_sort_pending = false;
	gui_hover_dirty = true;
	if (viewport_painted_gutters != Vector2()) scroll_container->queue_redraw();
	canvas->queue_redraw();
	queue_interaction();
}

void HTMLLabel::request_hover_check() {
	hover_passes_remaining = HOVER_PASSES;
	queue_interaction();
}

void HTMLLabel::queue_interaction() {
	if (canvas && is_inside_tree() && !interaction_queued) {
		interaction_queued = true;
		callable_mp(this, &HTMLLabel::update_interaction).call_deferred();
	}
}

void HTMLLabel::update_interaction() {
	interaction_queued = false;
	if (!is_inside_tree() || refresh_queued || view_update_pending || scroll_sort_pending) {
		return;
	}
	update_select_menu();

	if (std::exchange(reveal_focused_element, false)) {
		if (const auto target = layout->scroll_element_into_view(canvas->focused_element, std::nullopt, *this,
				callable_mp(this, &HTMLLabel::on_nested_scroll_advanced))) {
			apply_scroll_target(*target, std::nullopt);
			if (scroll_sort_pending) {
				return;
			}
		}
	}

	std::optional<Vector2> point;
	if (is_visible_in_tree() && viewport_size != Vector2i()) {
		Viewport* viewport = get_viewport();
		if (gui_hover_dirty) {
			gui_hover_dirty = false;
			// Refresh Godot's hovered Control after moving the canvas or its scrollbars.
			updating_gui_hover = true;
			viewport->update_mouse_cursor_state();
			updating_gui_hover = false;
			if (!is_inside_tree() || refresh_queued || scroll_sort_pending) {
				return;
			}
		}
		const Vector2 mouse = canvas->get_local_mouse_position();
		if (viewport->gui_get_hovered_control() == canvas && get_visible_rect().has_point(mouse)) {
			point = mouse;
		}
	}

	layout->update_scrollbar_hover(point);
	if (!canvas->selection_dragged && !canvas->scrollbar_drag && hover_passes_remaining > 0 && layout->update_hover(point)) {
		// A hover rule can move or hide its own target. Allow one correction after sorting.
		--hover_passes_remaining;
		hover_reflow = true;
		refresh();
		return;
	}

	hover_passes_remaining = 0;
	hover_reflow = false;
	canvas->update_cursor(point);
	canvas->refresh_diagnostic_tooltip(point);
	if (layout_signal_pending && !refresh_queued) {
		layout_signal_pending = false;
		emit_signal("layout_changed");
	}
}
