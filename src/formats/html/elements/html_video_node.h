#pragma once

#include "html_video_element.h"
#include "formats/html/paint/html_paint_recorder.h"

#include <litehtml/html_tag.h>

namespace dockit {
	enum class HTMLVideoPart {
		none,
		play,
		position,
		mute,
		volume,
		fullscreen
	};

	struct HTMLVideoControlPainter {
		std::function<void(const godot::Rect2&, const godot::Color&)> fill;
		std::function<void(HTMLControlIcon, const godot::Rect2&)> icon;
		std::function<void(const godot::Rect2&, const godot::String&)> text;
	};

	class HTMLVideoNode final : public litehtml::html_tag {
	public:
		using html_tag::html_tag;
		godot::Rect2 content_box;
		godot::Ref<godot::Texture2D> frame;
		godot::Vector2i dimensions;
		godot::String current_src;
		double volume = 1.0;
		double time = 0.0;
		double duration = 0.0;
		std::optional<double> seek_preview;
		HTMLVideoPart focus_part = HTMLVideoPart::play;
		bool available = false;
		bool paused = true;
		bool seekable = false;
		bool fullscreen = false;
		bool show_controls = true;
		std::optional<bool> muted;

		bool is_replaced() const override;														// 检查替换元素
		void parse_attributes() override;														// 解析属性
		void get_content_size(litehtml::size& r_size, litehtml::pixel_t p_width) override;		// 获取内容尺寸
		std::shared_ptr<litehtml::render_item> create_render_item(
			const std::shared_ptr<litehtml::render_item>& p_parent
		) override;																				// 创建 video 控件的布局渲染项
		void draw(
			litehtml::uint_ptr p_hdc,
			litehtml::pixel_t p_x,
			litehtml::pixel_t p_y,
			const litehtml::position* p_clip,
			const std::shared_ptr<litehtml::render_item>& p_item
		) override;																				// 绘制视频画面及播放控件
		bool has_controls() const;																// 检查是否包含控件
		bool is_muted() const;																	// 检查视频是否已静音
		godot::Rect2 part_rect(const godot::Rect2& p_box, HTMLVideoPart p_part) const;			// 获取视频控件部件的绘制区域
		HTMLVideoPart part_at(const godot::Rect2& p_box, const godot::Vector2& p_point) const;	// 查找指定坐标对应的视频控件部件
		void draw_controls(
			const godot::Rect2& p_box,
			const HTMLVideoControlPainter& p_painter,
			bool p_focused
		) const;																				// 绘制控件
	};
}
