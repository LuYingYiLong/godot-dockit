#include "html_input_node.h"

#include "formats/html/litehtml_container.h"

#include <litehtml/document.h>
#include <litehtml/render_item.h>

using namespace godot;

namespace docrender {
	HTMLInputNode::HTMLInputNode(const std::shared_ptr<litehtml::document>& p_document) : HTMLTextControlNode(p_document, false) {
		update_pseudos();
	}

	HTMLInputNode::Type HTMLInputNode::type() const {
		const String name = String::utf8(get_attr("type", "")).to_lower();
		if (name.is_empty() || name == "text") return Type::TEXT;
		return name == "checkbox" ? Type::CHECKBOX : Type::OTHER;
	}

	void HTMLInputNode::set_attr(const char* p_name, const char* p_value) {
		html_tag::set_attr(p_name, p_value);
		attribute_changed(p_name);
	}

	void HTMLInputNode::attribute_changed(std::string_view p_name) {
		if (p_name == "value" || p_name == "type") {
			editor.use_default(String::utf8(get_attr("value", "")));
		}
		if ((p_name == "checked" || p_name == "type") && !dirty_checked) checked = get_attr("checked");
		update_pseudos();
	}

	bool HTMLInputNode::update_pseudos() {
		const Type kind = type();
		const bool supported = kind != Type::OTHER;
		const bool disabled = supported && get_attr("disabled");
		bool changed = set_pseudo_class(litehtml::_id("disabled"), disabled);
		changed |= set_pseudo_class(litehtml::_id("enabled"), supported && !disabled);
		changed |= set_pseudo_class(litehtml::_id("checked"), kind == Type::CHECKBOX && checked);
		changed |= set_pseudo_class(litehtml::_id("placeholder-shown"),
			kind == Type::TEXT && editor.get_value().is_empty() && get_attr("placeholder"));
		return changed;
	}

	String HTMLInputNode::get_value() const {
		return type() == Type::CHECKBOX ? String::utf8(get_attr("value", "on")) : editor.get_value();
	}

	bool HTMLInputNode::set_checked(bool p_checked) {
		dirty_checked = true;
		if (checked == p_checked) return false;
		checked = p_checked;
		return true;
	}

	void HTMLInputNode::draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y,
		const litehtml::position* p_clip, const std::shared_ptr<litehtml::render_item>& p_item) {
		html_tag::draw(p_hdc, p_x, p_y, p_clip, p_item);
		const auto pos = p_item->pos();
		auto* container = static_cast<LiteHTMLContainer*>(get_document()->container());
		paint_contents(*container, Rect2(pos.x + p_x, pos.y + p_y, pos.width, pos.height));
	}

	void HTMLInputNode::paint_contents(LiteHTMLContainer& p_container, const Rect2& p_box) {
		const auto kind = type();
		if (kind == Type::OTHER || !p_box.has_area()) return;
		const auto color = css().get_color();
		if (kind == Type::CHECKBOX) {
			if (checked && css().get_appearance() != litehtml::appearance_none) {
				auto mark = p_container.shape_input(css().get_font(), String::chr(0x2713));
				if (mark.is_valid()) {
					const Vector2 origin(p_box.get_center().x - mark.width() * 0.5f,
						p_box.get_center().y + (mark.line_ascent() - mark.line_descent()) * 0.5f);
					p_container.draw_input(p_box, mark, origin, color, {}, {}, {});
				}
			}
			return;
		}

		editor.paint_single(p_container, p_box, css(), String::utf8(get_attr("placeholder", "")), focused);
	}
}
