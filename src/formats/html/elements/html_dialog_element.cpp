#include "html_dialog_element.h"
#include "html_dialog_node.h"

#include "formats/html/html_layout.h"
#include "nodes/html_label.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;
using docrender::HTMLDialogNode;

void HTMLDialogElement::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_open"), &HTMLDialogElement::is_open);
	ClassDB::bind_method(D_METHOD("set_open", "open"), &HTMLDialogElement::set_open);
	ClassDB::bind_method(D_METHOD("is_modal"), &HTMLDialogElement::is_modal);
	ClassDB::bind_method(D_METHOD("get_return_value"), &HTMLDialogElement::get_return_value);
	ClassDB::bind_method(D_METHOD("set_return_value", "value"), &HTMLDialogElement::set_return_value);
	ClassDB::bind_method(D_METHOD("show"), &HTMLDialogElement::show);
	ClassDB::bind_method(D_METHOD("show_modal"), &HTMLDialogElement::show_modal);
	ClassDB::bind_method(D_METHOD("close", "return_value"), &HTMLDialogElement::close, DEFVAL(Variant()));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "open"), "set_open", "is_open");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "return_value"), "set_return_value", "get_return_value");
	ADD_SIGNAL(MethodInfo("closed", PropertyInfo(Variant::STRING, "return_value")));
}

bool HTMLDialogElement::is_open() const { return has_attribute("open"); }

void HTMLDialogElement::set_open(bool p_open) {
	if (p_open) set_attribute("open", "");
	else remove_attribute("open");
}

bool HTMLDialogElement::is_modal() const {
	if (const auto node = std::dynamic_pointer_cast<HTMLDialogNode>(lock_element()))
		return node->has_pseudo_class(litehtml::_id("modal"));
	return false;
}

String HTMLDialogElement::get_return_value() const {
	if (const auto node = std::dynamic_pointer_cast<HTMLDialogNode>(lock_element())) return node->return_value;
	return {};
}

void HTMLDialogElement::set_return_value(const String& p_value) {
	if (const auto node = std::dynamic_pointer_cast<HTMLDialogNode>(lock_element())) node->return_value = p_value;
}

Error HTMLDialogElement::show() {
	return open_dialog(false);
}

Error HTMLDialogElement::show_modal() {
	return open_dialog(true);
}

Error HTMLDialogElement::open_dialog(bool p_modal) {
	auto* owner = get_owner();
	const auto node = lock_element();
	if (!owner || !node) return ERR_INVALID_PARAMETER;
	if (is_open()) return is_modal() == p_modal ? OK : ERR_INVALID_PARAMETER;
	const bool connected = is_connected();
	if (p_modal && !connected) return ERR_INVALID_PARAMETER;

	const ObjectID host = owner->dialog_host;
	if (connected) {
		if (auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(host)))
			label->remember_dialog_focus(Ref<HTMLDialogElement>(this), p_modal);
	}
	if (p_modal) owner->push_modal(node);
	const Error error = set_attribute("open", "");
	if (error == OK && p_modal && get_owner() == owner && is_modal()) {
		if (auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(host))) label->on_dialog_opened();
	}
	return error;
}

Error HTMLDialogElement::close(const Variant& p_return_value) {
	if (!is_valid()) return ERR_INVALID_PARAMETER;
	if (!is_open()) return OK;
	if (p_return_value.get_type() != Variant::NIL && p_return_value.get_type() != Variant::STRING) return ERR_INVALID_PARAMETER;
	const Error error = remove_attribute("open");
	if (error != OK) return error;
	if (p_return_value.get_type() == Variant::STRING) set_return_value(p_return_value);
	const String value = get_return_value();
	const auto* owner = get_owner();
	if (auto* label = owner ? Object::cast_to<HTMLLabel>(ObjectDB::get_instance(owner->dialog_host)) : nullptr)
		label->queue_dialog_closed(Ref<HTMLDialogElement>(this), value);
	else callable_mp(this, &HTMLDialogElement::emit_closed).call_deferred(value);
	return OK;
}

void HTMLDialogElement::emit_closed(const String& p_value) {
	if (is_valid()) emit_signal("closed", p_value);
}
