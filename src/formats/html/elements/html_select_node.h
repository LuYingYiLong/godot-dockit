#pragma once

#include "html_node_copy_state.h"

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <litehtml/html_tag.h>

#include <optional>
#include <vector>

namespace dockit {
	class HTMLSelectNode;
	class HTMLOptionNode : public litehtml::html_tag, public HTMLNodeCopyState {
		friend class HTMLSelectNode;
		std::optional<bool> selected;
		bool dirty = false;
		void assign_selected(bool p_selected, bool p_dirty);

	public:
		using litehtml::html_tag::html_tag;
		HTMLSelectNode* select() const;															// 获取所属的 select 控件
		godot::String get_value() const;														// 获取值
		godot::String get_label() const;														// 获取挂载此行为脚本的 HTMLLabel
		bool is_selected() const;																// 检查选中项
		void set_selected(bool p_selected);														// 设置选中项
		void attribute_changed();																// 响应 select 元素属性变化并更新控件状态
		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override;// 将当前控件状态复制到目标节点
	};

	class HTMLSelectNode : public litehtml::html_tag, public HTMLNodeCopyState {
		bool dropdown_open = false;
		bool allow_empty = false;
		godot::Rect2 list_box;
		double row_height = 1.0;
		int scroll_row = 0;
		int active_row = -1;
		int anchor_row = -1;
		struct SelectionSnapshot {
			std::weak_ptr<HTMLOptionNode> option;
			bool selected;
		};
		std::vector<SelectionSnapshot> selection_snapshot;
		godot::String selected_value_snapshot;
		bool has_selection_snapshot = false;

	public:
		using litehtml::html_tag::html_tag;
		using Options = std::vector<std::shared_ptr<HTMLOptionNode>>;
		Options get_options() const;															// 获取选项
		Options get_list_options() const;														// 获取列表选项
		void use_single_selection();															// 按单选模式应用选区范围
		bool is_multiple() const { return get_attr("multiple") != nullptr; }					// 检查 select 是否允许同时选择多个选项
		int64_t get_size() const;																// 获取尺寸
		bool is_list_box() const { return is_multiple() || get_size() > 1; }					// 检查列表盒子
		void set_dropdown_open(bool p_open) { dropdown_open = p_open; }							// 设置下拉列表open
		int64_t display_size() const;															// 获取下拉列表的可见行数
		bool is_option_selected(const HTMLOptionNode& p_option) const;							// 检查选项选中项
		int list_row_at(const godot::Vector2& p_point) const;									// 查找指定坐标对应的列表行
		int list_active_row() const { return active_row; }										// 获取下拉列表当前活动行
		bool scroll_list(int p_rows);															// 按指定行数滚动下拉列表
		bool select_list_row(int p_row, bool p_extend, bool p_toggle, bool p_move_only = false);// 选择下拉列表中的指定行
		int64_t selected_index(const Options& p_options) const;									// 获取当前选项在列表中的索引
		void set_selected_index(const Options& p_options, int64_t p_index);						// 设置选中项index
		bool is_missing() const;																// 检查 required select 是否缺少有效选项
		void reset();																			// 重置 select 控件的交互状态
		void sync_mutation_revision();															// 同步变更修订号
		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override;// 将当前控件状态复制到目标节点
		std::shared_ptr<litehtml::render_item> create_render_item(
			const std::shared_ptr<litehtml::render_item>& p_parent
		) override;																				// 创建 select 控件的布局渲染项
		void draw(
			litehtml::uint_ptr p_hdc,
			litehtml::pixel_t p_x,
			litehtml::pixel_t p_y,
			const litehtml::position* p_clip,
			const std::shared_ptr<litehtml::render_item>& p_item
		) override;																				// 绘制 select 控件及其选项状态
	};
}
