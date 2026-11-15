#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace docrender {
	struct HTMLAssets {
		godot::Dictionary textures;
		godot::Dictionary stylesheets;
		godot::Dictionary fonts;
		godot::Dictionary image_dpi;
		godot::Dictionary image_orientations;
	};

	HTMLAssets collect_html_assets(
		const godot::String& p_html,
		const godot::String& p_base_dir
	);																				// 从 HTML 和关联样式表中收集纹理与 CSS 依赖
}
