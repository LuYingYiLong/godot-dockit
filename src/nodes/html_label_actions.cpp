#include "html_label.h"

#include "formats/html/dom/html_dom.h"
#include "formats/html/elements/html_form_elements.h"
#include "formats/html/elements/html_input_node.h"
#include "formats/html/html_layout.h"
#include "formats/html/text/html_text_selection.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/object_id.hpp>

#include <algorithm>
#include <iterator>
#include <utility>

using namespace godot;

using docrender::HTMLInputNode;

namespace {
	constexpr int DRAG_DEADZONE = 6;

	std::optional<bool> tab_direction(const InputEventKey& p_key) {
		if (p_key.is_action_pressed("ui_focus_prev", true, true)) return true;
		if (p_key.is_action_pressed("ui_focus_next", true, true)) return false;
		return std::nullopt;
	}
}

void HTMLLabelCanvas::accept_pointer_event(const PointerInfo& p_info) {
	if (p_info.accepts_event) accept_event();
}

void HTMLLabelCanvas::handle_pointer_move(const PointerInfo& p_info) {
	if (!label || label->layout.is_null()) return;
	const Rect2 visible_rect = label->get_visible_rect();
	const Vector2 point = p_info.position;
	update_dialog_pointer(point, p_info.device);
	if (move_video_control(point) || move_audio_control(point)) {
		accept_pointer_event(p_info);
		return;
	}
	if (dragging_range.is_valid()) {
		const auto range = dragging_range;
		accept_pointer_event(p_info);
		move_range(range, point, false);
		return;
	}
	if (scrollbar_drag) {
		label->layout->drag_scrollbar(*scrollbar_drag, point);
		accept_pointer_event(p_info);
		return;
	}
	if (box_drag) {
		move_box_drag(point);
		accept_pointer_event(p_info);
		return;
	}
	if (dragging_input.is_valid()) {
		set_input_caret(dragging_input, point, true);
		label->request_hover_check();
		accept_pointer_event(p_info);
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
			pressed_blank = false;
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
		accept_pointer_event(p_info);
	}
	label->request_hover_check();
}

void HTMLLabelCanvas::handle_pointer_press(const PointerInfo& p_info) {
	if (!label || label->layout.is_null()) return;
	const Ref<InputEventMouseButton> button = p_info.source_event;
	if (button.is_null()) return;
	const Rect2 visible_rect = label->get_visible_rect();
	const Vector2 point = p_info.position;
	if (handle_video_button(*button.ptr(), point) || handle_audio_button(*button.ptr(), point)) {
		accept_pointer_event(p_info);
		return;
	}
	if (visible_rect.has_point(point) &&
		label->handle_listbox_mouse(label->layout->get_element_at(point), *button.ptr(), point)) {
		cancel_press();
		accept_pointer_event(p_info);
		return;
	}
	if (visible_rect.has_point(point)) {
		if (begin_resize_drag(point)) {
			accept_pointer_event(p_info);
			return;
		}
		if (const auto drag = label->layout->scrollbar_at(point)) {
			cancel_press();
			scrollbar_drag = drag;
			label->layout->drag_scrollbar(*drag, point);
			set_default_cursor_shape(CURSOR_ARROW);
			accept_pointer_event(p_info);
			return;
		}
	}
	Ref<HTMLElement> element;
	const HTMLLayout::LinkHit hit = visible_rect.has_point(point)
		? label->layout->find_link_at(point, &element) : HTMLLayout::LinkHit{};
	if (element.is_null() && label->layout->top_modal().is_valid()) {
		cancel_press();
		accept_pointer_event(p_info);
		return;
	}
	const Ref<HTMLElement> target_button = label->layout->get_button_ancestor(element);
	if (handle_input_decoration(Ref<HTMLInputElement>(element), *button.ptr(), point, p_info.accepts_event)) return;
	const Ref<HTMLElement> form_target = hit.id == 0 ? get_form_click_target(element) : Ref<HTMLElement>();
	const Ref<HTMLInputElement> target_input = element;
	if (target_input.is_valid() && target_input->get_input()->type() == HTMLInputNode::Type::RANGE &&
		label->layout->is_focusable(target_input)) {
		cancel_press();
		if (label->focus_element(target_input)) {
			dragging_range = target_input;
			accept_pointer_event(p_info);
			move_range(target_input, point, false);
		}
		return;
	}
	if (begin_textarea_scrollbar_drag(element, point)) {
		accept_pointer_event(p_info);
		return;
	}
	if (!dispatch_pointer_event(StringName("mousedown"), label->layout->get_element_at(point),
		*button.ptr(), point, true)) {
		// 监听器取消了按下：只保留点击合成所需的记录，跳过焦点、光标、选区与 :active。
		pressed_position = point - visible_rect.position;
		pressed_link = hit.id;
		pressed_element = element;
		pressed_form_target = form_target;
		pressed_blank = false;
		selecting = false;
		selection_dragged = false;
		accept_pointer_event(p_info);
		return;
	}
	cancel_button_press();
	cancel_key_press();
	pressed_position = point - visible_rect.position;
	pressed_link = hit.id;
	pressed_element = element;
	pressed_form_target = form_target;
	pressed_blank = false;
	selecting = visible_rect.has_point(point) && label->layout->hit_test_selectable_text(point, &pressed_blank) &&
		label->selection_enabled;
	pressed_blank = pressed_blank && hit.id == 0 && target_button.is_null() && form_target.is_null();
	if (selecting) {
		selection_origin = label->layout->selection_point_at(point);
		selecting = !selection_origin.excluded;
	}
	selection_dragged = false;
	const Ref<HTMLElement> target_focus = label->layout->get_focus_ancestor(element);
	if (target_focus.is_valid()) label->grab_focus();
	set_focused_element(target_focus);
	const auto control = label->layout->text_control(element);
	if (control && (control->is_text_control() || (target_input.is_valid() && target_input->get_input()->is_toggle()))) {
		selecting = false;
		pressed_blank = false;
		pressed_element.unref();
		pressed_link = 0;
		if (target_focus == element) {
			if (control->is_text_control()) {
				dragging_input = element;
				set_input_caret(element, point, p_info.shift);
				if (control->editor.is_multiline()) set_process_internal(true);
			}
			else if (target_input.is_valid() && target_input->get_input()->is_toggle()) {
				pressed_checkbox = target_input;
				set_element_pseudo(pressed_checkbox, "active", true);
			}
		}
		accept_pointer_event(p_info);
		return;
	}
	if (label->layout->is_enabled_button(target_button)) {
		pressed_button = target_button;
		set_element_pseudo(pressed_button, "active", true);
	}
	if (selecting) {
		const Vector2i selection = docrender::html_selection_between(selection_origin, selection_origin);
		selection_anchor = selection.x;
		selection_caret = selection.y;
		label->capture_node_selection();
		if (target_button.is_null()) label->grab_focus();
		queue_redraw();
	}
	if (selecting || hit.id != 0 || target_button.is_valid() || form_target.is_valid() || label->layout->top_modal_node()) {
		accept_pointer_event(p_info);
	}
}

void HTMLLabelCanvas::handle_pointer_release(const PointerInfo& p_info) {
	if (!label || label->layout.is_null()) return;
	const Ref<InputEventMouseButton> button = p_info.source_event;
	if (button.is_null()) return;
	const Rect2 visible_rect = label->get_visible_rect();
	const Vector2 point = p_info.position;
	if (handle_video_button(*button.ptr(), point) || handle_audio_button(*button.ptr(), point)) {
		accept_pointer_event(p_info);
		return;
	}
	if (p_info.button == MOUSE_BUTTON_LEFT && dragging_range.is_valid()) {
		const auto range = dragging_range;
		dragging_range.unref();
		accept_pointer_event(p_info);
		if (!p_info.canceled) move_range(range, point, true);
		else commit_input(range);
		return;
	}
	if (scrollbar_drag) {
		if (!p_info.canceled) {
			label->layout->drag_scrollbar(*scrollbar_drag, point);
			label->layout->finish_scrollbar_drag(*scrollbar_drag);
		}
		scrollbar_drag.reset();
		label->layout->set_scrollbar_pressed(std::nullopt);
		label->request_hover_check();
		accept_pointer_event(p_info);
		return;
	}
	if (box_drag) {
		if (!p_info.canceled) move_box_drag(point);
		flush_box_resize();
		box_drag.reset();
		label->layout->set_scrollbar_pressed(std::nullopt);
		label->request_hover_check();
		accept_pointer_event(p_info);
		return;
	}
	Ref<HTMLElement> element;
	const HTMLLayout::LinkHit hit = visible_rect.has_point(point)
		? label->layout->find_link_at(point, &element) : HTMLLayout::LinkHit{};
	const Ref<HTMLElement> target_button = label->layout->get_button_ancestor(element);
	const Ref<HTMLElement> form_target = hit.id == 0 ? get_form_click_target(element) : Ref<HTMLElement>();
	const Ref<HTMLInputElement> target_input = element;
	if (handle_input_decoration(Ref<HTMLInputElement>(element), *button.ptr(), point, p_info.accepts_event)) return;
	if (dragging_input.is_valid()) {
		if (!p_info.canceled && point != pressed_position + visible_rect.position) set_input_caret(dragging_input, point, true);
		dragging_input.unref();
		set_process_internal(false);
		accept_pointer_event(p_info);
		return;
	}
	if (pressed_checkbox.is_valid()) {
		const Ref<HTMLInputElement> checkbox = pressed_checkbox;
		set_element_pseudo(checkbox, "active", false);
		pressed_checkbox.unref();
		accept_pointer_event(p_info);
		if (!p_info.canceled && checkbox == target_input) {
			const Ref<HTMLElement> checkbox_target = checkbox;
			if (dispatch_pointer_event(StringName("click"), checkbox_target, *button.ptr(), point, true)) {
				toggle_checkbox(checkbox);
			}
		}
		return;
	}
	const bool clicked = !selection_dragged && !p_info.canceled &&
		(point - visible_rect.position).distance_squared_to(pressed_position) <= DRAG_DEADZONE * DRAG_DEADZONE;
	const bool blank_clicked = clicked && pressed_blank && visible_rect.has_point(point);
	const bool link_clicked = clicked && pressed_link != 0 && pressed_link == hit.id;
	const bool element_clicked = clicked && element.is_valid() && pressed_element == element;
	const uint64_t click_version = element_clicked ? element->adoption_version : 0;
	const bool button_clicked = !selection_dragged && !p_info.canceled &&
		pressed_button == target_button && label->layout->is_enabled_button(target_button);
	const bool form_clicked = clicked && form_target.is_valid() && pressed_form_target == form_target;
	if (selecting && selection_dragged && !p_info.canceled) {
		extend_document_selection(point.clamp(visible_rect.position, visible_rect.get_end()));
		queue_redraw();
	}
	if (selecting || link_clicked || pressed_button.is_valid() || form_clicked || label->layout->top_modal_node()) {
		accept_pointer_event(p_info);
	}
	const bool was_dragging = selection_dragged;
	cancel_press();
	if (blank_clicked) label->clear_selection();
	if (was_dragging) {
		label->request_hover_check();
		callable_mp(label->layout.ptr(), &HTMLLayout::request_auto_content_update).call_deferred();
	}
	const ObjectID label_id(label->get_instance_id());
	// mouseup 与可取消的 click 都在默认动作之前派发。
	const Ref<HTMLElement> event_target = label->layout->get_element_at(point);
	dispatch_pointer_event(StringName("mouseup"), event_target, *button.ptr(), point, false);
	const bool click_prevented = clicked && !dispatch_pointer_event(StringName("click"), event_target,
		*button.ptr(), point, true);
	if (link_clicked && !click_prevented) {
		label->activate_link(hit.uri);
	}
	if (button_clicked && !click_prevented) {
		if (auto* target = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(label_id))) {
			const Ref<HTMLInputElement> image = target_button;
			if (image.is_valid() && image->get_input() && image->get_input()->type() == HTMLInputNode::Type::IMAGE) {
				const auto local = target->layout->input_local_point(*image.ptr(), point);
				target->activate_form_control(image, local ? image->get_input()->image_coordinates(*local) : Vector2i());
			}
			else target->activate_element(target_button);
		}
	}
	else if (form_clicked && !click_prevented) {
		if (auto* target = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(label_id))) target->activate_element(form_target);
	}
	if (element_clicked && element->is_valid() && element->adoption_version == click_version) {
		if (auto* target = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(label_id))) {
			target->emit_signal("element_clicked", element);
		}
	}
}

bool HTMLLabelCanvas::handle_key_default_action(const InputEventKey& p_key) {
	const bool modal = label->layout->top_modal().is_valid();
	if (const auto direction = tab_direction(p_key); direction && (!tab_elements.empty() || modal)) {
		label->close_select_menu();
		const bool backwards = *direction;
		if (tab_elements.empty()) {
			label->accept_event();
			focus_tab_endpoint(backwards);
			return true;
		}
		if (move_video_focus(backwards) || move_audio_focus(backwards)) {
			accept_event();
			return true;
		}
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
				if (!modal && next && next != label) {
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
		set_audio_focus_edge(backwards);
		set_video_focus_edge(backwards);
		return true;
	}

	if (!modal || label->layout->allows_modal_interaction(focused_element)) {
		if (handle_video_key(p_key) || handle_audio_key(p_key) || label->handle_select_key(p_key) || handle_input_key(p_key)) return true;
	}
	if (label->handle_dialog_escape(p_key)) return true;
	if (modal && !label->layout->allows_modal_interaction(focused_element)) return false;
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
