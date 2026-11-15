#pragma once

#include "formats/html/elements/html_element.h"

namespace docrender { class HTMLTemplateNode; }

class HTMLDocumentFragment : public godot::RefCounted {
	GDCLASS(HTMLDocumentFragment, godot::RefCounted)

	friend class docrender::HTMLDOM;
	friend class docrender::HTMLTemplateNode;
	std::shared_ptr<HTMLElement::DetachedTree> tree;

protected:
	static void _bind_methods();

public:
	godot::Error append_child(const godot::Ref<HTMLElement>& p_child);	// 将同一布局中的元素移到片段末尾
	godot::TypedArray<HTMLElement> get_children() const;				// 按顺序返回片段的直接子元素
	godot::Ref<HTMLDocumentFragment> clone_node(bool p_deep = false) const;	// 创建独立片段，可选复制其中的全部内容
	godot::Ref<HTMLElement> query_selector(const godot::String& p_selector) const;	// 返回片段中第一个匹配选择器的元素
	godot::TypedArray<HTMLElement> query_selector_all(const godot::String& p_selector) const;	// 按树顺序返回片段中所有匹配元素的快照
	godot::Ref<HTMLElement> get_element_by_id(const godot::String& p_id) const;	// 按区分大小写的原始 ID 查找片段中的第一个元素
};
