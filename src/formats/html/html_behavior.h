#pragma once

#include "formats/html/dom/html_node.h"
#include "nodes/html_label.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

class HTMLBehavior : public godot::RefCounted {
	GDCLASS(HTMLBehavior, godot::RefCounted)

	friend class HTMLLabel;
	godot::Ref<HTMLDocumentNode> document;

	GDVIRTUAL1(_attach_document, godot::Ref<HTMLDocumentNode>);
	GDVIRTUAL1(_attach, HTMLLabel *);
	GDVIRTUAL0(_detach);

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLDocumentNode> get_document() const; // 返回当前挂载的实时文档节点
};
