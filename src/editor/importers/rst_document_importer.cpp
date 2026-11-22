#include "rst_document_importer.h"

#include "formats/rst/rst_document.h"
#include "formats/rst/rst_reference_index.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace {
	static String normalize_path(const String& p_path) {
		String path = p_path.strip_edges().replace("\\", "/");
		while (path.begins_with("./")) {
			path = path.substr(2);
		}
		return path.simplify_path();
	}

	static String role_target(const String& p_content) {
		const int64_t open = p_content.rfind("<");
		const int64_t close = p_content.rfind(">");
		if (open >= 0 && close > open) {
			return p_content.substr(open + 1, close - open - 1).strip_edges();
		}
		return p_content.strip_edges();
	}

	static void append_reference(PackedStringArray& r_references, const String& p_reference) {
		const String reference = normalize_path(p_reference);
		if (!reference.is_empty() && !reference.begins_with(":") && !r_references.has(reference)) {
			r_references.append(reference);
		}
	}

	static String resolve_reference_path(const String& p_source_file, const String& p_reference, bool p_is_document);

	static void append_resolved_role_references(PackedStringArray& r_references, const String& p_source_file, const String& p_line, const String& p_role, bool p_is_document) {
		const String marker = String(":") + p_role + ":" + String::chr(96);
		int64_t offset = 0;
		while (true) {
			const int64_t start = p_line.find(marker, offset);
			if (start < 0) {
				break;
			}
			const int64_t content_start = start + marker.length();
			const int64_t end = p_line.find(String::chr(96), content_start);
			if (end < 0) {
				break;
			}
			const String target = role_target(p_line.substr(content_start, end - content_start));
			append_reference(r_references, resolve_reference_path(p_source_file, target, p_is_document));
			offset = end + 1;
		}
	}

	static String resolve_reference_path(const String& p_source_file, const String& p_reference, bool p_is_document) {
		String reference = p_reference.strip_edges().replace("\\", "/");
		const int64_t fragment = reference.find("#");
		if (fragment >= 0) {
			reference = reference.substr(0, fragment);
		}
		if (reference.begins_with("uid://")) {
			reference = ResourceUID::uid_to_path(reference);
		}
		if (reference.is_empty()) {
			return String();
		}
		if (!reference.begins_with("res://")) {
			reference = p_source_file.get_base_dir().path_join(reference);
		}
		reference = normalize_path(reference);
		if (!reference.begins_with("res://")) {
			return String();
		}
		if (p_is_document && reference.get_extension().is_empty()) {
			reference += ".rst";
		}
		return reference;
	}

	static void append_reference_role_dependencies(PackedStringArray& r_references, const String& p_source_file, const String& p_line, const Ref<RSTReferenceIndex>& p_reference_index) {
		if (p_reference_index.is_null()) {
			return;
		}
		const String marker = String(":ref:") + String::chr(96);
		int64_t offset = 0;
		while (true) {
			const int64_t start = p_line.find(marker, offset);
			if (start < 0) {
				break;
			}
			const int64_t content_start = start + marker.length();
			const int64_t end = p_line.find(String::chr(96), content_start);
			if (end < 0) {
				break;
			}
			String target = p_reference_index->resolve_reference(p_source_file, role_target(p_line.substr(content_start, end - content_start)));
			const int64_t fragment = target.find("#");
			if (fragment >= 0) {
				target = target.substr(0, fragment);
			}
			if (target.begins_with("uid://")) {
				target = ResourceUID::uid_to_path(target);
			}
			append_reference(r_references, target);
			offset = end + 1;
		}
	}

	static PackedStringArray scan_rst_references(const String& p_source_file, const String& p_text, const Ref<RSTReferenceIndex>& p_reference_index) {
		const PackedStringArray lines = p_text.replace("\r\n", "\n").replace("\r", "\n").split("\n", true);
		PackedStringArray references;
		for (int32_t i = 0; i < lines.size(); i++) {
			const String line = lines[i];
			const String trimmed = line.strip_edges();
			if (trimmed.begins_with(".. image::")) {
				append_reference(references, resolve_reference_path(p_source_file, trimmed.substr(String(".. image::").length()), false));
			}
			else if (trimmed.begins_with(".. figure::")) {
				append_reference(references, resolve_reference_path(p_source_file, trimmed.substr(String(".. figure::").length()), false));
			}
			else if (trimmed.begins_with(".. toctree::")) {
				const int32_t indent = line.length() - line.strip_edges(true, false).length();
				for (int32_t j = i + 1; j < lines.size(); j++) {
					const String item_line = lines[j];
					const String item = item_line.strip_edges();
					const int32_t item_indent = item_line.length() - item_line.strip_edges(true, false).length();
					if (!item.is_empty() && item_indent <= indent) {
						break;
					}
					if (item.is_empty() || item.begins_with(":")) {
						continue;
					}
					append_reference(references, resolve_reference_path(p_source_file, role_target(item), true));
				}
			}
			append_resolved_role_references(references, p_source_file, line, "doc", true);
			append_resolved_role_references(references, p_source_file, line, "download", false);
			append_reference_role_dependencies(references, p_source_file, line, p_reference_index);
		}
		return references;
	}

	static bool dependency_exists(const String& p_path) {
		if (!p_path.begins_with("res://")) {
			return false;
		}
		ResourceLoader* loader = ResourceLoader::get_singleton();
		return loader != nullptr && loader->exists(p_path);
	}

	static void append_import_dependency(const RSTDocumentImporter* p_importer, const String& p_path) {
		String path = p_path;
		if (path.begins_with("uid://")) {
			path = ResourceUID::uid_to_path(path);
		}
		if (dependency_exists(path)) {
			const_cast<RSTDocumentImporter*>(p_importer)->append_import_external_resource(path);
		}
	}
} // namespace

void RSTDocumentImporter::_bind_methods() {
}

String RSTDocumentImporter::_get_importer_name() const {
	return "dockit.rst_document";
}

String RSTDocumentImporter::_get_visible_name() const {
	return tr("reStructuredText Document", "Godot Dockit");
}

PackedStringArray RSTDocumentImporter::_get_recognized_extensions() const {
	return { "rst" };
}

String RSTDocumentImporter::_get_save_extension() const {
	return "res";
}

String RSTDocumentImporter::_get_resource_type() const {
	return String(RSTDocument::get_class_static());
}

int32_t RSTDocumentImporter::_get_format_version() const {
	return FORMAT_VERSION;
}

PackedStringArray RSTDocumentImporter::_get_build_dependencies(const String& p_path) const {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	if (file.is_null()) {
		return {};
	}
	const String text = file->get_as_text();
	file->close();
	const Ref<RSTReferenceIndex> reference_index = RSTReferenceIndex::rebuild_from_project_settings(false, false);
	return scan_rst_references(p_path, text, reference_index);
}

int32_t RSTDocumentImporter::_get_preset_count() const {
	return 1;
}

String RSTDocumentImporter::_get_preset_name(int32_t) const {
	return "Default";
}

TypedArray<Dictionary> RSTDocumentImporter::_get_import_options(const String&, int32_t) const {
	return {};
}

bool RSTDocumentImporter::_get_option_visibility(const String&, const StringName&, const Dictionary&) const {
	return false;
}

Error RSTDocumentImporter::_import(const String& p_source_file, const String& p_save_path, const Dictionary&, const TypedArray<String>&, const TypedArray<String>&) const {
	Ref<FileAccess> file = FileAccess::open(p_source_file, FileAccess::READ);
	if (file.is_null()) {
		ERR_PRINT("Cannot open RST file: " + p_source_file);
		return ERR_CANT_OPEN;
	}
	const String text = file->get_as_text();
	file->close();

	Ref<RSTReferenceIndex> reference_index = RSTReferenceIndex::rebuild_from_project_settings(false, false);
	if (reference_index.is_null()) {
		UtilityFunctions::push_warning("RST reference index is unavailable; relative links will use resource paths: " + p_source_file);
	}

	Ref<RSTDocument> document;
	document.instantiate();
	document->set_text(text);
	document->set_source_path(p_source_file);
	document->set_reference_index(reference_index);

	const PackedStringArray dependencies = scan_rst_references(p_source_file, text, reference_index);
	for (int32_t i = 0; i < dependencies.size(); i++) {
		// Cross-document targets are path/UID data, not imported resource references.
		// Keeping RST documents out avoids cycles such as reciprocal :doc: links.
		if (dependencies[i].get_extension().to_lower() == "rst") {
			continue;
		}
		append_import_dependency(this, dependencies[i]);
	}
	const String save_path = p_save_path + String(".") + _get_save_extension();
	const Error save_error = ResourceSaver::get_singleton()->save(document, save_path);
	if (save_error != OK) {
		ERR_PRINT("Failed to save RST document resource: " + save_path + ", error: " + String::num_int64(save_error));
	}
	return save_error;
}

bool RSTDocumentImporter::_can_import_threaded() const {
	return false;
}
