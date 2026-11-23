#pragma once

#include <godot_cpp/classes/font_file.hpp>

namespace dockit {
	godot::Ref<godot::FontFile> load_system_font_face(const godot::String& p_name);				// 按字体名称加载系统字体文件
}
