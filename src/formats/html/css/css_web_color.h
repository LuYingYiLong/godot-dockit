// Portions adapted from litehtml.
// Copyright (c) 2013, Yuri Kobets (tordex). All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// See addons/docrender/LiteHTML-LICENSES/litehtml.txt for the license and disclaimer.

#ifndef LH_WEB_COLOR_H
#define LH_WEB_COLOR_H

#include "css_token_compat.h"
#include "css_types.h"
#include <memory>

namespace litehtml
{
	class document_container;
	struct css_color_data;

	struct web_color
	{
		byte red   = 0;
		byte green = 0;
		byte blue  = 0;
		byte alpha = 255;
		bool is_current_color = false;
		// Float samples and deferred mixes are shared across inherited style values.
		std::shared_ptr<const css_color_data> css_data;

		bool needs_current_color() const;
		web_color resolve(const web_color& current) const;
		web_color to_srgb(const web_color& current = black) const;

		static const web_color transparent;
		static const web_color black;
		static const web_color white;
		static const web_color current_color;

		web_color() {}
		web_color(byte r, byte g, byte b, byte a = 255) : red(r), green(g), blue(b), alpha(a) {}
		web_color(bool is_current_color) : is_current_color(is_current_color) {}

		bool operator==(web_color color) const { return red == color.red && green == color.green && blue == color.blue && alpha == color.alpha; }
		bool operator!=(web_color color) const { return !(*this == color); }

		web_color darken(double fraction) const;
		string to_string() const;
	};

	bool parse_color(const css_token& token, web_color& color, document_container* container);
	bool parse_color_at_depth(const css_token& token, web_color& color, document_container* container, unsigned depth);
}

#endif  // LH_WEB_COLOR_H
