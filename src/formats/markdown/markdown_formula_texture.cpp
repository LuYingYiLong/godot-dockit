#include "markdown_formula_texture.h"

using namespace godot;
namespace dockit {
Error MarkdownFormulaTexture::configure(const String& p_source, int32_t p_size, const Color& p_color, bool p_display) {
	formula.instantiate();
	set_meta("_dockit_copy_text", p_display ? String("$$") + p_source + "$$" : String("$") + p_source + "$");
	return formula->parse(p_source, p_size, p_display ? TeXRender::STYLE_DISPLAY : TeXRender::STYLE_TEXT, p_color);
}
int32_t MarkdownFormulaTexture::_get_width() const { return formula.is_valid() ? formula->get_width() : 0; }
int32_t MarkdownFormulaTexture::_get_height() const { return formula.is_valid() ? formula->get_height() : 0; }
void MarkdownFormulaTexture::_draw_rect_region(const RID& p_canvas, const Rect2& p_rect, const Rect2& p_source,
	const Color& p_modulate, bool p_transpose, bool p_clip_uv) const {
	if (formula.is_null() || !p_source.has_area()) return;
	const Vector2 scale = p_rect.size / p_source.size;
	formula->draw_to_canvas(p_canvas, Transform2D(scale.x, 0, 0, scale.y,
		p_rect.position.x - p_source.position.x * scale.x, p_rect.position.y - p_source.position.y * scale.y));
}
}
