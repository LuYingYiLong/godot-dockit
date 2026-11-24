#pragma once

#include <godot_cpp/classes/editor_syntax_highlighter.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace godot {
	namespace css_highlight {
		struct CSSLexState {
			bool in_comment = false;
			char32_t quote = 0;
			bool escaped = false;
			bool pending_at_rule = false;
			bool in_at_rule_prelude = false;
			bool expect_property = false;
			bool expect_value = false;
			bool in_value = false;
			int32_t rule_depth = 0;
			int32_t at_rule_depth = 0;
		};

		struct CSSHighlightColors {
			Color at_rule;
			Color selector;
			Color property;
			Color value;
			Color string;
			Color comment;
			Color symbol;
			Color number;
			Color function;
		};

		CSSHighlightColors get_css_highlight_colors();
		void scan_css_line(
			const String& p_line,
			CSSLexState& r_state,
			const CSSHighlightColors& p_colors,
			Dictionary* r_color_map,
			int32_t p_start = 0,
			int32_t p_end = -1);
		int32_t find_style_end_tag(const String& p_line, int32_t p_start);
	}

	class CSSSyntaxHighlighter : public EditorSyntaxHighlighter {
		GDCLASS(CSSSyntaxHighlighter, EditorSyntaxHighlighter)

		css_highlight::CSSHighlightColors colors;
		mutable std::vector<css_highlight::CSSLexState> line_state_cache;
		mutable std::vector<uint8_t> line_state_cache_valid;
		mutable uint32_t cached_text_version = std::numeric_limits<uint32_t>::max();

	protected:
		static void _bind_methods() {}

	public:
		CSSSyntaxHighlighter();

		String _get_name() const override;
		PackedStringArray _get_supported_languages() const override;
		Ref<EditorSyntaxHighlighter> _create() const override;
		Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;
		void _clear_highlighting_cache() override;
		void _update_cache() override;
	};
} // namespace godot
