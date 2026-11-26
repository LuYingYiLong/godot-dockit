#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace dockit {

class StyleBoxCSSHelper : public godot::RefCounted {
	GDCLASS(StyleBoxCSSHelper, godot::RefCounted)

protected:
	static void _bind_methods();

public:
	static void clear_cache(); // 在扩展卸载前释放转换缓存
	static godot::Dictionary stylebox_to_css(const godot::Ref<godot::StyleBox>& p_stylebox); // 将样式框转换为 CSS 声明、纹理资源映射和诊断
	static godot::Dictionary css_to_stylebox(const godot::String& p_css,
		const godot::Dictionary& p_context = {}, const godot::StringName& p_target_type = "auto"); // 在独立上下文中计算 CSS 并转换为样式框
};

}
