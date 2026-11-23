#pragma once

#include "css_selector_matcher.h"

#include <string>

namespace litehtml {
	class element;

	// 处理 CSS 选择器所需的 DOM 信息
	const dockit::css::SelectorDOM& selector_dom_view();										// 获取用于选择器匹配的 DOM 视图
	int selector_match_flags(
		unsigned result
	);																							// 将选择器匹配结果转换为 litehtml 标记位
	const char* get_element_language(
		const element& node
	);																							// 获取元素语言
	std::string compute_auto_direction(
		const element& node
	);																							// 根据文本首个强方向字符计算自动方向
	bool matches_element_language(
		const element& node,
		const std::vector<std::string>& ranges
	);																							// 检查元素语言是否匹配任一语言范围

} // namespace litehtml
