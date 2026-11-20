#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cstdint>

class HTMLStyleSheet;
class CSSStyleDeclaration;

class CSSRule : public godot::RefCounted {
	GDCLASS(CSSRule, godot::RefCounted)
	friend class HTMLStyleSheet;
	friend class CSSStyleDeclaration;

	godot::Ref<HTMLStyleSheet> stylesheet;
	godot::PackedInt32Array path;
	uint64_t revision = 0;

	void configure(const godot::Ref<HTMLStyleSheet>& p_stylesheet, const godot::PackedInt32Array& p_path);
	void set_declarations_text(const godot::String& p_text);
	bool has_style_declaration_block() const;

protected:
	static void _bind_methods();

public:
	bool is_valid() const;
	int64_t get_index() const;
	godot::String get_type() const;
	godot::String get_name() const;
	godot::String get_selector_text() const;
	godot::String get_css_text() const;
	void set_css_text(const godot::String& p_text);
	godot::Ref<CSSStyleDeclaration> get_style() const;
	godot::PackedInt32Array get_path() const;
	godot::TypedArray<CSSRule> get_css_rules() const;
	godot::Error insert_rule(const godot::String& p_rule, int64_t p_index = 0);
	godot::Error delete_rule(int64_t p_index);
};

class CSSStyleDeclaration : public godot::RefCounted {
	GDCLASS(CSSStyleDeclaration, godot::RefCounted)
	friend class CSSRule;

	godot::Ref<CSSRule> rule;

protected:
	static void _bind_methods();

public:
	bool is_valid() const;
	int64_t get_length() const;
	godot::String item(int64_t p_index) const;
	godot::String get_css_text() const;
	void set_css_text(const godot::String& p_text);
	godot::String get_property_value(const godot::String& p_name) const;
	godot::String get_property_priority(const godot::String& p_name) const;
	void set_property(const godot::String& p_name, const godot::String& p_value,
		const godot::String& p_priority = "");
	godot::String remove_property(const godot::String& p_name);
};
