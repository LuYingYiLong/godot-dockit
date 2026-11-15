#pragma once

#include "html_text_control_node.h"

#include <string_view>

namespace docrender {
	class HTMLInputNode final : public HTMLTextControlNode {
		bool checked = false;
		bool dirty_checked = false;

		void paint_contents(LiteHTMLContainer& p_container, const godot::Rect2& p_box);

	public:
		enum class Type { TEXT, CHECKBOX, OTHER };

		explicit HTMLInputNode(const std::shared_ptr<litehtml::document>& p_document);
		void set_attr(const char* p_name, const char* p_value) override;
		void draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y,
			const litehtml::position* p_clip, const std::shared_ptr<litehtml::render_item>& p_item) override;
		void attribute_changed(std::string_view p_name);
		Type type() const;
		bool is_text_control() const override { return type() == Type::TEXT; }
		bool update_pseudos() override;
		godot::String get_value() const;
		bool is_checked() const { return checked; }
		bool set_checked(bool p_checked);
		void copy_state_for_clone(const HTMLInputNode& p_source);
	};
}
