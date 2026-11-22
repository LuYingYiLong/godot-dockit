#include "rst_document.h"

#include "rst_reference_index.h"

#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace {
	static String normalize_rst_path(const String& p_path) {
		String path = p_path.strip_edges().replace("\\", "/");
		while (path.begins_with("./")) {
			path = path.substr(2);
		}
		return path.simplify_path();
	}
} // namespace

void RSTDocument::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_text", "text"), &RSTDocument::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &RSTDocument::get_text);
	ClassDB::bind_method(D_METHOD("set_source_path", "source_path"), &RSTDocument::set_source_path);
	ClassDB::bind_method(D_METHOD("get_source_path"), &RSTDocument::get_source_path);
	ClassDB::bind_method(D_METHOD("set_reference_index", "reference_index"), &RSTDocument::set_reference_index);
	ClassDB::bind_method(D_METHOD("get_reference_index"), &RSTDocument::get_reference_index);
	ClassDB::bind_method(D_METHOD("resolve_path", "path"), &RSTDocument::resolve_path);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_path", PROPERTY_HINT_FILE, "*.rst"), "set_source_path", "get_source_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "reference_index", PROPERTY_HINT_RESOURCE_TYPE, "Resource", PROPERTY_USAGE_STORAGE), "set_reference_index", "get_reference_index");
}

void RSTDocument::set_text(const String& p_text) {
	text = p_text;
	emit_changed();
}

String RSTDocument::get_text() const {
	return text;
}

void RSTDocument::set_source_path(const String& p_source_path) {
	source_path = p_source_path.strip_edges().replace("\\", "/");
	emit_changed();
}

String RSTDocument::get_source_path() const {
	return source_path;
}

void RSTDocument::set_reference_index(const Ref<RSTReferenceIndex>& p_reference_index) {
	reference_index = p_reference_index;
	emit_changed();
}

Ref<RSTReferenceIndex> RSTDocument::get_reference_index() const {
	return reference_index;
}

String RSTDocument::resolve_path(const String& p_path) const {
	const String normalized_path = normalize_rst_path(p_path);
	if (normalized_path.is_empty()) {
		return String();
	}
	if (normalized_path.begins_with("uid://")) {
		return ResourceUID::uid_to_path(normalized_path);
	}
	if (normalized_path.begins_with("res://")) {
		return normalized_path;
	}
	if (source_path.is_empty()) {
		return normalized_path;
	}
	return source_path.get_base_dir().path_join(normalized_path).simplify_path();
}
