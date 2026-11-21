#pragma once

#include <godot_cpp/classes/resource.hpp>

class CSSStyleSheet : public godot::Resource {
	GDCLASS(CSSStyleSheet, godot::Resource)

	godot::String css_text;
	godot::String source_path;

protected:
	static void _bind_methods();

public:
	void set_css_text(const godot::String& p_text);
	godot::String get_css_text() const;
	void set_source_path(const godot::String& p_path);
	godot::String get_source_path() const;
};
