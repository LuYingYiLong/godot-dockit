#include "css_syntax_highlighter.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/text_edit.hpp>

#include <algorithm>
#include <cstring>

namespace godot {
	namespace {
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

		char32_t to_ascii_lower(char32_t p_character) {
			return p_character >= 'A' && p_character <= 'Z' ? p_character + ('a' - 'A') : p_character;
		}

		bool is_ascii_letter(char32_t p_character) {
			return (p_character >= 'a' && p_character <= 'z') || (p_character >= 'A' && p_character <= 'Z');
		}

		bool is_name_character(char32_t p_character) {
			return is_ascii_letter(p_character) || (p_character >= '0' && p_character <= '9') ||
				p_character == '-' || p_character == '_' || p_character >= 0x80;
		}

		bool is_css_hex_digit(char32_t p_character) {
			return (p_character >= '0' && p_character <= '9') ||
				(to_ascii_lower(p_character) >= 'a' && to_ascii_lower(p_character) <= 'f');
		}

		bool is_space(char32_t p_character) {
			return p_character == ' ' || p_character == '\t' || p_character == '\r' || p_character == '\n';
		}

		bool matches_at(const String& p_text, int32_t p_index, const char* p_pattern) {
			const int32_t pattern_length = static_cast<int32_t>(std::strlen(p_pattern));
			if (p_index < 0 || pattern_length > p_text.length() || p_index > p_text.length() - pattern_length) return false;
			for (int32_t index = 0; index < pattern_length; ++index) {
				if (p_text[p_index + index] != static_cast<unsigned char>(p_pattern[index])) return false;
			}
			return true;
		}

		bool matches_case_insensitive_at(const String& p_text, int32_t p_index, const char* p_pattern) {
			const int32_t pattern_length = static_cast<int32_t>(std::strlen(p_pattern));
			if (p_index < 0 || pattern_length > p_text.length() || p_index > p_text.length() - pattern_length) return false;
			for (int32_t index = 0; index < pattern_length; ++index) {
				if (to_ascii_lower(p_text[p_index + index]) != static_cast<unsigned char>(p_pattern[index])) return false;
			}
			return true;
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

		int32_t skip_spaces(const String& p_text, int32_t p_index, int32_t p_end) {
			while (p_index < p_end && is_space(p_text[p_index])) ++p_index;
			return p_index;
		}

		bool is_number_start(const String& p_line, int32_t p_index, int32_t p_end) {
			const char32_t character = p_line[p_index];
			if (character >= '0' && character <= '9') return true;
			if (character == '.' && p_index + 1 < p_end && p_line[p_index + 1] >= '0' && p_line[p_index + 1] <= '9') return true;
			return (character == '+' || character == '-') && p_index + 1 < p_end &&
				((p_line[p_index + 1] >= '0' && p_line[p_index + 1] <= '9') ||
					(p_line[p_index + 1] == '.' && p_index + 2 < p_end && p_line[p_index + 2] >= '0' && p_line[p_index + 2] <= '9'));
		}

		int32_t scan_number(const String& p_line, int32_t p_start, int32_t p_end) {
			int32_t index = p_start;
			if (p_line[index] == '+' || p_line[index] == '-') ++index;
			bool decimal_seen = false;
			while (index < p_end) {
				const char32_t character = p_line[index];
				if (character >= '0' && character <= '9') {
					++index;
					continue;
				}
				if (character == '.' && !decimal_seen) {
					decimal_seen = true;
					++index;
					continue;
				}
				break;
			}
			while (index < p_end && (is_ascii_letter(p_line[index]) || p_line[index] == '%' || p_line[index] >= 0x80)) ++index;
			return index;
		}

		int32_t scan_name(const String& p_line, int32_t p_start, int32_t p_end) {
			int32_t index = p_start;
			while (index < p_end) {
				if (is_name_character(p_line[index])) {
					++index;
					continue;
				}
				if (p_line[index] == '\\' && index + 1 < p_end) {
					index += 2;
					continue;
				}
				break;
			}
			return index;
		}
	} // namespace

	namespace css_highlight {
		CSSHighlightColors get_css_highlight_colors() {
			return {
				get_highlighting_color(setting_name("keyword_color"), Color(0.52, 0.78, 0.95)),
				get_highlighting_color(setting_name("keyword_color"), Color(0.52, 0.78, 0.95)),
				get_highlighting_color(setting_name("member_variable_color"), Color(0.91, 0.72, 0.43)),
				get_highlighting_color(setting_name("keyword_color"), Color(0.73, 0.83, 0.67)),
				get_highlighting_color(setting_name("string_color"), Color(0.77, 0.64, 0.91)),
				get_highlighting_color(setting_name("comment_color"), Color(0.55, 0.62, 0.58)),
				get_highlighting_color(setting_name("symbol_color"), Color(0.78, 0.82, 0.86)),
				get_highlighting_color(setting_name("number_color"), Color(0.95, 0.67, 0.38)),
				get_highlighting_color(setting_name("function_color"), Color(0.45, 0.72, 0.92)),
			};
		}

		void scan_css_line(
			const String& p_line,
			CSSLexState& r_state,
			const CSSHighlightColors& p_colors,
			Dictionary* r_color_map,
			int32_t p_start,
			int32_t p_end) {
			const int32_t line_length = p_line.length();
			const int32_t end = p_end < 0 ? line_length : std::clamp(p_end, p_start, line_length);
			int32_t index = std::clamp(p_start, 0, end);
			if (r_state.in_comment) set_color(r_color_map, index, &p_colors.comment);
			else if (r_state.quote != 0) set_color(r_color_map, index, &p_colors.string);

			while (index < end) {
				if (r_state.in_comment) {
					int32_t comment_end = index;
					while (comment_end < end && !matches_at(p_line, comment_end, "*/")) ++comment_end;
					if (comment_end == end) {
						set_range_color(r_color_map, index, end, p_colors.comment, line_length);
						break;
					}
					comment_end += 2;
					set_range_color(r_color_map, index, comment_end, p_colors.comment, line_length);
					r_state.in_comment = false;
					index = comment_end;
					continue;
				}

				if (r_state.quote != 0) {
					int32_t string_end = index;
					while (string_end < end) {
						const char32_t character = p_line[string_end];
						if (r_state.escaped) {
							r_state.escaped = false;
							++string_end;
							continue;
						}
						if (character == '\\') {
							r_state.escaped = true;
							++string_end;
							continue;
						}
						if (character == r_state.quote) {
							++string_end;
							r_state.quote = 0;
							break;
						}
						++string_end;
					}
					set_range_color(r_color_map, index, string_end, p_colors.string, line_length);
					if (r_state.quote != 0) break;
					index = string_end;
					continue;
				}

				const char32_t character = p_line[index];
				if (is_space(character)) {
					++index;
					continue;
				}
				if (matches_at(p_line, index, "/*")) {
					r_state.in_comment = true;
					continue;
				}
				if (character == '\'' || character == '"') {
					r_state.quote = character;
					r_state.escaped = false;
					set_color(r_color_map, index, &p_colors.string);
					++index;
					continue;
				}

				if (character == '@') {
					const int32_t start = index++;
					while (index < end && (is_name_character(p_line[index]) || p_line[index] == '\\')) ++index;
					set_range_color(r_color_map, start, index, p_colors.at_rule, line_length);
					r_state.pending_at_rule = true;
					r_state.in_at_rule_prelude = true;
					continue;
				}

				if (character == '!' && matches_case_insensitive_at(p_line, index, "!important")) {
					const int32_t after = index + 10;
					if (after == end || !is_name_character(p_line[after])) {
						set_range_color(r_color_map, index, after, p_colors.at_rule, line_length);
						index = after;
						continue;
					}
				}

				if (character == '{') {
					set_color(r_color_map, index, &p_colors.symbol);
					if (r_state.pending_at_rule) {
						++r_state.at_rule_depth;
						r_state.pending_at_rule = false;
					} else {
						++r_state.rule_depth;
						r_state.expect_property = true;
					}
					r_state.expect_value = false;
					r_state.in_value = false;
					r_state.in_at_rule_prelude = false;
					++index;
					continue;
				}
				if (character == '}') {
					set_color(r_color_map, index, &p_colors.symbol);
					if (r_state.rule_depth > 0) --r_state.rule_depth;
					else if (r_state.at_rule_depth > 0) --r_state.at_rule_depth;
					r_state.expect_property = false;
					r_state.expect_value = false;
					r_state.in_value = false;
					r_state.in_at_rule_prelude = false;
					++index;
					continue;
				}
				if (character == ';') {
					set_color(r_color_map, index, &p_colors.symbol);
					if (r_state.rule_depth > 0) {
						r_state.expect_property = true;
						r_state.expect_value = false;
						r_state.in_value = false;
					}
					r_state.pending_at_rule = false;
					r_state.in_at_rule_prelude = false;
					++index;
					continue;
				}
				if (character == ':') {
					set_color(r_color_map, index, &p_colors.symbol);
					if (r_state.rule_depth > 0 && r_state.expect_value) {
						r_state.expect_value = false;
						r_state.in_value = true;
					}
					++index;
					continue;
				}
				if (character == '#') {
					const int32_t start = index++;
					if (r_state.rule_depth > 0 && r_state.in_value) {
						while (index < end && is_css_hex_digit(p_line[index])) ++index;
						set_range_color(r_color_map, start, index, p_colors.number, line_length);
					} else {
						while (index < end && is_name_character(p_line[index])) ++index;
						set_range_color(r_color_map, start, index, p_colors.selector, line_length);
					}
					continue;
				}
				if (is_number_start(p_line, index, end)) {
					const int32_t start = index;
					index = scan_number(p_line, index, end);
					set_range_color(r_color_map, start, index, p_colors.number, line_length);
					continue;
				}
				if (is_name_character(character) || character == '\\') {
					const int32_t start = index;
					index = scan_name(p_line, index, end);
					const int32_t next = skip_spaces(p_line, index, end);
					const bool is_function = next < end && p_line[next] == '(';
					if (r_state.rule_depth > 0 && r_state.expect_property) {
						set_range_color(r_color_map, start, index, p_colors.property, line_length);
						r_state.expect_property = false;
						r_state.expect_value = true;
					} else if (is_function && (r_state.in_value || r_state.in_at_rule_prelude)) {
						set_range_color(r_color_map, start, index, p_colors.function, line_length);
					} else if (r_state.in_value || r_state.in_at_rule_prelude) {
						set_range_color(r_color_map, start, index, p_colors.value, line_length);
					} else {
						set_range_color(r_color_map, start, index, p_colors.selector, line_length);
					}
					continue;
				}

				set_color(r_color_map, index, &p_colors.symbol);
				++index;
			}
		}

		int32_t find_style_end_tag(const String& p_line, int32_t p_start) {
			for (int32_t index = std::max(0, p_start); index < p_line.length(); ++index) {
				if (!matches_case_insensitive_at(p_line, index, "</style")) continue;
				const int32_t boundary = index + 7;
				if (boundary == p_line.length() || p_line[boundary] == '>' || p_line[boundary] == '/' ||
					p_line[boundary] == ' ' || p_line[boundary] == '\t' || p_line[boundary] == '\r' || p_line[boundary] == '\n') {
					return index;
				}
			}
			return -1;
		}
	} // namespace css_highlight

	CSSSyntaxHighlighter::CSSSyntaxHighlighter() {
		_update_cache();
	}

	String CSSSyntaxHighlighter::_get_name() const {
		return "CSS";
	}

	PackedStringArray CSSSyntaxHighlighter::_get_supported_languages() const {
		return PackedStringArray{ "css" };
	}

	Ref<EditorSyntaxHighlighter> CSSSyntaxHighlighter::_create() const {
		Ref<CSSSyntaxHighlighter> instance;
		instance.instantiate();
		return instance;
	}

	Dictionary CSSSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
		TextEdit* text_edit = get_text_edit();
		if (text_edit == nullptr || p_line < 0 || p_line >= text_edit->get_line_count()) return {};

		const uint32_t text_version = text_edit->get_version();
		const int32_t line_count = text_edit->get_line_count();
		if (cached_text_version != text_version || static_cast<int32_t>(line_state_cache.size()) != line_count) {
			line_state_cache.assign(line_count, css_highlight::CSSLexState{});
			line_state_cache_valid.assign(line_count, 0);
			cached_text_version = text_version;
		}

		css_highlight::CSSLexState state;
		int32_t first_line = 0;
		for (int32_t line = p_line - 1; line >= 0; --line) {
			if (line_state_cache_valid[line] != 0) {
				state = line_state_cache[line];
				first_line = line + 1;
				break;
			}
		}
		for (int32_t line = first_line; line < p_line; ++line) {
			css_highlight::scan_css_line(text_edit->get_line(line), state, colors, nullptr);
			line_state_cache[line] = state;
			line_state_cache_valid[line] = 1;
		}

		Dictionary color_map;
		css_highlight::scan_css_line(text_edit->get_line(p_line), state, colors, &color_map);
		line_state_cache[p_line] = state;
		line_state_cache_valid[p_line] = 1;
		return color_map;
	}

	void CSSSyntaxHighlighter::_clear_highlighting_cache() {
		line_state_cache.clear();
		line_state_cache_valid.clear();
		cached_text_version = std::numeric_limits<uint32_t>::max();
	}

	void CSSSyntaxHighlighter::_update_cache() {
		colors = css_highlight::get_css_highlight_colors();
	}
} // namespace godot
