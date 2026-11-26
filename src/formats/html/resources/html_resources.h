#pragma once

#include "html_image_metadata.h"
#include "html_resource_paths.h"
#include "html_remote_stylesheets.h"
#include "formats/html/css/css_style_sheet.h"

#include <litehtml/types.h>

#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dockit {
	bool is_html_stylesheet_link(const char* p_rel);											// 判断 rel 是否声明默认应用的样式表
	std::optional<godot::String> read_html_stylesheet(const godot::String& p_path);				// 读取大小受限的本地 UTF-8 样式表
	std::optional<godot::String> read_html_svg_source(const godot::String& p_path);				// 读取大小受限的本地 UTF-8 SVG 文件

	class HTMLResources {
		HTMLResourceBase fallback_base;
		HTMLResourceBase document_base;
		godot::Dictionary textures;
		godot::Dictionary stylesheet_resources;
		godot::Dictionary local_fonts;
		godot::Dictionary image_dpi;
		godot::Dictionary image_orientations;
		godot::Dictionary svg_clip_sources;
		godot::HashMap<godot::String, godot::Ref<godot::Texture2D>> images;
		godot::HashMap<uint64_t, double> image_elapsed;
		godot::HashMap<godot::String, godot::Ref<godot::FontFile>> fonts;
		godot::HashMap<godot::String, HTMLImageMetadata> image_metadata;
		godot::HashSet<godot::String> pending_layout_images;
		godot::HashSet<godot::String> pending_paint_images;
		godot::Callable remote_image_callback;
		godot::HashMap<godot::String, RemoteStylesheet> remote_stylesheets;
		godot::HashSet<godot::String> pending_stylesheets;
		godot::Callable remote_stylesheet_callback;
		godot::Callable stylesheet_changed_callback;
		std::vector<std::pair<godot::Ref<CSSStyleSheet>, godot::Callable>> stylesheet_connections;
		bool recording_paint = false;
		int stylesheet_reads = 0;
		int failed_remote_resources = 0;

		const HTMLImageMetadata& get_image_metadata(const godot::String& p_path);
		godot::String get_url_dir(const char* p_css_base) const;
		godot::String resolve_baked_stylesheet_path(const godot::String& p_path, const godot::String& p_url) const;
		godot::String resolve_remote_resource(const godot::String& p_url, std::string_view p_css_base) const;
		godot::Ref<godot::Texture2D> load_texture_file(const godot::String& p_path);
		godot::Ref<godot::FontFile> load_font_file(const godot::String& p_path);

	public:
		bool advance_embedded_images(double p_delta); // 推进可选动画图像资源并报告是否需要继续处理
		bool update_embedded_textures(const godot::Dictionary& p_textures); // 更新内存纹理资源并清除对应缓存
		HTMLResources(const godot::String& p_base_dir, const godot::Dictionary& p_textures,
			const godot::Dictionary& p_stylesheet_resources, const godot::Dictionary& p_local_fonts, const godot::Dictionary& p_image_dpi,
			const godot::Dictionary& p_image_orientations, const godot::Dictionary& p_svg_clip_sources);
		~HTMLResources();

		static std::string_view stylesheet_path(std::string_view p_css_base);					// 获取 CSS 基准地址对应的本地样式表路径
		void set_source_path(const godot::String& p_path);										// 设置源路径
		void set_document_base_href(const char* p_href);										// 设置文档基准链接地址
		const HTMLResourceBase& get_document_base() const { return document_base; }				// 获取文档基准
		const HTMLResourceBase& get_fallback_base() const { return fallback_base; }				// 获取回退基准
		void reset_stylesheet_reads();															// 重置样式表读取次数
		godot::String resolve_stylesheet_source(
			const godot::String& p_url,
			std::string_view p_css_base = {}
		) const;																				// 解析并定位样式表源
		void set_remote_stylesheet_callback(const godot::Callable& p_callback);					// 设置远程样式表回调
		void set_stylesheet_changed_callback(const godot::Callable& p_callback);				// 设置样式表变化回调
		godot::Ref<CSSStyleSheet> get_stylesheet_resource(const godot::String& p_path);			// 获取样式表资源
		bool receive_remote_stylesheet(
			const godot::String& p_url,
			const godot::String& p_final_url,
			const godot::String& p_text
		);																						// 接收远程样式表
		void set_remote_image_callback(const godot::Callable& p_callback);						// 设置远程图像回调
		void begin_image_layout();																// 开始图像布局
		int pending_resource_count() const;														// 获取尚未完成加载的资源数量
		int get_failed_remote_resource_count() const { return failed_remote_resources; }		// 获取失败项远程资源数量
		void begin_image_paint();																// 开始图像绘制
		void end_image_paint();																	// 结束图像绘制
		enum class ImageUpdate { none, paint, layout };
		ImageUpdate receive_remote_image(
			const godot::String& p_url,
			const godot::Ref<godot::Texture2D>& p_texture
		);																						// 接收远程图像
		void import_css(std::string& r_text, const std::string& p_url, std::string& r_css_base);// 读取并展开 CSS 中的导入样式表
		godot::String resolve_svg_clip_source(
			std::string_view p_url,
			const char* p_css_base
		) const;																				// 解析并定位SVG裁剪源
		std::optional<godot::String> get_svg_clip_source(const godot::String& p_path) const;	// 获取SVG裁剪源
		godot::String resolve_image_source(std::string_view p_source) const;					// 返回保留片段的项目图片路径
		bool is_vector_image(const char* p_src, const char* p_css_base) const;					// 判断本地图片是否为 SVG
		double get_image_dpi(const char* p_src, const char* p_css_base);						// 获取导入保存或从源图片读取的 DPI
		godot::Ref<godot::Texture2D> load_texture(
			const char* p_src,
			const char* p_css_base,
			litehtml::css_image_orientation p_orientation = litehtml::css_image_orientation::from_image,
			bool p_affects_layout = true);														// 加载纹理
		godot::Ref<godot::FontFile> load_font(const char* p_src, const char* p_css_base);		// 加载字体
	};
}
