#pragma once

#include "html_element.h"

class HTMLDetailsElement : public HTMLElement {
	GDCLASS(HTMLDetailsElement, HTMLElement)
	friend class docrender::HTMLDOM;

	bool toggle_pending = false;
	void queue_toggle();
	void emit_toggle();

protected:
	static void _bind_methods();

public:
	bool is_open() const; // 返回展开状态
	void set_open(bool p_open); // 展开或折叠内容
	godot::String get_name() const; // 返回互斥分组名
	void set_name(const godot::String& p_name); // 设置互斥分组名
};
