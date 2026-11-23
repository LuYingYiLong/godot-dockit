#pragma once

#include "css_length.h"

#include <array>
#include <memory>
#include <optional>

namespace litehtml {
	class document;
	struct css_transform_blend;

	struct css_transform_function {
		enum class kind {
			translate,
			rotate,
			scale,
			skew,
			matrix,
			matrix3d,
			perspective,
			blend
		};

		kind type;
		css_length x;
		css_length y;
		css_length z;
		double first = 0;
		double second = 0;
		double third = 1;
		std::array<double, 3> axis{ 0, 0, 1 };
		std::array<double, 16> matrix{};
		std::shared_ptr<const css_transform_blend> blend;
	};

	using css_transform_list = std::vector<css_transform_function>;
	using css_individual_transform = std::optional<css_transform_function>;

	// 计算出的端点会保留百分比，直到布局提供参考框
	struct css_transform_blend {
		css_transform_list from;
		css_transform_list to;
		double progress;
		bool allow_discrete;
	};

	enum class css_transform_box { content, border, fill, stroke, view };

	struct css_individual_transforms {
		css_individual_transform translate;
		css_individual_transform rotate;
		css_individual_transform scale;

		bool enabled() const { return translate || rotate || scale; }							// 检查是否设置了任一独立变换属性
	};

	struct css_transform_origin {
		css_length x{ 50, css_units_percentage };
		css_length y{ 50, css_units_percentage };
		css_length z;
	};

	// 解析 CSS 变换
	bool parse_css_transform(const css_token_vector& p_tokens, css_transform_list& r_value);	// 解析CSS变换
	bool parse_css_transform_origin(
		const css_token_vector& p_tokens,
		css_transform_origin& r_value
	);																							// 解析CSS变换原点
	bool parse_css_individual_transform(
		css_transform_function::kind p_kind,
		const css_token_vector& p_tokens,
		css_individual_transform& r_value
	);																							// 解析 CSS 独立变换属性
}
