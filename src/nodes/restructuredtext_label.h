#pragma once

#include "formats/rst/rst_document.h"
#include "formats/html/html_layout.h"
#include "formats/html/css/css_style_sheet.h"

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

#include <optional>
#include <vector>

using namespace godot;

class RestructuredtextLabel;

struct RSTInlineSpan {
	String text;
	String link_uri;
	int64_t start = 0;
	int64_t end = 0;
	String footnote_id;
	Color color;
	bool bold = false;
	bool italic = false;
	bool code = false;
	bool footnote_reference = false;
	bool custom_color = false;
};

class RSTDocumentCanvas : public Control {
	GDCLASS(RSTDocumentCanvas, Control);

private:
	RestructuredtextLabel* document_view = nullptr;
	String rst_text;
	String plain_text;
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
	Ref<HTMLLayout> html_layout;
	struct HTMLScrollbarGesture {
		Ref<HTMLLayout> layout;
		dockit::HTMLScrollbarDrag drag;
		Vector2 origin;
	};
	std::optional<HTMLScrollbarGesture> html_scrollbar_drag;
	Ref<HTMLLayout> hovered_html_scrollbar_layout;
	bool html_scrollbar_hovered = false;

	void rebuild_layout();
	void update_html_scrollbar_hover(const Ref<HTMLLayout>& p_layout, const Vector2& p_point = Vector2());
	void scroll_html_input(const Vector2& p_position, const Ref<InputEvent>& p_event);
	void queue_scroll_redraw();
	void _generate_context_menu();
	void _update_context_menu();
	Key _get_menu_action_accelerator(const String& p_action);
	int64_t hit_test_document(const Vector2& p_position) const;
	String link_uri_at_position(const Vector2& p_position) const;
	float footnote_target_at_position(const Vector2& p_position);
	int64_t line_column_to_offset(int32_t p_line, int32_t p_column) const;
	Vector2i offset_to_line_column(int64_t p_offset) const;
	bool has_selection() const;
	bool is_position_inside_selection(const Vector2& p_position);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void mark_layout_dirty();																	// 标记文档布局需要重新计算
	void _gui_input(const Ref<InputEvent>& p_event) override;									// 处理控件收到的 GUI 输入事件
	Variant _get_drag_data(const Vector2& p_at_position) override;								// 根据当前位置构造拖动数据
	Vector2 _get_minimum_size() const override;													// 返回控件所需的最小尺寸
	void set_document_view(RestructuredtextLabel* p_document_view);								// 将画布绑定到所属的 reStructuredText 标签控件
	void set_rst_text(const String& p_text);													// 设置要显示的 reStructuredText 文本
	void clear_document();																		// 清除当前文档内容及其渲染状态
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
	float get_content_height();																	// 返回当前文档内容的总高度
	int32_t get_table_cell_count();																// 返回当前文档中表格单元格的数量
	Rect2 get_table_cell_rect(int32_t p_index);													// 返回指定表格单元格在画布中的矩形
	bool has_anchor(const String& p_anchor) const;												// 检查文档中是否存在指定锚点
	PackedStringArray get_anchors() const;														// 返回文档中所有锚点标识
	float get_anchor_offset(const String& p_anchor) const;										// 返回指定锚点在文档中的纵向偏移
	float get_search_result_y() const;															// 返回当前搜索结果的纵向位置
	PopupMenu* get_menu();																		// 返回控件的上下文菜单
	bool is_menu_visible() const;																// 返回上下文菜单当前是否可见
	void menu_option(int p_option);																// 执行指定的上下文菜单选项
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

	enum WordSelectionMode {
		WORD_SELECTION_MODE_AUTO,
		WORD_SELECTION_MODE_TEXT_SERVER,
		WORD_SELECTION_MODE_JIEBA,
	};

	void set_word_selection_mode(WordSelectionMode p_mode);											// 设置鼠标选词使用的分词模式
	WordSelectionMode get_word_selection_mode() const;											// 返回鼠标选词的分词模式

private:
	String rst_text;
	Ref<RSTDocument> document;
	Ref<CSSStyleSheet> stylesheet;
	int32_t access = ACCESS_RESOURCES;
	String root_subfolder;
	ScrollContainer* scroll_container = nullptr;
	MarginContainer* margin_container = nullptr;
	RSTDocumentCanvas* document_canvas = nullptr;
	int32_t extra_font_size = 0;
	bool open_external_links = true;
	bool selection_enabled = true;
	WordSelectionMode word_selection_mode = WORD_SELECTION_MODE_AUTO;
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
	void _validate_property(PropertyInfo& p_property) const;
	void _notification(int p_what);

public:
	Vector2 _get_minimum_size() const override;													// 返回控件所需的最小尺寸
	void set_access(int32_t p_access);															// 设置资源访问范围
	int32_t get_access() const;																	// 返回资源访问范围
	void set_root_subfolder(const String& p_root_subfolder);									// 设置资源根子目录
	String get_root_subfolder() const;															// 返回资源根子目录
	String get_resource_base_dir() const;														// 返回解析文档相对资源时使用的基础目录
	void set_document(const Ref<RSTDocument>& p_document);										// 设置文档资源
	Ref<RSTDocument> get_document() const;														// 返回文档资源
	void set_stylesheet(const Ref<CSSStyleSheet>& p_stylesheet);									// 设置作者样式表
	Ref<CSSStyleSheet> get_stylesheet() const;														// 返回作者样式表
	void set_rst_text(const String& p_text);													// 设置要显示的 reStructuredText 文本
	String get_rst_text() const;																// 返回当前 reStructuredText 文本
	void set_extra_font_size(int32_t p_extra);													// 设置附加字号
	int32_t get_extra_font_size() const;														// 返回附加字号
	void set_open_external_links(bool p_enabled);												// 设置自动打开外部链接功能
	bool get_open_external_links() const;														// 返回自动打开外部链接功能
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
	void set_fit_content(bool p_enabled);														// 设置是否按内容调整控件尺寸
	bool is_fit_content_enabled() const;														// 返回是否按内容调整控件尺寸
	void set_scroll_active(bool p_active);														// 设置是否启用滚动
	bool is_scroll_active() const;																// 返回是否启用滚动功能
	void set_autowrap_mode(TextServer::AutowrapMode p_mode);									// 设置自动换行模式
	TextServer::AutowrapMode get_autowrap_mode() const;											// 返回自动换行模式
	void set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags);					// 设置自动换行裁剪标志
	BitField<TextServer::LineBreakFlag> get_autowrap_trim_flags() const;						// 返回自动换行裁剪标志

	Error load_rst_file(const String& p_path);													// 加载指定路径的 reStructuredText 文件
	void render_text(const String& p_text);														// 解析并渲染给定的 reStructuredText 文本
	void refresh();																				// 重新解析文档并刷新渲染结果
	void clear_document();																		// 清除当前文档内容及其渲染状态
	void select_all();																			// 选中当前文档中的全部可选文本
	void clear_selection();																		// 清除当前文本选区
	Vector2i search(
		const String& p_text,
		int32_t p_flags,
		int32_t p_from_line,
		int32_t p_from_column
	) const;																					// 从指定行列开始查找符合条件的文本
	String get_selected_text() const;															// 返回当前选区中的文本
	int32_t get_rendered_block_count() const;													// 返回当前文档中已渲染的块数量
	float get_content_height() const;															// 返回当前文档内容的总高度
	String resolve_document_path(const String& p_path) const;									// 根据当前文档和资源访问范围解析路径
	String resolve_link_uri(const String& p_role, const String& p_target) const;				// 根据链接角色和目标解析链接 URI
	HScrollBar* get_h_scroll_bar() const;														// 返回水平滚动条节点
	VScrollBar* get_v_scroll_bar() const;														// 返回垂直滚动条节点
	void activate_link_uri(const String& p_uri);												// 打开或处理指定的链接 URI
	void _on_meta_clicked(const Variant& p_meta);												// 响应富文本链接的点击
	bool has_anchor(const String& p_anchor) const;												// 检查文档中是否存在指定锚点
	PackedStringArray get_anchors() const;														// 返回文档中所有锚点标识
	bool scroll_to_anchor(const String& p_anchor, float p_align = 0.0f);						// 将指定锚点滚动到视口中的对齐位置
	bool scroll_to_search_result(float p_align = 0.0f);											// 将当前搜索结果滚动到指定对齐位置
	PopupMenu* get_menu();																		// 返回控件的上下文菜单
	bool is_menu_visible() const;																// 返回上下文菜单当前是否可见
	void menu_option(int p_option);																// 执行指定的上下文菜单选项
};

VARIANT_ENUM_CAST(RestructuredtextLabel::MenuItems);
VARIANT_ENUM_CAST(RestructuredtextLabel::Access);
VARIANT_ENUM_CAST(RestructuredtextLabel::WordSelectionMode);
