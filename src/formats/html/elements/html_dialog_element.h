#pragma once

#include "html_element.h"

class HTMLDialogElement : public HTMLElement {
	GDCLASS(HTMLDialogElement, HTMLElement)
	friend class HTMLLabel;

	godot::Error open_dialog(bool p_modal);
	void emit_closed(const godot::String& p_value);

protected:
	static void _bind_methods();

public:
	bool is_open() const; // 返回对话框的打开属性
	void set_open(bool p_open); // 设置非模态打开属性或移除该属性
	bool is_modal() const; // 判断对话框是否位于当前文档的模态层
	godot::String get_return_value() const; // 返回最近设置的结果
	void set_return_value(const godot::String& p_value); // 设置对话框结果
	godot::Error show(); // 以非模态模式打开对话框
	godot::Error show_modal(); // 将已连接的对话框放入文档顶层
	godot::Error close(const godot::Variant& p_return_value = {}); // 关闭对话框并可选地更新结果
};
