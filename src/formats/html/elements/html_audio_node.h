#pragma once

#include "html_audio_element.h"
#include <litehtml/html_tag.h>

namespace dockit {
class HTMLAudioNode final : public litehtml::html_tag {
	godot::Rect2 content_box;

public:
	using html_tag::html_tag;
	double volume = 1.0;
	double time = 0.0;
	double duration = 0.0;
	godot::String current_src;
	bool muted = false;
	bool mute_overridden = false;
	bool paused = true;
	HTMLAudioPart focus_part = HTMLAudioPart::play;

	void compute_styles(bool p_recursive) override;												// 计算样式
	std::shared_ptr<litehtml::render_item> create_render_item(
		const std::shared_ptr<litehtml::render_item>& p_parent
	) override;																					// 创建音频控件的布局渲染项
	void draw(
		litehtml::uint_ptr p_hdc,
		litehtml::pixel_t p_x,
		litehtml::pixel_t p_y,
		const litehtml::position* p_clip,
		const std::shared_ptr<litehtml::render_item>& p_item
	) override;																					// 绘制音频控件及其播放状态
	HTMLAudioPart part_at(const godot::Vector2& p_point) const;									// 查找指定坐标对应的音频控件部件
	godot::Rect2 part_rect(HTMLAudioPart p_part) const;											// 获取音频控件部件的绘制区域
	bool has_controls() const;																	// 检查是否包含控件
	bool is_muted() const;																		// 检查音频是否已静音
};
}
