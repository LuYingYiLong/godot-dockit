#pragma once

#include <litehtml/types.h>

#include <godot_cpp/variant/string.hpp>

namespace litehtml {
	class element;
	class render_item;

// 处理文本自动间距
	std::shared_ptr<element> inline_boundary_owner(
		const element& p_before,
		const element& p_after
	);																							// 查找两个行内元素之间边界的所属元素
	bool separates_autospace(const render_item& p_inline, bool p_start, pixel_t p_basis);		// 检查两个行内元素之间是否需要自动间距
	bool is_autospace_fragment_boundary(
		const element& p_owner,
		const godot::String& p_text,
		int64_t p_offset
	);																							// 检查位置是否为自动间距片段边界
	pixel_t text_autospace_gap(const element& p_before, const element& p_after);				// 计算两个文本元素之间的自动间距
	bool has_non_breaking_autospace(const element& p_before, const element& p_after);			// 检查元素之间是否包含不换行自动间距
	void prepare_autospace_replacements(
		const std::shared_ptr<render_item>& p_root,
		pixel_t p_basis
	);																							// 根据文字边界准备自动间距替换片段
}
