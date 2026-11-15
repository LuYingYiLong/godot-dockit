#pragma once

#include "html_element.h"

class HTMLAnchorElement : public HTMLElement {
	GDCLASS(HTMLAnchorElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::String get_href() const; // 返回源 href 属性
	void set_href(const godot::String& p_href); // 写入 href 属性
};
