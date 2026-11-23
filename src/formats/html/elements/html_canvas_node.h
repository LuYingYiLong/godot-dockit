#pragma once

#include "html_canvas_element.h"

#include <litehtml/html_tag.h>

namespace dockit {
	class HTMLCanvasNode final : public litehtml::html_tag {
	public:
		using html_tag::html_tag;
		godot::Ref<HTMLCanvasRenderingContext2D> context;

		bool is_replaced() const override { return true; }										// 检查替换元素
		void parse_attributes() override;														// 解析属性
		void get_content_size(litehtml::size& r_size, litehtml::pixel_t p_width) override;		// 获取内容尺寸
		std::shared_ptr<litehtml::render_item> create_render_item(
			const std::shared_ptr<litehtml::render_item>& p_parent
		) override;																				// 创建 Canvas 元素的布局渲染项
		void draw(
			litehtml::uint_ptr p_hdc,
			litehtml::pixel_t p_x,
			litehtml::pixel_t p_y,
			const litehtml::position* p_clip,
			const std::shared_ptr<litehtml::render_item>& p_item
		) override;																				// 绘制 Canvas 元素的图像内容
	};
}
