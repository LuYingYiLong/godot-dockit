#pragma once

#include <litehtml/render_block.h>

namespace docrender {
	// Controls paint their current label themselves, but its intrinsic width still participates in layout.
	class HTMLControlRenderItem final : public litehtml::render_item_block {
		litehtml::size intrinsic;

	protected:
		litehtml::rendered_width _render_content(litehtml::pixel_t, litehtml::pixel_t, bool,
			const litehtml::containing_block_context&, litehtml::formatting_context*) override {
			m_pos.height = intrinsic.height;
			return {intrinsic.width, intrinsic.width};
		}

	public:
		HTMLControlRenderItem(const litehtml::element::ptr& p_element, litehtml::size p_intrinsic) :
			render_item_block(p_element), intrinsic(p_intrinsic) {}

		std::shared_ptr<litehtml::render_item> init() override {
			src_el()->add_render(shared_from_this());
			return shared_from_this();
		}

		std::shared_ptr<litehtml::render_item> clone() override {
			return std::make_shared<HTMLControlRenderItem>(src_el(), intrinsic);
		}
	};
}
