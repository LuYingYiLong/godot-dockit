#pragma once

#include "css_font_face.h"

#include <array>

namespace litehtml {
	enum class css_alternate_type { stylistic, styleset, character_variant, swash, ornaments, annotation, count };
	constexpr size_t css_alternate_count = static_cast<size_t>(css_alternate_type::count);
	struct css_named_alternate {
		css_alternate_type type;
		string name;
	};

	struct css_font_alternates {
		bool historical_forms = false;
		std::vector<css_named_alternate> names;

		std::string cache_key() const;															// 构建字体特性规则的缓存键
	};

	using css_font_feature_names = std::array<std::map<string, std::vector<uint32_t>>, css_alternate_count>;

	struct css_font_feature_values_rule {
		string_vector families;
		css_font_feature_names names;

		bool operator==(const css_font_feature_values_rule& other) const {
			return families == other.families && names == other.names;
		}																						// 比较两个值是否相等
	};

	std::optional<css_font_alternates> parse_css_font_alternates(
		const css_token_vector& tokens
	);																							// 解析 CSS 字体替代字形设置
	std::optional<css_font_feature_values_rule> parse_font_feature_values_rule(
		const css_token_vector& prelude,
		const css_token_vector& contents
	);																							// 解析 CSS @font-feature-values 规则
	css_font_features resolve_font_feature_names(
		const css_font_alternates& alternates,
		const css_font_feature_names& definitions
	);																							// 解析字体特性名称对应的 OpenType 标记
}
