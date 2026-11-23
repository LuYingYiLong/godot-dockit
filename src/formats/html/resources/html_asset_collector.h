#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace dockit {
	struct HTMLAssets {
		godot::Dictionary textures;
		godot::Dictionary stylesheet_resources;
		godot::PackedStringArray stylesheet_paths;
		godot::Dictionary fonts;
		godot::Dictionary image_dpi;
		godot::Dictionary image_orientations;
		godot::Dictionary svg_clip_sources;
		godot::Dictionary audio_streams;
		godot::Dictionary video_sources;
	};

	HTMLAssets collect_html_assets(
		const godot::String& p_html,
		const godot::String& p_base_dir,
		const godot::String& p_source_path = {}
	);																							// 从 HTML 和关联样式表中收集纹理与 CSS 依赖

	// 收集 HTML 和 CSS 资源路径
	godot::PackedStringArray collect_html_stylesheet_paths(
		const godot::String& p_html,
		const godot::String& p_base_dir,
		const godot::String& p_source_path = {}
	);																							// 收集 HTML 中引用的本地样式表路径
	godot::PackedStringArray collect_css_stylesheet_imports(
		const godot::String& p_css,
		const godot::String& p_source_path
	);																							// 收集 CSS 中通过 @import 引用的样式表
}
