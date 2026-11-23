#pragma once

#include "html_element.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

class HTMLCanvasRenderingContext2D;

class HTMLCanvasGradient : public godot::RefCounted {
	GDCLASS(HTMLCanvasGradient, godot::RefCounted)
	friend class HTMLCanvasRenderingContext2D;
	struct Stop { double offset = 0.0; godot::Color color; };
	std::vector<Stop> stops;
	godot::Vector2 start;
	godot::Vector2 end;
protected:
	static void _bind_methods();
public:
	godot::Error add_color_stop(double p_offset, const godot::Variant& p_color);				// 为渐变添加颜色停止点
	void configure(const godot::Vector2& p_start, const godot::Vector2& p_end);					// 设置渐变的起点和终点
	godot::Color sample(const godot::Vector2& p_point) const;									// 获取指定坐标处的渐变颜色
};

class HTMLCanvasPattern : public godot::RefCounted {
	GDCLASS(HTMLCanvasPattern, godot::RefCounted)
	friend class HTMLCanvasRenderingContext2D;
	godot::Ref<godot::Image> image;
	bool repeat_x = true;
	bool repeat_y = true;
protected:
	static void _bind_methods();
public:
	void configure(const godot::Ref<godot::Image>& p_image, const godot::String& p_repeat);		// 设置图案的图像来源和重复方式
	godot::Color sample(const godot::Vector2& p_point) const;									// 获取指定坐标处的图案颜色
};

class HTMLCanvasElement : public HTMLElement {
	GDCLASS(HTMLCanvasElement, HTMLElement)
	friend class HTMLCanvasRenderingContext2D;

protected:
	static void _bind_methods();

public:
	void canvas_changed();																		// 标记画布内容已变化并更新图像
	int64_t get_width() const;																	// 获取宽度
	void set_width(int64_t p_width);															// 设置宽度
	int64_t get_height() const;																	// 获取高度
	void set_height(int64_t p_height);															// 设置高度
	godot::Ref<HTMLCanvasRenderingContext2D> get_context(const godot::String& p_context_id);	// 获取上下文
	godot::Ref<godot::Image> get_image() const;													// 获取图像
	godot::String to_data_url(
		const godot::String& p_type = "image/png",
		double p_quality = 0.92
	) const;																					// 将画布图像编码为 Data URL
};

class HTMLCanvasRenderingContext2D : public godot::RefCounted {
	GDCLASS(HTMLCanvasRenderingContext2D, godot::RefCounted)
	friend class HTMLCanvasElement;

	struct State {
		godot::Variant fill_style = godot::Color(0, 0, 0, 1);
		godot::Variant stroke_style = godot::Color(0, 0, 0, 1);
		float line_width = 1.0f;
		godot::Ref<godot::Font> font;
		int32_t font_size = 16;
		godot::Transform2D transform;
	};

	godot::Ref<godot::Image> image;
	godot::Ref<godot::ImageTexture> texture;
	godot::ObjectID owner_id;
	std::vector<State> states;
	State state;
	std::vector<godot::Vector2> path;
	bool path_closed = false;
	bool texture_dirty = true;

	void mark_dirty();
	void put_pixel(const godot::Vector2& p_point, const godot::Color& p_color);
	void draw_line_segment(const godot::Vector2& p_from, const godot::Vector2& p_to, const godot::Variant& p_style);
	void fill_polygon(const std::vector<godot::Vector2>& p_points, const godot::Variant& p_style);
	void draw_text(const godot::String& p_text, const godot::Vector2& p_position, const godot::Variant& p_style);
	godot::Color style_color(const godot::Variant& p_style, const godot::Vector2& p_position) const;
	void reset(int64_t p_width, int64_t p_height);
	void set_owner_id(godot::ObjectID p_owner) { owner_id = p_owner; }

protected:
	static void _bind_methods();

public:
	HTMLCanvasRenderingContext2D();
	void set_fill_style(const godot::Variant& p_style);											// 设置 Canvas 当前填充样式
	godot::Variant get_fill_style() const { return state.fill_style; }							// 获取填充样式
	void set_stroke_style(const godot::Variant& p_style);										// 设置 Canvas 当前描边样式
	godot::Variant get_stroke_style() const { return state.stroke_style; }						// 获取描边样式
	void set_line_width(double p_width);														// 设置行宽度
	double get_line_width() const { return state.line_width; }									// 获取行宽度
	void set_font(const godot::Ref<godot::Font>& p_font);										// 设置字体
	godot::Ref<godot::Font> get_font() const { return state.font; }								// 根据字体描述获取字体及度量信息
	void set_font_size(int32_t p_size);															// 设置字体尺寸
	int32_t get_font_size() const { return state.font_size; }									// 获取字体尺寸
	void save();																				// 保存当前绘图状态
	void restore();																				// 恢复最近保存的绘图状态
	void reset_transform();																		// 重置变换
	void translate(double p_x, double p_y);														// 平移当前变换矩阵
	void scale(double p_x, double p_y);															// 缩放当前变换矩阵
	void rotate(double p_angle);																// 旋转当前变换矩阵
	void set_transform(double p_a, double p_b, double p_c, double p_d, double p_e, double p_f);	// 设置变换
	void clear_rect(double p_x, double p_y, double p_width, double p_height);					// 清除矩形
	void fill_rect(double p_x, double p_y, double p_width, double p_height);					// 使用当前填充样式绘制矩形
	void stroke_rect(double p_x, double p_y, double p_width, double p_height);					// 使用当前描边样式绘制矩形
	void begin_path();																			// 开始路径
	void close_path();																			// 关闭路径
	void move_to(double p_x, double p_y);														// 将当前路径起点移动到指定坐标
	void line_to(double p_x, double p_y);														// 向当前路径添加一条直线
	void rect(double p_x, double p_y, double p_width, double p_height);							// 向当前路径添加矩形
	void arc(
		double p_x,
		double p_y,
		double p_radius,
		double p_start_angle,
		double p_end_angle,
		bool p_counterclockwise = false
	);																							// 向当前路径添加圆弧
	void fill();																				// 使用当前填充样式填充当前路径
	void stroke();																				// 使用当前描边样式描绘当前路径
	void fill_text(const godot::String& p_text, double p_x, double p_y);						// 使用当前填充样式绘制文本
	godot::Ref<HTMLCanvasGradient> create_linear_gradient(
		double p_x0,
		double p_y0,
		double p_x1,
		double p_y1
	);																							// 创建线性渐变
	godot::Ref<HTMLCanvasPattern> create_pattern(
		const godot::Variant& p_source,
		const godot::String& p_repeat = "repeat"
	);																							// 创建图案
	void draw_image(const godot::Variant& p_source, double p_dx, double p_dy);					// 绘制图像
	void draw_image_sized(
		const godot::Variant& p_source,
		double p_dx,
		double p_dy,
		double p_dw,
		double p_dh
	);																							// 按指定尺寸绘制图像
	void draw_image_region(const godot::Variant& p_source, double p_sx, double p_sy, double p_sw, double p_sh,
		double p_dx, double p_dy, double p_dw, double p_dh);									// 绘制图像区域
	void put_image_data(const godot::Ref<godot::Image>& p_data, int64_t p_dx, int64_t p_dy);	// 将像素数据写入 Canvas 图像区域
	godot::Ref<godot::Image> get_image_data(
		int64_t p_sx,
		int64_t p_sy,
		int64_t p_sw,
		int64_t p_sh
	) const;																					// 获取图像数据
	godot::Ref<godot::Image> get_image() const;													// 获取图像
	godot::Ref<godot::Texture2D> get_texture();													// 获取纹理
};

