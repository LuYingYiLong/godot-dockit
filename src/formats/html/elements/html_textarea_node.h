#pragma once

#include "html_text_control_node.h"
#include "html_node_copy_state.h"

namespace dockit {
	class HTMLTextAreaNode final : public HTMLTextControlNode, public HTMLNodeCopyState {
		godot::Rect2 resize_handle;
		godot::Vector2 border_extras;
		godot::Vector2 border_size;

	public:
		explicit HTMLTextAreaNode(const std::shared_ptr<litehtml::document>& p_document);
		bool is_text_control() const override { return true; }									// 检查文本控件
		bool is_replaced() const override { return true; }										// 检查替换元素
		bool update_pseudos() override;															// 更新伪类
		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override;// 将当前控件状态复制到目标节点
		bool appendChild(const litehtml::element::ptr& p_child) override;						// 将节点追加为当前节点的最后一个子节点
		void parse_attributes() override;														// 解析属性
		godot::String get_default_value() const;												// 获取默认值值
		void sync_default_value();																// 同步默认值值
		std::shared_ptr<litehtml::render_item> create_render_item(
			const std::shared_ptr<litehtml::render_item>& p_parent
		) override;																				// 创建 textarea 控件的布局渲染项
		void draw(
			litehtml::uint_ptr p_hdc,
			litehtml::pixel_t p_x,
			litehtml::pixel_t p_y,
			const litehtml::position* p_clip,
			const std::shared_ptr<litehtml::render_item>& p_item
		) override;																				// 绘制 textarea 控件及其当前内容
		godot::Vector2i resize_axes() const;													// 获取 textarea 允许调整大小的坐标轴
		godot::Rect2 get_resize_handle() const { return resize_handle; }						// 获取调整大小句柄
		godot::Vector2 get_border_size() const { return border_size; }							// 获取边框尺寸
		godot::Vector2 get_border_extras() const { return border_extras; }						// 获取边框附加项
	};
}
