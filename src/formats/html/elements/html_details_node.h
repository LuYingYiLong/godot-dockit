#pragma once

#include <litehtml/html_tag.h>

namespace dockit {
	litehtml::element::ptr details_summary(const litehtml::element& p_details);					// 获取 details 元素的摘要节点
	litehtml::element::ptr details_activation_target(const litehtml::element::ptr& p_candidate);// 查找 details 控件交互时应激活的目标元素

	class HTMLDetailsNode : public litehtml::html_tag {
	public:
		using litehtml::html_tag::html_tag;
		std::shared_ptr<litehtml::render_item> create_render_item(
			const std::shared_ptr<litehtml::render_item>& p_parent
		) override;																				// 创建 details 元素的布局渲染项
	};
}
