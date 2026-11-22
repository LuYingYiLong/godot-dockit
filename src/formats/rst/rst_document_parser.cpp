#include "rst_document_parser.h"

#include "nodes/restructuredtext_label.h"

#include <algorithm>
#include <climits>
#include <utility>

using namespace godot;

namespace {
	struct RSTOption {
		String name;
		String value;
	};

	struct RSTDirective {
		String name;
		String argument;
		int32_t indent = 0;
	};
}
	static bool is_blank_line(const String& p_line) {
		return p_line.strip_edges().is_empty();
	}

	static int32_t get_indent(const String& p_line) {
		int32_t indent = 0;

		while (indent < p_line.length()) {
			const char32_t character = p_line[indent];
			if (character != ' ' && character != '\t') {
				break;
			}
			indent++;
		}

		return indent;
	}

	static String trim_left_indent(const String& p_line) {
		return p_line.substr(get_indent(p_line));
	}

	static bool is_section_marker(char32_t p_marker) {
		return p_marker == '!' || p_marker == '"' || p_marker == '#' || p_marker == '$' || p_marker == '%' || p_marker == '&' || p_marker == '\'' || p_marker == '(' || p_marker == ')' || p_marker == '*' || p_marker == '+' || p_marker == ',' || p_marker == '-' || p_marker == '.' || p_marker == '/' || p_marker == ':' || p_marker == ';' || p_marker == '<' || p_marker == '=' || p_marker == '>' || p_marker == '?' || p_marker == '@' || p_marker == '[' || p_marker == '\\' || p_marker == ']' || p_marker == '^' || p_marker == '_' || p_marker == '`' || p_marker == '{' || p_marker == '|' || p_marker == '}' || p_marker == '~';
	}

	static bool is_section_underline(const String& p_line, int32_t p_title_length) {
		const String stripped = p_line.strip_edges();

		if (stripped.length() < 2 || stripped.length() < p_title_length) {
			return false;
		}

		const char32_t marker = stripped[0];
		if (!is_section_marker(marker)) {
			return false;
		}

		for (int32_t i = 1; i < stripped.length(); i++) {
			if (stripped[i] != marker) {
				return false;
			}
		}

		return true;
	}

	static String get_heading_style_key(const String& p_underline, bool p_overline) {
		const String stripped = p_underline.strip_edges();
		const String prefix = p_overline ? "over:" : "under:";
		return prefix + String::chr(stripped[0]);
	}

	static int32_t get_heading_level_for_style(const String& p_style_key, std::vector<String>& r_heading_styles) {
		for (int32_t i = 0; i < static_cast<int32_t>(r_heading_styles.size()); i++) {
			if (r_heading_styles[i] == p_style_key) {
				return std::min<int32_t>(i + 1, 5);
			}
		}

		r_heading_styles.push_back(p_style_key);
		return std::min<int32_t>(static_cast<int32_t>(r_heading_styles.size()), 5);
	}

	static bool is_heading_start(const std::vector<String>& p_lines, int32_t p_index) {
		if (p_index + 1 >= static_cast<int32_t>(p_lines.size())) {
			return false;
		}

		const String title = p_lines[p_index].strip_edges();
		if (title.is_empty()) {
			return false;
		}

		return is_section_underline(p_lines[p_index + 1], title.length());
	}

	static bool is_overline_heading_start(const std::vector<String>& p_lines, int32_t p_index) {
		if (p_index + 2 >= static_cast<int32_t>(p_lines.size())) {
			return false;
		}
		if (!is_section_underline(p_lines[p_index], 2)) {
			return false;
		}

		const String title = p_lines[p_index + 1].strip_edges();
		if (title.is_empty()) {
			return false;
		}

		return p_lines[p_index].strip_edges() == p_lines[p_index + 2].strip_edges();
	}

	static bool is_transition_line(const String& p_line) {
		const String stripped = p_line.strip_edges();
		if (stripped.length() < 4) {
			return false;
		}

		const char32_t marker = stripped[0];
		if (!is_section_marker(marker)) {
			return false;
		}

		for (int32_t i = 1; i < stripped.length(); i++) {
			if (stripped[i] != marker) {
				return false;
			}
		}

		return true;
	}

	static bool parse_bullet_line(const String& p_line, String& r_item) {
		const int32_t indent = get_indent(p_line);
		const String trimmed_left = p_line.substr(indent);

		if (trimmed_left.length() < 3) {
			return false;
		}

		const char32_t marker = trimmed_left[0];
		if ((marker != '-' && marker != '*' && marker != '+') || trimmed_left[1] != ' ') {
			return false;
		}

		r_item = trimmed_left.substr(2).strip_edges();
		return true;
	}

	static bool parse_enumerated_line(const String& p_line, String& r_item) {
		const String trimmed_left = trim_left_indent(p_line);
		if (trimmed_left.length() < 4) {
			return false;
		}

		if (trimmed_left.begins_with("#. ")) {
			r_item = trimmed_left.substr(3).strip_edges();
			return true;
		}

		int32_t index = 0;
		while (index < trimmed_left.length() && trimmed_left[index] >= '0' && trimmed_left[index] <= '9') {
			index++;
		}

		if (index == 0 || index + 2 > trimmed_left.length()) {
			return false;
		}
		if (trimmed_left[index] != '.' || trimmed_left[index + 1] != ' ') {
			return false;
		}

		r_item = trimmed_left.substr(index + 2).strip_edges();
		return true;
	}

	static bool parse_directive_line(const String& p_line, RSTDirective& r_directive) {
		const int32_t indent = get_indent(p_line);
		const String trimmed_left = p_line.substr(indent);
		if (!trimmed_left.begins_with(".. ")) {
			return false;
		}

		const int64_t marker_position = trimmed_left.find("::");
		if (marker_position < 0) {
			return false;
		}

		const String name = trimmed_left.substr(3, marker_position - 3).strip_edges();
		if (name.is_empty() || name.begins_with("_")) {
			return false;
		}

		r_directive.name = name;
		r_directive.argument = trimmed_left.substr(marker_position + 2).strip_edges();
		r_directive.indent = indent;
		return true;
	}

	static bool is_explicit_target(const String& p_line) {
		const String trimmed_left = trim_left_indent(p_line);
		return trimmed_left.begins_with(".. _") && trimmed_left.ends_with(":");
	}

	static bool parse_footnote_definition_line(const String& p_line, String& r_label, String& r_text) {
		const int32_t indent = get_indent(p_line);
		const String trimmed_left = p_line.substr(indent);
		if (!trimmed_left.begins_with(".. [")) {
			return false;
		}

		const int64_t close_position = trimmed_left.find("]");
		if (close_position <= 4) {
			return false;
		}

		if (close_position + 1 < trimmed_left.length()) {
			const char32_t next_character = trimmed_left[close_position + 1];
			if (next_character != ' ' && next_character != '\t') {
				return false;
			}
		}

		r_label = trimmed_left.substr(4, close_position - 4).strip_edges();
		r_text = trimmed_left.substr(close_position + 1).strip_edges();
		return !r_label.is_empty();
	}

	static String get_footnote_display_label(const String& p_label, const RSTFootnoteMap* p_footnotes) {
		if (p_footnotes != nullptr) {
			for (const RSTFootnoteEntry& entry : p_footnotes->entries) {
				if (entry.label == p_label) {
					return entry.display_label;
				}
			}
		}

		return p_label;
	}

String format_footnote_label(const String& p_label, const RSTFootnoteMap* p_footnotes) {
		return String("[") + get_footnote_display_label(p_label, p_footnotes) + "]";
	}

	static String make_footnote_text(const String& p_label, const String& p_text, const RSTFootnoteMap* p_footnotes) {
		const String display_label = format_footnote_label(p_label, p_footnotes);
		if (p_text.is_empty()) {
			return display_label;
		}

		return display_label + String(" ") + p_text;
	}

RSTFootnoteMap build_footnote_map(const std::vector<String>& p_lines) {
		RSTFootnoteMap footnotes;
		int32_t auto_number = 1;

		for (const String& line : p_lines) {
			String label;
			String text;
			if (!parse_footnote_definition_line(line, label, text)) {
				continue;
			}

			bool exists = false;
			for (const RSTFootnoteEntry& entry : footnotes.entries) {
				if (entry.label == label) {
					exists = true;
					break;
				}
			}
			if (exists) {
				continue;
			}

			RSTFootnoteEntry entry;
			entry.label = label;
			if (label.begins_with("#")) {
				entry.display_label = String::num_int64(auto_number++);
			}
			else {
				entry.display_label = label;
			}
			footnotes.entries.push_back(entry);
		}

		return footnotes;
	}

	static bool is_comment_line(const String& p_line) {
		RSTDirective directive;
		String footnote_label;
		String footnote_text;
		const String trimmed_left = trim_left_indent(p_line);
		return trimmed_left.begins_with(".. ") && !parse_directive_line(p_line, directive) && !is_explicit_target(p_line) && !parse_footnote_definition_line(p_line, footnote_label, footnote_text);
	}

	static int32_t consume_comment_block(const std::vector<String>& p_lines, int32_t p_index) {
		const int32_t comment_indent = get_indent(p_lines[p_index]);
		int32_t index = p_index + 1;
		bool body_started = false;

		while (index < static_cast<int32_t>(p_lines.size())) {
			if (is_blank_line(p_lines[index])) {
				if (!body_started) {
					break;
				}
				index++;
				continue;
			}

			if (get_indent(p_lines[index]) <= comment_indent) {
				break;
			}

			body_started = true;
			index++;
		}

		return index;
	}

	static bool is_option_line(const String& p_line, RSTOption& r_option) {
		const String trimmed_left = trim_left_indent(p_line);
		if (!trimmed_left.begins_with(":")) {
			return false;
		}

		const int64_t end_position = trimmed_left.find(":", 1);
		if (end_position < 0) {
			return false;
		}

		r_option.name = trimmed_left.substr(1, end_position - 1).strip_edges();
		r_option.value = trimmed_left.substr(end_position + 1).strip_edges();
		return !r_option.name.is_empty();
	}

	static String get_option_value(const std::vector<RSTOption>& p_options, const String& p_name, const String& p_default = String()) {
		for (const RSTOption& option : p_options) {
			if (option.name == p_name) {
				return option.value;
			}
		}

		return p_default;
	}

	static int32_t get_option_int(const std::vector<RSTOption>& p_options, const String& p_name, int32_t p_default = 0) {
		const String value = get_option_value(p_options, p_name);
		if (value.is_empty()) {
			return p_default;
		}

		return value.to_int();
	}

	static int32_t consume_directive_block(const std::vector<String>& p_lines, int32_t p_index, const RSTDirective& p_directive, std::vector<RSTOption>& r_options, PackedStringArray& r_body) {
		int32_t index = p_index + 1;
		bool body_started = false;
		int32_t min_indent = INT_MAX;
		std::vector<String> raw_body;

		while (index < static_cast<int32_t>(p_lines.size())) {
			const String line = p_lines[index];
			if (is_blank_line(line)) {
				if (body_started) {
					raw_body.push_back(String());
				}
				index++;
				continue;
			}

			const int32_t indent = get_indent(line);
			if (indent <= p_directive.indent) {
				break;
			}

			RSTOption option;
			if (!body_started && is_option_line(line, option)) {
				r_options.push_back(option);
				index++;
				continue;
			}

			body_started = true;
			raw_body.push_back(line);
			min_indent = std::min(min_indent, indent);
			index++;
		}

		if (min_indent == INT_MAX) {
			min_indent = 0;
		}

		for (const String& raw_line : raw_body) {
			if (raw_line.is_empty()) {
				r_body.push_back(String());
			}
			else {
				r_body.push_back(raw_line.substr(std::min<int32_t>(min_indent, raw_line.length())));
			}
		}

		return index;
	}


std::vector<String> split_lines(const String& p_text) {
		const String normalized = p_text.replace("\r\n", "\n").replace("\r", "\n");
		const PackedStringArray split = normalized.split("\n", true);
		std::vector<String> lines;

		lines.reserve(split.size());
		for (int32_t i = 0; i < split.size(); i++) {
			lines.push_back(split[i].rstrip(" \t"));
		}

		return lines;
	}

	static String join_wrapped_line(const String& p_current, const String& p_next) {
		if (p_current.is_empty()) {
			return p_next.strip_edges();
		}

		return p_current + String(" ") + p_next.strip_edges();
	}

	static String join_body_as_text(const PackedStringArray& p_lines) {
		String text;

		for (int32_t i = 0; i < p_lines.size(); i++) {
			if (is_blank_line(p_lines[i])) {
				if (!text.ends_with("\n")) {
					text += "\n";
				}
				continue;
			}

			if (text.is_empty() || text.ends_with("\n")) {
				text += p_lines[i].strip_edges();
			}
			else {
				text += " ";
				text += p_lines[i].strip_edges();
			}
		}

		return text.strip_edges();
	}

	static String strip_code_indent(const PackedStringArray& p_lines, int32_t p_indent) {
		String code;

		for (int32_t i = 0; i < p_lines.size(); i++) {
			String line = p_lines[i];
			if (!line.is_empty() && p_indent > 0) {
				line = line.substr(std::min<int32_t>(p_indent, line.length()));
			}

			code += line;
			if (i + 1 < p_lines.size()) {
				code += "\n";
			}
		}

		return code;
	}

	static bool is_directive_name(const String& p_name, const char* p_expected) {
		return p_name == String(p_expected);
	}

	static bool is_admonition_directive(const String& p_name) {
		return is_directive_name(p_name, "admonition") || is_directive_name(p_name, "attention") || is_directive_name(p_name, "caution") || is_directive_name(p_name, "danger") || is_directive_name(p_name, "error") || is_directive_name(p_name, "hint") || is_directive_name(p_name, "important") || is_directive_name(p_name, "note") || is_directive_name(p_name, "tip") || is_directive_name(p_name, "seealso") || is_directive_name(p_name, "warning");
	}

	static String get_admonition_title(const String& p_name, const String& p_argument = String()) {
		if (p_name == "admonition") {
			return p_argument.is_empty() ? String("Admonition") : p_argument;
		}
		if (p_name == "attention") {
			return "Attention";
		}
		if (p_name == "caution") {
			return "Caution";
		}
		if (p_name == "danger") {
			return "Danger";
		}
		if (p_name == "error") {
			return "Error";
		}
		if (p_name == "hint") {
			return "Hint";
		}
		if (p_name == "important") {
			return "Important";
		}
		if (p_name == "warning") {
			return "Warning";
		}
		if (p_name == "tip") {
			return "Tip";
		}
		if (p_name == "seealso") {
			return "See also";
		}

		return "Note";
	}

	static bool starts_new_block(const std::vector<String>& p_lines, int32_t p_index);

	static bool is_grid_border_line(const String& p_line) {
		const String stripped = p_line.strip_edges();
		if (stripped.length() < 3) {
			return false;
		}

		if (stripped[0] != '+' || stripped[stripped.length() - 1] != '+') {
			return false;
		}

		bool has_horizontal = false;
		for (int32_t i = 0; i < stripped.length(); i++) {
			const char32_t c = stripped[i];
			if (c != '+' && c != '-' && c != '=' && c != ' ') {
				return false;
			}
			if (c == '-' || c == '=') {
				has_horizontal = true;
			}
		}

		return has_horizontal;
	}

	static bool is_grid_cell_line(const String& p_line) {
		const int32_t indent = get_indent(p_line);
		if (indent >= p_line.length()) {
			return false;
		}

		return p_line[indent] == '|';
	}

	static int32_t parse_grid_columns(const String& p_border_line, std::vector<int32_t>& r_col_starts) {
		const String stripped = p_border_line.strip_edges();
		r_col_starts.clear();

		for (int32_t i = 0; i < stripped.length(); i++) {
			if (stripped[i] == '+') {
				r_col_starts.push_back(i);
			}
		}

		if (r_col_starts.size() < 3) {
			r_col_starts.clear();
			return 0;
		}

		return static_cast<int32_t>(r_col_starts.size()) - 1;
	}

	static bool is_grid_header_separator(const String& p_border_line) {
		const String stripped = p_border_line.strip_edges();
		if (!is_grid_border_line(stripped)) {
			return false;
		}

		for (int32_t i = 0; i < stripped.length(); i++) {
			if (stripped[i] == '=') {
				return true;
			}
		}

		return false;
	}

	static bool parse_grid_table(const std::vector<String>& p_lines, int32_t p_index, RSTBlock& r_block, int32_t& r_out_next_index) {
		std::vector<int32_t> col_starts;
		const int32_t column_count = parse_grid_columns(p_lines[p_index], col_starts);
		if (column_count < 1) {
			return false;
		}

		std::vector<PackedStringArray> all_rows;
		PackedStringArray current_row;
		for (int32_t c = 0; c < column_count; c++) {
			current_row.push_back(String());
		}

		int32_t header_rows = 0;
		bool header_finalized = false;
		bool row_has_content = false;
		const int32_t base_indent = get_indent(p_lines[p_index]);
		int32_t index = p_index;

		auto finalize_row = [&]() {
			if (row_has_content) {
				while (current_row.size() < column_count) {
					current_row.push_back(String());
				}
				all_rows.push_back(current_row);
				current_row = PackedStringArray();
				for (int32_t c = 0; c < column_count; c++) {
					current_row.push_back(String());
				}
				row_has_content = false;
			}
		};

		while (index < static_cast<int32_t>(p_lines.size())) {
			const String line = p_lines[index];

			if (is_blank_line(line)) {
				const int32_t peek = index + 1;
				if (peek < static_cast<int32_t>(p_lines.size()) && (is_grid_border_line(p_lines[peek]) || is_grid_cell_line(p_lines[peek]))) {
					index = peek;
					continue;
				}
				break;
			}

			if (is_grid_border_line(line)) {
				const int32_t line_indent = get_indent(line);
				if (line_indent < base_indent) {
					break;
				}

				std::vector<int32_t> verify_cols;
				const int32_t verify_count = parse_grid_columns(line, verify_cols);
				if (verify_count != column_count) {
					break;
				}
				for (int32_t c = 0; c < static_cast<int32_t>(verify_cols.size()); c++) {
					if (verify_cols[c] != col_starts[c]) {
						break;
					}
				}

				finalize_row();

				if (!header_finalized && is_grid_header_separator(line)) {
					header_rows = static_cast<int32_t>(all_rows.size());
					header_finalized = true;
				}

				index++;
				continue;
			}

			if (is_grid_cell_line(line)) {
				const int32_t line_indent = get_indent(line);
				if (line_indent < base_indent) {
					break;
				}

				const String stripped = line.strip_edges();
				int32_t col_idx = -1;
				int32_t pos = base_indent;

				while (pos < line.length()) {
					if (line[pos] == '|') {
						col_idx++;
						pos++;
						if (col_idx >= column_count) {
							break;
						}

						int32_t text_start = pos;
						while (pos < line.length() && line[pos] != '|') {
							pos++;
						}

						String cell_text = line.substr(text_start, pos - text_start).rstrip(" \t");
						if (!cell_text.is_empty()) {
							if (current_row[col_idx].is_empty()) {
								current_row.set(col_idx, cell_text);
							} else {
								current_row.set(col_idx, current_row[col_idx] + " " + cell_text);
							}
							row_has_content = true;
						}
						continue;
					}
					pos++;
				}

				index++;
				continue;
			}

			break;
		}

		finalize_row();

		if (all_rows.empty()) {
			return false;
		}

		r_block.type = RST_BLOCK_TABLE;
		r_block.columns = column_count;
		r_block.header_rows = header_rows;
		r_block.rows = all_rows;
		r_block.line = p_index + 1;
		r_out_next_index = index;
		return true;
	}

	static bool is_simple_table_separator(const String& p_line, bool p_require_column_spaces = true) {
		const String stripped = p_line.strip_edges();
		if (stripped.length() < 2) {
			return false;
		}

		int32_t equal_count = 0;
		bool has_space_between_equals = false;
		bool in_equals_run = false;
		bool after_first_equals_run = false;

		for (int32_t i = 0; i < stripped.length(); i++) {
			const char32_t c = stripped[i];
			if (c == '=') {
				equal_count++;
				if (!in_equals_run) {
					in_equals_run = true;
					if (after_first_equals_run) {
						has_space_between_equals = true;
					}
				}
			} else if (c == ' ') {
				if (in_equals_run) {
					in_equals_run = false;
					after_first_equals_run = true;
				}
			} else {
				return false;
			}
		}

		if (equal_count < 2) {
			return false;
		}

		if (p_require_column_spaces && !has_space_between_equals) {
			return false;
		}

		return true;
	}

	static int32_t parse_simple_table_columns(const String& p_separator, std::vector<int32_t>& r_bounds) {
		const String stripped = p_separator.strip_edges();
		r_bounds.clear();

		int32_t col_start = -1;
		for (int32_t i = 0; i < stripped.length(); i++) {
			if (stripped[i] == '=') {
				if (col_start < 0) {
					col_start = i;
				}
			} else if (stripped[i] == ' ') {
				if (col_start >= 0) {
					r_bounds.push_back(col_start);
					r_bounds.push_back(i);
					col_start = -1;
				}
			}
		}

		if (col_start >= 0) {
			r_bounds.push_back(col_start);
			r_bounds.push_back(static_cast<int32_t>(stripped.length()));
		}

		if (r_bounds.size() < 4) {
			r_bounds.clear();
			return 0;
		}

		return static_cast<int32_t>(r_bounds.size()) / 2;
	}

	static String extract_simple_cell_text(const String& p_line, int32_t p_col_start, int32_t p_col_end) {
		if (p_col_start >= p_line.length()) {
			return String();
		}

		const int32_t end = std::min<int32_t>(p_col_end, static_cast<int32_t>(p_line.length()));
		return p_line.substr(p_col_start, end - p_col_start).rstrip(" \t");
	}

	static bool parse_simple_table(const std::vector<String>& p_lines, int32_t p_index, RSTBlock& r_block, int32_t& r_out_next_index) {
		if (!is_simple_table_separator(p_lines[p_index], true)) {
			return false;
		}

		const String first_sep = p_lines[p_index].strip_edges();
		std::vector<int32_t> col_bounds;
		const int32_t column_count = parse_simple_table_columns(first_sep, col_bounds);
		if (column_count < 2) {
			return false;
		}

		std::vector<String> separator_lines;
		separator_lines.push_back(first_sep);
		std::vector<int32_t> separator_indices;
		separator_indices.push_back(p_index);

		const int32_t base_indent = get_indent(p_lines[p_index]);
		int32_t index = p_index + 1;

		while (index < static_cast<int32_t>(p_lines.size())) {
			if (is_blank_line(p_lines[index])) {
				index++;
				continue;
			}

			if (is_simple_table_separator(p_lines[index], false)) {
				const String sep_stripped = p_lines[index].strip_edges();
				std::vector<int32_t> verify_bounds;
				const int32_t verify_cols = parse_simple_table_columns(sep_stripped, verify_bounds);
				if (verify_cols != column_count) {
					break;
				}
				bool bounds_match = true;
				for (int32_t c = 0; c < static_cast<int32_t>(col_bounds.size()); c++) {
					if (col_bounds[c] != verify_bounds[c]) {
						bounds_match = false;
						break;
					}
				}
				if (!bounds_match) {
					break;
				}

				separator_lines.push_back(sep_stripped);
				separator_indices.push_back(index);
				index++;
				continue;
			}

			if (starts_new_block(p_lines, index) || is_transition_line(p_lines[index])) {
				break;
			}

			if (get_indent(p_lines[index]) < base_indent && !is_blank_line(p_lines[index])) {
				break;
			}

			index++;
		}

		if (separator_indices.size() < 2) {
			return false;
		}

		std::vector<PackedStringArray> all_rows;
		int32_t header_rows = 0;

		for (int32_t group = 0; group + 1 < static_cast<int32_t>(separator_indices.size()); group++) {
			const int32_t content_start = separator_indices[group] + 1;
			const int32_t content_end = separator_indices[group + 1];
			std::vector<PackedStringArray> group_rows;

			for (int32_t li = content_start; li < content_end; li++) {
				if (is_blank_line(p_lines[li])) {
					continue;
				}

				const String line = p_lines[li];
				const int32_t line_indent = get_indent(line);

				if (line_indent < base_indent && !is_blank_line(line)) {
					break;
				}

				PackedStringArray row;
				for (int32_t c = 0; c < column_count; c++) {
					row.push_back(String());
				}

				bool row_has_content = false;
				for (int32_t c = 0; c < column_count; c++) {
					const String cell = extract_simple_cell_text(line, col_bounds[c * 2], col_bounds[c * 2 + 1]);
					if (!cell.is_empty()) {
						row.set(c, cell);
						row_has_content = true;
					}
				}

				if (row_has_content) {
					group_rows.push_back(row);
				}
			}

			if (group == 0) {
				header_rows = static_cast<int32_t>(group_rows.size());
			}

			for (const PackedStringArray& row : group_rows) {
				all_rows.push_back(row);
			}
		}

		if (all_rows.empty()) {
			return false;
		}

		r_block.type = RST_BLOCK_TABLE;
		r_block.columns = column_count;
		r_block.header_rows = header_rows;
		r_block.rows = all_rows;
		r_block.line = p_index + 1;
		r_out_next_index = index;
		return true;
	}

	static std::vector<PackedStringArray> parse_list_table_rows(const PackedStringArray& p_body) {
		std::vector<PackedStringArray> rows;
		PackedStringArray current_row;
		int32_t current_cell = -1;

		for (int32_t i = 0; i < p_body.size(); i++) {
			const String trimmed_left = trim_left_indent(p_body[i]);
			if (trimmed_left.begins_with("* - ")) {
				if (!current_row.is_empty()) {
					rows.push_back(current_row);
				}
				current_row = PackedStringArray();
				current_row.push_back(trimmed_left.substr(4).strip_edges());
				current_cell = 0;
				continue;
			}

			if (trimmed_left.begins_with("- ") && current_cell >= 0) {
				current_row.push_back(trimmed_left.substr(2).strip_edges());
				current_cell = current_row.size() - 1;
				continue;
			}

			if (current_cell >= 0 && !trimmed_left.strip_edges().is_empty()) {
				current_row.set(current_cell, join_wrapped_line(current_row[current_cell], trimmed_left));
			}
		}

		if (!current_row.is_empty()) {
			rows.push_back(current_row);
		}

		return rows;
	}

	static bool starts_new_block(const std::vector<String>& p_lines, int32_t p_index) {
		if (p_index >= static_cast<int32_t>(p_lines.size())) {
			return false;
		}
		if (is_heading_start(p_lines, p_index) || is_overline_heading_start(p_lines, p_index)) {
			return true;
		}

		String item;
		String footnote_label;
		String footnote_text;
		RSTDirective directive;
		return parse_bullet_line(p_lines[p_index], item) || parse_enumerated_line(p_lines[p_index], item) || parse_directive_line(p_lines[p_index], directive) || parse_footnote_definition_line(p_lines[p_index], footnote_label, footnote_text) || is_explicit_target(p_lines[p_index]) || is_comment_line(p_lines[p_index]) || is_grid_border_line(p_lines[p_index]) || is_simple_table_separator(p_lines[p_index], true);
	}

	static String make_unique_heading_anchor(const String& p_heading, std::vector<std::pair<String, int32_t>>& r_counts) {
		const String base = p_heading.strip_edges().to_lower().replace(" ", "-");
		for (std::pair<String, int32_t>& entry : r_counts) {
			if (entry.first == base) {
				const int32_t occurrence = entry.second++;
				return base + String("-") + String::num_int64(occurrence);
			}
		}
		r_counts.push_back({ base, 1 });
		return base;
	}

std::vector<RSTBlock> parse_rst_blocks(const String& p_text, RestructuredtextLabel* p_owner, const RSTFootnoteMap* p_footnotes) {
		std::vector<String> lines = split_lines(p_text);
		std::vector<RSTBlock> blocks;
		std::vector<String> heading_styles;
		std::vector<std::pair<String, int32_t>> heading_anchor_counts;
		String pending_anchor;
		int32_t index = 0;

		while (index < static_cast<int32_t>(lines.size())) {
			if (is_blank_line(lines[index])) {
				index++;
				continue;
			}

			if (is_explicit_target(lines[index])) {
				const String trimmed = lines[index].strip_edges();
				String raw = trimmed.substr(3, trimmed.length() - 4).strip_edges();
				if (raw.begins_with("_")) {
					pending_anchor = raw.substr(1);
				} else {
					pending_anchor = raw;
				}
				index++;
				continue;
			}

			String footnote_label;
			String footnote_text;
			if (parse_footnote_definition_line(lines[index], footnote_label, footnote_text)) {
				RSTBlock block;
				block.type = RST_BLOCK_PARAGRAPH;
				block.line = index + 1;
				block.text = make_footnote_text(footnote_label, footnote_text, p_footnotes);
				block.anchor = pending_anchor;
				pending_anchor = String();

				const int32_t footnote_indent = get_indent(lines[index]);
				index++;
				while (index < static_cast<int32_t>(lines.size()) && !is_blank_line(lines[index]) && get_indent(lines[index]) > footnote_indent && !starts_new_block(lines, index)) {
					block.text = join_wrapped_line(block.text, lines[index]);
					index++;
				}

				blocks.push_back(block);
				continue;
			}

			if (is_comment_line(lines[index])) {
				index = consume_comment_block(lines, index);
				continue;
			}

			if (is_overline_heading_start(lines, index)) {
				RSTBlock block;
				block.type = RST_BLOCK_HEADING;
				block.text = lines[index + 1].strip_edges();
				block.level = get_heading_level_for_style(get_heading_style_key(lines[index], true), heading_styles);
				block.line = index + 2;
				const String generated_anchor = make_unique_heading_anchor(block.text, heading_anchor_counts);
				block.anchor = pending_anchor.is_empty() ? generated_anchor : pending_anchor;
				pending_anchor = String();
				blocks.push_back(block);
				index += 3;
				continue;
			}

			if (is_heading_start(lines, index)) {
				RSTBlock block;
				block.type = RST_BLOCK_HEADING;
				block.text = lines[index].strip_edges();
				block.level = get_heading_level_for_style(get_heading_style_key(lines[index + 1], false), heading_styles);
				block.line = index + 1;
				const String generated_anchor = make_unique_heading_anchor(block.text, heading_anchor_counts);
				block.anchor = pending_anchor.is_empty() ? generated_anchor : pending_anchor;
				pending_anchor = String();
				blocks.push_back(block);
				index += 2;
				continue;
			}

			RSTDirective directive;
			if (parse_directive_line(lines[index], directive)) {
				std::vector<RSTOption> options;
				PackedStringArray body;
				const int32_t next_index = consume_directive_block(lines, index, directive, options, body);

				if (directive.name == "code-block" || directive.name == "sourcecode" || directive.name == "literalinclude") {
					RSTBlock block;
					block.type = RST_BLOCK_CODE;
					block.argument = directive.argument;
					block.line = index + 1;
					block.text = strip_code_indent(body, 0);
					block.anchor = pending_anchor;
					pending_anchor = String();
					blocks.push_back(block);
				}
				else if (is_admonition_directive(directive.name)) {
					RSTBlock block;
					block.type = RST_BLOCK_ADMONITION;
					block.directive = directive.name;
					block.argument = get_admonition_title(directive.name, directive.argument);
					block.text = directive.name == "admonition" ? String() : directive.argument;
					if (!block.text.is_empty() && !body.is_empty()) {
						block.text += "\n";
					}
					block.text += join_body_as_text(body);
					block.line = index + 1;
					block.anchor = pending_anchor;
					pending_anchor = String();
					blocks.push_back(block);
				}
				else if (directive.name == "list-table" || directive.name == "csv-table") {
					RSTBlock block;
					block.type = RST_BLOCK_TABLE;
					block.argument = directive.argument;
					block.header_rows = get_option_int(options, "header-rows", 0);
					block.rows = parse_list_table_rows(body);
					block.line = index + 1;
					for (const PackedStringArray& row : block.rows) {
						block.columns = std::max<int32_t>(block.columns, row.size());
					}
					if (block.columns > 0) {
						block.anchor = pending_anchor;
						pending_anchor = String();
						blocks.push_back(block);
					}
				}
				else if (directive.name == "image" || directive.name == "figure") {
					RSTBlock block;
					block.type = RST_BLOCK_IMAGE;
					block.directive = directive.name;
					block.argument = directive.argument;
					block.image_alt = get_option_value(options, "alt");
					block.image_align = get_option_value(options, "align");
					block.image_width = get_option_value(options, "width");
					block.image_height = get_option_value(options, "height");
					block.image_scale = get_option_value(options, "scale");
					const String caption = join_body_as_text(body);
					if (!caption.is_empty()) {
						block.text = caption;
					}
					block.line = index + 1;
					block.anchor = pending_anchor;
					pending_anchor = String();
					blocks.push_back(block);
				}
				else if (directive.name == "toctree") {
					RSTBlock block;
					block.type = RST_BLOCK_TOCTREE;
					block.argument = get_option_value(options, "caption", "Contents");
					block.line = index + 1;
					for (int32_t i = 0; i < body.size(); i++) {
						const String item = body[i].strip_edges();
						if (!item.is_empty()) {
							block.items.push_back(item);
						}
					}
					if (!block.items.is_empty()) {
						block.anchor = pending_anchor;
						pending_anchor = String();
						blocks.push_back(block);
					}
				}
				else if (p_owner != nullptr) {
					p_owner->emit_signal("render_warning", "Unsupported directive: " + directive.name, index + 1);
				}

				index = next_index;
				continue;
			}

			if (is_grid_border_line(lines[index])) {
				RSTBlock table_block;
				int32_t next_index = index;
				if (parse_grid_table(lines, index, table_block, next_index)) {
					if (table_block.columns > 0) {
						table_block.anchor = pending_anchor;
						pending_anchor = String();
						blocks.push_back(table_block);
					}
					index = next_index;
					continue;
				}
			}

			if (is_simple_table_separator(lines[index], true)) {
				RSTBlock table_block;
				int32_t next_index = index;
				if (parse_simple_table(lines, index, table_block, next_index)) {
					if (table_block.columns > 0) {
						table_block.anchor = pending_anchor;
						pending_anchor = String();
						blocks.push_back(table_block);
					}
					index = next_index;
					continue;
				}
			}

			String bullet_item;
			if (parse_bullet_line(lines[index], bullet_item)) {
				RSTBlock block;
				block.type = RST_BLOCK_BULLET_LIST;
				block.line = index + 1;

				while (index < static_cast<int32_t>(lines.size())) {
					String current_item;
					const int32_t bullet_indent = get_indent(lines[index]);
					if (!parse_bullet_line(lines[index], current_item)) {
						break;
					}

					index++;
					while (index < static_cast<int32_t>(lines.size()) && !is_blank_line(lines[index])) {
						String next_item;
						if (parse_bullet_line(lines[index], next_item) || parse_enumerated_line(lines[index], next_item)) {
							break;
						}
						if (starts_new_block(lines, index) || get_indent(lines[index]) <= bullet_indent) {
							break;
						}

						current_item = join_wrapped_line(current_item, lines[index]);
						index++;
					}

					block.items.push_back(current_item);
					if (index >= static_cast<int32_t>(lines.size()) || is_blank_line(lines[index])) {
						break;
					}
				}

				block.anchor = pending_anchor;
				pending_anchor = String();
				blocks.push_back(block);
				continue;
			}

			String enumerated_item;
			if (parse_enumerated_line(lines[index], enumerated_item)) {
				RSTBlock block;
				block.type = RST_BLOCK_ENUMERATED_LIST;
				block.line = index + 1;

				while (index < static_cast<int32_t>(lines.size())) {
					String current_item;
					const int32_t item_indent = get_indent(lines[index]);
					if (!parse_enumerated_line(lines[index], current_item)) {
						break;
					}

					index++;
					while (index < static_cast<int32_t>(lines.size()) && !is_blank_line(lines[index])) {
						String next_item;
						if (parse_enumerated_line(lines[index], next_item) || parse_bullet_line(lines[index], next_item)) {
							break;
						}
						if (starts_new_block(lines, index) || get_indent(lines[index]) <= item_indent) {
							break;
						}

						current_item = join_wrapped_line(current_item, lines[index]);
						index++;
					}

					block.items.push_back(current_item);
					if (index >= static_cast<int32_t>(lines.size()) || is_blank_line(lines[index])) {
						break;
					}
				}

				block.anchor = pending_anchor;
				pending_anchor = String();
				blocks.push_back(block);
				continue;
			}

			if (is_transition_line(lines[index])) {
				RSTBlock block;
				block.type = RST_BLOCK_TRANSITION;
				block.line = index + 1;
				block.anchor = pending_anchor;
				pending_anchor = String();
				blocks.push_back(block);
				index++;
				continue;
			}

			RSTBlock paragraph;
			paragraph.type = RST_BLOCK_PARAGRAPH;
			paragraph.line = index + 1;

			while (index < static_cast<int32_t>(lines.size()) && !is_blank_line(lines[index]) && !starts_new_block(lines, index)) {
				paragraph.text = join_wrapped_line(paragraph.text, lines[index]);
				index++;
			}

			if (paragraph.text.ends_with("::")) {
				String visible_text = paragraph.text.substr(0, paragraph.text.length() - 1).strip_edges();
				if (visible_text == ":") {
					visible_text = String();
				}
				if (!visible_text.is_empty()) {
					paragraph.text = visible_text;
					paragraph.anchor = pending_anchor;
					pending_anchor = String();
					blocks.push_back(paragraph);
				}

				while (index < static_cast<int32_t>(lines.size()) && is_blank_line(lines[index])) {
					index++;
				}

				PackedStringArray code_lines;
				int32_t min_indent = INT_MAX;
				const int32_t code_line = index + 1;

				while (index < static_cast<int32_t>(lines.size())) {
					if (is_blank_line(lines[index])) {
						code_lines.push_back(String());
						index++;
						continue;
					}

					const int32_t indent = get_indent(lines[index]);
					if (indent == 0) {
						break;
					}

					min_indent = std::min(min_indent, indent);
					code_lines.push_back(lines[index]);
					index++;
				}

				if (code_lines.is_empty()) {
					if (p_owner != nullptr) {
						p_owner->emit_signal("render_warning", "Literal block marker has no indented content.", code_line);
					}
					continue;
				}

				RSTBlock code_block;
				code_block.type = RST_BLOCK_CODE;
				code_block.line = code_line;
				code_block.text = strip_code_indent(code_lines, min_indent == INT_MAX ? 0 : min_indent);
				code_block.anchor = pending_anchor;
				pending_anchor = String();
				blocks.push_back(code_block);
				continue;
			}

			if (!paragraph.text.is_empty()) {
				paragraph.anchor = pending_anchor;
				pending_anchor = String();
				blocks.push_back(paragraph);
			}
		}

		return blocks;
	}

