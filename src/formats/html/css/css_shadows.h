#pragma once

#include "css_length.h"
#include "css_web_color.h"

#include <vector>

namespace litehtml {
	struct box_shadow {
		css_length x;
		css_length y;
		css_length blur;
		css_length spread;
		web_color color = web_color::current_color;
		bool inset = false;
	};

	// 解析 CSS 阴影
	bool parse_box_shadows(
		const css_token_vector& p_tokens,
		document_container* p_container,
		std::vector<box_shadow>& r_shadows
	);																							// 解析 CSS box-shadow 列表
	bool parse_text_shadows(
		const css_token_vector& p_tokens,
		document_container* p_container,
		std::vector<box_shadow>& r_shadows
	);																							// 解析 CSS text-shadow 列表
}
