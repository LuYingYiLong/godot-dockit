#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

class RSTReferenceIndex : public godot::Resource {
	GDCLASS(RSTReferenceIndex, godot::Resource)

	godot::Dictionary document_targets;
	godot::Dictionary reference_targets;
	godot::String source_fingerprint;

protected:
	static void _bind_methods();

public:
	void set_document_targets(const godot::Dictionary& p_targets);
	godot::Dictionary get_document_targets() const;
	void set_reference_targets(const godot::Dictionary& p_targets);
	godot::Dictionary get_reference_targets() const;
	void set_source_fingerprint(const godot::String& p_fingerprint);
	godot::String get_source_fingerprint() const;

	godot::String resolve_document(const godot::String& p_path) const;
	godot::String resolve_reference(const godot::String& p_source_path, const godot::String& p_label) const;

	static godot::String get_generated_path();
	static godot::Ref<RSTReferenceIndex> rebuild_from_project_settings(bool p_save_resource = true, bool p_emit_warnings = true);
};
