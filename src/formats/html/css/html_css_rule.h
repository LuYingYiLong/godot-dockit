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
	bool is_valid() const;																		// 检查当前对象是否有效
	int64_t get_index() const;																	// 获取index
	godot::String get_type() const;																// 获取类型
	godot::String get_name() const;																// 获取名称
	godot::String get_selector_text() const;													// 获取选择器文本
	godot::String get_css_text() const;															// 获取CSS文本
	void set_css_text(const godot::String& p_text);												// 设置CSS文本
	godot::Ref<CSSStyleDeclaration> get_style() const;											// 获取样式
	godot::PackedInt32Array get_path() const;													// 获取路径
	godot::TypedArray<CSSRule> get_css_rules() const;											// 获取CSS规则
	godot::Error insert_rule(const godot::String& p_rule, int64_t p_index = 0);					// 在指定索引处插入 CSS 规则
	godot::Error delete_rule(int64_t p_index);													// 删除规则
};

class CSSStyleDeclaration : public godot::RefCounted {
	GDCLASS(CSSStyleDeclaration, godot::RefCounted)
	
	friend class CSSRule;
	godot::Ref<CSSRule> rule;

protected:
	static void _bind_methods();

public:
	bool is_valid() const;																		// 检查当前对象是否有效
	int64_t get_length() const;																	// 获取长度
	godot::String item(int64_t p_index) const;													// 获取指定索引处的 CSS 规则文本
	godot::String get_css_text() const;															// 获取CSS文本
	void set_css_text(const godot::String& p_text);												// 设置CSS文本
	godot::String get_property_value(const godot::String& p_name) const;						// 获取属性值
	godot::String get_property_priority(const godot::String& p_name) const;						// 获取属性优先级
	void set_property(
		const godot::String& p_name,
		const godot::String& p_value,
		const godot::String& p_priority = ""
	);																							// 设置属性
	godot::String remove_property(const godot::String& p_name);									// 移除属性
};
