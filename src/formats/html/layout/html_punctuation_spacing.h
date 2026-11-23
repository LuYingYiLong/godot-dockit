#pragma once

#include <litehtml/types.h>

namespace litehtml {
	class element;

	enum class spacing_punctuation { other, opening, closing, middle, narrow_opening, narrow_closing, ideographic_space };

	struct punctuation_spacing {
		spacing_punctuation kind = spacing_punctuation::other;
		punctuation_trim trim = punctuation_trim::none;
		pixel_t reduction = 0;
		pixel_t font_size = 0;
		int flags = 0;

		bool at_start(bool p_paragraph_start) const;											// 检查标点是否位于段落起始位置
		bool before(const punctuation_spacing& p_next) const;									// 判断当前标点间距是否位于另一个标点之前
		bool after(const punctuation_spacing& p_previous) const;								// 判断当前标点间距是否位于另一个标点之后
	};

// 计算标点间距
	bool is_fullwidth_spacing_punctuation(char32_t p_character);								// 检查字符是否为需要全角间距的标点
	bool is_punctuation_variation_selector(char32_t p_character);								// 检查字符是否为标点变体选择符
	punctuation_spacing get_punctuation_spacing(
		const element& p_element,
		bool p_trailing,
		pixel_t p_untrimmed_width
	);																							// 获取字符两侧适用的标点间距规则
}
