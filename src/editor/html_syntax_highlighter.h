#pragma once

#include <godot_cpp/classes/editor_syntax_highlighter.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {
	class HTMLSyntaxHighlighter : public EditorSyntaxHighlighter {
		GDCLASS(HTMLSyntaxHighlighter, EditorSyntaxHighlighter)

		Color tag_color;
		Color attribute_color;
		Color string_color;
		Color comment_color;
		Color symbol_color;

	protected:
		static void _bind_methods() {}

	public:
		HTMLSyntaxHighlighter();

		String _get_name() const override;
		PackedStringArray _get_supported_languages() const override;
		Ref<EditorSyntaxHighlighter> _create() const override;
		Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;
		void _clear_highlighting_cache() override;
		void _update_cache() override;
	};
} // namespace godot
