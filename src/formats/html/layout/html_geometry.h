#pragma once

#include "html_plane_transform.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace godot { struct Projection; }

namespace litehtml {
	class element;
	class render_item;
	struct border_radiuses;
	struct css_clip_path;
	struct position;
	enum class scrollbar_width;
	enum class color_scheme : uint8_t;
	enum class system_color : uint8_t;
	struct css_scroll_snap_type;
	struct css_transform_function;
}

namespace dockit::css { class ComputedStyle; }

namespace dockit {
	struct HTMLClipPolygon;
	inline constexpr int MAX_ROUNDED_CLIPS = 16;

	godot::Rect2 to_rect(const litehtml::position& p_position);									// 将几何边界转换为矩形

	// Drawing offsets map document coordinates to the target canvas. Box and text
	// hit tests use the inverse translation to reach their local coordinates.
	inline godot::Rect2 translate_rect(const godot::Rect2& p_rect, const godot::Vector2& p_offset) {
		return { p_rect.position + p_offset, p_rect.size };
	}

	inline godot::Vector2 to_local_point(const godot::Vector2& p_point, const godot::Vector2& p_origin) {
		return p_point - p_origin;
	}																							// 将坐标转换为对象的局部坐标

	struct RoundedRect {
		godot::Rect2 rect;
		godot::Vector4 radii_x;
		godot::Vector4 radii_y;

		RoundedRect(const godot::Rect2& p_rect, const litehtml::border_radiuses& p_radii);
		RoundedRect inset(const godot::Rect2& p_rect) const;									// 返回内缩到指定矩形并相应缩小圆角后的形状
		RoundedRect outset(const godot::Rect2& p_rect) const;									// 按各边距更新圆角，使用外阴影的扩张公式
		RoundedRect spread(godot::real_t p_distance) const;										// 按阴影扩张距离调整边界和圆角
		bool has_radius() const;																// 检查是否存在有效的圆角
		bool has_point(const godot::Vector2& p_position) const;									// 检查指定坐标是否在圆角矩形内
	};

	enum class ClipAxes { both, x, y, horizon };

	struct TransformedClip {
		RoundedRect shape;
		HTMLPlaneTransform matrix;
		ClipAxes axes = ClipAxes::both;
		bool is_path = false;
		std::shared_ptr<const HTMLClipPolygon> polygon = {};

		bool needs_precise_geometry() const {
			return polygon || axes != ClipAxes::both || shape.has_radius() || matrix != godot::Transform2D();
		}																						// 检查命中测试是否需要精确几何信息
		bool contains_point(const godot::Vector2& p_point, bool p_precise) const;				// 检查几何区域是否包含指定坐标
	};

	// Visit overflow clips from the closest ancestor outward. The element's own
	// shape is applied separately, and the remaining ancestors use their bounds.
	inline bool use_precise_clip(const TransformedClip& p_clip, int& r_remaining) {
		if (p_clip.is_path || p_clip.axes == ClipAxes::horizon) return true;
		if (r_remaining == 0 || !p_clip.needs_precise_geometry()) {
			return false;
		}
		--r_remaining;
		return true;
	}

	godot::Rect2 clipped_visual_bounds(const godot::Rect2& p_rect, const HTMLPlaneTransform& p_matrix,
		const std::vector<TransformedClip>& p_clips);											// 获取经过裁剪后的视觉边界
	HTMLClipPolygon svg_basic_clip_polygon(const litehtml::css_clip_path& p_path, const godot::Rect2& p_box,
		const godot::Transform2D& p_matrix, const godot::Transform2D& p_visual_matrix);			// 将基础裁剪形状映射到 SVG 图形的坐标系

	// Geometry uses document coordinates after litehtml has placed and scrolled
	// the boxes. A link to the next ancestor shares clip geometry across siblings.
	struct OverflowClip : TransformedClip {
		HTMLPlaneTransform inverse;
		litehtml::render_item* item;
		const OverflowClip* parent;
	};

	struct HTMLScrollbarColors {
		godot::Color thumb;
		godot::Color track;
		bool operator==(const HTMLScrollbarColors& p_other) const;								// 比较两个值是否相等
	};

	struct HTMLScrollbarAppearance {
		litehtml::scrollbar_width width{};
		std::optional<HTMLScrollbarColors> colors;
		litehtml::color_scheme scheme{};
		bool operator==(const HTMLScrollbarAppearance& p_other) const;							// 比较两个值是否相等
		float thickness() const;																// 获取边框对应方向的厚度
		float thumb_thickness(bool p_hot) const;												// 计算滚动条滑块的绘制厚度
		godot::Color thumb_color(bool p_pressed) const;											// 获取滚动条滑块在指定按压状态下的颜色
		godot::Color track_color() const;														// 获取滚动条轨道颜色
		godot::Rect2 visible_thumb(const godot::Rect2& p_thumb, int p_axis, bool p_hot) const;	// 根据悬停状态计算滚动条滑块的可见矩形
	};

	HTMLScrollbarAppearance html_scrollbar_appearance(const css::ComputedStyle& p_style);		// 根据计算样式确定滚动条外观
	godot::Color html_system_color(
		litehtml::system_color p_color,
		litehtml::color_scheme p_scheme
	);																							// 将 HTML 系统颜色转换为 Godot 颜色

	struct HTMLScrollTarget {
		godot::Rect2 area;
		bool viewport_fixed;
		const litehtml::element* element;
	};

	struct HTMLScrollOptions {
		enum class Alignment { START, CENTER, END, NEAREST };
		enum class Behavior { AUTO, INSTANT, SMOOTH };
		Alignment block = Alignment::START;
		Alignment inline_axis = Alignment::NEAREST;
		Behavior behavior = Behavior::AUTO;
		bool smooth(
			bool p_default
		) const {
			return behavior == Behavior::AUTO ? p_default : behavior == Behavior::SMOOTH;
		}																						// 获取滚动行为是否应使用平滑动画
	};

	enum class HTMLSnapMethod { destination, direction, relayout };
	struct HTMLSnapArea {
		godot::Rect2 area;
		const css::ComputedStyle* style;
		const litehtml::element* element;
	};

	struct HTMLSnapRequest {
		godot::Vector2 start;
		godot::Vector2 end;
		godot::Vector2 limit;
		godot::Vector2i axes;
		HTMLSnapMethod method;
		std::array<const litehtml::element*, 2> preferred{};
	};

	struct HTMLSnapResult {
		godot::Vector2 offset;
		godot::Vector2i stopped;
		std::array<const litehtml::element*, 2> areas{};
	};

	HTMLSnapResult html_select_scroll_snap(const css::ComputedStyle& p_style, const godot::Rect2& p_snapport,
		const std::vector<HTMLSnapArea>& p_areas, const HTMLSnapRequest& p_request);			// 根据 select 菜单的滚动位置计算吸附结果

	godot::Rect2 html_scroll_margin_area(
		const godot::Rect2& p_box,
		const css::ComputedStyle& p_style
	);																							// 计算 HTML 元素的滚动外边距区域
	godot::Rect2 html_scroll_padding_area(
		const godot::Rect2& p_port,
		const css::ComputedStyle& p_style
	);																							// 计算 HTML 元素的滚动内边距区域
	godot::Vector2 html_scroll_delta(const godot::Rect2& p_target, const godot::Rect2& p_port,
		std::optional<float> p_align, const HTMLScrollOptions* p_options = nullptr);			// 根据目标位置和对齐方式计算滚动增量

	struct HTMLScrollbar {
		godot::Rect2 track;
		godot::Rect2 thumb;
		godot::real_t range = 0;
	};
	HTMLScrollbar make_html_scrollbar(const godot::Rect2& p_track, int p_axis,
		godot::real_t p_page, godot::real_t p_range, godot::real_t p_offset);					// 构造HTML滚动条
	godot::Vector2i html_resize_axes(const css::ComputedStyle& p_style);						// 获取允许调整大小的坐标轴
	godot::Rect2 html_resize_grip(const godot::Rect2& p_box);									// 计算元素边角的调整大小手柄区域

	struct HTMLScrollbarDrag {
		std::weak_ptr<litehtml::element> element;
		int axis;
		godot::real_t grab;
	};

	struct RenderGeometry {
		std::shared_ptr<litehtml::render_item> item;
		std::vector<RoundedRect> fragments;
		std::vector<godot::Rect2> visual_boxes;
		std::optional<godot::Rect2> anchor_fallback;
		HTMLPlaneTransform matrix;
		std::optional<HTMLPlaneTransform> inverse;
		const OverflowClip* overflow;
		bool viewport_fixed_subtree;
		const OverflowClip* child_overflow = nullptr;
		const OverflowClip* path_clip = nullptr;
		const litehtml::render_item* layout_overflow_boundary = nullptr;
		std::optional<godot::Vector3> depth_plane;												// Coefficients of Z/W in document space
		std::optional<godot::Vector3> enclosing_depth_plane;
		bool starts_3d_context = false;
		bool flattens_3d_context = false;

		std::array<HTMLScrollbar, 2> scrollbars() const;										// 计算两个轴的覆盖式滚动条
		godot::Rect2 resize_grip() const;														// 获取元素的调整大小手柄区域
		bool contains_visible_point(const godot::Vector2& p_point) const;						// 检查坐标是否落在元素可见区域内
		bool has_visible_area(const godot::Rect2& p_box) const;									// 检查是否包含可见性区域
	};

	class HTMLGeometryIndex {
		using ElementGeometry = std::vector<const RenderGeometry*>;

		// Renderer and clip addresses are stable for the lifetime of one reflow.
		std::deque<OverflowClip> clips;
		std::unordered_map<const litehtml::render_item*, RenderGeometry> renderers;
		godot::HashMap<const litehtml::element*, ElementGeometry> elements;
		std::vector<const RenderGeometry*> snap_areas;
		std::vector<const RenderGeometry*> auto_content_boxes;
		std::vector<std::shared_ptr<litehtml::render_item>> snap_containers;
		bool transforms = false;
		bool sticky = false;
		bool group_effects = false;
		bool element_blending = false;
		bool backdrop_filtering = false;
		bool overflow_controls = false;
		bool top_layer = false;
		bool indexing_top_layer = false;
		godot::Rect2 viewport;

		struct SpatialParent;
		void index(const std::shared_ptr<litehtml::render_item>& p_item, const OverflowClip* p_overflow,
			const HTMLPlaneTransform& p_parent_matrix, bool p_viewport_fixed_subtree,
			const godot::Rect2& p_scrollport, const godot::Rect2& p_containing_box, const godot::Vector2& p_sticky_offset,
			const litehtml::render_item* p_layout_overflow_boundary, bool p_transform_culled = false,
			const godot::Projection* p_parent_perspective = nullptr, const SpatialParent* p_spatial_parent = nullptr,
			std::optional<godot::Vector3> p_flattened_depth = std::nullopt);

	public:
		HTMLGeometryIndex() = default;
		HTMLGeometryIndex(const HTMLGeometryIndex&) = delete;
		HTMLGeometryIndex& operator=(const HTMLGeometryIndex&) = delete;						// 将另一个值赋给当前对象

		void rebuild(const std::shared_ptr<litehtml::render_item>& p_root, const godot::Rect2& p_viewport = {},
			const std::vector<std::shared_ptr<litehtml::render_item>>& p_top_layer = {});		// 重新构建当前索引数据
		void clear();																			// 清除当前存储的数据
		const ElementGeometry* find_element(const litehtml::element* p_element) const;			// 查找元素
		const RenderGeometry* find_renderer(const litehtml::render_item* p_item) const;			// 查找渲染器
		std::optional<godot::Projection> css_transform_matrix(const litehtml::element& p_element,
			const std::vector<litehtml::css_transform_function>& p_transforms) const;			// 将 CSS 变换函数转换为二维变换矩阵
		bool can_interpolate_transform(const litehtml::element& p_element,
			const std::vector<litehtml::css_transform_function>& p_sample) const;				// 检查是否可以interpolate变换
		const std::vector<const RenderGeometry*>& get_snap_areas() const { return snap_areas; }	// 获取可参与滚动吸附的区域列表
		const std::vector<const RenderGeometry*>& get_auto_content_boxes() const {
			return auto_content_boxes;
		}																						// 获取自动内容可见性对应的元素盒子
		const std::vector<std::shared_ptr<litehtml::render_item>>& get_snap_containers() const {
			return snap_containers;
		}																						// 获取可参与滚动吸附的容器
		bool has_transforms() const { return transforms; }										// 检查是否包含变换
		bool has_element_blending() const { return element_blending; }							// 检查是否包含元素混合模式
		bool has_backdrop_filtering() const { return backdrop_filtering; }						// 检查是否包含背景滤镜滤镜
		bool has_sticky() const { return sticky; }												// 检查是否包含粘性定位
		bool uses_visual_hit_order() const {
			return transforms || sticky || group_effects || top_layer;
		}																						// 检查命中测试是否按视觉绘制顺序执行
		bool has_overflow_controls() const { return overflow_controls; }						// 检查是否包含溢出控件
		godot::Vector2 extend_overflow(const godot::Vector2& p_size) const;						// 计算包含溢出内容后的扩展尺寸
	};
}
