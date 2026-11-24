#include "html_syntax_highlighter.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/text_edit.hpp>

#include <algorithm>
#include <cstring>

namespace godot {
	namespace {
		struct HTMLHighlightColors {
			Color tag;
			Color attribute;
			Color string;
			Color comment;
			Color symbol;
		};

		Color get_highlighting_color(const StringName& p_setting, const Color& p_fallback) {
			EditorInterface* editor = EditorInterface::get_singleton();
			if (editor == nullptr) return p_fallback;
			const Ref<EditorSettings> settings = editor->get_editor_settings();
			if (settings.is_null() || !settings->has_setting(p_setting)) return p_fallback;
			const Variant value = settings->get_setting(p_setting);
			return value.get_type() == Variant::COLOR ? static_cast<Color>(value) : p_fallback;
		}

		StringName setting_name(const char* p_suffix) {
			return StringName(String("text_editor/theme/highlighting/") + p_suffix);
		}

		bool is_ascii_letter(char32_t p_character) {
			return (p_character >= 'a' && p_character <= 'z') || (p_character >= 'A' && p_character <= 'Z');
		}

		bool is_name_character(char32_t p_character) {
			return is_ascii_letter(p_character) || (p_character >= '0' && p_character <= '9') ||
				p_character == '-' || p_character == '_' || p_character == ':' || p_character == '.' || p_character >= 0x80;
		}

		bool matches_at(const String& p_text, int32_t p_index, const char* p_pattern) {
			const int32_t pattern_length = static_cast<int32_t>(std::strlen(p_pattern));
			if (p_index < 0 || pattern_length > p_text.length() || p_index > p_text.length() - pattern_length) return false;
			for (int32_t index = 0; index < pattern_length; ++index) {
				if (p_text[p_index + index] != static_cast<unsigned char>(p_pattern[index])) return false;
			}
			return true;
		}

		bool can_start_tag(const String& p_text, int32_t p_index) {
			int32_t name = p_index + 1;
			if (name < p_text.length() && p_text[name] == '/') ++name;
			if (name < p_text.length() && (p_text[name] == '!' || p_text[name] == '?')) ++name;
			return name < p_text.length() && is_ascii_letter(p_text[name]);
		}

		void set_color(Dictionary* p_colors, int32_t p_index, const Color* p_color) {
			if (p_colors == nullptr || p_index < 0) return;
			if (p_color == nullptr) {
				(*p_colors)[p_index] = Dictionary();
				return;
			}
			Dictionary info;
			info["color"] = *p_color;
			(*p_colors)[p_index] = info;
		}

		void set_range_color(Dictionary* p_colors, int32_t p_start, int32_t p_end, const Color& p_color, int32_t p_line_length) {
			if (p_start >= p_end) return;
			set_color(p_colors, p_start, &p_color);
			if (p_end < p_line_length) set_color(p_colors, p_end, nullptr);
		}

		void scan_html_line(
			const String& p_line,
			HTMLSyntaxHighlighter::HTMLLexState& r_state,
			const HTMLHighlightColors& p_html_colors,
			const css_highlight::CSSHighlightColors& p_css_colors,
			Dictionary* r_color_map) {
			const int32_t length = p_line.length();
			int32_t index = 0;

			if (r_state.in_comment) set_color(r_color_map, 0, &p_html_colors.comment);
			else if (r_state.quote != 0) set_color(r_color_map, 0, &p_html_colors.string);

			while (index < length) {
				if (r_state.in_style) {
					const int32_t style_end = css_highlight::find_style_end_tag(p_line, index);
					const int32_t css_end = style_end < 0 ? length : style_end;
					css_highlight::scan_css_line(p_line, r_state.css_state, p_css_colors, r_color_map, index, css_end);
					if (style_end < 0) break;

					r_state.in_style = false;
					r_state.css_state = {};
					r_state.in_tag = true;
					r_state.expect_tag_name = true;
					r_state.expect_value = false;
					r_state.closing_tag = true;
					r_state.self_closing_tag = false;
					r_state.current_tag_is_style = false;
					set_color(r_color_map, style_end, &p_html_colors.symbol);
					index = style_end + 1;
					continue;
				}

				if (r_state.in_comment) {
					int32_t comment_end = index;
					while (comment_end < length && !matches_at(p_line, comment_end, "-->")) ++comment_end;
					if (comment_end == length) {
						set_range_color(r_color_map, index, length, p_html_colors.comment, length);
						break;
					}
					comment_end += 3;
					set_range_color(r_color_map, index, comment_end, p_html_colors.comment, length);
					r_state.in_comment = false;
					index = comment_end;
					continue;
				}

				if (r_state.quote != 0) {
					int32_t string_end = index;
					while (string_end < length && p_line[string_end] != r_state.quote) ++string_end;
					if (string_end == length) {
						set_range_color(r_color_map, index, length, p_html_colors.string, length);
						break;
					}
					++string_end;
					set_range_color(r_color_map, index, string_end, p_html_colors.string, length);
					r_state.quote = 0;
					r_state.expect_value = false;
					index = string_end;
					continue;
				}

				if (!r_state.in_tag) {
					if (matches_at(p_line, index, "<!--")) {
						r_state.in_comment = true;
						continue;
					}
					if (p_line[index] != '<') {
						++index;
						continue;
					}
					set_color(r_color_map, index, &p_html_colors.symbol);
					if (can_start_tag(p_line, index)) {
						r_state.in_tag = true;
						r_state.expect_tag_name = true;
						r_state.expect_value = false;
						r_state.closing_tag = index + 1 < length && p_line[index + 1] == '/';
						r_state.self_closing_tag = false;
						r_state.current_tag_is_style = false;
					} else if (index + 1 < length) {
						set_color(r_color_map, index + 1, nullptr);
					}
					++index;
					continue;
				}

				const char32_t character = p_line[index];
				if (r_state.expect_value) {
					if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
						++index;
						continue;
					}
					if (character == '>') {
						r_state.expect_value = false;
						continue;
					}
					if (character == '/' && index + 1 < length && p_line[index + 1] == '>') {
						r_state.expect_value = false;
						r_state.self_closing_tag = true;
						continue;
					}
					if (character == '\'' || character == '"') {
						r_state.quote = character;
						set_color(r_color_map, index, &p_html_colors.string);
						++index;
						continue;
					}
					const int32_t start = index;
					while (index < length && p_line[index] != ' ' && p_line[index] != '\t' && p_line[index] != '>' && p_line[index] != '\r') ++index;
					set_range_color(r_color_map, start, index, p_html_colors.string, length);
					r_state.expect_value = false;
					continue;
				}

				if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
					++index;
					continue;
				}
				if (character == '>') {
					set_color(r_color_map, index, &p_html_colors.symbol);
					const bool enter_style = r_state.current_tag_is_style && !r_state.closing_tag && !r_state.self_closing_tag;
					r_state.in_tag = false;
					r_state.expect_tag_name = false;
					r_state.expect_value = false;
					++index;
					if (enter_style) {
						r_state.in_style = true;
						r_state.css_state = {};
					}
					if (index < length) set_color(r_color_map, index, nullptr);
					continue;
				}
				if (character == '/') {
					set_color(r_color_map, index, &p_html_colors.symbol);
					r_state.self_closing_tag = true;
					++index;
					continue;
				}
				if (character == '!') {
					set_color(r_color_map, index, &p_html_colors.symbol);
					++index;
					continue;
				}
				if (character == '=') {
					set_color(r_color_map, index, &p_html_colors.symbol);
					r_state.expect_value = true;
					++index;
					continue;
				}
				if (character == '\'' || character == '"') {
					r_state.quote = character;
					set_color(r_color_map, index, &p_html_colors.string);
					++index;
					continue;
				}

				if (r_state.expect_tag_name) {
					if (is_name_character(character)) {
						const int32_t start = index;
						while (index < length && is_name_character(p_line[index])) ++index;
						set_range_color(r_color_map, start, index, p_html_colors.tag, length);
						r_state.current_tag_is_style = p_line.substr(start, index - start).to_lower() == "style";
						r_state.expect_tag_name = false;
						continue;
					}
					++index;
					continue;
				}

				if (is_name_character(character)) {
					const int32_t start = index;
					while (index < length && is_name_character(p_line[index])) ++index;
					set_range_color(r_color_map, start, index, p_html_colors.attribute, length);
					continue;
				}
				++index;
			}
		}
	} // namespace

	HTMLSyntaxHighlighter::HTMLSyntaxHighlighter() {
		_update_cache();
	}

	String HTMLSyntaxHighlighter::_get_name() const {
		return "HTML";
	}

	PackedStringArray HTMLSyntaxHighlighter::_get_supported_languages() const {
		return PackedStringArray{ "html", "htm", "xhtml" };
	}

	Ref<EditorSyntaxHighlighter> HTMLSyntaxHighlighter::_create() const {
		Ref<HTMLSyntaxHighlighter> instance;
		instance.instantiate();
		return instance;
	}

	Dictionary HTMLSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
		TextEdit* text_edit = get_text_edit();
		if (text_edit == nullptr || p_line < 0 || p_line >= text_edit->get_line_count()) return {};

		const uint32_t text_version = text_edit->get_version();
		const int32_t line_count = text_edit->get_line_count();
		if (cached_text_version != text_version || static_cast<int32_t>(line_state_cache.size()) != line_count) {
			line_state_cache.assign(line_count, HTMLLexState{});
			line_state_cache_valid.assign(line_count, 0);
			cached_text_version = text_version;
		}

		HTMLLexState state;
		int32_t first_line = 0;
		for (int32_t line = p_line - 1; line >= 0; --line) {
			if (line_state_cache_valid[line] != 0) {
				state = line_state_cache[line];
				first_line = line + 1;
				break;
			}
		}

		const HTMLHighlightColors html_colors{ tag_color, attribute_color, string_color, comment_color, symbol_color };
		for (int32_t line = first_line; line < p_line; ++line) {
			scan_html_line(text_edit->get_line(line), state, html_colors, css_colors, nullptr);
			line_state_cache[line] = state;
			line_state_cache_valid[line] = 1;
		}
		Dictionary color_map;
		scan_html_line(text_edit->get_line(p_line), state, html_colors, css_colors, &color_map);
		line_state_cache[p_line] = state;
		line_state_cache_valid[p_line] = 1;
		return color_map;
	}

	void HTMLSyntaxHighlighter::_clear_highlighting_cache() {
		line_state_cache.clear();
		line_state_cache_valid.clear();
		cached_text_version = std::numeric_limits<uint32_t>::max();
	}

	void HTMLSyntaxHighlighter::_update_cache() {
		tag_color = get_highlighting_color(setting_name("keyword_color"), Color(0.52, 0.78, 0.95));
		attribute_color = get_highlighting_color(setting_name("member_variable_color"), Color(0.91, 0.72, 0.43));
		string_color = get_highlighting_color(setting_name("string_color"), Color(0.77, 0.64, 0.91));
		comment_color = get_highlighting_color(setting_name("comment_color"), Color(0.55, 0.62, 0.58));
		symbol_color = get_highlighting_color(setting_name("symbol_color"), Color(0.78, 0.82, 0.86));
					css_colors = css_highlight::get_css_highlight_colors();
	}
} // namespace godot
