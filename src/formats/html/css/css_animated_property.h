#pragma once

#include "css_property_value.h"

#include <variant>
#include <vector>

namespace dockit::css {
	class ComputedStyle;
	using AnimatedValue = std::variant<
		float,
		litehtml::web_color,
		litehtml::css_length,
		litehtml::css_transform_list,
		litehtml::css_text_indent,
		litehtml::css_font_size_adjust
	>;
	using AnimatedValues = std::vector<AnimatedValue>;

	enum class AnimatedRange {
		unrestricted,
		non_negative,
		unit_interval,
		font_weight
	};
	struct AnimatedProperty {
		litehtml::string_id name;
		AnimatedValue(*read)(const ComputedStyle&);
		AnimatedRange range = AnimatedRange::non_negative;
		bool distinguish_number = false;
	};

	// 读取和计算 CSS 动画属性
	const std::vector<AnimatedProperty>& animated_properties();									// 获取支持动画的 CSS 属性列表
	AnimatedValues read_animated_values(const ComputedStyle& style);							// 读取动画值
	bool equal_animated_value(
		const AnimatedProperty& property,
		const AnimatedValue& a,
		const AnimatedValue& b
	);																							// 比较两个动画属性值是否相等
	bool can_interpolate_animated_value(
		const AnimatedProperty& property,
		const AnimatedValue& a,
		const AnimatedValue& b
	);																							// 检查动画属性值是否支持插值
	AnimatedValue interpolate_animated_value(
		const AnimatedProperty& property,
		const AnimatedValue& a,
		const AnimatedValue& b,
		double progress,
		bool allow_discrete
	);																							// 插值动画值
}
