#pragma once

#include <litehtml/css_length.h>

#include <memory>
#include <optional>

namespace litehtml {
	class document;
	struct css_math_node;
	using css_math_node_ptr = std::shared_ptr<const css_math_node>;

	struct css_math_expression {
		css_math_node_ptr root;
		bool has_percentage;
		bool non_negative;
		bool is_number;

		double evaluate(double p_basis) const;
		string to_string() const;
	};

	std::shared_ptr<const css_math_expression> parse_css_math(const css_token& p_token, int p_options);
	enum class css_math_scalar { number, percentage, number_or_percentage, angle, number_or_angle };
	// Evaluate a context-free calculation. Angles are returned in radians.
	std::optional<double> evaluate_css_scalar_math(const css_token& p_token, css_math_scalar p_type,
		double p_percentage_basis = 1.0);
	// Parse an angle in radians and normalize it to a signed half turn.
	std::optional<double> parse_css_angle(const css_token& p_token, bool p_allow_unitless_zero = false);
	inline constexpr double CSS_DPI_PER_DPPX = 96.0;
	// Parse a nonnegative resolution in canonical dppx units.
	std::optional<double> parse_css_resolution(const css_token& p_token);
	std::shared_ptr<const css_math_expression> compute_css_math(const css_math_expression& p_value,
		const document& p_document, const font_metrics& p_metrics, css_length_context p_context = {});
}
