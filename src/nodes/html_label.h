#pragma once

#include "formats/html/html_document.h"
#include "formats/html/elements/html_form_elements.h"
#include "formats/html/text/html_text_selection.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/core/object_id.hpp>

#include <optional>
#include <vector>

namespace godot {
	class InputEventKey;
	class PopupMenu;
	class ScrollContainer;
	class Timer;
}

namespace docrender {
	class HTMLTextEditor;
}

class HTMLLabel;
class HTMLButtonElement;
class HTMLFormElement;
class HTMLLabelElement;
class HTMLSelectElement;

class HTMLLabelCanvas : public godot::Control {
	GDCLASS(HTMLLabelCanvas, godot::Control)

	friend class HTMLLabel;

	HTMLLabel* label = nullptr;
	godot::Vector2 pressed_position;
	godot::Ref<HTMLElement> pressed_element;
	godot::Ref<HTMLElement> pressed_button;
	godot::Ref<HTMLElement> pressed_form_target;
	godot::Ref<HTMLElement> dragging_input;
	struct TextareaDrag {
		godot::Ref<HTMLElement> element;
		godot::Vector2i axes;
		docrender::HTMLPlaneTransform inverse;
		godot::Vector2 start_point;
		godot::Vector2 start_size;
		float scrollbar_grab = 0;
		bool vertical_scrollbar = false;
	};
	std::optional<TextareaDrag> textarea_drag;
	std::optional<docrender::HTMLScrollbarDrag> scrollbar_drag;
	std::optional<godot::Vector2> resize_pending;
	bool resize_queued = false;
	bool dragged_resize = false;
	bool applying_textarea_resize = false;
	godot::Ref<HTMLInputElement> pressed_checkbox;
	std::optional<int32_t> ime_window;
	std::optional<godot::Vector2i> ime_position;
	godot::Timer* caret_blink_timer = nullptr;
	bool caret_window_active = true;
	godot::PopupMenu* input_menu = nullptr;
	godot::Ref<HTMLElement> input_menu_target;
	bool checkbox_key_pressed = false;
	godot::Ref<HTMLElement> focused_element;
	uint64_t focused_reset_version = 0;
	std::vector<godot::Ref<HTMLElement>> tab_elements;
	godot::Key activation_key = godot::KEY_NONE;
	std::optional<bool> tab_entry_backward;
	uintptr_t pressed_link = 0;
	int64_t selection_anchor = 0;
	int64_t selection_caret = 0;
	docrender::HTMLSelectionPoint selection_origin;
	bool selecting = false;
	bool selection_dragged = false;
	bool has_focusable_elements = false;
	mutable godot::ObjectID diagnostic_panel_id;
	mutable godot::String diagnostic_panel_text;

	void cancel_press();
	void extend_document_selection(const godot::Vector2& p_point);
	void cancel_click();
	void cancel_button_press();
	void cancel_key_press();
	bool handle_dom_key(const godot::InputEventKey& p_key);
	void handle_selection_key(const godot::InputEventKey& p_key);
	void toggle_checkbox(const godot::Ref<HTMLInputElement>& p_input);
	bool move_radio(const godot::Ref<HTMLInputElement>& p_input, int p_direction);
	godot::Ref<HTMLElement> get_form_click_target(const godot::Ref<HTMLElement>& p_element) const;
	bool handle_input_key(const godot::InputEventKey& p_key);
	bool open_input_menu(const godot::Ref<HTMLElement>& p_element, const godot::Vector2& p_position, bool p_pointer);
	void close_input_menu();
	void on_input_menu_hidden();
	void on_input_menu_selected(int p_command);
	void run_input_command(const godot::Ref<HTMLElement>& p_element, int p_command);
	void change_input(const godot::Ref<HTMLElement>& p_input);
	void commit_input(const godot::Ref<HTMLElement>& p_input, bool p_deferred = false);
	void set_input_caret(const godot::Ref<HTMLElement>& p_input, const godot::Vector2& p_point, bool p_extend);
	bool begin_textarea_drag(const godot::Ref<HTMLElement>& p_element, const godot::Vector2& p_point);
	void move_textarea_drag(const godot::Vector2& p_point);
	void flush_textarea_resize();
	void update_input_autoscroll();
	std::optional<CursorShape> textarea_resize_cursor(const godot::Vector2& p_point) const;
	void set_input_caret_visible(docrender::HTMLTextEditor& p_editor, bool p_visible);
	void restart_caret_blink();
	void stop_caret_blink();
	void blink_caret();
	void update_ime();
	void close_ime();
	void on_ime_update();
	void set_focused_element(const godot::Ref<HTMLElement>& p_button, bool p_keyboard = false);
	void set_element_pseudo(const godot::Ref<HTMLElement>& p_button, const char* p_name, bool p_enabled);
	void clear_tab_entry();
	void focus_tab_endpoint(bool p_backward);
	void update_focus_mode();
	void update_cursor(const std::optional<godot::Vector2>& p_point);
	void refresh_diagnostic_tooltip(const std::optional<godot::Vector2>& p_point);

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	void _draw() override;
	bool _has_point(const godot::Vector2& p_point) const override;
	godot::String _get_tooltip(const godot::Vector2& p_position) const override;
	godot::Object* _make_custom_tooltip(const godot::String& p_for_text) const override;
	void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
};

class HTMLLabel : public godot::Control {
	GDCLASS(HTMLLabel, godot::Control)

public:
	enum AnimationProcessMode {
		ANIMATION_PROCESS_IDLE,
		ANIMATION_PROCESS_PHYSICS,
		ANIMATION_PROCESS_MANUAL,
	};

private:
	friend class HTMLLabelCanvas;

	godot::Ref<HTMLDocument> document;
	godot::Ref<HTMLLayout> layout;
	godot::ScrollContainer* scroll_container = nullptr;
	godot::Control* scroll_content = nullptr;
	HTMLLabelCanvas* canvas = nullptr;
	godot::Control* select_menu = nullptr;
	godot::Ref<HTMLSelectElement> open_select;
	std::vector<docrender::HTMLSelectMenuRow> select_rows;
	int select_highlight = -1;
	int select_pressed = -1;
	int select_scroll = 0;
	int select_visible_rows = 0;
	float select_row_height = 0.0f;
	godot::String text;
	godot::String base_dir = "res://";
	godot::String stylesheet;
	godot::Vector2i viewport_size;
	godot::Vector2i content_size;
	godot::Vector2i sorted_scroll_offset;
	godot::Vector2i viewport_input_axes{1, 1};
	godot::Vector2i hidden_scroll_offset;
	godot::String render_error;
	std::optional<godot::Vector2i> pending_scroll_offset;
	bool pending_scroll_requested = false;
	godot::Ref<godot::Tween> viewport_scroll_tween;
	bool applying_scroll_tween = false;
	godot::Vector2i dragging_viewport_scrollbar;
	godot::Ref<godot::Tween> viewport_pan_snap_timer;
	godot::Vector2i viewport_pan_axes;
	bool source_dirty = true;
	bool parsed = false;
	bool refresh_queued = false;
	bool layout_updated = false;
	bool layout_signal_pending = false;
	bool selection_enabled = true;
	bool context_menu_enabled = true;
	AnimationProcessMode animation_process_mode = ANIMATION_PROCESS_IDLE;
	bool caret_blink = true;
	double caret_blink_interval = 0.65;
	bool view_update_pending = false;
	bool scroll_sort_pending = false;
	bool interaction_queued = false;
	bool gui_hover_dirty = false;
	bool updating_gui_hover = false;
	bool hover_reflow = false;
	bool interaction_styles_dirty = false;
	bool reveal_focused_element = false;
	int hover_passes_remaining = 0;
	docrender::HTMLScrollbarAppearance viewport_scrollbar_style;
	bool viewport_scrollbar_theme_dirty = false;
	godot::Ref<godot::StyleBox> viewport_panel;
	godot::Vector2 viewport_panel_gutters;
	godot::Vector2 viewport_painted_gutters;

	void ensure_controls();
	void queue_refresh();
	void flush_queued_refresh();
	void queue_interaction_style_refresh();
	void on_auto_content_update();
	void on_transition_frame(bool p_size_changed);
	void update_internal_processing();
	godot::Vector2i get_content_selection_range() const;
	void source_changed();
	void document_changed(uint64_t p_document_id);
	void parse_pending_source();
	void on_layout_edited();
	void refresh();
	void update_viewport_scrollbar_style(const docrender::HTMLScrollbarAppearance& p_style);
	void update_viewport_gutters(const godot::Vector2& p_gutters);
	void draw_viewport_gutters();
	void update_view(const godot::Vector2i& p_viewport, const godot::Vector2i& p_content,
		const godot::Vector2i& p_visible_bars, const godot::Vector2i& p_input_axes, bool p_layout_changed,
		const godot::String& p_error = {}, const godot::Vector2& p_gutters = {}, const godot::Vector2& p_painted_gutters = {});
	godot::Rect2 get_visible_rect() const;
	void apply_scroll_target(const docrender::HTMLScrollTarget& p_target, std::optional<float> p_align);
	void advance_viewport_scroll(double p_fraction, godot::Vector2 p_from, godot::Vector2 p_to);
	void cancel_viewport_scroll();
	void on_nested_scroll_advanced();
	void apply_scroll_offset(const godot::Vector2i& p_offset);
	void apply_pending_scroll();
	void on_scroll_changed(double p_value);
	void on_viewport_gui_input(const godot::Ref<godot::InputEvent>& p_event);
	void on_viewport_scrollbar_input(const godot::Ref<godot::InputEvent>& p_event, int p_axis);
	void settle_viewport_snap(const godot::Vector2i& p_axes);
	void settle_viewport_pan();
	void cancel_viewport_pan_snap();
	void on_content_sorted();
	void request_hover_check();
	void queue_interaction();
	void update_interaction();
	void activate_link(const godot::String& p_uri);
	bool activate_form_control(const godot::Ref<HTMLElement>& p_element);
	void submit_from_text(const godot::Ref<HTMLElement>& p_element);
	bool open_select_menu(const godot::Ref<HTMLSelectElement>& p_element);
	void close_select_menu();
	void update_select_menu();
	void draw_select_menu();
	void select_menu_input(const godot::Ref<godot::InputEvent>& p_event);
	bool handle_select_key(const godot::InputEventKey& p_key);
	void highlight_select_row(int p_row);
	void commit_select_row(int p_row);

protected:
	static void _bind_methods();
	void _validate_property(godot::PropertyInfo& p_property) const;
	void _notification(int p_what);

public:
	HTMLLabel();
	void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
	void _input(const godot::Ref<godot::InputEvent>& p_event) override;
	void set_css_diagnostics_enabled(bool p_enabled); // 设置是否收集 CSS 声明诊断
	bool is_css_diagnostics_enabled() const; // 返回是否收集 CSS 声明诊断
	godot::TypedArray<godot::Dictionary> get_css_diagnostics() const; // 返回 CSS 声明诊断的快照

	godot::PackedStringArray _get_configuration_warnings() const override;

	void set_document(const godot::Ref<HTMLDocument>& p_document);					// 设置用于显示的 HTML 文档资源
	godot::Ref<HTMLDocument> get_document() const;									// 返回控件当前关联的文档资源
	void set_text(const godot::String& p_text);										// 设置控件的内联 HTML 文本
	godot::String get_text() const;													// 返回控件的内联 HTML 文本
	void set_base_dir(const godot::String& p_base_dir);								// 设置内联 HTML 的相对资源路径基准目录
	godot::String get_base_dir() const;												// 返回内联 HTML 的资源基准目录
	void set_stylesheet(const godot::String& p_stylesheet);							// 设置应用于内联 HTML 的附加 CSS
	godot::String get_stylesheet() const;											// 返回内联 HTML 的附加 CSS
	godot::Vector2i get_content_size() const;										// 返回排版后的内容尺寸
	godot::String get_render_error() const;											// 返回最近的解析或排版错误
	godot::Vector2i get_scroll_offset() const;										// 返回横向和纵向的文档滚动偏移
	void set_scroll_offset(const godot::Vector2i& p_offset);						// 设置滚动偏移并限制在可滚动范围内
	bool scroll_to_anchor(const godot::String& p_id, float p_align = 0.0f);			// 将指定 HTML ID 的锚点滚到视口内的对齐位置
	godot::Ref<HTMLElement> create_element(const godot::String& p_tag_name);		// 创建可插入当前控件文档的元素
	godot::Ref<HTMLDocumentFragment> create_document_fragment();					// 创建可插入当前控件文档的片段
	godot::Ref<HTMLElement> query_selector(const godot::String& p_selector) const;	// 返回当前控件文档中第一个匹配元素
	godot::TypedArray<HTMLElement> query_selector_all(
		const godot::String& p_selector
	) const;																		// 返回当前控件文档中的全部匹配元素
	godot::Ref<HTMLElement> get_element_by_id(const godot::String& p_id) const;		// 按区分大小写的 HTML ID 查找当前控件文档中的第一个元素
	godot::Ref<HTMLElement> get_element_at(const godot::Vector2& p_position) const; // 将控件局部坐标换算为文档坐标并命中可见元素
	godot::Ref<HTMLElement> get_focused_element() const; // 返回当前聚焦的 DOM 元素
	bool focus_element(const godot::Ref<HTMLElement>& p_element); // 将有效元素设为 DOM 焦点
	bool activate_element(const godot::Ref<HTMLElement>& p_element); // 激活当前控件中可用的链接或表单控件
	godot::Error submit_form(const godot::Ref<HTMLFormElement>& p_form,
		const godot::Ref<HTMLButtonElement>& p_submitter = {}); // 校验并发出有序的表单数据
	bool scroll_to_element(
		const godot::Ref<HTMLElement>& p_element,
		float p_align = 0.0f
	);																				// 将当前控件的目标元素滚到视口内的对齐位置
	void set_selection_enabled(bool p_enabled);										// 设置是否允许选择文本
	bool is_selection_enabled() const;												// 返回是否允许选择文本
	void set_context_menu_enabled(bool p_enabled); // 设置文本字段是否显示编辑菜单
	bool is_context_menu_enabled() const; // 返回文本字段是否显示编辑菜单
	void set_animation_process_mode(AnimationProcessMode p_mode); // 设置 CSS 动画和过渡的帧更新方式
	AnimationProcessMode get_animation_process_mode() const; // 返回 CSS 动画和过渡的帧更新方式
	void advance_animation(double p_delta); // 手动推进 CSS 动画和过渡，时间增量以秒为单位
	void set_caret_blink_enabled(bool p_enabled); // 设置聚焦文本字段的插入光标是否闪烁
	bool is_caret_blink_enabled() const; // 返回插入光标是否闪烁
	void set_caret_blink_interval(double p_interval); // 设置插入光标切换显示状态的间隔，单位为秒
	double get_caret_blink_interval() const; // 返回插入光标切换显示状态的间隔
	void select_all();																// 选中全部可复制文本
	void clear_selection();															// 清除当前文本选区
	godot::String get_selected_text() const;										// 返回当前选中的文本
};

VARIANT_ENUM_CAST(HTMLLabel::AnimationProcessMode);
