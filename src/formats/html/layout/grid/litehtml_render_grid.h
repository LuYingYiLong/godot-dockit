#pragma once

#include <litehtml/render_block.h>

namespace litehtml {
	struct grid_subgrid_layout;

	size size_grid_image(const render_item& item, const containing_block_context& self, const size& intrinsic,
		bool use_intrinsic_ratio = true);														// 按网格单元尺寸布置图像并可保留其固有宽高比

	class render_item_grid : public render_item_block {
		const grid_subgrid_layout* shared_tracks = nullptr;
		size track_extent;
		pixel_t occupied_row_height = 0;
		std::optional<pixel_t> first_baseline;
		std::optional<pixel_t> last_baseline;

		rendered_width _render(pixel_t x, pixel_t y, const containing_block_context& containing_block_size,
			formatting_context* fmt_ctx, bool second_pass) override;
		size layout_overflow() const override { return track_extent; }
		std::optional<pixel_t> find_layout_baseline(bool last) const override;

	public:
		explicit render_item_grid(const std::shared_ptr<element>& element) : render_item_block(element) {}
		pixel_t get_first_baseline() override { return baseline_or_border_bottom(false); }		// 获取第一行内容的基线位置
		pixel_t get_last_baseline() override { return baseline_or_border_bottom(true); }		// 获取最后一行内容的基线位置
		pixel_t get_inline_baseline() override {
			return layout_baseline(false).value_or(height());
		}																						// 获取行内内容的基线位置
		pixel_t baseline_content_height() const override { return occupied_row_height; }		// 计算内容区域相对于基线的高度
		std::shared_ptr<render_item> init() override;											// 根据布局上下文初始化网格渲染项
		std::shared_ptr<render_item> clone() override {
			return std::make_shared<render_item_grid>(src_el());
		}																						// 复制当前网格渲染项
	};
}
