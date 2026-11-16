#include "html_input_element.h"
#include "html_form_elements.h"

#include "html_input_node.h"
#include "formats/html/html_layout.h"

#include <cmath>
#include <limits>

using namespace godot;

Ref<HTMLFormElement> HTMLInputElement::get_form() const { return get_form_owner(); }
using docrender::HTMLInputNode;

void HTMLInputElement::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_form"), &HTMLInputElement::get_form);
	ClassDB::bind_method(D_METHOD("get_type"), &HTMLInputElement::get_type);
	ClassDB::bind_method(D_METHOD("set_type", "value"), &HTMLInputElement::set_type);
	ClassDB::bind_method(D_METHOD("get_value_as_number"), &HTMLInputElement::get_value_as_number);
	ClassDB::bind_method(D_METHOD("set_value_as_number", "value"), &HTMLInputElement::set_value_as_number);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "type"), "set_type", "get_type");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value_as_number"), "set_value_as_number", "get_value_as_number");
	ClassDB::bind_method(D_METHOD("get_value"), &HTMLInputElement::get_value);
	ClassDB::bind_method(D_METHOD("set_value", "value"), &HTMLInputElement::set_value);
	ClassDB::bind_method(D_METHOD("is_checked"), &HTMLInputElement::is_checked);
	ClassDB::bind_method(D_METHOD("set_checked", "checked"), &HTMLInputElement::set_checked);
	ClassDB::bind_method(D_METHOD("get_selection_start"), &HTMLInputElement::get_selection_start);
	ClassDB::bind_method(D_METHOD("get_selection_end"), &HTMLInputElement::get_selection_end);
	ClassDB::bind_method(D_METHOD("set_selection_range", "start", "end"), &HTMLInputElement::set_selection_range);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "value"), "set_value", "get_value");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "checked"), "set_checked", "is_checked");
}

std::shared_ptr<HTMLInputNode> HTMLInputElement::get_input() const {
	return std::dynamic_pointer_cast<HTMLInputNode>(lock_element());
}

String HTMLInputElement::get_type() const {
	const auto input = get_input();
	return input ? input->type_name() : String();
}

void HTMLInputElement::set_type(const String& p_value) { set_attribute("type", p_value); }

double HTMLInputElement::get_value_as_number() const {
	const auto input = get_input();
	return input ? input->value_as_number() : std::numeric_limits<double>::quiet_NaN();
}

void HTMLInputElement::set_value_as_number(double p_value) {
	const auto input = get_input();
	if (!input || (input->type() != HTMLInputNode::Type::NUMBER && input->type() != HTMLInputNode::Type::RANGE)) return;
	if (std::isfinite(p_value)) set_value(String::num_real(p_value, false));
	else if (std::isnan(p_value)) set_value("");
}

String HTMLInputElement::get_value() const {
	const auto input = get_input();
	return input ? input->get_value() : String();
}

void HTMLInputElement::set_value(const String& p_value) {
	if (const auto input = get_input()) {
		if (input->is_toggle() || input->is_button() || input->type() == HTMLInputNode::Type::HIDDEN) {
			set_attribute("value", p_value);
		} else if (input->editor.set_value(input->sanitize_value(p_value))) {
			get_owner()->update_input(*input);
		}
	}
}

bool HTMLInputElement::is_checked() const {
	const auto input = get_input();
	return input && input->is_checked();
}

void HTMLInputElement::set_checked(bool p_checked) {
	if (const auto input = get_input(); input && input->set_checked(p_checked)) {
		get_owner()->update_input(*input);
	}
}

int64_t HTMLInputElement::get_selection_start() const {
	const auto input = get_input();
	return input && input->is_text_control() ? input->editor.selection_start() : -1;
}

int64_t HTMLInputElement::get_selection_end() const {
	const auto input = get_input();
	return input && input->is_text_control() ? input->editor.selection_end() : -1;
}

void HTMLInputElement::set_selection_range(int64_t p_start, int64_t p_end) {
	if (const auto input = get_input(); input && input->is_text_control() &&
		input->editor.set_selection(std::min(p_start, p_end), p_end)) {
		get_owner()->update_input(*input);
	}
}
