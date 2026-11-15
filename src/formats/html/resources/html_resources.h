#pragma once

#include "html_image_metadata.h"

#include <litehtml/types.h>

#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace docrender {
	bool is_html_stylesheet_link(const char* p_rel);								// 判断 rel 是否声明默认应用的样式表
	std::optional<godot::String> read_html_stylesheet(const godot::String& p_path); // 读取大小受限的本地 UTF-8 样式表

	class HTMLResources {
		godot::String base_dir;
		godot::Dictionary textures;
		godot::Dictionary local_stylesheets;
		godot::Dictionary local_fonts;
		godot::Dictionary image_dpi;
		godot::Dictionary image_orientations;
		godot::HashMap<godot::String, godot::Ref<godot::Texture2D>> images;
		godot::HashMap<godot::String, godot::Ref<godot::FontFile>> fonts;
		godot::HashMap<godot::String, HTMLImageMetadata> image_metadata;
		int stylesheet_reads = 0;

		const HTMLImageMetadata& get_image_metadata(const godot::String& p_path);
		godot::String get_url_dir(const char* p_css_base) const;
		godot::Ref<godot::Texture2D> load_texture_file(const godot::String& p_path);
		godot::Ref<godot::FontFile> load_font_file(const godot::String& p_path);

	public:
		HTMLResources(const godot::String& p_base_dir, const godot::Dictionary& p_textures,
			const godot::Dictionary& p_local_stylesheets, const godot::Dictionary& p_local_fonts, const godot::Dictionary& p_image_dpi,
			const godot::Dictionary& p_image_orientations);

		static std::string_view stylesheet_path(std::string_view p_css_base);
		void reset_stylesheet_reads();
		void import_css(std::string& r_text, const std::string& p_url, std::string& r_css_base);
		godot::String resolve_image_source(std::string_view p_source) const; // 返回保留片段的项目图片路径
		bool is_vector_image(const char* p_src, const char* p_css_base) const; // 判断本地图片是否为 SVG
		double get_image_dpi(const char* p_src, const char* p_css_base); // 获取导入保存或从源图片读取的 DPI
		godot::Ref<godot::Texture2D> load_texture(const char* p_src, const char* p_css_base,
			litehtml::css_image_orientation p_orientation = litehtml::css_image_orientation::from_image);
		godot::Ref<godot::FontFile> load_font(const char* p_src, const char* p_css_base);
	};
}
