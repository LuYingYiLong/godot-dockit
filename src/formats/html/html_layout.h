#ifndef DOCRENDER_HTML_LAYOUT_H
#define DOCRENDER_HTML_LAYOUT_H

#include "elements/html_input_element.h"
#include "elements/html_textarea_element.h"
#include "dom/html_selection.h"
#include "formats/html/css/css_types.h"
#include "formats/html/css/html_animation.h"
#include "formats/html/layout/html_geometry.h"
#include "formats/html/text/html_font_config.h"
#include "formats/html/text/html_text_layout.h"
#include "formats/html/text/html_text_selection.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_set>
#include <string_view>
#include <vector>

namespace godot {
	class CanvasItem;
	class InputEvent;
	class Node;
	class StyleBox;
	class Tween;
}

namespace docrender {
	class HTMLDOM;
	class HTMLTextControlNode;
	struct HTMLAnchor {
		godot::Rect2 bounds;
		std::weak_ptr<const litehtml::element> element;
	};
	enum class HTMLScrollInput { ignored, forwarded, handled };
	struct HTMLViewportStyle {
		HTMLScrollbarAppearance scrollbar;
		std::array<litehtml::overflow, 2> overflow{litehtml::overflow_auto, litehtml::overflow_auto};
		std::array<litehtml::overflow, 2> root_overflow{litehtml::overflow_visible, litehtml::overflow_visible};
		litehtml::scrollbar_gutter gutter = litehtml::scrollbar_gutter::automatic;
		litehtml::color_scheme scheme = litehtml::color_scheme::light;
	};
	struct HTMLSelectMenuRow {
		HTMLTextLayout text;
		godot::Color color;
		godot::Color background;
		int64_t option_index;
		bool enabled;
		godot::String value;
	};
}

class HTMLLayout : public godot::RefCounted {
	GDCLASS(HTMLLayout, godot::RefCounted)

	friend class HTMLElement;
	friend class HTMLNode;
	friend class HTMLImageElement;
	friend class HTMLInputElement;
	friend class HTMLTextAreaElement;
	friend class HTMLLabel;
	friend class HTMLLabelCanvas;
	friend class HTMLRange;
	friend class HTMLAnimation;
	friend class docrender::HTMLDOM;
	int64_t document_text_offset(const docrender::HTMLBoundary& p_boundary, bool p_forward = true) const;
	docrender::HTMLBoundary text_boundary_at(int64_t p_offset) const;
	godot::TypedArray<godot::Rect2> range_client_rects(const HTMLRange& p_range) const;
	godot::Vector2i get_node_selection_offsets() const;
	bool set_node_selection_offsets(int64_t p_anchor, int64_t p_focus);
	void set_dom_selection_change_callback(const godot::Callable& p_callback);
	void set_element_scroll_callback(const godot::Callable& p_callback);

	struct State;
	std::unique_ptr<State> state;
	uint64_t parse_version = 0;
	bool uses_document_viewport = false;
	bool uses_owner_animation_clock = false;

	bool initialize_document(const godot::CharString& p_html);
	bool ensure_document_for_editing();
	docrender::HTMLDOM& get_dom() const;
	bool has_current_layout() const;
	const docrender::RenderGeometry* element_geometry(const HTMLElement& p_element) const;
	std::shared_ptr<litehtml::render_item> offset_parent_of(const litehtml::render_item& p_item) const;
	godot::Vector2 element_client_size(const HTMLElement& p_element) const;
	godot::Vector2 element_client_offset(const HTMLElement& p_element) const;
	godot::Ref<HTMLElement> element_offset_parent(const HTMLElement& p_element) const;
	godot::Rect2 element_offset_rect(const HTMLElement& p_element) const;
	godot::Vector2 element_scroll_size(const HTMLElement& p_element) const;
	godot::Vector2 element_scroll_offset(const HTMLElement& p_element) const;
	godot::Error set_element_scroll_offset(const HTMLElement& p_element, const godot::Vector2& p_offset);
	godot::Error layout_viewport(const godot::Vector2i& p_viewport_size, const godot::Vector2i& p_media_size,
		const godot::Vector2i& p_viewport_unit_size);
	godot::Error reflow(const godot::Vector2i& p_viewport_size, const godot::Vector2i& p_media_size,
		const godot::Vector2i& p_viewport_unit_size);
	void record_paint();
	bool update_auto_content_visibility(const godot::Vector2i& p_viewport_size,
		const std::unordered_set<const litehtml::element*>& p_selected,
		std::unordered_set<const litehtml::element*>& r_revealed, bool p_test_only = false);
	std::unordered_set<const litehtml::element*> selected_auto_content() const;
	void request_auto_content_update();
	bool prepare_content_focus(const godot::Ref<HTMLElement>& p_element);
	void set_content_visibility_callbacks(const godot::Callable& p_update, const godot::Callable& p_selection);
	void invalidate_styles();
	void queue_reflow();
	static void reflow_after_edit(uint64_t p_id, uint64_t p_version);
	void set_edit_callback(const godot::Callable& p_callback);
	void set_transition_callback(const godot::Callable& p_callback);
	void update_transition_subscription();
	void disconnect_transition_frames();
	void sample_transition_frame(uint64_t p_version);
	bool has_active_css_animations() const;
	void advance_css_animations(double p_delta);
	void sample_css_animations(bool p_force = false);
	double animation_clock_time() const;
	godot::Ref<HTMLAnimation> animation_handle(uint64_t p_track, bool p_transition);
	std::optional<docrender::css::PlaybackInfo> animation_info(uint64_t p_track, bool p_transition) const;
	bool control_animation(uint64_t p_track, bool p_transition, docrender::css::PlaybackCommand p_command, double p_value);
	godot::TypedArray<godot::Rect2> get_element_rects(const std::shared_ptr<litehtml::element>& p_element) const;
	std::shared_ptr<litehtml::element> hit_test_element(const godot::Vector2& p_position, bool* r_scrollbar = nullptr,
		bool p_text_selection = false) const;
	godot::Ref<HTMLElement> wrap_hit_element(std::shared_ptr<litehtml::element> p_element) const;
	bool update_hover(const std::optional<godot::Vector2>& p_position);
	godot::String get_activatable_link_uri(const godot::Ref<HTMLElement>& p_element) const;
	godot::Ref<HTMLElement> get_button_ancestor(const godot::Ref<HTMLElement>& p_element) const;
	bool is_enabled_button(const godot::Ref<HTMLElement>& p_element) const;
	std::vector<docrender::HTMLSelectMenuRow> suggestion_menu_rows(const godot::Ref<HTMLElement>& p_element) const;
	std::vector<docrender::HTMLSelectMenuRow> select_menu_rows(const godot::Ref<HTMLElement>& p_element) const;
	godot::Vector2 get_image_intrinsic_size(const HTMLElement& p_element) const;
	godot::String get_image_current_src(const HTMLElement& p_element) const;
	bool is_focusable(const godot::Ref<HTMLElement>& p_element) const;
	bool is_element_connected(const godot::Ref<HTMLElement>& p_element) const;
	godot::Ref<HTMLElement> get_focus_ancestor(const godot::Ref<HTMLElement>& p_element) const;
	std::vector<godot::Ref<HTMLElement>> get_tab_elements(bool& r_has_focusable_elements) const;
	std::optional<godot::Vector2> input_local_point(const HTMLElement& p_input, const godot::Vector2& p_point) const;
	std::optional<godot::Vector2> input_caret_point(const HTMLElement& p_input) const;
	void update_input(docrender::HTMLTextControlNode& p_input);
	std::shared_ptr<docrender::HTMLTextControlNode> text_control(const godot::Ref<HTMLElement>& p_element) const;
	std::optional<docrender::HTMLPlaneTransform> input_inverse(const HTMLElement& p_input) const;
	bool resize_textarea(const godot::Ref<HTMLElement>& p_element, const godot::Vector2& p_size, const godot::Vector2i& p_axes);
	godot::Vector2 scroll_overflow(const godot::Vector2& p_position, const godot::Vector2& p_delta,
		godot::Node* p_pan_owner, const godot::Callable& p_changed);
	void refresh_overflow_paint(bool p_moved_boxes);
	void rebuild_scroll_geometry();
	std::optional<docrender::HTMLScrollTarget> scroll_element_into_view(const litehtml::element& p_element,
		std::optional<float> p_align, bool p_allow_fallback, godot::Node& p_owner, const godot::Callable& p_changed,
		const docrender::HTMLScrollOptions* p_options = nullptr);
	bool viewport_uses_smooth_scroll() const;
	godot::Vector2i viewport_snap_axes() const;
	docrender::HTMLSnapResult snap_viewport(const godot::Vector2& p_viewport, docrender::HTMLSnapRequest p_request);
	docrender::HTMLSnapResult snap_nested(const litehtml::render_item& p_container, docrender::HTMLSnapRequest p_request);
	void resnap_nested(const godot::Vector2i& p_viewport_size);
	static godot::Ref<godot::Tween> start_scroll_tween(godot::Node& p_owner, const godot::Callable& p_update);
	static godot::Ref<godot::Tween> start_snap_timer(godot::Node& p_owner, const godot::Callable& p_callback);
	void queue_nested_snap(const std::shared_ptr<litehtml::element>& p_element, const godot::Vector2i& p_axes,
		godot::Node& p_owner, const godot::Callable& p_changed);
	void cancel_nested_snap(const litehtml::element& p_element);
	static void settle_nested_pan(uint64_t p_layout_id, uint64_t p_motion_id);
	bool settle_nested_snap(const litehtml::element& p_element, const godot::Vector2i& p_axes);
	static void advance_nested_scroll(double p_fraction, uint64_t p_layout_id, uint64_t p_motion_id);
	void cancel_nested_scroll(const litehtml::element& p_element);
	godot::Rect2 viewport_scroll_region(const godot::Rect2& p_viewport) const;
	void set_viewport_scroll_offset(const godot::Vector2i& p_offset);
	void set_viewport_gutters(const godot::Vector2& p_gutters);
	void draw_viewport_background(godot::CanvasItem& p_canvas, const godot::Vector2& p_position,
		const godot::Rect2& p_clip, const godot::Vector2& p_viewport_origin);
	docrender::HTMLViewportStyle get_viewport_style() const;
	litehtml::color_scheme get_element_color_scheme(const godot::Ref<HTMLElement>& p_element) const;
	docrender::HTMLTextLayout shape_element_text(const godot::Ref<HTMLElement>& p_element, const godot::String& p_text) const;
	bool update_preferred_color_scheme();
	godot::Vector2 viewport_contained_scroll(const godot::Vector2& p_delta) const;
	static godot::Vector2 scroll_input_steps(const godot::Ref<godot::InputEvent>& p_event);
	static bool forward_scroll_remainder(const godot::Ref<godot::InputEvent>& p_event, const godot::Vector2& p_remaining);
	void flush_input_paint();
	void repaint_input();
	bool set_interaction_pseudo(const godot::Ref<HTMLElement>& p_element, const char* p_name, bool p_enabled);
	void mark_styles_pending();
	std::string_view get_cursor_at(const godot::Vector2& p_position, bool& r_over_link) const;
	void record_css_diagnostics();
	bool refresh_cssom();
	struct CSSOMSheet;
	std::vector<CSSOMSheet> collect_stylesheets();
	godot::Error edit_css_rule(int64_t p_stylesheet, int64_t p_index, const godot::String* p_rule);
	godot::TypedArray<godot::Dictionary> get_css_diagnostics_for(const litehtml::element& p_element) const;
	godot::String get_css_diagnostic_tooltip(const godot::Vector2& p_position) const;
protected:
	static void _bind_methods();

public:
	godot::TypedArray<HTMLAnimation> get_animations(const godot::Ref<HTMLElement>& p_element = {}); // 返回 CSS 动画、过渡和脚本动画
	godot::Ref<HTMLAnimation> animate(const godot::Ref<HTMLElement>& p_element,
		const godot::TypedArray<godot::Dictionary>& p_keyframes, const godot::Dictionary& p_options = {}); // 创建并播放脚本关键帧动画
	godot::Dictionary get_computed_style(const godot::Ref<HTMLElement>& p_element, const godot::String& p_pseudo = ""); // 返回计算样式快照
	godot::String get_computed_style_property(const godot::Ref<HTMLElement>& p_element, const godot::String& p_name,
		const godot::String& p_pseudo = ""); // 返回指定属性的计算值
	godot::TypedArray<godot::Dictionary> get_stylesheets(); // 返回作者样式表及其布局内标识
	godot::PackedStringArray get_css_rules(int64_t p_stylesheet); // 返回样式表顶层规则的快照
	godot::Error insert_rule(int64_t p_stylesheet, const godot::String& p_rule, int64_t p_index = 0); // 插入一条顶层 CSS 规则
	godot::Error delete_rule(int64_t p_stylesheet, int64_t p_index); // 删除指定的顶层 CSS 规则
	bool supports_css(const godot::String& p_property_or_condition, const godot::String& p_value = "") const; // 验证 CSS 属性值或支持条件
	bool supports_selector(const godot::String& p_selector) const; // 验证 CSS 选择器是否受支持
	bool matches_media(const godot::String& p_query) const; // 查询当前布局是否匹配媒体条件

	void set_css_diagnostics_enabled(bool p_enabled);								// 设置是否收集 CSS 声明诊断
	bool is_css_diagnostics_enabled() const;										// 返回是否收集 CSS 声明诊断
	godot::TypedArray<godot::Dictionary> get_css_diagnostics() const;				// 返回最近一轮样式计算的诊断快照

	struct LinkHit {
		godot::String uri;
		uintptr_t id = 0;
	};

	HTMLLayout();
	~HTMLLayout() override;

	godot::Error parse(const godot::String& p_html,
		const godot::String& p_base_dir = "res://",
		const godot::String& p_stylesheet = ""
	);																				// 解析 HTML 和附加样式，替换当前布局的文档
	godot::Error parse_source(const godot::String& p_html,
		const godot::String& p_base_dir,
		const godot::String& p_stylesheet,
		const godot::Dictionary& p_textures,
		bool p_fit_images = false,
		const godot::String& p_default_stylesheet = {},
		const docrender::HTMLFontConfig& p_fonts = {},
		const godot::Dictionary& p_local_stylesheets = {},
		const godot::Dictionary& p_local_fonts = {},
		const godot::Dictionary& p_image_dpi = {},
		const godot::Dictionary& p_image_orientations = {},
		const godot::Dictionary& p_svg_clip_sources = {}
	);																				// 使用指定字体、样式表和资源依赖解析 HTML 并替换当前文档
	godot::Error layout(const godot::Vector2i& p_viewport_size);					// 按给定视口尺寸排版并更新几何
	godot::Error layout_embedded_block(const godot::Vector2i& p_viewport_size);	// 为外层排版器保留完整块的几何和文字
	godot::Vector2i get_content_size() const;										// 返回最近一次成功排版的内容尺寸
	godot::String get_plain_text() const;											// 返回排版后的可复制文本
	godot::String get_accessible_text() const;									// 返回应用 CSS 替代文本后的无障碍文本
	int64_t hit_test_text(const godot::Vector2& p_position) const;					// 返回离指定文档坐标最近的文本光标偏移
	docrender::HTMLSelectionPoint selection_point_at(const godot::Vector2& p_position) const;
	godot::String get_copyable_text(int64_t p_from, int64_t p_to) const;
	godot::TypedArray<godot::Rect2> get_selection_rects(
		int64_t p_from,
		int64_t p_to
	) const;																		// 返回文本范围在文档坐标中的可见选区矩形
	void draw(godot::CanvasItem* p_canvas,
		const godot::Vector2& p_position = {},
		const godot::Rect2& p_clip = {}
	);																				// 在画布绘制回调中按给定位置和裁剪区域绘制 HTML
	void draw_with_highlights(
		godot::CanvasItem* p_canvas,
		const godot::Vector2& p_position,
		const godot::Rect2& p_clip,
		const godot::Vector2i& p_selection,
		const godot::StyleBox* p_selection_style,
		const godot::Color& p_selection_color,
		const godot::Vector2i& p_search = {},
		const godot::StyleBox* p_search_style = nullptr,
		const godot::Vector2& p_viewport_origin = {}
	);																				// 在指定画布上绘制 HTML，并显示选区与搜索高亮
	godot::String get_link_at(const godot::Vector2& p_position) const;				// 返回指定文档坐标处链接的原始 href
	LinkHit find_link_at(
		const godot::Vector2& p_position,
		godot::Ref<HTMLElement>* r_element = nullptr
	) const;																		// 命中文档坐标处的链接，可同时写出命中的元素
	godot::Rect2 get_anchor_rect(const godot::String& p_id) const;					// 返回指定 HTML ID 的锚点矩形
	const godot::HashMap<godot::String, docrender::HTMLAnchor>& get_anchors() const; // 提供当前布局的锚点位置及节点
	std::optional<docrender::HTMLScrollTarget> scroll_anchor_into_view(const godot::String& p_id, float p_align,
		godot::Node& p_owner, const godot::Callable& p_changed = {});
	std::optional<docrender::HTMLScrollTarget> scroll_element_into_view(const godot::Ref<HTMLElement>& p_element,
		std::optional<float> p_align, godot::Node& p_owner, const godot::Callable& p_changed,
		const docrender::HTMLScrollOptions* p_options = nullptr);
	void cancel_scroll_activity();
	godot::Ref<HTMLElement> create_element(const godot::String& p_tag_name);		// 创建同一布局中尚未插入文档的元素
	godot::Ref<HTMLDocumentFragment> create_document_fragment();					// 创建属于当前布局的空片段
	godot::Ref<HTMLDocumentNode> get_document_node() const; // 返回实时文档的节点句柄
	godot::Ref<HTMLRange> create_range(); // 创建关联当前文档的实时范围
	godot::Ref<HTMLSelection> get_selection() const; // 返回文档的节点选区
	godot::Ref<HTMLElement> get_document_element() const; // 返回文档的根元素
	godot::Ref<HTMLElement> get_head() const; // 返回文档的 head 元素
	godot::Ref<HTMLElement> get_body() const; // 返回文档的 body 或 frameset 元素
	godot::String get_title() const; // 返回归并空白后的文档标题
	godot::Error set_title(const godot::String& p_title); // 设置当前布局中的文档标题
	godot::TypedArray<HTMLElement> get_elements_by_tag_name(const godot::String& p_name) const; // 按标签名查找文档元素
	godot::TypedArray<HTMLElement> get_elements_by_class_name(const godot::String& p_names) const; // 按类名查找文档元素
	godot::TypedArray<HTMLElement> get_elements_by_name(const godot::String& p_name) const; // 按 name 属性查找文档元素
	godot::Ref<HTMLTextNode> create_text_node(const godot::String& p_data); // 创建分离文本节点
	godot::Ref<HTMLCommentNode> create_comment(const godot::String& p_data); // 创建分离注释节点
	godot::Ref<HTMLDocumentFragment> parse_fragment(const godot::String& p_html, const godot::Ref<HTMLElement>& p_context = {}); // 解析可独立插入的片段
	godot::Ref<HTMLDocumentFragment> import_fragment(const godot::Ref<HTMLDocumentFragment>& p_fragment); // 深复制片段到当前布局
	godot::Ref<HTMLDocumentFragment> adopt_fragment(const godot::Ref<HTMLDocumentFragment>& p_fragment); // 将分离片段迁到当前布局
	godot::Ref<HTMLElement> import_node(const godot::Ref<HTMLElement>& p_element, bool p_deep = false);	// 在当前布局中创建分离副本，可选复制后代
	godot::Ref<HTMLElement> adopt_node(const godot::Ref<HTMLElement>& p_element);	// 将整棵子树迁到当前布局，保持分离并保留原句柄
	godot::Ref<HTMLElement> query_selector(const godot::String& p_selector) const;	// 按文档顺序返回第一个匹配元素
	godot::TypedArray<HTMLElement> query_selector_all(
		const godot::String& p_selector
	) const;																		// 按文档顺序返回所有匹配元素的去重数组
	godot::Ref<HTMLElement> get_element_by_id(const godot::String& p_id) const;		// 按区分大小写的 HTML ID 查找第一个元素
	godot::Ref<HTMLElement> get_element_at(const godot::Vector2& p_position) const; // 返回文档坐标处考虑裁剪后的最上层元素
	godot::TypedArray<HTMLElement> get_elements_at(const godot::Vector2& p_position) const; // 按命中顺序返回文档坐标下的元素
	std::optional<docrender::HTMLScrollbarDrag> scrollbar_at(const godot::Vector2& p_position) const; // 获取命中的滚动条和滑块抓取位置
	void update_scrollbar_hover(const std::optional<godot::Vector2>& p_position); // 更新滚动条的可见悬停状态
	void set_scrollbar_pressed(const std::optional<docrender::HTMLScrollbarDrag>& p_drag); // 更新滚动条的可见按压状态
	void drag_scrollbar(const docrender::HTMLScrollbarDrag& p_drag, const godot::Vector2& p_position); // 按文档坐标移动滑块
	void finish_scrollbar_drag(const docrender::HTMLScrollbarDrag& p_drag);
	docrender::HTMLScrollInput scroll_overflow_from_input(const godot::Vector2& p_position, const godot::Ref<godot::InputEvent>& p_event,
		godot::Node& p_owner, const godot::Callable& p_changed = {});
	std::optional<godot::Rect2> get_element_scroll_bounds(
		const godot::Ref<HTMLElement>& p_element
	) const; // 获取同一布局中目标元素可用于滚动定位的边界
	godot::String get_render_error() const;											// 返回最近的解析或排版错误
	void clear();																	// 清空文档及排版结果并使元素句柄失效
};

#endif
