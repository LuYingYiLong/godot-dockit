#ifndef LITEHTML_CSS_PROPERTIES_H
#define LITEHTML_CSS_PROPERTIES_H

#include "types.h"
#include "formats/html/css/css_computed_style.h"

namespace litehtml
{
	class html_tag;
	class style;

	using docrender::css::css_line_height_t;

	class css_properties : public docrender::css::ComputedStyle
	{
		bool viewport_overflow_source = false;
		overflow used_overflow(overflow value) const {
			if (viewport_overflow_source) value = overflow_visible;
			return value == overflow_visible && has_paint_containment() ? overflow_clip : value;
		}

	public:
		// Propagation changes the used overflow of the source box and leaves inheritance intact.
		void propagate_overflow_to_viewport() { viewport_overflow_source = true; }
		overflow get_overflow_x() const { return used_overflow(ComputedStyle::get_overflow_x()); }
		overflow get_overflow_y() const { return used_overflow(ComputedStyle::get_overflow_y()); }
		bool clips_overflow() const { return get_overflow_x() != overflow_visible || get_overflow_y() != overflow_visible; }
		bool is_scroll_container() const { return !viewport_overflow_source && ComputedStyle::is_scroll_container(); }
		void compute(const html_tag* element, const std::shared_ptr<document>& doc);
		void compute_placeholder(const html_tag& element, const style& declarations, document& doc);
	};
}

#endif // LITEHTML_CSS_PROPERTIES_H
