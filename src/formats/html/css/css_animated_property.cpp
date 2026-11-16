#include "css_animated_property.h"

#include "css_computed_style.h"
#include "css_math.h"

#include <algorithm>
#include <cmath>

namespace docrender::css {
	using namespace litehtml;

	namespace {
		using Kind = css_transform_function::kind;

		bool equal_length(const css_length& a, const css_length& b) {
			if (a.is_predefined() != b.is_predefined() || a.intrinsic_size() != b.intrinsic_size()) return false;
			if (a.is_predefined()) return a.predef() == b.predef();
			if (a.expression() || b.expression()) return a.expression() == b.expression() || a.to_string() == b.to_string();
			return (a.val() == 0 && b.val() == 0) || (a.units() == b.units() && a.val() == b.val());
		}

		bool equal_function(const css_transform_function& a, const css_transform_function& b) {
			return a.type == b.type && equal_length(a.x, b.x) && equal_length(a.y, b.y) && equal_length(a.z, b.z) &&
				a.first == b.first && a.second == b.second && a.third == b.third && a.axis == b.axis &&
				a.matrix == b.matrix && a.blend == b.blend;
		}

		css_transform_function identity(const css_transform_function& model) {
			css_transform_function result{model.type};
			result.axis = model.axis;
			if (model.type == Kind::scale) result.first = result.second = 1;
			if (model.type == Kind::perspective) result.z = css_length::predef_value();
			if (model.type == Kind::matrix) result.matrix[0] = result.matrix[3] = 1;
			if (model.type == Kind::matrix3d) result.matrix[0] = result.matrix[5] = result.matrix[10] = result.matrix[15] = 1;
			return result;
		}

		css_transform_function blend_matrices(css_transform_list from, css_transform_list to, double progress,
				bool allow_discrete) {
			css_transform_function result{Kind::blend};
			result.blend = std::make_shared<css_transform_blend>(
				css_transform_blend{std::move(from), std::move(to), progress, allow_discrete});
			return result;
		}

		css_transform_list interpolate_transforms(const css_transform_list& a, const css_transform_list& b,
				double progress, bool allow_discrete) {
			css_transform_list result;
			result.reserve(std::max(a.size(), b.size()));
			const auto lerp = [progress](double from, double to) { return from + (to - from) * progress; };
			for (std::size_t index = 0; index < std::max(a.size(), b.size()); ++index) {
				if ((index < a.size() && a[index].type == Kind::blend) ||
						(index < b.size() && b[index].type == Kind::blend) ||
						(index < a.size() && index < b.size() && a[index].type != b[index].type)) {
					result.push_back(blend_matrices(
						{a.begin() + std::min(index, a.size()), a.end()},
						{b.begin() + std::min(index, b.size()), b.end()}, progress, allow_discrete));
					break;
				}
				auto first = index < a.size() ? a[index] : identity(b[index]);
				const auto second = index < b.size() ? b[index] : identity(a[index]);
				if (equal_function(first, second)) {
					result.push_back(std::move(first));
					continue;
				}
				if (first.type == Kind::matrix || first.type == Kind::matrix3d || first.type == Kind::perspective ||
						(first.type == Kind::rotate && first.axis != second.axis && first.first != 0 && second.first != 0)) {
					result.push_back(blend_matrices({std::move(first)}, {second}, progress, allow_discrete));
					continue;
				}
				if (first.type == Kind::translate) {
					first.x = interpolate_css_length(first.x, second.x, progress);
					first.y = interpolate_css_length(first.y, second.y, progress);
					first.z = interpolate_css_length(first.z, second.z, progress);
				} else {
					if (first.type == Kind::rotate && first.first == 0) first.axis = second.axis;
					first.first = lerp(first.first, second.first);
					first.second = lerp(first.second, second.second);
					first.third = lerp(first.third, second.third);
				}
				result.push_back(std::move(first));
			}
			return result;
		}

	}

	AnimatedValues read_animated_values(const ComputedStyle& css) {
		const auto color = css.get_color();
		const auto& borders = css.get_borders();
		return {static_cast<float>(css.get_opacity()), color, css.get_bg().m_color.resolve(color),
			borders.top.color.resolve(color), borders.right.color.resolve(color),
			borders.bottom.color.resolve(color), borders.left.color.resolve(color),
			css.get_width(), css.get_height(), css.get_transforms()};
	}

	bool equal_animated_value(const AnimatedValue& a, const AnimatedValue& b) {
		if (a.index() != b.index()) return false;
		if (const auto* value = std::get_if<float>(&a)) return *value == std::get<float>(b);
		if (const auto* value = std::get_if<web_color>(&a)) return equal_css_colors(*value, std::get<web_color>(b));
		if (const auto* value = std::get_if<css_length>(&a)) return equal_length(*value, std::get<css_length>(b));
		const auto& first = std::get<css_transform_list>(a);
		const auto& second = std::get<css_transform_list>(b);
		return first.size() == second.size() && std::equal(first.begin(), first.end(), second.begin(), equal_function);
	}

	bool can_interpolate_animated_value(const AnimatedValue& a, const AnimatedValue& b) {
		if (a.index() != b.index()) return false;
		if (const auto* from = std::get_if<css_length>(&a)) {
			const auto& to = std::get<css_length>(b);
			return !from->is_predefined() && !to.is_predefined() &&
				from->intrinsic_size() == css_intrinsic_size::none && to.intrinsic_size() == css_intrinsic_size::none;
		}
		return true;
	}

	AnimatedValue interpolate_animated_value(const AnimatedValue& a, const AnimatedValue& b, double progress,
			bool allow_discrete) {
		if (const auto* value = std::get_if<float>(&a)) return static_cast<float>(
			std::clamp(*value + (std::get<float>(b) - *value) * progress, 0.0, 1.0));
		if (const auto* value = std::get_if<web_color>(&a)) return interpolate_css_color(*value, std::get<web_color>(b), progress);
		if (const auto* value = std::get_if<css_length>(&a)) return interpolate_css_length(*value, std::get<css_length>(b), progress, true);
		return interpolate_transforms(std::get<css_transform_list>(a), std::get<css_transform_list>(b), progress, allow_discrete);
	}

}
