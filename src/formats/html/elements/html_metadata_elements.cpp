#include "html_metadata_elements.h"

#include "formats/html/html_layout.h"

using namespace godot;

void HTMLTitleElement::_bind_methods() {
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text_content", "get_text_content");
}

void HTMLMetaElement::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_name"), &HTMLMetaElement::get_name);
	ClassDB::bind_method(D_METHOD("set_name", "value"), &HTMLMetaElement::set_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
	ClassDB::bind_method(D_METHOD("get_content"), &HTMLMetaElement::get_content);
	ClassDB::bind_method(D_METHOD("set_content", "value"), &HTMLMetaElement::set_content);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "content"), "set_content", "get_content");
	ClassDB::bind_method(D_METHOD("get_http_equiv"), &HTMLMetaElement::get_http_equiv);
	ClassDB::bind_method(D_METHOD("set_http_equiv", "value"), &HTMLMetaElement::set_http_equiv);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "http_equiv"), "set_http_equiv", "get_http_equiv");
	ClassDB::bind_method(D_METHOD("get_media"), &HTMLMetaElement::get_media);
	ClassDB::bind_method(D_METHOD("set_media", "value"), &HTMLMetaElement::set_media);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "media"), "set_media", "get_media");
	ClassDB::bind_method(D_METHOD("get_charset"), &HTMLMetaElement::get_charset);
	ClassDB::bind_method(D_METHOD("set_charset", "value"), &HTMLMetaElement::set_charset);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "charset"), "set_charset", "get_charset");
}

String HTMLMetaElement::get_name() const { return get_attribute("name"); }
void HTMLMetaElement::set_name(const String& p_value) { set_attribute("name", p_value); }

String HTMLMetaElement::get_content() const { return get_attribute("content"); }
void HTMLMetaElement::set_content(const String& p_value) { set_attribute("content", p_value); }

String HTMLMetaElement::get_http_equiv() const { return get_attribute("http-equiv"); }
void HTMLMetaElement::set_http_equiv(const String& p_value) { set_attribute("http-equiv", p_value); }

String HTMLMetaElement::get_media() const { return get_attribute("media"); }
void HTMLMetaElement::set_media(const String& p_value) { set_attribute("media", p_value); }

String HTMLMetaElement::get_charset() const { return get_attribute("charset"); }
void HTMLMetaElement::set_charset(const String& p_value) { set_attribute("charset", p_value); }

void HTMLBaseElement::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_href"), &HTMLBaseElement::get_href);
	ClassDB::bind_method(D_METHOD("set_href", "value"), &HTMLBaseElement::set_href);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "href"), "set_href", "get_href");
	ClassDB::bind_method(D_METHOD("get_target"), &HTMLBaseElement::get_target);
	ClassDB::bind_method(D_METHOD("set_target", "value"), &HTMLBaseElement::set_target);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "target"), "set_target", "get_target");
	ClassDB::bind_method(D_METHOD("get_resolved_href"), &HTMLBaseElement::get_resolved_href);
}

String HTMLBaseElement::get_href() const { return get_attribute("href"); }
void HTMLBaseElement::set_href(const String& p_value) { set_attribute("href", p_value); }

String HTMLBaseElement::get_target() const { return get_attribute("target"); }
void HTMLBaseElement::set_target(const String& p_value) { set_attribute("target", p_value); }

String HTMLBaseElement::get_resolved_href() const {
	const auto* owner = get_owner();
	return owner ? owner->resolve_link_href(get_href(), true) : String();
}

void HTMLScriptElement::_bind_methods() {
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text_content", "get_text_content");
	ClassDB::bind_method(D_METHOD("get_src"), &HTMLScriptElement::get_src);
	ClassDB::bind_method(D_METHOD("set_src", "value"), &HTMLScriptElement::set_src);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "src"), "set_src", "get_src");
	ClassDB::bind_method(D_METHOD("get_type"), &HTMLScriptElement::get_type);
	ClassDB::bind_method(D_METHOD("set_type", "value"), &HTMLScriptElement::set_type);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "type"), "set_type", "get_type");
	ClassDB::bind_method(D_METHOD("is_async"), &HTMLScriptElement::is_async);
	ClassDB::bind_method(D_METHOD("set_async", "value"), &HTMLScriptElement::set_async);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "async"), "set_async", "is_async");
	ClassDB::bind_method(D_METHOD("is_defer"), &HTMLScriptElement::is_defer);
	ClassDB::bind_method(D_METHOD("set_defer", "value"), &HTMLScriptElement::set_defer);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "defer"), "set_defer", "is_defer");
	ClassDB::bind_method(D_METHOD("is_no_module"), &HTMLScriptElement::is_no_module);
	ClassDB::bind_method(D_METHOD("set_no_module", "value"), &HTMLScriptElement::set_no_module);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "no_module"), "set_no_module", "is_no_module");
}

String HTMLScriptElement::get_src() const { return get_attribute("src"); }
void HTMLScriptElement::set_src(const String& p_value) { set_attribute("src", p_value); }

String HTMLScriptElement::get_type() const { return get_attribute("type"); }
void HTMLScriptElement::set_type(const String& p_value) { set_attribute("type", p_value); }

bool HTMLScriptElement::is_async() const { return has_attribute("async"); }
void HTMLScriptElement::set_async(bool p_value) {
	if (p_value) set_attribute("async", "");
	else remove_attribute("async");
}

bool HTMLScriptElement::is_defer() const { return has_attribute("defer"); }
void HTMLScriptElement::set_defer(bool p_value) {
	if (p_value) set_attribute("defer", "");
	else remove_attribute("defer");
}

bool HTMLScriptElement::is_no_module() const { return has_attribute("nomodule"); }
void HTMLScriptElement::set_no_module(bool p_value) {
	if (p_value) set_attribute("nomodule", "");
	else remove_attribute("nomodule");
}
