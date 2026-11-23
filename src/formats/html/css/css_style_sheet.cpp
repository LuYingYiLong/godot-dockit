#include "css_style_sheet.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void CSSStyleSheet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_css_text", "text"), &CSSStyleSheet::set_css_text);
	ClassDB::bind_method(D_METHOD("get_css_text"), &CSSStyleSheet::get_css_text);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "css_text", PROPERTY_HINT_MULTILINE_TEXT), "set_css_text", "get_css_text");

	ClassDB::bind_method(D_METHOD("set_source_path", "path"), &CSSStyleSheet::set_source_path);
	ClassDB::bind_method(D_METHOD("get_source_path"), &CSSStyleSheet::get_source_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_source_path", "get_source_path");
}

void CSSStyleSheet::set_css_text(const String& p_text) {
	if (css_text == p_text) return;
	css_text = p_text;
	emit_changed();
}

String CSSStyleSheet::get_css_text() const {
	return css_text;
}

void CSSStyleSheet::set_source_path(const String& p_path) {
	source_path = p_path;
}

String CSSStyleSheet::get_source_path() const {
	return source_path;
}
