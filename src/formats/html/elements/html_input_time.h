#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <optional>

namespace dockit::input_time {
	enum class Kind {
		none,
		date,
		month,
		week,
		time,
		local
	};

	constexpr double day = 86400000.0;

	std::optional<double> parse(Kind p_kind, const godot::String& p_value);						// 解析 HTML 日期或时间控件的字符串值
	godot::String format(Kind p_kind, double p_number);											// 将数值格式化为 HTML 日期或时间控件的字符串值
	godot::Dictionary parts(Kind p_kind, double p_number);										// 拆分日期或时间数值的各个组成部分
	std::optional<double> from_parts(Kind p_kind, const godot::Dictionary& p_parts);			// 根据日期或时间的各个组成部分构造数值
	double scale(Kind p_kind);																	// 获取该日期或时间类型的数值换算比例
	double step(Kind p_kind);																	// 获取该日期或时间类型的默认步长
	double base(Kind p_kind);																	// 获取该日期或时间类型的数值基准
}
