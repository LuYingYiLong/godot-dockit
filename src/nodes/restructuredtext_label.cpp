#include "restructuredtext_label.h"

#include "formats/rst/rst_document_parser.h"
#include "formats/rst/rst_reference_index.h"
#include "formats/html/resources/html_resource_paths.h"
#include "formats/word_boundaries.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/h_scroll_bar.hpp>
#include <godot_cpp/classes/v_scroll_bar.hpp>

#include <algorithm>
#include <climits>
#include <cmath>
#include <string>
#include <vector>

using namespace godot;

namespace {
	static String localize_project_path(const String& p_path) {
		String path = p_path.strip_edges().replace("\\", "/");
		ProjectSettings* project_settings = ProjectSettings::get_singleton();
		if (project_settings != nullptr && path.is_absolute_path()) {
			path = project_settings->localize_path(path);
		}
		return path.simplify_path();
	}

	struct RSTAdmonitionDecoration {
		String directive;
		String title;
		String needle;
	};

	static String utf8_slice(const std::string& p_text, size_t p_from, size_t p_length) {
		if (p_length == 0) {
			return String();
		}

		return String::utf8(p_text.data() + p_from, static_cast<int64_t>(p_length));
	}

	static String escape_bbcode(const String& p_text) {
		String escaped;

		for (int32_t i = 0; i < p_text.length(); i++) {
			const char32_t character = p_text[i];
			if (character == '[') {
				escaped += "[lb]";
			}
			else if (character == ']') {
				escaped += "[rb]";
			}
			else {
				escaped += character;
			}
		}

		return escaped;
	}

	static String bbcode_color(const Color& p_color) {
		return String("#") + p_color.to_html();
	}

	static String wrap_color(const String& p_text, const Color& p_color) {
		return String("[color=") + bbcode_color(p_color) + "]" + p_text + "[/color]";
	}

	static String wrap_font_size(const String& p_text, int32_t p_font_size) {
		return String("[font_size=") + String::num_int64(p_font_size) + "]" + p_text + "[/font_size]";
	}

	static String wrap_background(const String& p_text, const Color& p_color) {
		return String("[bgcolor=") + bbcode_color(p_color) + "]" + p_text + "[/bgcolor]";
	}

	static bool split_angle_target(const String& p_content, String& r_label, String& r_target) {
		const int64_t marker_position = p_content.rfind("<");
		if (marker_position < 0 || !p_content.ends_with(">")) {
			return false;
		}

		r_label = p_content.substr(0, marker_position).strip_edges();
		r_target = p_content.substr(marker_position + 1, p_content.length() - marker_position - 2).strip_edges();
		if (r_label.is_empty()) {
			r_label = r_target;
		}

		return !r_target.is_empty();
	}

	static void append_escaped_slice(String& r_output, const std::string& p_text, size_t p_from, size_t p_length) {
		r_output += escape_bbcode(utf8_slice(p_text, p_from, p_length));
	}

	static String make_url(const String& p_uri, const String& p_label) {
		return String("[url=") + escape_bbcode(p_uri) + "]" + escape_bbcode(p_label) + "[/url]";
	}

	static String quote_bbcode_option(const String& p_value) {
		return String("\"") + p_value.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
	}

	static BitField<TextServer::LineBreakFlag> get_autowrap_trim_mask() {
		return TextServer::BREAK_TRIM_INDENT | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	}

	static BitField<TextServer::LineBreakFlag> get_autowrap_break_flags(const RestructuredtextLabel* p_owner) {
		const TextServer::AutowrapMode mode = p_owner == nullptr ? TextServer::AUTOWRAP_WORD_SMART : p_owner->get_autowrap_mode();
		BitField<TextServer::LineBreakFlag> flags = TextServer::BREAK_MANDATORY;

		switch (mode) {
			case TextServer::AUTOWRAP_WORD_SMART:
				flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_ADAPTIVE | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_WORD:
				flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_ARBITRARY:
				flags = TextServer::BREAK_GRAPHEME_BOUND | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_OFF:
				break;
		}

		const BitField<TextServer::LineBreakFlag> trim_flags = p_owner == nullptr
				? BitField<TextServer::LineBreakFlag>(TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES)
				: p_owner->get_autowrap_trim_flags();
		return flags | trim_flags;
	}

	static void configure_paragraph_autowrap(const Ref<TextParagraph>& p_paragraph, const RestructuredtextLabel* p_owner) {
		if (p_paragraph.is_valid()) {
			p_paragraph->set_break_flags(get_autowrap_break_flags(p_owner));
		}
	}

	static float parse_scale_percent(const String& p_value, float p_fallback = 100.0f) {
		String value = p_value.strip_edges();
		if (value.is_empty()) {
			return p_fallback;
		}
		if (value.ends_with("%")) {
			value = value.substr(0, value.length() - 1).strip_edges();
		}

		const float percent = value.to_float();
		return percent > 0.0f ? percent : p_fallback;
	}

	static void append_plain_slice(String& r_output, const std::string& p_text, size_t p_from, size_t p_length) {
		r_output += utf8_slice(p_text, p_from, p_length);
	}

	static String render_role_plain(const String& p_content) {
		String label;
		String target;

		if (split_angle_target(p_content, label, target)) {
			return label;
		}

		return p_content.strip_edges();
	}

	static bool parse_footnote_reference(const std::string& p_text, size_t p_index, String& r_label, size_t& r_next_index) {
		if (p_index >= p_text.size() || p_text[p_index] != '[') {
			return false;
		}

		const size_t end = p_text.find("]_", p_index + 1);
		if (end == std::string::npos || end <= p_index + 1) {
			return false;
		}

		r_label = utf8_slice(p_text, p_index + 1, end - p_index - 1);
		r_next_index = end + 2;
		return true;
	}

	static String render_inline_plain(const String& p_text, const RSTFootnoteMap* p_footnotes = nullptr) {
		CharString utf8 = p_text.utf8();
		const std::string text(utf8.get_data(), static_cast<size_t>(utf8.length()));
		String output;
		size_t index = 0;

		while (index < text.size()) {
			String footnote_label;
			size_t footnote_next_index = 0;
			if (parse_footnote_reference(text, index, footnote_label, footnote_next_index)) {
				output += format_footnote_label(footnote_label, p_footnotes);
				index = footnote_next_index;
				continue;
			}

			if (text.compare(index, 2, "``") == 0) {
				const size_t end = text.find("``", index + 2);
				if (end != std::string::npos) {
					append_plain_slice(output, text, index + 2, end - index - 2);
					index = end + 2;
					continue;
				}
			}

			if (text[index] == ':') {
				const size_t role_end = text.find(":`", index + 1);
				if (role_end != std::string::npos) {
					bool valid_role = role_end > index + 1;
					for (size_t i = index + 1; i < role_end; i++) {
						const char character = text[i];
						if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_' || character == '-')) {
							valid_role = false;
							break;
						}
					}

					if (valid_role) {
						const size_t end = text.find('`', role_end + 2);
						if (end != std::string::npos) {
							const String content = utf8_slice(text, role_end + 2, end - role_end - 2);
							output += render_role_plain(content);
							index = end + 1;
							continue;
						}
					}
				}
			}

			if (text[index] == '`') {
				const size_t end = text.find('`', index + 1);
				if (end != std::string::npos && end + 1 < text.size() && text[end + 1] == '_') {
					const String content = utf8_slice(text, index + 1, end - index - 1);
					output += render_role_plain(content);
					index = end + 2;
					continue;
				}
			}

			if (text.compare(index, 2, "**") == 0) {
				const size_t end = text.find("**", index + 2);
				if (end != std::string::npos) {
					append_plain_slice(output, text, index + 2, end - index - 2);
					index = end + 2;
					continue;
				}
			}

			if (text[index] == '*') {
				const size_t end = text.find('*', index + 1);
				if (end != std::string::npos) {
					append_plain_slice(output, text, index + 1, end - index - 1);
					index = end + 1;
					continue;
				}
			}

			const size_t next = text.find_first_of("*`:[", index + 1);
			const size_t length = next == std::string::npos ? text.size() - index : next - index;
			append_plain_slice(output, text, index, length);
			index += length;
		}

		return output;
	}

	static bool is_search_word_character(char32_t p_character) {
		return (p_character >= 'a' && p_character <= 'z') || (p_character >= 'A' && p_character <= 'Z') || (p_character >= '0' && p_character <= '9') || p_character == '_';
	}

	static bool is_whole_word_match(const String& p_text, int64_t p_start, int64_t p_length) {
		if (p_start > 0 && is_search_word_character(p_text[p_start - 1])) {
			return false;
		}

		const int64_t end = p_start + p_length;
		if (end < p_text.length() && is_search_word_character(p_text[end])) {
			return false;
		}

		return true;
	}

	static void append_inline_span(std::vector<RSTInlineSpan>& r_spans, String& r_text, const String& p_text, bool p_bold = false, bool p_italic = false, bool p_code = false, const String& p_link_uri = String(), bool p_custom_color = false, const Color& p_color = Color()) {
		if (p_text.is_empty()) {
			return;
		}

		if (!r_spans.empty()) {
			RSTInlineSpan& previous = r_spans.back();
			if (previous.bold == p_bold && previous.italic == p_italic && previous.code == p_code && previous.link_uri == p_link_uri && previous.custom_color == p_custom_color && (!p_custom_color || previous.color == p_color) && previous.end == r_text.length()) {
				previous.text += p_text;
				r_text += p_text;
				previous.end = r_text.length();
				return;
			}
		}

		RSTInlineSpan span;
		span.text = p_text;
		span.start = r_text.length();
		r_text += p_text;
		span.end = r_text.length();
		span.bold = p_bold;
		span.italic = p_italic;
		span.code = p_code;
		span.link_uri = p_link_uri;
		span.custom_color = p_custom_color;
		span.color = p_color;
		r_spans.push_back(span);
	}

	static bool is_link_role(const String& p_role) {
		return p_role == "ref" || p_role == "doc" || p_role == "download";
	}

	static bool is_code_role(const String& p_role) {
		return p_role == "class" || p_role == "meth" || p_role == "func" || p_role == "attr" || p_role == "data" || p_role == "const" || p_role == "enum" || p_role == "kbd";
	}

	static void append_role_inline_span(std::vector<RSTInlineSpan>& r_spans, String& r_text, const String& p_role, const String& p_content, const RestructuredtextLabel* p_owner) {
		String label;
		String target;

		if (!split_angle_target(p_content, label, target)) {
			label = p_content.strip_edges();
			target = label;
		}

		if (is_link_role(p_role)) {
			const String uri = p_owner == nullptr ? p_role + String(":") + target : p_owner->resolve_link_uri(p_role, target);
			append_inline_span(r_spans, r_text, label, false, false, false, uri);
			return;
		}
		if (is_code_role(p_role)) {
			append_inline_span(r_spans, r_text, label, false, false, true);
			return;
		}

		append_inline_span(r_spans, r_text, label);
	}

	static String parse_inline_spans(const String& p_text, const RestructuredtextLabel* p_owner, std::vector<RSTInlineSpan>& r_spans, const RSTFootnoteMap* p_footnotes = nullptr) {
		CharString utf8 = p_text.utf8();
		const std::string text(utf8.get_data(), static_cast<size_t>(utf8.length()));
		String output;
		size_t index = 0;

		while (index < text.size()) {
			String footnote_label;
			size_t footnote_next_index = 0;
			if (parse_footnote_reference(text, index, footnote_label, footnote_next_index)) {
				append_inline_span(r_spans, output, format_footnote_label(footnote_label, p_footnotes));
				index = footnote_next_index;
				continue;
			}

			if (text.compare(index, 2, "``") == 0) {
				const size_t end = text.find("``", index + 2);
				if (end != std::string::npos) {
					append_inline_span(r_spans, output, utf8_slice(text, index + 2, end - index - 2), false, false, true);
					index = end + 2;
					continue;
				}
			}

			if (text[index] == ':') {
				const size_t role_end = text.find(":`", index + 1);
				if (role_end != std::string::npos) {
					bool valid_role = role_end > index + 1;
					for (size_t i = index + 1; i < role_end; i++) {
						const char character = text[i];
						if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_' || character == '-')) {
							valid_role = false;
							break;
						}
					}

					if (valid_role) {
						const size_t end = text.find('`', role_end + 2);
						if (end != std::string::npos) {
							const String role = utf8_slice(text, index + 1, role_end - index - 1);
							const String content = utf8_slice(text, role_end + 2, end - role_end - 2);
							append_role_inline_span(r_spans, output, role, content, p_owner);
							index = end + 1;
							continue;
						}
					}
				}
			}

			if (text[index] == '`') {
				const size_t end = text.find('`', index + 1);
				if (end != std::string::npos && end + 1 < text.size() && text[end + 1] == '_') {
					const String content = utf8_slice(text, index + 1, end - index - 1);
					String label;
					String uri;

					if (split_angle_target(content, label, uri)) {
						append_inline_span(r_spans, output, label, false, false, false, uri);
					}
					else {
						const String target = content.strip_edges();
						const String resolved_uri = p_owner == nullptr ? String("ref:") + target : p_owner->resolve_link_uri("ref", target);
						append_inline_span(r_spans, output, target, false, false, false, resolved_uri);
					}

					index = end + 2;
					continue;
				}
			}

			if (text.compare(index, 2, "**") == 0) {
				const size_t end = text.find("**", index + 2);
				if (end != std::string::npos) {
					append_inline_span(r_spans, output, utf8_slice(text, index + 2, end - index - 2), true);
					index = end + 2;
					continue;
				}
			}

			if (text[index] == '*') {
				const size_t end = text.find('*', index + 1);
				if (end != std::string::npos) {
					append_inline_span(r_spans, output, utf8_slice(text, index + 1, end - index - 1), false, true);
					index = end + 1;
					continue;
				}
			}

			const size_t next = text.find_first_of("*`:[", index + 1);
			const size_t length = next == std::string::npos ? text.size() - index : next - index;
			append_inline_span(r_spans, output, utf8_slice(text, index, length));
			index += length;
		}

		return output;
	}

	static void append_parsed_inline_spans(std::vector<RSTInlineSpan>& r_spans, String& r_text, const String& p_text, const RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes = nullptr) {
		std::vector<RSTInlineSpan> parsed_spans;
		parse_inline_spans(p_text, p_owner, parsed_spans, p_footnotes);
		for (const RSTInlineSpan& span : parsed_spans) {
			append_inline_span(r_spans, r_text, span.text, span.bold, span.italic, span.code, span.link_uri);
		}
	}

	static String render_role(const String& p_role, const String& p_content, const RestructuredtextLabel* p_owner) {
		String label;
		String target;

		if (!split_angle_target(p_content, label, target)) {
			label = p_content.strip_edges();
			target = label;
		}

		if (p_role == "ref" || p_role == "doc" || p_role == "download") {
			const String uri = p_owner == nullptr ? p_role + String(":") + target : p_owner->resolve_link_uri(p_role, target);
			return make_url(uri, label);
		}
		if (p_role == "class" || p_role == "meth" || p_role == "func" || p_role == "attr" || p_role == "data" || p_role == "const" || p_role == "enum" || p_role == "kbd") {
			return String("[code]") + escape_bbcode(label) + "[/code]";
		}

		return escape_bbcode(label);
	}

	static String render_inline(const String& p_text, const RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes = nullptr) {
		CharString utf8 = p_text.utf8();
		const std::string text(utf8.get_data(), static_cast<size_t>(utf8.length()));
		String output;
		size_t index = 0;

		while (index < text.size()) {
			String footnote_label;
			size_t footnote_next_index = 0;
			if (parse_footnote_reference(text, index, footnote_label, footnote_next_index)) {
				output += escape_bbcode(format_footnote_label(footnote_label, p_footnotes));
				index = footnote_next_index;
				continue;
			}

			if (text.compare(index, 2, "``") == 0) {
				const size_t end = text.find("``", index + 2);
				if (end != std::string::npos) {
					output += "[code]";
					append_escaped_slice(output, text, index + 2, end - index - 2);
					output += "[/code]";
					index = end + 2;
					continue;
				}
			}

			if (text[index] == ':') {
				const size_t role_end = text.find(":`", index + 1);
				if (role_end != std::string::npos) {
					bool valid_role = role_end > index + 1;
					for (size_t i = index + 1; i < role_end; i++) {
						const char character = text[i];
						if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_' || character == '-')) {
							valid_role = false;
							break;
						}
					}

					if (valid_role) {
						const size_t end = text.find('`', role_end + 2);
						if (end != std::string::npos) {
							const String role = utf8_slice(text, index + 1, role_end - index - 1);
							const String content = utf8_slice(text, role_end + 2, end - role_end - 2);
							output += render_role(role, content, p_owner);
							index = end + 1;
							continue;
						}
					}
				}
			}

			if (text[index] == '`') {
				const size_t end = text.find('`', index + 1);
				if (end != std::string::npos && end + 1 < text.size() && text[end + 1] == '_') {
					const String content = utf8_slice(text, index + 1, end - index - 1);
					String label;
					String uri;

					if (split_angle_target(content, label, uri)) {
						output += make_url(uri, label);
					}
					else {
						const String target = content.strip_edges();
						const String uri = p_owner == nullptr ? String("ref:") + target : p_owner->resolve_link_uri("ref", target);
						output += make_url(uri, target);
					}

					index = end + 2;
					continue;
				}
			}

			if (text.compare(index, 2, "**") == 0) {
				const size_t end = text.find("**", index + 2);
				if (end != std::string::npos) {
					output += "[b]";
					append_escaped_slice(output, text, index + 2, end - index - 2);
					output += "[/b]";
					index = end + 2;
					continue;
				}
			}

			if (text[index] == '*') {
				const size_t end = text.find('*', index + 1);
				if (end != std::string::npos) {
					output += "[i]";
					append_escaped_slice(output, text, index + 1, end - index - 1);
					output += "[/i]";
					index = end + 1;
					continue;
				}
			}

			const size_t next = text.find_first_of("*`:[", index + 1);
			const size_t length = next == std::string::npos ? text.size() - index : next - index;
			append_escaped_slice(output, text, index, length);
			index += length;
		}

		return output;
	}

	static int32_t get_theme_font_size_or(Control* p_owner, const StringName& p_type, const StringName& p_name, int32_t p_default) {
		if (p_owner != nullptr && p_owner->has_theme_font_size_override(p_name)) {
			const int32_t value = p_owner->get_theme_font_size(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}

		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_font_size(p_name, p_type)) {
			const int32_t value = theme->get_font_size(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}
		return p_default;
	}

	static int32_t get_theme_constant_or(Control* p_owner, const StringName& p_type, const StringName& p_name, int32_t p_default) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_constant(p_name, p_type)) {
			return theme->get_constant(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_constant(p_name, p_type)) {
			return p_owner->get_theme_constant(p_name, p_type);
		}

		return p_default;
	}

	static Color get_theme_color_or(Control* p_owner, const StringName& p_type, const StringName& p_name, const Color& p_default) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_color(p_name, p_type)) {
			return theme->get_color(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_color(p_name, p_type)) {
			return p_owner->get_theme_color(p_name, p_type);
		}

		return p_default;
	}

	static Ref<Font> get_theme_font_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_font(p_name, p_type)) {
			return theme->get_font(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_font(p_name, p_type)) {
			return p_owner->get_theme_font(p_name, p_type);
		}
		if (p_owner != nullptr) {
			return p_owner->get_theme_default_font();
		}

		return Ref<Font>();
	}

	static Ref<StyleBox> get_theme_stylebox_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_stylebox(p_name, p_type)) {
			return theme->get_stylebox(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_stylebox(p_name, p_type)) {
			return p_owner->get_theme_stylebox(p_name, p_type);
		}

		return Ref<StyleBox>();
	}

	static StringName get_admonition_theme_item_name(const String& p_directive, const String& p_item) {
		String directive = p_directive.to_lower();
		if (directive == "admonition") {
			directive = "custom";
		}
		return StringName(String("admonition_") + directive + String("_") + p_item);
	}

	static int32_t get_admonition_theme_font_size_or(Control* p_owner, const String& p_directive, const String& p_item, int32_t p_default) {
		const StringName type = "RestructuredtextLabel";
		const int32_t shared_value = get_theme_font_size_or(p_owner, type, StringName(String("admonition_") + p_item), p_default);
		return get_theme_font_size_or(p_owner, type, get_admonition_theme_item_name(p_directive, p_item), shared_value);
	}

	static int32_t get_admonition_theme_constant_or(Control* p_owner, const String& p_directive, const String& p_item, int32_t p_default) {
		const StringName type = "RestructuredtextLabel";
		const int32_t shared_value = get_theme_constant_or(p_owner, type, StringName(String("admonition_") + p_item), p_default);
		return get_theme_constant_or(p_owner, type, get_admonition_theme_item_name(p_directive, p_item), shared_value);
	}

	static Color get_admonition_theme_color_or(Control* p_owner, const String& p_directive, const String& p_item, const Color& p_default) {
		const StringName type = "RestructuredtextLabel";
		const Color shared_value = get_theme_color_or(p_owner, type, StringName(String("admonition_") + p_item), p_default);
		return get_theme_color_or(p_owner, type, get_admonition_theme_item_name(p_directive, p_item), shared_value);
	}

	static Ref<StyleBox> get_admonition_theme_stylebox_or(Control* p_owner, const String& p_directive, const String& p_item) {
		const StringName type = "RestructuredtextLabel";
		const Ref<StyleBox> specific = get_theme_stylebox_or(p_owner, type, get_admonition_theme_item_name(p_directive, p_item));
		return specific.is_valid() ? specific : get_theme_stylebox_or(p_owner, type, StringName(String("admonition_") + p_item));
	}

	static Ref<StyleBoxFlat> create_internal_stylebox(const Color& p_background_color, const Color& p_border_color, int32_t p_border_left, int32_t p_border_top, int32_t p_border_right, int32_t p_border_bottom, float p_margin_left, float p_margin_top, float p_margin_right, float p_margin_bottom, int32_t p_corner_radius = 0) {
		Ref<StyleBoxFlat> stylebox;
		stylebox.instantiate();
		stylebox->set_bg_color(p_background_color);
		stylebox->set_border_color(p_border_color);
		stylebox->set_border_width(SIDE_LEFT, p_border_left);
		stylebox->set_border_width(SIDE_TOP, p_border_top);
		stylebox->set_border_width(SIDE_RIGHT, p_border_right);
		stylebox->set_border_width(SIDE_BOTTOM, p_border_bottom);
		stylebox->set_content_margin(SIDE_LEFT, p_margin_left);
		stylebox->set_content_margin(SIDE_TOP, p_margin_top);
		stylebox->set_content_margin(SIDE_RIGHT, p_margin_right);
		stylebox->set_content_margin(SIDE_BOTTOM, p_margin_bottom);
		stylebox->set_corner_radius_all(p_corner_radius);
		return stylebox;
	}

	static float get_stylebox_margin_or(const Ref<StyleBox>& p_stylebox, Side p_side, float p_default) {
		if (p_stylebox.is_valid()) {
			return p_stylebox->get_margin(p_side);
		}

		return p_default;
	}

	static Color default_admonition_accent(const String& p_directive) {
		if (p_directive == "warning" || p_directive == "caution" || p_directive == "attention") {
			return Color(0.95, 0.61, 0.14, 1.0);
		}
		if (p_directive == "danger" || p_directive == "error") {
			return Color(0.86, 0.22, 0.22, 1.0);
		}
		if (p_directive == "important") {
			return Color(0.20, 0.46, 0.95, 1.0);
		}
		if (p_directive == "tip" || p_directive == "hint") {
			return Color(0.03, 0.63, 0.68, 1.0);
		}
		if (p_directive == "seealso") {
			return Color(0.51, 0.38, 0.86, 1.0);
		}

		return Color(0.0, 0.796, 0.357, 1.0);
	}

	static String render_heading_block(const RSTBlock& p_block, RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes = nullptr) {
		const int32_t base_size = 16;
		const int32_t extra = p_owner == nullptr ? 0 : p_owner->get_extra_font_size();
		const int32_t fallback_sizes[] = { base_size + 12, base_size + 8, base_size + 5, base_size + 3, base_size + 1 };
		const int32_t index = std::min<int32_t>(std::max<int32_t>(p_block.level - 1, 0), 4);
		const String level_prefix = String("heading_") + String::num_int64(index + 1) + String("_");
		const int32_t size = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", StringName(level_prefix + String("font_size")), fallback_sizes[index]) + extra);
		const Color color = get_theme_color_or(p_owner, "RestructuredtextLabel", StringName(level_prefix + String("font_color")), Color(1, 1, 1, 1));
		return wrap_color(wrap_font_size(String("[b]") + render_inline(p_block.text, p_owner, p_footnotes) + "[/b]", size), color);
	}

	static String render_list_block(const RSTBlock& p_block, bool p_ordered, RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes = nullptr) {
		String bbcode = p_ordered ? String("[ol type=1]\n") : String("[ul]\n");
		for (int32_t i = 0; i < p_block.items.size(); i++) {
			bbcode += render_inline(p_block.items[i], p_owner, p_footnotes);
			bbcode += "\n";
		}
		bbcode += p_ordered ? String("[/ol]") : String("[/ul]");
		return bbcode;
	}

	static String render_code_block(const RSTBlock& p_block, RestructuredtextLabel* p_owner) {
		const int32_t base_size = 16;
		const int32_t extra = p_owner == nullptr ? 0 : p_owner->get_extra_font_size();
		const int32_t size = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "code_block_font_size", base_size) + extra);
		const Color text_color = get_theme_color_or(p_owner, "RestructuredtextLabel", "code_block_font_color", Color(0.86, 0.88, 0.91, 1.0));
		const String code = wrap_font_size(String("[code]") + escape_bbcode(p_block.text) + "[/code]", size);
		return wrap_color(code, text_color);
	}

	static String render_admonition_block(const RSTBlock& p_block, RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes = nullptr) {
		const int32_t base_size = 16;
		const int32_t extra = p_owner == nullptr ? 0 : p_owner->get_extra_font_size();
		const int32_t size = std::max<int32_t>(1, get_admonition_theme_font_size_or(p_owner, p_block.directive, "font_size", base_size) + extra);
		const Color text_color = get_admonition_theme_color_or(p_owner, p_block.directive, "font_color", Color(0.86, 0.88, 0.91, 1.0));

		const Color accent = get_admonition_theme_color_or(p_owner, p_block.directive, "accent_color", default_admonition_accent(p_block.directive));
		const Color border_color = accent;
		const Color header_color = accent;
		const Color background_color = Color(0.10, 0.12, 0.14, 1.0);
		const Color header_font_color = get_admonition_theme_color_or(p_owner, p_block.directive, "header_font_color", Color(1, 1, 1, 1));
		const int32_t internal_header_margin = 6;
		const int32_t internal_body_margin = 8;
		const int32_t internal_body_margin_left = 12;

		String bbcode = "[table=1]";
		bbcode += String("[cell expand=1 border=") + bbcode_color(border_color) + " bg=" + bbcode_color(header_color) + " padding=" + String::num_int64(internal_header_margin) + "," + String::num_int64(internal_header_margin) + "," + String::num_int64(internal_header_margin) + "," + String::num_int64(internal_header_margin) + "]";
		bbcode += wrap_color(String("[b]") + escape_bbcode(p_block.argument) + "[/b]", header_font_color);
		bbcode += "[/cell]";

		if (!p_block.text.is_empty()) {
			bbcode += String("[cell expand=1 border=") + bbcode_color(border_color) + " bg=" + bbcode_color(background_color) + " padding=" + String::num_int64(internal_body_margin_left) + "," + String::num_int64(internal_body_margin) + "," + String::num_int64(internal_body_margin) + "," + String::num_int64(internal_body_margin) + "]";
			bbcode += render_inline(p_block.text, p_owner, p_footnotes);
			bbcode += "[/cell]";
		}

		bbcode += "[/table]";
		return wrap_color(wrap_font_size(bbcode, size), text_color);
	}

	static String get_admonition_plain_text(const RSTBlock& p_block, const RSTFootnoteMap* p_footnotes = nullptr) {
		String text = p_block.argument;
		if (!p_block.text.is_empty()) {
			text += "\n";
			text += render_inline_plain(p_block.text, p_footnotes);
		}

		return text;
	}

	static String render_table_block(const RSTBlock& p_block, RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes = nullptr) {
		if (p_block.columns <= 0) {
			return String();
		}

		String bbcode = String("[table=") + String::num_int64(p_block.columns) + "]";
		for (int32_t row_index = 0; row_index < static_cast<int32_t>(p_block.rows.size()); row_index++) {
			const PackedStringArray row = p_block.rows[row_index];
			for (int32_t column = 0; column < p_block.columns; column++) {
				const bool header = row_index < p_block.header_rows;
				bbcode += "[cell expand=1]";
				const String cell_text = column < row.size() ? row[column] : String();
				if (header) {
					bbcode += "[b]";
				}
				bbcode += render_inline(cell_text, p_owner, p_footnotes);
				if (header) {
					bbcode += "[/b]";
				}
				bbcode += "[/cell]";
			}
		}
		bbcode += "[/table]";
		return bbcode;
	}

	static String render_image_block(const RSTBlock& p_block, RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes = nullptr) {
		if (p_block.argument.is_empty()) {
			return String();
		}
		const String image_path = p_owner == nullptr ? p_block.argument : p_owner->resolve_document_path(p_block.argument);
		const Ref<Texture2D> texture = dockit::load_html_image_file(image_path);
		if (texture.is_null()) {
			String placeholder = String("[i]Image: ") + escape_bbcode(p_block.argument) + "[/i]";
			if (!p_block.text.is_empty()) {
				placeholder += "\n";
				placeholder += String("[i]") + render_inline(p_block.text, p_owner, p_footnotes) + "[/i]";
			}

			return placeholder;
		}

		String image_tag = "[img";
		if (!p_block.image_width.is_empty()) {
			image_tag += " width=";
			image_tag += p_block.image_width;
		}
		if (!p_block.image_height.is_empty()) {
			image_tag += " height=";
			image_tag += p_block.image_height;
		}
		if (p_block.image_width.is_empty() && p_block.image_height.is_empty() && !p_block.image_scale.is_empty()) {
			if (texture.is_valid()) {
				const float scale = parse_scale_percent(p_block.image_scale) / 100.0f;
				image_tag += " width=";
				image_tag += String::num_int64(static_cast<int64_t>(std::round(static_cast<float>(texture->get_width()) * scale)));
			}
		}
		if (!p_block.image_alt.is_empty()) {
			image_tag += " alt=";
			image_tag += quote_bbcode_option(p_block.image_alt);
		}
		image_tag += "]";
		image_tag += escape_bbcode(image_path);
		image_tag += "[/img]";

		String bbcode = image_tag;
		const String align = p_block.image_align.to_lower();
		if (align == "center") {
			bbcode = String("[center]") + bbcode + "[/center]";
		}
		else if (align == "right") {
			bbcode = String("[right]") + bbcode + "[/right]";
		}

		if (!p_block.text.is_empty()) {
			bbcode += "\n";
			bbcode += String("[i]") + render_inline(p_block.text, p_owner, p_footnotes) + "[/i]";
		}

		return bbcode;
	}

	static String render_toctree_block(const RSTBlock& p_block, RestructuredtextLabel* p_owner) {
		String bbcode = String("[b]") + escape_bbcode(p_block.argument) + "[/b]\n[ul]\n";
		for (int32_t i = 0; i < p_block.items.size(); i++) {
			const String item = p_block.items[i];
			const String uri = p_owner == nullptr ? String("doc:") + item : p_owner->resolve_link_uri("doc", item);
			bbcode += make_url(uri, item);
			bbcode += "\n";
		}
		bbcode += "[/ul]";
		return bbcode;
	}

	static void append_block_bbcode(String& r_output, const String& p_block) {
		if (p_block.is_empty()) {
			return;
		}
		if (!r_output.is_empty()) {
			r_output += "\n\n";
		}

		r_output += p_block;
	}

	static String render_blocks_to_bbcode(const std::vector<RSTBlock>& p_blocks, RestructuredtextLabel* p_owner, int32_t& r_block_count, std::vector<RSTAdmonitionDecoration>& r_admonition_decorations, const RSTFootnoteMap* p_footnotes = nullptr) {
		String bbcode;
		r_block_count = 0;
		r_admonition_decorations.clear();

		for (const RSTBlock& block : p_blocks) {
			String block_bbcode;
			switch (block.type) {
			case RST_BLOCK_HEADING:
				block_bbcode = render_heading_block(block, p_owner, p_footnotes);
				break;
			case RST_BLOCK_BULLET_LIST:
				block_bbcode = render_list_block(block, false, p_owner, p_footnotes);
				break;
			case RST_BLOCK_ENUMERATED_LIST:
				block_bbcode = render_list_block(block, true, p_owner, p_footnotes);
				break;
			case RST_BLOCK_CODE:
				block_bbcode = render_code_block(block, p_owner);
				break;
			case RST_BLOCK_ADMONITION:
				block_bbcode = render_admonition_block(block, p_owner, p_footnotes);
				break;
			case RST_BLOCK_TABLE:
				block_bbcode = render_table_block(block, p_owner, p_footnotes);
				break;
			case RST_BLOCK_IMAGE:
				block_bbcode = render_image_block(block, p_owner, p_footnotes);
				break;
			case RST_BLOCK_TOCTREE:
				block_bbcode = render_toctree_block(block, p_owner);
				break;
			case RST_BLOCK_TRANSITION:
				block_bbcode = "[hr]";
				break;
			case RST_BLOCK_PARAGRAPH:
			default:
				block_bbcode = render_inline(block.text, p_owner, p_footnotes);
				break;
			}

			append_block_bbcode(bbcode, block_bbcode);
			if (!block_bbcode.is_empty()) {
				r_block_count++;
			}
		}

		return bbcode;
	}

} // namespace

void RSTDocumentCanvas::_bind_methods() {
	ClassDB::bind_method(D_METHOD("select_all"), &RSTDocumentCanvas::select_all);
	ClassDB::bind_method(D_METHOD("select_between_points", "from", "to"), &RSTDocumentCanvas::select_between_points);
	ClassDB::bind_method(D_METHOD("get_link_uri_at_point", "position"), &RSTDocumentCanvas::get_link_uri_at_point);
	ClassDB::bind_method(D_METHOD("clear_selection"), &RSTDocumentCanvas::clear_selection);
	ClassDB::bind_method(D_METHOD("search", "text", "flags", "from_line", "from_column"), &RSTDocumentCanvas::search);
	ClassDB::bind_method(D_METHOD("get_selected_text"), &RSTDocumentCanvas::get_selected_text);
	ClassDB::bind_method(D_METHOD("get_rendered_block_count"), &RSTDocumentCanvas::get_rendered_block_count);
	ClassDB::bind_method(D_METHOD("get_content_height"), &RSTDocumentCanvas::get_content_height);
	ClassDB::bind_method(D_METHOD("get_table_cell_count"), &RSTDocumentCanvas::get_table_cell_count);
	ClassDB::bind_method(D_METHOD("get_table_cell_rect", "index"), &RSTDocumentCanvas::get_table_cell_rect);
	ClassDB::bind_method(D_METHOD("has_anchor", "anchor"), &RSTDocumentCanvas::has_anchor);
	ClassDB::bind_method(D_METHOD("get_anchors"), &RSTDocumentCanvas::get_anchors);
	ClassDB::bind_method(D_METHOD("get_anchor_offset", "anchor"), &RSTDocumentCanvas::get_anchor_offset);
	ClassDB::bind_method(D_METHOD("get_search_result_y"), &RSTDocumentCanvas::get_search_result_y);
}

void RSTDocumentCanvas::set_document_view(RestructuredtextLabel* p_document_view) {
	document_view = p_document_view;
	mark_layout_dirty();
}

void RSTDocumentCanvas::set_rst_text(const String& p_text) {
	rst_text = p_text;
	selection_anchor = 0;
	selection_caret = 0;
	search_match_start = -1;
	search_match_end = -1;
	dragging_selection = false;
	selection_dragged = false;
	selection_drag_attempt = false;
	set_process_internal(false);
	mark_layout_dirty();
}

void RSTDocumentCanvas::clear_document() {
	rst_text = String();
	plain_text = String();
	items.clear();
	lines.clear();
	rendered_block_count = 0;
	selection_anchor = 0;
	selection_caret = 0;
	search_match_start = -1;
	search_match_end = -1;
	dragging_selection = false;
	selection_dragged = false;
	selection_drag_attempt = false;
	set_process_internal(false);
	content_height = 1.0f;
	layout_dirty = false;
	anchor_offsets.clear();
	set_custom_minimum_size(Vector2(1.0, content_height));
	update_minimum_size();
	queue_redraw();
}

void RSTDocumentCanvas::mark_layout_dirty() {
	layout_dirty = true;
	queue_redraw();
}

bool RSTDocumentCanvas::has_selection() const {
	return selection_anchor != selection_caret;
}

bool RSTDocumentCanvas::is_position_inside_selection(const Vector2& p_position) {
	if (!has_selection()) {
		return false;
	}

	rebuild_layout();
	const int64_t character = hit_test_document(p_position);
	const int64_t from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t to = std::max<int64_t>(selection_anchor, selection_caret);
	return character >= from && character <= to;
}

void RSTDocumentCanvas::select_all() {
	rebuild_layout();
	selection_anchor = 0;
	selection_caret = plain_text.length();
	queue_redraw();
}

void RSTDocumentCanvas::select_between_points(const Vector2& p_from, const Vector2& p_to) {
	rebuild_layout();
	selection_anchor = hit_test_document(p_from);
	selection_caret = hit_test_document(p_to);
	queue_redraw();
}

String RSTDocumentCanvas::get_link_uri_at_point(const Vector2& p_position) {
	rebuild_layout();
	return link_uri_at_position(p_position);
}

void RSTDocumentCanvas::clear_selection() {
	selection_anchor = selection_caret;
	queue_redraw();
}

void RSTDocumentCanvas::_generate_context_menu() {
	menu = memnew(PopupMenu);
	add_child(menu, false, Node::INTERNAL_MODE_FRONT);
	menu->connect("id_pressed", callable_mp(this, &RSTDocumentCanvas::menu_option));
	menu->add_item("Copy", RestructuredtextLabel::MENU_COPY);
	menu->add_item("Select All", RestructuredtextLabel::MENU_SELECT_ALL);
}

void RSTDocumentCanvas::_update_context_menu() {
	if (menu == nullptr) {
		_generate_context_menu();
	}

	const int32_t copy_index = menu->get_item_index(RestructuredtextLabel::MENU_COPY);
	if (copy_index >= 0) {
		menu->set_item_accelerator(copy_index, document_view != nullptr && document_view->is_shortcut_keys_enabled() ? _get_menu_action_accelerator("ui_copy") : KEY_NONE);
		menu->set_item_disabled(copy_index, document_view == nullptr || !document_view->is_selection_enabled());
	}

	const int32_t select_all_index = menu->get_item_index(RestructuredtextLabel::MENU_SELECT_ALL);
	if (select_all_index >= 0) {
		menu->set_item_accelerator(select_all_index, document_view != nullptr && document_view->is_shortcut_keys_enabled() ? _get_menu_action_accelerator("ui_text_select_all") : KEY_NONE);
		menu->set_item_disabled(select_all_index, document_view == nullptr || !document_view->is_selection_enabled());
	}
}

Key RSTDocumentCanvas::_get_menu_action_accelerator(const String& p_action) {
	InputMap* input_map = InputMap::get_singleton();
	if (input_map == nullptr) {
		return KEY_NONE;
	}

	const TypedArray<Ref<InputEvent>> events = input_map->action_get_events(p_action);
	if (events.is_empty()) {
		return KEY_NONE;
	}

	const Ref<InputEventKey> key_event = events[0];
	if (key_event.is_null()) {
		return KEY_NONE;
	}

	if (key_event->get_physical_keycode() != KEY_NONE) {
		return key_event->get_physical_keycode_with_modifiers();
	}
	return key_event->get_keycode_with_modifiers();
}

PopupMenu* RSTDocumentCanvas::get_menu() {
	if (menu == nullptr) {
		_generate_context_menu();
	}
	return menu;
}

bool RSTDocumentCanvas::is_menu_visible() const {
	return menu != nullptr && menu->is_visible();
}

void RSTDocumentCanvas::menu_option(int p_option) {
	switch (p_option) {
		case RestructuredtextLabel::MENU_COPY: {
			String text = get_selected_text();
			if (text.is_empty()) {
				text = plain_text;
			}
			DisplayServer* display_server = DisplayServer::get_singleton();
			if (display_server != nullptr && !text.is_empty()) {
				display_server->clipboard_set(text);
			}
		} break;
		case RestructuredtextLabel::MENU_SELECT_ALL: {
			select_all();
		} break;
	}
}

Vector2i RSTDocumentCanvas::search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column) {
	rebuild_layout();

	const String needle = p_text;
	if (needle.is_empty() || plain_text.is_empty()) {
		search_match_start = -1;
		search_match_end = -1;
		queue_redraw();
		return Vector2i(-1, -1);
	}

	const bool match_case = (p_flags & 1) != 0;
	const bool whole_words = (p_flags & 2) != 0;
	const bool backwards = (p_flags & 4) != 0;
	const String haystack = match_case ? plain_text : plain_text.to_lower();
	const String search_needle = match_case ? needle : needle.to_lower();
	const int64_t needle_length = search_needle.length();
	int64_t start_offset = line_column_to_offset(p_from_line, p_from_column);

	if (backwards) {
		int64_t index = std::min<int64_t>(start_offset, std::max<int64_t>(0, haystack.length() - needle_length));
		while (index >= 0) {
			if (haystack.substr(static_cast<int32_t>(index), static_cast<int32_t>(needle_length)) == search_needle && (!whole_words || is_whole_word_match(plain_text, index, needle_length))) {
				search_match_start = index;
				search_match_end = index + needle_length;
				queue_redraw();
				return offset_to_line_column(index);
			}
			index--;
		}
	}
	else {
		int64_t index = std::max<int64_t>(0, std::min<int64_t>(start_offset, haystack.length()));
		while (index <= haystack.length() - needle_length) {
			const int64_t found = haystack.find(search_needle, static_cast<int32_t>(index));
			if (found < 0) {
				break;
			}
			if (!whole_words || is_whole_word_match(plain_text, found, needle_length)) {
				search_match_start = found;
				search_match_end = found + needle_length;
				queue_redraw();
				return offset_to_line_column(found);
			}
			index = found + 1;
		}
	}

	search_match_start = -1;
	search_match_end = -1;
	queue_redraw();
	return Vector2i(-1, -1);
}

String RSTDocumentCanvas::get_selected_text() const {
	const int64_t from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t to = std::max<int64_t>(selection_anchor, selection_caret);
	if (from >= to || from < 0 || to > plain_text.length()) {
		return String();
	}

	return plain_text.substr(static_cast<int32_t>(from), static_cast<int32_t>(to - from));
}

int32_t RSTDocumentCanvas::get_rendered_block_count() {
	rebuild_layout();
	return rendered_block_count;
}

float RSTDocumentCanvas::get_content_height() {
	rebuild_layout();
	return content_height;
}

int32_t RSTDocumentCanvas::get_table_cell_count() {
	rebuild_layout();
	int32_t cell_count = 0;
	for (const RSTCanvasItem& item : items) {
		if (item.type == RST_CANVAS_ITEM_TABLE) {
			cell_count += static_cast<int32_t>(item.cells.size());
		}
	}

	return cell_count;
}

Rect2 RSTDocumentCanvas::get_table_cell_rect(int32_t p_index) {
	rebuild_layout();
	int32_t cell_index = 0;
	for (const RSTCanvasItem& item : items) {
		if (item.type != RST_CANVAS_ITEM_TABLE) {
			continue;
		}
		for (const RSTTableCell& cell : item.cells) {
			if (cell_index == p_index) {
				return cell.rect;
			}
			cell_index++;
		}
	}

	return Rect2();
}

bool RSTDocumentCanvas::has_anchor(const String& p_anchor) const {
	return anchor_offsets.has(p_anchor);
}

PackedStringArray RSTDocumentCanvas::get_anchors() const {
	PackedStringArray result;
	const Array keys = anchor_offsets.keys();
	for (int32_t i = 0; i < keys.size(); i++) {
		result.append(keys[i]);
	}
	return result;
}

float RSTDocumentCanvas::get_anchor_offset(const String& p_anchor) const {
	if (!anchor_offsets.has(p_anchor)) {
		return -1.0f;
	}
	return anchor_offsets[p_anchor];
}

float RSTDocumentCanvas::get_search_result_y() const {
	if (search_match_start < 0 || search_match_end <= search_match_start) {
		return -1.0f;
	}

	for (const RSTCanvasItem& item : items) {
		if (item.type == RST_CANVAS_ITEM_TABLE) {
			for (const RSTTableCell& cell : item.cells) {
				if (search_match_start >= cell.global_start && search_match_start < cell.global_end) {
					return cell.rect.position.y;
				}
			}
			continue;
		}

		if (search_match_start >= item.header_global_start && search_match_start < item.header_global_end) {
			return item.rect.position.y;
		}

		if (search_match_start >= item.paragraph_global_start && search_match_start < item.paragraph_global_end) {
			return item.rect.position.y;
		}
	}

	return -1.0f;
}

Vector2 RSTDocumentCanvas::_get_minimum_size() const {
	return Vector2(1.0, content_height);
}

static String canvas_list_text(const RSTBlock& p_block, bool p_ordered, const RSTFootnoteMap* p_footnotes = nullptr) {
	String text;
	for (int32_t i = 0; i < p_block.items.size(); i++) {
		if (!text.is_empty()) {
			text += "\n";
		}

		if (p_ordered) {
			text += String::num_int64(i + 1) + ". ";
		}
		else {
			text += "- ";
		}
		text += render_inline_plain(p_block.items[i], p_footnotes);
	}

	return text;
}

static String canvas_table_text(const RSTBlock& p_block, const RSTFootnoteMap* p_footnotes = nullptr) {
	String text;
	for (int32_t row_index = 0; row_index < static_cast<int32_t>(p_block.rows.size()); row_index++) {
		if (!text.is_empty()) {
			text += "\n";
		}

		const PackedStringArray row = p_block.rows[row_index];
		for (int32_t column = 0; column < row.size(); column++) {
			if (column > 0) {
				text += " | ";
			}
			text += render_inline_plain(row[column], p_footnotes);
		}
	}

	return text;
}

static String canvas_toctree_text(const RSTBlock& p_block) {
	String text = p_block.argument;
	for (int32_t i = 0; i < p_block.items.size(); i++) {
		if (!text.is_empty()) {
			text += "\n";
		}
		text += p_block.items[i];
	}

	return text;
}

	static String canvas_block_theme_prefix(const RSTBlock& p_block) {
	switch (p_block.type) {
	case RST_BLOCK_HEADING:
		return String("heading_") + String::num_int64(std::min<int32_t>(std::max<int32_t>(p_block.level, 1), 5));
	case RST_BLOCK_BULLET_LIST:
	case RST_BLOCK_ENUMERATED_LIST:
			return "list";
	case RST_BLOCK_CODE:
			return "code_block";
	case RST_BLOCK_ADMONITION:
			return "admonition";
	case RST_BLOCK_TABLE:
			return "table";
	default:
			return "text";
	}
}

static float parse_dimension(const String& p_value, float p_reference, float p_fallback) {
	const String value = p_value.strip_edges();
	if (value.is_empty()) {
		return p_fallback;
	}
	if (value.ends_with("%")) {
		const float percent = value.substr(0, value.length() - 1).to_float();
		if (percent > 0.0f) {
			return p_reference * percent / 100.0f;
		}
	}

	const float pixels = value.to_float();
	return pixels > 0.0f ? pixels : p_fallback;
}

static Ref<Font> first_valid_font(const Ref<Font>& p_primary, const Ref<Font>& p_secondary, const Ref<Font>& p_fallback) {
	if (p_primary.is_valid()) {
		return p_primary;
	}
	if (p_secondary.is_valid()) {
		return p_secondary;
	}

	return p_fallback;
}

static Ref<Font> get_span_font(RestructuredtextLabel* p_owner, const RSTInlineSpan& p_span, const Ref<Font>& p_normal_font, const Ref<Font>& p_mono_font) {
	if (p_span.code) {
		return p_mono_font.is_valid() ? p_mono_font : p_normal_font;
	}
	if (p_span.bold && p_span.italic) {
		return first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "bold_italics_font"), get_theme_font_or(p_owner, "RestructuredtextLabel", "bold_italic_font"), p_normal_font);
	}
	if (p_span.bold) {
		return first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "bold_font"), Ref<Font>(), p_normal_font);
	}
	if (p_span.italic) {
		return first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "italics_font"), get_theme_font_or(p_owner, "RestructuredtextLabel", "italic_font"), p_normal_font);
	}

	return p_normal_font;
}

static void add_spans_to_paragraph(const Ref<TextParagraph>& p_paragraph, const std::vector<RSTInlineSpan>& p_spans, RestructuredtextLabel* p_owner, const Ref<Font>& p_normal_font, const Ref<Font>& p_mono_font, int32_t p_font_size, int32_t p_code_font_size) {
	Ref<StyleBox> inline_code_stylebox = get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "inline_code_style");
	if (!inline_code_stylebox.is_valid()) {
		inline_code_stylebox = create_internal_stylebox(Color(0.16, 0.18, 0.20, 1.0), Color(0, 0, 0, 0), 0, 0, 0, 0, 2.0f, 1.0f, 2.0f, 1.0f);
	}

	int32_t object_index = 0;
	for (const RSTInlineSpan& span : p_spans) {
		const Ref<Font> font = get_span_font(p_owner, span, p_normal_font, p_mono_font);
		const int32_t font_size = span.code ? p_code_font_size : p_font_size;
		if (span.code) {
			const float margin_left = get_stylebox_margin_or(inline_code_stylebox, SIDE_LEFT, 2.0f);
			if (margin_left > 0.0f) {
				p_paragraph->add_object(String("code_margin_left_") + String::num_int64(object_index++), Vector2(margin_left, static_cast<float>(std::max<int32_t>(1, font_size))), INLINE_ALIGNMENT_CENTER, 0);
			}
		}
		p_paragraph->add_string(span.text, font, std::max<int32_t>(1, font_size));
		if (span.code) {
			const float margin_right = get_stylebox_margin_or(inline_code_stylebox, SIDE_RIGHT, 2.0f);
			if (margin_right > 0.0f) {
				p_paragraph->add_object(String("code_margin_right_") + String::num_int64(object_index++), Vector2(margin_right, static_cast<float>(std::max<int32_t>(1, font_size))), INLINE_ALIGNMENT_CENTER, 0);
			}
		}
	}
}

static Vector2 get_inline_code_vertical_padding(RestructuredtextLabel* p_owner, const std::vector<RSTInlineSpan>& p_spans) {
	bool has_code_span = false;
	for (const RSTInlineSpan& span : p_spans) {
		if (span.code) {
			has_code_span = true;
			break;
		}
	}

	if (!has_code_span) {
		return Vector2();
	}

	Ref<StyleBox> inline_code_stylebox = get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "inline_code_style");
	if (!inline_code_stylebox.is_valid()) {
		inline_code_stylebox = create_internal_stylebox(Color(0.16, 0.18, 0.20, 1.0), Color(0, 0, 0, 0), 0, 0, 0, 0, 2.0f, 1.0f, 2.0f, 1.0f);
	}

	const float margin_top = get_stylebox_margin_or(inline_code_stylebox, SIDE_TOP, 1.0f);
	const float margin_bottom = get_stylebox_margin_or(inline_code_stylebox, SIDE_BOTTOM, 1.0f);
	return Vector2(std::max<float>(0.0f, margin_top - 1.0f), std::max<float>(0.0f, margin_bottom - 1.0f));
}

void RSTDocumentCanvas::rebuild_layout() {
	const float width = std::max<float>(1.0f, get_size().x);
	if (!layout_dirty && std::abs(width - layout_width) < 0.01f) {
		return;
	}

	layout_dirty = false;
	layout_width = width;
	items.clear();
	lines.clear();
	plain_text = String();
	rendered_block_count = 0;

	const int32_t extra = document_view == nullptr ? 0 : document_view->get_extra_font_size();
	// Base font sizes resolved from theme (without extra), used as fallbacks.
	const int32_t body_base = get_theme_font_size_or(document_view, "RestructuredtextLabel", "normal_font_size", 16);
	const int32_t inline_base = get_theme_font_size_or(document_view, "RestructuredtextLabel", "inline_code_font_size", 16);
	const int32_t code_base = get_theme_font_size_or(document_view, "RestructuredtextLabel", "code_block_font_size", 16);
	// Final font sizes with extra applied.
	const int32_t body_size = std::max<int32_t>(1, body_base + extra);
	const int32_t inline_code_size = std::max<int32_t>(1, inline_base + extra);
	const int32_t code_block_size = std::max<int32_t>(1, code_base + extra);
	const int32_t line_separation = get_theme_constant_or(document_view, "RestructuredtextLabel", "line_separation", 2);
	const int32_t paragraph_separation = get_theme_constant_or(document_view, "RestructuredtextLabel", "paragraph_separation", 10);
	const Ref<Font> body_font = get_theme_font_or(document_view, "RestructuredtextLabel", "normal_font");
	const Ref<Font> inline_mono_font = get_theme_font_or(document_view, "RestructuredtextLabel", "inline_code_font").is_valid() ? get_theme_font_or(document_view, "RestructuredtextLabel", "inline_code_font") : body_font;
	const Ref<Font> code_block_font = get_theme_font_or(document_view, "RestructuredtextLabel", "code_block_font").is_valid() ? get_theme_font_or(document_view, "RestructuredtextLabel", "code_block_font") : inline_mono_font;
	const Color body_color = get_theme_color_or(document_view, "RestructuredtextLabel", "font_color", Color(1, 1, 1, 1));
	const int32_t list_indent = std::max<int32_t>(0, get_theme_constant_or(document_view, "RestructuredtextLabel", "list_indent", 26));
	const int32_t list_marker_gap = std::max<int32_t>(0, get_theme_constant_or(document_view, "RestructuredtextLabel", "list_marker_gap", 6));
	const int32_t list_marker_min_width = std::max<int32_t>(0, get_theme_constant_or(document_view, "RestructuredtextLabel", "list_marker_min_width", 14));
	const Ref<Font> list_marker_font = get_theme_font_or(document_view, "RestructuredtextLabel", "list_marker_font").is_valid() ? get_theme_font_or(document_view, "RestructuredtextLabel", "list_marker_font") : body_font;
	const int32_t list_marker_font_size = std::max<int32_t>(1, get_theme_font_size_or(document_view, "RestructuredtextLabel", "list_marker_font_size", 16) + extra);
	const Color list_marker_color = get_theme_color_or(document_view, "RestructuredtextLabel", "list_marker_color", body_color);

	float y = 0.0f;
	const RSTFootnoteMap footnotes = build_footnote_map(split_lines(rst_text));
	const std::vector<RSTBlock> blocks = parse_rst_blocks(rst_text, document_view, &footnotes);
	for (const RSTBlock& block : blocks) {
		RSTCanvasItem item;
		String item_text;
		std::vector<RSTInlineSpan> item_spans;
		std::vector<String> list_marker_texts;
		std::vector<int64_t> list_item_starts;
		Ref<Font> font = body_font;
		int32_t font_size = body_size;
		float left_padding = 0.0f;
		float right_padding = 0.0f;
		float top_padding = 0.0f;
		float bottom_padding = 0.0f;
		const String block_theme_prefix = canvas_block_theme_prefix(block);
		const int32_t block_spacing_before = std::max<int32_t>(0, block.type == RST_BLOCK_ADMONITION ?
			get_admonition_theme_constant_or(document_view, block.directive, "space_before", 0) :
			get_theme_constant_or(document_view, "RestructuredtextLabel", StringName(block_theme_prefix + String("_space_before")), 0));
		const int32_t block_spacing_after = std::max<int32_t>(0, block.type == RST_BLOCK_ADMONITION ?
			get_admonition_theme_constant_or(document_view, block.directive, "space_after", paragraph_separation) :
			get_theme_constant_or(document_view, "RestructuredtextLabel", StringName(block_theme_prefix + String("_space_after")), paragraph_separation));

		item.text_color = body_color;
		item.background_color = Color(0, 0, 0, 0);
		item.border_color = Color(0, 0, 0, 0);

		switch (block.type) {
		case RST_BLOCK_HEADING: {
			const int32_t heading_level = std::min<int32_t>(std::max<int32_t>(block.level, 1), 5);
			const String heading_prefix = canvas_block_theme_prefix(block) + "_";
			const int32_t fallback_sizes[] = { 28, 24, 21, 19, 17 };
			const int32_t index = heading_level - 1;
			font_size = std::max<int32_t>(1, get_theme_font_size_or(document_view, "RestructuredtextLabel", StringName(heading_prefix + String("font_size")), fallback_sizes[index]) + extra);
			font = get_theme_font_or(document_view, "RestructuredtextLabel", StringName(heading_prefix + String("font")));
			item.text_color = get_theme_color_or(document_view, "RestructuredtextLabel", StringName(heading_prefix + String("font_color")), body_color);
			item.heading = true;
			item_text = parse_inline_spans(block.text, document_view, item_spans, &footnotes);
		} break;
		case RST_BLOCK_BULLET_LIST: {
			for (int32_t i = 0; i < block.items.size(); i++) {
				if (!item_text.is_empty()) {
					append_inline_span(item_spans, item_text, "\n");
				}
				list_marker_texts.push_back(String::chr(0x2022));
				list_item_starts.push_back(item_text.length());
				append_parsed_inline_spans(item_spans, item_text, block.items[i], document_view, &footnotes);
			}
			left_padding = static_cast<float>(std::max<int32_t>(list_indent, list_marker_min_width + list_marker_gap));
		} break;
		case RST_BLOCK_ENUMERATED_LIST: {
			for (int32_t i = 0; i < block.items.size(); i++) {
				if (!item_text.is_empty()) {
					append_inline_span(item_spans, item_text, "\n");
				}
				list_marker_texts.push_back(String::num_int64(i + 1) + ".");
				list_item_starts.push_back(item_text.length());
				append_parsed_inline_spans(item_spans, item_text, block.items[i], document_view, &footnotes);
			}
			left_padding = static_cast<float>(std::max<int32_t>(list_indent, list_marker_min_width + list_marker_gap));
		} break;
		case RST_BLOCK_CODE:
			append_inline_span(item_spans, item_text, block.text);
			font = code_block_font;
			font_size = code_block_size;
			item.text_color = get_theme_color_or(document_view, "RestructuredtextLabel", "code_block_font_color", Color(0.86, 0.88, 0.91, 1.0));
			item.background_color = Color(0.08, 0.09, 0.11, 1.0);
			item.panel_stylebox = get_theme_stylebox_or(document_view, "RestructuredtextLabel", "code_block_panel");
			if (!item.panel_stylebox.is_valid()) {
				item.panel_stylebox = create_internal_stylebox(item.background_color, Color(0, 0, 0, 0), 0, 0, 0, 0, 8.0f, 8.0f, 8.0f, 8.0f);
			}
			item.framed = true;
			item.code = true;
			left_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_LEFT, 8.0f);
			top_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_TOP, 8.0f);
			right_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_RIGHT, 8.0f);
			bottom_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_BOTTOM, 8.0f);
			break;
		case RST_BLOCK_ADMONITION: {
			font_size = std::max<int32_t>(1, get_admonition_theme_font_size_or(document_view, block.directive, "font_size", body_base) + extra);
			item.text_color = get_admonition_theme_color_or(document_view, block.directive, "font_color", body_color);
			const Color header_text_color = get_admonition_theme_color_or(document_view, block.directive, "header_font_color", Color(1, 1, 1, 1));
			item.header_text_color = header_text_color;
			const Color accent = get_admonition_theme_color_or(document_view, block.directive, "accent_color", default_admonition_accent(block.directive));
			item.border_color = accent;
			item.header_color = accent;
			item.background_color = Color(0.10, 0.12, 0.14, 1.0);
			item.panel_stylebox = get_admonition_theme_stylebox_or(document_view, block.directive, "panel");
			item.header_stylebox = get_admonition_theme_stylebox_or(document_view, block.directive, "header_panel");
			item.framed = true;
			item.border_width = 4;
			const float internal_body_margin = 8.0f;
			const float internal_body_margin_left = 12.0f;
			const float internal_header_margin = 6.0f;
			if (!item.panel_stylebox.is_valid()) {
				item.panel_stylebox = create_internal_stylebox(item.background_color, item.border_color, item.border_width, 1, 1, 1, internal_body_margin_left, internal_body_margin, internal_body_margin, internal_body_margin);
			}
			if (!item.header_stylebox.is_valid()) {
				item.header_stylebox = create_internal_stylebox(item.header_color, item.border_color, item.border_width, 1, 1, 0, internal_body_margin_left, internal_header_margin, internal_body_margin, internal_header_margin);
			}

			if (!plain_text.is_empty()) {
				plain_text += "\n\n";
			}
			item.global_start = plain_text.length();
			item.header_global_start = item.global_start;
			append_inline_span(item.header_spans, item.header_text, block.argument, true, false, false, String(), true, header_text_color);
			plain_text += item.header_text;
			item.header_global_end = plain_text.length();

			if (!block.text.is_empty()) {
				plain_text += "\n";
				item.paragraph_global_start = plain_text.length();
				item_text = parse_inline_spans(block.text, document_view, item_spans, &footnotes);
				plain_text += item_text;
				item.paragraph_global_end = plain_text.length();
				item.text = item_text;
				item.spans = item_spans;
			}
			else {
				item.paragraph_global_start = plain_text.length();
				item.paragraph_global_end = item.paragraph_global_start;
			}
			item.global_end = plain_text.length();

			if (y > 0.0f) {
				y += static_cast<float>(block_spacing_before);
			}

			item.header_paragraph.instantiate();
			configure_paragraph_autowrap(item.header_paragraph, document_view);
			const float header_left = get_stylebox_margin_or(item.header_stylebox, SIDE_LEFT, internal_body_margin_left);
			const float header_top = get_stylebox_margin_or(item.header_stylebox, SIDE_TOP, internal_header_margin);
			const float header_right = get_stylebox_margin_or(item.header_stylebox, SIDE_RIGHT, internal_body_margin);
			const float header_margin_top = get_stylebox_margin_or(item.header_stylebox, SIDE_TOP, internal_header_margin);
			const float header_margin_bottom = get_stylebox_margin_or(item.header_stylebox, SIDE_BOTTOM, internal_header_margin);
			item.header_paragraph->set_width(std::max<float>(1.0f, width - header_left - header_right));
			item.header_paragraph->set_line_spacing(line_separation);
			add_spans_to_paragraph(item.header_paragraph, item.header_spans, document_view, body_font, inline_mono_font, font_size, inline_code_size);
			const Vector2 header_size = item.header_paragraph->get_size();
			item.header_height = static_cast<int32_t>(std::ceil(header_size.y + header_margin_top + header_margin_bottom));
			item.header_text_rect = Rect2(header_left, y + header_top, std::max<float>(1.0f, width - header_left - header_right), header_size.y);

			const float body_left = get_stylebox_margin_or(item.panel_stylebox, SIDE_LEFT, internal_body_margin_left);
			const float body_top = get_stylebox_margin_or(item.panel_stylebox, SIDE_TOP, internal_body_margin);
			const float body_right = get_stylebox_margin_or(item.panel_stylebox, SIDE_RIGHT, internal_body_margin);
			const float body_bottom = get_stylebox_margin_or(item.panel_stylebox, SIDE_BOTTOM, internal_body_margin);
			Vector2 body_size = Vector2();
			if (!item.text.is_empty()) {
				item.paragraph.instantiate();
				configure_paragraph_autowrap(item.paragraph, document_view);
				item.paragraph->set_width(std::max<float>(1.0f, width - body_left - body_right));
				item.paragraph->set_line_spacing(line_separation);
				add_spans_to_paragraph(item.paragraph, item.spans, document_view, body_font, inline_mono_font, font_size, inline_code_size);
				body_size = item.paragraph->get_size();
				item.text_rect = Rect2(body_left, y + static_cast<float>(item.header_height) + body_top, std::max<float>(1.0f, width - body_left - body_right), body_size.y);
			}

			item.rect = Rect2(0.0f, y, width, static_cast<float>(item.header_height) + (item.text.is_empty() ? 0.0f : body_top + body_size.y + body_bottom));

			const int32_t item_index = static_cast<int32_t>(items.size());
			for (int32_t line_index = 0; line_index < item.header_paragraph->get_line_count(); line_index++) {
				const Vector2i range = item.header_paragraph->get_line_range(line_index);
				RSTCanvasLine line;
				line.item_index = item_index;
				line.paragraph_line = line_index;
				line.global_start = item.header_global_start + range.x;
				line.global_end = item.header_global_start + range.y;
				line.position = item.header_text_rect.position;
				for (int32_t previous_line = 0; previous_line < line_index; previous_line++) {
					line.position.y += item.header_paragraph->get_line_size(previous_line).y + line_separation;
				}
				line.size = item.header_paragraph->get_line_size(line_index);
				lines.push_back(line);
			}
			if (item.paragraph.is_valid()) {
				for (int32_t line_index = 0; line_index < item.paragraph->get_line_count(); line_index++) {
					const Vector2i range = item.paragraph->get_line_range(line_index);
					RSTCanvasLine line;
					line.item_index = item_index;
					line.paragraph_line = line_index;
					line.global_start = item.paragraph_global_start + range.x;
					line.global_end = item.paragraph_global_start + range.y;
					line.position = item.text_rect.position;
					for (int32_t previous_line = 0; previous_line < line_index; previous_line++) {
						line.position.y += item.paragraph->get_line_size(previous_line).y + line_separation;
					}
					line.size = item.paragraph->get_line_size(line_index);
					lines.push_back(line);
				}
			}

			items.push_back(item);
			y += item.rect.size.y + static_cast<float>(block_spacing_after);
			if (!block.anchor.is_empty()) { item.anchor = block.anchor; anchor_offsets[block.anchor] = item.rect.position.y; }
			rendered_block_count++;
			continue;
		} break;
		case RST_BLOCK_TABLE: {
			const int32_t columns = std::max<int32_t>(1, block.columns);
			Ref<StyleBox> table_panel = get_theme_stylebox_or(document_view, "RestructuredtextLabel", "table_panel");
			if (!table_panel.is_valid()) {
				table_panel = create_internal_stylebox(Color(0, 0, 0, 0), Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
			}
			Ref<StyleBox> header_panel = get_theme_stylebox_or(document_view, "RestructuredtextLabel", "table_header_panel");
			if (!header_panel.is_valid()) {
				header_panel = create_internal_stylebox(Color(0.18, 0.19, 0.22, 1.0), Color(0.28, 0.30, 0.34, 1.0), 1, 1, 1, 1, 6.0f, 6.0f, 6.0f, 6.0f);
			}
			Ref<StyleBox> cell_panel = get_theme_stylebox_or(document_view, "RestructuredtextLabel", "table_cell_panel");
			if (!cell_panel.is_valid()) {
				cell_panel = create_internal_stylebox(Color(0, 0, 0, 0), Color(0.28, 0.30, 0.34, 1.0), 1, 1, 1, 1, 6.0f, 6.0f, 6.0f, 6.0f);
			}
			const bool striped_table = get_theme_constant_or(document_view, "RestructuredtextLabel", "table_striped", 0) != 0;
			Ref<StyleBox> striped_cell_panel = get_theme_stylebox_or(document_view, "RestructuredtextLabel", "table_striped_panel");
			if (striped_table && !striped_cell_panel.is_valid()) {
				striped_cell_panel = create_internal_stylebox(Color(0.10, 0.11, 0.13, 1.0), Color(0.28, 0.30, 0.34, 1.0), 1, 1, 1, 1, 6.0f, 6.0f, 6.0f, 6.0f);
			}

			const float table_left = get_stylebox_margin_or(table_panel, SIDE_LEFT, 0.0f);
			const float table_top = get_stylebox_margin_or(table_panel, SIDE_TOP, 0.0f);
			const float table_right = get_stylebox_margin_or(table_panel, SIDE_RIGHT, 0.0f);
			const float table_bottom = get_stylebox_margin_or(table_panel, SIDE_BOTTOM, 0.0f);
			const float table_content_width = std::max<float>(1.0f, width - table_left - table_right);
			const float column_width = std::max<float>(1.0f, table_content_width / static_cast<float>(columns));

			const Ref<Font> table_cell_font = get_theme_font_or(document_view, "RestructuredtextLabel", "table_font");
			const Ref<Font> table_header_font = get_theme_font_or(document_view, "RestructuredtextLabel", "table_header_font");
			const int32_t cell_base = get_theme_font_size_or(document_view, "RestructuredtextLabel", "table_font_size", 16);
			const int32_t table_cell_font_size = std::max<int32_t>(1, cell_base + extra);
			const int32_t table_header_font_size = std::max<int32_t>(1, get_theme_font_size_or(document_view, "RestructuredtextLabel", "table_header_font_size", cell_base) + extra);
			const Color table_cell_font_color = get_theme_color_or(document_view, "RestructuredtextLabel", "table_font_color", body_color);
			const Color table_header_font_color = get_theme_color_or(document_view, "RestructuredtextLabel", "table_header_font_color", table_cell_font_color);

			if (!plain_text.is_empty()) {
				plain_text += "\n\n";
			}
			item.type = RST_CANVAS_ITEM_TABLE;
			item.global_start = plain_text.length();
			item.text_color = body_color;
			item.panel_stylebox = table_panel;
			item.background_color = Color(0, 0, 0, 0);
			item.framed = true;

			std::vector<float> row_heights;
			row_heights.reserve(block.rows.size());
			for (int32_t row_index = 0; row_index < static_cast<int32_t>(block.rows.size()); row_index++) {
				if (row_index > 0) {
					plain_text += "\n";
				}

				float row_height = 1.0f;
				const PackedStringArray row = block.rows[row_index];
				for (int32_t column = 0; column < columns; column++) {
					if (column > 0) {
						plain_text += " | ";
					}

					RSTTableCell cell;
					cell.header = row_index < block.header_rows;
					const int32_t body_row_index = row_index - block.header_rows;
					const bool striped_row = striped_table && !cell.header && (body_row_index % 2) == 1;
					cell.stylebox = cell.header ? header_panel : (striped_row ? striped_cell_panel : cell_panel);
					cell.text_color = cell.header ? table_header_font_color : table_cell_font_color;
					cell.global_start = plain_text.length();
					const String cell_source = column < row.size() ? row[column] : String();
					cell.text = parse_inline_spans(cell_source, document_view, cell.spans, &footnotes);
					const Vector2 inline_code_padding = get_inline_code_vertical_padding(document_view, cell.spans);
					cell.inline_code_padding_top = inline_code_padding.x;
					cell.inline_code_padding_bottom = inline_code_padding.y;
					plain_text += cell.text;
					cell.global_end = plain_text.length();
					cell.paragraph.instantiate();
					configure_paragraph_autowrap(cell.paragraph, document_view);
					const float cell_left = get_stylebox_margin_or(cell.stylebox, SIDE_LEFT, 6.0f);
					const float cell_top = get_stylebox_margin_or(cell.stylebox, SIDE_TOP, 6.0f);
					const float cell_right = get_stylebox_margin_or(cell.stylebox, SIDE_RIGHT, 6.0f);
					const float cell_bottom = get_stylebox_margin_or(cell.stylebox, SIDE_BOTTOM, 6.0f);
					cell.paragraph->set_width(std::max<float>(1.0f, column_width - cell_left - cell_right));
					cell.paragraph->set_line_spacing(line_separation);
					add_spans_to_paragraph(cell.paragraph, cell.spans, document_view, cell.header ? table_header_font : table_cell_font, inline_mono_font, cell.header ? table_header_font_size : table_cell_font_size, inline_code_size);
					row_height = std::max<float>(row_height, cell.paragraph->get_size().y + cell_top + cell_bottom + cell.inline_code_padding_top + cell.inline_code_padding_bottom);
					item.cells.push_back(cell);
				}
				row_heights.push_back(row_height);
			}

			item.global_end = plain_text.length();
			item.text = plain_text.substr(static_cast<int32_t>(item.global_start), static_cast<int32_t>(item.global_end - item.global_start));

			if (y > 0.0f) {
				y += static_cast<float>(block_spacing_before);
			}

			float table_y = y + table_top;
			int32_t cell_index = 0;
			for (int32_t row_index = 0; row_index < static_cast<int32_t>(row_heights.size()); row_index++) {
				float x = table_left;
				for (int32_t column = 0; column < columns; column++) {
					RSTTableCell& cell = item.cells[cell_index];
					const float cell_left = get_stylebox_margin_or(cell.stylebox, SIDE_LEFT, 6.0f);
					const float cell_top = get_stylebox_margin_or(cell.stylebox, SIDE_TOP, 6.0f);
					const float cell_right = get_stylebox_margin_or(cell.stylebox, SIDE_RIGHT, 6.0f);
					const float cell_bottom = get_stylebox_margin_or(cell.stylebox, SIDE_BOTTOM, 6.0f);
					cell.rect = Rect2(x, table_y, column_width, row_heights[row_index]);
					cell.text_rect = Rect2(x + cell_left, table_y + cell_top + cell.inline_code_padding_top, std::max<float>(1.0f, column_width - cell_left - cell_right), std::max<float>(1.0f, row_heights[row_index] - cell_top - cell_bottom - cell.inline_code_padding_top - cell.inline_code_padding_bottom));
					x += column_width;
					cell_index++;
				}
				table_y += row_heights[row_index];
			}

			item.rect = Rect2(0.0f, y, width, std::max<float>(1.0f, table_y - y + table_bottom));
			items.push_back(item);
			y += item.rect.size.y + static_cast<float>(block_spacing_after);
			if (!block.anchor.is_empty()) { item.anchor = block.anchor; anchor_offsets[block.anchor] = item.rect.position.y; }
			rendered_block_count++;
			continue;
		} break;
		case RST_BLOCK_IMAGE: {
			const String image_path = document_view == nullptr ? block.argument : document_view->resolve_document_path(block.argument);
			item.texture = dockit::load_html_image_file(image_path);
			if (item.texture.is_valid()) {
				const Vector2 texture_size = item.texture->get_size();
				float image_width = parse_dimension(block.image_width, width, texture_size.x);
				float image_height = parse_dimension(block.image_height, texture_size.y, texture_size.y);
				if (block.image_height.is_empty() && texture_size.x > 0.0f) {
					image_height = texture_size.y * image_width / texture_size.x;
				}
				if (!block.image_scale.is_empty()) {
					const float image_scale = parse_scale_percent(block.image_scale) / 100.0f;
					image_width *= image_scale;
					image_height *= image_scale;
				}
				if (image_width > width) {
					const float scale = width / image_width;
					image_width = width;
					image_height *= scale;
				}

				float x = 0.0f;
				const String align = block.image_align.to_lower();
				if (align == "center") {
					x = std::max<float>(0.0f, (width - image_width) * 0.5f);
				}
				else if (align == "right") {
					x = std::max<float>(0.0f, width - image_width);
				}

				item.type = RST_CANVAS_ITEM_IMAGE;
				item.text = block.image_alt.is_empty() ? String("Image: ") + block.argument : block.image_alt;
				if (!plain_text.is_empty()) {
					plain_text += "\n\n";
				}
				item.global_start = plain_text.length();
				plain_text += item.text;
				item.global_end = plain_text.length();
				if (y > 0.0f) {
					y += static_cast<float>(block_spacing_before);
				}
				item.rect = Rect2(x, y, image_width, image_height);
				items.push_back(item);
				y += image_height + static_cast<float>(block_spacing_after);
				if (!block.anchor.is_empty()) { item.anchor = block.anchor; anchor_offsets[block.anchor] = item.rect.position.y; }
				rendered_block_count++;
				continue;
			}

			append_inline_span(item_spans, item_text, String("Image: ") + block.argument, false, true);
			if (!block.text.is_empty()) {
				append_inline_span(item_spans, item_text, "\n");
				append_parsed_inline_spans(item_spans, item_text, block.text, document_view, &footnotes);
			}
		} break;
		case RST_BLOCK_TOCTREE:
			append_inline_span(item_spans, item_text, block.argument, true);
			for (int32_t i = 0; i < block.items.size(); i++) {
				if (!item_text.is_empty()) {
					append_inline_span(item_spans, item_text, "\n");
				}
				const String target = block.items[i];
				const String uri = document_view == nullptr ? String("doc:") + target : document_view->resolve_link_uri("doc", target);
				append_inline_span(item_spans, item_text, target, false, false, false, uri);
			}
			break;
		case RST_BLOCK_TRANSITION:
			append_inline_span(item_spans, item_text, String("────────────────"));
			item.transition = true;
			item.text_color = get_theme_color_or(document_view, "RestructuredtextLabel", "font_separator_color", Color(0.45, 0.47, 0.52, 1.0));
			break;
		case RST_BLOCK_PARAGRAPH:
		default:
			item_text = parse_inline_spans(block.text, document_view, item_spans, &footnotes);
			break;
		}

		if (item_text.is_empty()) {
			continue;
		}

		if (!plain_text.is_empty()) {
			plain_text += "\n\n";
		}
		item.global_start = plain_text.length();
		plain_text += item_text;
		item.global_end = plain_text.length();
		item.paragraph_global_start = item.global_start;
		item.paragraph_global_end = item.global_end;
		item.text = item_text;
		item.spans = item_spans;

		item.paragraph.instantiate();
		configure_paragraph_autowrap(item.paragraph, document_view);
		item.paragraph->set_width(std::max<float>(1.0f, width - left_padding - right_padding));
		item.paragraph->set_line_spacing(line_separation);
		add_spans_to_paragraph(item.paragraph, item.spans, document_view, font, inline_mono_font, font_size, inline_code_size);

		const Vector2 paragraph_size = item.paragraph->get_size();
		if (y > 0.0f) {
			y += static_cast<float>(block_spacing_before);
		}
		item.text_rect = Rect2(left_padding, y + top_padding, std::max<float>(1.0f, width - left_padding - right_padding), paragraph_size.y);
		item.rect = Rect2(0.0f, y, width, paragraph_size.y + top_padding + bottom_padding);

		const int32_t item_index = static_cast<int32_t>(items.size());
		for (int32_t line_index = 0; line_index < item.paragraph->get_line_count(); line_index++) {
			const Vector2i range = item.paragraph->get_line_range(line_index);
			RSTCanvasLine line;
			line.item_index = item_index;
			line.paragraph_line = line_index;
			line.global_start = item.paragraph_global_start + range.x;
			line.global_end = item.paragraph_global_start + range.y;
			line.position = item.text_rect.position;
			for (int32_t previous_line = 0; previous_line < line_index; previous_line++) {
				line.position.y += item.paragraph->get_line_size(previous_line).y + line_separation;
			}
			line.size = item.paragraph->get_line_size(line_index);
			lines.push_back(line);
		}

		if (!list_marker_texts.empty() && list_marker_font.is_valid()) {
			for (int32_t marker_index = 0; marker_index < static_cast<int32_t>(list_marker_texts.size()); marker_index++) {
				const int64_t item_start = list_item_starts[marker_index];
				float line_y = item.text_rect.position.y;
				for (int32_t line_index = 0; line_index < item.paragraph->get_line_count(); line_index++) {
					const Vector2i range = item.paragraph->get_line_range(line_index);
					const bool starts_on_line = item_start >= range.x && item_start < range.y;
					if (starts_on_line) {
						const String marker_text = list_marker_texts[marker_index];
						const Vector2 marker_size = list_marker_font->get_string_size(marker_text, HORIZONTAL_ALIGNMENT_LEFT, -1, list_marker_font_size);
						RSTListMarker marker;
						marker.text = marker_text;
						marker.font = list_marker_font;
						marker.font_size = list_marker_font_size;
						marker.color = list_marker_color;
						marker.position = Vector2(std::max<float>(0.0f, item.text_rect.position.x - static_cast<float>(list_marker_gap) - marker_size.x), line_y + item.paragraph->get_line_ascent(line_index));
						item.list_markers.push_back(marker);
						break;
					}
					line_y += item.paragraph->get_line_size(line_index).y + line_separation;
				}
			}
		}

		items.push_back(item);
		y += item.rect.size.y + static_cast<float>(block_spacing_after);
		if (!block.anchor.is_empty()) { item.anchor = block.anchor; anchor_offsets[block.anchor] = item.rect.position.y; }
		rendered_block_count++;
	}

	content_height = std::max<float>(1.0f, y);
	set_custom_minimum_size(Vector2(1.0, content_height));
	update_minimum_size();

	const int64_t text_length = plain_text.length();
	selection_anchor = std::min<int64_t>(selection_anchor, text_length);
	selection_caret = std::min<int64_t>(selection_caret, text_length);
	if (search_match_start >= text_length || search_match_end > text_length || search_match_end <= search_match_start) {
		search_match_start = -1;
		search_match_end = -1;
	}
}

void RSTDocumentCanvas::draw_selection_for_item(const RSTCanvasItem& p_item) {
	if (p_item.type == RST_CANVAS_ITEM_TABLE) {
		for (const RSTTableCell& cell : p_item.cells) {
			draw_selection_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end);
		}
		return;
	}

	draw_selection_for_paragraph(p_item.header_paragraph, p_item.header_text_rect, p_item.header_global_start, p_item.header_global_end);
	draw_selection_for_paragraph(p_item.paragraph, p_item.text_rect, p_item.paragraph_global_start, p_item.paragraph_global_end);
}

void RSTDocumentCanvas::draw_search_highlight_for_item(const RSTCanvasItem& p_item) {
	if (search_match_start < 0 || search_match_end <= search_match_start) {
		return;
	}

	const Color search_result_color = get_theme_color_or(document_view, "RestructuredtextLabel", "search_result_color", Color(1.0, 0.72, 0.16, 0.42));
	if (p_item.type == RST_CANVAS_ITEM_TABLE) {
		for (const RSTTableCell& cell : p_item.cells) {
			draw_range_background_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end, search_match_start, search_match_end, search_result_color);
		}
		return;
	}

	draw_range_background_for_paragraph(p_item.header_paragraph, p_item.header_text_rect, p_item.header_global_start, p_item.header_global_end, search_match_start, search_match_end, search_result_color);
	draw_range_background_for_paragraph(p_item.paragraph, p_item.text_rect, p_item.paragraph_global_start, p_item.paragraph_global_end, search_match_start, search_match_end, search_result_color);
}

void RSTDocumentCanvas::draw_range_background_for_paragraph(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, int64_t p_global_start, int64_t p_global_end, int64_t p_range_start, int64_t p_range_end, const Color& p_color) {
	if (p_paragraph.is_null()) {
		return;
	}

	TextServerManager* text_server_manager = TextServerManager::get_singleton();
	if (text_server_manager == nullptr) {
		return;
	}
	const Ref<TextServer> text_server = text_server_manager->get_primary_interface();
	if (text_server.is_null()) {
		return;
	}

	if (p_range_end <= p_global_start || p_range_start >= p_global_end) {
		return;
	}

	float y = p_text_rect.position.y;
	for (int32_t line_index = 0; line_index < p_paragraph->get_line_count(); line_index++) {
		const Vector2i range = p_paragraph->get_line_range(line_index);
		const int64_t local_from = std::max<int64_t>(range.x, p_range_start - p_global_start);
		const int64_t local_to = std::min<int64_t>(range.y, p_range_end - p_global_start);
		const Vector2 line_size = p_paragraph->get_line_size(line_index);
		if (local_from < local_to) {
			const PackedVector2Array selected_ranges = text_server->shaped_text_get_selection(p_paragraph->get_line_rid(line_index), local_from, local_to);
			for (int32_t range_index = 0; range_index < selected_ranges.size(); range_index++) {
				const Vector2 selected_range = selected_ranges[range_index];
				draw_rect(Rect2(p_text_rect.position.x + selected_range.x, y, selected_range.y - selected_range.x, line_size.y), p_color, true);
			}
		}
		y += line_size.y + p_paragraph->get_line_spacing();
	}
}

void RSTDocumentCanvas::draw_selection_for_paragraph(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, int64_t p_global_start, int64_t p_global_end) {
	if (!has_selection()) {
		return;
	}

	const int64_t selection_from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t selection_to = std::max<int64_t>(selection_anchor, selection_caret);
	const Color selection_color = get_theme_color_or(document_view, "RestructuredtextLabel", "selection_color", Color(0.22, 0.45, 0.95, 0.45));
	draw_range_background_for_paragraph(p_paragraph, p_text_rect, p_global_start, p_global_end, selection_from, selection_to, selection_color);
}

void RSTDocumentCanvas::draw_list_markers(const RSTCanvasItem& p_item) {
	const RID canvas = get_canvas_item();
	for (const RSTListMarker& marker : p_item.list_markers) {
		if (marker.font.is_null() || marker.text.is_empty()) {
			continue;
		}
		marker.font->draw_string(canvas, marker.position, marker.text, HORIZONTAL_ALIGNMENT_LEFT, -1, marker.font_size, marker.color);
	}
}

void RSTDocumentCanvas::draw_stylebox_or_rect(const Ref<StyleBox>& p_stylebox, const Rect2& p_rect, const Color& p_fallback_color) {
	if (p_stylebox.is_valid()) {
		p_stylebox->draw(get_canvas_item(), p_rect);
		return;
	}

	if (p_fallback_color.a > 0.0f) {
		draw_rect(p_rect, p_fallback_color, true);
	}
}

void RSTDocumentCanvas::draw_span_decorations(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, const std::vector<RSTInlineSpan>& p_spans, int64_t p_global_start, bool p_draw_code_backgrounds, bool p_draw_link_underlines) {
	if (p_paragraph.is_null() || p_spans.empty()) {
		return;
	}

	TextServerManager* text_server_manager = TextServerManager::get_singleton();
	if (text_server_manager == nullptr) {
		return;
	}
	const Ref<TextServer> text_server = text_server_manager->get_primary_interface();
	if (text_server.is_null()) {
		return;
	}

	const Color code_background_color = Color(0.16, 0.18, 0.20, 1.0);
	Ref<StyleBox> code_stylebox;
	if (p_draw_code_backgrounds) {
		code_stylebox = get_theme_stylebox_or(document_view, "RestructuredtextLabel", "inline_code_style");
		if (!code_stylebox.is_valid()) {
			code_stylebox = create_internal_stylebox(code_background_color, Color(0, 0, 0, 0), 0, 0, 0, 0, 2.0f, 1.0f, 2.0f, 1.0f);
		}
	}
	const Color link_color = get_theme_color_or(document_view, "RestructuredtextLabel", "link_color", Color(0.36, 0.62, 1.0, 1.0));

	for (const RSTInlineSpan& span : p_spans) {
		if ((!p_draw_code_backgrounds || !span.code) && (!p_draw_link_underlines || span.link_uri.is_empty())) {
			continue;
		}

		float y = p_text_rect.position.y;
		for (int32_t line_index = 0; line_index < p_paragraph->get_line_count(); line_index++) {
			const Vector2i range = p_paragraph->get_line_range(line_index);
			const int64_t local_from = std::max<int64_t>(range.x, span.start);
			const int64_t local_to = std::min<int64_t>(range.y, span.end);
			const Vector2 line_size = p_paragraph->get_line_size(line_index);
			if (local_from < local_to) {
				const PackedVector2Array selected_ranges = text_server->shaped_text_get_selection(p_paragraph->get_line_rid(line_index), local_from, local_to);
				for (int32_t range_index = 0; range_index < selected_ranges.size(); range_index++) {
					const Vector2 selected_range = selected_ranges[range_index];
					if (p_draw_code_backgrounds && span.code) {
						const float margin_left = get_stylebox_margin_or(code_stylebox, SIDE_LEFT, 2.0f);
						const float margin_top = get_stylebox_margin_or(code_stylebox, SIDE_TOP, 1.0f);
						const float margin_right = get_stylebox_margin_or(code_stylebox, SIDE_RIGHT, 2.0f);
						const float margin_bottom = get_stylebox_margin_or(code_stylebox, SIDE_BOTTOM, 1.0f);
						const Rect2 code_rect = Rect2(p_text_rect.position.x + selected_range.x - margin_left, y + 1.0f - margin_top, selected_range.y - selected_range.x + margin_left + margin_right, line_size.y - 2.0f + margin_top + margin_bottom);
						draw_stylebox_or_rect(code_stylebox, code_rect, code_background_color);
					}
					if (p_draw_link_underlines && !span.link_uri.is_empty()) {
						const float underline_y = y + line_size.y - 2.0f;
						draw_line(Vector2(p_text_rect.position.x + selected_range.x, underline_y), Vector2(p_text_rect.position.x + selected_range.y, underline_y), link_color, 1.0f);
					}
				}
			}
			y += line_size.y + p_paragraph->get_line_spacing();
		}
	}
}

void RSTDocumentCanvas::draw_paragraph_text(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, const std::vector<RSTInlineSpan>& p_spans, int64_t p_global_start, const Color& p_default_color) {
	if (p_paragraph.is_null()) {
		return;
	}

	TextServerManager* text_server_manager = TextServerManager::get_singleton();
	if (text_server_manager == nullptr) {
		return;
	}
	const Ref<TextServer> text_server = text_server_manager->get_primary_interface();
	if (text_server.is_null()) {
		return;
	}

	const RID canvas = get_canvas_item();
	const Color code_color = get_theme_color_or(document_view, "RestructuredtextLabel", "inline_code_font_color", Color(0.86, 0.88, 0.91, 1.0));
	const Color link_color = get_theme_color_or(document_view, "RestructuredtextLabel", "link_color", Color(0.36, 0.62, 1.0, 1.0));

	float y = p_text_rect.position.y;
	for (int32_t line_index = 0; line_index < p_paragraph->get_line_count(); line_index++) {
		const RID line_rid = p_paragraph->get_line_rid(line_index);
		const Vector2 line_position = Vector2(p_text_rect.position.x, y + p_paragraph->get_line_ascent(line_index));
		const Vector2i range = p_paragraph->get_line_range(line_index);
		if (p_spans.empty()) {
			text_server->shaped_text_draw(line_rid, canvas, line_position, -1, -1, p_default_color);
		}
		else {
			for (const RSTInlineSpan& span : p_spans) {
				const Color span_color = !span.link_uri.is_empty() ? link_color : (span.code ? code_color : (span.custom_color ? span.color : p_default_color));
				const int64_t local_from = std::max<int64_t>(range.x, span.start);
				const int64_t local_to = std::min<int64_t>(range.y, span.end);
				if (local_from >= local_to) {
					continue;
				}
				const PackedVector2Array selected_ranges = text_server->shaped_text_get_selection(line_rid, local_from, local_to);
				for (int32_t range_index = 0; range_index < selected_ranges.size(); range_index++) {
					const Vector2 selected_range = selected_ranges[range_index];
					text_server->shaped_text_draw(line_rid, canvas, line_position, selected_range.x, selected_range.y, span_color);
				}
			}
		}
		y += p_paragraph->get_line_size(line_index).y + p_paragraph->get_line_spacing();
	}
}

void RSTDocumentCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_RESIZED || p_what == NOTIFICATION_THEME_CHANGED) {
		mark_layout_dirty();
	}

	if (p_what == NOTIFICATION_FOCUS_EXIT) {
		if (document_view != nullptr && document_view->is_deselect_on_focus_loss_enabled()) {
			clear_selection();
		}
		selection_drag_attempt = false;
	}

	if (p_what == NOTIFICATION_DRAG_END) {
		selection_drag_attempt = false;
	}

		if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
			if (!dragging_selection || !selection_dragged || document_view == nullptr) {
				return;
			}

			VScrollBar* v_scroll = document_view->get_v_scroll_bar();
			if (v_scroll == nullptr) {
				return;
			}

			const Size2 viewer_size = document_view->get_size();
			const Vector2 viewer_mouse = document_view->get_local_mouse_position();

			// Auto-scroll when the mouse enters the dead zone near the edge.
			// scroll_margin defines how far from the edge scrolling begins.
			const float scroll_margin = 24.0f;
			float scroll_delta = 0.0f;
			const float above = scroll_margin - viewer_mouse.y;
			const float below = viewer_mouse.y - (viewer_size.y - scroll_margin);

			if (above > 0.0f) {
				const float base_speed = 80.0f;
				scroll_delta = -base_speed * (above / scroll_margin) * static_cast<float>(get_process_delta_time());
			} else if (below > 0.0f) {
				const float base_speed = 80.0f;
				scroll_delta = base_speed * (below / scroll_margin) * static_cast<float>(get_process_delta_time());
			}

			if (scroll_delta != 0.0f) {
				const float max_scroll = v_scroll->get_max() - v_scroll->get_page();
				const float new_value = std::max<float>(0.0f, std::min<float>(max_scroll, v_scroll->get_value() + scroll_delta));
				v_scroll->set_value(new_value);

				const Vector2 canvas_mouse = get_local_mouse_position();
				selection_caret = hit_test_document(canvas_mouse);
				queue_redraw();
			}

			return;
		}

	if (p_what != NOTIFICATION_DRAW) {
		return;
	}

	rebuild_layout();
	for (const RSTCanvasItem& item : items) {
		if (item.type == RST_CANVAS_ITEM_IMAGE) {
			if (item.texture.is_valid()) {
				draw_texture_rect(item.texture, item.rect, false);
			}
			continue;
		}

		if (item.type == RST_CANVAS_ITEM_TABLE) {
			draw_stylebox_or_rect(item.panel_stylebox, item.rect, item.background_color);
			for (const RSTTableCell& cell : item.cells) {
				draw_stylebox_or_rect(cell.stylebox, cell.rect, Color(0, 0, 0, 0));
				draw_span_decorations(cell.paragraph, cell.text_rect, cell.spans, cell.global_start, true, false);
				draw_range_background_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end, search_match_start, search_match_end, get_theme_color_or(document_view, "RestructuredtextLabel", "search_result_color", Color(1.0, 0.72, 0.16, 0.42)));
				draw_selection_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end);
				draw_paragraph_text(cell.paragraph, cell.text_rect, cell.spans, cell.global_start, cell.text_color);
				draw_span_decorations(cell.paragraph, cell.text_rect, cell.spans, cell.global_start, false, true);
			}
			continue;
		}

		if (item.framed) {
			draw_stylebox_or_rect(item.panel_stylebox, item.rect, item.background_color);
			if (item.header_height > 0) {
				const Rect2 header_rect = Rect2(item.rect.position, Vector2(item.rect.size.x, std::min<float>(item.rect.size.y, item.header_height)));
				draw_stylebox_or_rect(item.header_stylebox, header_rect, item.header_color);
			}
		}

		draw_span_decorations(item.header_paragraph, item.header_text_rect, item.header_spans, item.header_global_start, true, false);
		draw_span_decorations(item.paragraph, item.text_rect, item.spans, item.paragraph_global_start, true, false);
		draw_list_markers(item);
		draw_search_highlight_for_item(item);
		draw_selection_for_item(item);
		draw_paragraph_text(item.header_paragraph, item.header_text_rect, item.header_spans, item.header_global_start, item.header_text_color);
		draw_paragraph_text(item.paragraph, item.text_rect, item.spans, item.paragraph_global_start, item.text_color);
		draw_span_decorations(item.header_paragraph, item.header_text_rect, item.header_spans, item.header_global_start, false, true);
		draw_span_decorations(item.paragraph, item.text_rect, item.spans, item.paragraph_global_start, false, true);
	}
}

int64_t RSTDocumentCanvas::hit_test_document(const Vector2& p_position) const {
	if (items.empty()) {
		return 0;
	}

	for (const RSTCanvasItem& item : items) {
		if (p_position.y < item.rect.position.y) {
			return item.global_start;
		}
		if (p_position.y <= item.rect.get_end().y) {
			if (item.type == RST_CANVAS_ITEM_IMAGE || !item.paragraph.is_valid()) {
				if (item.type == RST_CANVAS_ITEM_TABLE) {
					for (const RSTTableCell& cell : item.cells) {
						if (!cell.rect.has_point(p_position)) {
							continue;
						}
						const Vector2 local_position = p_position - cell.text_rect.position;
						const int32_t local_character = std::min<int32_t>(std::max<int32_t>(cell.paragraph->hit_test(local_position), 0), static_cast<int32_t>(cell.global_end - cell.global_start));
						return cell.global_start + local_character;
					}
				}
				if (item.header_paragraph.is_valid()) {
					const Rect2 header_rect = Rect2(item.rect.position, Vector2(item.rect.size.x, std::min<float>(item.rect.size.y, item.header_height)));
					if (header_rect.has_point(p_position)) {
						const Vector2 local_position = p_position - item.header_text_rect.position;
						const int32_t local_character = std::min<int32_t>(std::max<int32_t>(item.header_paragraph->hit_test(local_position), 0), static_cast<int32_t>(item.header_global_end - item.header_global_start));
						return item.header_global_start + local_character;
					}
				}
				return p_position.x < item.rect.get_center().x ? item.global_start : item.global_end;
			}

			if (item.header_paragraph.is_valid()) {
				const Rect2 header_rect = Rect2(item.rect.position, Vector2(item.rect.size.x, std::min<float>(item.rect.size.y, item.header_height)));
				if (header_rect.has_point(p_position)) {
					const Vector2 local_position = p_position - item.header_text_rect.position;
					const int32_t local_character = std::min<int32_t>(std::max<int32_t>(item.header_paragraph->hit_test(local_position), 0), static_cast<int32_t>(item.header_global_end - item.header_global_start));
					return item.header_global_start + local_character;
				}
			}
			const Vector2 local_position = p_position - item.text_rect.position;
			const int32_t local_character = std::min<int32_t>(std::max<int32_t>(item.paragraph->hit_test(local_position), 0), static_cast<int32_t>(item.paragraph_global_end - item.paragraph_global_start));
			return item.paragraph_global_start + local_character;
		}
	}

	return plain_text.length();
}

String RSTDocumentCanvas::link_uri_at_position(const Vector2& p_position) const {
	return link_uri_at_character(hit_test_document(p_position));
}

int64_t RSTDocumentCanvas::line_column_to_offset(int32_t p_line, int32_t p_column) const {
	const int32_t target_line = std::max<int32_t>(0, p_line);
	const int32_t target_column = std::max<int32_t>(0, p_column);
	int32_t current_line = 0;
	int32_t current_column = 0;

	for (int64_t index = 0; index < plain_text.length(); index++) {
		if (current_line == target_line && current_column >= target_column) {
			return index;
		}

		if (plain_text[index] == '\n') {
			if (current_line == target_line) {
				return index;
			}
			current_line++;
			current_column = 0;
		}
		else {
			current_column++;
		}
	}

	return plain_text.length();
}

Vector2i RSTDocumentCanvas::offset_to_line_column(int64_t p_offset) const {
	const int64_t target_offset = std::max<int64_t>(0, std::min<int64_t>(p_offset, plain_text.length()));
	int32_t line = 0;
	int32_t column = 0;

	for (int64_t index = 0; index < target_offset; index++) {
		if (plain_text[index] == '\n') {
			line++;
			column = 0;
		}
		else {
			column++;
		}
	}

	return Vector2i(line, column);
}

String RSTDocumentCanvas::link_uri_at_character(int64_t p_character) const {
	for (const RSTCanvasItem& item : items) {
		if (item.type == RST_CANVAS_ITEM_TABLE) {
			for (const RSTTableCell& cell : item.cells) {
				if (p_character < cell.global_start || p_character > cell.global_end) {
					continue;
				}
				const int64_t local_character = p_character - cell.global_start;
				for (const RSTInlineSpan& span : cell.spans) {
					if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
						return span.link_uri;
					}
				}
			}
			continue;
		}

		if (p_character < item.global_start || p_character > item.global_end) {
			continue;
		}
		if (p_character >= item.header_global_start && p_character <= item.header_global_end) {
			const int64_t local_character = p_character - item.header_global_start;
			for (const RSTInlineSpan& span : item.header_spans) {
				if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
					return span.link_uri;
				}
			}
		}
		if (p_character < item.paragraph_global_start || p_character > item.paragraph_global_end) {
			continue;
		}
		const int64_t local_character = p_character - item.paragraph_global_start;
		for (const RSTInlineSpan& span : item.spans) {
			if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
				return span.link_uri;
			}
		}
	}

	return String();
}

void RSTDocumentCanvas::_gui_input(const Ref<InputEvent>& p_event) {
	const Ref<InputEventMouseButton> mouse_button = p_event;

	// Ctrl+MouseWheel zoom: adjust extra_font_size on the parent viewer.
	// Must intercept here before ScrollContainer consumes the wheel event.
	if (mouse_button.is_valid() && mouse_button->is_ctrl_pressed() &&
			(mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_UP ||
			 mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN)) {
		if (document_view != nullptr) {
			const int32_t delta = (mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_UP) ? 1 : -1;
			document_view->set_extra_font_size(document_view->get_extra_font_size() + delta);
		}
		accept_event();
		return;
	}

	if (mouse_button.is_valid() && mouse_button->get_button_index() == MOUSE_BUTTON_RIGHT && mouse_button->is_pressed() && document_view != nullptr && document_view->is_context_menu_enabled()) {
		get_menu();
		_update_context_menu();
		menu->set_position(get_screen_transform().xform(mouse_button->get_position()));
		menu->reset_size();
		menu->popup();
		menu->grab_focus();
		accept_event();
		return;
	}

	if (mouse_button.is_valid() && mouse_button->get_button_index() == MOUSE_BUTTON_LEFT) {
		if (mouse_button->is_pressed()) {
			rebuild_layout();
			grab_focus();
			const bool double_click = mouse_button->is_double_click();
			const bool can_select = document_view == nullptr || document_view->is_selection_enabled();
			selection_drag_attempt = false;
			dragging_selection = false;
			selection_dragged = false;
			press_position = mouse_button->get_position();
			if (can_select && !double_click && document_view != nullptr && document_view->is_drag_and_drop_selection_enabled() && is_position_inside_selection(press_position)) {
				selection_drag_attempt = true;
				accept_event();
				return;
			}

			dragging_selection = can_select;
			if (can_select) {
				selection_anchor = hit_test_document(mouse_button->get_position());
				selection_caret = selection_anchor;
				if (double_click) {
					bool text_hit = false;
					for (const RSTCanvasItem& item : items) {
						if (item.header_paragraph.is_valid() && item.header_text_rect.has_point(press_position)) {
							text_hit = true;
							break;
						}
						if (item.type == RST_CANVAS_ITEM_TABLE) {
							for (const RSTTableCell& cell : item.cells) {
								if (cell.paragraph.is_valid() && cell.text_rect.has_point(press_position)) {
									text_hit = true;
									break;
								}
							}
						}
						else if (item.type != RST_CANVAS_ITEM_IMAGE && item.paragraph.is_valid() && item.text_rect.has_point(press_position)) {
							text_hit = true;
						}
						if (text_hit) break;
					}
					if (text_hit) {
						const auto boundary = dockit::word_boundary_at(plain_text, selection_anchor, dockit::get_project_word_language(),
							0, -1, document_view != nullptr ? document_view->get_word_selection_mode() : RestructuredtextLabel::WORD_SELECTION_MODE_AUTO);
						if (boundary) {
							selection_anchor = boundary->start;
							selection_caret = boundary->end;
						}
					}
				}
			}
			set_process_internal(true);
			queue_redraw();
		}
		else {
			if (selection_drag_attempt) {
				selection_drag_attempt = false;
				if (is_position_inside_selection(mouse_button->get_position())) {
					clear_selection();
				}
				accept_event();
				return;
			}

			if (!selection_dragged) {
				const String uri = link_uri_at_position(mouse_button->get_position());
				if (!uri.is_empty() && document_view != nullptr) {
					document_view->activate_link_uri(uri);
				}
			}
			dragging_selection = false;
			selection_dragged = false;
			set_process_internal(false);
			// Restore cursor after interaction ends
			const String uri = link_uri_at_position(mouse_button->get_position());
			if (!uri.is_empty()) {
				set_default_cursor_shape(CURSOR_POINTING_HAND);
			} else {
				set_default_cursor_shape(CURSOR_ARROW);
			}
		}
		accept_event();
		return;
	}

	const Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid() && selection_drag_attempt) {
		return;
	}

	if (mouse_motion.is_valid() && dragging_selection) {
		const Vector2 delta = mouse_motion->get_position() - press_position;
		if (std::abs(delta.x) > 3.0f || std::abs(delta.y) > 3.0f) {
			if (!selection_dragged) {
				selection_dragged = true;
				set_default_cursor_shape(CURSOR_IBEAM);
			}
		}
		if (selection_dragged) {
			selection_caret = hit_test_document(mouse_motion->get_position());
			queue_redraw();
		}
		accept_event();
		return;
	}

	if (mouse_motion.is_valid()) {
		// Update cursor based on what's under the mouse
		const String uri = link_uri_at_position(mouse_motion->get_position());
		if (!uri.is_empty()) {
			set_default_cursor_shape(CURSOR_POINTING_HAND);
		} else {
			set_default_cursor_shape(CURSOR_ARROW);
		}
	}

	const Ref<InputEventKey> key = p_event;
	if (key.is_valid() && key->is_pressed() && !key->is_echo()) {
		if (document_view != nullptr && document_view->is_shortcut_keys_enabled() && key->is_action("ui_text_select_all", true)) {
			select_all();
			accept_event();
			return;
		}
		if (document_view != nullptr && document_view->is_shortcut_keys_enabled() && key->is_action("ui_copy", true)) {
			if (has_selection()) {
				DisplayServer* display_server = DisplayServer::get_singleton();
				if (display_server != nullptr) {
					display_server->clipboard_set(get_selected_text());
				}
			}
			accept_event();
			return;
		}
		if (key->is_action("ui_menu", true)) {
			if (document_view != nullptr && document_view->is_context_menu_enabled()) {
				get_menu();
				_update_context_menu();
				menu->set_position(get_screen_position());
				menu->reset_size();
				menu->popup();
				menu->grab_focus();
			}
			accept_event();
			return;
		}
	}
}

Variant RSTDocumentCanvas::_get_drag_data(const Vector2& p_at_position) {
	Variant drag_data = Control::_get_drag_data(p_at_position);
	if (drag_data.get_type() != Variant::NIL) {
		return drag_data;
	}

	if (document_view == nullptr || !document_view->is_drag_and_drop_selection_enabled() || !selection_drag_attempt) {
		return Variant();
	}

	rebuild_layout();
	const String selected_text = get_selected_text();
	if (selected_text.is_empty()) {
		return Variant();
	}

	Label* preview = memnew(Label);
	preview->set_text(selected_text);
	preview->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	preview->set_auto_translate_mode(Node::AUTO_TRANSLATE_MODE_DISABLED);
	set_drag_preview(preview);
	return selected_text;
}

void RestructuredtextLabel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_document", "document"), &RestructuredtextLabel::set_document);
	ClassDB::bind_method(D_METHOD("get_document"), &RestructuredtextLabel::get_document);
	ClassDB::bind_method(D_METHOD("set_access", "access"), &RestructuredtextLabel::set_access);
	ClassDB::bind_method(D_METHOD("get_access"), &RestructuredtextLabel::get_access);
	ClassDB::bind_method(D_METHOD("set_root_subfolder", "root_subfolder"), &RestructuredtextLabel::set_root_subfolder);
	ClassDB::bind_method(D_METHOD("get_root_subfolder"), &RestructuredtextLabel::get_root_subfolder);
	ClassDB::bind_method(D_METHOD("get_resource_base_dir"), &RestructuredtextLabel::get_resource_base_dir);
	ClassDB::bind_method(D_METHOD("set_rst_text", "text"), &RestructuredtextLabel::set_rst_text);
	ClassDB::bind_method(D_METHOD("get_rst_text"), &RestructuredtextLabel::get_rst_text);
	ClassDB::bind_method(D_METHOD("set_content_margin", "margin"), &RestructuredtextLabel::set_content_margin);
	ClassDB::bind_method(D_METHOD("get_content_margin"), &RestructuredtextLabel::get_content_margin);
	ClassDB::bind_method(D_METHOD("set_extra_font_size", "extra"), &RestructuredtextLabel::set_extra_font_size);
	ClassDB::bind_method(D_METHOD("get_extra_font_size"), &RestructuredtextLabel::get_extra_font_size);
	ClassDB::bind_method(D_METHOD("set_open_external_links", "enabled"), &RestructuredtextLabel::set_open_external_links);
	ClassDB::bind_method(D_METHOD("get_open_external_links"), &RestructuredtextLabel::get_open_external_links);
	ClassDB::bind_method(D_METHOD("set_selection_enabled", "enabled"), &RestructuredtextLabel::set_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_selection_enabled"), &RestructuredtextLabel::is_selection_enabled);
	ClassDB::bind_method(D_METHOD("set_word_selection_mode", "mode"), &RestructuredtextLabel::set_word_selection_mode);
	ClassDB::bind_method(D_METHOD("get_word_selection_mode"), &RestructuredtextLabel::get_word_selection_mode);
	ClassDB::bind_method(D_METHOD("set_deselect_on_focus_loss_enabled", "enabled"), &RestructuredtextLabel::set_deselect_on_focus_loss_enabled);
	ClassDB::bind_method(D_METHOD("is_deselect_on_focus_loss_enabled"), &RestructuredtextLabel::is_deselect_on_focus_loss_enabled);
	ClassDB::bind_method(D_METHOD("set_drag_and_drop_selection_enabled", "enabled"), &RestructuredtextLabel::set_drag_and_drop_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_drag_and_drop_selection_enabled"), &RestructuredtextLabel::is_drag_and_drop_selection_enabled);
	ClassDB::bind_method(D_METHOD("set_context_menu_enabled", "enabled"), &RestructuredtextLabel::set_context_menu_enabled);
	ClassDB::bind_method(D_METHOD("is_context_menu_enabled"), &RestructuredtextLabel::is_context_menu_enabled);
	ClassDB::bind_method(D_METHOD("set_shortcut_keys_enabled", "enabled"), &RestructuredtextLabel::set_shortcut_keys_enabled);
	ClassDB::bind_method(D_METHOD("is_shortcut_keys_enabled"), &RestructuredtextLabel::is_shortcut_keys_enabled);
	ClassDB::bind_method(D_METHOD("set_fit_content", "enabled"), &RestructuredtextLabel::set_fit_content);
	ClassDB::bind_method(D_METHOD("is_fit_content_enabled"), &RestructuredtextLabel::is_fit_content_enabled);
	ClassDB::bind_method(D_METHOD("set_scroll_active", "active"), &RestructuredtextLabel::set_scroll_active);
	ClassDB::bind_method(D_METHOD("is_scroll_active"), &RestructuredtextLabel::is_scroll_active);
	ClassDB::bind_method(D_METHOD("set_autowrap_mode", "autowrap_mode"), &RestructuredtextLabel::set_autowrap_mode);
	ClassDB::bind_method(D_METHOD("get_autowrap_mode"), &RestructuredtextLabel::get_autowrap_mode);
	ClassDB::bind_method(D_METHOD("set_autowrap_trim_flags", "autowrap_trim_flags"), &RestructuredtextLabel::set_autowrap_trim_flags);
	ClassDB::bind_method(D_METHOD("get_autowrap_trim_flags"), &RestructuredtextLabel::get_autowrap_trim_flags);
	ClassDB::bind_method(D_METHOD("load_rst_file", "path"), &RestructuredtextLabel::load_rst_file);
	ClassDB::bind_method(D_METHOD("render_text", "text"), &RestructuredtextLabel::render_text);
	ClassDB::bind_method(D_METHOD("refresh"), &RestructuredtextLabel::refresh);
	ClassDB::bind_method(D_METHOD("clear_document"), &RestructuredtextLabel::clear_document);
	ClassDB::bind_method(D_METHOD("select_all"), &RestructuredtextLabel::select_all);
	ClassDB::bind_method(D_METHOD("clear_selection"), &RestructuredtextLabel::clear_selection);
	ClassDB::bind_method(D_METHOD("search", "text", "flags", "from_line", "from_column"), &RestructuredtextLabel::search);
	ClassDB::bind_method(D_METHOD("get_selected_text"), &RestructuredtextLabel::get_selected_text);
	ClassDB::bind_method(D_METHOD("get_rendered_block_count"), &RestructuredtextLabel::get_rendered_block_count);
	ClassDB::bind_method(D_METHOD("get_content_height"), &RestructuredtextLabel::get_content_height);
	ClassDB::bind_method(D_METHOD("resolve_document_path", "path"), &RestructuredtextLabel::resolve_document_path);
	ClassDB::bind_method(D_METHOD("get_h_scroll_bar"), &RestructuredtextLabel::get_h_scroll_bar);
	ClassDB::bind_method(D_METHOD("get_v_scroll_bar"), &RestructuredtextLabel::get_v_scroll_bar);
	ClassDB::bind_method(D_METHOD("resolve_link_uri", "role", "target"), &RestructuredtextLabel::resolve_link_uri);
	ClassDB::bind_method(D_METHOD("activate_link_uri", "uri"), &RestructuredtextLabel::activate_link_uri);
	ClassDB::bind_method(D_METHOD("_on_meta_clicked", "meta"), &RestructuredtextLabel::_on_meta_clicked);
	ClassDB::bind_method(D_METHOD("has_anchor", "anchor"), &RestructuredtextLabel::has_anchor);
	ClassDB::bind_method(D_METHOD("get_anchors"), &RestructuredtextLabel::get_anchors);
	ClassDB::bind_method(D_METHOD("scroll_to_anchor", "anchor", "align"), &RestructuredtextLabel::scroll_to_anchor, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("scroll_to_search_result", "align"), &RestructuredtextLabel::scroll_to_search_result, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("get_menu"), &RestructuredtextLabel::get_menu);
	ClassDB::bind_method(D_METHOD("is_menu_visible"), &RestructuredtextLabel::is_menu_visible);
	ClassDB::bind_method(D_METHOD("menu_option", "option"), &RestructuredtextLabel::menu_option);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "document", PROPERTY_HINT_RESOURCE_TYPE, "RSTDocument"), "set_document", "get_document");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "rst_text", PROPERTY_HINT_MULTILINE_TEXT), "set_rst_text", "get_rst_text");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "access", PROPERTY_HINT_ENUM, "Resources,User Data,File System"), "set_access", "get_access");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "root_subfolder", PROPERTY_HINT_DIR), "set_root_subfolder", "get_root_subfolder");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "content_margin", PROPERTY_HINT_RANGE, "0,128,1,or_greater"), "set_content_margin", "get_content_margin");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "extra_font_size", PROPERTY_HINT_RANGE, "0,16,1"), "set_extra_font_size", "get_extra_font_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "open_external_links"), "set_open_external_links", "get_open_external_links");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "autowrap_mode", PROPERTY_HINT_ENUM, "Off,Arbitrary,Word,Word (Smart)"), "set_autowrap_mode", "get_autowrap_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "autowrap_trim_flags", PROPERTY_HINT_FLAGS, "Trim Spaces After Break:64,Trim Spaces Before Break:128"), "set_autowrap_trim_flags", "get_autowrap_trim_flags");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fit_content"), "set_fit_content", "is_fit_content_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_active"), "set_scroll_active", "is_scroll_active");

	ADD_GROUP("Text Selection", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "selection_enabled", PROPERTY_HINT_GROUP_ENABLE), "set_selection_enabled", "is_selection_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "word_selection_mode", PROPERTY_HINT_ENUM, "Auto:0,TextServer:1,Jieba:2"),
		"set_word_selection_mode", "get_word_selection_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deselect_on_focus_loss_enabled"), "set_deselect_on_focus_loss_enabled", "is_deselect_on_focus_loss_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "drag_and_drop_selection_enabled"), "set_drag_and_drop_selection_enabled", "is_drag_and_drop_selection_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "context_menu_enabled"), "set_context_menu_enabled", "is_context_menu_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shortcut_keys_enabled"), "set_shortcut_keys_enabled", "is_shortcut_keys_enabled");

	BIND_ENUM_CONSTANT(MENU_COPY);
	BIND_ENUM_CONSTANT(MENU_SELECT_ALL);
	BIND_ENUM_CONSTANT(MENU_MAX);
	BIND_ENUM_CONSTANT(ACCESS_RESOURCES);
	BIND_ENUM_CONSTANT(ACCESS_USER_DATA);
	BIND_ENUM_CONSTANT(ACCESS_FILE_SYSTEM);
	BIND_ENUM_CONSTANT(WORD_SELECTION_MODE_AUTO);
	BIND_ENUM_CONSTANT(WORD_SELECTION_MODE_TEXT_SERVER);
	BIND_ENUM_CONSTANT(WORD_SELECTION_MODE_JIEBA);

	ADD_SIGNAL(MethodInfo("link_pressed", PropertyInfo(Variant::STRING, "uri")));
	ADD_SIGNAL(MethodInfo("render_warning", PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "line")));
	ADD_SIGNAL(MethodInfo("extra_font_size_changed", PropertyInfo(Variant::INT, "font_size")));
}

void RestructuredtextLabel::_validate_property(PropertyInfo& p_property) const {
	if (p_property.name == StringName("word_selection_mode")) {
		p_property.hint_string = dockit::get_word_selection_mode_hint();
	}
}

void RestructuredtextLabel::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		ensure_controls();
	}

	if (p_what == NOTIFICATION_READY) {
		refresh();
	}

	if (p_what == NOTIFICATION_RESIZED && document_canvas != nullptr && !fit_content) {
		document_canvas->mark_layout_dirty();
	}

	if (p_what == NOTIFICATION_THEME_CHANGED) {
		apply_theme_settings();
		if (document_canvas != nullptr) {
			refresh();
		}
	}

	if (p_what == NOTIFICATION_FOCUS_EXIT) {
		if (deselect_on_focus_loss_enabled && document_canvas != nullptr) {
			document_canvas->clear_selection();
		}
	}
}

Vector2 RestructuredtextLabel::_get_minimum_size() const {
	if (!fit_content) {
		return Vector2();
	}

	Control* self = const_cast<RestructuredtextLabel*>(this);
	const int32_t top = get_theme_constant_or(self, "RestructuredtextLabel", "margin_top", content_margin);
	const int32_t bottom = get_theme_constant_or(self, "RestructuredtextLabel", "margin_bottom", content_margin);
	const float text_height = document_canvas == nullptr ? 1.0f : document_canvas->get_content_height();
	return Vector2(0.0f, text_height + top + bottom);
}

void RestructuredtextLabel::ensure_controls() {
	if (scroll_container != nullptr) {
		return;
	}

	scroll_container = memnew(ScrollContainer);
	scroll_container->set_name("RSTScrollContainer");
	scroll_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(scroll_container, false, Node::INTERNAL_MODE_BACK);

	margin_container = memnew(MarginContainer);
	margin_container->set_name("RSTMarginContainer");
	margin_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	margin_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->add_child(margin_container, false, Node::INTERNAL_MODE_BACK);

	document_canvas = memnew(RSTDocumentCanvas);
	document_canvas->set_name("RSTDocumentCanvas");
	document_canvas->set_document_view(this);
	document_canvas->set_focus_mode(Control::FOCUS_CLICK);
	document_canvas->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	document_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	document_canvas->set_custom_minimum_size(Vector2(0.0, 1.0));
	margin_container->add_child(document_canvas, false, Node::INTERNAL_MODE_BACK);

	apply_theme_settings();
	apply_scroll_settings();
}

void RestructuredtextLabel::apply_theme_settings() {
	if (margin_container == nullptr) {
		return;
	}

	const int32_t left = get_theme_constant_or(this, "RestructuredtextLabel", "margin_left", content_margin);
	const int32_t top = get_theme_constant_or(this, "RestructuredtextLabel", "margin_top", content_margin);
	const int32_t right = get_theme_constant_or(this, "RestructuredtextLabel", "margin_right", content_margin);
	const int32_t bottom = get_theme_constant_or(this, "RestructuredtextLabel", "margin_bottom", content_margin);

	margin_container->add_theme_constant_override("margin_left", left);
	margin_container->add_theme_constant_override("margin_top", top);
	margin_container->add_theme_constant_override("margin_right", right);
	margin_container->add_theme_constant_override("margin_bottom", bottom);

	if (document_canvas != nullptr) {
		document_canvas->queue_redraw();
	}
	if (fit_content) {
		update_minimum_size();
	}
}

void RestructuredtextLabel::apply_scroll_settings() {
	if (scroll_container == nullptr) {
		return;
	}

	scroll_container->set_vertical_scroll_mode(scroll_active ? ScrollContainer::SCROLL_MODE_AUTO : ScrollContainer::SCROLL_MODE_DISABLED);
}

void RestructuredtextLabel::set_access(int32_t p_access) {
	if (p_access < ACCESS_RESOURCES || p_access > ACCESS_FILE_SYSTEM || access == p_access) {
		return;
	}

	access = p_access;
	if (document_canvas != nullptr) {
		document_canvas->mark_layout_dirty();
		if (fit_content) {
			update_minimum_size();
		}
	}
}

int32_t RestructuredtextLabel::get_access() const {
	return access;
}

void RestructuredtextLabel::set_root_subfolder(const String& p_root_subfolder) {
	if (root_subfolder == p_root_subfolder) {
		return;
	}

	root_subfolder = p_root_subfolder;
	if (document_canvas != nullptr) {
		document_canvas->mark_layout_dirty();
		if (fit_content) {
			update_minimum_size();
		}
	}
}

String RestructuredtextLabel::get_root_subfolder() const {
	return root_subfolder;
}

String RestructuredtextLabel::get_resource_base_dir() const {
	const String source_path = document.is_valid() ? document->get_source_path() : String();
	return dockit::resolve_html_resource_base_dir(access, root_subfolder, source_path);
}

void RestructuredtextLabel::set_document(const Ref<RSTDocument>& p_document) {
	document = p_document;
	if (document.is_valid()) {
		rst_text = document->get_text();
	}
	else {
		rst_text = String();
	}

	if (document_canvas != nullptr || is_inside_tree()) {
		refresh();
	}
}

Ref<RSTDocument> RestructuredtextLabel::get_document() const {
	return document;
}

void RestructuredtextLabel::set_rst_text(const String& p_text) {
	document.unref();
	rst_text = p_text;
	if (document_canvas != nullptr || is_inside_tree()) {
		refresh();
	}
}

String RestructuredtextLabel::get_rst_text() const {
	return rst_text;
}

void RestructuredtextLabel::set_content_margin(int32_t p_margin) {
	content_margin = std::max(0, p_margin);
	apply_theme_settings();
}

int32_t RestructuredtextLabel::get_content_margin() const {
	return content_margin;
}

void RestructuredtextLabel::set_extra_font_size(int32_t p_extra) {
	extra_font_size = std::clamp(p_extra, 0, 16);
	if (document_canvas != nullptr) {
		refresh();
	}
	emit_signal("extra_font_size_changed", extra_font_size);
}

int32_t RestructuredtextLabel::get_extra_font_size() const {
	return extra_font_size;
}

void RestructuredtextLabel::set_open_external_links(bool p_enabled) {
	open_external_links = p_enabled;
}

bool RestructuredtextLabel::get_open_external_links() const {
	return open_external_links;
}

Error RestructuredtextLabel::load_rst_file(const String& p_path) {
	String path = p_path.strip_edges().replace("\\", "/");
	if (path.is_empty()) {
		return ERR_INVALID_PARAMETER;
	}
	if (path.begins_with("uid://")) {
		path = ResourceUID::uid_to_path(path);
	}
	else if (path.is_absolute_path() || path.begins_with("res://") || path.begins_with("user://")) {
		path = localize_project_path(path);
	}
	else {
		path = resolve_document_path(path);
	}
	if (path.is_empty()) {
		return ERR_CANT_OPEN;
	}

	ResourceLoader* loader = ResourceLoader::get_singleton();
	if (loader != nullptr && path.begins_with("res://")) {
		const Ref<Resource> resource = loader->load(path, "RSTDocument");
		Ref<RSTDocument> loaded_document = resource;
		if (loaded_document.is_valid()) {
			set_document(loaded_document);
			return OK;
		}

		/* Fallback: untyped load works even when the import metadata
		 * doesn't specify "RSTDocument" as the resource type, which
		 * can happen in exported builds where the importer plugin
		 * is not registered. */
		const Ref<Resource> untyped_resource = loader->load(path);
		Ref<RSTDocument> untyped_document = untyped_resource;
		if (untyped_document.is_valid()) {
			set_document(untyped_document);
			return OK;
		}
	}

	if (!FileAccess::file_exists(path)) {
		ERR_PRINT("Cannot open RST file: " + path);
		return ERR_CANT_OPEN;
	}

	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	if (file.is_null()) {
		ERR_PRINT("Cannot open RST file: " + path);
		return ERR_CANT_OPEN;
	}

	Ref<RSTDocument> raw_document;
	raw_document.instantiate();
	raw_document->set_text(file->get_as_text());
	raw_document->set_source_path(path);
	if (loader != nullptr && loader->exists(RSTReferenceIndex::get_generated_path(), "RSTReferenceIndex")) {
		raw_document->set_reference_index(loader->load(RSTReferenceIndex::get_generated_path(), "RSTReferenceIndex"));
	}
	file->close();

	set_document(raw_document);
	return OK;
}

void RestructuredtextLabel::render_text(const String& p_text) {
	document.unref();
	rst_text = p_text;
	refresh();
}

void RestructuredtextLabel::refresh() {
	ensure_controls();
	if (document_canvas != nullptr) {
		document_canvas->set_rst_text(rst_text);
		rendered_block_count = document_canvas->get_rendered_block_count();
	}
	if (fit_content) {
		update_minimum_size();
	}
}

void RestructuredtextLabel::clear_document() {
	document.unref();
	rst_text = String();
	rendered_block_count = 0;
	if (document_canvas != nullptr) {
		document_canvas->clear_document();
	}
}

void RestructuredtextLabel::select_all() {
	ensure_controls();
	if (document_canvas != nullptr) {
		document_canvas->select_all();
	}
}

void RestructuredtextLabel::clear_selection() {
	if (document_canvas != nullptr) {
		document_canvas->clear_selection();
	}
}

Vector2i RestructuredtextLabel::search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column) const {
	if (document_canvas == nullptr) {
		const_cast<RestructuredtextLabel*>(this)->refresh();
	}
	if (document_canvas == nullptr) {
		return Vector2i(-1, -1);
	}

	return document_canvas->search(p_text, p_flags, p_from_line, p_from_column);
}

String RestructuredtextLabel::get_selected_text() const {
	if (document_canvas == nullptr) {
		return String();
	}

	return document_canvas->get_selected_text();
}

int32_t RestructuredtextLabel::get_rendered_block_count() const {
	return document_canvas == nullptr ? rendered_block_count : document_canvas->get_rendered_block_count();
}

float RestructuredtextLabel::get_content_height() const {
	return document_canvas == nullptr ? 0.0f : document_canvas->get_content_height();
}

String RestructuredtextLabel::resolve_document_path(const String& p_path) const {
	const dockit::HTMLResourceBase resource_base = dockit::make_html_resource_base(get_resource_base_dir());
	return dockit::resolve_html_link_href(p_path, resource_base);
}

String RestructuredtextLabel::resolve_link_uri(const String& p_role, const String& p_target) const {
	String target = p_target.strip_edges().replace("\\", "/");
	if (p_role == "ref") {
		if (target.begins_with("#")) {
			const String source_path = document.is_valid() ? document->get_source_path() : String();
			const String uid = ResourceUID::path_to_uid(source_path);
			return (uid.begins_with("uid://") ? uid : source_path) + target;
		}
		if (document.is_valid() && document->get_reference_index().is_valid()) {
			return document->get_reference_index()->resolve_reference(document->get_source_path(), target);
		}
		return String();
	}
	if (p_role == "doc") {
		String fragment;
		const int64_t fragment_index = target.find("#");
		if (fragment_index >= 0) {
			fragment = target.substr(fragment_index);
			target = target.substr(0, fragment_index);
		}
		String path = resolve_document_path(target);
		if (path.is_empty()) {
			return String();
		}
		if (path.get_extension().is_empty()) {
			path += ".rst";
		}
		if (path.begins_with("res://") && document.is_valid() && document->get_reference_index().is_valid()) {
			path = document->get_reference_index()->resolve_document(path);
		}
		else if (path.begins_with("res://")) {
			const String uid = ResourceUID::path_to_uid(path);
			if (uid.begins_with("uid://")) {
				path = uid;
			}
		}
		return path + fragment;
	}
	if (p_role == "download") {
		return resolve_document_path(target);
	}
	return target;
}

HScrollBar* RestructuredtextLabel::get_h_scroll_bar() const {
	return scroll_container->get_h_scroll_bar();
}

VScrollBar* RestructuredtextLabel::get_v_scroll_bar() const {
	return scroll_container->get_v_scroll_bar();
}

PopupMenu* RestructuredtextLabel::get_menu() {
	ensure_controls();
	return document_canvas->get_menu();
}

bool RestructuredtextLabel::is_menu_visible() const {
	return document_canvas != nullptr && document_canvas->is_menu_visible();
}

void RestructuredtextLabel::menu_option(int p_option) {
	if (document_canvas != nullptr) {
		document_canvas->menu_option(p_option);
	}
}

static bool is_external_link_uri(const String& p_uri) {
	const String lower_uri = p_uri.to_lower();
	return lower_uri.begins_with("http://") || lower_uri.begins_with("https://");
}

void RestructuredtextLabel::activate_link_uri(const String& p_uri) {
	if (p_uri.is_empty()) {
		return;
	}

	emit_signal("link_pressed", p_uri);
	const int64_t fragment_index = p_uri.find("#");
	if (fragment_index >= 0 && document.is_valid()) {
		String target_path = p_uri.substr(0, fragment_index);
		String source_path = document->get_source_path();
		if (target_path.begins_with("uid://")) {
			target_path = ResourceUID::uid_to_path(target_path);
		}
		if (source_path.begins_with("uid://")) {
			source_path = ResourceUID::uid_to_path(source_path);
		}
		if (target_path.is_empty() || localize_project_path(target_path) == localize_project_path(source_path)) {
			scroll_to_anchor(p_uri.substr(fragment_index + 1));
		}
	}
	if (open_external_links && is_external_link_uri(p_uri)) {
		OS* os = OS::get_singleton();
		if (os != nullptr) {
			const Error error = os->shell_open(p_uri);
			if (error != OK) {
				emit_signal("render_warning", String("Could not open external link: ") + p_uri, 0);
			}
		}
	}
}

void RestructuredtextLabel::_on_meta_clicked(const Variant& p_meta) {
	String uri;
	if (p_meta.get_type() == Variant::STRING || p_meta.get_type() == Variant::STRING_NAME) {
		uri = p_meta;
	}
	else {
		uri = p_meta.stringify();
	}

	activate_link_uri(uri);
}

bool RestructuredtextLabel::has_anchor(const String& p_anchor) const {
	if (document_canvas == nullptr) {
		return false;
	}
	return document_canvas->has_anchor(p_anchor);
}

PackedStringArray RestructuredtextLabel::get_anchors() const {
	if (document_canvas == nullptr) {
		return PackedStringArray();
	}
	return document_canvas->get_anchors();
}

bool RestructuredtextLabel::scroll_to_anchor(const String& p_anchor, float p_align) {
	if (document_canvas == nullptr || scroll_container == nullptr) {
		return false;
	}

	const float offset = document_canvas->get_anchor_offset(p_anchor);
	if (offset < 0.0f) {
		return false;
	}

	VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
	if (v_scroll == nullptr) {
		return false;
	}

	const float viewport_height = scroll_container->get_size().y;
	const float max_scroll = std::max<float>(0.0f, document_canvas->get_content_height() - viewport_height);
	const float target = std::max<float>(0.0f, offset - viewport_height * p_align);
	v_scroll->set_value(std::min<float>(target, max_scroll));
	return true;
}

bool RestructuredtextLabel::scroll_to_search_result(float p_align) {
	if (document_canvas == nullptr || scroll_container == nullptr) {
		return false;
	}

	const float offset = document_canvas->get_search_result_y();
	if (offset < 0.0f) {
		return false;
	}

	VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
	if (v_scroll == nullptr) {
		return false;
	}

	const float viewport_height = scroll_container->get_size().y;
	const float max_scroll = std::max<float>(0.0f, document_canvas->get_content_height() - viewport_height);
	const float target = std::max<float>(0.0f, offset - viewport_height * p_align);
	v_scroll->set_value(std::min<float>(target, max_scroll));
	return true;
}

void RestructuredtextLabel::set_selection_enabled(bool p_enabled) {
	if (selection_enabled == p_enabled) {
		return;
	}

	selection_enabled = p_enabled;
	if (!p_enabled) {
		if (document_canvas != nullptr) {
			document_canvas->clear_selection();
		}
		set_focus_mode(FOCUS_ACCESSIBILITY);
	} else {
		set_focus_mode(FOCUS_ALL);
	}
}

bool RestructuredtextLabel::is_selection_enabled() const {
	return selection_enabled;
}

void RestructuredtextLabel::set_word_selection_mode(WordSelectionMode p_mode) {
	if (p_mode < WORD_SELECTION_MODE_AUTO || p_mode > WORD_SELECTION_MODE_JIEBA) p_mode = WORD_SELECTION_MODE_AUTO;
	if (word_selection_mode == p_mode) return;
	word_selection_mode = p_mode;
	notify_property_list_changed();
}

RestructuredtextLabel::WordSelectionMode RestructuredtextLabel::get_word_selection_mode() const {
	return word_selection_mode;
}

void RestructuredtextLabel::set_deselect_on_focus_loss_enabled(bool p_enabled) {
	deselect_on_focus_loss_enabled = p_enabled;
}

bool RestructuredtextLabel::is_deselect_on_focus_loss_enabled() const {
	return deselect_on_focus_loss_enabled;
}

void RestructuredtextLabel::set_drag_and_drop_selection_enabled(bool p_enabled) {
	drag_and_drop_selection_enabled = p_enabled;
}

bool RestructuredtextLabel::is_drag_and_drop_selection_enabled() const {
	return drag_and_drop_selection_enabled;
}

void RestructuredtextLabel::set_context_menu_enabled(bool p_enabled) {
	context_menu_enabled = p_enabled;
}

bool RestructuredtextLabel::is_context_menu_enabled() const {
	return context_menu_enabled;
}

void RestructuredtextLabel::set_shortcut_keys_enabled(bool p_enabled) {
	shortcut_keys_enabled = p_enabled;
}

bool RestructuredtextLabel::is_shortcut_keys_enabled() const {
	return shortcut_keys_enabled;
}

void RestructuredtextLabel::set_fit_content(bool p_enabled) {
	if (fit_content == p_enabled) {
		return;
	}

	fit_content = p_enabled;
	update_minimum_size();
}

bool RestructuredtextLabel::is_fit_content_enabled() const {
	return fit_content;
}

void RestructuredtextLabel::set_scroll_active(bool p_active) {
	if (scroll_active == p_active) {
		return;
	}

	scroll_active = p_active;
	apply_scroll_settings();
	queue_redraw();
}

bool RestructuredtextLabel::is_scroll_active() const {
	return scroll_active;
}

void RestructuredtextLabel::set_autowrap_mode(TextServer::AutowrapMode p_mode) {
	if (autowrap_mode == p_mode) {
		return;
	}

	autowrap_mode = p_mode;
	if (document_canvas != nullptr) {
		document_canvas->mark_layout_dirty();
	}
	update_minimum_size();
}

TextServer::AutowrapMode RestructuredtextLabel::get_autowrap_mode() const {
	return autowrap_mode;
}

void RestructuredtextLabel::set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags) {
	const BitField<TextServer::LineBreakFlag> trimmed_flags = p_flags & get_autowrap_trim_mask();
	if (autowrap_flags_trim == trimmed_flags) {
		return;
	}

	autowrap_flags_trim = trimmed_flags;
	if (document_canvas != nullptr) {
		document_canvas->mark_layout_dirty();
	}
	update_minimum_size();
}

BitField<TextServer::LineBreakFlag> RestructuredtextLabel::get_autowrap_trim_flags() const {
	return autowrap_flags_trim;
}
