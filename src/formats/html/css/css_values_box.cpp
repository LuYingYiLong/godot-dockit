// Portions adapted from litehtml.
// Copyright (c) 2013, Yuri Kobets (tordex). All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// See addons/docrender/LiteHTML-LICENSES/litehtml.txt for the license and disclaimer.

#include "css_value_parser.h"
#include "formats/html/litehtml_css_math.h"

#include <litehtml/css_parser.h>

namespace litehtml {

namespace {
	std::optional<css_aspect_ratio> parse_aspect_ratio(const css_token_vector& tokens)
	{
		std::size_t first = 0;
		std::size_t end = tokens.size();
		const bool leading_auto = end && tokens.front().ident() == "auto";
		const bool trailing_auto = end > 1 && tokens.back().ident() == "auto";
		if (leading_auto) ++first;
		if (trailing_auto) --end;
		if (leading_auto && trailing_auto) return std::nullopt;
		if (first == end) return leading_auto ? std::optional(css_aspect_ratio{}) : std::nullopt;

		const auto count = end - first;
		if (count != 1 && (count != 3 || tokens[first + 1].type != '/')) return std::nullopt;
		const auto component = [](const css_token& token) -> std::optional<double> {
			if (token.type == NUMBER)
				return std::isfinite(token.n.number) && token.n.number >= 0 ? std::optional<double>(token.n.number) : std::nullopt;
			const auto number = evaluate_css_scalar_math(token, css_math_scalar::number);
			if (!number || !std::isfinite(*number)) return std::nullopt;
			return std::max(0.0, *number);
		};
		const auto width = component(tokens[first]);
		const auto height = count == 3 ? component(tokens[first + 2]) : std::optional(1.0);
		if (!width || !height) return std::nullopt;
		const double ratio = *width > 0 && *height > 0 ? *width / *height : 0;
		return css_aspect_ratio{std::isfinite(ratio) ? ratio : 0.0, leading_auto || trailing_auto};
	}

	bool parse_intrinsic_width(const css_token& value, css_length& width)
	{
		const auto keyword = value.ident();
		auto kind = css_intrinsic_size::none;
		if (keyword == "min-content") kind = css_intrinsic_size::min_content;
		else if (keyword == "max-content") kind = css_intrinsic_size::max_content;
		else if (keyword == "fit-content") kind = css_intrinsic_size::fit_content;
		else if (keyword == "stretch") kind = css_intrinsic_size::stretch;
		else if (value.type == CV_FUNCTION) {
			const auto function = lowcase(value.name);
			if (function == "fit-content") kind = css_intrinsic_size::fit_content_limit;
			else if (function == "stretch") kind = css_intrinsic_size::stretch_limit;
			else return false;
			if (value.value.size() != 1 || !width.from_token(value.value.front(), f_length_percentage | f_positive | f_math))
				return false;
		}
		if (kind == css_intrinsic_size::none) return false;
		width.set_intrinsic_size(kind);
		return true;
	}
}

void css_value_parser::parse_box(string_id name, const css_token_vector& value, bool important)
{
	css_length len[4];
	static const auto inset = _id("inset");
	if (name == inset)
	{
		if (int n = parse_1234_lengths(value, len, f_length_percentage | f_math, "auto"))
			add_four_properties(_top_, len, n, important);
		return;
	}
	const css_token val = value.size() == 1 ? value.front() : css_token();
	if (is_one_of(name, _width_, _min_width_, _max_width_)) {
		css_length width;
		if (parse_intrinsic_width(val, width)) {
			add_parsed_property(name, property_value(width, important));
			return;
		}
	}
	switch (name)
	{
	case _aspect_ratio_:
		if (const auto ratio = parse_aspect_ratio(value)) add_parsed_property(name, property_value(*ratio, important));
		return;

	case _z_index_:
		return add_length_property(name, val, "auto", f_integer, important);

	// <length-percentage [0,∞]>  https://developer.mozilla.org/en-US/docs/Web/CSS/padding-left
	case _padding_left_:
	case _padding_right_:
	case _padding_top_:
	case _padding_bottom_:
		return add_length_property(name, val, "", f_length_percentage|f_positive|f_math, important);

	// auto | <length-percentage>  https://developer.mozilla.org/en-US/docs/Web/CSS/left
	case _left_:
	case _right_:
	case _top_:
	case _bottom_:
	case _margin_left_:
	case _margin_right_:
	case _margin_top_:
	case _margin_bottom_:
		return add_length_property(name, val, "auto", f_length_percentage|f_math, important);

	// auto | <length-percentage [0,∞]>
	case _width_:
	case _height_:
	case _min_width_:
	case _min_height_:
		return add_length_property(name, val, "auto", f_length_percentage|f_positive|f_math, important);

	// none | <length-percentage [0,∞]>
	case _max_width_:
	case _max_height_:
		return add_length_property(name, val, "none", f_length_percentage|f_positive|f_math, important);

	//  normal | <number [0,∞]> | <length-percentage [0,∞]>   https://developer.mozilla.org/en-US/docs/Web/CSS/line-height#formal_syntax
	case _line_height_:
		return add_length_property(name, val, "normal", f_number|f_length_percentage|f_positive|f_math, important);

	// font-size = <absolute-size> | <relative-size> | <length-percentage [0,∞]>   https://developer.mozilla.org/en-US/docs/Web/CSS/font-size#formal_syntax
	case _font_size_:
		return add_length_property(name, val, font_size_strings, f_length_percentage|f_positive|f_math, important);

	case _margin_:
		if (int n = parse_1234_lengths(value, len, f_length_percentage | f_math, "auto"))
			add_four_properties(_margin_top_, len, n, important);
		break;

	case _padding_:
		if (int n = parse_1234_lengths(value, len, f_length_percentage | f_positive | f_math))
			add_four_properties(_padding_top_, len, n, important);
		break;
	default:
		break;
	}
}

void css_value_parser::add_length_property(string_id name, css_token val, string keywords, int options, bool important)
{
	css_length length;
	if (length.from_token(val, options, keywords))
		add_parsed_property(name, property_value(length, important));
}

bool parse_length(const css_token& tok, css_length& length, int options, string keywords)
{
	return length.from_token(tok, options, keywords);
}

// parses 1 or 2 lengths, but always returns 2 lengths
bool parse_two_lengths(const css_token_vector& tokens, css_length len[2], int options)
{
	auto n = tokens.size();
	if (n != 1 && n != 2) return false;

	css_length a, b;
	if (!a.from_token(tokens[0], options)) return false;
	if (n == 1) b = a;

	if (n == 2 && !b.from_token(tokens[1], options)) return false;

	len[0] = a;
	len[1] = b;
	return true;
}

int parse_1234_lengths(const css_token_vector& tokens, css_length len[4], int options, string keywords)
{
	return parse_1234_values(tokens, len, parse_length, options, keywords);
}

} // namespace litehtml
