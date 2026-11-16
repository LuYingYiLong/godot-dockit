// Portions adapted from litehtml.
// Copyright (c) 2013, Yuri Kobets (tordex). All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// See addons/docrender/LiteHTML-LICENSES/litehtml.txt for the license and disclaimer.

#include <litehtml/css_properties.h>
#include <litehtml/document.h>
#include <litehtml/document_container.h>
#include <litehtml/html.h>
#include <litehtml/html_tag.h>

#include "css_syntax_util.h"

#include "css_compute_context.h"
#include "css_image_value.h"

namespace {
	using namespace litehtml;

	class DocumentStyleHost final : public docrender::css::StyleHost
	{
		document& doc;
		const element& subject;
		const bool include_subject;

	public:
		DocumentStyleHost(document& value, const element& owner, bool include_owner = false) :
			doc(value), subject(owner), include_subject(include_owner) {}

		pixel_t default_font_size() const override { return doc.container()->get_default_font_size(); }
		font_metrics initial_font_metrics() const override { return doc.initial_font_metrics(); }

		pixel_t points_to_pixels(float points) const override { return doc.container()->pt_to_px(points); }

		css_container_units container_units() const override
		{
			return {this, [](const void* value, bool block_axis) {
				const auto& host = *static_cast<const DocumentStyleHost*>(value);
				return host.doc.resolve_container_unit(host.subject, block_axis, host.include_subject);
			}};
		}

		uint_ptr get_font(const font_description& description, font_metrics* metrics) const override
		{
			return doc.get_font(description, metrics);
		}

		void load_image(const string& url, const string& base, css_image_orientation orientation) const override
		{
			doc.container()->load_image(url.c_str(), base.c_str(), true, orientation);
		}

		void compute_image(image& value, const string& base, const font_metrics& metrics,
			const css_image_resolution& resolution, css_image_orientation orientation) const override
		{
			compute_css_image(value, base, doc, metrics, resolution, orientation, {nullptr, nullptr, container_units()});
		}
	};

	bool is_toggle(const html_tag& host) {
		if (host.tag() != _input_) return false;
		const auto type = docrender::css::detail::lowcase(std::string(host.get_attr("type", "")));
		return type == "checkbox" || type == "radio";
	}

	bool suppress_contents_display(const html_tag& element) {
		static const auto svg = _id("svg");
		static const auto wbr = _id("wbr");
		return element.is_replaced() ||
			docrender::css::detail::is_one_of(element.tag(), _input_, _textarea_, _select_, _br_, wbr, svg);
	}

	bool uses_native_chrome(const html_tag& host, const element* parent) {
		const auto& appearance = host.get_cascaded_property(_appearance_);
		if (appearance.is<int>()) return appearance.get<int>() != appearance_none;
		return appearance.is<inherit>() && parent && parent->css().get_appearance() != appearance_none;
	}

	struct ElementStyleInput {
		const html_tag& element;
		bool suppress_chrome;
		const document_container& container;
		const props_map* keyframe;
	};

	const property_value& read_declaration(const void* declarations, string_id name)
	{
		const auto& input = *static_cast<const ElementStyleInput*>(declarations);
		const auto& value = input.element.get_cascaded_property(name);
		if (input.keyframe) {
			const auto override = input.keyframe->find(name);
			if (override != input.keyframe->end()) return override->second;
		} else {
			if (const auto* transition = input.container.transition_override(&input.element, name)) return *transition;
			if (!value.m_important) {
				if (const auto* animation = input.container.animation_override(&input.element, name)) return *animation;
			}
		}
		if (input.suppress_chrome && value.priority.layer == css_default_layer &&
			docrender::css::detail::is_one_of(name, _background_color_,
				_border_top_width_, _border_right_width_, _border_bottom_width_, _border_left_width_,
				_border_top_style_, _border_right_style_, _border_bottom_style_, _border_left_style_,
				_border_top_color_, _border_right_color_, _border_bottom_color_, _border_left_color_,
				_border_top_left_radius_x_, _border_top_left_radius_y_,
				_border_top_right_radius_x_, _border_top_right_radius_y_,
				_border_bottom_left_radius_x_, _border_bottom_left_radius_y_,
				_border_bottom_right_radius_x_, _border_bottom_right_radius_y_))
		{
			static const property_value absent;
			return absent;
		}
		return value;
	}

	const property_value& read_placeholder(const void* declarations, string_id name)
	{
		const std::string_view spelling = _s(name);
		if (spelling.compare(0, 5, "font-") == 0 || docrender::css::detail::is_one_of(spelling, "color", "line-height", "opacity")) {
			return static_cast<const style*>(declarations)->get_property(name);
		}
		static const property_value absent;
		return absent;
	}
}

void litehtml::css_properties::compute(const html_tag* element, const document::ptr& doc, const props_map* keyframe)
{
	viewport_overflow_source = false;
	const auto parent = element->parent();
	auto box_parent = parent;
	while (box_parent && box_parent->css().get_display() == display_contents)
		box_parent = box_parent->parent();
	const DocumentStyleHost host(*doc, *element);
	const auto root = doc->root();
	const bool control = docrender::css::detail::is_one_of(element->tag(), _input_, _textarea_, _select_, _button_);
	const bool native = control && uses_native_chrome(*element, parent.get());
	const ElementStyleInput input{*element, control && !native, *doc->container(), keyframe};
	const auto viewport = doc->get_viewport_unit_features();
	ComputedStyle::compute({&input, read_declaration, parent ? &parent->css() : nullptr, host,
		viewport, root ? &root->css().get_font_metrics() : nullptr,
		element->is_root(), element->is_replaced(), box_parent ? &box_parent->css() : nullptr,
		suppress_contents_display(*element)});

	if (native && is_toggle(*element) && element->has_pseudo_class(_id("checked")) && !element->has_pseudo_class(_id("disabled")) &&
		!element->has_author_property(_background_color_) &&
		(keyframe ? !keyframe->count(_background_color_) : !doc->container()->transition_override(element, _background_color_) &&
			!doc->container()->animation_override(element, _background_color_)))
	{
		auto background = get_bg();
		background.m_color = get_accent_color();
		set_bg(background);
	}
}

void litehtml::css_properties::compute_placeholder(const html_tag& element, const style& declarations, document& doc)
{
	const DocumentStyleHost host(doc, element, true);
	const auto root = doc.root();
	const auto viewport = doc.get_viewport_unit_features();
	ComputedStyle::compute({&declarations, read_placeholder, &element.css(), host, viewport,
		root ? &root->css().get_font_metrics() : nullptr, false, false, &element.css()});
}
