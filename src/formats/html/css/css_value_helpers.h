#pragma once

#include "css_length.h"
#include "css_token_compat.h"

namespace litehtml {
// 解析 CSS 常用值
	bool parse_url(const css_token& token, string& url);										// 解析 CSS URL 值
	bool parse_length(
		const css_token& tok,
		css_length& length,
		int options,
		string keywords = ""
	);																							// 解析 CSS 长度值
	bool parse_angle(const css_token& tok, float& angle, bool percents_allowed = false);		// 解析 CSS 角度值
	bool parse_bg_position(const css_token_vector& tokens, int& index, css_length& x, css_length& y,
		bool convert_keywords_to_percents, int length_flags = f_length_percentage);				// 解析 CSS 背景图像定位值

	template<typename Enum>
	bool parse_keyword(const css_token& tok, Enum& val, string keywords, int first_keyword_value = 0)
	{
		int	value_index(const string& val, const string& strings, int defValue = -1, char delim = ';');
		int idx = value_index(tok.ident(), keywords);
		if (idx == -1) return false;
		val = (Enum)(first_keyword_value + idx);
		return true;
	}																							// 解析 CSS 关键字

}
