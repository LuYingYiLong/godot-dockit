#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

class RSTReferenceIndex;

using namespace godot;

class RSTDocument : public Resource {
	GDCLASS(RSTDocument, Resource);

private:
	String text;
	String source_path;
	Ref<RSTReferenceIndex> reference_index;

protected:
	static void _bind_methods();

public:
	void set_text(const String& p_text);
	String get_text() const;
	void set_source_path(const String& p_source_path);
	String get_source_path() const;
	void set_reference_index(const Ref<RSTReferenceIndex>& p_reference_index);
	Ref<RSTReferenceIndex> get_reference_index() const;

	String resolve_path(const String& p_path) const;
};
