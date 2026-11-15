#include "el_image.h"
#include "formats/html/css/css_image_value.h"
#include "render_image.h"
#include "document_container.h"
#include "html_microsyntaxes.h"

litehtml::el_image::el_image(const document::ptr& doc, const string& baseurl, bool decorative) :
	html_tag(doc), m_baseurl(baseurl), m_decorative(decorative)
{
	m_css.set_display(display_inline_block);
}

litehtml::css_image_orientation litehtml::el_image::image_orientation() const
{
	const auto orientation = css().get_image_orientation();
	return m_decorative ? decorative_image_orientation(orientation) : orientation;
}

void litehtml::el_image::get_content_size( size& sz, pixel_t /*max_width*/ )
{
	auto* container = get_document()->container();
	const double density = resolve_image_dppx(m_src, m_baseurl, *container, css().get_image_resolution());
	container->get_css_image_size(m_src.c_str(), m_baseurl.c_str(), density, sz, image_orientation());
}

bool litehtml::el_image::is_replaced() const
{
	return true;
}

void litehtml::el_image::parse_attributes()
{
	m_src = get_attr("src", "");

	// https://html.spec.whatwg.org/multipage/rendering.html#attributes-for-embedded-content-and-images:the-img-element-5
	const char* width = get_attr("width");
	const char* height = get_attr("height");
	if (width) map_to_dimension_property(_width_, width);
	if (height) map_to_dimension_property(_height_, height);

	float w = 0;
	float h = 0;
	html_dimension_type w_type;
	html_dimension_type h_type;
	if (width && height && html_parse_dimension_value(width, w, w_type) &&
		html_parse_dimension_value(height, h, h_type) && w_type == html_length && h_type == html_length)
	{
		m_style.add_property(_aspect_ratio_, css_token_vector{
			{ IDENT, "auto" }, { NUMBER, w, css_number_number }, { static_cast<css_token_type>('/') }, { NUMBER, h, css_number_number },
		});
	}
}

namespace
{
	litehtml::size fit_object(litehtml::size box, litehtml::size natural, litehtml::object_fit fit)
	{
		using litehtml::object_fit;
		if (fit == object_fit::fill || natural.width <= 0 || natural.height <= 0) return box;
		if (fit == object_fit::none) return natural;

		const auto horizontal = box.width / natural.width;
		const auto vertical = box.height / natural.height;
		const bool cover = fit == object_fit::cover || fit == object_fit::cover_scale_down;
		auto scale = cover ? std::max(horizontal, vertical) : std::min(horizontal, vertical);
		if (fit == object_fit::scale_down || fit == object_fit::cover_scale_down)
			scale = std::min(scale, 1.0f);
		return { natural.width * scale, natural.height * scale };
	}
}

void litehtml::el_image::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri)
{
	html_tag::draw(hdc, x, y, clip, ri);
	position pos = ri->pos();
	pos.x += x;
	pos.y += y;
	pos.round();

	// Reuse the image crop path with the element content box as its clip.
	if(pos.does_intersect(clip))
	{
		if (pos.width > 0 && pos.height > 0)
		{
			background_layer layer;
			layer.image_orientation = image_orientation();
			layer.clip_box = pos;
			layer.origin_box = pos;
			const auto fit = css().get_object_fit();
			if (fit != object_fit::fill)
			{
				size natural;
				get_content_size(natural, pos.width);
				const auto extent = fit_object({pos.width, pos.height}, natural, fit);
				layer.origin_box.width = extent.width;
				layer.origin_box.height = extent.height;
			}
			const auto& object_position = css().get_object_position();
			layer.origin_box.x += object_position.x.resolve(pos.width - layer.origin_box.width);
			layer.origin_box.y += object_position.y.resolve(pos.height - layer.origin_box.height);
			layer.border_box = pos;
			layer.border_box += ri->get_paddings();
			layer.border_box += ri->get_borders();
			layer.repeat = { background_repeat::no_repeat, background_repeat::no_repeat };
			layer.border_radius = css().get_borders().radius.calc_percents(layer.border_box.width, layer.border_box.height);
			get_document()->container()->draw_image(hdc, layer, m_src, m_baseurl);
		}
	}
}

void litehtml::el_image::compute_styles(bool recursive)
{
	html_tag::compute_styles(recursive);

	if(!m_src.empty())
	{
		if(!css().get_height().is_predefined() && !css().get_width().is_predefined())
		{
			get_document()->container()->load_image(m_src.c_str(), m_baseurl.c_str(), true, image_orientation());
		} else
		{
			get_document()->container()->load_image(m_src.c_str(), m_baseurl.c_str(), false, image_orientation());
		}
	}
}

litehtml::string litehtml::el_image::dump_get_name()
{
	return "img src=\"" + m_src + "\"";
}

std::shared_ptr<litehtml::render_item> litehtml::el_image::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	auto ret = std::make_shared<render_item_image>(shared_from_this());
	ret->parent(parent_ri);
	return ret;
}