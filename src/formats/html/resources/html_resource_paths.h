#pragma once

#include <godot_cpp/variant/string.hpp>

#include <optional>

namespace docrender {
	godot::String resolve_html_audio_path(const godot::String& p_src, const godot::String& p_base_dir);
	std::optional<godot::String> normalize_html_resource_path(
		const godot::String& p_path
	);																				// 规范化项目内的 res:// 路径并拒绝越界路径
	godot::String resolve_html_image_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																				// 按基目录解析并校验支持的本地图片路径
	godot::String resolve_html_stylesheet_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																				// 按基目录解析并校验本地 CSS 文件路径
	godot::String resolve_html_font_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																				// 解析项目内的动态字体路径
	godot::String resolve_html_svg_clip_path(
		const godot::String& p_url, const godot::String& p_base_dir
	);																				// 解析带非空片段的本地 SVG 文件路径
}
