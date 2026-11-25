#pragma once

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

class PDFTexture : public godot::ImageTexture {
	GDCLASS(PDFTexture, godot::ImageTexture)

public:
	static constexpr int32_t DEFAULT_DPI = 144;
	static constexpr int32_t DEFAULT_RENDER_FLAGS = 1 << 0;

	int32_t get_page() const;																	// 返回纹理对应的 PDF 页码，从 1 开始
	int32_t get_dpi() const;																	// 返回生成纹理时使用的 DPI
	int32_t get_page_count() const;																// 返回源 PDF 的总页数
	godot::Vector2 get_page_size_pt() const;													// 返回源页面的 PDF 点尺寸
	int32_t get_page_rotation() const;															// 返回源页面自身的旋转角度
	godot::String get_page_label() const;														// 返回源 PDF 为该页面指定的标签
	bool get_has_transparency() const;															// 检查源页面是否包含透明内容
	godot::Dictionary get_page_boxes() const;													// 返回源页面的各类页面框
	int32_t get_rotation() const;																// 返回导入时额外应用的旋转角度
	godot::Color get_background_color() const;													// 返回烘焙页面时使用的背景色
	int32_t get_render_flags() const;															// 返回烘焙页面时使用的 PDFReader 渲染标志
	void set_page(int32_t p_page);																// 记录导入页码，从 1 开始
	void set_dpi(int32_t p_dpi);																// 记录导入时使用的 DPI
	void set_page_count(int32_t p_page_count);													// 记录源 PDF 的总页数
	void set_page_size_pt(const godot::Vector2& p_page_size_pt);								// 记录源页面的 PDF 点尺寸
	void set_page_rotation(int32_t p_page_rotation);											// 记录源页面自身的旋转角度
	void set_page_label(const godot::String& p_page_label);										// 记录源 PDF 为该页面指定的标签
	void set_has_transparency(bool p_has_transparency);											// 记录源页面是否包含透明内容
	void set_page_boxes(const godot::Dictionary& p_page_boxes);									// 记录源页面的页面框
	void set_rotation(int32_t p_rotation);														// 记录导入时额外应用的旋转角度
	void set_background_color(const godot::Color& p_background_color);							// 记录烘焙页面时使用的背景色
	void set_render_flags(int32_t p_render_flags);												// 记录烘焙页面时使用的 PDFReader 渲染标志

protected:
	static void _bind_methods();

private:
	int32_t page = 1;
	int32_t dpi = DEFAULT_DPI;
	int32_t page_count = 0;
	godot::Vector2 page_size_pt;
	int32_t page_rotation = 0;
	godot::String page_label;
	bool has_transparency = false;
	godot::Dictionary page_boxes;
	int32_t rotation = 0;
	godot::Color background_color = godot::Color(1, 1, 1, 1);
	int32_t render_flags = DEFAULT_RENDER_FLAGS;
};
