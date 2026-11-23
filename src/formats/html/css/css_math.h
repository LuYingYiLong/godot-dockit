#pragma once

#include "css_length.h"

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace litehtml {
	struct css_math_node;
	using css_math_node_ptr = std::shared_ptr<const css_math_node>;

	struct css_math_expression {
		css_math_node_ptr root;
		bool has_percentage;
		bool non_negative;
		bool is_number;

		double evaluate(double p_basis) const;													// 根据给定基准计算数学表达式结果
		string to_string() const;																// 将数学表达式序列化为文本
	};

	// 解析和计算 CSS 数学表达式
	std::shared_ptr<const css_math_expression> parse_css_math(
		const css_token& p_token,
		int p_options
	);																							// 解析 CSS 数学表达式
	enum class css_math_scalar {
		number, percentage,
		number_or_percentage,
		angle,
		number_or_angle
	};

	using css_math_input_names = std::array<std::string_view, 4>;
	using css_math_input_values = std::array<double, 4>;
	css_math_node_ptr parse_css_scalar_math(
		const css_token& p_token,
		css_math_scalar p_type,
		const css_math_input_names* p_names = nullptr
	);																							// 解析 CSS 标量数学表达式
	double evaluate_css_scalar_math(
		const css_math_node& p_node,
		css_math_scalar p_type,
		double p_percentage_basis,
		const css_math_input_values* p_inputs = nullptr
	);																							// 计算 CSS 标量数学表达式的数值
	std::optional<double> evaluate_css_scalar_math(
		const css_token& p_token,
		css_math_scalar p_type,
		double p_percentage_basis = 1.0
	);																							// 评估一个上下文无关的计算，角度以弧度返回
	std::optional<double> parse_css_angle(
		const css_token& p_token,
		bool p_allow_unitless_zero = false,
		bool p_normalize = true
	);																							//解析一个弧度角，可选择将其归一化为带符号的半圈
	inline constexpr double CSS_DPI_PER_DPPX = 96.0;
	std::optional<double> parse_css_resolution(const css_token& p_token);						// 解析以标准 dppx 单位表示的非负分辨率
	std::shared_ptr<const css_math_expression> compute_css_math(
		const css_math_expression& p_value,
		const std::function<pixel_t(const css_length&)>& p_resolve_length
	);																							// 计算 CSS 数学表达式的结果
	css_length interpolate_css_length(
		const css_length& from,
		const css_length& to,
		double progress,
		bool non_negative = false
	);																							// 在两个 CSS 长度值之间执行插值
}
