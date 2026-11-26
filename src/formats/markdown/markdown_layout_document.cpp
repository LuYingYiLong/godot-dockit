#include "markdown_layout_document.h"

#include "markdown_html.h"
#include "nodes/markdown_label.h"
#include "utils/style_box_css_helper.h"
#include "formats/html/elements/html_element.h"
#include "formats/html/dom/html_document_fragment.h"
#include "markdown_formula_texture.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <unordered_map>

using namespace godot;

namespace dockit {
namespace {
	String escape(String p_text) {
		return p_text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;");
	}
	String css_color(const Color& p_color) { return "#" + p_color.to_html(true); }
	String font_rule(const String& p_selector, int p_size, const Color& p_color) {
		return p_selector + String("{font-size:") + String::num_int64(p_size) + "px;color:" + css_color(p_color) + ";}";
	}
	String style_rule(const String& p_selector, const Ref<StyleBox>& p_style, Dictionary& r_textures) {
		if (p_style.is_null()) return {};
		const Dictionary converted = StyleBoxCSSHelper::stylebox_to_css(p_style);
		if (int(converted.get("error", FAILED)) != OK) return {};
		r_textures.merge(converted.get("textures", Dictionary()), true);
		return p_selector + String("{") + String(converted.get("css", "")) + "}";
	}
	String task_marker_html(bool p_checked, const MarkdownThemeCache& p_theme, Dictionary& r_textures) {
		const Ref<Texture2D> icon = p_checked ? p_theme.icon.task_checked : p_theme.icon.task_unchecked;
		const int32_t size = std::max(1, p_theme.font_size.list_marker);
		if (icon.is_valid()) {
			const String key = "res://__dockit_runtime/markdown-task-" + String::num_uint64(icon->get_instance_id()) + ".png";
			r_textures[key] = icon;
			return "<img class=\"dockit-task-marker\" src=\"" + key + "\" alt=\"\" width=\"" +
				String::num_int64(size) + "\" height=\"" + String::num_int64(size) + "\">";
		}
		return String("<span class=\"dockit-task-marker dockit-task-marker-fallback\" role=\"checkbox\" aria-checked=\"") +
			(p_checked ? "true" : "false") + "\" width=\"" + String::num_int64(size) + "\" height=\"" +
			String::num_int64(size) + "\">" + (p_checked ? "✓" : String()) + "</span>";
	}
	Rect2 source_bounds(const MarkdownSourceBlock& p_block) {
		Rect2 bounds;
		for (const auto& node : p_block.nodes) {
			const Ref<HTMLElement> element = node;
			if (element.is_null()) continue;
			const Rect2 rect = element->get_bounding_rect();
			if (rect.has_area()) bounds = bounds.has_area() ? bounds.merge(rect) : rect;
		}
		return bounds;
	}
	int32_t source_line_at(const std::vector<int64_t>& p_starts, int64_t p_offset) {
		if (p_starts.empty()) return 0;
		const auto found = std::upper_bound(p_starts.begin(), p_starts.end(), p_offset);
		return CLAMP(static_cast<int32_t>(found - p_starts.begin()) - 1, 0, static_cast<int32_t>(p_starts.size()) - 1);
	}
	std::optional<Rect2> text_character_rect(const Ref<HTMLLayout>& p_layout, int64_t p_offset) {
		if (p_layout.is_null()) return std::nullopt;
		const TypedArray<Rect2> rects = p_layout->get_selection_rects(p_offset, p_offset + 1);
		for (int64_t i = 0; i < rects.size(); ++i) {
			const Rect2 rect = rects[i];
			if (rect.has_area()) return rect;
		}
		return std::nullopt;
	}
	String theme_stylesheet(const MarkdownThemeCache& p_theme, Dictionary& r_textures) {
		String css = "html,body{margin:0;padding:0;background:transparent;} body{overflow-wrap:anywhere;} "
			"p,ul,ol,blockquote,pre,table,figure{margin:0 0 10px;} img{max-width:100%;} "
			"pre{white-space:pre;tab-size:4;overflow:auto;} code{white-space:pre-wrap;tab-size:4;} "
			"pre code{font:inherit;color:inherit;white-space:inherit;background:none;border:0;padding:0;} "
			"table{width:100%;border-collapse:separate;border-spacing:0;} th,td{padding:4px 6px;text-align:left;vertical-align:top;} "
			"blockquote{margin-left:0;margin-right:0;} blockquote blockquote{margin-top:8px;} "
			"hr{border:0;height:1px;} .dockit-footnotes{list-style:none;padding-left:0;} .dockit-footnote-ref{font-size:.75em;vertical-align:super;} "
			".dockit-formula{display:inline-block;vertical-align:middle;} .dockit-display-math{text-align:center;} .dockit-display-formula{display:inline-block;vertical-align:middle;} "
			"ul.dockit-task-list{list-style:none;} ul.dockit-task-list>li{list-style:none;position:relative;}";
		css += "code{font-family:monospace;} pre,pre code{font-family:dockit-code-block;} th{font-family:dockit-table-header;} td{font-family:dockit-table-cell;} .dockit-footnotes,.dockit-footnotes>li{font-family:dockit-footnote-text;} .dockit-footnote-ref{font-family:dockit-footnote-ref;}";
		css += font_rule("html", p_theme.font_size.text, p_theme.color.text);
		css += font_rule("code", p_theme.font_size.code, p_theme.color.code_text);
		css += font_rule("pre", p_theme.font_size.code_block, p_theme.color.code_block);
		css += font_rule("th", p_theme.font_size.table_header, p_theme.color.table_header_font);
		css += font_rule("td", p_theme.font_size.table_cell, p_theme.color.table_cell_font);
		css += font_rule(".dockit-footnotes,.dockit-footnotes>li", p_theme.font_size.footnote_text, p_theme.color.footnote_text);
		css += font_rule("a.dockit-footnote-ref:link", p_theme.font_size.footnote_ref, p_theme.color.footnote_ref);
		css += font_rule("a.dockit-footnote-backref:link", p_theme.font_size.footnote_text, p_theme.color.footnote_text);
		css += "a:link{color:" + css_color(p_theme.color.link) + ";} mark{color:" + css_color(p_theme.color.highlight_font) + ";background:" + css_color(p_theme.color.highlight) + ";}";
		for (int i = 0; i < 6; ++i) {
			const String n = String::num_int64(i + 1);
			css += font_rule("h" + n, p_theme.font_size.heading[i], p_theme.color.heading[i]);
			css += "h" + n + "{font-family:dockit-heading-" + n + ";}";
		}
		css += style_rule("code", p_theme.stylebox.inline_code, r_textures);
		css += style_rule("pre", p_theme.stylebox.code_block_panel, r_textures);
		css += style_rule("blockquote", p_theme.stylebox.blockquote, r_textures);
		css += style_rule("blockquote blockquote", p_theme.stylebox.blockquote_nested, r_textures);
		css += style_rule("table", p_theme.stylebox.table_panel, r_textures);
		css += style_rule("td", p_theme.stylebox.table_cell_panel, r_textures);
		css += style_rule("th", p_theme.stylebox.table_header_panel, r_textures);
		if (p_theme.constant.table_striped) css += style_rule("tbody tr:nth-child(even) td", p_theme.stylebox.table_striped_panel, r_textures);
		css += style_rule("mark", p_theme.stylebox.highlight, r_textures);
		css += style_rule("hr", p_theme.stylebox.separator, r_textures);
		css += style_rule(".dockit-footnote-ref", p_theme.stylebox.footnote_ref, r_textures);
		const int32_t footnote_line_height = p_theme.font.footnote_text.is_valid()
			? int32_t(std::ceil(p_theme.font.footnote_text->get_height(p_theme.font_size.footnote_text))) + p_theme.constant.footnote_line_separation
			: p_theme.font_size.footnote_text + p_theme.constant.footnote_line_separation;
		css += ".dockit-footnotes{padding-top:" + String::num_int64(p_theme.constant.footnote_space_before) + "px;margin-bottom:" +
			String::num_int64(p_theme.constant.paragraph_separation) + "px;} .dockit-footnotes>li{line-height:" +
			String::num_int64(footnote_line_height) + "px;} a.dockit-footnote-backref:link{color:" + css_color(p_theme.color.footnote_text) + ";} ";
		css += "ul.dockit-task-list{list-style:none;padding-left:" + String::num_int64(p_theme.constant.list_indent) + "px;} .dockit-task-marker{position:absolute;left:-" + String::num_int64(p_theme.font_size.list_marker + p_theme.constant.list_marker_gap) +
			"px;top:0;width:" + String::num_int64(p_theme.font_size.list_marker) + "px;height:" +
			String::num_int64(p_theme.font_size.list_marker) + "px;} .dockit-task-marker-fallback{box-sizing:border-box;border:1px solid " +
			css_color(p_theme.color.list_marker) + ";text-align:center;line-height:" + String::num_int64(p_theme.font_size.list_marker - 2) + "px;}";
		css += "p,ul,ol,blockquote,pre,table{margin-bottom:" + String::num_int64(p_theme.constant.paragraph_separation) + "px;}";
		css += ".dockit-display-math{margin:0 0 " + String::num_int64(p_theme.constant.paragraph_separation) + "px;}";
		return css;
	}
}

String MarkdownLayoutDocument::render_inline(const String& p_text, const MarkdownParseOptions& p_options,
	const MarkdownThemeCache& p_theme, MermaidRenderCache& p_mermaid) {
	const auto parsed = parse_inline_spans(p_text, p_options);
	String output;
	bool inside_html_link = false;
	for (const auto& span : parsed.spans) {
		output += span.html_open;
		if (span.html_open.to_lower().contains("<a ")) inside_html_link = true;
		String text = escape(span.text);
		if (!span.latex.is_empty()) {
			const String key = "res://__dockit_runtime/formula-" + String::num_uint64(span.latex.hash()) + "-" +
				String::num_int64(p_theme.font_size.text) + "-" + p_theme.color.text.to_html(true) + ".png";
			if (!textures.has(key)) {
				Ref<MarkdownFormulaTexture> texture;
				texture.instantiate();
				if (texture->configure(span.latex, p_theme.font_size.text, p_theme.color.text, false) == OK) textures[key] = texture;
			}
			if (textures.has(key)) text = "<img class=\"dockit-formula\" src=\"" + key + "\" alt=\"" + escape(span.text) + "\">";
		}
		else if (!span.image_uri.is_empty()) {
			text = "<img src=\"" + escape(span.image_uri) + "\" alt=\"" + escape(span.text) + "\" title=\"" + escape(span.image_title) + "\"";
			if (span.image_size.x > 0) text += " width=\"" + String::num_int64(span.image_size.x) + "\"";
			if (span.image_size.y > 0) text += " height=\"" + String::num_int64(span.image_size.y) + "\"";
			text += ">";
		}
		else if (span.footnote_ref) {
			if (!footnotes.has(span.footnote_id)) footnotes[span.footnote_id] = footnotes.size() + 1;
			text = "<a class=\"dockit-footnote-ref\" id=\"dockit-ref-" + escape(span.footnote_id) + "\" href=\"#dockit-note-" +
				escape(span.footnote_id) + "\">" + String::num_int64(footnotes[span.footnote_id]) + "</a>";
		}
		else {
			text = text.replace("\n", "<br>");
		}
		if (span.code) text = "<code>" + text + "</code>";
		if (span.bold) text = "<strong>" + text + "</strong>";
		if (span.italic) text = "<em>" + text + "</em>";
		if (span.strikethrough) text = "<del>" + text + "</del>";
		if (span.highlight) text = "<mark>" + text + "</mark>";
		// 原始 HTML 的 style 属性由同一 DOM 计算，不能把局部颜色再次写成覆盖继承的行内声明
		if (!span.link_uri.is_empty() && !inside_html_link)
			text = "<a href=\"" + escape(span.link_uri) + "\">" + text + "</a>";
		output += text + span.html_close;
		if (span.html_close.to_lower().contains("</a>")) inside_html_link = false;
	}
	return output;
}

String MarkdownLayoutDocument::render_block(const MarkdownBlock& p_block, const MarkdownParseOptions& p_options,
	const MarkdownThemeCache& p_theme, MermaidRenderCache& p_mermaid) {
	const auto inline_html = [&](const String& p_text) { return render_inline(p_text, p_options, p_theme, p_mermaid); };
	switch (p_block.type) {
		case MARKDOWN_BLOCK_HTML: return p_options.parse_html ? p_block.text : escape(p_block.text);
		case MARKDOWN_BLOCK_HEADING: {
			const String tag = "h" + String::num_int64(CLAMP(p_block.level, 1, 6));
			return "<" + tag + " id=\"" + escape(p_block.anchor) + "\">" + inline_html(p_block.text) + "</" + tag + ">";
		}
		case MARKDOWN_BLOCK_TABLE: {
			String html = "<table>";
			for (int row = 0; row < int(p_block.rows.size()); ++row) {
				if (row == 0) html += p_block.header_rows > 0 ? "<thead>" : "<tbody>";
				if (row == p_block.header_rows && row > 0) html += "</thead><tbody>";
				html += "<tr>";
				for (int col = 0; col < p_block.columns; ++col) {
					const String tag = row < p_block.header_rows ? "th" : "td";
					const int alignment = col < int(p_block.column_alignments.size()) ? p_block.column_alignments[col] : 0;
					html += "<" + tag + " align=\"" + String(alignment == 1 ? "center" : alignment == 2 ? "right" : "left") + "\">";
					if (col < p_block.rows[row].size()) html += inline_html(p_block.rows[row][col]);
					html += "</" + tag + ">";
				}
				html += "</tr>";
			}
			return html + (int(p_block.rows.size()) <= p_block.header_rows ? "</thead>" : "</tbody>") + "</table>";
		}
		case MARKDOWN_BLOCK_THEMATIC_BREAK: return "<hr>";
		case MARKDOWN_BLOCK_CODE: {
			if (p_options.parse_mermaid && p_block.argument == "mermaid") {
				const auto cached = p_mermaid.get_result(p_block.text);
				if (cached && cached->texture.is_valid()) {
					const String key = "res://__dockit_runtime/mermaid-" + String::num_uint64(p_block.text.hash()) + ".png";
					cached->texture->set_meta("_dockit_copy_text", p_block.text);
					textures[key] = cached->texture;
					return "<figure><img src=\"" + key + "\" alt=\"" + escape(p_block.text) + "\"></figure>";
				}
			}
			return "<pre><code class=\"language-" + escape(p_block.argument) + "\">" + escape(p_block.text) + "</code></pre>";
		}
		case MARKDOWN_BLOCK_LATEX: {
			const String key = "res://__dockit_runtime/display-formula-" + String::num_uint64(p_block.argument.hash()) + ".png";
			Ref<MarkdownFormulaTexture> texture;
			texture.instantiate();
			if (texture->configure(p_block.argument, p_theme.font_size.text, p_theme.color.text, true) != OK) return "<pre>" + escape(p_block.text) + "</pre>";
			textures[key] = texture;
			return "<div class=\"dockit-display-math\"><img class=\"dockit-display-formula\" src=\"" + key + "\" alt=\"" + escape(p_block.text) + "\"></div>";
		}
		case MARKDOWN_BLOCK_BLOCKQUOTE: {
			String html = "<blockquote>";
			const auto children = p_block.children.empty() ? parse_markdown_document(p_block.text, nullptr, INT_MAX, p_options).blocks : p_block.children;
			for (const auto& child : children) html += render_block(child, p_options, p_theme, p_mermaid);
			return html + "</blockquote>";
		}
		case MARKDOWN_BLOCK_ORDERED_LIST:
		case MARKDOWN_BLOCK_UNORDERED_LIST:
		case MARKDOWN_BLOCK_TASK_LIST: {
			const bool task_list = p_block.type == MARKDOWN_BLOCK_TASK_LIST;
			const String tag = p_block.type == MARKDOWN_BLOCK_ORDERED_LIST ? "ol" : "ul";
			String html = "<" + tag + (task_list ? " class=\"dockit-task-list\" style=\"padding-left:" +
				String::num_int64(std::max(1, p_block.indent + 1) * p_theme.constant.list_indent) + "px\"" : "") +
				" start=\"" + String::num_int64(p_block.list_start) + "\">";
			if (!p_block.list_items.empty()) {
				for (const auto& item : p_block.list_items) {
					html += task_list ? "<li class=\"dockit-task-item\">" : "<li>";
					if (task_list) html += task_marker_html(item.checked, p_theme, textures);
					for (const auto& child : item.children) html += render_block(child, p_options, p_theme, p_mermaid);
					html += "</li>";
				}
			}
			else {
				for (int i = 0; i < p_block.items.size(); ++i) {
					html += task_list ? "<li class=\"dockit-task-item\">" : "<li>";
					if (task_list) html += task_marker_html(i < int(p_block.task_checked.size()) && p_block.task_checked[i], p_theme, textures);
					html += inline_html(p_block.items[i]) + "</li>";
				}
			}
			return html + "</" + tag + ">";
		}
		case MARKDOWN_BLOCK_FOOTNOTES: {
			String html = "<ol class=\"dockit-footnotes\">";
			for (int number = 1; number <= footnotes.size(); ++number) {
				for (const auto& definition : p_block.footnotes) {
					if (!footnotes.has(definition.id) || footnotes[definition.id] != number) continue;
					html += "<li id=\"dockit-note-" + escape(definition.id) + "\"><a class=\"dockit-footnote-backref\" href=\"#dockit-ref-" + escape(definition.id) + "\">" + String::num_int64(number) + ".</a> " + inline_html(definition.text) + "</li>";
				}
			}
			return html + "</ol>";
		}
		case MARKDOWN_BLOCK_IMAGE:
		case MARKDOWN_BLOCK_PARAGRAPH:
		default: return "<p>" + inline_html(p_block.text) + "</p>";
	}
}

Error MarkdownLayoutDocument::update(const std::vector<MarkdownBlock>& p_blocks, const String& p_source,
	const std::vector<int64_t>& p_line_starts,
	const MarkdownParseOptions& p_options,
	const MarkdownThemeCache& p_theme, Control* p_owner, MermaidRenderCache& p_mermaid,
	const Vector2i& p_viewport, const String& p_base, const Ref<CSSStyleSheet>& p_stylesheet) {
	Dictionary theme_textures;
	String css = theme_stylesheet(p_theme, theme_textures);
	if (const auto* label = Object::cast_to<MarkdownLabel>(p_owner)) {
		if (label->get_autowrap_mode() == TextServer::AUTOWRAP_OFF) css += "body{white-space:nowrap;}";
		else if (label->get_autowrap_mode() == TextServer::AUTOWRAP_ARBITRARY) css += "body{word-break:break-all;}";
	}
	const String author = p_stylesheet.is_valid() ? p_stylesheet->get_css_text() : String();
	const String stylesheet_path = p_stylesheet.is_valid() ? p_stylesheet->get_source_path() : String();
	const uint64_t checked_icon_id = p_theme.icon.task_checked.is_valid() ? uint64_t(p_theme.icon.task_checked->get_instance_id()) : 0;
	const uint64_t unchecked_icon_id = p_theme.icon.task_unchecked.is_valid() ? uint64_t(p_theme.icon.task_unchecked->get_instance_id()) : 0;
	const bool reset = layout.is_null() || theme_css != css || resource_base != p_base || author_css != author ||
		task_checked_icon_id != checked_icon_id || task_unchecked_icon_id != unchecked_icon_id;
	const bool viewport_changed = !has_last_viewport_size || last_viewport_size != p_viewport;
	if (reset) { textures.clear(); source_blocks.clear(); footnotes.clear(); }
	if (reset || viewport_changed) {
		source_lines.clear();
		visual_source_lines.clear();
	}
	textures.merge(theme_textures, true);
	const std::vector<int64_t>& line_starts = p_line_starts;
	std::vector<MarkdownSourceBlock> blocks;
	rebuilt_nodes = 0;
	retained_nodes = 0;
	for (size_t i = 0; i < p_blocks.size(); ++i) {
		MarkdownSourceBlock block;
		block.type = p_blocks[i].type;
		block.line = MAX(0, p_blocks[i].line - (p_blocks[i].type == MARKDOWN_BLOCK_FOOTNOTES ? 0 : 1));
		block.footnotes = p_blocks[i].footnotes;
		const int32_t next_source_line = i + 1 < p_blocks.size()
			? MAX(0, p_blocks[i + 1].line - (p_blocks[i + 1].type == MARKDOWN_BLOCK_FOOTNOTES ? 0 : 1))
			: static_cast<int32_t>(line_starts.size()) - 1;
		const int32_t source_end_line = i + 1 < p_blocks.size()
			? CLAMP(next_source_line, block.line, static_cast<int32_t>(line_starts.size()) - 1)
			: static_cast<int32_t>(line_starts.size()) - 1;
		block.end_line = std::max(block.line, source_end_line - 1);
		const int32_t bounded_start_line = CLAMP(block.line, 0, static_cast<int32_t>(line_starts.size()) - 1);
		block.source_start = line_starts[bounded_start_line];
		block.source_end = i + 1 < p_blocks.size()
			? line_starts[std::max(bounded_start_line, source_end_line)]
			: p_source.length();
		block.content_hash = compute_block_hash(p_blocks[i]);
		if (i < source_blocks.size() && source_blocks[i].content_hash == block.content_hash &&
			p_blocks[i].type != MARKDOWN_BLOCK_FOOTNOTES && !(p_options.parse_mermaid && p_blocks[i].type == MARKDOWN_BLOCK_CODE)) block.html = source_blocks[i].html;
		else block.html = render_block(p_blocks[i], p_options, p_theme, p_mermaid);
		if (i < source_blocks.size() && source_blocks[i].html == block.html) {
			block.id = source_blocks[i].id;
			block.element = source_blocks[i].element;
			block.nodes = source_blocks[i].nodes;
			block.bounds = source_blocks[i].bounds;
			++retained_nodes;
		}
		else { block.id = next_id++; ++rebuilt_nodes; }
		blocks.push_back(std::move(block));
	}
	int32_t source_map_prefix_blocks = 0;
	if (!reset && !viewport_changed) {
		const size_t limit = std::min(source_blocks.size(), blocks.size());
		while (source_map_prefix_blocks < static_cast<int32_t>(limit)) {
			const MarkdownSourceBlock& previous = source_blocks[source_map_prefix_blocks];
			const MarkdownSourceBlock& current = blocks[source_map_prefix_blocks];
			if (previous.id != current.id || previous.type != current.type || previous.line != current.line ||
				previous.end_line != current.end_line || previous.source_start != current.source_start ||
				previous.source_end != current.source_end) break;
			++source_map_prefix_blocks;
		}
	}
	// 全文 CSS 与 HTML 片段共用一个 DOM，源码标识仅保存在宿主表中
	if (reset) {
		layout.instantiate();
		HTMLFontConfig fonts{p_theme.font.text, p_theme.font.bold, p_theme.font.italic, p_theme.font.bold_italic, p_theme.font.code, p_theme.font.code_block};
		fonts.named_fonts["dockit-table-header"] = p_theme.font.table_header;
		fonts.named_fonts["dockit-table-cell"] = p_theme.font.table_cell;
		fonts.named_fonts["dockit-footnote-text"] = p_theme.font.footnote_text;
		fonts.named_fonts["dockit-footnote-ref"] = p_theme.font.footnote_ref;
		for (int i = 0; i < 6; ++i) fonts.named_fonts["dockit-heading-" + String::num_int64(i + 1)] = p_theme.font.heading[i];
		Dictionary stylesheet_resources;
		String author_source = author;
		if (!stylesheet_path.is_empty()) {
			stylesheet_resources[stylesheet_path] = p_stylesheet;
			author_source = "@import url(\"" + stylesheet_path.replace("\\", "\\\\").replace("\"", "\\\"") + "\");";
		}
		const Error error = layout->parse_source("<html><head></head><body></body></html>", p_base, author_source, textures, String(), true, css, fonts, stylesheet_resources);
		if (error != OK) return error;
	}
	else layout->update_embedded_textures(textures);
	const auto body = layout->get_body();
	Array children;
	for (auto& block : blocks) {
		if (block.nodes.empty()) {
			const auto fragment = layout->parse_fragment(block.html, body);
			if (fragment.is_null()) return ERR_PARSE_ERROR;
			const auto nodes = fragment->get_child_nodes();
			for (int j = 0; j < nodes.size(); ++j) {
				Ref<HTMLNode> node = nodes[j];
				block.nodes.push_back(node);
				if (block.element.is_null()) block.element = Ref<HTMLElement>(node);
			}
		}
		for (const auto& node : block.nodes) children.push_back(node);
	}
	if (rebuilt_nodes > 0 || source_blocks.size() != blocks.size() || reset) body->replace_children(children);
	source_blocks = std::move(blocks);
	theme_css = css;
	resource_base = p_base;
	author_css = author;
	task_checked_icon_id = checked_icon_id;
	task_unchecked_icon_id = unchecked_icon_id;
	const Error layout_error = layout->layout_embedded_block(p_viewport);
	if (layout_error == OK) {
		last_viewport_size = p_viewport;
		has_last_viewport_size = true;
		rebuild_source_map(p_source, line_starts, source_map_prefix_blocks);
	}
	return layout_error;
}

void MarkdownLayoutDocument::rebuild_source_map(const String& p_source, const std::vector<int64_t>& p_line_starts,
	int32_t p_prefix_block_count) {
	if (p_line_starts.empty()) return;
	const int32_t previous_line_count = static_cast<int32_t>(source_lines.size());
	for (int32_t i = 0; i < static_cast<int32_t>(source_blocks.size()); ++i) {
		MarkdownSourceBlock& block = source_blocks[i];
		const Rect2 previous_bounds = block.bounds;
		block.bounds = source_bounds(block);
		if (i < p_prefix_block_count && previous_bounds != block.bounds) p_prefix_block_count = i;
	}
	if (p_prefix_block_count > 0 && p_prefix_block_count < static_cast<int32_t>(source_blocks.size())) {
		--p_prefix_block_count;
	}
	const int32_t dirty_line = p_prefix_block_count == 0
		? 0
		: p_prefix_block_count < static_cast<int32_t>(source_blocks.size())
			? CLAMP(source_blocks[p_prefix_block_count].line, 0, static_cast<int32_t>(p_line_starts.size()) - 1)
			: std::min(previous_line_count, static_cast<int32_t>(p_line_starts.size()));
	for (int32_t line = dirty_line; line < previous_line_count; ++line) {
		const MarkdownSourceLineMap& previous = source_lines[line];
		if (previous.has_geometry) visual_source_lines.erase({ previous.anchor_y, line });
	}
	source_lines.resize(p_line_starts.size());
	for (int32_t line = dirty_line; line < static_cast<int32_t>(source_lines.size()); ++line) {
		source_lines[line] = MarkdownSourceLineMap();
		source_lines[line].source_start = p_line_starts[line];
	}
	const String rendered_text = layout.is_valid() ? layout->get_plain_text() : String();
	// 按字符顺序对齐源码与排版文本，语法标记没有可见字符时由相邻内容定位
	const auto append_source_range = [&](int64_t p_source_start, int64_t p_source_end, int64_t p_rendered_start, int64_t p_rendered_end) {
		const int64_t source_start = CLAMP(p_source_start, int64_t(0), p_source.length());
		const int64_t source_end = CLAMP(p_source_end, source_start, p_source.length());
		std::unordered_map<char32_t, std::vector<int64_t>> source_character_offsets;
		for (int64_t offset = source_start; offset < source_end; ++offset) {
			source_character_offsets[p_source[offset]].push_back(offset);
		}
		const int64_t rendered_start = CLAMP<int64_t>(p_rendered_start, 0, rendered_text.length());
		const int64_t rendered_end = CLAMP<int64_t>(p_rendered_end, rendered_start, rendered_text.length());
		int64_t source_cursor = source_start;
		for (int64_t rendered_offset = rendered_start; rendered_offset < rendered_end; ++rendered_offset) {
			const char32_t character = rendered_text[rendered_offset];
			int64_t mapped_source = source_cursor;
			const auto matches = source_character_offsets.find(character);
			if (matches != source_character_offsets.end()) {
				const auto candidate = std::lower_bound(matches->second.begin(), matches->second.end(), source_cursor);
				if (candidate != matches->second.end()) {
					mapped_source = *candidate;
					source_cursor = mapped_source + 1;
				}
			}
			if (source_end > source_start) mapped_source = CLAMP(mapped_source, source_start, source_end - 1);
			const int32_t source_line = source_line_at(p_line_starts, mapped_source);
			if (source_line >= 0 && source_line < static_cast<int32_t>(source_lines.size())) {
				MarkdownSourceLineMap& line = source_lines[source_line];
				line.rendered_offsets.push_back(rendered_offset);
				if (line.first_source_offset < 0 || mapped_source < line.first_source_offset) {
					line.first_source_offset = mapped_source;
					line.first_rendered_offset = rendered_offset;
				}
				if (line.last_source_offset < 0 || mapped_source > line.last_source_offset ||
					(mapped_source == line.last_source_offset && rendered_offset > line.last_rendered_offset)) {
					line.last_source_offset = mapped_source;
					line.last_rendered_offset = rendered_offset;
				}
			}
		}
	};
	for (int32_t block_index = p_prefix_block_count; block_index < static_cast<int32_t>(source_blocks.size()); ++block_index) {
		MarkdownSourceBlock& block = source_blocks[block_index];
		Vector2i rendered_range(rendered_text.length(), 0);
		bool has_rendered_text = false;
		for (const Ref<HTMLNode>& node : block.nodes) {
			HTMLElement* element = Object::cast_to<HTMLElement>(node.ptr());
			if (element == nullptr) continue;
			const Vector2i candidate = layout->get_element_text_range(Ref<HTMLElement>(element));
			if (candidate.x >= candidate.y) continue;
			if (!has_rendered_text) {
				rendered_range = candidate;
				has_rendered_text = true;
			}
			else {
				rendered_range.x = std::min(rendered_range.x, candidate.x);
				rendered_range.y = std::max(rendered_range.y, candidate.y);
			}
		}
		if (block.type == MARKDOWN_BLOCK_FOOTNOTES) {
			Ref<HTMLElement> list = block.element;
			if (list.is_null()) {
				for (const Ref<HTMLNode>& node : block.nodes) {
					list = Ref<HTMLElement>(Object::cast_to<HTMLElement>(node.ptr()));
					if (list.is_valid()) break;
				}
			}
			const TypedArray<HTMLElement> items = list.is_valid() ? list->get_children() : TypedArray<HTMLElement>();
			for (const MarkdownFootnoteDefinition& definition : block.footnotes) {
				const String expected_id = "dockit-note-" + definition.id;
				Ref<HTMLElement> item;
				for (int64_t child = 0; child < items.size(); ++child) {
					Ref<HTMLElement> candidate = items[child];
					if (candidate.is_valid() && candidate->get_tag_name() == "li" && candidate->get_attribute("id") == expected_id) {
						item = candidate;
						break;
					}
				}
				if (item.is_null()) continue;
				const Vector2i range = layout->get_element_text_range(item);
				if (range.x >= range.y) continue;
				const int32_t first_line = CLAMP(definition.line, 0, static_cast<int32_t>(p_line_starts.size()) - 1);
				const int32_t end_line = CLAMP(definition.end_line + 1, first_line, static_cast<int32_t>(p_line_starts.size()) - 1);
				const int64_t source_start = p_line_starts[first_line];
				const int64_t source_end = definition.end_line + 1 < static_cast<int32_t>(p_line_starts.size())
					? p_line_starts[end_line]
					: p_source.length();
				append_source_range(source_start, source_end, range.x, range.y);
			}
			continue;
		}
		const int32_t first_line = source_line_at(p_line_starts, block.source_start);
		const int32_t last_line = source_line_at(p_line_starts, block.source_end > block.source_start ? block.source_end - 1 : block.source_start);
		if (!has_rendered_text) {
			if (block.bounds.has_area()) {
				for (int32_t line = first_line; line <= last_line; ++line) {
					source_lines[line].has_object_bounds = true;
					source_lines[line].object_bounds = block.bounds;
				}
			}
			continue;
		}

		append_source_range(block.source_start, block.source_end, rendered_range.x, rendered_range.y);
	}
	for (int32_t line_index = dirty_line; line_index < static_cast<int32_t>(source_lines.size()); ++line_index) {
		MarkdownSourceLineMap& line = source_lines[line_index];
		std::sort(line.rendered_offsets.begin(), line.rendered_offsets.end());
		line.rendered_offsets.erase(std::unique(line.rendered_offsets.begin(), line.rendered_offsets.end()), line.rendered_offsets.end());
	}
	int64_t preceding_source_offset = -1;
	int64_t preceding_rendered_offset = -1;
	for (int32_t i = dirty_line - 1; i >= 0; --i) {
		if (source_lines[i].last_source_offset < 0) continue;
		preceding_source_offset = source_lines[i].last_source_offset;
		preceding_rendered_offset = source_lines[i].last_rendered_offset;
		break;
	}
	const size_t dirty_line_count = source_lines.size() - dirty_line;
	std::vector<int64_t> following_source_offsets(dirty_line_count, -1);
	std::vector<int64_t> following_rendered_offsets(dirty_line_count, -1);
	int64_t following_source_offset = -1;
	int64_t following_rendered_offset = -1;
	for (size_t i = source_lines.size(); i > static_cast<size_t>(dirty_line); --i) {
		const MarkdownSourceLineMap& line = source_lines[i - 1];
		if (line.first_source_offset >= line.source_start) {
			following_source_offset = line.first_source_offset;
			following_rendered_offset = line.first_rendered_offset;
		}
		following_source_offsets[i - 1 - dirty_line] = following_source_offset;
		following_rendered_offsets[i - 1 - dirty_line] = following_rendered_offset;
	}
	for (int32_t i = dirty_line; i < static_cast<int32_t>(source_lines.size()); ++i) {
		MarkdownSourceLineMap& line = source_lines[i];
		if (line.rendered_offsets.empty() && !line.has_object_bounds) {
			const int64_t after_source = following_source_offsets[i - dirty_line];
			const int64_t after = following_rendered_offsets[i - dirty_line];
			if (preceding_source_offset >= 0 && after_source >= 0) {
				line.fallback_rendered_offset = line.source_start - preceding_source_offset <= after_source - line.source_start
					? preceding_rendered_offset
					: after;
			}
			else line.fallback_rendered_offset = preceding_source_offset >= 0 ? preceding_rendered_offset : after;
		}
		if (line.last_source_offset >= 0) {
			preceding_source_offset = line.last_source_offset;
			preceding_rendered_offset = line.last_rendered_offset;
		}
	}

	for (int32_t line_index = dirty_line; line_index < static_cast<int32_t>(source_lines.size()); ++line_index) {
		MarkdownSourceLineMap& line = source_lines[line_index];
		if (line.has_object_bounds && line.rendered_offsets.empty()) {
			line.anchor_y = line.object_bounds.get_center().y;
			line.has_geometry = true;
			continue;
		}
		std::optional<Rect2> first_rect;
		std::optional<Rect2> last_rect;
		for (const int64_t offset : line.rendered_offsets) {
			first_rect = text_character_rect(layout, offset);
			if (first_rect.has_value()) break;
		}
		for (auto offset = line.rendered_offsets.rbegin(); offset != line.rendered_offsets.rend(); ++offset) {
			last_rect = text_character_rect(layout, *offset);
			if (last_rect.has_value()) break;
		}
		if (!first_rect.has_value() && line.fallback_rendered_offset >= 0) {
			first_rect = text_character_rect(layout, line.fallback_rendered_offset);
			last_rect = first_rect;
		}
		if (first_rect.has_value()) {
			const double first_y = first_rect->get_center().y;
			const double last_y = last_rect.has_value() ? last_rect->get_center().y : first_y;
			line.anchor_y = (first_y + last_y) * 0.5;
			line.has_geometry = true;
			line.has_text_geometry = true;
		}
		else if (line.has_object_bounds) {
			line.anchor_y = line.object_bounds.get_center().y;
			line.has_geometry = true;
		}
	}
	for (int32_t line_index = dirty_line; line_index < static_cast<int32_t>(source_lines.size()); ++line_index) {
		MarkdownSourceLineMap& line = source_lines[line_index];
		if (line.has_geometry) continue;
		for (const MarkdownSourceBlock& block : source_blocks) {
			if (line_index < block.line || line_index > block.end_line || !block.bounds.has_area()) continue;
			line.has_object_bounds = true;
			line.object_bounds = block.bounds;
			line.anchor_y = block.bounds.get_center().y;
			line.has_geometry = true;
			break;
		}
	}

	for (int32_t line = dirty_line; line < static_cast<int32_t>(source_lines.size()); ++line) {
		if (source_lines[line].has_geometry) visual_source_lines[{ source_lines[line].anchor_y, line }] = line;
	}
}

double MarkdownLayoutDocument::source_line_to_y(double p_line, int32_t p_line_count) const {
	if (layout.is_null() || source_blocks.empty() || source_lines.empty()) return 0;
	const int32_t mapped_line_count = std::min(p_line_count, static_cast<int32_t>(source_lines.size()));
	const double line_value = CLAMP(p_line, 0.0, std::max(0.0, double(mapped_line_count - 1)));
	const int32_t line_index = CLAMP(static_cast<int32_t>(line_value), 0, static_cast<int32_t>(source_lines.size()) - 1);
	const double fraction = line_value - line_index;
	const MarkdownSourceLineMap& line = source_lines[line_index];
	if (line.has_object_bounds && !line.has_text_geometry) return line.object_bounds.get_center().y;
	if (!line.rendered_offsets.empty()) {
		const size_t offset_index = static_cast<size_t>(std::round(fraction * (line.rendered_offsets.size() - 1)));
		if (const std::optional<Rect2> rect = text_character_rect(layout, line.rendered_offsets[offset_index])) return rect->get_center().y;
	}
	if (line.fallback_rendered_offset >= 0) {
		if (const std::optional<Rect2> rect = text_character_rect(layout, line.fallback_rendered_offset)) return rect->get_center().y;
	}
	if (line.has_geometry) return line.anchor_y;
	for (const auto& block : source_blocks) {
		if (line_index >= block.line && line_index <= block.end_line && block.bounds.has_area()) return block.bounds.position.y;
	}
	return 0.0;
}

double MarkdownLayoutDocument::y_to_source_line(double p_y, int32_t p_line_count) const {
	if (source_lines.empty() || visual_source_lines.empty()) return p_line_count;
	const auto clamp_source_line = [p_line_count](double p_line) {
		return CLAMP(p_line, 0.0, std::max(0.0, double(p_line_count - 1)));
	};
	const auto next = visual_source_lines.lower_bound({ p_y, std::numeric_limits<int32_t>::min() });
	if (next == visual_source_lines.begin()) return clamp_source_line(next->second);
	if (next == visual_source_lines.end()) return clamp_source_line(visual_source_lines.rbegin()->second);
	const auto previous = std::prev(next);
	const int32_t after = next->second;
	const int32_t before = previous->second;
	const double y_before = previous->first.first;
	const double y_after = next->first.first;
	if (y_after <= y_before) return clamp_source_line(before);
	return clamp_source_line(before + (after - before) * CLAMP((p_y - y_before) / (y_after - y_before), 0.0, 1.0));
}

void MarkdownLayoutDocument::clear() {
	layout.unref(); source_blocks.clear(); source_lines.clear(); visual_source_lines.clear(); textures.clear(); theme_css = String(); author_css = String(); footnotes.clear();
	task_checked_icon_id = 0; task_unchecked_icon_id = 0;
	has_last_viewport_size = false;
}

}
