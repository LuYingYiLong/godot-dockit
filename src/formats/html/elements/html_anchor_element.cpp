#include "html_anchor_element.h"

using namespace godot;

void HTMLAnchorElement::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_href"), &HTMLAnchorElement::get_href);
	ClassDB::bind_method(D_METHOD("set_href", "href"), &HTMLAnchorElement::set_href);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "href"), "set_href", "get_href");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text_content", "get_text_content");
}

String HTMLAnchorElement::get_href() const {
	return get_attribute("href");
}

void HTMLAnchorElement::set_href(const String& p_href) {
	set_attribute("href", p_href);
}
