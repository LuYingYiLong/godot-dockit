#pragma once

#include "css_parser.h"
#include "css_property_value.h"

#include <godot_cpp/variant/string.hpp>

namespace dockit::css {
	class ComputedStyle;

	// 读取和序列化 CSSOM 规则
	std::string property_name(const godot::String& name);										// 获取 CSS 属性标识对应的名称
	raw_declaration::vector inline_declarations(const godot::String& text);						// 解析 style 属性中的 CSS 声明
	std::string serialize_declarations(const raw_declaration::vector& declarations);			// 将 CSS 声明序列化为文本
	std::string serialize_rule(const raw_rule& rule);											// 将 CSS 规则序列化为文本
	raw_rule::ptr parse_cssom_rule(const godot::String& text);									// 解析 CSSOM 规则文本
	std::string serialize_value(
		litehtml::string_id name,
		const litehtml::property_value& value
	);																							// 将 CSS 值序列化为文本
	std::string serialize_computed(litehtml::string_id name, const ComputedStyle& style);		// 将计算样式序列化为 CSS 值
	bool is_box_shorthand(litehtml::string_id name);											// 检查属性是否属于盒模型简写属性
	std::string serialize_box_shorthand(std::vector<std::string> values);						// 将盒模型属性序列化为简写形式
}
