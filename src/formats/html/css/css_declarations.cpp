// Portions adapted from litehtml.
// Copyright (c) 2013, Yuri Kobets (tordex). All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// See addons/docrender/LiteHTML-LICENSES/litehtml.txt for the license and disclaimer.

#include "css_declarations.h"
#include "css_value_parser.h"

#include <litehtml/document.h>
#include <litehtml/document_container.h>
#include <litehtml/html_tag.h>

#include <litehtml/css_parser.h>

#include <algorithm>

namespace litehtml {

namespace {
	enum class property_family { keyword, box, background, text, layout };

	struct property_definition {
		property_family family;
		vector<string_id> longhands{};
		const char* keywords = nullptr;
	};

	const property_definition* find_property(string_id name) {
		static const std::map<string_id, property_definition> properties = {
			{ _z_index_, { property_family::box } },
			{ _text_indent_, { property_family::text } },
			{ _id("tab-size"), { property_family::text } },
			{ _padding_left_, { property_family::box } },
			{ _padding_right_, { property_family::box } },
			{ _padding_top_, { property_family::box } },
			{ _padding_bottom_, { property_family::box } },
			{ _left_, { property_family::box } },
			{ _right_, { property_family::box } },
			{ _top_, { property_family::box } },
			{ _bottom_, { property_family::box } },
			{ _id("inset"), { property_family::box, { _top_, _right_, _bottom_, _left_ } } },
			{ _margin_left_, { property_family::box } },
			{ _margin_right_, { property_family::box } },
			{ _margin_top_, { property_family::box } },
			{ _margin_bottom_, { property_family::box } },
			{ _width_, { property_family::box } },
			{ _aspect_ratio_, { property_family::box } },
			{ _height_, { property_family::box } },
			{ _min_width_, { property_family::box } },
			{ _min_height_, { property_family::box } },
			{ _max_width_, { property_family::box } },
			{ _max_height_, { property_family::box } },
			{ _line_height_, { property_family::box } },
			{ _font_size_, { property_family::box } },
			{ _margin_, { property_family::box, { _margin_top_, _margin_right_, _margin_bottom_, _margin_left_ } } },
			{ _padding_, { property_family::box, { _padding_top_, _padding_right_, _padding_bottom_, _padding_left_ } } },
			{ _color_, { property_family::background } },
			{ _background_color_, { property_family::background } },
			{ _border_top_color_, { property_family::background } },
			{ _border_bottom_color_, { property_family::background } },
			{ _border_left_color_, { property_family::background } },
			{ _border_right_color_, { property_family::background } },
			{ _background_, { property_family::background, {
				_background_color_, _background_position_x_, _background_position_y_,
				_id("background-repeat-x"), _id("background-repeat-y"),
				_background_attachment_, _background_image_, _background_image_baseurl_, _background_size_,
				_background_origin_, _background_clip_ } } },
			{ _id("object-fit"), { property_family::background } },
			{ _id("object-position"), { property_family::background } },
			{ _id("image-rendering"), { property_family::keyword, {}, image_rendering_keywords } },
			{ _id("image-orientation"), { property_family::background } },
			{ _id("image-resolution"), { property_family::background } },
			{ _id("direction"), { property_family::keyword, {}, direction_keywords } },
			{ _background_image_, { property_family::background, { _background_image_, _background_image_baseurl_ } } },
			{ _background_position_, { property_family::background, { _background_position_x_, _background_position_y_ } } },
			{ _background_position_x_, { property_family::background } },
			{ _background_position_y_, { property_family::background } },
			{ _id("background-position-inline"), { property_family::background, { _background_position_x_ } } },
			{ _id("background-position-block"), { property_family::background, { _background_position_y_ } } },
			{ _background_size_, { property_family::background } },
			{ _background_repeat_, { property_family::background, {
				_id("background-repeat-x"), _id("background-repeat-y") } } },
			{ _id("background-repeat-x"), { property_family::background } },
			{ _id("background-repeat-y"), { property_family::background } },
			{ _background_attachment_, { property_family::background, {}, background_attachment_strings } },
			{ _background_origin_, { property_family::background, {}, background_box_strings } },
			{ _background_clip_, { property_family::background, {}, background_box_strings } },
			{ _id("outline"), { property_family::background, {
				_id("outline-width"), _id("outline-style"), _id("outline-color") } } },
			{ _id("outline-width"), { property_family::background } },
			{ _id("outline-style"), { property_family::background } },
			{ _id("outline-color"), { property_family::background } },
			{ _id("outline-offset"), { property_family::background } },
			{ _id("border-image-source"), { property_family::background } },
			{ _id("border-image-slice"), { property_family::background } },
			{ _id("border-image-width"), { property_family::background } },
			{ _id("border-image-outset"), { property_family::background } },
			{ _id("border-image-repeat"), { property_family::background } },
			{ _id("border-image"), { property_family::background, {
				_id("border-image-source"), _id("border-image-slice"), _id("border-image-width"), _id("border-image-outset"), _id("border-image-repeat") } } },
			{ _border_, { property_family::background, {
				_border_top_width_, _border_right_width_, _border_bottom_width_, _border_left_width_,
				_border_top_style_, _border_right_style_, _border_bottom_style_, _border_left_style_,
				_border_top_color_, _border_right_color_, _border_bottom_color_, _border_left_color_,
				_id("border-image-source"), _id("border-image-slice"), _id("border-image-width"), _id("border-image-outset"), _id("border-image-repeat") } } },
			{ _border_left_, { property_family::background, { _border_left_width_, _border_left_style_, _border_left_color_ } } },
			{ _border_right_, { property_family::background, { _border_right_width_, _border_right_style_, _border_right_color_ } } },
			{ _border_top_, { property_family::background, { _border_top_width_, _border_top_style_, _border_top_color_ } } },
			{ _border_bottom_, { property_family::background, { _border_bottom_width_, _border_bottom_style_, _border_bottom_color_ } } },
			{ _border_width_, { property_family::background, { _border_top_width_, _border_right_width_, _border_bottom_width_, _border_left_width_ } } },
			{ _border_style_, { property_family::background, { _border_top_style_, _border_right_style_, _border_bottom_style_, _border_left_style_ } } },
			{ _border_color_, { property_family::background, { _border_top_color_, _border_right_color_, _border_bottom_color_, _border_left_color_ } } },
			{ _border_top_width_, { property_family::background } },
			{ _border_bottom_width_, { property_family::background } },
			{ _border_left_width_, { property_family::background } },
			{ _border_right_width_, { property_family::background } },
			{ _border_bottom_left_radius_, { property_family::background, { _border_bottom_left_radius_x_, _border_bottom_left_radius_y_ } } },
			{ _border_bottom_right_radius_, { property_family::background, { _border_bottom_right_radius_x_, _border_bottom_right_radius_y_ } } },
			{ _border_top_right_radius_, { property_family::background, { _border_top_right_radius_x_, _border_top_right_radius_y_ } } },
			{ _border_top_left_radius_, { property_family::background, { _border_top_left_radius_x_, _border_top_left_radius_y_ } } },
			{ _border_radius_x_, { property_family::background, { _border_top_left_radius_x_, _border_top_right_radius_x_, _border_bottom_right_radius_x_, _border_bottom_left_radius_x_ } } },
			{ _border_radius_y_, { property_family::background, { _border_top_left_radius_y_, _border_top_right_radius_y_, _border_bottom_right_radius_y_, _border_bottom_left_radius_y_ } } },
			{ _border_radius_, { property_family::background, {
				_border_top_left_radius_x_, _border_top_left_radius_y_, _border_top_right_radius_x_, _border_top_right_radius_y_,
				_border_bottom_left_radius_x_, _border_bottom_left_radius_y_, _border_bottom_right_radius_x_, _border_bottom_right_radius_y_ } } },
			{ _border_spacing_, { property_family::background, { __litehtml_border_spacing_x_, __litehtml_border_spacing_y_ } } },
			{ _list_style_image_, { property_family::background, { _list_style_image_, _list_style_image_baseurl_ } } },
			{ _list_style_, { property_family::background, { _list_style_image_, _list_style_image_baseurl_, _list_style_position_, _list_style_type_ } } },
			{ _id("box-shadow"), { property_family::background } },
			{ _id("text-shadow"), { property_family::background } },
			{ _font_, { property_family::text, { _font_style_, _font_weight_, _id("font-width"), _font_size_, _line_height_,
				_id("font-variant-ligatures"), _id("font-variant-numeric"), _id("font-variant-east-asian"), _id("font-variant-caps"),
				_id("font-variant-alternates"),
				_font_family_, _id("font-feature-settings"), _id("font-variation-settings"), _id("font-kerning"),
				_id("font-optical-sizing") } } },
			{ _font_variant_, { property_family::text, { _id("font-variant-ligatures"), _id("font-variant-numeric"),
				_id("font-variant-east-asian"), _id("font-variant-caps"), _id("font-variant-alternates") } } },
			{ _id("font-variant-ligatures"), { property_family::text } },
			{ _id("font-variant-numeric"), { property_family::text } },
			{ _id("font-variant-east-asian"), { property_family::text } },
			{ _id("font-variant-caps"), { property_family::text } },
			{ _id("font-variant-alternates"), { property_family::text } },
			{ _font_family_, { property_family::text } },
			{ _font_weight_, { property_family::text } },
			{ _id("font-width"), { property_family::text } },
			{ _id("font-feature-settings"), { property_family::text } },
			{ _id("font-variation-settings"), { property_family::text } },
			{ _id("font-kerning"), { property_family::keyword, {}, "auto;normal;none" } },
			{ _id("font-optical-sizing"), { property_family::keyword, {}, "auto;none" } },
			{ _id("letter-spacing"), { property_family::text } },
			{ _id("word-spacing"), { property_family::text } },
			{ _text_decoration_, { property_family::text, { _text_decoration_color_, _text_decoration_line_, _text_decoration_style_, _text_decoration_thickness_ } } },
			{ _text_decoration_thickness_, { property_family::text } },
			{ _text_decoration_color_, { property_family::text } },
			{ _text_decoration_line_, { property_family::text } },
			{ _id("text-underline-position"), { property_family::text } },
			{ _id("text-underline-offset"), { property_family::text } },
			{ _text_emphasis_, { property_family::text, { _text_emphasis_style_, _text_emphasis_color_ } } },
			{ _text_emphasis_style_, { property_family::text } },
			{ _text_emphasis_color_, { property_family::text } },
			{ _text_emphasis_position_, { property_family::text } },
			{ _gap_, { property_family::layout, { _row_gap_, _column_gap_ } } },
			{ _row_gap_, { property_family::layout } },
			{ _column_gap_, { property_family::layout } },
			{ _flex_, { property_family::layout, { _flex_grow_, _flex_shrink_, _flex_basis_ } } },
			{ _flex_grow_, { property_family::layout } },
			{ _flex_shrink_, { property_family::layout } },
			{ _flex_basis_, { property_family::layout } },
			{ _flex_flow_, { property_family::layout, { _flex_direction_, _flex_wrap_ } } },
			{ _justify_items_, { property_family::layout } },
			{ _align_items_, { property_family::layout } },
			{ _justify_self_, { property_family::layout } },
			{ _align_self_, { property_family::layout } },
			{ _justify_content_, { property_family::layout } },
			{ _align_content_, { property_family::layout } },
			{ _place_items_, { property_family::layout, { _align_items_, _justify_items_ } } },
			{ _place_self_, { property_family::layout, { _align_self_, _justify_self_ } } },
			{ _place_content_, { property_family::layout, { _align_content_, _justify_content_ } } },
			{ _transform_, { property_family::layout } },
			{ _transform_origin_, { property_family::layout } },
			{ _grid_template_, { property_family::layout, { _grid_template_rows_, _grid_template_columns_, _grid_template_areas_ } } },
			{ _grid_, { property_family::layout, { _grid_template_rows_, _grid_template_columns_, _grid_template_areas_, _grid_auto_rows_, _grid_auto_columns_, _grid_auto_flow_ } } },
			{ _grid_row_, { property_family::layout, { _grid_row_start_, _grid_row_end_ } } },
			{ _grid_column_, { property_family::layout, { _grid_column_start_, _grid_column_end_ } } },
			{ _grid_area_, { property_family::layout, { _grid_row_start_, _grid_column_start_, _grid_row_end_, _grid_column_end_ } } },
			{ _grid_row_start_, { property_family::layout } },
			{ _grid_row_end_, { property_family::layout } },
			{ _grid_column_start_, { property_family::layout } },
			{ _grid_column_end_, { property_family::layout } },
			{ _grid_template_columns_, { property_family::layout } },
			{ _grid_template_rows_, { property_family::layout } },
			{ _grid_template_areas_, { property_family::layout } },
			{ _grid_auto_rows_, { property_family::layout } },
			{ _grid_auto_columns_, { property_family::layout } },
			{ _grid_auto_flow_, { property_family::layout } },
			{ _order_, { property_family::layout } },
			{ _counter_increment_, { property_family::layout } },
			{ _counter_reset_, { property_family::layout } },
			{ _id("counter-set"), { property_family::layout } },
			{ _content_, { property_family::layout } },
			{ _id("quotes"), { property_family::layout } },
			{ _cursor_, { property_family::layout } },
			{ _display_, { property_family::keyword, {}, style_display_strings } },
			{ _visibility_, { property_family::keyword, {}, visibility_strings } },
			{ _position_, { property_family::keyword, {}, element_position_strings } },
			{ _float_, { property_family::keyword, {}, element_float_strings } },
			{ _clear_, { property_family::keyword, {}, element_clear_strings } },
			{ _overflow_, { property_family::keyword, {}, overflow_strings } },
			{ _appearance_, { property_family::keyword, {}, appearance_strings } },
			{ _box_sizing_, { property_family::keyword, {}, box_sizing_strings } },
			{ _text_align_, { property_family::text, { _id("text-align-all"), _id("text-align-last") } } },
			{ _id("text-align-all"), { property_family::text } },
			{ _id("text-align-last"), { property_family::text } },
			{ _id("text-justify"), { property_family::text } },
			{ _id("text-autospace"), { property_family::text } },
			{ _id("text-spacing-trim"), { property_family::text } },
			{ _id("text-spacing"), { property_family::text, { _id("text-spacing-trim"), _id("text-autospace") } } },
			{ _vertical_align_, { property_family::keyword, {}, vertical_align_strings } },
			{ _text_transform_, { property_family::text } },
			{ _white_space_, { property_family::text, { _id("white-space-collapse"), _id("text-wrap-mode"), _id("white-space-trim") } } },
			{ _id("white-space-collapse"), { property_family::keyword, {}, white_space_collapse_strings } },
			{ _id("text-wrap-mode"), { property_family::keyword, {}, text_wrap_mode_strings } },
			{ _id("text-wrap-style"), { property_family::keyword, {}, text_wrap_style_strings } },
			{ _id("word-break"), { property_family::keyword, {}, word_break_strings } },
			{ _id("line-break"), { property_family::keyword, {}, line_break_strings } },
			{ _id("overflow-wrap"), { property_family::keyword, {}, overflow_wrap_strings } },
			{ _id("hyphens"), { property_family::keyword, {}, hyphens_strings } },
			{ _id("hyphenate-character"), { property_family::text } },
			{ _id("hyphenate-limit-zone"), { property_family::text } },
			{ _id("hyphenate-limit-lines"), { property_family::text } },
			{ _id("hyphenate-limit-chars"), { property_family::text } },
			{ _id("hyphenate-limit-last"), { property_family::keyword, {}, hyphenate_limit_last_strings } },
			{ _id("text-wrap"), { property_family::text, { _id("text-wrap-mode"), _id("text-wrap-style") } } },
			{ _id("white-space-trim"), { property_family::text } },
			{ _font_style_, { property_family::text } },
			{ _list_style_type_, { property_family::background } },
			{ _list_style_position_, { property_family::keyword, {}, list_style_position_strings } },
			{ _border_left_style_, { property_family::keyword, {}, border_style_strings } },
			{ _border_right_style_, { property_family::keyword, {}, border_style_strings } },
			{ _border_top_style_, { property_family::keyword, {}, border_style_strings } },
			{ _border_bottom_style_, { property_family::keyword, {}, border_style_strings } },
			{ _border_collapse_, { property_family::keyword, {}, border_collapse_strings } },
			{ _flex_direction_, { property_family::keyword, {}, flex_direction_strings } },
			{ _flex_wrap_, { property_family::keyword, {}, flex_wrap_strings } },
			{ _caption_side_, { property_family::keyword, {}, caption_side_strings } },
			{ _text_decoration_style_, { property_family::keyword, {}, style_text_decoration_style_strings } },
		};
		const auto it = properties.find(name);
		return it == properties.end() ? nullptr : &it->second;
	}
}

const vector<string_id>& css_longhands(string_id name) {
	static const vector<string_id> empty;
	const auto* definition = find_property(name);
	return definition ? definition->longhands : empty;
}

const char* css_keywords(string_id name) {
	const auto* definition = find_property(name);
	return definition ? definition->keywords : nullptr;
}

std::optional<css_diagnostic> check_css_declaration(css_declaration_status status, string_id name,
	const css_token_vector& tokens, const css_source& source)
{
	if (source.kind == css_source_kind::none ||
		(status != css_declaration_status::unknown && status != css_declaration_status::invalid))
	{
		return std::nullopt;
	}
	const auto kind = status == css_declaration_status::unknown ?
		css_diagnostic_kind::unknown_property : css_diagnostic_kind::unsupported_value;
	return css_diagnostic{ kind, _s(name), tokens.empty() ? string() : get_repr(tokens, 0, -1, true), source };
}

static bool has_drawing_gap(string_id, const property_value&)
{
	// Keep this shared check for declarations that parse ahead of their painting support.
	return false;
}

static css_parsed_declaration parse_declaration(string_id name, const css_token_vector& tokens,
	const string& baseurl, bool important, document_container* container, const css_source& source,
	const css_token_vector& original_tokens)
{
	using status = css_declaration_status;
	const auto original_name = name;
	static const std::map<string_id, string_id> aliases = {
		{ _id("font-stretch"), _id("font-width") },
		{ _id("word-wrap"), _id("overflow-wrap") },
		// DocRender currently lays out boxes in the horizontal writing mode.
		{ _id("inline-size"), _width_ },
		{ _id("min-inline-size"), _min_width_ },
		{ _id("max-inline-size"), _max_width_ },
		{ _id("block-size"), _height_ },
		{ _id("background-repeat-inline"), _id("background-repeat-x") },
		{ _id("background-repeat-block"), _id("background-repeat-y") },
		{ _id("min-block-size"), _min_height_ },
		{ _id("max-block-size"), _max_height_ },
	};
	if (const auto alias = aliases.find(name); alias != aliases.end()) name = alias->second;
	const auto& spelling = _s(name);
	const bool custom = spelling.size() > 2 && spelling.compare(0, 2, "--") == 0;
	const auto* definition = find_property(name);
	if (!custom && !definition) return { status::unknown, {} };
	if (tokens.empty() ? !custom : !is_declaration_value(tokens)) return { status::invalid, {} };

	const auto variables = inspect_css_variables(tokens);
	if (variables == css_variable_syntax::invalid) return { status::invalid, {} };
	const auto keyword = tokens.size() == 1 ? tokens.front().ident() : string();
	const auto& longhands = css_longhands(name);
	const auto expand = [&](const property_value& value, status result) {
		props_map properties;
		if (longhands.empty()) properties.emplace(name, value);
		else for (const auto longhand : longhands) properties.emplace(longhand, value);
		return css_parsed_declaration{ result, std::move(properties) };
	};

	const auto wide_keyword = read_css_wide_keyword(tokens);
	switch (wide_keyword)
	{
	case css_wide_keyword::inherit: return expand(property_value(inherit{}, important), status::parsed);
	case css_wide_keyword::unset:
	case css_wide_keyword::initial:
		// This non-inherited logical longhand defaults to the flow-start edge, even in RTL.
		if (name == _id("background-position-inline")) {
			const vector<css_axis_position> initial{ { {}, css_position_edge::flow_start } };
			return expand(property_value(initial, important), status::parsed);
		}
		return wide_keyword == css_wide_keyword::initial ?
			expand(property_value(initial_value{}, important), status::parsed) :
			expand(property_value(invalid{}, important), status::parsed);
	case css_wide_keyword::revert: return expand(property_value(revert_value{}, important), status::parsed);
	case css_wide_keyword::revert_layer: return expand(property_value(revert_layer_value{}, important), status::parsed);
	case css_wide_keyword::none: break;
	}

	if (custom)
		return expand(property_value(tokens, important), status::parsed);
	if (variables == css_variable_syntax::valid)
	{
		property_value value(invalid{}, important);
		value.deferred = std::make_shared<property_value::deferred_declaration>(
			property_value::deferred_declaration{ original_name, tokens, baseurl, source });
		return expand(value, status::deferred);
	}

	css_value_parser parser;
	switch (definition->family)
	{
	case property_family::keyword:
		if (const int index = value_index(keyword, definition->keywords); index >= 0)
			parser.add_parsed_property(name, property_value(index, important));
		break;
	case property_family::box:
		parser.parse_box(name, tokens, important);
		break;
	case property_family::background:
		parser.parse_background_and_border(name, tokens, baseurl, important, container);
		break;
	case property_family::text:
		parser.parse_text(name, tokens, important, container);
		break;
	case property_family::layout:
		parser.parse_layout(name, tokens, baseurl, important);
		break;
	}
	auto properties = parser.take_properties();
	if (properties.empty()) return { status::invalid, {} };
	for (const auto longhand : longhands)
		properties.try_emplace(longhand, initial_value{}, important);

	if (source.kind != css_source_kind::none)
	{
		std::shared_ptr<const property_value::declaration_origin> origin;
		for (auto& [longhand, value] : properties)
		{
			if (!has_drawing_gap(longhand, value)) continue;
			if (!origin) origin = std::make_shared<property_value::declaration_origin>(
				property_value::declaration_origin{ original_name, get_repr(original_tokens, 0, -1, true), source });
			value.origin = origin;
		}
	}
	return { status::parsed, std::move(properties) };
}

css_parsed_declaration parse_css_declaration(string_id name, const css_token_vector& tokens,
	const string& baseurl, bool important, document_container* container, const css_source& source)
{
	return parse_declaration(name, tokens, baseurl, important, container, source, tokens);
}

bool supports_css_declaration(const css_token_vector& tokens)
{
	int index = 0;
	skip_whitespace(tokens, index);
	if (index == static_cast<int>(tokens.size()) || tokens[index].type != IDENT)
		return false;

	auto declaration = css_parser(css_token_vector(tokens.begin() + index, tokens.end())).consume_declaration();
	if (!declaration) return false;

	remove_whitespace(declaration.value);
	const auto name = _id(declaration.name.compare(0, 2, "--") == 0 ? declaration.name : lowcase(declaration.name));
	const auto result = parse_css_declaration(name, declaration.value, "", declaration.important, nullptr, {});
	if (result.status == css_declaration_status::deferred)
		return true;
	return result.status == css_declaration_status::parsed &&
		std::none_of(result.properties.begin(), result.properties.end(), [](const auto& entry) {
			return has_drawing_gap(entry.first, entry.second);
		});
}

static bool is_rollback(const property_value& value)
{
	return value.is<revert_value>() || value.is<revert_layer_value>();
}

static const property_value& rollback_declaration(string_id name, int layer, bool layer_only,
	const props_map& defaults, const css_layer_candidates& layers, bool has_author_layers)
{
	if (has_author_layers && layer_only)
	{
		if (const auto it = layers.find(name); it != layers.end())
		{
			const property_value* best = nullptr;
			for (const auto& [rank, candidate] : it->second)
			{
				if (rank >= layer) break;
				if (!best || takes_cascade_precedence(candidate, *best))
					best = &candidate;
			}
			if (best) return *best;
		}
	}
	// A rollback in the baseline itself has nowhere further to go.
	if (layer != css_default_layer)
	{
		if (const auto it = defaults.find(name); it != defaults.end())
			return it->second;
	}
	static const property_value unset;
	return unset;
}

void resolve_css_declarations(const props_map& specified, const props_map& defaults,
	const css_layer_candidates& layers, const html_tag& element, css_computed_variables& computed_variables,
	props_map& computed, std::vector<css_diagnostic>& diagnostics)
{
	computed.clear();
	computed_variables.clear();
	diagnostics.clear();
	const auto doc = element.get_document();
	const bool has_author_layers = doc->has_author_layers();
	const auto rollback = [&](string_id name, int layer, bool layer_only) -> const property_value& {
		return rollback_declaration(name, layer, layer_only, defaults, layers, has_author_layers);
	};
	const auto restore = [&](string_id name, const property_value& value) -> const property_value& {
		const auto* effective = &value;
		while (is_rollback(*effective))
			effective = &rollback(name, effective->m_layer, effective->is<revert_layer_value>());
		return *effective;
	};

	css_custom_properties local_variables;
	std::vector<std::pair<string_id, const property_value*>> deferred_winners;
	for (const auto& [name, value] : specified)
	{
		const auto& effective = restore(name, value);
		if (effective.deferred)
			deferred_winners.emplace_back(name, &effective);
		else if (is_rollback(value))
			computed.emplace(name, effective);
		// Only custom declarations carry raw CSS token lists.
		if (effective.is<css_token_vector>() || (effective.is<initial_value>() && _s(name).compare(0, 2, "--") == 0))
			local_variables.emplace(name, &effective);
	}

	if (!local_variables.empty() || !deferred_winners.empty())
	{
		const auto parent = element.parent();
		const css_custom_rollback restore_custom = [&](string_id name, const property_value& value,
			css_wide_keyword keyword) -> const property_value& {
			return restore(name, rollback(name, value.m_layer, keyword == css_wide_keyword::revert_layer));
		};
		css_variable_resolver resolver(dynamic_cast<const html_tag*>(parent.get()), std::move(local_variables), restore_custom);
		auto* container = doc->container();
		std::map<const property_value::deferred_declaration*, props_map> parsed;
		const auto evaluate = [&](const property_value& value) -> const props_map& {
			const auto& declaration = *value.deferred;
			auto [it, inserted] = parsed.try_emplace(&declaration);
			if (!inserted) return it->second;
			if (const auto tokens = resolver.substitute(declaration.tokens))
			{
				auto result = parse_declaration(declaration.name, *tokens, declaration.baseurl,
					value.m_important, container, declaration.source, declaration.tokens);
				if (result.status == css_declaration_status::parsed)
				{
					it->second = std::move(result.properties);
					return it->second;
				}
			}
			if (auto diagnostic = check_css_declaration(css_declaration_status::invalid, declaration.name,
				declaration.tokens, declaration.source))
				diagnostics.push_back(std::move(*diagnostic));
			return it->second;
		};

		for (const auto& [name, winner] : deferred_winners)
		{
			const auto* effective = winner;
			for (;;)
			{
				if (!effective->deferred)
				{
					computed.insert_or_assign(name, *effective);
					break;
				}
				const auto& properties = evaluate(*effective);
				const auto it = properties.find(name);
				if (it == properties.end())
				{
					computed.insert_or_assign(name, property_value{});
					break;
				}
				if (!is_rollback(it->second))
				{
					computed.insert_or_assign(name, it->second);
					break;
				}
				// A substituted rollback uses the original declaration's layer.
				effective = &restore(name, rollback(name, effective->m_layer, it->second.is<revert_layer_value>()));
			}
		}
		computed_variables = resolver.take_computed();
	}

	for (const auto& [name, value] : specified)
	{
		const auto it = computed.find(name);
		const auto& effective = it == computed.end() ? value : it->second;
		if (effective.origin)
			diagnostics.push_back({ css_diagnostic_kind::not_rendered, _s(effective.origin->name),
				effective.origin->value, effective.origin->source });
	}
}

} // namespace litehtml
