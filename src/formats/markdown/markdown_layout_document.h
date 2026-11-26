#pragma once

#include "markdown_parser.h"
#include "formats/html/text/html_font_config.h"
#include "formats/html/css/css_style_sheet.h"
#include <godot_cpp/classes/control.hpp>
#include <map>
#include <utility>

struct MarkdownThemeCache;
class MermaidRenderCache;

namespace dockit {

struct MarkdownSourceBlock {
	uint64_t id = 0;
	uint64_t content_hash = 0;
	MarkdownBlockType type = MARKDOWN_BLOCK_PARAGRAPH;
	int32_t line = 0;
	int32_t end_line = 0;
	int64_t source_start = 0;
	int64_t source_end = 0;
	godot::Rect2 bounds;
	godot::String html;
	godot::Ref<HTMLElement> element;
	std::vector<godot::Ref<HTMLNode>> nodes;
	std::vector<MarkdownFootnoteDefinition> footnotes;
};

struct MarkdownSourceLineMap {
	int64_t source_start = 0;
	int64_t first_source_offset = -1;
	int64_t first_rendered_offset = -1;
	int64_t last_source_offset = -1;
	int64_t last_rendered_offset = -1;
	int64_t fallback_rendered_offset = -1;
	double anchor_y = 0.0;
	bool has_geometry = false;
	bool has_text_geometry = false;
	bool has_object_bounds = false;
	godot::Rect2 object_bounds;
	std::vector<int64_t> rendered_offsets;
};

class MarkdownLayoutDocument {
	godot::Ref<HTMLLayout> layout;
	std::vector<MarkdownSourceBlock> source_blocks;
	std::vector<MarkdownSourceLineMap> source_lines;
	std::map<std::pair<double, int32_t>, int32_t> visual_source_lines;
	godot::Dictionary textures;
	godot::String theme_css;
	godot::String resource_base;
	godot::String author_css;
	godot::HashMap<godot::String, int32_t> footnotes;
	uint64_t next_id = 1;
	uint64_t task_checked_icon_id = 0;
	uint64_t task_unchecked_icon_id = 0;
	int32_t rebuilt_nodes = 0;
	int32_t retained_nodes = 0;
	godot::Vector2i last_viewport_size;
	bool has_last_viewport_size = false;

	godot::String render_inline(const godot::String& p_text, const MarkdownParseOptions& p_options,
		const MarkdownThemeCache& p_theme, MermaidRenderCache& p_mermaid);
	godot::String render_block(const MarkdownBlock& p_block, const MarkdownParseOptions& p_options,
		const MarkdownThemeCache& p_theme, MermaidRenderCache& p_mermaid);
	void rebuild_source_map(const godot::String& p_source, const std::vector<int64_t>& p_line_starts,
		int32_t p_prefix_block_count);

public:
	godot::Error update(const std::vector<MarkdownBlock>& p_blocks, const godot::String& p_source,
		const std::vector<int64_t>& p_line_starts,
		const MarkdownParseOptions& p_options,
		const MarkdownThemeCache& p_theme, godot::Control* p_owner, MermaidRenderCache& p_mermaid,
		const godot::Vector2i& p_viewport, const godot::String& p_base, const godot::Ref<CSSStyleSheet>& p_stylesheet);
	const godot::Ref<HTMLLayout>& get_layout() const { return layout; } // 获取全文共享的 HTML 排版对象
	double source_line_to_y(double p_line, int32_t p_line_count) const; // 将源码行映射为文档纵向位置
	double y_to_source_line(double p_y, int32_t p_line_count) const; // 将文档纵向位置映射为源码行
	void clear(); // 清除文档、布局与资源缓存
	int32_t get_rebuilt_nodes() const { return rebuilt_nodes; } // 获取最近一次更新替换的顶层节点数
	int32_t get_retained_nodes() const { return retained_nodes; } // 获取最近一次更新复用的顶层节点数
};

}
