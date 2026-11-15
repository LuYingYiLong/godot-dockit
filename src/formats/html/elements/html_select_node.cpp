#include "html_select_node.h"

#include "html_form_state.h"
#include "formats/html/backend/litehtml_container.h"

#include <litehtml/document.h>
#include <litehtml/render_item.h>

#include <algorithm>
#include <utility>

using namespace godot;

namespace docrender {
namespace {
	String option_text(const litehtml::element& p_option) {
		litehtml::string text;
		const auto append_text = [&](const auto& p_visit, const litehtml::element& p_node) -> void {
			if (litehtml::is_generated_tag(p_node.tag()) || p_node.tag() == litehtml::_script_) return;
			if (p_node.is_text()) p_node.get_text(text);
			for (const auto& child : p_node.children()) p_visit(p_visit, *child);
		};
		append_text(append_text, p_option);
		std::string collapsed;
		bool space = false;
		for (const char ch : text) {
			if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f') {
				space = !collapsed.empty();
			} else {
				if (space) collapsed += ' ';
				space = false;
				collapsed += ch;
			}
		}
		return String::utf8(collapsed.c_str());
	}
}

HTMLSelectNode* HTMLOptionNode::select() const {
	auto node = parent();
	if (node && node->tag() == litehtml::_optgroup_) node = node->parent();
	return dynamic_cast<HTMLSelectNode*>(node.get());
}

String HTMLOptionNode::get_value() const {
	if (const char* value = get_attr("value")) return String::utf8(value);
	return option_text(*this);
}

String HTMLOptionNode::get_label() const {
	if (const char* label = get_attr("label"); label && *label) return String::utf8(label);
	return option_text(*this);
}

bool HTMLOptionNode::is_selected() const {
	if (const auto* owner = select()) {
		const auto options = owner->get_options();
		const auto index = owner->selected_index(options);
		return index >= 0 && options[index].get() == this;
	}
	return selected.value_or(get_attr("selected") != nullptr);
}

void HTMLOptionNode::assign_selected(bool p_selected, bool p_dirty) {
	selected = p_selected;
	dirty |= p_dirty;
}

void HTMLOptionNode::set_selected(bool p_selected) {
	if (auto* owner = select()) {
		const auto options = owner->get_options();
		if (p_selected) {
			const auto item = std::find_if(options.begin(), options.end(), [this](const auto& p_option) { return p_option.get() == this; });
			owner->set_selected_index(options, std::distance(options.begin(), item));
			return;
		}
	}
	assign_selected(p_selected, true);
}

void HTMLOptionNode::attribute_changed() {
	if (!dirty) {
		selected.reset();
		if (get_attr("selected")) {
			if (auto* owner = select()) {
				for (const auto& option : owner->get_options()) {
					if (option.get() != this) option->assign_selected(false, false);
				}
			}
		}
	}
}

bool HTMLOptionNode::copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode) const {
	auto* target = dynamic_cast<HTMLOptionNode*>(&p_target);
	if (!target) return false;
	target->selected = is_selected();
	target->dirty = dirty;
	return true;
}

HTMLSelectNode::Options HTMLSelectNode::get_options() const {
	Options options;
	const auto add = [&](const litehtml::element::ptr& p_element) {
		if (auto option = std::dynamic_pointer_cast<HTMLOptionNode>(p_element)) options.push_back(std::move(option));
	};
	for (const auto& child : m_children) {
		if (child->tag() == litehtml::_optgroup_) {
			for (const auto& option : child->children()) add(option);
		} else add(child);
	}
	return options;
}

int64_t HTMLSelectNode::selected_index(const Options& p_options) const {
	int64_t fallback = -1;
	int64_t chosen = -1;
	for (size_t i = 0; i < p_options.size(); ++i) {
		const auto& option = *p_options[i];
		if (option.selected.value_or(option.get_attr("selected") != nullptr)) chosen = i;
		if (fallback < 0 && !forms::is_disabled(option)) fallback = i;
	}
	return chosen >= 0 || allow_empty ? chosen : fallback;
}

void HTMLSelectNode::set_selected_index(const Options& p_options, int64_t p_index) {
	allow_empty = p_index < 0 || p_index >= static_cast<int64_t>(p_options.size());
	for (size_t i = 0; i < p_options.size(); ++i) {
		const bool select = p_index == static_cast<int64_t>(i);
		p_options[i]->assign_selected(select, select);
	}
}

bool HTMLSelectNode::is_missing() const {
	const auto options = get_options();
	const auto index = selected_index(options);
	return index < 0 || (index == 0 && options.front()->parent().get() == this && options.front()->get_value().is_empty());
}

void HTMLSelectNode::reset() {
	allow_empty = false;
	for (const auto& option : get_options()) {
		option->selected.reset();
		option->dirty = false;
	}
}

bool HTMLSelectNode::copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode) const {
	auto* target = dynamic_cast<HTMLSelectNode*>(&p_target);
	if (!target) return false;
	target->allow_empty = allow_empty;
	return true;
}

std::shared_ptr<litehtml::render_item> HTMLSelectNode::create_render_item(const std::shared_ptr<litehtml::render_item>& p_parent) {
	struct RestoreChildren {
		litehtml::elements_list& target;
		litehtml::elements_list children;
		~RestoreChildren() { target.swap(children); }
	} restore{m_children, std::exchange(m_children, {})};
	return html_tag::create_render_item(p_parent);
}

void HTMLSelectNode::draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y,
	const litehtml::position* p_clip, const std::shared_ptr<litehtml::render_item>& p_item) {
	html_tag::draw(p_hdc, p_x, p_y, p_clip, p_item);
	const auto pos = p_item->pos();
	const Rect2 box(pos.x + p_x, pos.y + p_y, pos.width, pos.height);
	if (!box.has_area()) return;
	auto* container = static_cast<LiteHTMLContainer*>(get_document()->container());
	const auto options = get_options();
	const auto index = selected_index(options);
	const auto paint = [&](const String& p_text, const Rect2& p_clip_box) {
		auto line = container->shape_input(css().get_font(), p_text);
		if (!line.is_valid() || !p_clip_box.has_area()) return;
		const Vector2 baseline(p_clip_box.position.x, box.get_center().y + (line.line_ascent() - line.line_descent()) * 0.5);
		container->draw_input(p_clip_box, line, baseline, css().get_color(), {}, {}, {});
	};
	const bool arrow = css().get_appearance() != litehtml::appearance_none;
	if (index >= 0) paint(options[index]->get_label(), Rect2(box.position, Vector2(box.size.x - (arrow ? 16 : 0), box.size.y)));
	if (arrow) paint(String::chr(0x25be), Rect2(box.get_end().x - 14, box.position.y, 14, box.size.y));
}
}
