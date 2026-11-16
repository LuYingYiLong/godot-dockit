#pragma once

#include "css_property_value.h"

#include <array>
#include <variant>

namespace docrender::css {
	class ComputedStyle;
	using AnimatedValue = std::variant<float, litehtml::web_color, litehtml::css_length, litehtml::css_transform_list>;
	inline constexpr std::array animated_property_names{"opacity", "color", "background-color",
		"border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
		"width", "height", "transform"};
	using AnimatedValues = std::array<AnimatedValue, animated_property_names.size()>;

	AnimatedValues read_animated_values(const ComputedStyle& style);
	bool equal_animated_value(const AnimatedValue& a, const AnimatedValue& b);
	bool can_interpolate_animated_value(const AnimatedValue& a, const AnimatedValue& b);
	AnimatedValue interpolate_animated_value(const AnimatedValue& a, const AnimatedValue& b, double progress,
		bool allow_discrete);
}
