#include "html_label.h"

#include "formats/html/dom/html_dom.h"
#include "formats/html/elements/html_form_elements.h"
#include "formats/html/elements/html_input_node.h"
#include "formats/html/events/html_event.h"
#include "formats/html/html_layout.h"
#include "formats/html/text/html_text_selection.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/object_id.hpp>

#include <algorithm>
#include <iterator>
#include <utility>

using namespace godot;

using docrender::HTMLInputNode;

int HTMLLabelCanvas::click_detail_for(const Ref<HTMLElement>& p_target, const Vector2& p_position) const {
	if (click_count == 0 || p_target.is_null() || !p_target->is_valid() || last_click_target != p_target ||
		last_click_version != p_target->adoption_version) return 1;
	const uint64_t now = Time::get_singleton()->get_ticks_msec();
	if (now < last_click_msec || now - last_click_msec >= 500 ||
		p_position.distance_squared_to(last_click_position) >= 25.0) return 1;
	return click_count + 1;
}

void HTMLLabelCanvas::record_click(const Ref<HTMLElement>& p_target, const Vector2& p_position, int p_detail) {
	if (p_target.is_null() || !p_target->is_valid()) {
		reset_click_sequence();
		return;
	}
	click_count = p_detail;
	last_click_msec = Time::get_singleton()->get_ticks_msec();
	last_click_version = p_target->adoption_version;
	last_click_position = p_position;
	last_click_target = p_target;
}

void HTMLLabelCanvas::reset_click_sequence() {
	click_count = 0;
	last_click_msec = 0;
	last_click_version = 0;
	last_click_target.unref();
}

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

bool HTMLLabelCanvas::handle_wheel_input(const PointerInfo& p_info, float p_pixels_per_step) {
	if (!label || label->layout.is_null() || p_info.source_event.is_null()) return true;
	auto& dom = label->layout->get_dom();
	if (!dom.has_event_listener_for_type(StringName("wheel"))) return true;
	const Ref<HTMLElement> target = label->layout->get_element_at(p_info.position);
	if (target.is_null() || !target->is_valid()) return true;
	const Vector2 steps = HTMLLayout::scroll_input_steps(p_info.source_event);
	if (steps.is_zero_approx()) return true;
	Ref<HTMLWheelEvent> event;
	event.instantiate();
	event->set_type(StringName("wheel"));
	event->set_bubbles(true);
	event->set_cancelable(true);
	event->mark_trusted();
	event->set_delta(steps * p_pixels_per_step);
	event->set_delta_mode(HTMLWheelEvent::DELTA_MODE_PIXEL);
	event->set_position(p_info.position - label->get_visible_rect().position);
	event->set_shift_pressed(p_info.shift);
	event->set_ctrl_pressed(p_info.ctrl);
	event->set_alt_pressed(p_info.alt);
	event->set_meta_pressed(p_info.meta);
	dom.dispatch_event(target, event);
	label->wheel_dispatched_in_canvas = true;
	const uint64_t sequence = ++label->wheel_dispatch_sequence;
	callable_mp(label, &HTMLLabel::clear_canvas_wheel_dispatch).call_deferred(sequence);
	return !event->is_default_prevented();
}

HTMLLabelCanvas::PointerInfo HTMLLabelCanvas::make_touch_pointer_info(const TouchPointer& p_touch,
	const Vector2& p_position, bool p_pressed, bool p_canceled) const {
	PointerInfo info;
	info.position = p_position;
	info.button = MOUSE_BUTTON_LEFT;
	info.buttons = p_pressed ? 1 : 0;
	info.pressed = p_pressed;
	info.canceled = p_canceled;
	info.device = p_touch.device;
	info.pointer_id = p_touch.pointer_id;
	info.detail = p_touch.detail;
	info.kind = PointerKind::TOUCH;
	info.primary = p_touch.primary;
	info.accepts_event = false;
	info.target = p_touch.target;
	return info;
}

Ref<InputEventMouseButton> HTMLLabelCanvas::make_touch_mouse_event(const PointerInfo& p_info) const {
	Ref<InputEventMouseButton> event;
	event.instantiate();
	event->set_button_index(MouseButton(MOUSE_BUTTON_LEFT));
	event->set_pressed(p_info.pressed);
	event->set_canceled(p_info.canceled);
	event->set_device(p_info.device);
	event->set_position(p_info.position);
	return event;
}

void HTMLLabelCanvas::handle_touch_press(const InputEventScreenTouch& p_touch) {
	if (!label || label->layout.is_null()) return;
	const TouchKey key{ p_touch.get_device(), p_touch.get_index() };
	if (active_touch_pointers.find(key) != active_touch_pointers.end()) cancel_touch_pointer(key.first, key.second);
	auto id = touch_pointer_ids.find(key);
	if (id == touch_pointer_ids.end()) id = touch_pointer_ids.emplace(key, next_touch_pointer_id++).first;
	TouchPointer touch;
	touch.device = key.first;
	touch.index = key.second;
	touch.pointer_id = id->second;
	touch.start = p_touch.get_position();
	touch.last = p_touch.get_position();
	touch.target = label->layout->get_element_at(touch.start);
	touch.detail = click_detail_for(touch.target, touch.start - label->get_visible_rect().position);
	touch.primary = active_touch_pointers.empty();
	touch.compatibility_suppressed = !touch.primary;
	auto [active, inserted] = active_touch_pointers.emplace(key, touch);
	if (!inserted) return;
	PointerInfo info = make_touch_pointer_info(active->second, touch.start, true, false);
	if (!dispatch_pointer_event(StringName("pointerdown"), active->second.target, info, true)) {
		active->second.compatibility_suppressed = true;
		return;
	}
	if (active->second.compatibility_suppressed) return;
	Ref<InputEventMouseButton> mouse = make_touch_mouse_event(info);
	info.source_event = mouse;
	info.mouse_down_prevented = !dispatch_pointer_event(StringName("mousedown"), active->second.target, info, true);
	handle_pointer_press(info);
}

void HTMLLabelCanvas::handle_touch_drag(const InputEventScreenDrag& p_drag) {
	const TouchKey key{ p_drag.get_device(), p_drag.get_index() };
	auto active = active_touch_pointers.find(key);
	if (active == active_touch_pointers.end() || active->second.canceled) return;
	TouchPointer& touch = active->second;
	touch.last = p_drag.get_position();
	PointerInfo info = make_touch_pointer_info(touch, touch.last, true, false);
	info.detail = 0;
	if (!touch.dragged && (touch.last - touch.start).length_squared() > DRAG_DEADZONE * DRAG_DEADZONE) {
		touch.dragged = true;
		cancel_press();
	}
	if (label->viewport_touch_dragged) {
		cancel_touch_pointer(key.first, key.second);
		return;
	}
	if (!touch.dragged && !touch.compatibility_suppressed) handle_pointer_move(info);
	else dispatch_pointer_event(StringName("pointermove"), touch.target, info, false);
}

void HTMLLabelCanvas::handle_touch_release(const InputEventScreenTouch& p_touch) {
	const TouchKey key{ p_touch.get_device(), p_touch.get_index() };
	auto active = active_touch_pointers.find(key);
	if (active == active_touch_pointers.end()) return;
	TouchPointer touch = active->second;
	active_touch_pointers.erase(active);
	if (p_touch.is_canceled()) {
		reset_click_sequence();
		PointerInfo canceled = make_touch_pointer_info(touch, p_touch.get_position(), false, true);
		dispatch_pointer_event(StringName("pointercancel"), touch.target, canceled, false);
		cancel_press();
		return;
	}
	touch.last = p_touch.get_position();
	PointerInfo info = make_touch_pointer_info(touch, touch.last, false, false);
	info.detail = click_detail_for(touch.target, touch.last - label->get_visible_rect().position);
	dispatch_pointer_event(StringName("pointerup"), touch.target, info, false);
	const Ref<HTMLElement> release_target = label->layout->get_element_at(touch.last);
	if (!touch.primary || touch.compatibility_suppressed || touch.dragged || release_target != touch.target) {
		reset_click_sequence();
		cancel_press();
		return;
	}
	Ref<InputEventMouseButton> mouse = make_touch_mouse_event(info);
	info.source_event = mouse;
	dispatch_pointer_event(StringName("mouseup"), release_target, info, false);
	handle_pointer_release(info);
}

void HTMLLabelCanvas::cancel_touch_pointer(int p_device, int p_index) {
	const TouchKey key{ p_device, p_index };
	auto active = active_touch_pointers.find(key);
	if (active == active_touch_pointers.end()) return;
	const TouchPointer touch = active->second;
	active_touch_pointers.erase(active);
	PointerInfo info = make_touch_pointer_info(touch, touch.last, false, true);
	dispatch_pointer_event(StringName("pointercancel"), touch.target, info, false);
	cancel_press();
}

void HTMLLabelCanvas::cancel_all_touch_pointers() {
	while (!active_touch_pointers.empty()) {
		const TouchKey key = active_touch_pointers.begin()->first;
		cancel_touch_pointer(key.first, key.second);
	}
}

void HTMLLabelCanvas::handle_pointer_move(const PointerInfo& p_info) {
	if (!label || label->layout.is_null()) return;
	const Rect2 visible_rect = label->get_visible_rect();
	const Vector2 point = p_info.position;
	const Ref<HTMLElement> event_target = p_info.target.is_valid() ? p_info.target : label->layout->get_element_at(point);
	dispatch_pointer_event(StringName("pointermove"), event_target, p_info, false);
	dispatch_pointer_event(StringName("mousemove"), event_target, p_info, false);
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
	Ref<HTMLElement> element;
	const HTMLLayout::LinkHit hit = visible_rect.has_point(point)
		? label->layout->find_link_at(point, &element) : HTMLLayout::LinkHit{};
	const Ref<HTMLElement> form_target = hit.id == 0 ? get_form_click_target(element) : Ref<HTMLElement>();
	if (p_info.mouse_down_prevented) {
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
	if (element.is_null() && label->layout->top_modal().is_valid()) {
		cancel_press();
		accept_pointer_event(p_info);
		return;
	}
	const Ref<HTMLElement> target_button = label->layout->get_button_ancestor(element);
	if (handle_input_decoration(Ref<HTMLInputElement>(element), *button.ptr(), point, p_info.accepts_event)) return;
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
	reset_click_sequence();
	const Ref<InputEventMouseButton> button = p_info.source_event;
	if (button.is_null()) return;
	const Rect2 visible_rect = label->get_visible_rect();
	const Vector2 point = p_info.position;
	const Ref<HTMLElement> event_target = label->layout->get_element_at(point);
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
			const Vector2 click_position = point - visible_rect.position;
			record_click(checkbox_target, click_position, p_info.detail);
			if (dispatch_pointer_event(StringName("click"), checkbox_target, p_info, true)) {
				toggle_checkbox(checkbox);
			}
			if (p_info.detail == 2) dispatch_pointer_event(StringName("dblclick"), checkbox_target, p_info, true);
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
	// 可取消的 click 在对应默认动作之前派发。
	if (clicked) record_click(event_target, point - visible_rect.position, p_info.detail);
	const bool click_prevented = clicked && !dispatch_pointer_event(StringName("click"), event_target,
		p_info, true);
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
	if (clicked && p_info.detail == 2 && event_target.is_valid()) {
		dispatch_pointer_event(StringName("dblclick"), event_target, p_info, true);
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
