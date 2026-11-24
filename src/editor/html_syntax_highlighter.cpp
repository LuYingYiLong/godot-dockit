#include "html_syntax_highlighter.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/text_edit.hpp>

#include <cstring>

namespace godot {
	namespace {
		struct HTMLLexState {
			bool in_comment = false;
			bool in_tag = false;
			bool expect_tag_name = false;
			bool expect_value = false;
			char32_t quote = 0;
		};

		struct HighlightColors {
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
				p_character == '-' || p_character == '_' || p_character == ':' || p_character == '.';
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

		void scan_line(const String& p_line, HTMLLexState& r_state, const HighlightColors& p_colors, Dictionary* r_color_map) {
			const int32_t length = p_line.length();
			int32_t index = 0;

			if (r_state.in_comment) set_color(r_color_map, 0, &p_colors.comment);
			else if (r_state.quote != 0) set_color(r_color_map, 0, &p_colors.string);

			while (index < length) {
				if (r_state.in_comment) {
					int32_t end = index;
					while (end < length && !matches_at(p_line, end, "-->")) ++end;
					if (end == length) {
						set_range_color(r_color_map, index, length, p_colors.comment, length);
						break;
					}
					end += 3;
					set_range_color(r_color_map, index, end, p_colors.comment, length);
					r_state.in_comment = false;
					index = end;
					continue;
				}

				if (r_state.quote != 0) {
					int32_t end = index;
					while (end < length && p_line[end] != r_state.quote) ++end;
					if (end == length) {
						set_range_color(r_color_map, index, length, p_colors.string, length);
						break;
					}
					++end;
					set_range_color(r_color_map, index, end, p_colors.string, length);
					r_state.quote = 0;
					r_state.expect_value = false;
					index = end;
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
					set_color(r_color_map, index, &p_colors.symbol);
					if (can_start_tag(p_line, index)) {
						r_state.in_tag = true;
						r_state.expect_tag_name = true;
						r_state.expect_value = false;
					}
					else if (index + 1 < length) {
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
					if (character == '>' || character == '/') {
						r_state.expect_value = false;
						continue;
					}
					if (character == '\'' || character == '"') {
						r_state.quote = character;
						set_color(r_color_map, index, &p_colors.string);
						++index;
						continue;
					}
					const int32_t start = index;
					while (index < length && p_line[index] != ' ' && p_line[index] != '\t' && p_line[index] != '>' && p_line[index] != '\r') ++index;
					set_range_color(r_color_map, start, index, p_colors.string, length);
					r_state.expect_value = false;
					continue;
				}

				if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
					++index;
					continue;
				}
				if (character == '>') {
					set_color(r_color_map, index, &p_colors.symbol);
					r_state.in_tag = false;
					r_state.expect_tag_name = false;
					++index;
					if (index < length) set_color(r_color_map, index, nullptr);
					continue;
				}
				if (character == '/' || character == '!') {
					set_color(r_color_map, index, &p_colors.symbol);
					++index;
					continue;
				}
				if (character == '=') {
					set_color(r_color_map, index, &p_colors.symbol);
					r_state.expect_value = true;
					++index;
					continue;
				}
				if (character == '\'' || character == '"') {
					r_state.quote = character;
					set_color(r_color_map, index, &p_colors.string);
					++index;
					continue;
				}

				if (r_state.expect_tag_name) {
					if (is_name_character(character)) {
						const int32_t start = index;
						while (index < length && is_name_character(p_line[index])) ++index;
						set_range_color(r_color_map, start, index, p_colors.tag, length);
						r_state.expect_tag_name = false;
						continue;
					}
					++index;
					continue;
				}

				if (is_name_character(character)) {
					const int32_t start = index;
					while (index < length && is_name_character(p_line[index])) ++index;
					set_range_color(r_color_map, start, index, p_colors.attribute, length);
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

		const HighlightColors colors{ tag_color, attribute_color, string_color, comment_color, symbol_color };
		HTMLLexState state;
		for (int32_t line = 0; line < p_line; ++line) scan_line(text_edit->get_line(line), state, colors, nullptr);
		Dictionary color_map;
		scan_line(text_edit->get_line(p_line), state, colors, &color_map);
		return color_map;
	}

	void HTMLSyntaxHighlighter::_clear_highlighting_cache() {}

	void HTMLSyntaxHighlighter::_update_cache() {
		tag_color = get_highlighting_color(setting_name("keyword_color"), Color(0.52, 0.78, 0.95));
		attribute_color = get_highlighting_color(setting_name("member_variable_color"), Color(0.91, 0.72, 0.43));
		string_color = get_highlighting_color(setting_name("string_color"), Color(0.77, 0.64, 0.91));
		comment_color = get_highlighting_color(setting_name("comment_color"), Color(0.55, 0.62, 0.58));
		symbol_color = get_highlighting_color(setting_name("symbol_color"), Color(0.78, 0.82, 0.86));
	}
} // namespace godot
