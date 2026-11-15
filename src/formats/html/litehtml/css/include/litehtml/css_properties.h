#ifndef LITEHTML_CSS_PROPERTIES_H
#define LITEHTML_CSS_PROPERTIES_H

#include "types.h"
#include "formats/html/css/css_computed_style.h"

namespace litehtml
{
	class html_tag;

	using docrender::css::css_line_height_t;

	class css_properties : public docrender::css::ComputedStyle
	{
	public:
		void compute(const html_tag* element, const std::shared_ptr<document>& doc);
	};
}

#endif // LITEHTML_CSS_PROPERTIES_H
