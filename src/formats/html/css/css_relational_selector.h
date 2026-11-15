#pragma once

#include <litehtml/css_selector.h>

namespace litehtml {
	class element;

	// 查询期间覆盖 :scope，退出后恢复外层查询上下文
	class selector_scope {
		const element* previous;

	public:
		explicit selector_scope(const element* p_root);
		~selector_scope();
		selector_scope(const selector_scope&) = delete;
		selector_scope& operator=(const selector_scope&) = delete;
	};

	bool matches_selector_scope(const element& p_element);
	const char* get_element_language(const element& p_element);
	bool matches_element_language(const element& p_element, const string_vector& p_ranges);
	bool matches_relative_selector(const element& p_anchor, const css_selector::vector& p_selectors);
}
