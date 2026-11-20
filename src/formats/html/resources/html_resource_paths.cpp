#include "html_resource_paths.h"

#include "formats/http_url.h"
#include "formats/html/markup/html_markup.h"

#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <litehtml/html.h>

#include <algorithm>

using namespace godot;

namespace dockit {
	namespace {
		String resolve_local_path(const String& p_src, const String& p_base_dir) {
			const String source = decode_html_fragment(p_src.strip_edges()).replace("\\", "/");
			if (source.is_empty()) {
				return {};
			}
			if (source.begins_with("uid://")) {
				auto* uids = ResourceUID::get_singleton();
				const int64_t id = uids->text_to_id(source);
				if (id == ResourceUID::INVALID_ID || uids->id_to_text(id) != source || !uids->has_id(id)) return {};
				return normalize_html_resource_path(uids->get_id_path(id)).value_or(String());
			}
			if (source.begins_with("res://") || source.begins_with("user://")) {
				return normalize_html_resource_path(source).value_or(String());
			}
			if (source.is_absolute_path() || source.begins_with("/")) {
				return p_base_dir.is_absolute_path() ? normalize_html_resource_path(source).value_or(String()) : String();
			}
			return normalize_html_resource_path(p_base_dir.path_join(source)).value_or(String());
		}

		String directory_uri(const String& p_path) {
			return p_path.is_empty() || p_path.ends_with("/") ? p_path : p_path + String("/");
		}

		int64_t url_suffix(const String& p_url) {
			const int64_t query = p_url.find("?");
			const int64_t fragment = p_url.find("#");
			return query < 0 ? fragment : fragment < 0 ? query : std::min(query, fragment);
		}

		String without_url_suffix(const String& p_url) {
			const int64_t suffix = url_suffix(p_url);
			return suffix < 0 ? p_url : p_url.substr(0, suffix);
		}

		String resolve_local_url(const String& p_href, const HTMLResourceBase& p_base) {
			const int64_t suffix = url_suffix(p_href);
			if (p_href.is_empty() || suffix == 0) {
				const int64_t cut = p_href.begins_with("?") ? url_suffix(p_base.uri) : p_base.uri.find("#");
				return (cut < 0 ? p_base.uri : p_base.uri.substr(0, cut)) + p_href;
			}
			const String reference = suffix < 0 ? p_href : p_href.substr(0, suffix);
			const String path = resolve_local_path(reference, p_base.directory);
			if (path.is_empty()) return {};
			const String decoded = decode_html_fragment(reference).replace("\\", "/");
			const String last = decoded.get_file();
			const bool directory = decoded.ends_with("/") || last == "." || last == "..";
			return (directory ? directory_uri(path) : path) + (suffix < 0 ? String() : p_href.substr(suffix));
		}

		String normalize_access_root(int32_t p_access, const String& p_root_subfolder) {
			const String subfolder = p_root_subfolder.strip_edges().replace("\\", "/");
			if (p_access == HTML_RESOURCE_ACCESS_RESOURCES) {
				if (subfolder.is_empty()) return "res://";
				if (!subfolder.begins_with("res://") && (subfolder.is_absolute_path() || subfolder.begins_with("/"))) return {};
				const String path = subfolder.begins_with("res://") ? subfolder : String("res://").path_join(subfolder);
				const auto normalized = normalize_html_resource_path(path);
				return normalized ? directory_uri(*normalized) : String();
			}
			if (p_access == HTML_RESOURCE_ACCESS_USER_DATA) {
				if (subfolder.is_empty()) return "user://";
				if (!subfolder.begins_with("user://") && (subfolder.is_absolute_path() || subfolder.begins_with("/"))) return {};
				const String path = subfolder.begins_with("user://") ? subfolder : String("user://").path_join(subfolder);
				const auto normalized = normalize_html_resource_path(path);
				return normalized ? directory_uri(*normalized) : String();
			}
			if (p_access == HTML_RESOURCE_ACCESS_FILE_SYSTEM && !subfolder.is_empty() && subfolder.is_absolute_path()) {
				if (subfolder.begins_with("res://") || subfolder.begins_with("user://")) return {};
				const auto normalized = normalize_html_resource_path(subfolder);
				return normalized ? directory_uri(*normalized) : String();
			}
			return {};
		}
	}

	String resolve_html_resource_base_dir(int32_t p_access, const String& p_root_subfolder, const String& p_source_path) {
		String fallback = normalize_access_root(p_access, p_root_subfolder);
		if (p_source_path.is_empty() || is_http_url(p_source_path)) return fallback;

		String source_path = p_source_path;
		if (source_path.begins_with("uid://")) {
			auto* uids = ResourceUID::get_singleton();
			const int64_t id = uids->text_to_id(source_path);
			if (id == ResourceUID::INVALID_ID || !uids->has_id(id)) return fallback;
			source_path = uids->get_id_path(id);
		}
		const auto normalized_source = normalize_html_resource_path(source_path);
		return normalized_source ? directory_uri(normalized_source->get_base_dir()) : fallback;
	}

	HTMLResourceBase make_html_resource_base(const String& p_directory, const String& p_source_path) {
		const String directory = normalize_html_resource_path(p_directory).value_or(String());
		const auto source = normalize_html_resource_path(p_source_path);
		return source ? HTMLResourceBase{ *source, directory_uri(source->get_base_dir()) } :
			HTMLResourceBase{ directory_uri(directory), directory_uri(directory) };
	}

	HTMLResourceBase resolve_html_document_base(const HTMLResourceBase& p_fallback, const char* p_href) {
		if (!p_href) return p_fallback;
		const String href = String::utf8(p_href).strip_edges();
		if (is_http_url(p_fallback.uri) || is_http_url(href)) {
			const String uri = resolve_http_url(href, p_fallback.uri);
			if (uri.is_empty()) return p_fallback;
			return { uri, resolve_http_url(".", uri) };
		}
		const String uri = resolve_local_url(href, p_fallback);
		if (uri.is_empty()) return p_fallback;
		const String path = without_url_suffix(uri);
		return { uri, path.ends_with("/") ? normalize_html_resource_path(path).value_or(p_fallback.directory) : path.get_base_dir() };
	}

	String resolve_html_link_href(const String& p_href, const HTMLResourceBase& p_base) {
		const String href = p_href.strip_edges();
		if (href.begins_with("res://") || href.begins_with("user://") || href.begins_with("uid://"))
			return resolve_local_url(href, p_base);
		if (is_http_url(p_base.uri) || is_http_url(href)) return resolve_http_url(href, p_base.uri);
		const String prefix = without_url_suffix(href).get_slice("/", 0);
		if (prefix.contains(":") && !href.begins_with("res://") && !href.begins_with("user://") && !href.begins_with("uid://"))
			return href;
		return resolve_local_url(href, p_base);
	}

	std::optional<String> normalize_html_resource_path(const String& p_path) {
		const String normalized_path = p_path.replace("\\", "/");
		const int64_t prefix = normalized_path.begins_with("res://") ? 6 : normalized_path.begins_with("user://") ? 7 : 0;
		if (prefix == 0 && normalized_path.is_absolute_path()) {
			if (normalized_path.contains("?") || normalized_path.contains("#")) return std::nullopt;
			for (int64_t i = 0; i < normalized_path.length(); ++i) {
				if (normalized_path[i] < 32) return std::nullopt;
			}
			return normalized_path.simplify_path();
		}
		if (prefix == 0) return std::nullopt;
		const String relative = normalized_path.substr(prefix);
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
		return normalized_path.substr(0, prefix) + String("/").join(components);
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
			? String("res://.dockit/video/") + p_path.sha256_text() + "." + p_path.get_extension().to_lower()
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
