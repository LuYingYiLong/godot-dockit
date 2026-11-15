// Portions adapted from litehtml.
// Copyright (c) 2013, Yuri Kobets (tordex). All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// See addons/docrender/LiteHTML-LICENSES/litehtml.txt for the license and disclaimer.

#include <litehtml/css_properties.h>
#include <litehtml/document.h>
#include <litehtml/document_container.h>
#include <litehtml/html.h>
#include <litehtml/html_tag.h>

#include "css_compute_context.h"
#include "css_image_value.h"

namespace {
	using namespace litehtml;

	class DocumentStyleHost final : public docrender::css::StyleHost
	{
		document& doc;

	public:
		explicit DocumentStyleHost(document& value) : doc(value) {}

		pixel_t default_font_size() const override { return doc.container()->get_default_font_size(); }
		font_metrics initial_font_metrics() const override { return doc.initial_font_metrics(); }

		pixel_t points_to_pixels(float points) const override { return doc.container()->pt_to_px(points); }

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
			compute_css_image(value, base, doc, metrics, resolution, orientation);
		}
	};

	const property_value& read_declaration(const void* element, string_id name)
	{
		return static_cast<const html_tag*>(element)->get_cascaded_property(name);
	}
}

void litehtml::css_properties::compute(const html_tag* element, const document::ptr& doc)
{
	const auto parent = element->parent();
	const DocumentStyleHost host(*doc);
	const auto root = doc->root();
	ComputedStyle::compute({element, read_declaration, parent ? &parent->css() : nullptr, host,
		doc->get_media_features(), root ? &root->css().get_font_metrics() : nullptr,
		element->is_root(), element->is_replaced()});
}
