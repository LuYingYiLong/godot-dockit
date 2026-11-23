#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

class RSTReferenceIndex : public godot::Resource {
	GDCLASS(RSTReferenceIndex, godot::Resource)

	godot::Dictionary document_targets;
	godot::Dictionary reference_targets;
	godot::String source_fingerprint;

protected:
	static void _bind_methods();

public:
	void set_document_targets(const godot::Dictionary& p_targets);								// 设置文档路径到目标标签的索引
	godot::Dictionary get_document_targets() const;												// 获取文档路径到目标标签的索引
	void set_reference_targets(const godot::Dictionary& p_targets);								// 设置 RST 标签到文档目标的索引
	godot::Dictionary get_reference_targets() const;											// 获取 RST 标签到文档目标的索引
	void set_source_fingerprint(const godot::String& p_fingerprint);							// 设置用于校验文档来源的指纹
	godot::String get_source_fingerprint() const;												// 获取用于校验文档来源的指纹

	godot::String resolve_document(const godot::String& p_path) const;							// 解析相对于文档的 RST 文档路径
	godot::String resolve_reference(
		const godot::String& p_source_path,
		const godot::String& p_label
	) const;																					// 根据来源文档和标签解析 RST 引用目标

	static godot::String get_generated_path();													// 获取自动生成的 RST 引用索引资源路径
	static godot::Ref<RSTReferenceIndex> rebuild_from_project_settings(
		bool p_save_resource = true,
		bool p_emit_warnings = true
	);																							// 根据项目设置重建 RST 文档引用索引
};
