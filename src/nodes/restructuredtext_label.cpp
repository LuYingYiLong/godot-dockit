#include "restructuredtext_label.h"

#include "formats/rst/rst_document_parser.h"
#include "formats/rst/rst_reference_index.h"
#include "formats/html/resources/html_resource_paths.h"
#include "formats/html/text/html_font_config.h"
#include "formats/html/paint/html_paint_run.h"
#include "formats/word_boundaries.h"
#include "utils/style_box_css_helper.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
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
#include <godot_cpp/variant/packed_float32_array.hpp>
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


	static String utf8_slice(const std::string& p_text, size_t p_from, size_t p_length) {
		if (p_length == 0) {
			return String();
		}

		return String::utf8(p_text.data() + p_from, static_cast<int64_t>(p_length));
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

	static BitField<TextServer::LineBreakFlag> get_autowrap_trim_mask() {
		return TextServer::BREAK_TRIM_INDENT | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
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

	static void append_inline_span(std::vector<RSTInlineSpan>& r_spans, String& r_text, const String& p_text, bool p_bold = false, bool p_italic = false, bool p_code = false, const String& p_link_uri = String(), bool p_custom_color = false, const Color& p_color = Color(), const String& p_footnote_id = String(), bool p_footnote_reference = false) {
		if (p_text.is_empty()) {
			return;
		}

		if (!r_spans.empty()) {
			RSTInlineSpan& previous = r_spans.back();
			if (previous.bold == p_bold && previous.italic == p_italic && previous.code == p_code && previous.link_uri == p_link_uri && previous.footnote_id == p_footnote_id && previous.footnote_reference == p_footnote_reference && previous.custom_color == p_custom_color && (!p_custom_color || previous.color == p_color) && previous.end == r_text.length()) {
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
		span.footnote_id = p_footnote_id;
		span.footnote_reference = p_footnote_reference;
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
				append_inline_span(r_spans, output, format_footnote_label(footnote_label, p_footnotes), false, false, false, String(), false, Color(), footnote_label, true);
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
		if (p_owner != nullptr && p_owner->has_theme_constant_override(p_name)) {
			return p_owner->get_theme_constant(p_name, p_type);
		}
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
		if (p_owner != nullptr && p_owner->has_theme_color_override(p_name)) {
			return p_owner->get_theme_color(p_name, p_type);
		}
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

	static Color get_theme_stylebox_color_or(Control* p_owner, const StringName& p_type, const StringName& p_name,
		const Color& p_default) {
		const Ref<StyleBox> stylebox = get_theme_stylebox_or(p_owner, p_type, p_name);
		const auto* flat = stylebox.is_valid() ? Object::cast_to<StyleBoxFlat>(stylebox.ptr()) : nullptr;
		return flat == nullptr ? p_default : flat->get_bg_color();
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
	layout_dirty = true;
	anchor_offsets.clear();
	set_custom_minimum_size(Vector2(1.0, content_height));
	update_minimum_size();
	queue_redraw();
}

void RSTDocumentCanvas::mark_layout_dirty() {
	update_html_scrollbar_hover(Ref<HTMLLayout>());
	if (html_scrollbar_drag) {
		html_scrollbar_drag->layout->set_scrollbar_pressed(std::nullopt);
		html_scrollbar_drag.reset();
	}
	layout_dirty = true;
	queue_redraw();
}

void RSTDocumentCanvas::update_html_scrollbar_hover(const Ref<HTMLLayout>& p_layout, const Vector2& p_point) {
	const bool was_hovered = html_scrollbar_hovered;
	if (hovered_html_scrollbar_layout != p_layout) {
		if (hovered_html_scrollbar_layout.is_valid()) hovered_html_scrollbar_layout->update_scrollbar_hover(std::nullopt);
		hovered_html_scrollbar_layout = p_layout;
	}
	html_scrollbar_hovered = false;
	if (p_layout.is_valid()) {
		p_layout->update_scrollbar_hover(p_point);
		html_scrollbar_hovered = p_layout->scrollbar_at(p_point).has_value();
	}
	if (was_hovered || html_scrollbar_hovered) queue_redraw();
}

void RSTDocumentCanvas::scroll_html_input(const Vector2& p_position, const Ref<InputEvent>& p_event) {
	if (html_layout.is_null() || !Rect2(Vector2(), get_size()).has_point(p_position)) return;
	const dockit::HTMLScrollInput result = html_layout->scroll_overflow_from_input(p_position, p_event, *this,
		callable_mp(this, &RSTDocumentCanvas::queue_scroll_redraw));
	if (result == dockit::HTMLScrollInput::ignored) return;
	queue_redraw();
	if (result == dockit::HTMLScrollInput::handled) accept_event();
}

void RSTDocumentCanvas::queue_scroll_redraw() {
	queue_redraw();
}

bool RSTDocumentCanvas::has_selection() const {
	return selection_anchor != selection_caret;
}

bool RSTDocumentCanvas::is_position_inside_selection(const Vector2& p_position) {
	if (!has_selection()) return false;
	rebuild_layout();
	if (html_layout.is_null()) return false;
	const int64_t from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t to = std::max<int64_t>(selection_anchor, selection_caret);
	const TypedArray<Rect2> rects = html_layout->get_selection_rects(from, to);
	for (int64_t i = 0; i < rects.size(); ++i) {
		if (Rect2(rects[i]).has_point(p_position)) return true;
	}
	return false;
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
	if (from >= to || from < 0 || to > plain_text.length()) return String();
	if (html_layout.is_valid()) return html_layout->get_copyable_text(from, to);
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
	if (html_layout.is_null()) return 0;
	return html_layout->query_selector_all("th, td").size();
}

Rect2 RSTDocumentCanvas::get_table_cell_rect(int32_t p_index) {
	rebuild_layout();
	if (html_layout.is_null()) return Rect2();
	const TypedArray<HTMLElement> cells = html_layout->query_selector_all("th, td");
	if (p_index < 0 || p_index >= cells.size()) return Rect2();
	const Ref<HTMLElement> cell = cells[p_index];
	return cell.is_valid() ? cell->get_bounding_rect() : Rect2();
}

bool RSTDocumentCanvas::has_anchor(const String& p_anchor) const {
	return anchor_offsets.has(p_anchor);
}

PackedStringArray RSTDocumentCanvas::get_anchors() const {
	PackedStringArray result;
	const Array keys = anchor_offsets.keys();
	for (int32_t i = 0; i < keys.size(); ++i) result.append(keys[i]);
	return result;
}

float RSTDocumentCanvas::get_anchor_offset(const String& p_anchor) const {
	if (!anchor_offsets.has(p_anchor)) return -1.0f;
	return anchor_offsets[p_anchor];
}

float RSTDocumentCanvas::get_search_result_y() const {
	if (html_layout.is_null() || search_match_start < 0 || search_match_end <= search_match_start) return -1.0f;
	const TypedArray<Rect2> rects = html_layout->get_selection_rects(search_match_start, search_match_end);
	for (int64_t i = 0; i < rects.size(); ++i) {
		const Rect2 rect = rects[i];
		if (rect.has_area()) return rect.position.y;
	}
	return -1.0f;
}

Vector2 RSTDocumentCanvas::_get_minimum_size() const {
	return Vector2(1.0, content_height);
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

static String rst_html_escape(const String& p_text) {
	return p_text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;");
}

static String rst_css_color(const Color& p_color) {
	return String("#") + p_color.to_html(true);
}

static String rst_css_dimension(const String& p_dimension) {
	const String value = p_dimension.strip_edges();
	if (value.is_empty()) {
		return String();
	}
	const String lower = value.to_lower();
	if (lower.ends_with("%") || lower.ends_with("px") || lower.ends_with("em") || lower.ends_with("rem") ||
		lower.ends_with("pt") || lower.ends_with("cm") || lower.ends_with("mm") || lower.ends_with("in") ||
		lower.ends_with("vw") || lower.ends_with("vh")) {
		return value;
	}
	return value + String("px");
}

static String rst_stylebox_rule(const String& p_selector, const Ref<StyleBox>& p_stylebox, Dictionary& r_textures) {
	if (p_stylebox.is_null()) {
		return String();
	}
	const Dictionary converted = dockit::StyleBoxCSSHelper::stylebox_to_css(p_stylebox);
	if (int(converted.get("error", FAILED)) != OK) {
		return String();
	}
	r_textures.merge(converted.get("textures", Dictionary()), true);
	return p_selector + String("{") + String(converted.get("css", "")) + String("}");
}

static String rst_render_inline_html(const String& p_source, const RestructuredtextLabel* p_owner,
	const RSTFootnoteMap* p_footnotes, int32_t& r_footnote_reference_index) {
	std::vector<RSTInlineSpan> spans;
	parse_inline_spans(p_source, p_owner, spans, p_footnotes);
	String html;
	for (const RSTInlineSpan& span : spans) {
		String text = rst_html_escape(span.text).replace("\n", "<br>");
		if (span.footnote_reference) {
			const String id = "rst-footnote-ref-" + rst_html_escape(span.footnote_id) + String("-") + String::num_int64(r_footnote_reference_index++);
			text = String("<a class=\"rst-footnote-ref\" id=\"") + id + String("\" href=\"#rst-footnote-def-") +
				rst_html_escape(span.footnote_id) + String("\">") + text + String("</a>");
		}
		else if (!span.link_uri.is_empty()) {
			text = String("<a href=\"") + rst_html_escape(span.link_uri) + String("\">") + text + String("</a>");
		}
		if (span.code) {
			text = "<code>" + text + String("</code>");
		}
		if (span.bold) {
			text = "<strong>" + text + String("</strong>");
		}
		if (span.italic) {
			text = "<em>" + text + String("</em>");
		}
		if (span.custom_color) {
			text = String("<span style=\"color:") + rst_css_color(span.color) + String("\">") + text + String("</span>");
		}
		html += text;
	}
	return html;
}

static String rst_render_document_html(const std::vector<RSTBlock>& p_blocks, const RestructuredtextLabel* p_owner,
	const RSTFootnoteMap* p_footnotes) {
	String html = "<!doctype html><html><head><meta charset=\"utf-8\"></head><body>";
	int32_t footnote_reference_index = 0;
	for (const RSTBlock& block : p_blocks) {
		const String anchor = block.anchor.is_empty() ? String() : String(" id=\"") + rst_html_escape(block.anchor) + String("\"");
		String block_html;
		switch (block.type) {
		case RST_BLOCK_HEADING: {
			const String tag = String("h") + String::num_int64(CLAMP(block.level, 1, 5));
			block_html = String("<") + tag + anchor + String(">") + rst_render_inline_html(block.text, p_owner, p_footnotes, footnote_reference_index) + String("</") + tag + String(">");
		} break;
		case RST_BLOCK_BULLET_LIST:
		case RST_BLOCK_ENUMERATED_LIST: {
			const String tag = block.type == RST_BLOCK_ENUMERATED_LIST ? "ol" : "ul";
			block_html = String("<") + tag + String(" class=\"rst-list\"") + anchor + String(">");
			for (int32_t i = 0; i < block.items.size(); ++i) {
				block_html += "<li>" + rst_render_inline_html(block.items[i], p_owner, p_footnotes, footnote_reference_index) + String("</li>");
			}
			block_html += "</" + tag + String(">");
		} break;
		case RST_BLOCK_CODE:
			block_html = String("<pre class=\"rst-code-block\"") + anchor + String("><code class=\"language-") + rst_html_escape(block.argument) + String("\">") +
				rst_html_escape(block.text) + String("</code></pre>");
			break;
		case RST_BLOCK_ADMONITION: {
			const String directive = block.directive.to_lower();
			const String title = block.argument.is_empty() ? directive.capitalize() : block.argument;
			block_html = String("<aside class=\"rst-admonition rst-admonition-") + rst_html_escape(directive) + String("\"") + anchor + String(">");
			block_html += String("<div class=\"rst-admonition-title\">") + rst_render_inline_html(title, p_owner, p_footnotes, footnote_reference_index) + String("</div>");
			block_html += String("<div class=\"rst-admonition-body\">") + rst_render_inline_html(block.text, p_owner, p_footnotes, footnote_reference_index) + String("</div></aside>");
		} break;
		case RST_BLOCK_TABLE: {
			block_html = String("<table class=\"rst-table\"") + anchor + String("><tbody>");
			for (int32_t row_index = 0; row_index < static_cast<int32_t>(block.rows.size()); ++row_index) {
				block_html += "<tr>";
				const PackedStringArray row = block.rows[row_index];
				for (int32_t column = 0; column < block.columns; ++column) {
					const bool header = row_index < block.header_rows;
					const String tag = header ? "th" : "td";
					const String cell = column < row.size() ? row[column] : String();
					block_html += String("<") + tag + String(">") + rst_render_inline_html(cell, p_owner, p_footnotes, footnote_reference_index) + String("</") + tag + String(">");
				}
				block_html += "</tr>";
			}
			block_html += "</tbody></table>";
		} break;
		case RST_BLOCK_IMAGE: {
			const String src = p_owner == nullptr ? block.argument : p_owner->resolve_document_path(block.argument);
			if (!src.is_empty()) {
				String alignment = block.image_align.to_lower();
				if (alignment != "left" && alignment != "right") alignment = "center";
				block_html = String("<figure class=\"rst-text-block rst-align-") + alignment + String("\"") + anchor + String("><img src=\"") + rst_html_escape(src) + String("\" alt=\"") + rst_html_escape(block.image_alt) + String("\"");
				const String width = rst_css_dimension(block.image_width);
				const String height = rst_css_dimension(block.image_height);
				String image_style;
				if (!width.is_empty()) image_style += String("width:") + width + String(";");
				if (!height.is_empty()) image_style += String("height:") + height + String(";");
				if (block.image_width.is_empty() && block.image_height.is_empty() && !block.image_scale.is_empty()) {
					image_style += String("width:") + String::num_real(parse_scale_percent(block.image_scale)) + String("%;");
				}
				if (!image_style.is_empty()) block_html += String(" style=\"") + image_style + String("\"");
				block_html += ">";
				if (!block.text.is_empty()) block_html += "<figcaption>" + rst_render_inline_html(block.text, p_owner, p_footnotes, footnote_reference_index) + String("</figcaption>");
				block_html += "</figure>";
			}
		} break;
		case RST_BLOCK_TOCTREE:
			block_html = String("<nav class=\"rst-toctree rst-text-block\"") + anchor + String("><strong>") + rst_html_escape(block.argument) + String("</strong><ul>");
			for (int32_t i = 0; i < block.items.size(); ++i) {
				const String item = block.items[i];
				const String uri = p_owner == nullptr ? String("doc:") + item : p_owner->resolve_link_uri("doc", item);
				block_html += String("<li><a href=\"") + rst_html_escape(uri) + String("\">") + rst_html_escape(item) + String("</a></li>");
			}
			block_html += "</ul></nav>";
			break;
		case RST_BLOCK_TRANSITION:
			block_html = String("<hr class=\"rst-text-block\"") + anchor + String(">");
			break;
		case RST_BLOCK_PARAGRAPH:
		default:
			if (!block.footnote_label.is_empty()) {
				const String id = rst_html_escape(block.footnote_label);
				const String prefix = format_footnote_label(block.footnote_label, p_footnotes) + String(".");
				String body = block.text;
				if (body.begins_with(prefix)) body = body.substr(prefix.length()).strip_edges();
				const String back = String("<a class=\"rst-footnote-backref\" href=\"#rst-footnote-ref-") + id + String("-0\">") + rst_html_escape(prefix) + String("</a> ");
				block_html = String("<p class=\"rst-footnote\" id=\"rst-footnote-def-") + id + String("\"") + anchor + String(">") + back + rst_render_inline_html(body, p_owner, p_footnotes, footnote_reference_index) + String("</p>");
			}
			else {
				block_html = String("<p class=\"rst-text-block\"") + anchor + String(">") + rst_render_inline_html(block.text, p_owner, p_footnotes, footnote_reference_index) + String("</p>");
			}
			break;
		}
		if (!block_html.is_empty()) html += block_html;
	}
	return html + String("</body></html>");
}

static String rst_autowrap_css(const RestructuredtextLabel* p_owner) {
	const TextServer::AutowrapMode mode = p_owner == nullptr ? TextServer::AUTOWRAP_WORD_SMART : p_owner->get_autowrap_mode();
	switch (mode) {
	case TextServer::AUTOWRAP_OFF:
		return "white-space:nowrap;";
	case TextServer::AUTOWRAP_ARBITRARY:
		return "word-break:break-all;";
	case TextServer::AUTOWRAP_WORD:
		return "overflow-wrap:normal;";
	case TextServer::AUTOWRAP_WORD_SMART:
	default:
		return "overflow-wrap:break-word;";
	}
}

static String rst_theme_stylesheet(RestructuredtextLabel* p_owner, Dictionary& r_textures,
	dockit::HTMLFontConfig& r_fonts) {
	const int32_t extra = p_owner == nullptr ? 0 : p_owner->get_extra_font_size();
	const int32_t body_base = get_theme_font_size_or(p_owner, "RestructuredtextLabel", "normal_font_size", 16);
	const int32_t body_size = std::max<int32_t>(1, body_base + extra);
	const Color body_color = get_theme_color_or(p_owner, "RestructuredtextLabel", "font_color", Color(1, 1, 1, 1));
	const Ref<Font> body_font = get_theme_font_or(p_owner, "RestructuredtextLabel", "normal_font");
	const Ref<Font> bold_font = get_theme_font_or(p_owner, "RestructuredtextLabel", "bold_font");
	const Ref<Font> italic_font = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "italics_font"), get_theme_font_or(p_owner, "RestructuredtextLabel", "italic_font"), body_font);
	const Ref<Font> bold_italic_font = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "bold_italics_font"), get_theme_font_or(p_owner, "RestructuredtextLabel", "bold_italic_font"), bold_font);
	const Ref<Font> inline_font = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "inline_code_font"), Ref<Font>(), body_font);
	const Ref<Font> code_font = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "code_block_font"), inline_font, body_font);
	const Ref<Font> marker_font = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "list_marker_font"), Ref<Font>(), body_font);
	r_fonts.text = body_font;
	r_fonts.bold = bold_font;
	r_fonts.italic = italic_font;
	r_fonts.bold_italic = bold_italic_font;
	r_fonts.code = inline_font;
	r_fonts.code_block = code_font;
	r_fonts.named_fonts["rst-body"] = body_font;
	r_fonts.named_fonts["rst-inline-code"] = inline_font;
	r_fonts.named_fonts["rst-code"] = code_font;
	r_fonts.named_fonts["rst-list-marker"] = marker_font;
	r_fonts.named_fonts["rst-table"] = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "table_font"), Ref<Font>(), body_font);
	r_fonts.named_fonts["rst-table-header"] = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "table_header_font"), Ref<Font>(), body_font);
	r_fonts.named_fonts["rst-footnote"] = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "footnote_text_font"), Ref<Font>(), body_font);
	r_fonts.named_fonts["rst-footnote-ref"] = first_valid_font(get_theme_font_or(p_owner, "RestructuredtextLabel", "footnote_ref_font"), Ref<Font>(), body_font);
	const int32_t body_line_height = std::max<int32_t>(1, body_size + get_theme_constant_or(p_owner, "RestructuredtextLabel", "line_separation", 2));
	const double line_height_ratio = static_cast<double>(body_line_height) / static_cast<double>(body_size);
	String css = "html,body{margin:0;padding:0;background:transparent;} body{font-family:rst-body;font-size:" + String::num_int64(body_size) + String("px;color:") + rst_css_color(body_color) + String(";line-height:") + String::num_real(line_height_ratio, false) + String(";") + rst_autowrap_css(p_owner) + String("}");
	css += "p,ul,ol,pre,table,figure,nav.rst-toctree,hr,.rst-admonition{margin:0;}";
	const int32_t paragraph_separation = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "paragraph_separation", 10));
	const int32_t text_space_before = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "text_space_before", 0));
	const int32_t text_space_after = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "text_space_after", paragraph_separation));
	const int32_t list_space_before = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "list_space_before", 0));
	const int32_t list_space_after = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "list_space_after", paragraph_separation));
	const int32_t code_space_before = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "code_block_space_before", 0));
	const int32_t code_space_after = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "code_block_space_after", paragraph_separation));
	const int32_t table_space_before = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "table_space_before", 0));
	const int32_t table_space_after = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "table_space_after", paragraph_separation));
	css += String(".rst-text-block{margin-top:") + String::num_int64(text_space_before) + String("px;margin-bottom:") + String::num_int64(text_space_after) + String("px;}");
	css += String(".rst-list{margin-top:") + String::num_int64(list_space_before) + String("px;margin-bottom:") + String::num_int64(list_space_after) + String("px;}");
	css += String(".rst-code-block{margin-top:") + String::num_int64(code_space_before) + String("px;margin-bottom:") + String::num_int64(code_space_after) + String("px;}");
	css += String("table.rst-table{margin-top:") + String::num_int64(table_space_before) + String("px;margin-bottom:") + String::num_int64(table_space_after) + String("px;}");
	css += "a:link{color:" + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "link_color", Color(0.36, 0.62, 1.0, 1.0))) + String(";text-decoration:underline;}");
	const int32_t marker_size = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "list_marker_font_size", 16) + extra);
	const int32_t marker_gap = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "list_marker_gap", 6));
	const int32_t marker_min_width = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "list_marker_min_width", 14));
	const int32_t list_indent = std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", "list_indent", 26));
	css += String("ul.rst-list,ol.rst-list{padding-left:") + String::num_int64(std::max<int32_t>(list_indent, marker_min_width + marker_gap)) + String("px;}li{margin:0;}li::marker{font-family:rst-list-marker;font-size:") + String::num_int64(marker_size) + String("px;color:") + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "list_marker_color", body_color)) + String(";margin-right:") + String::num_int64(marker_gap) + String("px;}");
	css += "pre{white-space:pre;tab-size:4;overflow-x:auto;overflow-y:hidden;scrollbar-width:thin;font-family:rst-code;font-size:" + String::num_int64(std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "code_block_font_size", 16) + extra)) + String("px;color:") + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "code_block_font_color", Color(0.86, 0.88, 0.91, 1.0))) + String(";}pre code{font:inherit;color:inherit;white-space:inherit;background:none;border:0;padding:0;}");
	css += "code{font-family:rst-inline-code;font-size:" + String::num_int64(std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "inline_code_font_size", 16) + extra)) + String("px;color:") + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "inline_code_font_color", Color(0.86, 0.88, 0.91, 1.0))) + String(";white-space:pre-wrap;}");
	Ref<StyleBox> inline_code = get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "inline_code_style");
	if (!inline_code.is_valid()) inline_code = create_internal_stylebox(Color(0.16, 0.18, 0.20, 1.0), Color(0, 0, 0, 0), 0, 0, 0, 0, 2.0f, 1.0f, 2.0f, 1.0f);
	Ref<StyleBox> code_block = get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "code_block_panel");
	if (!code_block.is_valid()) code_block = create_internal_stylebox(Color(0.08, 0.09, 0.11, 1.0), Color(0, 0, 0, 0), 0, 0, 0, 0, 8.0f, 8.0f, 8.0f, 8.0f);
	css += rst_stylebox_rule("code", inline_code, r_textures) + rst_stylebox_rule("pre", code_block, r_textures);
	css += "table.rst-table{width:100%;border-collapse:separate;border-spacing:0;}th,td{font-family:rst-table;font-size:" + String::num_int64(std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "table_font_size", 16) + extra)) + String("px;color:") + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "table_font_color", body_color)) + String(";padding:4px 6px;text-align:left;vertical-align:top;}");
	css += "th{font-family:rst-table-header;font-size:" + String::num_int64(std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "table_header_font_size", get_theme_font_size_or(p_owner, "RestructuredtextLabel", "table_font_size", 16)) + extra)) + String("px;color:") + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "table_header_font_color", get_theme_color_or(p_owner, "RestructuredtextLabel", "table_font_color", body_color))) + String(";}");
	css += rst_stylebox_rule("table.rst-table", get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "table_panel"), r_textures);
	css += rst_stylebox_rule("td", get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "table_cell_panel"), r_textures);
	css += rst_stylebox_rule("th", get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "table_header_panel"), r_textures);
	if (get_theme_constant_or(p_owner, "RestructuredtextLabel", "table_striped", 0) != 0) css += rst_stylebox_rule("tr:nth-child(even) td", get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "table_striped_panel"), r_textures);
	css += "hr{border:0;height:1px;background:" + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "font_separator_color", Color(0.45, 0.47, 0.52, 1.0))) + String(";}");
	css += "figure{margin-left:0;margin-right:0;text-align:center;}figure.rst-align-left{text-align:left;}figure.rst-align-right{text-align:right;}figure img{max-width:100%;}figcaption{text-align:inherit;font-style:italic;}";
	const String footnote_color = rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "footnote_ref_font_color", Color(0.809f, 0.809f, 0.809f, 1.0f)));
	const int32_t footnote_size = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "footnote_ref_font_size", std::max<int32_t>(1, body_base - body_base / 4)) + extra);
	css += ".rst-footnote-ref{font-family:rst-footnote-ref;font-size:" + String::num_int64(footnote_size) + String("px;color:") + footnote_color + String(";vertical-align:super;}");
	const String footnote_text_color = rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", "footnote_text_font_color", body_color));
	css += ".rst-footnote{font-family:rst-footnote;font-size:" + String::num_int64(std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", "footnote_text_font_size", body_base) + extra)) + String("px;color:") + footnote_text_color + String(";}");
	css += ".rst-footnote .rst-footnote-backref{color:" + footnote_text_color + String(";}");
	css += rst_stylebox_rule(".rst-footnote-ref", get_theme_stylebox_or(p_owner, "RestructuredtextLabel", "footnote_ref"), r_textures);
	for (int32_t level = 1; level <= 5; ++level) {
		const String tag = String("h") + String::num_int64(level);
		const int32_t fallback[] = { 28, 24, 21, 19, 17 };
		const String prefix = String("heading_") + String::num_int64(level) + String("_");
		css += tag + String("{font-family:rst-heading-") + String::num_int64(level) + String(";font-size:") + String::num_int64(std::max<int32_t>(1, get_theme_font_size_or(p_owner, "RestructuredtextLabel", StringName(prefix + String("font_size")), fallback[level - 1]) + extra)) + String("px;color:") + rst_css_color(get_theme_color_or(p_owner, "RestructuredtextLabel", StringName(prefix + String("font_color")), body_color)) + String(";margin-top:") + String::num_int64(std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", StringName(prefix + String("space_before")), 0))) + String("px;margin-bottom:") + String::num_int64(std::max<int32_t>(0, get_theme_constant_or(p_owner, "RestructuredtextLabel", StringName(prefix + String("space_after")), get_theme_constant_or(p_owner, "RestructuredtextLabel", "paragraph_separation", 10)))) + String("px;}");
		r_fonts.named_fonts[String("rst-heading-") + String::num_int64(level)] = get_theme_font_or(p_owner, "RestructuredtextLabel", StringName(prefix + String("font")));
	}
	const PackedStringArray directives = PackedStringArray({ "admonition", "attention", "caution", "danger", "error", "hint", "important", "note", "tip", "seealso", "warning" });
	for (int32_t i = 0; i < directives.size(); ++i) {
		const String directive = directives[i];
		const String item_directive = directive == "admonition" ? "custom" : directive;
		const String selector = String(".rst-admonition-") + directive;
		const int32_t size = std::max<int32_t>(1, get_admonition_theme_font_size_or(p_owner, item_directive, "font_size", body_base) + extra);
		const Color text_color = get_admonition_theme_color_or(p_owner, item_directive, "font_color", body_color);
		const Color header_color = get_admonition_theme_color_or(p_owner, item_directive, "header_font_color", Color(1, 1, 1, 1));
		const Color accent = get_admonition_theme_color_or(p_owner, item_directive, "accent_color", default_admonition_accent(directive));
		Ref<StyleBox> panel = get_admonition_theme_stylebox_or(p_owner, item_directive, "panel");
		Ref<StyleBox> header = get_admonition_theme_stylebox_or(p_owner, item_directive, "header_panel");
		if (!panel.is_valid()) panel = create_internal_stylebox(Color(0.10, 0.12, 0.14, 1.0), accent, 4, 1, 1, 1, 12.0f, 8.0f, 8.0f, 8.0f);
		if (!header.is_valid()) header = create_internal_stylebox(accent, accent, 4, 1, 1, 0, 12.0f, 6.0f, 8.0f, 6.0f);
		css += selector + String("{font-size:") + String::num_int64(size) + String("px;color:") + rst_css_color(text_color) + String(";margin-top:") + String::num_int64(std::max<int32_t>(0, get_admonition_theme_constant_or(p_owner, item_directive, "space_before", 0))) + String("px;margin-bottom:") + String::num_int64(std::max<int32_t>(0, get_admonition_theme_constant_or(p_owner, item_directive, "space_after", get_theme_constant_or(p_owner, "RestructuredtextLabel", "paragraph_separation", 10)))) + String("px;}");
		css += selector + String(">.rst-admonition-title{font-weight:bold;color:") + rst_css_color(header_color) + String(";}");
		css += rst_stylebox_rule(selector, panel, r_textures) + rst_stylebox_rule(selector + String(">.rst-admonition-title"), header, r_textures);
	}
	return css;
}

void RSTDocumentCanvas::rebuild_layout() {
	const float width = std::max<float>(1.0f, get_size().x);
	if (!layout_dirty && std::abs(width - layout_width) < 0.01f) {
		return;
	}

	layout_dirty = false;
	layout_width = width;
	anchor_offsets.clear();
	const RSTFootnoteMap footnotes = build_footnote_map(split_lines(rst_text));
	const std::vector<RSTBlock> blocks = parse_rst_blocks(rst_text, document_view, &footnotes);
	rendered_block_count = static_cast<int32_t>(blocks.size());
	const String html = rst_render_document_html(blocks, document_view, &footnotes);
	Dictionary textures;
	dockit::HTMLFontConfig fonts;
	const String default_css = rst_theme_stylesheet(document_view, textures, fonts);
	String author_css;
	Dictionary stylesheet_resources;
	if (document_view != nullptr && document_view->get_stylesheet().is_valid()) {
		const Ref<CSSStyleSheet> author_stylesheet = document_view->get_stylesheet();
		const String source_path = author_stylesheet->get_source_path();
		if (source_path.is_empty()) {
			author_css = author_stylesheet->get_css_text();
		}
		else {
			stylesheet_resources[source_path] = author_stylesheet;
			author_css = String("@import url(\"") + source_path.replace("\\", "\\\\").replace("\"", "\\\"") + "\");";
		}
	}
	if (html_layout.is_null()) {
		html_layout.instantiate();
	}
	const String base_dir = document_view == nullptr ? String("res://") : document_view->get_resource_base_dir();
	const String source_path = document_view != nullptr && document_view->get_document().is_valid()
		? document_view->get_document()->get_source_path() : String();
	const Error parse_error = html_layout->parse_source(html, base_dir, author_css, textures, source_path, true,
		default_css, fonts, stylesheet_resources);
	if (parse_error != OK) {
		plain_text = String();
		content_height = 1.0f;
		set_custom_minimum_size(Vector2(1.0f, content_height));
		update_minimum_size();
		queue_redraw();
		return;
	}
	const int32_t viewport_height = std::max<int32_t>(1, static_cast<int32_t>(std::ceil(get_size().y)));
	const Error layout_error = html_layout->layout(Vector2i(std::max<int32_t>(1, static_cast<int32_t>(std::ceil(width))), viewport_height));
	if (layout_error != OK) {
		plain_text = String();
		content_height = 1.0f;
		set_custom_minimum_size(Vector2(1.0f, content_height));
		update_minimum_size();
		queue_redraw();
		return;
	}
	const Color scrollbar_track = get_theme_stylebox_color_or(document_view, "VScrollBar", "scroll", Color(0.1f, 0.1f, 0.1f, 0.6f));
	const Color scrollbar_thumb = get_theme_stylebox_color_or(document_view, "VScrollBar", "grabber", Color(1.0f, 1.0f, 1.0f, 0.4f));
	const Color scrollbar_hover = get_theme_stylebox_color_or(document_view, "VScrollBar", "grabber_highlight", Color(1.0f, 1.0f, 1.0f, 0.75f));
	const Color scrollbar_pressed = get_theme_stylebox_color_or(document_view, "VScrollBar", "grabber_pressed", Color(0.75f, 0.75f, 0.75f, 0.75f));
	html_layout->set_scrollbar_theme(scrollbar_track, scrollbar_thumb, scrollbar_hover, scrollbar_pressed);

	plain_text = html_layout->get_plain_text();
	content_height = std::max<int32_t>(1, html_layout->get_content_size().y);
	selection_anchor = std::min<int64_t>(selection_anchor, plain_text.length());
	selection_caret = std::min<int64_t>(selection_caret, plain_text.length());
	if (search_match_end > plain_text.length() || search_match_end <= search_match_start) {
		search_match_start = -1;
		search_match_end = -1;
	}
	for (const auto& anchor : html_layout->get_anchors()) {
		anchor_offsets[anchor.key] = anchor.value.bounds.position.y;
	}
	set_custom_minimum_size(Vector2(1.0f, content_height));
	update_minimum_size();
	queue_redraw();
}

void RSTDocumentCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_RESIZED || p_what == NOTIFICATION_THEME_CHANGED) mark_layout_dirty();
	if (p_what == NOTIFICATION_MOUSE_EXIT) {
		update_html_scrollbar_hover(Ref<HTMLLayout>());
		if (!html_scrollbar_drag) set_default_cursor_shape(CURSOR_ARROW);
	}
	if (p_what == NOTIFICATION_EXIT_TREE || (p_what == NOTIFICATION_VISIBILITY_CHANGED && !is_visible_in_tree())) {
		update_html_scrollbar_hover(Ref<HTMLLayout>());
		if (html_scrollbar_drag) {
			html_scrollbar_drag->layout->set_scrollbar_pressed(std::nullopt);
			html_scrollbar_drag.reset();
		}
	}
	if (p_what == NOTIFICATION_FOCUS_EXIT) {
		if (document_view != nullptr && document_view->is_deselect_on_focus_loss_enabled()) clear_selection();
		selection_drag_attempt = false;
		if (html_scrollbar_drag) {
			html_scrollbar_drag->layout->set_scrollbar_pressed(std::nullopt);
			html_scrollbar_drag.reset();
		}
	}
	if (p_what == NOTIFICATION_DRAG_END) selection_drag_attempt = false;
	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		if (!dragging_selection || !selection_dragged || document_view == nullptr) return;
		VScrollBar* v_scroll = document_view->get_v_scroll_bar();
		if (v_scroll == nullptr) return;
		const Size2 viewer_size = document_view->get_size();
		const Vector2 viewer_mouse = document_view->get_local_mouse_position();
		const float scroll_margin = 24.0f;
		float scroll_delta = 0.0f;
		const float above = scroll_margin - viewer_mouse.y;
		const float below = viewer_mouse.y - (viewer_size.y - scroll_margin);
		if (above > 0.0f) scroll_delta = -80.0f * (above / scroll_margin) * static_cast<float>(get_process_delta_time());
		else if (below > 0.0f) scroll_delta = 80.0f * (below / scroll_margin) * static_cast<float>(get_process_delta_time());
		if (scroll_delta != 0.0f) {
			const float max_scroll = v_scroll->get_max() - v_scroll->get_page();
			v_scroll->set_value(std::clamp<float>(v_scroll->get_value() + scroll_delta, 0.0f, max_scroll));
			selection_caret = hit_test_document(get_local_mouse_position());
			queue_redraw();
		}
		return;
	}
	if (p_what != NOTIFICATION_DRAW) return;
	rebuild_layout();
	if (html_layout.is_null()) return;
	dockit::HTMLTextRenderEffects effects;
	effects.outline_size = std::clamp<int32_t>(get_theme_constant_or(document_view, "RestructuredtextLabel", "outline_size", 0), 0, 16);
	effects.outline_color = get_theme_color_or(document_view, "RestructuredtextLabel", "font_outline_color", Color(0, 0, 0, 1));
	effects.shadow_outline_size = std::clamp<int32_t>(get_theme_constant_or(document_view, "RestructuredtextLabel", "shadow_outline_size", 1), 0, 16);
	effects.shadow_offset = Vector2(get_theme_constant_or(document_view, "RestructuredtextLabel", "shadow_offset_x", 1), get_theme_constant_or(document_view, "RestructuredtextLabel", "shadow_offset_y", 1));
	effects.shadow_color = get_theme_color_or(document_view, "RestructuredtextLabel", "font_shadow_color", Color(0, 0, 0, 0));
	const Color selection_color = get_theme_color_or(document_view, "RestructuredtextLabel", "selection_color", Color(0.22, 0.45, 0.95, 0.45));
	const Color search_color = get_theme_color_or(document_view, "RestructuredtextLabel", "search_result_color", Color(1.0, 0.72, 0.16, 0.42));
	const Ref<StyleBox> search_style = create_internal_stylebox(search_color, Color(0, 0, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0);
	const Vector2i selection_range(std::min<int64_t>(selection_anchor, selection_caret), std::max<int64_t>(selection_anchor, selection_caret));
	const Vector2i search_range(search_match_start, search_match_end);
	html_layout->draw_with_text_effects(this, Vector2(), Rect2(Vector2(), get_size()), selection_range,
		nullptr, selection_color, search_range, search_style.ptr(), Vector2(), effects);
}

int64_t RSTDocumentCanvas::hit_test_document(const Vector2& p_position) const {
	return html_layout.is_valid() ? html_layout->hit_test_text(p_position) : 0;
}

String RSTDocumentCanvas::link_uri_at_position(const Vector2& p_position) const {
	return html_layout.is_valid() ? html_layout->get_link_at(p_position) : String();
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

float RSTDocumentCanvas::footnote_target_at_position(const Vector2& p_position) {
	rebuild_layout();
	if (html_layout.is_null()) return -1.0f;
	const String uri = html_layout->get_link_at(p_position);
	const int64_t fragment_index = uri.find("#");
	if (fragment_index < 0) return -1.0f;
	const Rect2 anchor = html_layout->get_anchor_rect(uri.substr(fragment_index + 1));
	return anchor.has_area() ? anchor.position.y : -1.0f;
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
	const Ref<InputEventPanGesture> pan = p_event;
	if (pan.is_valid()) {
		scroll_html_input(pan->get_position(), p_event);
		return;
	}
	if (mouse_button.is_valid() &&
		(mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_UP || mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN ||
			mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_LEFT || mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_RIGHT)) {
		scroll_html_input(mouse_button->get_position(), p_event);
		return;
	}

	if (mouse_button.is_valid() && mouse_button->get_button_index() == MOUSE_BUTTON_LEFT && !mouse_button->is_pressed() && html_scrollbar_drag) {
		const HTMLScrollbarGesture gesture = *html_scrollbar_drag;
		html_scrollbar_drag.reset();
		if (!mouse_button->is_canceled()) {
			gesture.layout->drag_scrollbar(gesture.drag, mouse_button->get_position() - gesture.origin);
			gesture.layout->finish_scrollbar_drag(gesture.drag);
		}
		gesture.layout->set_scrollbar_pressed(std::nullopt);
		update_html_scrollbar_hover(Rect2(Vector2(), get_size()).has_point(mouse_button->get_position()) ? html_layout : Ref<HTMLLayout>(), mouse_button->get_position());
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
			const Vector2 point = mouse_button->get_position();
			if (html_layout.is_valid()) {
				if (const auto drag = html_layout->scrollbar_at(point)) {
					html_scrollbar_drag = HTMLScrollbarGesture{ html_layout, *drag, point - point };
					html_layout->set_scrollbar_pressed(*drag);
					html_layout->drag_scrollbar(*drag, point);
					update_html_scrollbar_hover(html_layout, point);
					set_default_cursor_shape(CURSOR_ARROW);
					accept_event();
					return;
				}
			}
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
					if (html_layout.is_valid()) text_hit = html_layout->hit_test_selectable_text(press_position);
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
				const Vector2 release_position = mouse_button->get_position();
				const String uri = link_uri_at_position(release_position);
				if (!uri.is_empty() && document_view != nullptr) {
					document_view->activate_link_uri(uri);
				}
				else if (document_view != nullptr) {
					const float footnote_target = footnote_target_at_position(release_position);
					VScrollBar* v_scroll = document_view->get_v_scroll_bar();
					if (footnote_target >= 0.0f && v_scroll != nullptr) {
						const float max_scroll = std::max<float>(0.0f, v_scroll->get_max() - v_scroll->get_page());
						v_scroll->set_value(std::min<float>(std::max<float>(0.0f, footnote_target - 8.0f), max_scroll));
					}
				}
			}
			dragging_selection = false;
			selection_dragged = false;
			set_process_internal(false);
			// Restore cursor after interaction ends
			const Vector2 release_position = mouse_button->get_position();
			const String uri = link_uri_at_position(release_position);
			if (!uri.is_empty() || footnote_target_at_position(release_position) >= 0.0f) {
				set_default_cursor_shape(CURSOR_POINTING_HAND);
			}
			else {
				set_default_cursor_shape(CURSOR_ARROW);
			}
		}
		accept_event();
		return;
	}

	const Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid() && html_scrollbar_drag) {
		const HTMLScrollbarGesture& gesture = *html_scrollbar_drag;
		gesture.layout->drag_scrollbar(gesture.drag, mouse_motion->get_position() - gesture.origin);
		queue_redraw();
		accept_event();
		return;
	}
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
		const Vector2 mouse_position = mouse_motion->get_position();
		const bool in_canvas = Rect2(Vector2(), get_size()).has_point(mouse_position);
		update_html_scrollbar_hover(in_canvas ? html_layout : Ref<HTMLLayout>(), mouse_position);
		if (html_layout.is_valid() && html_layout->scrollbar_at(mouse_position)) {
			set_default_cursor_shape(CURSOR_ARROW);
			return;
		}
		const String uri = link_uri_at_position(mouse_position);
		if (!uri.is_empty() || footnote_target_at_position(mouse_position) >= 0.0f) {
			set_default_cursor_shape(CURSOR_POINTING_HAND);
		}
		else {
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
	ClassDB::bind_method(D_METHOD("set_stylesheet", "stylesheet"), &RestructuredtextLabel::set_stylesheet);
	ClassDB::bind_method(D_METHOD("get_stylesheet"), &RestructuredtextLabel::get_stylesheet);
	ClassDB::bind_method(D_METHOD("set_access", "access"), &RestructuredtextLabel::set_access);
	ClassDB::bind_method(D_METHOD("get_access"), &RestructuredtextLabel::get_access);
	ClassDB::bind_method(D_METHOD("set_root_subfolder", "root_subfolder"), &RestructuredtextLabel::set_root_subfolder);
	ClassDB::bind_method(D_METHOD("get_root_subfolder"), &RestructuredtextLabel::get_root_subfolder);
	ClassDB::bind_method(D_METHOD("get_resource_base_dir"), &RestructuredtextLabel::get_resource_base_dir);
	ClassDB::bind_method(D_METHOD("set_rst_text", "text"), &RestructuredtextLabel::set_rst_text);
	ClassDB::bind_method(D_METHOD("get_rst_text"), &RestructuredtextLabel::get_rst_text);
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
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stylesheet", PROPERTY_HINT_RESOURCE_TYPE, "CSSStyleSheet"), "set_stylesheet", "get_stylesheet");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "rst_text", PROPERTY_HINT_MULTILINE_TEXT), "set_rst_text", "get_rst_text");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "access", PROPERTY_HINT_ENUM, "Resources,User Data,File System"), "set_access", "get_access");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "root_subfolder", PROPERTY_HINT_DIR), "set_root_subfolder", "get_root_subfolder");
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
	const int32_t top = get_theme_constant_or(self, "RestructuredtextLabel", "margin_top", 16);
	const int32_t bottom = get_theme_constant_or(self, "RestructuredtextLabel", "margin_bottom", 16);
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

	const int32_t left = get_theme_constant_or(this, "RestructuredtextLabel", "margin_left", 16);
	const int32_t top = get_theme_constant_or(this, "RestructuredtextLabel", "margin_top", 16);
	const int32_t right = get_theme_constant_or(this, "RestructuredtextLabel", "margin_right", 16);
	const int32_t bottom = get_theme_constant_or(this, "RestructuredtextLabel", "margin_bottom", 16);

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

void RestructuredtextLabel::set_stylesheet(const Ref<CSSStyleSheet>& p_stylesheet) {
	if (stylesheet == p_stylesheet) return;
	const Callable changed = callable_mp(this, &RestructuredtextLabel::refresh);
	if (stylesheet.is_valid() && stylesheet->is_connected("changed", changed)) stylesheet->disconnect("changed", changed);
	stylesheet = p_stylesheet;
	if (stylesheet.is_valid()) stylesheet->connect("changed", changed);
	if (document_canvas != nullptr || is_inside_tree()) refresh();
}

Ref<CSSStyleSheet> RestructuredtextLabel::get_stylesheet() const {
	return stylesheet;
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

	const int64_t fragment_index = p_uri.find("#");
	if (fragment_index >= 0) {
		String target_path = p_uri.substr(0, fragment_index);
		String source_path;
		if (document.is_valid()) {
			source_path = document->get_source_path();
		}
		if (target_path.begins_with("uid://")) {
			target_path = ResourceUID::uid_to_path(target_path);
		}
		if (source_path.begins_with("uid://")) {
			source_path = ResourceUID::uid_to_path(source_path);
		}
		if (target_path.is_empty() || (document.is_valid() && localize_project_path(target_path) == localize_project_path(source_path))) {
			scroll_to_anchor(p_uri.substr(fragment_index + 1));
		}
	}
	emit_signal("link_pressed", p_uri);
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
	}
	else {
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
