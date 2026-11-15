#include "html_input_node.h"

#include "formats/html/backend/litehtml_container.h"

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
		if (name == "checkbox") return Type::CHECKBOX;
		if (name == "radio") return Type::RADIO;
		if (name == "hidden") return Type::HIDDEN;
		return Type::OTHER;
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
		const bool disabled = supported && forms::is_disabled(*this);
		bool changed = set_pseudo_class(litehtml::_id("disabled"), disabled);
		changed |= set_pseudo_class(litehtml::_id("enabled"), supported && !disabled);
		changed |= set_pseudo_class(litehtml::_id("checked"), is_toggle() && checked);
		changed |= set_pseudo_class(litehtml::_id("placeholder-shown"),
			kind == Type::TEXT && editor.get_value().is_empty() && get_attr("placeholder"));
		return changed;
	}

	String HTMLInputNode::get_value() const {
		if (is_toggle()) return String::utf8(get_attr("value", "on"));
		if (type() == Type::HIDDEN) return String::utf8(get_attr("value", ""));
		return editor.get_value();
	}

	bool HTMLInputNode::set_checked(bool p_checked) {
		dirty_checked = true;
		bool changed = checked != p_checked;
		checked = p_checked;
		if (p_checked && type() == Type::RADIO) {
			for (const auto& peer : forms::radio_group(*this)) {
				auto* input = static_cast<HTMLInputNode*>(peer.get());
				if (input != this && input->checked) {
					input->clear_group_checked();
					changed = true;
				}
			}
		}
		return changed;
	}

	void HTMLInputNode::clear_group_checked() {
		checked = false;
		update_pseudos();
	}

	void HTMLInputNode::reset() {
		editor.reset_value(String::utf8(get_attr("value", "")));
		dirty_checked = false;
		checked = get_attr("checked") != nullptr;
	}

	bool HTMLInputNode::copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const {
		auto* target = dynamic_cast<HTMLInputNode*>(&p_target);
		if (!target) {
			return false;
		}
		target->editor.copy_value_state(editor, p_mode == HTMLNodeCopyMode::ADOPT);
		target->checked = checked;
		target->dirty_checked = dirty_checked;
		target->update_pseudos();
		return true;
	}

	void HTMLInputNode::draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y,
		const litehtml::position* p_clip, const std::shared_ptr<litehtml::render_item>& p_item) {
		html_tag::draw(p_hdc, p_x, p_y, p_clip, p_item);
		const auto pos = p_item->pos();
		auto* container = static_cast<LiteHTMLContainer*>(get_document()->container());
		paint_contents(*container, Rect2(pos.x + p_x, pos.y + p_y, pos.width, pos.height));
	}

	std::shared_ptr<litehtml::render_item> HTMLInputNode::create_render_item(const std::shared_ptr<litehtml::render_item>& p_parent) {
		if (type() == Type::HIDDEN) return {};
		return html_tag::create_render_item(p_parent);
	}

	void HTMLInputNode::paint_contents(LiteHTMLContainer& p_container, const Rect2& p_box) {
		const auto kind = type();
		if (kind == Type::OTHER || kind == Type::HIDDEN || !p_box.has_area()) return;
		const auto color = css().get_color();
		if (is_toggle()) {
			if (checked && css().get_appearance() != litehtml::appearance_none) {
				auto mark = p_container.shape_input(css().get_font(), String::chr(kind == Type::RADIO ? 0x25cf : 0x2713));
				if (mark.is_valid()) {
					const Vector2 origin(p_box.get_center().x - mark.width() * 0.5f,
						p_box.get_center().y + (mark.line_ascent() - mark.line_descent()) * 0.5f);
					auto ink = color;
					if (!has_author_property(litehtml::_color_) && !has_author_property(litehtml::_background_color_) &&
						!has_pseudo_class(litehtml::_id("disabled"))) {
						const auto accent = css().get_accent_color().to_srgb(color);
						const float lightness = (accent.red * 0.2126f + accent.green * 0.7152f + accent.blue * 0.0722f) / 255;
						ink = lightness > 0.55f ? litehtml::web_color::black : litehtml::web_color::white;
					}
					p_container.draw_input(p_box, mark, origin, ink, {}, {}, {});
				}
			}
			return;
		}

		editor.paint_single(p_container, p_box, *this, String::utf8(get_attr("placeholder", "")), focused);
	}
}
