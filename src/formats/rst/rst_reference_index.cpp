#include "rst_reference_index.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace godot;

namespace {
	struct SourceDocument {
		String path;
		String root;
		String uid;
		String text;
		Dictionary labels;
		PackedStringArray referenced_labels;
		PackedStringArray document_references;
	};

	static String normalize_path(const String& p_path) {
		String path = p_path.strip_edges().replace("\\", "/");
		ProjectSettings* settings = ProjectSettings::get_singleton();
		if (settings != nullptr && path.is_absolute_path()) {
			path = settings->localize_path(path);
		}
		while (path.begins_with("./")) {
			path = path.substr(2);
		}
		return path.simplify_path();
	}

	static String normalize_label(const String& p_label) {
		return p_label.strip_edges().to_lower().replace(" ", "-");
	}

	static String make_reference_key(const String& p_source_path, const String& p_label) {
		return normalize_path(p_source_path) + String("|") + normalize_label(p_label);
	}

	static Error save_index_atomically(const Ref<RSTReferenceIndex>& p_index, const String& p_path) {
		ProjectSettings* settings = ProjectSettings::get_singleton();
		if (settings == nullptr || p_index.is_null()) {
			return ERR_UNAVAILABLE;
		}
		const String pending_path = "res://.godot/dockit_rst_reference_index.pending.tres";
		const String backup_path = "res://.godot/dockit_rst_reference_index.previous.tres";
		const String absolute_path = settings->globalize_path(p_path);
		const String pending_absolute_path = settings->globalize_path(pending_path);
		const String backup_absolute_path = settings->globalize_path(backup_path);
		DirAccess::remove_absolute(pending_absolute_path);
		DirAccess::remove_absolute(backup_absolute_path);

		const Error save_error = ResourceSaver::get_singleton()->save(p_index, pending_path);
		if (save_error != OK) {
			return save_error;
		}

		const bool had_previous = FileAccess::file_exists(p_path);
		if (had_previous && DirAccess::rename_absolute(absolute_path, backup_absolute_path) != OK) {
			DirAccess::remove_absolute(pending_absolute_path);
			return ERR_CANT_CREATE;
		}
		const Error rename_error = DirAccess::rename_absolute(pending_absolute_path, absolute_path);
		if (rename_error != OK) {
			if (had_previous) {
				DirAccess::rename_absolute(backup_absolute_path, absolute_path);
			}
			DirAccess::remove_absolute(pending_absolute_path);
			return rename_error;
		}
		if (had_previous) {
			DirAccess::remove_absolute(backup_absolute_path);
		}
		p_index->set_path(p_path);
		return OK;
	}

	static void update_fingerprint(uint64_t& r_hash, const String& p_value) {
		const CharString bytes = p_value.utf8();
		for (int64_t i = 0; i < bytes.length(); i++) {
			r_hash ^= static_cast<uint8_t>(bytes[i]);
			r_hash *= UINT64_C(1099511628211);
		}
		r_hash ^= UINT64_C(255);
		r_hash *= UINT64_C(1099511628211);
	}

	static String get_uid_for_path(const String& p_path) {
		const String uid = ResourceUID::path_to_uid(p_path);
		return uid.begins_with("uid://") ? uid : String();
	}

	static bool is_section_marker(char32_t p_character) {
		return p_character == '!' || p_character == '"' || p_character == '#' || p_character == '$' || p_character == '%' ||
			p_character == '&' || p_character == '\'' || p_character == '(' || p_character == ')' || p_character == '*' ||
			p_character == '+' || p_character == ',' || p_character == '-' || p_character == '.' || p_character == '/' ||
			p_character == ':' || p_character == ';' || p_character == '<' || p_character == '=' || p_character == '>' ||
			p_character == '?' || p_character == '@' || p_character == '[' || p_character == '\\' || p_character == ']' ||
			p_character == '^' || p_character == '_' || p_character == '`' || p_character == '{' || p_character == '|' ||
			p_character == '}' || p_character == '~';
	}

	static bool is_section_underline(const String& p_line, int32_t p_title_length) {
		const String line = p_line.strip_edges();
		if (line.length() < 2 || line.length() < p_title_length || !is_section_marker(line[0])) {
			return false;
		}
		for (int32_t i = 1; i < line.length(); i++) {
			if (line[i] != line[0]) {
				return false;
			}
		}
		return true;
	}

	static String role_target(const String& p_content) {
		const int64_t open = p_content.rfind("<");
		const int64_t close = p_content.rfind(">");
		if (open >= 0 && close > open) {
			return p_content.substr(open + 1, close - open - 1).strip_edges();
		}
		return p_content.strip_edges();
	}

	static void append_role_targets(PackedStringArray& r_targets, const String& p_line, const String& p_role) {
		const String marker = String(":") + p_role + ":`";
		int64_t offset = 0;
		while (true) {
			const int64_t start = p_line.find(marker, offset);
			if (start < 0) {
				break;
			}
			const int64_t content_start = start + marker.length();
			const int64_t end = p_line.find("`", content_start);
			if (end < 0) {
				break;
			}
			const String target = role_target(p_line.substr(content_start, end - content_start));
			if (!target.is_empty() && !r_targets.has(target)) {
				r_targets.append(target);
			}
			offset = end + 1;
		}
	}

	static void add_label(Dictionary& r_labels, const String& p_label, const String& p_anchor) {
		const String key = normalize_label(p_label);
		if (key.is_empty()) {
			return;
		}
		Array anchors;
		if (r_labels.has(key)) {
			anchors = r_labels[key];
		}
		if (!anchors.has(p_anchor)) {
			anchors.append(p_anchor);
		}
		r_labels[key] = anchors;
	}

	static void parse_document(SourceDocument& r_document) {
		const PackedStringArray lines = r_document.text.replace("\r\n", "\n").replace("\r", "\n").split("\n", true);
		Dictionary heading_counts;
		String pending_anchor;
		for (int32_t i = 0; i < lines.size(); i++) {
			const String line = lines[i];
			const String trimmed = line.strip_edges();
			const bool is_explicit_target = trimmed.begins_with(".. _") && trimmed.ends_with(":");
			const bool starts_heading = !trimmed.is_empty() && i + 1 < lines.size() && is_section_underline(lines[i + 1], trimmed.length());
			if (!pending_anchor.is_empty() && !trimmed.is_empty() && !is_explicit_target && !starts_heading) {
				pending_anchor = String();
			}
			if (is_explicit_target) {
				const String target = trimmed.substr(4, trimmed.length() - 5).strip_edges();
				if (!target.is_empty()) {
					add_label(r_document.labels, target, target);
					pending_anchor = target;
				}
			}
			if (i + 1 < lines.size()) {
				const String heading = trimmed;
				if (!heading.is_empty() && is_section_underline(lines[i + 1], heading.length())) {
					String anchor = normalize_label(heading);
					const int32_t occurrence = int32_t(heading_counts.get(anchor, 0));
					heading_counts[anchor] = occurrence + 1;
					if (occurrence > 0) {
						anchor += String("-") + String::num_int64(occurrence);
					}
					const String target_anchor = pending_anchor.is_empty() ? anchor : pending_anchor;
					add_label(r_document.labels, heading, target_anchor);
					add_label(r_document.labels, anchor, target_anchor);
					pending_anchor = String();
				}
			}
			append_role_targets(r_document.referenced_labels, line, "ref");
			append_role_targets(r_document.document_references, line, "doc");
			if (trimmed.begins_with(".. toctree::")) {
				const int32_t directive_indent = line.length() - line.strip_edges(true, false).length();
				for (int32_t j = i + 1; j < lines.size(); j++) {
					const String entry_line = lines[j];
					const String entry = entry_line.strip_edges();
					const int32_t entry_indent = entry_line.length() - entry_line.strip_edges(true, false).length();
					if (!entry.is_empty() && entry_indent <= directive_indent) {
						break;
					}
					if (entry.is_empty() || entry.begins_with(":")) {
						continue;
					}
					const String target = role_target(entry);
					if (!target.is_empty() && !r_document.document_references.has(target)) {
						r_document.document_references.append(target);
					}
				}
			}
		}
	}

	static void scan_directory(const String& p_root, const String& p_path, std::vector<SourceDocument>& r_documents) {
		Ref<DirAccess> directory = DirAccess::open(p_path);
		if (directory.is_null() || directory->list_dir_begin() != OK) {
			return;
		}
		while (true) {
			const String name = directory->get_next();
			if (name.is_empty()) {
				break;
			}
			if (name == "." || name == "..") {
				continue;
			}
			const String path = normalize_path(p_path.path_join(name));
			if (directory->current_is_dir()) {
				if (!name.begins_with(".")) {
					scan_directory(p_root, path, r_documents);
				}
				continue;
			}
			if (name.get_extension().to_lower() != "rst") {
				continue;
			}
			Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
			if (file.is_null()) {
				continue;
			}
			SourceDocument document;
			document.path = path;
			document.root = p_root;
			document.uid = get_uid_for_path(path);
			document.text = file->get_as_text();
			file->close();
			const bool already_scanned = std::any_of(r_documents.begin(), r_documents.end(), [&document](const SourceDocument& p_existing) {
				return p_existing.path == document.path;
			});
			if (already_scanned) {
				continue;
			}
			parse_document(document);
			r_documents.push_back(document);
		}
		directory->list_dir_end();
	}

	static String resolve_document_path(const String& p_source, const String& p_target) {
		String target = p_target.strip_edges().replace("\\", "/");
		const int64_t fragment = target.find("#");
		if (fragment >= 0) {
			target = target.substr(0, fragment);
		}
		if (target.begins_with("uid://")) {
			target = ResourceUID::uid_to_path(target);
		}
		if (target.is_empty()) {
			return String();
		}
		if (!target.begins_with("res://")) {
			target = p_source.get_base_dir().path_join(target);
		}
		target = normalize_path(target);
		if (!target.begins_with("res://")) {
			return String();
		}
		if (target.get_extension().is_empty()) {
			target += ".rst";
		}
		return target;
	}

	static int32_t find_document(const std::vector<SourceDocument>& p_documents, const String& p_path) {
		const String wanted = normalize_path(p_path);
		for (int32_t i = 0; i < int32_t(p_documents.size()); i++) {
			if (p_documents[i].path == wanted) {
				return i;
			}
		}
		return -1;
	}

	static Dictionary make_target(const SourceDocument& p_document, const String& p_anchor = String()) {
		Dictionary target;
		target["path"] = p_document.path;
		target["uid"] = p_document.uid;
		target["anchor"] = p_anchor;
		return target;
	}

	static String resolve_target_value(const Dictionary& p_target) {
		String value = p_target.has("uid") ? String(p_target["uid"]) : String();
		if (!value.is_empty() && ResourceUID::uid_to_path(value).is_empty()) {
			value = String();
		}
		if (value.is_empty()) {
			value = p_target.has("path") ? String(p_target["path"]) : String();
		}
		const String anchor = p_target.has("anchor") ? String(p_target["anchor"]) : String();
		if (!anchor.is_empty()) {
			value += String("#") + anchor;
		}
		return value;
	}
} // namespace

void RSTReferenceIndex::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_document_targets", "targets"), &RSTReferenceIndex::set_document_targets);
	ClassDB::bind_method(D_METHOD("get_document_targets"), &RSTReferenceIndex::get_document_targets);
	ClassDB::bind_method(D_METHOD("set_reference_targets", "targets"), &RSTReferenceIndex::set_reference_targets);
	ClassDB::bind_method(D_METHOD("get_reference_targets"), &RSTReferenceIndex::get_reference_targets);
	ClassDB::bind_method(D_METHOD("set_source_fingerprint", "fingerprint"), &RSTReferenceIndex::set_source_fingerprint);
	ClassDB::bind_method(D_METHOD("get_source_fingerprint"), &RSTReferenceIndex::get_source_fingerprint);
	ClassDB::bind_method(D_METHOD("resolve_document", "path"), &RSTReferenceIndex::resolve_document);
	ClassDB::bind_method(D_METHOD("resolve_reference", "source_path", "label"), &RSTReferenceIndex::resolve_reference);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "document_targets", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_document_targets", "get_document_targets");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "reference_targets", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_reference_targets", "get_reference_targets");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_fingerprint", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_source_fingerprint", "get_source_fingerprint");
}

void RSTReferenceIndex::set_document_targets(const Dictionary& p_targets) {
	document_targets = p_targets.duplicate(true);
	emit_changed();
}

Dictionary RSTReferenceIndex::get_document_targets() const {
	return document_targets;
}

void RSTReferenceIndex::set_reference_targets(const Dictionary& p_targets) {
	reference_targets = p_targets.duplicate(true);
	emit_changed();
}

Dictionary RSTReferenceIndex::get_reference_targets() const {
	return reference_targets;
}

void RSTReferenceIndex::set_source_fingerprint(const String& p_fingerprint) {
	source_fingerprint = p_fingerprint;
}

String RSTReferenceIndex::get_source_fingerprint() const {
	return source_fingerprint;
}

String RSTReferenceIndex::resolve_document(const String& p_path) const {
	String path = normalize_path(p_path);
	String fragment;
	const int64_t hash = path.find("#");
	if (hash >= 0) {
		fragment = path.substr(hash);
		path = path.substr(0, hash);
	}
	if (path.begins_with("uid://")) {
		const String uid_path = ResourceUID::uid_to_path(path);
		if (!uid_path.is_empty()) {
			path = uid_path;
		} else {
			const Array keys = document_targets.keys();
			for (int32_t i = 0; i < keys.size(); i++) {
				const Dictionary target = document_targets[keys[i]];
				if (target.has("uid") && String(target["uid"]) == path) {
					return resolve_target_value(target) + fragment;
				}
			}
		}
	}
	if (path.get_extension().is_empty() && !document_targets.has(path)) {
		path += String(".rst");
	}
	if (!document_targets.has(path)) {
		return normalize_path(p_path);
	}
	return resolve_target_value(document_targets[path]) + fragment;
}

String RSTReferenceIndex::resolve_reference(const String& p_source_path, const String& p_label) const {
	const String key = make_reference_key(p_source_path, p_label);
	if (!reference_targets.has(key)) {
		return String();
	}
	return resolve_target_value(reference_targets[key]);
}

String RSTReferenceIndex::get_generated_path() {
	return "res://dockit_generated/rst_reference_index.tres";
}

Ref<RSTReferenceIndex> RSTReferenceIndex::rebuild_from_project_settings(bool p_save_resource, bool p_emit_warnings) {
	ProjectSettings* settings = ProjectSettings::get_singleton();
	ResourceLoader* loader = ResourceLoader::get_singleton();
	if (settings == nullptr || loader == nullptr) {
		return Ref<RSTReferenceIndex>();
	}
	PackedStringArray roots;
	if (settings->has_setting("dockit/rst/document_roots")) {
		roots = settings->get_setting("dockit/rst/document_roots", PackedStringArray());
	}
	else {
		roots = settings->get_setting("rst_viewer/link_map_scan_roots", PackedStringArray());
	}

	std::vector<SourceDocument> documents;
	for (int32_t i = 0; i < roots.size(); i++) {
		const String root = normalize_path(roots[i]);
		if (root.is_empty() || !root.begins_with("res://")) {
			continue;
		}
		if (root.get_extension().to_lower() == "rst" && FileAccess::file_exists(root)) {
			Ref<FileAccess> file = FileAccess::open(root, FileAccess::READ);
			if (file.is_valid()) {
				SourceDocument document;
				document.path = root;
				document.root = root.get_base_dir();
				document.uid = get_uid_for_path(root);
				document.text = file->get_as_text();
				file->close();
				parse_document(document);
				documents.push_back(document);
			}
		}
		else {
			scan_directory(root, root, documents);
		}
	}
	std::sort(documents.begin(), documents.end(), [](const SourceDocument& p_left, const SourceDocument& p_right) {
		return p_left.path < p_right.path;
	});
	Dictionary document_targets;
	Dictionary reference_targets;

	Dictionary labels_by_root;
	for (int32_t source_index = 0; source_index < int32_t(documents.size()); source_index++) {
		const SourceDocument& source = documents[source_index];
		const Array labels = source.labels.keys();
		for (int32_t label_index = 0; label_index < labels.size(); label_index++) {
			const String label = labels[label_index];
			const String key = source.root + String("|") + label;
			Array candidates;
			if (labels_by_root.has(key)) {
				candidates = labels_by_root[key];
			}
			const Array anchors = source.labels[label];
			for (int32_t anchor_index = 0; anchor_index < anchors.size(); anchor_index++) {
				Dictionary candidate;
				candidate["path"] = source.path;
				candidate["uid"] = source.uid;
				candidate["anchor"] = anchors[anchor_index];
				bool duplicate_target = false;
				for (int32_t candidate_index = 0; candidate_index < candidates.size(); candidate_index++) {
					const Dictionary existing = candidates[candidate_index];
					if (String(existing["path"]) == source.path && String(existing["anchor"]) == String(candidate["anchor"])) {
						duplicate_target = true;
						break;
					}
				}
				if (!duplicate_target) {
					candidates.append(candidate);
				}
			}
			labels_by_root[key] = candidates;
		}
	}

	for (int32_t source_index = 0; source_index < int32_t(documents.size()); source_index++) {
		const SourceDocument& source = documents[source_index];
		for (int32_t i = 0; i < source.document_references.size(); i++) {
			const String target_path = resolve_document_path(source.path, source.document_references[i]);
			if (target_path.is_empty()) {
				continue;
			}
			const int32_t target_index = find_document(documents, target_path);
			if (target_index < 0) {
				continue;
			}
			const SourceDocument& target = documents[target_index];
			Dictionary entry = make_target(target);
			document_targets[target.path] = entry;
		}
		for (int32_t i = 0; i < source.referenced_labels.size(); i++) {
			const String label = normalize_label(source.referenced_labels[i]);
			const String key = source.root + String("|") + label;
			if (!labels_by_root.has(key)) {
				if (p_emit_warnings) {
					UtilityFunctions::push_warning("RST reference index: unresolved label '" + source.referenced_labels[i] + "' in " + source.path);
				}
				continue;
			}
			const Array candidates = labels_by_root[key];
			if (candidates.size() != 1) {
				if (p_emit_warnings) {
					UtilityFunctions::push_warning("RST reference index: ambiguous label '" + source.referenced_labels[i] + "' in " + source.path);
				}
				continue;
			}
			const Dictionary target = candidates[0];
			const String reference_key = make_reference_key(source.path, label);
			reference_targets[reference_key] = target;
			const String target_path = target["path"];
			document_targets[target_path] = target;
		}
	}

	uint64_t fingerprint_hash = UINT64_C(14695981039346656037);
	update_fingerprint(fingerprint_hash, String("rst-index-generator-v2"));
	PackedStringArray fingerprint_roots = roots;
	fingerprint_roots.sort();
	for (int32_t i = 0; i < fingerprint_roots.size(); i++) {
		update_fingerprint(fingerprint_hash, normalize_path(fingerprint_roots[i]));
	}
	Array document_keys = document_targets.keys();
	document_keys.sort();
	for (int32_t i = 0; i < document_keys.size(); i++) {
		const String key = document_keys[i];
		const Dictionary target = document_targets[key];
		update_fingerprint(fingerprint_hash, key);
		update_fingerprint(fingerprint_hash, target.has("path") ? String(target["path"]) : String());
		update_fingerprint(fingerprint_hash, target.has("uid") ? String(target["uid"]) : String());
		update_fingerprint(fingerprint_hash, target.has("anchor") ? String(target["anchor"]) : String());
	}
	Array reference_keys = reference_targets.keys();
	reference_keys.sort();
	for (int32_t i = 0; i < reference_keys.size(); i++) {
		const String key = reference_keys[i];
		const Dictionary target = reference_targets[key];
		update_fingerprint(fingerprint_hash, key);
		update_fingerprint(fingerprint_hash, target.has("path") ? String(target["path"]) : String());
		update_fingerprint(fingerprint_hash, target.has("uid") ? String(target["uid"]) : String());
		update_fingerprint(fingerprint_hash, target.has("anchor") ? String(target["anchor"]) : String());
	}
	fingerprint_hash &= UINT64_C(0x7FFFFFFFFFFFFFFF);
	const String source_fingerprint = String::num_uint64(static_cast<int64_t>(fingerprint_hash));

	const String output_path = get_generated_path();
	if (!p_save_resource) {
		Ref<RSTReferenceIndex> memory_index;
		memory_index.instantiate();
		memory_index->set_document_targets(document_targets);
		memory_index->set_reference_targets(reference_targets);
		memory_index->set_source_fingerprint(source_fingerprint);
		return memory_index;
	}
	const String output_directory = output_path.get_base_dir();
	if (!DirAccess::dir_exists_absolute(output_directory)) {
		DirAccess::make_dir_recursive_absolute(output_directory);
	}
	Ref<RSTReferenceIndex> index;
	if (FileAccess::file_exists(output_path)) {
		const Ref<Resource> stored = loader->load(output_path, "RSTReferenceIndex");
		index = stored;
		if (index.is_valid() && index->get_source_fingerprint() == source_fingerprint) {
			return index;
		}
	}
	if (index.is_null()) {
		index.instantiate();
	}
	if (index->get_source_fingerprint() == source_fingerprint) {
		return index;
	}
	index->set_document_targets(document_targets);
	index->set_reference_targets(reference_targets);
	index->set_source_fingerprint(source_fingerprint);
	const Error save_error = save_index_atomically(index, output_path);
	if (save_error != OK) {
		UtilityFunctions::push_error("Dockit could not save the generated RST reference index: " + output_path);
		return Ref<RSTReferenceIndex>();
	}
	return index;
}
