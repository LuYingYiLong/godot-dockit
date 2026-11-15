#include "html_form_state.h"

#include "html_input_node.h"
#include "html_select_node.h"
#include "html_textarea_node.h"
#include "formats/html/dom/html_dom.h"

#include <litehtml/document.h>

#include <algorithm>
#include <string_view>
#include <unordered_set>

namespace docrender::forms {
using namespace litehtml;

namespace {
	element::ptr root_of(const element& p_node) {
		auto root = std::const_pointer_cast<element>(p_node.shared_from_this());
		while (const auto parent = root->parent()) root = parent;
		return root;
	}

	element::ptr by_id(const element::ptr& p_root, const char* p_id) {
		if (!p_id || !*p_id) return {};
		element::ptr found;
		HTMLDOM::visit_elements(p_root, [&](const element::ptr& p_node) {
			if (std::string_view(p_node->get_attr("id", "")) == p_id) found = p_node;
			return bool(found);
		});
		return found;
	}

	bool labelable(const element& p_node) {
		if (const auto* input = dynamic_cast<const HTMLInputNode*>(&p_node)) {
			return input->type() != HTMLInputNode::Type::HIDDEN && input->type() != HTMLInputNode::Type::OTHER;
		}
		return p_node.tag() == _textarea_ || p_node.tag() == _select_ || p_node.tag() == _button_;
	}
}

bool is_listed(const element& p_node) {
	const auto tag = p_node.tag();
	return tag == _input_ || tag == _textarea_ || tag == _button_ || tag == _select_ || tag == _fieldset_;
}

bool is_disabled(const element& p_node) {
	if (p_node.get_attr("disabled")) return true;
	if (p_node.tag() == _option_) {
		const auto group = p_node.parent();
		return group && group->tag() == _optgroup_ && group->get_attr("disabled");
	}
	const element* branch = &p_node;
	for (auto parent = p_node.parent(); parent; branch = parent.get(), parent = parent->parent()) {
		if (parent->tag() != _fieldset_ || !parent->get_attr("disabled")) continue;
		const auto& children = parent->children();
		const auto legend = std::find_if(children.begin(), children.end(), [](const element::ptr& p_child) {
			return p_child->tag() == _legend_;
		});
		if (legend == children.end() || legend->get() != branch) return true;
	}
	return false;
}

element::ptr owner(const element& p_node) {
	if (p_node.tag() == _legend_) {
		const auto parent = p_node.parent();
		return parent && parent->tag() == _fieldset_ ? owner(*parent) : element::ptr();
	}
	if (!is_listed(p_node)) return {};
	if (const auto* form = p_node.get_attr("form")) {
		const auto root = root_of(p_node);
		const auto doc = p_node.get_document();
		if (doc && root == doc->root()) {
			const auto match = by_id(root, form);
			return match && match->tag() == _form_ ? match : element::ptr();
		}
	}
	for (auto parent = p_node.parent(); parent; parent = parent->parent()) {
		if (parent->tag() == _form_) return parent;
	}
	return {};
}

element::ptr labeled_control(const element& p_label) {
	if (const auto* id = p_label.get_attr("for")) {
		const auto candidate = by_id(root_of(p_label), id);
		return candidate && labelable(*candidate) ? candidate : element::ptr();
	}
	element::ptr result;
	HTMLDOM::visit_elements(std::const_pointer_cast<element>(p_label.shared_from_this()), [&](const element::ptr& p_node) {
		if (labelable(*p_node)) result = p_node;
		return bool(result);
	});
	return result;
}

ButtonAction button_action(const element& p_node) {
	if (p_node.tag() != _button_) return ButtonAction::NONE;
	const auto type = godot::String::utf8(p_node.get_attr("type", "")).to_lower();
	if (type == "button") return ButtonAction::BUTTON;
	if (!owner(p_node)) return ButtonAction::NONE;
	return type == "reset" ? ButtonAction::RESET : ButtonAction::SUBMIT;
}

std::vector<element::ptr> controls(const element& p_form) {
	std::vector<element::ptr> result;
	HTMLDOM::visit_elements(root_of(p_form), [&](const element::ptr& p_node) {
		if (is_listed(*p_node) && owner(*p_node).get() == &p_form) result.push_back(p_node);
		return false;
	});
	return result;
}

std::vector<element::ptr> radio_group(const element& p_radio) {
	const std::string_view name(p_radio.get_attr("name", ""));
	if (name.empty()) return {std::const_pointer_cast<element>(p_radio.shared_from_this())};
	const auto form = owner(p_radio);
	std::vector<element::ptr> result;
	HTMLDOM::visit_elements(root_of(p_radio), [&](const element::ptr& p_node) {
		const auto* input = dynamic_cast<const HTMLInputNode*>(p_node.get());
		if (input && input->type() == HTMLInputNode::Type::RADIO && name == input->get_attr("name", "") && owner(*input) == form) {
			result.push_back(p_node);
		}
		return false;
	});
	return result;
}

element::ptr first_invalid(const element& p_form) {
	for (const auto& node : controls(p_form)) {
		if (is_disabled(*node)) continue;
		const bool required = node->get_attr("required");
		if (const auto* input = dynamic_cast<const HTMLInputNode*>(node.get())) {
			if (input->type() == HTMLInputNode::Type::RADIO) {
				bool group_required = false;
				bool checked = false;
				for (const auto& peer : radio_group(*node)) {
					group_required |= peer->get_attr("required") != nullptr;
					checked |= static_cast<const HTMLInputNode&>(*peer).is_checked();
				}
				if (group_required && !checked) return node;
			} else if (required && ((input->type() == HTMLInputNode::Type::TEXT && !node->get_attr("readonly") && input->get_value().is_empty()) ||
				(input->type() == HTMLInputNode::Type::CHECKBOX && !input->is_checked()))) return node;
		} else if (required) {
			if (const auto* area = dynamic_cast<const HTMLTextAreaNode*>(node.get()); area && !node->get_attr("readonly") && area->editor.get_value().is_empty()) return node;
			if (const auto* select = dynamic_cast<const HTMLSelectNode*>(node.get()); select && select->is_missing()) return node;
		}
	}
	return {};
}

void synchronize(const element::ptr& p_tree, const element::ptr& p_preferred) {
	std::unordered_set<const element*> preferred;
	if (p_preferred) HTMLDOM::visit_elements(p_preferred, [&](const element::ptr& p_node) {
		if (const auto* radio = dynamic_cast<const HTMLInputNode*>(p_node.get()); radio && radio->type() == HTMLInputNode::Type::RADIO && radio->is_checked()) {
			preferred.insert(radio);
		}
		return false;
	});
	std::unordered_set<const element*> handled_radios;
	const auto reconcile = [&](const element::ptr& p_node) {
		auto* radio = dynamic_cast<HTMLInputNode*>(p_node.get());
		if (!radio || radio->type() != HTMLInputNode::Type::RADIO || handled_radios.count(radio)) return false;
		const auto peers = radio_group(*radio);
		HTMLInputNode* checked = nullptr;
		for (const auto& peer : peers) {
			handled_radios.insert(peer.get());
			auto* candidate = static_cast<HTMLInputNode*>(peer.get());
			if (candidate->is_checked() && (!checked || preferred.count(candidate) || !preferred.count(checked))) checked = candidate;
		}
		for (const auto& peer : peers) {
			auto* candidate = static_cast<HTMLInputNode*>(peer.get());
			if (candidate != checked) candidate->clear_group_checked();
		}
		return false;
	};
	if (!p_tree) return;
	const auto root = root_of(*p_tree);
	// Checked nodes moved into a group take precedence over its old selection.
	HTMLDOM::visit_elements(root, reconcile);
	HTMLDOM::visit_elements(root, [&](const element::ptr& p_node) {
		if (auto* input = dynamic_cast<HTMLTextControlNode*>(p_node.get())) input->update_pseudos();
		else if (is_listed(*p_node) || p_node->tag() == _option_ || p_node->tag() == _optgroup_) {
			const bool disabled = is_disabled(*p_node);
			p_node->set_pseudo_class(_id("disabled"), disabled);
			p_node->set_pseudo_class(_id("enabled"), !disabled);
		}
		if (const auto* select = dynamic_cast<const HTMLSelectNode*>(p_node.get())) {
			const auto options = select->get_options();
			const auto selected = select->selected_index(options);
			for (size_t i = 0; i < options.size(); ++i) options[i]->set_pseudo_class(_id("checked"), static_cast<int64_t>(i) == selected);
		}
		if (const auto* option = dynamic_cast<const HTMLOptionNode*>(p_node.get()); option && !option->select()) {
			p_node->set_pseudo_class(_id("checked"), option->is_selected());
		}
		return false;
	});
}
}
