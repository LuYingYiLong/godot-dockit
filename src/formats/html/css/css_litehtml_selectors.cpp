#include <litehtml/css_parser.h>
#include <litehtml/css_selector.h>

namespace litehtml {

namespace {

namespace syntax = docrender::css;

syntax::SelectorMode selector_mode(document_mode mode) {
	return mode == quirks_mode ? syntax::SelectorMode::quirks : syntax::SelectorMode::standards;
}

css_selector::ptr bind_selector(const syntax::Selector& source) {
	auto selector = std::make_shared<css_selector>();
	selector->m_specificity = source.m_specificity;
	if (source.m_left) {
		selector->m_left = bind_selector(*source.m_left);
	}
	selector->m_combinator = source.m_combinator;
	selector->m_anchor_combinator = source.m_anchor_combinator;
	auto& right = selector->m_right;
	right.m_prefix = _id(source.m_right.m_prefix);
	right.m_tag = _id(source.m_right.m_tag);
	right.m_attrs.reserve(source.m_right.m_attrs.size());
	for (const auto& source_attr : source.m_right.m_attrs) {
		auto& attr = right.m_attrs.emplace_back(source_attr.type, source_attr.name);
		attr.prefix = _id(source_attr.prefix);
		attr.value = source_attr.value;
		attr.language_ranges = source_attr.language_ranges;
		attr.matcher = source_attr.matcher;
		attr.caseless_match = source_attr.caseless_match;
		attr.a = source_attr.a;
		attr.b = source_attr.b;
		attr.selector_list.reserve(source_attr.selector_list.size());
		for (const auto& argument : source_attr.selector_list) {
			attr.selector_list.push_back(bind_selector(*argument));
		}
	}
	return selector;
}

} // namespace

css_selector::vector parse_selector_list(const css_token_vector& tokens, int options, document_mode mode) {
	const auto selectors = syntax::parse_selector_list(tokens, options, selector_mode(mode));
	css_selector::vector bound;
	bound.reserve(selectors.size());
	for (const auto& selector : selectors) {
		selector->calc_specificity();
		bound.push_back(bind_selector(*selector));
	}
	return bound;
}

bool supports_css_selector(const css_token_vector& tokens, document_mode mode) {
	return syntax::supports_css_selector(tokens, selector_mode(mode));
}

bool css_selector::parse(const string& text, document_mode mode) {
	auto parsed = syntax::parse_complex_selector(normalize(text, f_componentize), selector_mode(mode), strict_mode);
	if (!parsed) {
		return false;
	}
	parsed->calc_specificity();
	*this = std::move(*bind_selector(*parsed));
	return true;
}

} // namespace litehtml
