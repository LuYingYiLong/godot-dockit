#pragma once

#include "html_button_element.h"

class HTMLFormElement : public HTMLElement {
	GDCLASS(HTMLFormElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::TypedArray<HTMLElement> get_controls() const; // 按树顺序返回关联的表单控件
	godot::TypedArray<godot::Dictionary> get_form_data(const godot::Ref<HTMLButtonElement>& p_submitter = {}) const; // 返回有序的表单名称和值
	godot::Error reset(); // 恢复关联控件的默认状态
};

class HTMLLabelElement : public HTMLElement {
	GDCLASS(HTMLLabelElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLElement> get_control() const; // 返回当前关联的控件
	godot::String get_html_for() const; // 返回关联控件的 ID
	void set_html_for(const godot::String& p_value); // 设置关联控件的 ID
};

class HTMLFieldSetElement : public HTMLElement {
	GDCLASS(HTMLFieldSetElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLFormElement> get_form() const; // 返回所属表单
	bool is_disabled() const; // 返回禁用属性
	void set_disabled(bool p_value); // 设置禁用属性
};

class HTMLLegendElement : public HTMLElement {
	GDCLASS(HTMLLegendElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLFormElement> get_form() const; // 返回分组所属表单
};

class HTMLSelectElement : public HTMLElement {
	GDCLASS(HTMLSelectElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::TypedArray<HTMLElement> get_options() const; // 按树顺序返回选项
	godot::Ref<HTMLFormElement> get_form() const; // 返回所属表单
	godot::String get_name() const; // 返回提交字段名
	void set_name(const godot::String& p_value); // 设置提交字段名
	bool is_disabled() const; // 返回禁用属性
	void set_disabled(bool p_value); // 设置禁用属性
	bool is_required() const; // 返回必填属性
	void set_required(bool p_value); // 设置必填属性
	godot::String get_value() const; // 返回当前值
	void set_value(const godot::String& p_value); // 设置当前值
	int64_t get_selected_index() const; // 返回选中的零基索引
	void set_selected_index(int64_t p_value); // 设置选中的零基索引
};

class HTMLOptionElement : public HTMLElement {
	GDCLASS(HTMLOptionElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::String get_value() const; // 返回当前值
	void set_value(const godot::String& p_value); // 设置当前值
	godot::String get_label() const; // 返回显示标签
	void set_label(const godot::String& p_value); // 设置显示标签
	bool is_disabled() const; // 返回禁用属性
	void set_disabled(bool p_value); // 设置禁用属性
	bool is_selected() const; // 返回当前选中状态
	void set_selected(bool p_value); // 设置当前选中状态
	bool is_default_selected() const; // 返回默认选中属性
	void set_default_selected(bool p_value); // 设置默认选中属性
};

class HTMLOptGroupElement : public HTMLElement {
	GDCLASS(HTMLOptGroupElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::String get_label() const; // 返回显示标签
	void set_label(const godot::String& p_value); // 设置显示标签
	bool is_disabled() const; // 返回禁用属性
	void set_disabled(bool p_value); // 设置禁用属性
};
