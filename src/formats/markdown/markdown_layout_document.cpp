#include "markdown_layout_document.h"

#include "markdown_html.h"
#include "nodes/markdown_label.h"
#include "utils/style_box_css_helper.h"
#include "formats/html/elements/html_element.h"
#include "formats/html/dom/html_document_fragment.h"
#include "markdown_formula_texture.h"

#include <algorithm>

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
	String theme_stylesheet(const MarkdownThemeCache& p_theme, Dictionary& r_textures) {
		String css = "html,body{margin:0;padding:0;background:transparent;} body{overflow-wrap:anywhere;} "
			"p,ul,ol,blockquote,pre,table,figure{margin:0 0 10px;} img{max-width:100%;} "
			"pre{white-space:pre;tab-size:4;overflow:auto;} code{white-space:pre-wrap;tab-size:4;} "
			"pre code{font:inherit;color:inherit;white-space:inherit;background:none;border:0;padding:0;} "
			"table{width:100%;border-collapse:separate;border-spacing:0;} th,td{padding:4px 6px;text-align:left;vertical-align:top;} "
			"blockquote{margin-left:0;margin-right:0;} blockquote blockquote{margin-top:8px;} "
			"hr{border:0;height:1px;} .dockit-footnotes{padding-left:2em;} .dockit-footnote-ref{font-size:.75em;vertical-align:super;} "
			".dockit-footnotes{list-style:none;} .dockit-formula{display:inline-block;vertical-align:middle;} .dockit-display-formula{display:block;}";
		css += "code{font-family:monospace;} pre,pre code{font-family:dockit-code-block;} th{font-family:dockit-table-header;} td{font-family:dockit-table-cell;} .dockit-footnotes{font-family:dockit-footnote-text;} .dockit-footnote-ref{font-family:dockit-footnote-ref;}";
		css += font_rule("html", p_theme.font_size.text, p_theme.color.text);
		css += font_rule("code", p_theme.font_size.code, p_theme.color.code_text);
		css += font_rule("pre", p_theme.font_size.code_block, p_theme.color.code_block);
		css += font_rule("th", p_theme.font_size.table_header, p_theme.color.table_header_font);
		css += font_rule("td", p_theme.font_size.table_cell, p_theme.color.table_cell_font);
		css += font_rule(".dockit-footnotes", p_theme.font_size.footnote_text, p_theme.color.footnote_text);
		css += font_rule(".dockit-footnote-ref", p_theme.font_size.footnote_ref, p_theme.color.footnote_ref);
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
		css += "p,ul,ol,blockquote,pre,table{margin-bottom:" + String::num_int64(p_theme.constant.paragraph_separation) + "px;}";
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
			return "<img class=\"dockit-display-formula\" src=\"" + key + "\" alt=\"" + escape(p_block.text) + "\">";
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
			const String tag = p_block.type == MARKDOWN_BLOCK_ORDERED_LIST ? "ol" : "ul";
			String html = "<" + tag + " start=\"" + String::num_int64(p_block.list_start) + "\">";
			if (!p_block.list_items.empty()) {
				for (const auto& item : p_block.list_items) {
					html += "<li>";
					if (p_block.type == MARKDOWN_BLOCK_TASK_LIST) html += item.checked ? "&#x2611; " : "&#x2610; ";
					for (const auto& child : item.children) html += render_block(child, p_options, p_theme, p_mermaid);
					html += "</li>";
				}
			}
			else {
				for (int i = 0; i < p_block.items.size(); ++i) {
					html += "<li>";
					if (p_block.type == MARKDOWN_BLOCK_TASK_LIST) html += i < int(p_block.task_checked.size()) && p_block.task_checked[i] ? "&#x2611; " : "&#x2610; ";
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
					html += "<li id=\"dockit-note-" + escape(definition.id) + "\"><a href=\"#dockit-ref-" + escape(definition.id) + "\">" + String::num_int64(number) + ".</a> " + inline_html(definition.text) + "</li>";
				}
			}
			return html + "</ol>";
		}
		case MARKDOWN_BLOCK_IMAGE:
		case MARKDOWN_BLOCK_PARAGRAPH:
		default: return "<p>" + inline_html(p_block.text) + "</p>";
	}
}

Error MarkdownLayoutDocument::update(const std::vector<MarkdownBlock>& p_blocks, const MarkdownParseOptions& p_options,
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
	const bool reset = layout.is_null() || theme_css != css || resource_base != p_base || author_css != author;
	if (reset) { textures.clear(); source_blocks.clear(); footnotes.clear(); }
	textures.merge(theme_textures, true);
	std::vector<MarkdownSourceBlock> blocks;
	rebuilt_nodes = 0;
	retained_nodes = 0;
	for (size_t i = 0; i < p_blocks.size(); ++i) {
		MarkdownSourceBlock block;
		block.line = MAX(0, p_blocks[i].line - 1);
		block.end_line = p_blocks[i].end_line > 0 ? p_blocks[i].end_line - 1 :
			i + 1 < p_blocks.size() ? p_blocks[i + 1].line - 2 : block.line;
		block.content_hash = compute_block_hash(p_blocks[i]);
		if (i < source_blocks.size() && source_blocks[i].content_hash == block.content_hash &&
			p_blocks[i].type != MARKDOWN_BLOCK_FOOTNOTES && !(p_options.parse_mermaid && p_blocks[i].type == MARKDOWN_BLOCK_CODE)) block.html = source_blocks[i].html;
		else block.html = render_block(p_blocks[i], p_options, p_theme, p_mermaid);
		if (i < source_blocks.size() && source_blocks[i].html == block.html) {
			block.id = source_blocks[i].id;
			block.element = source_blocks[i].element;
			block.nodes = source_blocks[i].nodes;
			++retained_nodes;
		}
		else { block.id = next_id++; ++rebuilt_nodes; }
		blocks.push_back(std::move(block));
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
	return layout->layout_embedded_block(p_viewport);
}

double MarkdownLayoutDocument::source_line_to_y(double p_line, int32_t p_line_count) const {
	if (layout.is_null() || source_blocks.empty()) return 0;
	for (size_t i = 0; i < source_blocks.size(); ++i) {
		const auto& block = source_blocks[i];
		const double end_line = i + 1 < source_blocks.size() ? source_blocks[i + 1].line : p_line_count;
		if (p_line >= end_line && i + 1 < source_blocks.size()) continue;
		const Rect2 rect = source_bounds(block);
		return rect.position.y + rect.size.y * CLAMP((p_line - block.line) / MAX(1.0, end_line - block.line), 0.0, 1.0);
	}
	return layout->get_content_size().y;
}

double MarkdownLayoutDocument::y_to_source_line(double p_y, int32_t p_line_count) const {
	for (size_t i = 0; i < source_blocks.size(); ++i) {
		const auto& block = source_blocks[i];
		const Rect2 rect = source_bounds(block);
		if (p_y > rect.get_end().y && i + 1 < source_blocks.size()) continue;
		const double end = i + 1 < source_blocks.size() ? source_blocks[i + 1].line : p_line_count;
		return block.line + (end - block.line) * CLAMP((p_y - rect.position.y) / MAX(1.0, double(rect.size.y)), 0.0, 1.0);
	}
	return p_line_count;
}

void MarkdownLayoutDocument::clear() {
	layout.unref(); source_blocks.clear(); textures.clear(); theme_css = String(); author_css = String(); footnotes.clear();
}

}
