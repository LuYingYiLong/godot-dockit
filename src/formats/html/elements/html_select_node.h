#pragma once

#include "html_node_copy_state.h"

#include <godot_cpp/variant/string.hpp>
#include <litehtml/html_tag.h>

#include <optional>
#include <vector>

namespace docrender {
	class HTMLSelectNode;
	class HTMLOptionNode : public litehtml::html_tag, public HTMLNodeCopyState {
		friend class HTMLSelectNode;
		std::optional<bool> selected;
		bool dirty = false;
		void assign_selected(bool p_selected, bool p_dirty);

	public:
		using litehtml::html_tag::html_tag;
		HTMLSelectNode* select() const;
		godot::String get_value() const;
		godot::String get_label() const;
		bool is_selected() const;
		void set_selected(bool p_selected);
		void attribute_changed();
		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override;
	};

	class HTMLSelectNode : public litehtml::html_tag, public HTMLNodeCopyState {
		bool allow_empty = false;

	public:
		using litehtml::html_tag::html_tag;
		using Options = std::vector<std::shared_ptr<HTMLOptionNode>>;
		Options get_options() const;
		int64_t selected_index(const Options& p_options) const;
		void set_selected_index(const Options& p_options, int64_t p_index);
		bool is_missing() const;
		void reset();
		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override;
		std::shared_ptr<litehtml::render_item> create_render_item(const std::shared_ptr<litehtml::render_item>& p_parent) override;
		void draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y,
			const litehtml::position* p_clip, const std::shared_ptr<litehtml::render_item>& p_item) override;
	};
}
