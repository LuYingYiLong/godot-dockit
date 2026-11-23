#include "html_css_rule.h"

#include "formats/html/css/html_cssom.h"
#include "formats/html/css/html_style_sheet.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>

using namespace godot;

namespace {
	struct ParsedStyleRule {
		dockit::css::raw_rule::ptr rule;
		dockit::css::raw_declaration::vector declarations;
		dockit::css::raw_rule::vector children;
	};

	ParsedStyleRule parse_style_rule(const String& p_text) {
		ParsedStyleRule result;
		result.rule = dockit::css::parse_cssom_rule(p_text);
		if (result.rule && result.rule->block.type == dockit::css::CURLY_BLOCK) {
			dockit::css::css_parser(result.rule->block.value).consume_style_block_contents(result.declarations, result.children);
		}
		return result;
	}

	void rebuild_style_rule(ParsedStyleRule& p_style) {
		std::string body = dockit::css::serialize_declarations(p_style.declarations);
		if (!body.empty() && !p_style.children.empty()) body += '\n';
		for (const auto& child : p_style.children) body += dockit::css::serialize_rule(*child) + '\n';
		p_style.rule->block = dockit::css::css_token(dockit::css::CURLY_BLOCK);
		p_style.rule->block.value = dockit::css::normalize(body);
	}

	std::string normalized_name(const String& p_name) {
		return dockit::css::property_name(p_name);
	}

	bool is_declaration_at_rule(const std::string& p_name) {
		return p_name == "counter-style" || p_name == "font-face" || p_name == "font-palette-values" ||
			p_name == "page" || p_name == "property" || p_name == "viewport";
	}

	dockit::css::raw_declaration::vector unique_declarations(const dockit::css::raw_declaration::vector& p_declarations) {
		dockit::css::raw_declaration::vector result;
		for (const auto& declaration : p_declarations) {
			const auto found = std::find_if(result.begin(), result.end(), [&](const auto& item) { return item.name == declaration.name; });
			if (found == result.end()) result.push_back(declaration);
			else *found = declaration;
		}
		return result;
	}
}

void CSSRule::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_valid"), &CSSRule::is_valid);
	ClassDB::bind_method(D_METHOD("get_index"), &CSSRule::get_index);
	ClassDB::bind_method(D_METHOD("get_type"), &CSSRule::get_type);
	ClassDB::bind_method(D_METHOD("get_name"), &CSSRule::get_name);
	ClassDB::bind_method(D_METHOD("get_selector_text"), &CSSRule::get_selector_text);
	ClassDB::bind_method(D_METHOD("get_css_text"), &CSSRule::get_css_text);
	ClassDB::bind_method(D_METHOD("set_css_text", "text"), &CSSRule::set_css_text);
	ClassDB::bind_method(D_METHOD("get_style"), &CSSRule::get_style);
	ClassDB::bind_method(D_METHOD("get_path"), &CSSRule::get_path);
	ClassDB::bind_method(D_METHOD("get_css_rules"), &CSSRule::get_css_rules);
	ClassDB::bind_method(D_METHOD("insert_rule", "rule", "index"), &CSSRule::insert_rule, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("delete_rule", "index"), &CSSRule::delete_rule);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "valid"), "", "is_valid");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "index"), "", "get_index");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "type"), "", "get_type");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "selector_text"), "", "get_selector_text");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "css_text"), "set_css_text", "get_css_text");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "style", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "CSSStyleDeclaration"), "", "get_style");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "css_rules", PROPERTY_HINT_ARRAY_TYPE, "CSSRule"), "", "get_css_rules");
}

void CSSRule::configure(const Ref<HTMLStyleSheet>& p_stylesheet, const PackedInt32Array& p_path) {
	stylesheet = p_stylesheet;
	path = p_path;
	revision = stylesheet.is_valid() ? stylesheet->get_rule_revision() : 0;
}

bool CSSRule::is_valid() const {
	return stylesheet.is_valid() && stylesheet->is_valid() && stylesheet->get_rule_revision() == revision &&
		!stylesheet->get_rule_text(path).is_empty();
}

int64_t CSSRule::get_index() const {
	if (!is_valid()) return -1;
	const int64_t path_size = path.size();
	return path_size > 0 ? path[path_size - 1] : -1;
}

String CSSRule::get_type() const {
	if (!is_valid()) return {};
	const auto parsed = dockit::css::parse_cssom_rule(stylesheet->get_rule_text(path));
	if (!parsed) return {};
	if (parsed->type == dockit::css::raw_rule::at) return "at";
	return parsed->block.type == dockit::css::CURLY_BLOCK ? "style" : "declaration-list";
}

String CSSRule::get_name() const {
	if (!is_valid()) return {};
	const auto parsed = dockit::css::parse_cssom_rule(stylesheet->get_rule_text(path));
	return parsed && parsed->type == dockit::css::raw_rule::at ? String::utf8(parsed->name.c_str()) : String();
}

String CSSRule::get_selector_text() const {
	if (!is_valid()) return {};
	const auto parsed = dockit::css::parse_cssom_rule(stylesheet->get_rule_text(path));
	return parsed && parsed->type == dockit::css::raw_rule::qualified ?
		String::utf8(dockit::css::get_repr(parsed->prelude).c_str()).strip_edges() : String();
}

String CSSRule::get_css_text() const { return is_valid() ? stylesheet->get_rule_text(path) : String(); }

void CSSRule::set_css_text(const String& p_text) {
	if (is_valid()) stylesheet->set_rule_text(path, p_text);
}

Ref<CSSStyleDeclaration> CSSRule::get_style() const {
	if (!has_style_declaration_block()) return {};
	Ref<CSSStyleDeclaration> result;
	result.instantiate();
	result->rule = Ref<CSSRule>(const_cast<CSSRule*>(this));
	return result;
}

PackedInt32Array CSSRule::get_path() const { return path; }

TypedArray<CSSRule> CSSRule::get_css_rules() const {
	TypedArray<CSSRule> result;
	if (!is_valid()) return result;
	const auto children = stylesheet->get_rule_children(path);
	for (int64_t index = 0; index < children.size(); ++index) {
		Ref<CSSRule> child;
		child.instantiate();
		PackedInt32Array child_path = path;
		child_path.append(static_cast<int32_t>(index));
		child->configure(stylesheet, child_path);
		result.append(child);
	}
	return result;
}

Error CSSRule::insert_rule(const String& p_rule, int64_t p_index) {
	return is_valid() ? stylesheet->edit_rule(path, p_index, &p_rule) : ERR_INVALID_PARAMETER;
}

Error CSSRule::delete_rule(int64_t p_index) {
	return is_valid() ? stylesheet->edit_rule(path, p_index, nullptr) : ERR_INVALID_PARAMETER;
}

void CSSRule::set_declarations_text(const String& p_text) {
	if (!is_valid()) return;
	auto style = parse_style_rule(get_css_text());
	if (!style.rule || style.rule->block.type != dockit::css::CURLY_BLOCK ||
		(style.rule->type != dockit::css::raw_rule::qualified && style.declarations.empty())) return;
	style.declarations = dockit::css::inline_declarations(p_text);
	rebuild_style_rule(style);
	stylesheet->set_rule_text(path, String::utf8(dockit::css::serialize_rule(*style.rule).c_str()));
}

bool CSSRule::has_style_declaration_block() const {
	if (!is_valid()) return false;
	const auto style = parse_style_rule(get_css_text());
	return style.rule && style.rule->block.type == dockit::css::CURLY_BLOCK &&
		(style.rule->type == dockit::css::raw_rule::qualified ||
			(style.rule->type == dockit::css::raw_rule::at && is_declaration_at_rule(style.rule->name)));
}

void CSSStyleDeclaration::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_valid"), &CSSStyleDeclaration::is_valid);
	ClassDB::bind_method(D_METHOD("get_length"), &CSSStyleDeclaration::get_length);
	ClassDB::bind_method(D_METHOD("item", "index"), &CSSStyleDeclaration::item);
	ClassDB::bind_method(D_METHOD("get_css_text"), &CSSStyleDeclaration::get_css_text);
	ClassDB::bind_method(D_METHOD("set_css_text", "text"), &CSSStyleDeclaration::set_css_text);
	ClassDB::bind_method(D_METHOD("get_property_value", "name"), &CSSStyleDeclaration::get_property_value);
	ClassDB::bind_method(D_METHOD("get_property_priority", "name"), &CSSStyleDeclaration::get_property_priority);
	ClassDB::bind_method(D_METHOD("set_property", "name", "value", "priority"), &CSSStyleDeclaration::set_property, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("remove_property", "name"), &CSSStyleDeclaration::remove_property);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "valid"), "", "is_valid");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "length"), "", "get_length");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "css_text"), "set_css_text", "get_css_text");
}

bool CSSStyleDeclaration::is_valid() const { return rule.is_valid() && rule->has_style_declaration_block(); }

int64_t CSSStyleDeclaration::get_length() const {
	if (!is_valid()) return 0;
	return unique_declarations(parse_style_rule(rule->get_css_text()).declarations).size();
}

String CSSStyleDeclaration::item(int64_t p_index) const {
	if (!is_valid()) return {};
	const auto declarations = unique_declarations(parse_style_rule(rule->get_css_text()).declarations);
	return p_index >= 0 && static_cast<size_t>(p_index) < declarations.size() ?
		String::utf8(declarations[static_cast<size_t>(p_index)].name.c_str()) : String();
}

String CSSStyleDeclaration::get_css_text() const {
	if (!is_valid()) return {};
	return String::utf8(dockit::css::serialize_declarations(
		unique_declarations(parse_style_rule(rule->get_css_text()).declarations)).c_str());
}

void CSSStyleDeclaration::set_css_text(const String& p_text) {
	if (is_valid()) rule->set_declarations_text(p_text);
}

String CSSStyleDeclaration::get_property_value(const String& p_name) const {
	if (!is_valid()) return {};
	const std::string name = normalized_name(p_name);
	const auto style = parse_style_rule(rule->get_css_text());
	const auto& declarations = style.declarations;
	for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
		if (it->name == name) return String::utf8(dockit::css::get_repr(it->value).c_str());
	}
	return {};
}

String CSSStyleDeclaration::get_property_priority(const String& p_name) const {
	if (!is_valid()) return {};
	const std::string name = normalized_name(p_name);
	const auto style = parse_style_rule(rule->get_css_text());
	const auto& declarations = style.declarations;
	for (auto it = declarations.rbegin(); it != declarations.rend(); ++it) {
		if (it->name == name) return it->important ? String("important") : String();
	}
	return {};
}

void CSSStyleDeclaration::set_property(const String& p_name, const String& p_value, const String& p_priority) {
	if (!is_valid() || (!p_priority.is_empty() && p_priority.to_lower() != "important")) return;
	const String candidate = p_name + String(": ") + p_value +
		(p_priority.is_empty() ? String(";") : String(" !important;"));
	const auto parsed = dockit::css::inline_declarations(candidate);
	if (parsed.size() != 1) return;
	auto style = parse_style_rule(rule->get_css_text());
	const auto name = normalized_name(p_name);
	bool replaced = false;
	for (auto it = style.declarations.begin(); it != style.declarations.end();) {
		if (it->name != name) {
			++it;
		}
		else if (!replaced) {
			*it = parsed.front();
			replaced = true;
			++it;
		}
		else {
			it = style.declarations.erase(it);
		}
	}
	if (!replaced) style.declarations.push_back(parsed.front());
	rebuild_style_rule(style);
	rule->stylesheet->set_rule_text(rule->path, String::utf8(dockit::css::serialize_rule(*style.rule).c_str()));
}

String CSSStyleDeclaration::remove_property(const String& p_name) {
	if (!is_valid()) return {};
	const auto name = normalized_name(p_name);
	auto style = parse_style_rule(rule->get_css_text());
	String previous;
	bool found = false;
	for (auto it = style.declarations.rbegin(); it != style.declarations.rend(); ++it) {
		if (it->name == name) {
			previous = String::utf8(dockit::css::get_repr(it->value).c_str());
			found = true;
			break;
		}
	}
	style.declarations.erase(std::remove_if(style.declarations.begin(), style.declarations.end(),
		[&](const auto& item) { return item.name == name; }), style.declarations.end());
	if (found) {
		rebuild_style_rule(style);
		rule->stylesheet->set_rule_text(rule->path, String::utf8(dockit::css::serialize_rule(*style.rule).c_str()));
	}
	return previous;
}
