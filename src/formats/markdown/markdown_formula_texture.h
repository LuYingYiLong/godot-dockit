#pragma once

#include "formats/latex/tex_render.h"
#include <godot_cpp/classes/texture2d.hpp>

namespace dockit {
class MarkdownFormulaTexture : public godot::Texture2D {
	GDCLASS(MarkdownFormulaTexture, godot::Texture2D)
	godot::Ref<TeXRender> formula;
protected:
	static void _bind_methods() {}
public:
	godot::Error configure(const godot::String& p_source, int32_t p_size, const godot::Color& p_color, bool p_display); // 创建保留矢量绘制的公式对象
	int32_t _get_width() const override;
	int32_t _get_height() const override;
	bool _has_alpha() const override { return true; }
	void _draw_rect_region(const godot::RID& p_canvas, const godot::Rect2& p_rect, const godot::Rect2& p_source,
		const godot::Color& p_modulate, bool p_transpose, bool p_clip_uv) const override;
};
}
