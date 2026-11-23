#pragma once

#include "formats/html/text/html_text_layout.h"

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <optional>
#include <functional>
#include <vector>

namespace litehtml { class html_tag; }

namespace dockit {
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

		struct Selection {
			int64_t anchor;
			int64_t caret;
			bool trailing_affinity;
		};

		struct Replacement {
			int64_t offset;
			godot::String removed;
			godot::String inserted;
		};

		struct Edit {
			Selection before;
			Selection after;
			std::vector<Replacement> replacements;
		};

		struct ImeEdit {
			uint64_t frame;
			Selection before;
			bool recorded = false;
		};

		const bool multiline;
		godot::String value;
		godot::String preedit;
		godot::Vector2i preedit_selection;
		std::vector<Edit> history;
		size_t history_position = 0;
		std::optional<ImeEdit> ime_edit;
		bool dirty_value = false;
		bool user_changed = false;
		bool last_edit_was_user = false;
		uint64_t reset_version = 0;
		int64_t anchor = 0;
		int64_t caret = 0;
		bool trailing_affinity = false;
		float horizontal_scroll = 0;
		HTMLTextLayout shaped;
		godot::Vector2 text_origin;
		godot::Rect2 caret_rect;
		bool showing_placeholder = false;
		bool caret_visible = true;

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
		int layout_overflow_x = 0;
		int layout_overflow_y = 0;
		float layout_scrollbar_width = 0;
		int layout_align = 0;
		bool layout_resize = false;
		uintptr_t layout_font = 0;

		Selection selection() const { return {anchor, caret, trailing_affinity}; }
		void clear_history();
		void restore_selection(const Selection& p_selection);
		bool replace_selection(const godot::String& p_text, const char* p_maxlength, const Selection& p_before, bool p_merge);
		void set_preedit(const godot::String& p_text, const godot::Vector2i& p_selection);
		godot::String normalize(const godot::String& p_value) const;
		float caret_x(int64_t p_offset, float p_near = 0.0f, bool p_nearest = false) const;
		float line_caret_x(const Line& p_line, int64_t p_offset, std::optional<float> p_near = {}) const;
		size_t line_at_offset(int64_t p_offset) const;
		size_t line_at_y(float p_y) const;
		void hit_single(float p_x, bool p_extend);
		void move_single(int p_direction, bool p_extend);
		void layout_lines(LiteHTMLContainer& p_container, const css::ComputedStyle& p_style,
			const css::ComputedStyle& p_text_style, const godot::String& p_text, float p_width, float p_line_height);
		godot::Rect2 scrollbar_thumb(const godot::Rect2& p_track, bool p_vertical) const;

	public:
		std::function<void()> value_changed;
		explicit HTMLTextEditor(bool p_multiline) : multiline(p_multiline) {}
		bool is_multiline() const { return multiline; }											// 检查编辑器是否允许输入多行文本
		const godot::String& get_value() const { return value; }								// 获取值
		void use_default(const godot::String& p_value);											// 在控件值未修改时应用默认值
		bool set_value(const godot::String& p_value);											// 设置值
		void reset_value(const godot::String& p_value);											// 重置值
		uint64_t get_reset_version() const { return reset_version; }							// 获取reset版本
		void copy_value_state(const HTMLTextEditor& p_source, bool p_preserve_selection);		// 从另一个编辑器复制值并按需保留选区
		int64_t selection_start() const;														// 获取当前选区的起始字符偏移
		int64_t selection_end() const;															// 获取当前选区的结束字符偏移
		int64_t caret_offset() const { return caret; }											// 获取插入光标的字符偏移
		bool set_selection(int64_t p_anchor, int64_t p_caret);									// 设置选区
		void hit_caret(const godot::Vector2& p_point, bool p_extend);							// 将插入光标移动到指定坐标对应的位置
		void move_caret(int p_direction, bool p_extend);										// 按指定方向移动插入光标
		void move_vertical(int p_direction, bool p_extend);										// 按指定方向移动插入光标所在行
		void move_to_edge(bool p_end, bool p_document, bool p_extend);							// 将插入光标移动到行首、行尾或文档边界
		bool erase(int p_direction);															// 按指定方向删除选中文本或相邻字符
		bool insert(const godot::String& p_text, const char* p_maxlength = "");					// 插入文本并遵守最大长度限制
		bool insert_character(
			char32_t p_character,
			const char* p_maxlength,
			uint64_t p_frame,
			bool p_ime_event
		);																						// 在当前插入光标处插入字符
		bool can_undo() const { return history_position > 0; }									// 检查是否可以撤销
		bool can_redo() const { return history_position < history.size(); }						// 检查是否可以重做
		bool undo();																			// 撤销最近一次文本编辑
		bool redo();																			// 重做最近撤销的文本编辑
		void finish_ime_edit();																	// 完成输入法编辑
		bool cancel_preedit();																	// 取消预编辑
		void update_preedit(
			const godot::String& p_text,
			const godot::Vector2i& p_selection,
			uint64_t p_frame
		);																						// 更新预编辑
		bool has_preedit() const { return !preedit.is_empty(); }								// 检查是否包含预编辑
		const godot::String& get_preedit() const { return preedit; }							// 获取预编辑
		bool take_user_change();																// 读取并清除待应用的用户输入变更
		bool has_length_violation(const char* p_minlength, const char* p_maxlength) const;		// 检查是否包含长度违规项
		bool has_dirty_value() const { return dirty_value; }									// 检查当前值是否偏离默认值
		godot::Rect2 get_caret_rect() const { return caret_rect; }								// 获取插入光标矩形
		bool is_caret_visible() const { return caret_visible; }									// 检查插入光标可见性
		bool set_caret_visible(bool p_visible) {
			if (caret_visible == p_visible) return false;
			caret_visible = p_visible;
			return true;
		}																						// 设置插入光标可见性
		godot::Rect2 get_viewport() const { return viewport; }
		godot::Vector2 scroll_by(const godot::Vector2& p_delta);
		std::optional<bool> scrollbar_at(const godot::Vector2& p_point) const;
		void drag_scrollbar(bool p_vertical, const godot::Vector2& p_point, float p_grab_offset);
		float scrollbar_grab_offset(bool p_vertical, const godot::Vector2& p_point) const;
		void paint_single(LiteHTMLContainer& p_container, const godot::Rect2& p_box,
			const litehtml::html_tag& p_host, const godot::String& p_placeholder, bool p_focused, bool p_password = false,
			const godot::String* p_preview = nullptr);
		void paint_multiline(LiteHTMLContainer& p_container, const godot::Rect2& p_box,
			const litehtml::html_tag& p_host, const godot::String& p_placeholder, bool p_focused, bool p_resize_handle);
	};
}
