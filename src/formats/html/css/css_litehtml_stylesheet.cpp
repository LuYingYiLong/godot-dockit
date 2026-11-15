// Portions adapted from litehtml.
// Copyright (c) 2013, Yuri Kobets (tordex). All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// See addons/docrender/LiteHTML-LICENSES/litehtml.txt for the license and disclaimer.

#include <litehtml/css_parser.h>
#include <litehtml/document.h>
#include <litehtml/document_container.h>
#include <litehtml/html.h>

#include "css_stylesheet_rules.h"
#include "css_supports.h"

namespace litehtml {

namespace syntax = docrender::css;

size_t css::register_layer(size_t parent, const string_vector& names)
{
	const auto append_child = [&](size_t index) {
		const size_t child = m_layers.size();
		m_layers[index].children.push_back(child);
		m_layers.emplace_back();
		return child;
	};
	if (names.empty()) return append_child(parent);

	for (const auto& name : names)
	{
		const auto [entry, inserted] = m_layers[parent].named_children.try_emplace(name, m_layers.size());
		const auto child = entry->second;
		if (inserted) append_child(parent);
		parent = child;
	}
	return parent;
}

static bool media_matches(document& doc, const media_query_list_list::ptr& media)
{
	if (!media) return true;
	media_features features{};
	doc.container()->get_media_features(features);
	media->apply_media_features(features);
	return media->is_used();
}


template<class Input>
void css::parse_css_stylesheet(const Input& input, string baseurl, document::ptr doc,
	media_query_list_list::ptr media, bool top_level, size_t layer_id) {
	if (doc && media) doc->add_media_list(media);

	using Kind = syntax::StylesheetRuleKind;
	const auto rules = css_parser::parse_stylesheet(input, top_level);
	syntax::visit_stylesheet_rules(rules, top_level, [&](const syntax::StylesheetRule& rule) {
		const auto& prelude = rule.syntax.prelude;
		switch (rule.kind) {
			case Kind::style:
				return parse_style_rule(rule.syntax, baseurl, doc, media, layer_id);
			case Kind::import:
				return apply_import(*rule.import, baseurl, doc, media, layer_id);
			case Kind::layer_block:
				if (media_matches(*doc, media)) {
					const auto child = register_layer(layer_id, rule.layers.front());
					parse_css_stylesheet(rule.syntax.block.value, baseurl, doc, media, false, child);
				}
				return true;
			case Kind::layer_order:
				if (media_matches(*doc, media)) {
					for (const auto& names : rule.layers) register_layer(layer_id, names);
				}
				return true;
			case Kind::media: {
				auto new_media = media;
				const auto list = parse_media_query_list(prelude, doc);
				// An empty media query list evaluates to true.
				if (!list.empty()) {
					new_media = make_shared<media_query_list_list>(media ? *media : media_query_list_list());
					new_media->add(list);
				}
				parse_css_stylesheet(rule.syntax.block.value, baseurl, doc, new_media, false, layer_id);
				return true;
			}
			case Kind::supports: {
				const auto condition = evaluate_css_supports(prelude, doc->mode());
				if (condition && *condition) parse_css_stylesheet(rule.syntax.block.value, baseurl, doc, media, false, layer_id);
				return condition.has_value();
			}
			case Kind::font_face:
				if (auto face = parse_font_face_rule(prelude, rule.syntax.block.value, baseurl)) {
					if (media_matches(*doc, media)) m_font_face_rules.push_back({std::move(*face), layer_id});
					return true;
				}
				return false;
			case Kind::font_feature_values:
				if (auto values = parse_font_feature_values_rule(prelude, rule.syntax.block.value)) {
					if (media_matches(*doc, media)) m_font_feature_values_rules.push_back({std::move(*values), layer_id});
					return true;
				}
				return false;
			case Kind::counter_style:
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
		layer_id = register_layer(layer_id, rule.layer);
	}

	string css_text;
	string css_baseurl = baseurl;
	doc->container()->import_css(css_text, rule.url, css_baseurl);
	parse_css_stylesheet(css_text, css_baseurl, doc, new_media, true, layer_id);
	return true;
}

bool css::parse_style_rule(const raw_rule& rule, const string& baseurl, document::ptr doc,
	media_query_list_list::ptr media, size_t layer_id) {
	auto selectors = parse_selector_list(rule.prelude, strict_mode, doc->mode());
	if (selectors.empty()) {
		css_parse_error("invalid selector");
		return false;
	}

	auto declarations = make_shared<style>();
	declarations->add(rule.block.value, baseurl, doc->container());
	for (const auto& selector : selectors) {
		selector->m_style = declarations;
		selector->m_media_query = media;
		selector->m_layer_id = layer_id;
		add_selector(selector);
	}
	return true;
}

void css::sort_selectors(bool author)
{
	if (author)
	{
		// Visit layers in reverse cascade order, with the parent before its children.
		std::vector<size_t> pending = m_layers[0].children;
		int rank = static_cast<int>(m_layers.size()) - 1;
		while (!pending.empty())
		{
			auto& layer = m_layers[pending.back()];
			pending.pop_back();
			layer.rank = --rank;
			pending.insert(pending.end(), layer.children.begin(), layer.children.end());
		}
	}
	m_font_faces.clear();
	if (author)
	{
		const auto by_layer = [&](const auto& left, const auto& right) {
			return m_layers[left.layer_id].rank < m_layers[right.layer_id].rank;
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
		const int rank = author ? m_layers[rule.layer_id].rank : css_default_layer;
		const auto found = m_counter_styles.find(rule.name);
		if (found == m_counter_styles.end() || found->second.first <= rank)
			m_counter_styles.insert_or_assign(rule.name, std::make_pair(rank, rule.style));
	}
	for (const auto& selector : m_selectors)
		selector->m_layer = author ? m_layers[selector->m_layer_id].rank : css_default_layer;

	std::sort(m_selectors.begin(), m_selectors.end(),
		 [](const css_selector::ptr& v1, const css_selector::ptr& v2)
		 {
			 return (*v1) < (*v2);
		 }
	);
}

} // namespace litehtml
