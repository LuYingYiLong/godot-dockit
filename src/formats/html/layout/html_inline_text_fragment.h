#pragma once

#include "formats/html/text/html_text_layout.h"

#include <litehtml/el_text.h>

namespace dockit {
	class HTMLInlineTextFragment final : public litehtml::el_text {
		std::shared_ptr<litehtml::render_item> original;
		std::shared_ptr<const godot::String> rendered;
		int64_t run_start;
		int64_t start;
		int64_t end;
		bool lexical_break_before;
		bool hyphenation_fallback;

	public:
		HTMLInlineTextFragment(const std::shared_ptr<litehtml::render_item>& p_original,
			const std::shared_ptr<const godot::String>& p_rendered, int64_t p_run_start,
			const HTMLTextCluster& p_cluster, bool p_lexical_break_before,
			bool p_hyphenation_fallback = false);
		const std::shared_ptr<litehtml::render_item>& original_renderer() const {
			return original;
		}																						// 获取文本片段对应的原始渲染项
		const godot::String& shaping_text() const { return *rendered; }							// 获取参与同一文本塑形运行的内容
		int64_t shaping_start() const { return start - run_start; }								// 获取片段在塑形运行中的起始偏移
		int64_t shaping_end() const { return end - run_start; }									// 获取片段在塑形运行中的结束偏移
		int64_t source_start() const { return start; }											// 获取片段在源文本中的起始偏移
		int64_t source_end() const { return end; }												// 获取片段在源文本中的结束偏移
		bool has_lexical_break_before() const { return lexical_break_before; }					// 检查文本片段前是否存在词法断点
		bool is_automatic_hyphen() const { return start == end; }								// 检查字符是否为自动断词插入的连字符
		bool is_hyphenation_fallback() const { return hyphenation_fallback; }					// 检查断词是否使用了回退处理
		bool shares_shaping_run(
			const HTMLInlineTextFragment& p_other
		) const {
			return rendered == p_other.rendered;
		}																						// 检查两个片段是否属于同一文本塑形运行
		void extend_to(int64_t p_end) { end = p_end; }											// 将文本片段的结束位置扩展到指定偏移
		void draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y, const litehtml::position* p_clip,
			const std::shared_ptr<litehtml::render_item>& p_item) override;						// 绘制行内文本片段
	};
}
