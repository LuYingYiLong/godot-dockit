#pragma once

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace dockit {
	void initialize_html_ui_translations();														// 初始化 HTML 原生控件的本地化词条
	void shutdown_html_ui_translations();														// 释放 HTML 原生控件的本地化数据
	godot::String html_ui_translate(
		const godot::String& p_message,
		const godot::StringName& p_context
	);																							// 按上下文翻译 HTML 原生控件文案
}
