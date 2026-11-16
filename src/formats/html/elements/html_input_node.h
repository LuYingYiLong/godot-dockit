#pragma once

#include "html_text_control_node.h"
#include "html_node_copy_state.h"

#include <optional>
#include <string_view>

namespace docrender {
	class HTMLInputNode final : public HTMLTextControlNode, public HTMLNodeCopyState {
		bool checked = false;
		bool dirty_checked = false;
		godot::Rect2 range_box;

		const char* button_text() const;
		void paint_contents(LiteHTMLContainer& p_container, const godot::Rect2& p_box);

	public:
		enum class Type { TEXT, SEARCH, PASSWORD, EMAIL, URL, TEL, NUMBER, RANGE, CHECKBOX, RADIO, HIDDEN, BUTTON, SUBMIT, RESET, OTHER };

		explicit HTMLInputNode(const std::shared_ptr<litehtml::document>& p_document);
		void set_attr(const char* p_name, const char* p_value) override;
		std::shared_ptr<litehtml::render_item> create_render_item(const std::shared_ptr<litehtml::render_item>& p_parent) override;
		void draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y,
			const litehtml::position* p_clip, const std::shared_ptr<litehtml::render_item>& p_item) override;
		void attribute_changed(std::string_view p_name);
		Type type() const;
		godot::String type_name() const;
		bool is_text_control() const override;
		bool is_button() const;
		bool is_password() const { return type() == Type::PASSWORD; }
		bool accepts_suggestions() const;
		bool is_suggestion_valid(const godot::String& p_value) const;
		bool has_type_violation() const;
		double value_as_number() const;
		godot::String sanitize_value(const godot::String& p_value) const;
		bool step_number(int p_step, std::optional<bool> p_endpoint = {});
		bool set_range_point(const godot::Vector2& p_point);
		bool set_numeric_from_user(double p_value);
		bool update_pseudos() override;
		godot::String get_value() const;
		bool is_checked() const { return checked; }
		bool set_checked(bool p_checked);
		void clear_group_checked();
		void reset();
		bool is_toggle() const { const auto kind = type(); return kind == Type::CHECKBOX || kind == Type::RADIO; }
		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override;
	};
}
