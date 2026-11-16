// Portions adapted from litehtml.
// Copyright (c) 2013, Yuri Kobets (tordex). All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// See addons/docrender/LiteHTML-LICENSES/litehtml.txt for the license and disclaimer.

#include <litehtml/css_parser.h>
#include <litehtml/document.h>
#include <litehtml/document_container.h>
#include <litehtml/html.h>

#include "css_stylesheet_rules.h"
#include "css_selector_matcher.h"
#include "css_supports.h"

namespace litehtml {

namespace syntax = docrender::css;

static bool media_matches(document& doc, const media_query_list_list::ptr& media)
{
	if (!media) return true;
	media_features features{};
	doc.container()->get_media_features(features);
	media->apply_media_features(features);
	return media->is_used();
}


void css::parse_css_stylesheet(const string& input, const string& baseurl, document::ptr doc,
	media_query_list_list::ptr media, size_t layer_id, const element* implicit_scope_root) {
	parse_rules(css_parser::parse_stylesheet(input, true),
		{baseurl, doc, std::move(media), layer_id, {}, {}, implicit_scope_root ? implicit_scope_root : doc->root().get()},
		true);
}

void css::parse_block(const css_token_vector& tokens, RuleContext context) {
	// Bound recursion for deeply nested rules in a stylesheet.
	if (++context.depth > 128) return;
	const auto rules = context.nesting || context.scope ? css_parser(tokens).consume_block_contents() :
		css_parser::parse_stylesheet(tokens, false);
	parse_rules(rules, context, false);
}

void css::parse_rules(const raw_rule::vector& rules, const RuleContext& context, bool top_level) {
	const auto& [baseurl, doc, media, layer_id, nesting, scope, implicit_scope_root, in_style_rule, depth, containers] = context;
	if (doc && media) doc->add_media_list(media);

	using Kind = syntax::StylesheetRuleKind;
	syntax::visit_stylesheet_rules(rules, top_level, [&](const syntax::StylesheetRule& rule) {
		const auto& prelude = rule.syntax.prelude;
		switch (rule.kind) {
			case Kind::style:
				return parse_style_rule(rule.syntax, context);
			case Kind::declarations:
				if (!nesting && !scope) return false;
				apply_declarations(rule.syntax.declarations, context);
				return true;
			case Kind::import:
				return apply_import(*rule.import, baseurl, doc, media, layer_id);
			case Kind::layer_block:
				if (media_matches(*doc, media)) {
					auto child = context;
					child.layer_id = cascade_layers.register_layer(layer_id, rule.layers.front());
					parse_block(rule.syntax.block.value, std::move(child));
				}
				return true;
			case Kind::layer_order:
				if (in_style_rule) return false;
				if (media_matches(*doc, media)) {
					for (const auto& names : rule.layers) cascade_layers.register_layer(layer_id, names);
				}
				return true;
			case Kind::media: {
				auto child = context;
				const auto list = parse_media_query_list(prelude, doc);
				// An empty media query list evaluates to true.
				if (!list.empty()) {
					child.media = make_shared<media_query_list_list>(media ? *media : media_query_list_list());
					child.media->add(list);
				}
				parse_block(rule.syntax.block.value, std::move(child));
				return true;
			}
			case Kind::container: {
				auto conditions = parse_container_conditions(prelude, doc);
				if (!conditions) return false;
				auto child = context;
				child.container_queries = make_shared<container_query>(container_query{std::move(*conditions), containers});
				parse_block(rule.syntax.block.value, std::move(child));
				return true;
			}
			case Kind::supports: {
				const auto condition = evaluate_css_supports(prelude, doc->mode());
				if (condition && *condition) parse_block(rule.syntax.block.value, context);
				return condition.has_value();
			}
			case Kind::scope:
				return parse_scope_rule(rule.syntax, context);
			case Kind::font_face:
				if (in_style_rule) return false;
				if (auto face = parse_font_face_rule(prelude, rule.syntax.block.value, baseurl)) {
					if (media_matches(*doc, media)) m_font_face_rules.push_back({std::move(*face), layer_id});
					return true;
				}
				return false;
			case Kind::font_feature_values:
				if (in_style_rule) return false;
				if (auto values = parse_font_feature_values_rule(prelude, rule.syntax.block.value)) {
					if (media_matches(*doc, media)) m_font_feature_values_rules.push_back({std::move(*values), layer_id});
					return true;
				}
				return false;
			case Kind::counter_style:
				if (in_style_rule) return false;
				if (auto parsed = parse_counter_style_rule(prelude, rule.syntax.block.value)) {
					if (media_matches(*doc, media)) {
						m_counter_style_rules.push_back({std::move(parsed->first), std::move(parsed->second), layer_id});
					}
					return true;
				}
				return false;
		}
		return false;
	});
}

bool css::apply_import(const syntax::StylesheetImport& rule, const string& baseurl, document::ptr doc,
	media_query_list_list::ptr media, size_t layer_id) {
	if (rule.supports) {
		const auto supported = evaluate_css_supports(*rule.supports, doc->mode(), true);
		if (!supported) return false;
		if (!*supported) return true;
	}

	auto new_media = media;
	const auto list = parse_media_query_list(rule.media, doc);
	if (!list.empty()) {
		new_media = make_shared<media_query_list_list>(media ? *media : media_query_list_list());
		new_media->add(list);
	}

	if (rule.layered) {
		if (new_media) doc->add_media_list(new_media);
		if (!media_matches(*doc, new_media)) return true;
		layer_id = cascade_layers.register_layer(layer_id, rule.layer);
	}

	string css_text;
	string css_baseurl = baseurl;
	doc->container()->import_css(css_text, rule.url, css_baseurl);
	parse_css_stylesheet(css_text, css_baseurl, doc, new_media, layer_id);
	return true;
}

bool css::parse_style_rule(const raw_rule& rule, const RuleContext& context) {
	const auto mode = selector_mode(context.doc->mode());
	auto selectors = context.nesting ? syntax::parse_nested_selector_list(rule.prelude, mode, context.nesting) : context.scope ?
		syntax::parse_scoped_selector_list(rule.prelude, mode) :
		syntax::parse_selector_list(rule.prelude, syntax::strict_mode, mode);
	if (selectors.empty()) {
		css_parse_error("invalid selector");
		return false;
	}
	auto child = context;
	child.in_style_rule = true;
	child.nesting = std::make_shared<syntax::NestingSelectorList>(std::move(selectors));
	parse_block(rule.block.value, std::move(child));
	return true;
}

bool css::parse_scope_rule(const raw_rule& rule, const RuleContext& context) {
	auto scope = std::make_shared<syntax::ScopeRule>();
	scope->parent = context.scope;
	scope->implicit_root = context.implicit_scope_root;
	const auto& tokens = rule.prelude;
	const auto mode = selector_mode(context.doc->mode());
	constexpr int options = syntax::strict_mode | syntax::forbid_pseudo_elements;
	const int size = static_cast<int>(tokens.size());
	int index = 0;
	skip_whitespace(tokens, index);
	if (index < size && tokens[index].type == ROUND_BLOCK) {
		const auto& start = tokens[index++].value;
		if (context.nesting) scope->start = syntax::parse_nested_selector_list(start, mode, context.nesting, options);
		else if (context.scope) scope->start = syntax::parse_scoped_selector_list(start, mode, options);
		else scope->start = syntax::parse_selector_list(start, options, mode);
		if (scope->start.empty()) return false;
		skip_whitespace(tokens, index);
	}
	if (index < size) {
		if (tokens[index].type != IDENT || lowcase(tokens[index++].ident()) != "to") return false;
		skip_whitespace(tokens, index);
		if (index >= size || tokens[index].type != ROUND_BLOCK) return false;
		scope->end = syntax::parse_scoped_selector_list(tokens[index++].value, mode, options);
		if (scope->end.empty()) return false;
		skip_whitespace(tokens, index);
		if (index != size) return false;
	}
	auto child = context;
	child.nesting.reset();
	child.scope = std::move(scope);
	parse_block(rule.block.value, std::move(child));
	return true;
}

void css::apply_declarations(const raw_declaration::vector& properties, const RuleContext& context) {
	static const auto root_selector = syntax::parse_selector_list(
		{syntax::css_token(syntax::AMPERSAND)}, syntax::strict_mode, syntax::SelectorMode::standards);
	const auto& selectors = context.nesting ? context.nesting->selectors : root_selector;
	auto declarations = make_shared<style>();
	declarations->add_declarations(properties, context.baseurl, context.doc->container());
	for (const auto& syntax : selectors) {
		auto selector = make_shared<css_selector>();
		selector->m_syntax = syntax;
		selector->m_scope = context.scope;
		selector->m_style = declarations;
		selector->m_media_query = context.media;
		selector->m_container_query = context.container_queries;
		selector->m_layer_id = context.layer_id;
		add_selector(selector);
	}
}

void css::sort_selectors(bool author)
{
	if (author) cascade_layers.assign_ranks();
	m_font_faces.clear();
	if (author)
	{
		const auto by_layer = [&](const auto& left, const auto& right) {
			return cascade_layers.rank(left.layer_id) < cascade_layers.rank(right.layer_id);
		};
		std::stable_sort(m_font_face_rules.begin(), m_font_face_rules.end(), by_layer);
		std::stable_sort(m_font_feature_values_rules.begin(), m_font_feature_values_rules.end(), by_layer);
	}
	for (const auto& rule : m_font_face_rules)
		m_font_faces.push_back(rule.face);
	m_font_feature_values.clear();
	for (const auto& rule : m_font_feature_values_rules)
		m_font_feature_values.push_back(rule.values);
	m_counter_styles.clear();
	for (auto& rule : m_counter_style_rules)
	{
		const int rank = author ? cascade_layers.rank(rule.layer_id) : css_default_layer;
		const auto found = m_counter_styles.find(rule.name);
		if (found == m_counter_styles.end() || found->second.first <= rank)
			m_counter_styles.insert_or_assign(rule.name, std::make_pair(rank, rule.style));
	}
	for (const auto& selector : m_selectors)
		selector->m_layer = author ? cascade_layers.rank(selector->m_layer_id) : css_default_layer;

	std::sort(m_selectors.begin(), m_selectors.end(),
		 [](const css_selector::ptr& v1, const css_selector::ptr& v2)
		 {
			 return (*v1) < (*v2);
		 }
	);
}

} // namespace litehtml
