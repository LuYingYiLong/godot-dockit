#include "rst_index_editor_plugin.h"

#include "formats/rst/rst_reference_index.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace {
	static void collect_rst_paths(const String& p_path, PackedStringArray& r_paths) {
		if (p_path.get_extension().to_lower() == "rst" && FileAccess::file_exists(p_path)) {
			if (!r_paths.has(p_path)) {
				r_paths.append(p_path);
			}
			return;
		}
		Ref<DirAccess> directory = DirAccess::open(p_path);
		if (directory.is_null() || directory->list_dir_begin() != OK) {
			return;
		}
		while (true) {
			const String name = directory->get_next();
			if (name.is_empty()) {
				break;
			}
			if (name == "." || name == ".." || name.begins_with(".")) {
				continue;
			}
			const String child_path = p_path.path_join(name).simplify_path();
			if (directory->current_is_dir()) {
				collect_rst_paths(child_path, r_paths);
			}
			else if (name.get_extension().to_lower() == "rst" && !r_paths.has(child_path)) {
				r_paths.append(child_path);
			}
		}
		directory->list_dir_end();
	}
}

void RSTIndexEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("on_filesystem_changed"), &RSTIndexEditorPlugin::on_filesystem_changed);
	ClassDB::bind_method(D_METHOD("on_project_settings_changed"), &RSTIndexEditorPlugin::on_project_settings_changed);
	ClassDB::bind_method(D_METHOD("refresh_index"), &RSTIndexEditorPlugin::refresh_index);
}

void RSTIndexEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		migrate_project_settings();
		if (importer.is_null()) {
			importer.instantiate();
			add_import_plugin(importer);
		}
		EditorInterface* editor = EditorInterface::get_singleton();
		resource_filesystem = editor == nullptr ? nullptr : editor->get_resource_filesystem();
		if (resource_filesystem != nullptr) {
			const Callable callback(this, "on_filesystem_changed");
			if (!resource_filesystem->is_connected("filesystem_changed", callback)) {
				resource_filesystem->connect("filesystem_changed", callback);
			}
		}
		ProjectSettings* settings = ProjectSettings::get_singleton();
		if (settings != nullptr) {
			const Callable callback(this, "on_project_settings_changed");
			if (!settings->is_connected("settings_changed", callback)) {
				settings->connect("settings_changed", callback);
			}
		}
		if (resource_filesystem == nullptr || !resource_filesystem->is_scanning()) {
			refresh_index();
		}
	}
	else if (p_what == NOTIFICATION_EXIT_TREE) {
		if (resource_filesystem != nullptr) {
			const Callable callback(this, "on_filesystem_changed");
			if (resource_filesystem->is_connected("filesystem_changed", callback)) {
				resource_filesystem->disconnect("filesystem_changed", callback);
			}
		}
		ProjectSettings* settings = ProjectSettings::get_singleton();
		if (settings != nullptr) {
			const Callable callback(this, "on_project_settings_changed");
			if (settings->is_connected("settings_changed", callback)) {
				settings->disconnect("settings_changed", callback);
			}
		}
		resource_filesystem = nullptr;
		if (importer.is_valid()) {
			remove_import_plugin(importer);
			importer.unref();
		}
	}
}

void RSTIndexEditorPlugin::migrate_project_settings() {
	ProjectSettings* settings = ProjectSettings::get_singleton();
	if (settings == nullptr) {
		return;
	}
	const String setting_name = "dockit/rst/document_roots";
	if (!settings->has_setting(setting_name)) {
		const PackedStringArray previous_roots = settings->get_setting("rst_viewer/link_map_scan_roots", PackedStringArray());
		settings->set_setting(setting_name, previous_roots);
		settings->set_initial_value(setting_name, previous_roots);
		settings->set_as_basic(setting_name, true);

		Dictionary property;
		property["name"] = setting_name;
		property["type"] = Variant::PACKED_STRING_ARRAY;
		settings->add_property_info(property);
		if (!previous_roots.is_empty()) {
			UtilityFunctions::print("Dockit migrated RST document roots from rst_viewer/link_map_scan_roots.");
			settings->save();
		}
	}
}

void RSTIndexEditorPlugin::queue_index_refresh() {
	if (refresh_queued || !is_inside_tree()) {
		return;
	}
	refresh_queued = true;
	call_deferred("refresh_index");
}

void RSTIndexEditorPlugin::refresh_index() {
	refresh_queued = false;
	if (resource_filesystem != nullptr && resource_filesystem->is_scanning()) {
		return;
	}
	String previous_fingerprint;
	const String generated_path = RSTReferenceIndex::get_generated_path();
	if (FileAccess::file_exists(generated_path)) {
		ResourceLoader* loader = ResourceLoader::get_singleton();
		if (loader != nullptr) {
			Ref<Resource> previous_resource = loader->load(generated_path, "RSTReferenceIndex");
			Ref<RSTReferenceIndex> previous_index = previous_resource;
			if (previous_index.is_valid()) {
				previous_fingerprint = previous_index->get_source_fingerprint();
			}
		}
	}
	const Ref<RSTReferenceIndex> index = RSTReferenceIndex::rebuild_from_project_settings();
	if (!index.is_valid() || resource_filesystem == nullptr || indexed_fingerprint == index->get_source_fingerprint()) {
		return;
	}
	indexed_fingerprint = index->get_source_fingerprint();
	resource_filesystem->update_file(generated_path);
	if (previous_fingerprint != indexed_fingerprint) {
		ProjectSettings* settings = ProjectSettings::get_singleton();
		PackedStringArray roots;
		if (settings != nullptr) {
			roots = settings->get_setting("dockit/rst/document_roots", PackedStringArray());
		}
		PackedStringArray documents;
		for (int32_t i = 0; i < roots.size(); i++) {
			collect_rst_paths(roots[i], documents);
		}
		if (!documents.is_empty()) {
			resource_filesystem->reimport_files(documents);
		}
	}
}

void RSTIndexEditorPlugin::on_filesystem_changed() {
	queue_index_refresh();
}

void RSTIndexEditorPlugin::on_project_settings_changed() {
	queue_index_refresh();
}
