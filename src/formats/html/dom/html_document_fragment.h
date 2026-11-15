#pragma once

#include "formats/html/elements/html_element.h"

class HTMLDocumentFragment : public godot::RefCounted {
	GDCLASS(HTMLDocumentFragment, godot::RefCounted)

	friend class docrender::HTMLDOM;
	std::shared_ptr<HTMLElement::DetachedTree> tree;

protected:
	static void _bind_methods();

public:
	godot::Error append_child(const godot::Ref<HTMLElement>& p_child);	// 将同一布局中的元素移到片段末尾
	godot::TypedArray<HTMLElement> get_children() const;				// 按顺序返回片段的直接子元素
};
