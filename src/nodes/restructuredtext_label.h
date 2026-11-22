#pragma once

#include "formats/rst/rst_document.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/text_paragraph.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <vector>

using namespace godot;

class RestructuredtextLabel;

enum RSTCanvasItemType {
	RST_CANVAS_ITEM_TEXT,
	RST_CANVAS_ITEM_IMAGE,
	RST_CANVAS_ITEM_TABLE,
};

struct RSTInlineSpan {
	String text;
	String link_uri;
	int64_t start = 0;
	int64_t end = 0;
	Color color;
	bool bold = false;
	bool italic = false;
	bool code = false;
	bool custom_color = false;
};

struct RSTCanvasLine {
	int32_t item_index = -1;
	int32_t paragraph_line = -1;
	int64_t global_start = 0;
	int64_t global_end = 0;
	Vector2 position;
	Vector2 size;
};

struct RSTTableCell {
	Ref<TextParagraph> paragraph;
	Ref<StyleBox> stylebox;
	Rect2 rect;
	Rect2 text_rect;
	String text;
	std::vector<RSTInlineSpan> spans;
	int64_t global_start = 0;
	int64_t global_end = 0;
	float inline_code_padding_top = 0.0f;
	float inline_code_padding_bottom = 0.0f;
	Color text_color;
	bool header = false;
};

struct RSTListMarker {
	String text;
	Vector2 position;
	Ref<Font> font;
	int32_t font_size = 16;
	Color color;
};

struct RSTCanvasItem {
	RSTCanvasItemType type = RST_CANVAS_ITEM_TEXT;
	Ref<TextParagraph> paragraph;
	Ref<TextParagraph> header_paragraph;
	Ref<Texture2D> texture;
	Ref<StyleBox> panel_stylebox;
	Ref<StyleBox> header_stylebox;
	std::vector<RSTInlineSpan> spans;
	std::vector<RSTInlineSpan> header_spans;
	std::vector<RSTTableCell> cells;
	std::vector<RSTListMarker> list_markers;
	Rect2 rect;
	Rect2 text_rect;
	Rect2 header_text_rect;
	String text;
	String header_text;
	int64_t global_start = 0;
	int64_t global_end = 0;
	int64_t paragraph_global_start = 0;
	int64_t paragraph_global_end = 0;
	int64_t header_global_start = 0;
	int64_t header_global_end = 0;
	Color text_color;
	Color header_text_color;
	Color background_color;
	Color border_color;
	Color header_color;
	bool framed = false;
	bool code = false;
	bool heading = false;
	bool transition = false;
	int32_t header_height = 0;
	int32_t border_width = 0;
	String anchor;
};

class RSTDocumentCanvas : public Control {
	GDCLASS(RSTDocumentCanvas, Control);

private:
	RestructuredtextLabel* document_view = nullptr;
	String rst_text;
	String plain_text;
	std::vector<RSTCanvasItem> items;
	std::vector<RSTCanvasLine> lines;
	bool layout_dirty = true;
	bool dragging_selection = false;
	bool selection_dragged = false;
	bool selection_drag_attempt = false;
	PopupMenu* menu = nullptr;
	int64_t selection_anchor = 0;
	int64_t selection_caret = 0;
	int64_t search_match_start = -1;
	int64_t search_match_end = -1;
	Vector2 press_position;
	int32_t rendered_block_count = 0;
	float content_height = 1.0f;
	float layout_width = 0.0f;
	Dictionary anchor_offsets;

	void rebuild_layout();
	void draw_selection_for_item(const RSTCanvasItem& p_item);
	void draw_selection_for_paragraph(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, int64_t p_global_start, int64_t p_global_end);
	void draw_search_highlight_for_item(const RSTCanvasItem& p_item);
	void draw_range_background_for_paragraph(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, int64_t p_global_start, int64_t p_global_end, int64_t p_range_start, int64_t p_range_end, const Color& p_color);
	void draw_list_markers(const RSTCanvasItem& p_item);
	void draw_stylebox_or_rect(const Ref<StyleBox>& p_stylebox, const Rect2& p_rect, const Color& p_fallback_color);
	void draw_span_decorations(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, const std::vector<RSTInlineSpan>& p_spans, int64_t p_global_start, bool p_draw_code_backgrounds, bool p_draw_link_underlines);
	void draw_paragraph_text(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, const std::vector<RSTInlineSpan>& p_spans, int64_t p_global_start, const Color& p_default_color);
	void _generate_context_menu();
	void _update_context_menu();
	Key _get_menu_action_accelerator(const String& p_action);
	int64_t hit_test_document(const Vector2& p_position) const;
	String link_uri_at_position(const Vector2& p_position) const;
	String link_uri_at_character(int64_t p_character) const;
	int64_t line_column_to_offset(int32_t p_line, int32_t p_column) const;
	Vector2i offset_to_line_column(int64_t p_offset) const;
	bool has_selection() const;
	bool is_position_inside_selection(const Vector2& p_position);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void mark_layout_dirty();
	void _gui_input(const Ref<InputEvent>& p_event) override;
	Variant _get_drag_data(const Vector2& p_at_position) override;
	Vector2 _get_minimum_size() const override;
	void set_document_view(RestructuredtextLabel* p_document_view);
	void set_rst_text(const String& p_text);
	void clear_document();
	void select_all();
	void select_between_points(const Vector2& p_from, const Vector2& p_to);
	String get_link_uri_at_point(const Vector2& p_position);
	void clear_selection();
	Vector2i search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column);
	String get_selected_text() const;
	int32_t get_rendered_block_count();
	float get_content_height();
	int32_t get_table_cell_count();
	Rect2 get_table_cell_rect(int32_t p_index);
	bool has_anchor(const String& p_anchor) const;
	PackedStringArray get_anchors() const;
	float get_anchor_offset(const String& p_anchor) const;
	float get_search_result_y() const;
	PopupMenu* get_menu();
	bool is_menu_visible() const;
	void menu_option(int p_option);
};

class RestructuredtextLabel : public Control {
	GDCLASS(RestructuredtextLabel, Control);

public:
	enum MenuItems {
		MENU_COPY,
		MENU_SELECT_ALL,
		MENU_MAX,
	};
	enum Access {
		ACCESS_RESOURCES,
		ACCESS_USER_DATA,
		ACCESS_FILE_SYSTEM,
	};

private:
	String rst_text;
	Ref<RSTDocument> document;
	int32_t access = ACCESS_RESOURCES;
	String root_subfolder;
	ScrollContainer* scroll_container = nullptr;
	MarginContainer* margin_container = nullptr;
	RSTDocumentCanvas* document_canvas = nullptr;
	int32_t content_margin = 16;
	int32_t extra_font_size = 0;
	bool open_external_links = true;
	bool selection_enabled = true;
	bool deselect_on_focus_loss_enabled = true;
	bool drag_and_drop_selection_enabled = true;
	bool context_menu_enabled = false;
	bool shortcut_keys_enabled = true;
	bool fit_content = false;
	bool scroll_active = true;
	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_WORD_SMART;
	BitField<TextServer::LineBreakFlag> autowrap_flags_trim = TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	int32_t rendered_block_count = 0;

	void ensure_controls();
	void apply_theme_settings();
	void apply_scroll_settings();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	Vector2 _get_minimum_size() const override;
	void set_access(int32_t p_access);
	int32_t get_access() const;
	void set_root_subfolder(const String& p_root_subfolder);
	String get_root_subfolder() const;
	String get_resource_base_dir() const;
	void set_document(const Ref<RSTDocument>& p_document);
	Ref<RSTDocument> get_document() const;
	void set_rst_text(const String& p_text);
	String get_rst_text() const;
	void set_content_margin(int32_t p_margin);
	int32_t get_content_margin() const;
	void set_extra_font_size(int32_t p_extra);
	int32_t get_extra_font_size() const;
	void set_open_external_links(bool p_enabled);
	bool get_open_external_links() const;
	void set_selection_enabled(bool p_enabled);
	bool is_selection_enabled() const;
	void set_deselect_on_focus_loss_enabled(bool p_enabled);
	bool is_deselect_on_focus_loss_enabled() const;
	void set_drag_and_drop_selection_enabled(bool p_enabled);
	bool is_drag_and_drop_selection_enabled() const;
	void set_context_menu_enabled(bool p_enabled);
	bool is_context_menu_enabled() const;
	void set_shortcut_keys_enabled(bool p_enabled);
	bool is_shortcut_keys_enabled() const;
	void set_fit_content(bool p_enabled);
	bool is_fit_content_enabled() const;
	void set_scroll_active(bool p_active);
	bool is_scroll_active() const;
	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;
	void set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags);
	BitField<TextServer::LineBreakFlag> get_autowrap_trim_flags() const;

	Error load_rst_file(const String& p_path);
	void render_text(const String& p_text);
	void refresh();
	void clear_document();
	void select_all();
	void clear_selection();
	Vector2i search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column) const;
	String get_selected_text() const;
	int32_t get_rendered_block_count() const;
	float get_content_height() const;
	String resolve_document_path(const String& p_path) const;
	String resolve_link_uri(const String& p_role, const String& p_target) const;
	HScrollBar* get_h_scroll_bar() const;
	VScrollBar* get_v_scroll_bar() const;
	void activate_link_uri(const String& p_uri);
	void _on_meta_clicked(const Variant& p_meta);
	bool has_anchor(const String& p_anchor) const;
	PackedStringArray get_anchors() const;
	bool scroll_to_anchor(const String& p_anchor, float p_align = 0.0f);
	bool scroll_to_search_result(float p_align = 0.0f);
	PopupMenu* get_menu();
	bool is_menu_visible() const;
	void menu_option(int p_option);
};

VARIANT_ENUM_CAST(RestructuredtextLabel::MenuItems);
VARIANT_ENUM_CAST(RestructuredtextLabel::Access);
