#include "html_resource_paths.h"

#include "formats/html/markup/html_markup.h"

#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <litehtml/html.h>

using namespace godot;

namespace docrender {
	namespace {
		String resolve_local_path(const String& p_src, const String& p_base_dir) {
			const String source = decode_html_fragment(p_src.strip_edges()).replace("\\", "/");
			if (source.is_empty() || source.begins_with("/")) {
				return {};
			}
			if (source.begins_with("uid://")) {
				auto* uids = ResourceUID::get_singleton();
				const int64_t id = uids->text_to_id(source);
				if (id == ResourceUID::INVALID_ID || uids->id_to_text(id) != source || !uids->has_id(id)) return {};
				return normalize_html_resource_path(uids->get_id_path(id)).value_or(String());
			}
			const String absolute = source.begins_with("res://") || source.begins_with("user://") ? source : p_base_dir.path_join(source);
			return normalize_html_resource_path(absolute).value_or(String());
		}
	}

	std::optional<String> normalize_html_resource_path(const String& p_path) {
		const int64_t prefix = p_path.begins_with("res://") ? 6 : p_path.begins_with("user://") ? 7 : 0;
		if (prefix == 0) return std::nullopt;
		const String relative = p_path.substr(prefix).replace("\\", "/");
		if (relative.begins_with("/") || relative.contains(":") || relative.contains("?") || relative.contains("#")) {
			return std::nullopt;
		}
		for (int64_t i = 0; i < relative.length(); ++i) {
			if (relative[i] < 32) {
				return std::nullopt;
			}
		}

		PackedStringArray components;
		for (const String& component : relative.split("/", false)) {
			if (component == "..") {
				if (components.is_empty()) {
					return std::nullopt;
				}
				components.remove_at(components.size() - 1);
			}
			else if (component != ".") {
				// Windows may normalize trailing spaces and periods while opening paths.
				if (component.ends_with(" ") || component.ends_with(".")) {
					return std::nullopt;
				}
				components.append(component);
			}
		}
		return p_path.substr(0, prefix) + String("/").join(components);
	}

	String resolve_html_audio_path(const String& p_src, const String& p_base_dir) {
		return resolve_local_path(p_src, p_base_dir);
	}

	String resolve_html_video_path(const String& p_src, const String& p_base_dir) {
		const String path = resolve_local_path(p_src, p_base_dir);
		const String extension = path.get_extension().to_lower();
		return litehtml::is_one_of(extension, "mp4", "webm", "mkv", "mov", "m4v", "avi", "ogv", "mpeg", "mpg", "ts")
			? path : String();
	}

	String packed_html_video_path(const String& p_path) {
		return p_path.begins_with("res://")
			? String("res://.docrender/video/") + p_path.sha256_text() + "." + p_path.get_extension().to_lower()
			: String();
	}

	String resolve_html_image_path(const String& p_src, const String& p_base_dir) {
		const int64_t fragment = p_src.find("#");
		const String path = resolve_local_path(fragment < 0 ? p_src : p_src.substr(0, fragment), p_base_dir);
		const String extension = path.get_extension().to_lower();
		if (!litehtml::is_one_of(extension, "png", "jpg", "jpeg", "webp", "svg", "res", "tres")) {
			return {};
		}
		return path;
	}

	String resolve_html_stylesheet_path(const String& p_src, const String& p_base_dir) {
		const String path = resolve_local_path(p_src, p_base_dir);
		return path.get_extension().to_lower() == "css" ? path : String();
	}

	String resolve_html_font_path(const String& p_src, const String& p_base_dir) {
		const int64_t fragment = p_src.find("#");
		const String path = resolve_local_path(fragment < 0 ? p_src : p_src.substr(0, fragment), p_base_dir);
		const String extension = path.get_extension().to_lower();
		return litehtml::is_one_of(extension, "ttf", "otf", "ttc", "otc", "woff", "woff2", "res", "tres") ? path : String();
	}

	String resolve_html_svg_clip_path(const String& p_url, const String& p_base_dir) {
		const int64_t fragment = p_url.find("#");
		if (fragment <= 0 || fragment + 1 == p_url.length()) return {};
		const String path = resolve_local_path(p_url.substr(0, fragment), p_base_dir);
		return path.get_extension().nocasecmp_to("svg") == 0 ? path : String();
	}
}
