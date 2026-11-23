#pragma once

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

#include "formats/latex/latex_document.h"
#include "formats/latex/tex_render.h"

namespace godot {
	class Node2D;
	class SubViewport;
}

class LatexLabel3D : public godot::MeshInstance3D {
	GDCLASS(LatexLabel3D, godot::MeshInstance3D)

public:
	enum DrawFlags {
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX,
	};

	enum AlphaCutMode {
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS,
		ALPHA_CUT_HASH,
	};

	LatexLabel3D();

	void set_text(const godot::String& p_text);													// 设置渲染文本
	godot::String get_text() const;																// 返回渲染文本
	void set_document(const godot::Ref<LatexDocument>& p_document);								// 设置文档资源
	godot::Ref<LatexDocument> get_document() const;												// 返回文档资源
	void set_display_mode(bool p_enabled);														// 设置独立公式显示模式
	bool is_display_mode() const;																// 返回是否启用独立公式显示模式
	void set_font_size(int p_size);																// 设置字体大小
	int get_font_size() const;																	// 返回字体大小
	void set_modulate(const godot::Color& p_color);												// 设置调制颜色
	godot::Color get_modulate() const;															// 返回调制颜色
	void set_horizontal_alignment(godot::HorizontalAlignment p_alignment);						// 设置水平对齐方式
	godot::HorizontalAlignment get_horizontal_alignment() const;								// 返回水平对齐方式
	void set_vertical_alignment(godot::VerticalAlignment p_alignment);							// 设置垂直对齐方式
	godot::VerticalAlignment get_vertical_alignment() const;									// 返回垂直对齐方式
	void set_pixel_size(float p_size);															// 设置像素尺寸
	float get_pixel_size() const;																// 返回像素尺寸
	void set_offset(const godot::Vector2& p_offset);											// 设置位置偏移
	godot::Vector2 get_offset() const;															// 返回位置偏移

	void set_billboard_mode(godot::BaseMaterial3D::BillboardMode p_mode);						// 设置公告板模式
	godot::BaseMaterial3D::BillboardMode get_billboard_mode() const;							// 返回公告板模式
	void set_draw_flag(DrawFlags p_flag, bool p_enabled);										// 设置绘制标志状态
	bool get_draw_flag(DrawFlags p_flag) const;													// 返回绘制标志状态
	void set_alpha_cut_mode(AlphaCutMode p_mode);												// 设置透明度裁剪模式
	AlphaCutMode get_alpha_cut_mode() const;													// 返回透明度裁剪模式
	void set_alpha_scissor_threshold(float p_threshold);										// 设置透明度裁剪阈值
	float get_alpha_scissor_threshold() const;													// 返回透明度裁剪阈值
	void set_alpha_hash_scale(float p_scale);													// 设置透明度哈希缩放
	float get_alpha_hash_scale() const;															// 返回透明度哈希缩放
	void set_render_scale(float p_scale);														// 设置渲染缩放比例
	float get_render_scale() const;																// 返回渲染缩放比例
	void set_texture_filter(godot::BaseMaterial3D::TextureFilter p_filter);						// 设置纹理过滤方式
	godot::BaseMaterial3D::TextureFilter get_texture_filter() const;							// 返回纹理过滤方式
	void set_render_priority(int p_priority);													// 设置渲染优先级
	int get_render_priority() const;															// 返回渲染优先级

	godot::String get_render_error() const;														// 返回最近一次渲染错误
	godot::PackedStringArray _get_configuration_warnings() const override;						// 返回节点配置警告

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void _validate_property(godot::PropertyInfo& p_property) const;

private:
	godot::String text;
	godot::Ref<LatexDocument> document;
	bool display_mode = true;
	int font_size = 32;
	godot::HorizontalAlignment horizontal_alignment = godot::HORIZONTAL_ALIGNMENT_CENTER;
	godot::VerticalAlignment vertical_alignment = godot::VERTICAL_ALIGNMENT_CENTER;
	float pixel_size = 0.005f;
	float render_scale = 1.0f;
	godot::Vector2 offset;

	godot::Ref<TeXRender> render;
	godot::Ref<godot::QuadMesh> quad;
	godot::Ref<godot::StandardMaterial3D> material;
	godot::SubViewport* viewport = nullptr;
	godot::Node2D* canvas = nullptr;
	bool rebuild_queued = false;

	void queue_rebuild();
	void rebuild();
	void ensure_viewport();
	void update_texture();
	void update_quad();
	void draw_formula();
};

VARIANT_ENUM_CAST(LatexLabel3D::DrawFlags);
VARIANT_ENUM_CAST(LatexLabel3D::AlphaCutMode);
