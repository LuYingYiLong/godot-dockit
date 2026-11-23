#include "html_style_sheet.h"

#include "formats/html/dom/html_dom.h"
#include "formats/html/elements/html_stylesheet_node.h"
#include "formats/html/css/html_css_rule.h"
#include "formats/html/html_layout.h"

using namespace godot;

void HTMLStyleSheet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_rules"), &HTMLStyleSheet::get_rules);
	ClassDB::bind_method(D_METHOD("insert_rule", "rule", "index"), &HTMLStyleSheet::insert_rule, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("delete_rule", "index"), &HTMLStyleSheet::delete_rule);
	ClassDB::bind_method(D_METHOD("is_valid"), &HTMLStyleSheet::is_valid);
	
	ClassDB::bind_method(D_METHOD("get_id"), &HTMLStyleSheet::get_id);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "id"), "", "get_id");
	
	ClassDB::bind_method(D_METHOD("get_owner_node"), &HTMLStyleSheet::get_owner_node);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "owner_node", PROPERTY_HINT_NONE, "",
		PROPERTY_USAGE_DEFAULT, "HTMLElement"), "", "get_owner_node");
	
	ClassDB::bind_method(D_METHOD("get_href"), &HTMLStyleSheet::get_href);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "href"), "", "get_href");
	
	ClassDB::bind_method(D_METHOD("get_media"), &HTMLStyleSheet::get_media);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "media"), "", "get_media");
	
	ClassDB::bind_method(D_METHOD("is_disabled"), &HTMLStyleSheet::is_disabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "disabled"), "", "is_disabled");
	
	ClassDB::bind_method(D_METHOD("is_read_only"), &HTMLStyleSheet::is_read_only);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "read_only"), "", "is_read_only");
	
	ClassDB::bind_method(D_METHOD("get_origin"), &HTMLStyleSheet::get_origin);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "origin"), "", "get_origin");
	
	ClassDB::bind_method(D_METHOD("get_css_rules"), &HTMLStyleSheet::get_css_rules);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rules", PROPERTY_HINT_ARRAY_TYPE, "CSSRule"), "", "get_rules");
}

HTMLLayout* HTMLStyleSheet::get_layout() const {
	auto* layout = Object::cast_to<HTMLLayout>(ObjectDB::get_instance(layout_id));
	return layout && layout->is_stylesheet_handle_current(*this) && layout->refresh_cssom() &&
		layout->is_stylesheet_handle_current(*this) ? layout : nullptr;
}

bool HTMLStyleSheet::is_valid() const {
	return get_layout() != nullptr;
}

int64_t HTMLStyleSheet::get_id() const {
	return is_valid() ? id : 0;
}

Ref<HTMLElement> HTMLStyleSheet::get_owner_node() const {
	const auto* layout = get_layout();
	return layout ? layout->wrap_hit_element(owner.lock()) : Ref<HTMLElement>();
}

String HTMLStyleSheet::get_href() const {
	const auto node = owner.lock();
	return is_valid() && node ? String::utf8(node->linked_path.c_str()) : String();
}

String HTMLStyleSheet::get_media() const {
	const auto node = owner.lock();
	return is_valid() && node ? String::utf8(node->get_attr("media", "")) : String();
}

bool HTMLStyleSheet::is_disabled() const {
	if (!is_valid()) return false;
	const auto node = owner.lock();
	return node && (node->tag() == litehtml::_link_ ? node->get_attr("disabled") != nullptr : node->style_disabled);
}

bool HTMLStyleSheet::is_read_only() const { return source_kind == DEFAULT_SOURCE; }

String HTMLStyleSheet::get_origin() const {
	if (!is_valid()) return {};
	if (source_kind == DEFAULT_SOURCE) return "user_agent";
	return source_kind == EXTRA_SOURCE ? String("author_extra") : String("author");
}

PackedStringArray HTMLStyleSheet::get_css_rules() const {
	auto* layout = get_layout();
	return layout ? layout->get_css_rules(id) : PackedStringArray();
}

TypedArray<CSSRule> HTMLStyleSheet::get_rules() const {
	TypedArray<CSSRule> result;
	auto* layout = get_layout();
	if (!layout) return result;
	const auto rules = layout->get_css_rule_children(id, PackedInt32Array());
	for (int64_t index = 0; index < rules.size(); ++index) {
		Ref<CSSRule> rule;
		rule.instantiate();
		PackedInt32Array path;
		path.append(static_cast<int32_t>(index));
		rule->configure(Ref<HTMLStyleSheet>(const_cast<HTMLStyleSheet*>(this)), path);
		result.append(rule);
	}
	return result;
}

String HTMLStyleSheet::get_rule_text(const PackedInt32Array& p_path) const {
	auto* layout = get_layout();
	return layout ? layout->get_css_rule_text(id, p_path) : String();
}

PackedStringArray HTMLStyleSheet::get_rule_children(const PackedInt32Array& p_path) const {
	auto* layout = get_layout();
	return layout ? layout->get_css_rule_children(id, p_path) : PackedStringArray();
}

Error HTMLStyleSheet::set_rule_text(const PackedInt32Array& p_path, const String& p_text) {
	auto* layout = get_layout();
	return layout ? layout->set_css_rule_text(id, p_path, p_text) : ERR_INVALID_PARAMETER;
}

Error HTMLStyleSheet::edit_rule(const PackedInt32Array& p_parent_path, int64_t p_index, const String* p_text) {
	auto* layout = get_layout();
	return layout ? layout->edit_css_rule_nested(id, p_parent_path, p_index, p_text) : ERR_INVALID_PARAMETER;
}

uint64_t HTMLStyleSheet::get_rule_revision() const {
	auto* layout = get_layout();
	return layout ? layout->css_rule_revision(id) : 0;
}

Error HTMLStyleSheet::insert_rule(const String& p_rule, int64_t p_index) {
	auto* layout = get_layout();
	if (!layout) return ERR_INVALID_PARAMETER;
	if (is_read_only()) return ERR_UNAUTHORIZED;
	return layout->insert_rule(id, p_rule, p_index);
}

Error HTMLStyleSheet::delete_rule(int64_t p_index) {
	auto* layout = get_layout();
	if (!layout) return ERR_INVALID_PARAMETER;
	if (is_read_only()) return ERR_UNAUTHORIZED;
	return layout->delete_rule(id, p_index);
}
