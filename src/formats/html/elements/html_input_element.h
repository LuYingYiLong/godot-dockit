#pragma once

#include "html_element.h"

namespace docrender { class HTMLInputNode; }

class HTMLInputElement : public HTMLElement {
	GDCLASS(HTMLInputElement, HTMLElement)
	friend class HTMLLabel;
	friend class HTMLLabelCanvas;
	friend class HTMLLayout;

	std::shared_ptr<docrender::HTMLInputNode> get_input() const;

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLFormElement> get_form() const; // 返回所属表单
	godot::String get_value() const; // 返回输入框的当前值
	void set_value(const godot::String& p_value); // 设置当前值且不发出用户编辑信号
	bool is_checked() const; // 返回复选框的当前选中状态
	void set_checked(bool p_checked); // 设置当前选中状态且不修改 checked 属性
	int64_t get_selection_start() const; // 返回文本选区起点
	int64_t get_selection_end() const; // 返回文本选区终点
	void set_selection_range(int64_t p_start, int64_t p_end); // 按 Godot 字符索引设置文本选区
};
