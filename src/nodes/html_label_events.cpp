#include "html_label.h"

#include "formats/html/dom/html_dom.h"
#include "formats/html/elements/html_form_elements.h"
#include "formats/html/events/html_event.h"
#include "formats/html/html_layout.h"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>

using namespace godot;

namespace {
	Ref<HTMLFocusEvent> make_focus_event(const StringName& p_type, bool p_bubbles, const Ref<HTMLElement>& p_related) {
		Ref<HTMLFocusEvent> event;
		event.instantiate();
		event->set_type(p_type);
		event->set_bubbles(p_bubbles);
		event->set_cancelable(false);
		event->set_related_target(p_related);
		event->mark_trusted();
		return event;
	}
}

bool HTMLLabelCanvas::dispatch_pointer_event(const StringName& p_type, const Ref<HTMLElement>& p_target,
	const InputEventMouseButton& p_event, const Vector2& p_point, bool p_cancelable) {
	if (!label || label->layout.is_null()) return false;
	if (p_target.is_null() || !p_target->is_valid()) return false;
	auto& dom = label->layout->get_dom();
	// 没有监听器时保持既有输入路径，避免为每次点击构造事件对象。
	if (!dom.has_event_listeners()) return false;
	Ref<HTMLMouseEvent> event;
	event.instantiate();
	event->set_type(p_type);
	event->set_bubbles(true);
	event->set_cancelable(p_cancelable);
	event->mark_trusted();
	const int button = int(p_event.get_button_index());
	event->set_button(button);
	event->set_buttons(p_event.is_pressed() && button >= 0 && button < 32 ? 1 << button : 0);
	event->set_position(p_point - label->get_visible_rect().position);
	event->set_shift_pressed(p_event.is_shift_pressed());
	event->set_ctrl_pressed(p_event.is_ctrl_pressed());
	event->set_alt_pressed(p_event.is_alt_pressed());
	event->set_meta_pressed(p_event.is_meta_pressed());
	dom.dispatch_event(p_target, event);
	return event->is_default_prevented();
}

bool HTMLLabelCanvas::dispatch_hover_events(const TypedArray<HTMLElement>& p_previous) {
	if (!label || label->layout.is_null()) return false;
	auto& dom = label->layout->get_dom();
	if (!dom.has_event_listeners()) return false;
	const TypedArray<HTMLElement> next = label->layout->get_hovered_elements();
	// 离开：由最内层向外层派发 mouseleave。
	for (int64_t index = 0; index < p_previous.size(); ++index) {
		const Ref<HTMLElement> element = p_previous[index];
		if (element.is_null() || !element->is_valid() || !element->is_connected()) continue;
		bool present = false;
		for (int64_t other = 0; other < next.size(); ++other) {
			const Ref<HTMLElement> candidate = next[other];
			if (candidate.ptr() == element.ptr()) {
				present = true;
				break;
			}
		}
		if (present) continue;
		dom.dispatch_event(element, docrender::make_dom_event(StringName("mouseleave"), false, false));
	}
	// 进入：由最外层向内层派发 mouseenter。
	for (int64_t index = next.size() - 1; index >= 0; --index) {
		const Ref<HTMLElement> element = next[index];
		if (element.is_null() || !element->is_valid() || !element->is_connected()) continue;
		bool present = false;
		for (int64_t other = 0; other < p_previous.size(); ++other) {
			const Ref<HTMLElement> candidate = p_previous[other];
			if (candidate.ptr() == element.ptr()) {
				present = true;
				break;
			}
		}
		if (present) continue;
		dom.dispatch_event(element, docrender::make_dom_event(StringName("mouseenter"), false, false));
	}
	return false;
}

bool HTMLLabelCanvas::dispatch_key_event(const StringName& p_type, const InputEventKey& p_key) {
	if (!label || label->layout.is_null()) return false;
	auto& dom = label->layout->get_dom();
	if (!dom.has_event_listeners()) return false;
	const bool pressed = p_type == StringName("keydown");
	Ref<HTMLKeyboardEvent> event;
	event.instantiate();
	event->set_type(p_type);
	event->set_bubbles(true);
	event->set_cancelable(pressed);
	event->mark_trusted();
	event->set_keycode(int(p_key.get_keycode()));
	event->set_physical_keycode(int(p_key.get_physical_keycode()));
	event->set_key_label(int(p_key.get_key_label()));
	event->set_unicode(int(p_key.get_unicode()));
	event->set_shift_pressed(p_key.is_shift_pressed());
	event->set_ctrl_pressed(p_key.is_ctrl_pressed());
	event->set_alt_pressed(p_key.is_alt_pressed());
	event->set_meta_pressed(p_key.is_meta_pressed());
	event->set_echo(p_key.is_echo());
	Ref<HTMLElement> target = focused_element;
	if (target.is_null() || !target->is_valid()) {
		target = label->layout->get_body();
		if (target.is_null()) return false;
	}
	dom.dispatch_event(target, event);
	return event->is_default_prevented();
}

void HTMLLabelCanvas::dispatch_blur_events(const Ref<HTMLElement>& p_previous, const Ref<HTMLElement>& p_next) {
	if (!label || label->layout.is_null()) return;
	if (p_previous.ptr() == p_next.ptr()) return;
	auto& dom = label->layout->get_dom();
	if (!dom.has_event_listeners()) return;
	if (p_previous.is_valid() && p_previous->is_valid()) {
		dom.dispatch_event(p_previous, make_focus_event(StringName("blur"), false, p_next));
		dom.dispatch_event(p_previous, make_focus_event(StringName("focusout"), true, p_next));
	}
}

void HTMLLabelCanvas::dispatch_focus_events(const Ref<HTMLElement>& p_previous, const Ref<HTMLElement>& p_next) {
	if (!label || label->layout.is_null()) return;
	if (p_previous.ptr() == p_next.ptr()) return;
	auto& dom = label->layout->get_dom();
	if (!dom.has_event_listeners()) return;
	if (p_next.is_valid() && p_next->is_valid()) {
		dom.dispatch_event(p_next, make_focus_event(StringName("focus"), false, p_previous));
		dom.dispatch_event(p_next, make_focus_event(StringName("focusin"), true, p_previous));
	}
}

bool HTMLLabel::dispatch_form_event(const StringName& p_type, const Ref<HTMLFormElement>& p_form,
	const Ref<HTMLElement>& p_submitter) {
	if (layout.is_null() || p_form.is_null() || !p_form->is_valid()) return false;
	auto& dom = layout->get_dom();
	if (!dom.has_event_listeners()) return false;
	Ref<HTMLSubmitEvent> event;
	event.instantiate();
	event->set_type(p_type);
	event->set_bubbles(true);
	event->set_cancelable(true);
	event->set_submitter(p_submitter);
	event->mark_trusted();
	dom.dispatch_event(p_form, event);
	return event->is_default_prevented();
}

bool HTMLLabel::dispatch_control_event(const StringName& p_type, const Ref<HTMLElement>& p_element) {
	if (layout.is_null() || p_element.is_null() || !p_element->is_valid()) return false;
	auto& dom = layout->get_dom();
	if (!dom.has_event_listeners()) return false;
	dom.dispatch_event(p_element, docrender::make_dom_event(p_type, true, false));
	return false;
}

void HTMLLabel::dispatch_scroll_event(const Ref<HTMLElement>& p_element) {
	if (layout.is_null() || p_element.is_null() || !p_element->is_valid()) return;
	layout->get_dom().queue_scroll_event(p_element);
}

Ref<HTMLDocumentNode> HTMLLabel::get_document_node() const {
	if (layout.is_null()) return {};
	return layout->get_dom().get_document_node();
}
