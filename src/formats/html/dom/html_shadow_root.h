#pragma once
#include <vector>

#include "html_document_fragment.h"

class HTMLShadowRoot : public HTMLDocumentFragment {
	GDCLASS(HTMLShadowRoot, HTMLDocumentFragment)
	
	friend class dockit::HTMLDOM;
	std::weak_ptr<litehtml::element> host;
	godot::String mode;
	godot::String slot_assignment;
	bool delegates_focus = false;
	std::vector<std::weak_ptr<litehtml::element>> previous_slots;

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLElement> get_host() const;													// 获取组件挂载的宿主元素
	godot::String get_mode() const { return mode; }												// 获取模式
	godot::String get_slot_assignment() const { return slot_assignment; }						// 获取插槽分配结果
	bool get_delegates_focus() const { return delegates_focus; }								// 检查影子根是否将焦点委托给内部元素
};

class HTMLSlotElement : public HTMLElement {
	GDCLASS(HTMLSlotElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::String get_name() const { return get_attribute("name"); }							// 获取名称
	void set_name(const godot::String& p_name) { set_attribute("name", p_name); }				// 设置名称
	godot::TypedArray<HTMLNode> assigned_nodes(bool p_flatten = false) const;					// 获取分配到插槽的节点
	godot::TypedArray<HTMLElement> assigned_elements(bool p_flatten = false) const;				// 获取分配到插槽的元素
	godot::Error assign(const godot::Array& p_nodes);											// 将给定节点分配到插槽
};
