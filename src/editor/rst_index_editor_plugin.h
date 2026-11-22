#pragma once

#include "editor/importers/rst_document_importer.h"

#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

class RSTIndexEditorPlugin : public godot::EditorPlugin {
	GDCLASS(RSTIndexEditorPlugin, godot::EditorPlugin)

	godot::Ref<RSTDocumentImporter> importer;
	godot::EditorFileSystem* resource_filesystem = nullptr;
	bool refresh_queued = false;
	godot::String indexed_fingerprint;

	void migrate_project_settings();
	void queue_index_refresh();
	void refresh_index();
	void on_filesystem_changed();
	void on_project_settings_changed();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
};
