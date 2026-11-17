#pragma once

#include "nodes/html_label.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

class HTMLBehavior : public godot::RefCounted {
	GDCLASS(HTMLBehavior, godot::RefCounted)

	friend class HTMLLabel;
	GDVIRTUAL1(_attach, HTMLLabel *);
	GDVIRTUAL0(_detach);

protected:
	static void _bind_methods();
};
