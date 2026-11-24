#pragma once

#include "css_syntax_highlighter.h"

#include <godot_cpp/classes/editor_syntax_highlighter.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace godot {
	class HTMLSyntaxHighlighter : public EditorSyntaxHighlighter {
		GDCLASS(HTMLSyntaxHighlighter, EditorSyntaxHighlighter)

	public:
		struct HTMLLexState {
			bool in_comment = false;
			bool in_tag = false;
			bool expect_tag_name = false;
			bool expect_value = false;
			bool closing_tag = false;
			bool self_closing_tag = false;
			bool current_tag_is_style = false;
			bool in_style = false;
			char32_t quote = 0;
			css_highlight::CSSLexState css_state;
		};

	private:
		Color tag_color;
		Color attribute_color;
		Color string_color;
		Color comment_color;
		Color symbol_color;
		css_highlight::CSSHighlightColors css_colors;
		mutable std::vector<HTMLLexState> line_state_cache;
		mutable std::vector<uint8_t> line_state_cache_valid;
		mutable uint32_t cached_text_version = std::numeric_limits<uint32_t>::max();

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
