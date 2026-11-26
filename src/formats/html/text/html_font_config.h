#pragma once

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace dockit {
	struct HTMLFontConfig {
		godot::Ref<godot::Font> text;
		godot::Ref<godot::Font> bold;
		godot::Ref<godot::Font> italic;
		godot::Ref<godot::Font> bold_italic;
		godot::Ref<godot::Font> code;
		godot::Ref<godot::Font> code_block;
	godot::HashMap<godot::String, godot::Ref<godot::Font>> named_fonts;
	};
}
