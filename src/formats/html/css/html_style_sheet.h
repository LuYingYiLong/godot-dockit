#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cstdint>
#include <memory>

namespace dockit { class HTMLStylesheetNode; }
class HTMLElement;
class HTMLLayout;
class CSSRule;
class CSSStyleDeclaration;

class HTMLStyleSheet : public godot::RefCounted {
	GDCLASS(HTMLStyleSheet, godot::RefCounted)
	friend class HTMLLayout;
	friend class CSSRule;
	friend class CSSStyleDeclaration;

public:
	enum SourceKind { OWNER_SOURCE, EXTRA_SOURCE, DEFAULT_SOURCE };

private:

	godot::ObjectID layout_id;
	std::weak_ptr<dockit::HTMLStylesheetNode> owner;
	uint64_t parse_version = 0;
	uint64_t source_version = 0;
	int64_t id = 0;
	SourceKind source_kind = OWNER_SOURCE;

	HTMLLayout* get_layout() const;
	godot::String get_rule_text(const godot::PackedInt32Array& p_path) const;
	godot::PackedStringArray get_rule_children(const godot::PackedInt32Array& p_path) const;
	godot::Error set_rule_text(const godot::PackedInt32Array& p_path, const godot::String& p_text);
	godot::Error edit_rule(const godot::PackedInt32Array& p_parent_path, int64_t p_index,
		const godot::String* p_text);
	uint64_t get_rule_revision() const;

protected:
	static void _bind_methods();

public:
	bool is_valid() const; // 检查句柄是否仍指向当前样式表
	int64_t get_id() const; // 返回兼容现有 CSSOM 方法的数字 ID
	godot::Ref<HTMLElement> get_owner_node() const; // 返回关联的 link 或 style 元素
	godot::String get_href() const; // 返回外部样式表的实际资源路径
	godot::String get_media() const; // 返回样式表当前的媒体条件
	bool is_disabled() const; // 检查样式表当前是否已禁用
	bool is_read_only() const;
	godot::String get_origin() const;
	godot::PackedStringArray get_css_rules() const; // 返回顶层规则快照
	godot::TypedArray<CSSRule> get_rules() const;
	godot::Error insert_rule(const godot::String& p_rule, int64_t p_index = 0); // 插入一条顶层规则
	godot::Error delete_rule(int64_t p_index); // 删除指定的顶层规则
};
