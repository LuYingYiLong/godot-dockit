#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include "rst_reference_index.h"

using namespace godot;

class RSTDocument : public Resource {
	GDCLASS(RSTDocument, Resource);

private:
	String text;
	String source_path;
	Ref<RSTReferenceIndex> reference_index;

protected:
	static void _bind_methods();

public:
	void set_text(const String& p_text);														// 设置文本
	String get_text() const;																	// 获取文本
	void set_source_path(const String& p_source_path);											// 设置源路径
	String get_source_path() const;																// 获取源路径
	void set_reference_index(const Ref<RSTReferenceIndex>& p_reference_index);					// 设置文档使用的 RST 引用索引
	Ref<RSTReferenceIndex> get_reference_index() const;											// 获取文档使用的 RST 引用索引

	String resolve_path(const String& p_path) const;											// 根据文档来源路径解析相对资源路径
};
