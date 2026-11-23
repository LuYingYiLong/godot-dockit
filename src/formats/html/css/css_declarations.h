#pragma once

#include "css_property_value.h"

#include <array>

namespace litehtml {
	enum class css_edge_group {
		margin,
		padding,
		inset,
		scroll_padding,
		scroll_margin,
		border_width,
		border_style,
		border_color,
		radius_x,
		radius_y,
		count
	};

	struct css_flow_edges {
		std::array<string_id, 4> physical;
		std::array<string_id, 4> logical;
		bool corners = false;

		size_t map_direction(size_t side, bool rtl) const {
			if (!rtl) return side;
			if (corners) return side ^ 1;
			return side % 2 == 1 ? 4 - side : side;
		}																						// 将逻辑方向映射为物理方向
	};

	const css_flow_edges& css_edge_properties(css_edge_group group);							// 获取指定 CSS 盒模型边缘组对应的属性
	string_id css_map_logical_property(string_id name, bool rtl);								// 将 CSS 逻辑属性映射为对应的物理属性
	string_id css_canonical_property(string_id name);											// 获取 CSS 属性对应的规范化属性标识
	const vector<string_id>& css_longhands(string_id name);										// 获取简写 CSS 属性展开后的长属性列表

	enum class css_declaration_status {
		unknown,
		invalid,
		deferred,
		parsed
	};

	struct css_parsed_declaration {
		css_declaration_status status;
		props_map properties;
	};

	css_parsed_declaration parse_css_declaration(
		string_id name,
		const css_token_vector& tokens,
		const string& baseurl,
		bool important,
		document_container* container,
		const css_source& source
	);																							// 解析 CSS 属性声明及其来源信息
	css_parsed_declaration reparse_css_declaration(
		const property_value::deferred_declaration& declaration,
		const css_token_vector& tokens,
		bool important,
		document_container* container
	);																							// 根据解析后的令牌重新构建 CSS 属性声明
	bool supports_css_declaration(const css_token_vector& tokens);								// 检查 CSS 声明值是否受支持
	std::optional<css_diagnostic> check_css_declaration(css_declaration_status status, string_id name,
		const css_token_vector& tokens, const css_source& source);								// 检查 CSS 声明并生成诊断结果
}
