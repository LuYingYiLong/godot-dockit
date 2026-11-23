#pragma once

#include "css_computed_style.h"
#include "css_font_description.h"
#include "css_property_value.h"
#include "css_units.h"

namespace dockit::css {
	// The host supplies the display scale and resource queries for the current document.
	class StyleHost {
	public:
		virtual ~StyleHost() = default;
		virtual pixel_t default_font_size() const = 0;											// 获取默认字号
		virtual pixel_t default_fixed_font_size() const { return default_font_size(); }			// 获取固定宽度字体的默认字号
		virtual font_metrics initial_font_metrics() const = 0;									// 获取初始字体度量信息
		virtual pixel_t points_to_pixels(float points) const = 0;								// 将磅值转换为当前文档使用的像素值
		virtual css_container_units container_units() const = 0;								// 获取当前容器单位上下文
		virtual uint_ptr get_font(
			const font_description& description,
			font_metrics* metrics
		) const = 0;																			// 根据字体描述获取字体及度量信息
		virtual void compute_image(
			image& value,
			const string& base,
			const font_metrics& metrics,
			const css_image_resolution& resolution,
			css_image_orientation orientation,
			color_scheme scheme
		) const = 0;																			// 根据样式和分辨率计算图像资源
	};

	struct ComputeContext {
		const void* declarations;
		const property_value& (*read_declaration)(const void*, string_id);
		const ComputedStyle* parent;
		const StyleHost& host;
		const media_features& viewport;
		const font_metrics* root_font;
		bool is_root;
		bool is_replaced;
		const ComputedStyle* box_parent = nullptr;
		bool suppress_contents_display = false;
		bool allow_content_replacement = false;
		bool replacement_ignores_size_hints = false;

		pixel_t to_pixels(const css_length& length, const font_metrics& metrics,
			pixel_t basis, css_length_context overrides = {}) const;							// 将 CSS 长度转换为像素值
		void cvt_units(css_length& length, const font_metrics& metrics, pixel_t basis) const;	// 将 CSS 长度单位转换为像素值

	private:
		UnitContext unit_context(const font_metrics& metrics, css_length_context overrides = {}) const;
	};
}
