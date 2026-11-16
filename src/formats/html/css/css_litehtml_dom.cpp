#include "css_litehtml_dom.h"
#include "formats/html/elements/html_form_state.h"

#include <litehtml/html_tag.h>

#include <algorithm>

namespace litehtml {

namespace syntax = docrender::css;
using syntax::SelectorNode;
using syntax::SelectorNodeKind;

namespace {

const element& as_element(SelectorNode node) {
	return *static_cast<const element*>(node);
}

} // namespace

struct selector_dom final : syntax::SelectorDOM {
	SelectorNode parent(SelectorNode node) const override {
		return as_element(node).parent().get();
	}

	void visit_children(SelectorNode node, bool reverse,
		const std::function<bool(SelectorNode)>& visit) const override {
		const auto& children = as_element(node).children();
		if (reverse) {
			for (auto child = children.rbegin(); child != children.rend(); ++child) {
				if (visit(child->get())) return;
			}
		} else {
			for (const auto& child : children) {
				if (visit(child.get())) return;
			}
		}
	}

	SelectorNodeKind kind(SelectorNode node) const override {
		const auto& source = as_element(node);
		const auto tag = source.tag();
		if (is_generated_tag(tag)) return SelectorNodeKind::generated;
		if (tag != empty_id) return SelectorNodeKind::element;
		if (source.is_text()) return source.is_space() ? SelectorNodeKind::whitespace : SelectorNodeKind::text;
		return SelectorNodeKind::other;
	}

	bool is_matchable(SelectorNode node) const override {
		return dynamic_cast<const html_tag*>(&as_element(node)) != nullptr;
	}

	std::string_view tag(SelectorNode node) const override { return _s(as_element(node).tag()); }
	bool same_tag(SelectorNode left, SelectorNode right) const override {
		return as_element(left).tag() == as_element(right).tag();
	}
	std::string_view id(SelectorNode node) const override { return _s(as_element(node).id()); }

	bool has_class(SelectorNode node, std::string_view name) const override {
		const auto* tag = dynamic_cast<const html_tag*>(&as_element(node));
		if (!tag) return false;
		const auto& classes = tag->str_classes();
		return std::find(classes.begin(), classes.end(), name) != classes.end();
	}

	std::optional<std::string_view> attribute(SelectorNode node, const std::string& name) const override {
		if (const auto* value = as_element(node).get_attr(name.c_str())) return std::string_view(value);
		return {};
	}

	bool has_pseudo_class(SelectorNode node, std::string_view name) const override {
		const auto* tag = dynamic_cast<const html_tag*>(&as_element(node));
		if (!tag) return false;
		if (name == "required" || name == "optional") {
			return docrender::forms::supports_required(*tag) && (name == "required") == bool(tag->get_attr("required"));
		}
		if (name == "read-only" || name == "read-write") {
			return kind(node) == SelectorNodeKind::element && (name == "read-write") == docrender::forms::is_read_write(*tag);
		}
		return std::any_of(tag->m_pseudo_classes.begin(), tag->m_pseudo_classes.end(),
			[&](string_id pseudo) { return _s(pseudo) == name; });
	}
};

const syntax::SelectorDOM& selector_dom_view() {
	static const selector_dom dom;
	return dom;
}

int selector_match_flags(unsigned result) {
	if (result == syntax::match_none) return select_no_match;
	int flags = select_match;
	if (result & syntax::match_deferred_pseudo_class) flags |= select_match_pseudo_class;
	if (result & syntax::match_before) flags |= select_match_with_before;
	if (result & syntax::match_after) flags |= select_match_with_after;
	if (result & syntax::match_marker) flags |= select_match_with_marker;
	if (result & syntax::match_placeholder) flags |= select_match_with_placeholder;
	if (result & syntax::match_selection) flags |= select_match_with_selection;
	return flags;
}

const char* get_element_language(const element& node) {
	const auto language = syntax::inherited_language(selector_dom_view(), &node);
	return language ? language->data() : nullptr;
}

bool matches_element_language(const element& node, const std::vector<std::string>& ranges) {
	return syntax::matches_language(selector_dom_view(), &node, ranges);
}

} // namespace litehtml
