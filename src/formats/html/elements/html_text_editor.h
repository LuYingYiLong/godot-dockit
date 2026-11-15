#pragma once

#include "formats/html/text/html_text_layout.h"

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <optional>
#include <vector>

namespace litehtml { class html_tag; }

namespace docrender {
	class LiteHTMLContainer;
	namespace css { class ComputedStyle; }

	class HTMLTextEditor {
		struct Line {
			int64_t start;
			int64_t end;
			HTMLTextLayout shaped;
			godot::Vector2 baseline;
			float bottom;
		};

		const bool multiline;
		godot::String value;
		godot::String preedit;
		godot::Vector2i preedit_selection;
		bool dirty_value = false;
		bool user_changed = false;
		uint64_t reset_version = 0;
		int64_t anchor = 0;
		int64_t caret = 0;
		bool trailing_affinity = false;
		float horizontal_scroll = 0;
		HTMLTextLayout shaped;
		godot::Vector2 text_origin;
		godot::Rect2 caret_rect;
		bool showing_placeholder = false;

		std::vector<Line> lines;
		godot::String laid_out_text;
		godot::Rect2 viewport;
		godot::Vector2 scroll;
		godot::Vector2 full_size;
		godot::Vector2 scroll_limit;
		godot::Rect2 vertical_track;
		godot::Rect2 horizontal_track;
		bool vertical_scrollbar = false;
		bool horizontal_scrollbar = false;
		std::optional<float> preferred_x;
		std::optional<size_t> caret_line_hint;
		bool reveal_caret = true;
		godot::Vector2 layout_size = {-1, -1};
		float layout_line_height = -1;
		int layout_wrap = 0;
		int layout_overflow = 0;
		int layout_align = 0;
		bool layout_resize = false;
		uintptr_t layout_font = 0;

		godot::String normalize(const godot::String& p_value) const;
		float caret_x(int64_t p_offset, float p_near = 0.0f, bool p_nearest = false) const;
		float line_caret_x(const Line& p_line, int64_t p_offset, std::optional<float> p_near = {}) const;
		size_t line_at_offset(int64_t p_offset) const;
		size_t line_at_y(float p_y) const;
		void hit_single(float p_x, bool p_extend);
		void move_single(int p_direction, bool p_extend);
		void layout_lines(LiteHTMLContainer& p_container, const css::ComputedStyle& p_style,
			const godot::String& p_text, uintptr_t p_font, float p_width, float p_line_height);
		godot::Rect2 scrollbar_thumb(const godot::Rect2& p_track, bool p_vertical) const;

	public:
		explicit HTMLTextEditor(bool p_multiline) : multiline(p_multiline) {}
		bool is_multiline() const { return multiline; }
		const godot::String& get_value() const { return value; }
		void use_default(const godot::String& p_value);
		bool set_value(const godot::String& p_value);
		void reset_value(const godot::String& p_value);
		uint64_t get_reset_version() const { return reset_version; }
		void copy_value_state(const HTMLTextEditor& p_source, bool p_preserve_selection);
		int64_t selection_start() const;
		int64_t selection_end() const;
		int64_t caret_offset() const { return caret; }
		bool set_selection(int64_t p_anchor, int64_t p_caret);
		void hit_caret(const godot::Vector2& p_point, bool p_extend);
		void move_caret(int p_direction, bool p_extend);
		void move_vertical(int p_direction, bool p_extend);
		void move_to_edge(bool p_end, bool p_document, bool p_extend);
		bool erase(int p_direction);
		bool insert(const godot::String& p_text, const char* p_maxlength = "");
		void set_preedit(const godot::String& p_text, const godot::Vector2i& p_selection);
		bool has_preedit() const { return !preedit.is_empty(); }
		bool take_user_change();
		godot::Rect2 get_caret_rect() const { return caret_rect; }
		godot::Rect2 get_viewport() const { return viewport; }
		godot::Vector2 scroll_by(const godot::Vector2& p_delta);
		std::optional<bool> scrollbar_at(const godot::Vector2& p_point) const;
		void drag_scrollbar(bool p_vertical, const godot::Vector2& p_point, float p_grab_offset);
		float scrollbar_grab_offset(bool p_vertical, const godot::Vector2& p_point) const;
		void paint_single(LiteHTMLContainer& p_container, const godot::Rect2& p_box,
			const litehtml::html_tag& p_host, const godot::String& p_placeholder, bool p_focused);
		void paint_multiline(LiteHTMLContainer& p_container, const godot::Rect2& p_box,
			const litehtml::html_tag& p_host, const godot::String& p_placeholder, bool p_focused, bool p_resize_handle);
	};
}
