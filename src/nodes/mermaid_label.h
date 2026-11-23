#pragma once

#include "formats/mermaid/mermaid_document.h"
#include "formats/mermaid/mermaid_render.h"

#include <godot_cpp/classes/control.hpp>

class MermaidLabel : public godot::Control {
	GDCLASS(MermaidLabel, godot::Control)

public:
	enum ThemeMode {
		THEME_AUTO,
		THEME_LIGHT,
		THEME_DARK,
	};

	MermaidLabel();

	void set_text(const godot::String& p_text);													// 设置渲染文本
	godot::String get_text() const;																// 返回渲染文本
	void set_document(const godot::Ref<MermaidDocument>& p_document);							// 设置文档资源
	godot::Ref<MermaidDocument> get_document() const;											// 返回文档资源
	void set_theme_mode(ThemeMode p_mode);														// 设置主题模式
	ThemeMode get_theme_mode() const;															// 返回当前 Mermaid 图表的主题模式
	godot::Ref<godot::Texture2D> get_texture() const;											// 返回当前渲染结果的纹理
	godot::String get_render_error() const;														// 返回最近一次渲染错误
	godot::Vector2 _get_minimum_size() const override;											// 返回控件所需的最小尺寸
	godot::PackedStringArray _get_configuration_warnings() const override;						// 返回节点配置警告

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	godot::String text;
	godot::Ref<MermaidDocument> document;
	ThemeMode theme_mode = THEME_AUTO;
	MermaidRenderCache render_cache;
	godot::Ref<godot::Texture2D> texture;
	godot::String rendered_source;
	godot::String render_error;

	bool uses_light_theme() const;
	void refresh_render();
};

VARIANT_ENUM_CAST(MermaidLabel::ThemeMode);
