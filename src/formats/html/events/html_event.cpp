#include "html_event.h"

#include <chrono>

using namespace godot;

namespace {
	int64_t now_milliseconds() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	// 只在派发器接管前允许修改，避免脚本篡改正在传播的事件。
	bool accepts_configuration(const HTMLEvent& p_event) {
		return !p_event.is_dispatching();
	}
}

void HTMLEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_type"), &HTMLEvent::get_type);
	ClassDB::bind_method(D_METHOD("set_type", "type"), &HTMLEvent::set_type);
	ClassDB::bind_method(D_METHOD("get_target"), &HTMLEvent::get_target);
	ClassDB::bind_method(D_METHOD("get_current_target"), &HTMLEvent::get_current_target);
	ClassDB::bind_method(D_METHOD("get_event_phase"), &HTMLEvent::get_event_phase);
	ClassDB::bind_method(D_METHOD("get_bubbles"), &HTMLEvent::get_bubbles);
	ClassDB::bind_method(D_METHOD("set_bubbles", "bubbles"), &HTMLEvent::set_bubbles);
	ClassDB::bind_method(D_METHOD("get_cancelable"), &HTMLEvent::get_cancelable);
	ClassDB::bind_method(D_METHOD("set_cancelable", "cancelable"), &HTMLEvent::set_cancelable);
	ClassDB::bind_method(D_METHOD("is_default_prevented"), &HTMLEvent::is_default_prevented);
	ClassDB::bind_method(D_METHOD("is_trusted"), &HTMLEvent::is_trusted);
	ClassDB::bind_method(D_METHOD("get_time_stamp"), &HTMLEvent::get_time_stamp);
	ClassDB::bind_method(D_METHOD("prevent_default"), &HTMLEvent::prevent_default);
	ClassDB::bind_method(D_METHOD("stop_propagation"), &HTMLEvent::stop_propagation);
	ClassDB::bind_method(D_METHOD("stop_immediate_propagation"), &HTMLEvent::stop_immediate_propagation);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "type"), "set_type", "get_type");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_RESOURCE_TYPE, "HTMLNode", PROPERTY_USAGE_NONE),
		"", "get_target");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "current_target", PROPERTY_HINT_RESOURCE_TYPE, "HTMLNode", PROPERTY_USAGE_NONE),
		"", "get_current_target");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "event_phase", PROPERTY_HINT_ENUM, "NONE,CAPTURING,AT_TARGET,BUBBLING"),
		"", "get_event_phase");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bubbles"), "set_bubbles", "get_bubbles");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cancelable"), "set_cancelable", "get_cancelable");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "default_prevented"), "", "is_default_prevented");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "trusted"), "", "is_trusted");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "time_stamp"), "", "get_time_stamp");
	BIND_ENUM_CONSTANT(PHASE_NONE);
	BIND_ENUM_CONSTANT(PHASE_CAPTURING);
	BIND_ENUM_CONSTANT(PHASE_AT_TARGET);
	BIND_ENUM_CONSTANT(PHASE_BUBBLING);
}

void HTMLEvent::begin_dispatch(const Ref<HTMLNode>& p_target) {
	target = p_target;
	current_target = p_target;
	phase = PHASE_NONE;
	propagation_stopped = false;
	immediate_stopped = false;
	dispatching = true;
	if (!time_stamp) time_stamp = now_milliseconds();
}

void HTMLEvent::set_dispatch_phase(const Ref<HTMLNode>& p_node, int p_phase) {
	current_target = p_node;
	phase = p_phase;
	immediate_stopped = false;
}

void HTMLEvent::end_dispatch() {
	dispatching = false;
	phase = PHASE_NONE;
	current_target = Ref<HTMLNode>();
}

void HTMLEvent::set_type(const StringName& p_type) {
	if (!accepts_configuration(*this)) return;
	type = p_type;
}

void HTMLEvent::set_bubbles(bool p_bubbles) {
	if (!accepts_configuration(*this)) return;
	bubbles = p_bubbles;
}

void HTMLEvent::set_cancelable(bool p_cancelable) {
	if (!accepts_configuration(*this)) return;
	cancelable = p_cancelable;
}

void HTMLEvent::prevent_default() {
	if (cancelable) default_prevented = true;
}

void HTMLEvent::stop_propagation() {
	if (dispatching) propagation_stopped = true;
}

void HTMLEvent::stop_immediate_propagation() {
	if (!dispatching) return;
	propagation_stopped = true;
	immediate_stopped = true;
}

void HTMLMouseEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_button"), &HTMLMouseEvent::get_button);
	ClassDB::bind_method(D_METHOD("set_button", "button"), &HTMLMouseEvent::set_button);
	ClassDB::bind_method(D_METHOD("get_buttons"), &HTMLMouseEvent::get_buttons);
	ClassDB::bind_method(D_METHOD("set_buttons", "buttons"), &HTMLMouseEvent::set_buttons);
	ClassDB::bind_method(D_METHOD("get_position"), &HTMLMouseEvent::get_position);
	ClassDB::bind_method(D_METHOD("set_position", "position"), &HTMLMouseEvent::set_position);
	ClassDB::bind_method(D_METHOD("is_shift_pressed"), &HTMLMouseEvent::is_shift_pressed);
	ClassDB::bind_method(D_METHOD("set_shift_pressed", "pressed"), &HTMLMouseEvent::set_shift_pressed);
	ClassDB::bind_method(D_METHOD("is_ctrl_pressed"), &HTMLMouseEvent::is_ctrl_pressed);
	ClassDB::bind_method(D_METHOD("set_ctrl_pressed", "pressed"), &HTMLMouseEvent::set_ctrl_pressed);
	ClassDB::bind_method(D_METHOD("is_alt_pressed"), &HTMLMouseEvent::is_alt_pressed);
	ClassDB::bind_method(D_METHOD("set_alt_pressed", "pressed"), &HTMLMouseEvent::set_alt_pressed);
	ClassDB::bind_method(D_METHOD("is_meta_pressed"), &HTMLMouseEvent::is_meta_pressed);
	ClassDB::bind_method(D_METHOD("set_meta_pressed", "pressed"), &HTMLMouseEvent::set_meta_pressed);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "button"), "set_button", "get_button");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "buttons"), "set_buttons", "get_buttons");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "position"), "set_position", "get_position");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shift_pressed"), "set_shift_pressed", "is_shift_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ctrl_pressed"), "set_ctrl_pressed", "is_ctrl_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "alt_pressed"), "set_alt_pressed", "is_alt_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "meta_pressed"), "set_meta_pressed", "is_meta_pressed");
}

void HTMLKeyboardEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_keycode"), &HTMLKeyboardEvent::get_keycode);
	ClassDB::bind_method(D_METHOD("set_keycode", "keycode"), &HTMLKeyboardEvent::set_keycode);
	ClassDB::bind_method(D_METHOD("get_physical_keycode"), &HTMLKeyboardEvent::get_physical_keycode);
	ClassDB::bind_method(D_METHOD("set_physical_keycode", "physical_keycode"), &HTMLKeyboardEvent::set_physical_keycode);
	ClassDB::bind_method(D_METHOD("get_key_label"), &HTMLKeyboardEvent::get_key_label);
	ClassDB::bind_method(D_METHOD("set_key_label", "key_label"), &HTMLKeyboardEvent::set_key_label);
	ClassDB::bind_method(D_METHOD("get_unicode"), &HTMLKeyboardEvent::get_unicode);
	ClassDB::bind_method(D_METHOD("set_unicode", "unicode"), &HTMLKeyboardEvent::set_unicode);
	ClassDB::bind_method(D_METHOD("is_shift_pressed"), &HTMLKeyboardEvent::is_shift_pressed);
	ClassDB::bind_method(D_METHOD("set_shift_pressed", "pressed"), &HTMLKeyboardEvent::set_shift_pressed);
	ClassDB::bind_method(D_METHOD("is_ctrl_pressed"), &HTMLKeyboardEvent::is_ctrl_pressed);
	ClassDB::bind_method(D_METHOD("set_ctrl_pressed", "pressed"), &HTMLKeyboardEvent::set_ctrl_pressed);
	ClassDB::bind_method(D_METHOD("is_alt_pressed"), &HTMLKeyboardEvent::is_alt_pressed);
	ClassDB::bind_method(D_METHOD("set_alt_pressed", "pressed"), &HTMLKeyboardEvent::set_alt_pressed);
	ClassDB::bind_method(D_METHOD("is_meta_pressed"), &HTMLKeyboardEvent::is_meta_pressed);
	ClassDB::bind_method(D_METHOD("set_meta_pressed", "pressed"), &HTMLKeyboardEvent::set_meta_pressed);
	ClassDB::bind_method(D_METHOD("is_echo"), &HTMLKeyboardEvent::is_echo);
	ClassDB::bind_method(D_METHOD("set_echo", "echo"), &HTMLKeyboardEvent::set_echo);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "keycode"), "set_keycode", "get_keycode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "physical_keycode"), "set_physical_keycode", "get_physical_keycode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "key_label"), "set_key_label", "get_key_label");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "unicode"), "set_unicode", "get_unicode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shift_pressed"), "set_shift_pressed", "is_shift_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ctrl_pressed"), "set_ctrl_pressed", "is_ctrl_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "alt_pressed"), "set_alt_pressed", "is_alt_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "meta_pressed"), "set_meta_pressed", "is_meta_pressed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "echo"), "set_echo", "is_echo");
}

void HTMLFocusEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_related_target"), &HTMLFocusEvent::get_related_target);
	ClassDB::bind_method(D_METHOD("set_related_target", "target"), &HTMLFocusEvent::set_related_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "related_target", PROPERTY_HINT_RESOURCE_TYPE, "HTMLNode", PROPERTY_USAGE_NONE),
		"set_related_target", "get_related_target");
}

void HTMLSubmitEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_submitter"), &HTMLSubmitEvent::get_submitter);
	ClassDB::bind_method(D_METHOD("set_submitter", "submitter"), &HTMLSubmitEvent::set_submitter);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "submitter", PROPERTY_HINT_RESOURCE_TYPE, "HTMLElement", PROPERTY_USAGE_NONE),
		"set_submitter", "get_submitter");
}

namespace docrender {
	Ref<HTMLEvent> make_dom_event(const StringName& p_type, bool p_bubbles, bool p_cancelable) {
		Ref<HTMLEvent> event;
		if (p_type == StringName("mousedown") || p_type == StringName("mouseup") ||
			p_type == StringName("click") || p_type == StringName("contextmenu")) {
			Ref<HTMLMouseEvent> mouse;
			mouse.instantiate();
			event = mouse;
		}
		else if (p_type == StringName("keydown") || p_type == StringName("keyup")) {
			Ref<HTMLKeyboardEvent> key;
			key.instantiate();
			event = key;
		}
		else if (p_type == StringName("focus") || p_type == StringName("blur") ||
			p_type == StringName("focusin") || p_type == StringName("focusout")) {
			Ref<HTMLFocusEvent> focus;
			focus.instantiate();
			event = focus;
		}
		else if (p_type == StringName("submit") || p_type == StringName("reset")) {
			Ref<HTMLSubmitEvent> submit;
			submit.instantiate();
			event = submit;
		}
		else {
			event.instantiate();
		}
		event->set_type(p_type);
		event->set_bubbles(p_bubbles);
		event->set_cancelable(p_cancelable);
		event->mark_trusted();
		return event;
	}
}
