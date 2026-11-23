#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label_settings.hpp>
#include <godot_cpp/classes/style_box.hpp>

#include "formats/latex/latex_document.h"
#include "formats/latex/tex_render.h"

class LatexLabel : public godot::Control {
	GDCLASS(LatexLabel, godot::Control)

public:
	LatexLabel();

	void set_text(const godot::String& p_text);													// 设置渲染文本
	godot::String get_text() const;																// 返回渲染文本
	void set_document(const godot::Ref<LatexDocument>& p_document);								// 设置文档资源
	godot::Ref<LatexDocument> get_document() const;												// 返回文档资源
	void set_label_settings(const godot::Ref<godot::LabelSettings>& p_settings);				// 设置标签设置资源
	godot::Ref<godot::LabelSettings> get_label_settings() const;								// 返回当前使用的标签设置资源
	void set_horizontal_alignment(godot::HorizontalAlignment p_alignment);						// 设置水平对齐方式
	godot::HorizontalAlignment get_horizontal_alignment() const;								// 返回水平对齐方式
	void set_vertical_alignment(godot::VerticalAlignment p_alignment);							// 设置垂直对齐方式
	godot::VerticalAlignment get_vertical_alignment() const;									// 返回垂直对齐方式
	void set_clip_text(bool p_clip);															// 设置文本裁剪功能
	bool is_clipping_text() const;																// 返回是否启用文本裁剪
	void set_display_mode(bool p_enabled);														// 设置独立公式显示模式
	bool is_display_mode() const;																// 返回是否启用独立公式显示模式
	godot::String get_render_error() const;														// 返回最近一次渲染错误
	godot::Vector2 _get_minimum_size() const override;											// 返回控件所需的最小尺寸
	godot::PackedStringArray _get_configuration_warnings() const override;						// 返回节点配置警告

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	godot::String text;
	godot::Ref<LatexDocument> document;
	godot::Ref<godot::LabelSettings> label_settings;
	godot::Ref<godot::StyleBox> normal_style;
	godot::Ref<godot::StyleBox> focus_style;
	godot::HorizontalAlignment horizontal_alignment = godot::HORIZONTAL_ALIGNMENT_LEFT;
	godot::VerticalAlignment vertical_alignment = godot::VERTICAL_ALIGNMENT_TOP;
	bool clip_text = false;
	bool display_mode = true;
	godot::Ref<TeXRender> render;

	void rebuild();
	void update_styles();
	void draw_formula();
};
