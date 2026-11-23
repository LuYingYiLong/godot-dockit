#pragma once

#include <litehtml/types.h>

namespace litehtml {
	class css_properties;
	class element;

	// 判断 HTML 文本换行边界
	bool is_explicit_wrap_point(const element& p_element);										// 检查元素是否指定了显式换行位置
	bool is_soft_hyphen(const element& p_element);												// 检查元素是否为软连字符
	bool is_hyphenation_fallback(const element& p_element);										// 检查断词是否使用了回退处理
	pixel_t measure_hyphen(const element& p_element);											// 测量连字符
	bool is_lexical_character(char32_t p_character);											// 检查词法字符
	bool extends_lexical_character(char32_t p_character);										// 检查字符是否延续当前词法单元
	bool is_grapheme_boundary(const element& p_left, const element& p_right);					// 检查两个文本片段之间是否为字素边界
	bool allows_text_boundary_wrap(
		const element& p_left,
		const element& p_right,
		const css_properties& p_style
	);																							// 根据元素样式判断两个文本片段之间是否允许换行
}
