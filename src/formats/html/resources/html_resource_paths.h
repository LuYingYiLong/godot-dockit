#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <optional>

namespace godot { class Texture2D; }

namespace dockit {
	enum HTMLResourceAccess : int32_t {
		HTML_RESOURCE_ACCESS_RESOURCES,
		HTML_RESOURCE_ACCESS_USER_DATA,
		HTML_RESOURCE_ACCESS_FILE_SYSTEM,
	};

	// 解析 HTML 资源路径
	struct HTMLResourceBase {
		godot::String uri;
		godot::String directory;
	};

	godot::String resolve_html_resource_base_dir(
		int32_t p_access,
		const godot::String& p_root_subfolder,
		const godot::String& p_source_path = {}
	);																							// 根据访问方式解析 HTML 资源基准目录
	HTMLResourceBase make_html_resource_base(
		const godot::String& p_directory,
		const godot::String& p_source_path = {}
	);																							// 从目录和来源路径构造 HTML 资源基准
	HTMLResourceBase resolve_html_document_base(
		const HTMLResourceBase& p_fallback,
		const char* p_href
	);																							// 根据 HTML base 地址解析文档资源基准
	godot::String resolve_html_link_href(
		const godot::String& p_href,
		const HTMLResourceBase& p_base
	);																							// 根据资源基准解析 HTML 链接地址

	godot::Ref<godot::Texture2D> load_html_image_file(const godot::String& p_path);				// 读取已解析的本地纹理路径
	godot::String resolve_html_audio_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																							// 解析 HTML 音频资源路径
	godot::String resolve_html_video_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																							// 解析 HTML 视频资源路径
	godot::String packed_html_video_path(const godot::String& p_path);							// 将视频路径转换为打包资源可用的地址
	std::optional<godot::String> normalize_html_resource_path(
		const godot::String& p_path
	);																							// 规范化项目路径、用户数据路径或绝对文件系统路径
	godot::String resolve_html_image_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																							// 按基目录解析并校验支持的本地图片路径
	godot::String resolve_html_stylesheet_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																							// 按基目录解析并校验本地 CSS 文件路径
	godot::String resolve_html_font_path(
		const godot::String& p_src,
		const godot::String& p_base_dir
	);																							// 解析项目内的动态字体路径
	godot::String resolve_html_svg_clip_path(
		const godot::String& p_url, const godot::String& p_base_dir
	);																							// 解析带非空片段的本地 SVG 文件路径
}
