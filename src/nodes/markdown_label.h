#pragma once

#include "formats/html/layout/html_geometry.h"
#include "formats/html/text/html_text_selection.h"
#include "formats/markdown/markdown_parser.h"
#include "formats/markdown/markdown_layout_document.h"
#include "formats/mermaid/mermaid_render.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_scroll_bar.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/text_paragraph.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/v_scroll_bar.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <vector>

namespace godot {
	class PopupMenu;
}

using namespace godot;

class MarkdownLabel;

struct MarkdownThemeCache {
	float base_scale = 1.0f;

	struct Constants {
		int32_t line_separation = 2;
		int32_t paragraph_separation = 10;
		int32_t list_indent = 26;
		int32_t list_marker_gap = 6;
		int32_t table_striped = 1;
		int32_t footnote_space_before = 16;
		int32_t footnote_line_separation = 2;
		int32_t outline_size = 0;
		int32_t shadow_outline_size = 1;
		int32_t shadow_offset_x = 1;
		int32_t shadow_offset_y = 1;
	} constant;

	struct FontSizes {
		int32_t text = 16;
		int32_t code = 16;
		int32_t code_block = 16;
		int32_t list_marker = 16;
		int32_t heading[6] = { 36, 26, 20, 18, 12, 10 };
		int32_t table_header = 16;
		int32_t table_cell = 16;
		int32_t footnote_ref = 12;
		int32_t footnote_text = 16;
	} font_size;

	struct Colors {
		Color text = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color link = Color(0.36f, 0.62f, 1.0f, 1.0f);
		Color code_text = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color code_block = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color code_block_background = Color(0.08f, 0.09f, 0.11f, 1.0f);
		Color scrollbar_track = Color(0.1f, 0.1f, 0.1f, 0.6f);
		Color scrollbar_thumb = Color(1.0f, 1.0f, 1.0f, 0.4f);
		Color scrollbar_hover = Color(1.0f, 1.0f, 1.0f, 0.75f);
		Color scrollbar_pressed = Color(0.75f, 0.75f, 0.75f, 0.75f);
		Color highlight = Color(1.0f, 0.92f, 0.35f, 0.5f);
		Color highlight_font = Color(1.0f, 0.700f, 0.211f, 1.0f);
		Color font_outline = Color(0.0f, 0.0f, 0.0f, 1.0f);
		Color font_shadow = Color(0.0f, 0.0f, 0.0f, 0.0f);
		Color strikethrough = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color heading[6];
		Color selection_color = Color(0.102f, 0.102f, 1.0f, 0.8f);
		Color list_marker = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color table_header_font = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color table_cell_font = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color footnote_ref = Color(0.809f, 0.809f, 0.809f, 1.0f);
		Color footnote_text = Color(0.875f, 0.875f, 0.875f, 0.502f);
	} color;

	struct Fonts {
		Ref<Font> text;
		Ref<Font> bold;
		Ref<Font> bold_italic;
		Ref<Font> italic;
		Ref<Font> code;
		Ref<Font> code_block;
		Ref<Font> heading[6];
		Ref<Font> list_marker;
		Ref<Font> table_header;
		Ref<Font> table_cell;
		Ref<Font> footnote_ref;
		Ref<Font> footnote_text;
	} font;

	struct StyleBoxes {
		Ref<StyleBox> inline_code;
		Ref<StyleBox> code_block_panel;
		Ref<StyleBox> blockquote;
		Ref<StyleBox> blockquote_nested;
		Ref<StyleBox> selection;
		Ref<StyleBox> search_result;
		Ref<StyleBox> separator;
		Ref<StyleBox> highlight;
		Ref<StyleBox> table_panel;
		Ref<StyleBox> table_header_panel;
		Ref<StyleBox> table_cell_panel;
		Ref<StyleBox> table_striped_panel;
		Ref<StyleBox> footnote_ref;
	} stylebox;

	struct Icons {
		Ref<Texture2D> task_checked;
		Ref<Texture2D> task_unchecked;
		Ref<Texture2D> file_broken;
	} icon;

	void build(Control* p_owner, int32_t p_extra_font_size);
};


class MarkdownLabelCanvas : public Control {
	GDCLASS(MarkdownLabelCanvas, Control);

	friend class MarkdownLabel;

private:
	MarkdownLabel* label = nullptr;
	String raw_text;
	String plain_text;
	std::vector<MarkdownCanvasItem> items;
	std::vector<MarkdownCanvasLine> lines;
	bool layout_dirty = true;
	bool dragging_selection = false;
	bool selection_dragged = false;
	bool selection_drag_attempt = false;
	bool pointer_update_queued = false;
	bool internal_processing_enabled = false;
	PopupMenu* menu = nullptr;
	int64_t selection_anchor = 0;
	int64_t selection_caret = 0;
	dockit::HTMLSelectionPoint selection_origin;
	int64_t search_match_start = -1;
	int64_t search_match_end = -1;
	Vector2 press_position;
	bool pressed_markdown = false;
	bool pressed_blank = false;
	int32_t rendered_block_count = 0;
	float content_height = 1.0f;
	float layout_width = 0.0f;
	int32_t layout_height = 0;
	int32_t layout_html_height = 0;
	struct HTMLScrollbarGesture {
		Ref<HTMLLayout> layout;
		dockit::HTMLScrollbarDrag drag;
		Vector2 origin;
	};
	std::optional<HTMLScrollbarGesture> html_scrollbar_drag;
	godot::Ref<HTMLLayout> hovered_html_scrollbar_layout;
	void update_html_scrollbar_hover(const godot::Ref<HTMLLayout>& p_layout, const godot::Vector2& p_point = {});
	Ref<HTMLLayout> pressed_html_layout;
	uintptr_t pressed_html_link = 0;
	Dictionary anchor_offsets;
	struct HTMLAnchorSource {
		Ref<HTMLLayout> layout;
		float top;
	};
	HashMap<String, HTMLAnchorSource> html_anchor_sources;
	HashMap<String, float> footnote_reference_offsets;
	HashMap<String, float> footnote_definition_offsets;

	MarkdownThemeCache theme_cache;
	MermaidRenderCache mermaid_cache;

	MarkdownParserState parser_state;
	dockit::MarkdownLayoutDocument layout_document;
	std::vector<MarkdownBlock> cached_blocks;
	std::vector<int64_t> source_line_starts{ 0 };
	int32_t parsed_source_line_count = 0;
	bool streaming_enabled = false;
	bool deferred_layout_enabled = true;
	bool pending_stream_parse = false;
	int32_t max_unstable_lines = 32;
	int32_t layout_flush_interval_msec = 33;
	double pending_layout_elapsed_msec = 0.0;

	void mark_layout_dirty();
	void reset_source_line_starts();
	void append_source_line_starts(int64_t p_previous_length);
	MarkdownParseOptions get_parse_options() const;
	void parse_current_text_incremental();
	void build_theme_cache();
	void _generate_context_menu();
	void _update_context_menu();
	Key _get_menu_action_accelerator(const String& p_action);
	void rebuild_layout();
	double get_offset_for_source_line(double p_line, int32_t p_line_count);
	double get_source_line_at_offset(double p_offset, int32_t p_line_count);
	Rect2 get_visible_content_rect() const;
	Rect2 get_html_visible_rect() const;
	bool is_rect_visible(const Rect2& p_rect, const Rect2& p_visible_rect) const;
	Ref<HTMLLayout> html_at_position(const Vector2& p_position, Vector2& r_local_position, int64_t* r_offset = nullptr) const;
	dockit::HTMLSelectionPoint selection_point_at(const Vector2& p_position) const;
	void extend_document_selection(const Vector2& p_position);
	void scroll_html_input(const Vector2& p_position, const Ref<InputEvent>& p_event);
	void on_margin_gui_input(const Ref<InputEvent>& p_event);
	void update_pointer(const Vector2& p_position);
	void queue_pointer_update();
	void refresh_pointer();
	int64_t hit_test_document(const Vector2& p_position) const;
	String link_uri_at_position(const Vector2& p_position) const;
	String link_uri_at_character(int64_t p_character) const;
	String image_tooltip_at_position(const Vector2& p_position) const;
	float footnote_target_at_position(const Vector2& p_position) const;
	String footnote_id_at_character(int64_t p_character, bool p_reference) const;
	int64_t line_column_to_offset(int32_t p_line, int32_t p_column) const;
	Vector2i offset_to_line_column(int64_t p_offset) const;
	bool has_selection() const;
	bool is_character_selected(int64_t p_character) const;
	bool is_position_inside_selection(const Vector2& p_position);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	MarkdownLabelCanvas();
	~MarkdownLabelCanvas();

	void _gui_input(const Ref<InputEvent>& p_event) override;									// 处理控件收到的 GUI 输入事件
	void _draw() override;																		// 绘制文档内容
	Variant _get_drag_data(const Vector2& p_at_position) override;								// 根据当前位置构造拖动数据
	Vector2 _get_minimum_size() const override;													// 返回控件所需的最小尺寸

	void set_label(MarkdownLabel* p_label);														// 将画布绑定到所属的 Markdown 标签控件
	void set_markdown_text(const String& p_text);												// 设置要显示的 Markdown 文本
	String resolve_image_uri(const String& p_uri, bool p_html) const;							// 将 Markdown 或 HTML 图像 URI 解析为资源路径
	int32_t get_layout_height() const;															// 返回画布当前内容的排版高度
	void append_text_incremental(const String& p_delta);										// 将增量文本追加到当前 Markdown 文档
	void finish_stream();																		// 结束当前流式输入并刷新待处理排版
	void clear_document();																		// 清除当前文档内容及其渲染状态
	void set_streaming_enabled(bool p_enabled);													// 设置流式渲染功能
	void set_max_unstable_lines(int32_t p_lines);												// 设置流式解析的不稳定行数上限
	void set_deferred_layout_enabled(bool p_enabled);											// 设置延迟排版功能
	bool is_deferred_layout_enabled() const;													// 返回是否启用延迟排版功能
	void set_layout_flush_interval_msec(int32_t p_msec);										// 设置排版刷新间隔
	int32_t get_layout_flush_interval_msec() const;												// 返回排版刷新间隔
	bool flush_pending_layout();																// 立即处理等待中的排版更新
	void request_local_image(const String& p_uri);												// 请求加载指定的本地图片
	void refresh_image_content();																// 刷新文档中图片的加载与显示状态
	static void remote_image_completed(
		const String& p_url,
		const Ref<Texture2D>& p_texture,
		uint64_t p_canvas
	);																							// 处理远程图片加载完成后的画布更新
	static void refresh_after_html_resource(uint64_t p_canvas);									// 处理 HTML 资源加载完成后的画布更新
	void update_internal_processing();															// 根据待处理资源更新内部处理状态
	bool process_pending_local_images();														// 处理等待加载的本地图片并返回是否有变化
	bool update_gif_animations(double p_delta);													// 按时间增量更新 GIF 动画并返回是否有变化
	void select_all();																			// 选中当前文档中的全部可选文本
	void select_between_points(const Vector2& p_from, const Vector2& p_to);						// 按两个画布坐标设置文本选区
	String get_link_uri_at_point(const Vector2& p_position);									// 返回指定坐标处链接的 URI
	void clear_selection();																		// 清除当前文本选区
	Vector2i search(
		const String& p_text,
		int32_t p_flags,
		int32_t p_from_line,
		int32_t p_from_column
	);																							// 从指定行列开始查找符合条件的文本
	String get_selected_text() const;															// 返回当前选区中的文本
	int32_t get_rendered_block_count();															// 返回当前文档中已渲染的块数量
	String get_rendered_html();																	// 返回当前排版 DOM 中 body 的 HTML 内容
	float get_content_height();																	// 返回当前文档内容的总高度
	bool has_anchor(const String& p_anchor) const;												// 检查文档中是否存在指定锚点
	PackedStringArray get_anchors() const;														// 返回文档中所有锚点标识
	float get_anchor_offset(const String& p_anchor) const;										// 返回指定锚点在文档中的纵向偏移
	std::optional<float> scroll_to_html_anchor(const String& p_anchor, float p_align);			// 将 HTML 锚点滚动到视口中的对齐位置
	float get_search_result_y() const;															// 返回当前搜索结果的纵向位置
	float get_bottom_position() const;															// 返回文档内容底部的纵向位置
	PopupMenu* get_menu();																		// 返回控件的上下文菜单
	bool is_menu_visible() const;																// 返回上下文菜单当前是否可见
	void menu_option(int p_option);																// 执行指定的上下文菜单选项
};

class MarkdownLabel : public Control {
	GDCLASS(MarkdownLabel, Control);

public:
	enum MenuItems {
		MENU_COPY,
		MENU_SELECT_ALL,
		MENU_MAX
	};

	enum Access {
		ACCESS_RESOURCES,
		ACCESS_USER_DATA,
		ACCESS_FILE_SYSTEM,
	};

	enum WordSelectionMode {
		WORD_SELECTION_MODE_AUTO,
		WORD_SELECTION_MODE_TEXT_SERVER,
		WORD_SELECTION_MODE_JIEBA,
	};

	void set_word_selection_mode(WordSelectionMode p_mode);											// 设置鼠标选词使用的分词模式
	WordSelectionMode get_word_selection_mode() const;											// 返回鼠标选词的分词模式

private:
	friend class MarkdownLabelCanvas;

	String raw_text;
	int32_t access = ACCESS_RESOURCES;
	String root_subfolder;
	String source_path;
	ScrollContainer* scroll_container = nullptr;
	MarginContainer* margin_container = nullptr;
	MarkdownLabelCanvas* canvas = nullptr;
	int32_t extra_font_size = 0;
	bool parse_latex = false;
	bool parse_html = false;
	Ref<CSSStyleSheet> stylesheet;
	bool parse_gfm = true;
	bool parse_mermaid = false;
	bool selection_enabled = true;
	WordSelectionMode word_selection_mode = WORD_SELECTION_MODE_AUTO;
	bool deselect_on_focus_loss_enabled = true;
	bool drag_and_drop_selection_enabled = true;
	bool context_menu_enabled = false;
	bool shortcut_keys_enabled = true;
	bool streaming_enabled = false;
	bool deferred_layout_enabled = true;
	int32_t max_unstable_lines = 32;
	int32_t layout_flush_interval_msec = 33;
	bool fit_content = false;
	bool scroll_active = true;
	bool scroll_follow = false;
	bool scroll_following = false;
	bool scroll_follow_visible_characters = false;
	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_WORD_SMART;
	BitField<TextServer::LineBreakFlag> autowrap_flags_trim = TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;

	void ensure_controls();
	void apply_theme_settings();
	void apply_scroll_settings();
	void set_markdown_source(const String& p_text, const String& p_source_path);
	String get_resource_base_dir() const;
	void update_scroll_following();
	void scroll_to_following();
	void on_scroll_value_changed(double p_value);

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo& p_property) const;
	void _notification(int p_what);

public:
	Vector2 _get_minimum_size() const override;													// 返回控件所需的最小尺寸

	void set_markdown_text(const String& p_text);												// 设置要显示的 Markdown 文本
	String get_markdown_text() const;															// 返回当前 Markdown 文本
	void set_access(int32_t p_access);															// 设置资源访问范围
	int32_t get_access() const;																	// 返回资源访问范围
	void set_root_subfolder(const String& p_root_subfolder);									// 设置资源根子目录
	String get_root_subfolder() const;															// 返回资源根子目录
	void set_stylesheet(const Ref<CSSStyleSheet>& p_stylesheet); // 设置参与全文级联的作者样式表
	Ref<CSSStyleSheet> get_stylesheet() const; // 获取作者样式表
	void set_parse_latex(bool p_enabled);														// 设置是否解析 LaTeX 语法
	void set_parse_html(bool p_enabled);														// 设置是否解析 HTML 语法
	bool is_parsing_html() const;																// 返回是否解析 HTML
	bool is_parsing_latex() const;																// 返回是否解析 LaTeX
	void set_parse_gfm(bool p_enabled);															// 设置是否解析 GFM 语法
	bool is_parsing_gfm() const;																// 返回是否解析 GFM
	void set_parse_mermaid(bool p_enabled);														// 设置是否解析 Mermaid 语法
	bool is_parsing_mermaid() const;															// 返回是否解析 Mermaid

	void append_text(const String& p_delta);													// 将增量文本追加到当前文档
	void finish_stream();																		// 结束当前流式输入并刷新待处理排版
	void clear();																				// 清空当前文档并重置渲染状态

	void set_streaming_enabled(bool p_enabled);													// 设置流式渲染功能
	bool is_streaming_enabled() const;															// 返回是否启用流式渲染功能

	void set_max_unstable_lines(int32_t p_lines);												// 设置流式解析的不稳定行数上限
	int32_t get_max_unstable_lines() const;														// 返回流式解析的不稳定行数上限

	void set_deferred_layout_enabled(bool p_enabled);											// 设置延迟排版功能
	bool is_deferred_layout_enabled() const;													// 返回是否启用延迟排版功能

	void set_layout_flush_interval_msec(int32_t p_msec);										// 设置排版刷新间隔
	int32_t get_layout_flush_interval_msec() const;												// 返回排版刷新间隔

	bool flush_pending_layout();																// 立即处理等待中的排版更新

	void set_fit_content(bool p_enabled);														// 设置是否按内容调整控件尺寸
	bool is_fit_content_enabled() const;														// 返回是否按内容调整控件尺寸

	void set_scroll_active(bool p_active);														// 设置是否启用滚动
	bool is_scroll_active() const;																// 返回是否启用滚动功能

	void set_scroll_follow(bool p_follow);														// 设置滚动跟随功能
	bool is_scroll_following() const;															// 返回是否启用滚动跟随

	void set_scroll_follow_visible_characters(bool p_follow);									// 设置跟随可见字符功能
	bool is_scroll_following_visible_characters() const;										// 返回是否按可见字符启用滚动跟随

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);									// 设置自动换行模式
	TextServer::AutowrapMode get_autowrap_mode() const;											// 返回自动换行模式

	void set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags);					// 设置自动换行裁剪标志
	BitField<TextServer::LineBreakFlag> get_autowrap_trim_flags() const;						// 返回自动换行裁剪标志


	void set_extra_font_size(int32_t p_extra);													// 设置附加字号
	int32_t get_extra_font_size() const;														// 返回附加字号

	void set_selection_enabled(bool p_enabled);													// 设置是否允许选择文本
	bool is_selection_enabled() const;															// 返回是否启用文本选择功能

	void set_deselect_on_focus_loss_enabled(bool p_enabled);									// 设置失焦时清除选区功能
	bool is_deselect_on_focus_loss_enabled() const;												// 返回是否启用失焦时清除选区功能

	void set_drag_and_drop_selection_enabled(bool p_enabled);									// 设置选中文本拖放功能
	bool is_drag_and_drop_selection_enabled() const;											// 返回是否启用选中文本拖放功能

	void set_context_menu_enabled(bool p_enabled);												// 设置是否显示上下文菜单
	bool is_context_menu_enabled() const;														// 返回是否启用上下文菜单

	void set_shortcut_keys_enabled(bool p_enabled);												// 设置是否启用快捷键
	bool is_shortcut_keys_enabled() const;														// 返回是否启用快捷键

	void select_all();																			// 选中当前文档中的全部可选文本
	void clear_selection();																		// 清除当前文本选区
	String get_selected_text() const;															// 返回当前选区中的文本

	Vector2i search(
		const String& p_text,
		int32_t p_flags,
		int32_t p_from_line,
		int32_t p_from_column
	);																							// 从指定行列开始查找符合条件的文本

	String get_link_uri_at_point(const Vector2& p_position);									// 返回指定坐标处链接的 URI
	void activate_link_uri(const String& p_uri);												// 发出链接信号并滚动到文内锚点

	bool has_anchor(const String& p_anchor) const;												// 检查文档中是否存在指定锚点
	PackedStringArray get_anchors() const;														// 返回文档中所有锚点标识
	bool scroll_to_anchor(const String& p_anchor, float p_align = 0.0f);						// 将指定锚点滚动到视口中的对齐位置
	bool scroll_to_search_result(float p_align = 0.0f);											// 将当前搜索结果滚动到指定对齐位置
	double get_scroll_offset_for_source_line(double p_line, int32_t p_line_count);				// 返回源文件指定行对应的滚动偏移
	double get_source_line_at_scroll_offset(double p_offset, int32_t p_line_count);				// 返回指定滚动偏移对应的源文件行

	Error load_markdown_file(const String& p_path);												// 加载指定路径的 Markdown 文件
	void refresh();																				// 重新解析文档并刷新渲染结果
	void notify_canvas_layout_changed();														// 通知画布尺寸或布局已发生变化

	int32_t get_rendered_block_count() const;													// 返回当前文档中已渲染的块数量
	String get_rendered_html() const;															// 返回当前排版 DOM 中 body 的 HTML 内容
	float get_content_height() const;															// 返回当前文档内容的总高度

	HScrollBar* get_h_scroll_bar() const;														// 返回水平滚动条节点
	VScrollBar* get_v_scroll_bar() const;														// 返回垂直滚动条节点

	PopupMenu* get_menu();																		// 返回控件的上下文菜单
	bool is_menu_visible() const;																// 返回上下文菜单当前是否可见
	void menu_option(int p_option);																// 执行指定的上下文菜单选项
};

VARIANT_ENUM_CAST(MarkdownLabel::MenuItems);
VARIANT_ENUM_CAST(MarkdownLabel::Access);
VARIANT_ENUM_CAST(MarkdownLabel::WordSelectionMode);
