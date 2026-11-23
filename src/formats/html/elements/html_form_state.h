#pragma once

#include <litehtml/element.h>

#include <vector>

namespace dockit::forms {
	enum class ButtonAction {
		NONE,
		BUTTON,
		SUBMIT,
		RESET
	};

	bool is_listed(const litehtml::element& p_node);											// 检查节点是否属于表单控件列表
	bool is_disabled(const litehtml::element& p_node);											// 检查表单控件是否已禁用
	bool supports_required(const litehtml::element& p_node);									// 检查表单控件是否支持 required 属性
	bool is_read_write(const litehtml::element& p_node);										// 检查readwrite
	litehtml::element::ptr owner(const litehtml::element& p_node);								// 查找控件所属的表单
	std::vector<litehtml::element::ptr> labels(const litehtml::element& p_control);				// 获取关联到控件的 label 元素
	litehtml::element::ptr labeled_control(const litehtml::element& p_label);					// 查找通过 label 关联的表单控件
	ButtonAction button_action(const litehtml::element& p_node);								// 根据按钮类型执行对应的表单动作
	std::vector<litehtml::element::ptr> controls(const litehtml::element& p_form);				// 获取表单中的控件列表
	std::vector<litehtml::element::ptr> radio_group(const litehtml::element& p_radio);			// 获取与指定单选框互斥的单选框组
	std::vector<litehtml::element::ptr> datalist_options(const litehtml::element& p_control);	// 获取控件关联 datalist 中的选项
	litehtml::element::ptr first_invalid(const litehtml::element& p_form);						// 查找表单中的首个无效控件
	std::vector<litehtml::element::ptr> invalid_controls(const litehtml::element& p_form);		// 获取表单中所有无效控件
	bool update_validity(litehtml::element& p_control);											// 更新有效性
	void synchronize(
		const litehtml::element::ptr& p_tree,
		const litehtml::element::ptr& p_preferred = {}
	);																							// 同步控件与表单的验证状态
}

