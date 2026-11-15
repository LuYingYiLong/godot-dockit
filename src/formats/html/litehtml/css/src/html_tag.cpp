#include <algorithm>
#include <cstdint>

#include "html.h"
#include "formats/html/css/css_relational_selector.h"
#include "html_tag.h"
#include "document.h"
#include "html_microsyntaxes.h"
#include "iterators.h"
#include "stylesheet.h"
#include "table.h"
#include "line_box.h"
#include "render_item.h"
#include "internal.h"
#include "document_container.h"
#include "el_before_after.h"

namespace litehtml
{

static bool is_generated_content(const element& el)
{
	return is_generated_tag(el.tag());
}

static bool is_selector_tree_element(const element& el)
{
	return el.tag() != empty_id && !is_generated_content(el);
}

static bool matches_sibling_index(int index, int a, int b)
{
	const auto delta = static_cast<int64_t>(index) - b;
	if (delta == 0) return true;
	return a != 0 && (delta > 0) == (a > 0) && delta % a == 0;
}

litehtml::html_tag::html_tag(const std::shared_ptr<document>& doc) : element(doc)
{
	m_tag = empty_id;
	m_id = empty_id;
}

litehtml::html_tag::html_tag(const element::ptr& parent, const string& style) : element(parent->get_document()),
	m_tag(empty_id),
	m_id(empty_id)
{
	litehtml::style st;
	st.add(style);
	add_style(st);
	this->parent(parent);
	compute_styles();
}

bool litehtml::html_tag::appendChild(const element::ptr &el)
{
	if(el)
	{
		el->parent(shared_from_this());
		m_children.push_back(el);
		return true;
	}
	return false;
}

bool litehtml::html_tag::removeChild(const element::ptr &el)
{
	if(el && el->parent() == shared_from_this())
	{
		el->parent(nullptr);
		m_children.erase(std::remove(m_children.begin(), m_children.end(), el), m_children.end());
		return true;
	}
	return false;
}

void litehtml::html_tag::clearRecursive()
{
	for(auto& el : m_children)
	{
		el->clearRecursive();
		el->parent(nullptr);
	}
	m_children.clear();
}

string_id html_tag::id() const
{
	return m_id;
}

string_id html_tag::tag() const
{
	return m_tag;
}

const char* html_tag::get_tagName() const
{
	return _s(m_tag).c_str();
}

void html_tag::set_tagName( const char* tag )
{
	m_tag = _id(lowcase(tag));
}

void html_tag::set_attr( const char* _name, const char* _val )
{
	if (_name && _val)
	{
		// attribute names in attribute selector are matched ASCII case-insensitively regardless of document mode
		string name = lowcase(_name);
		// m_attrs has all attribute values, including class and id, in their original case
		// because in attribute selector values are matched case-sensitively even in quirks mode
		m_attrs[name] = _val;

		if (name == "class")
		{
			string val = _val;
			// class names in class selector (.xxx) are matched ASCII case-insensitively in quirks mode
			if (get_document()->mode() == quirks_mode) lcase(val);
			m_str_classes = split_string(val, whitespace, "", "");
			m_classes.clear();
			for (auto cls : m_str_classes) m_classes.push_back(_id(cls));
		}
		else if (name == "id")
		{
			string val = _val;
			// ids in id selector (#xxx) are matched ASCII case-insensitively in quirks mode
			if (get_document()->mode() == quirks_mode) lcase(val);
			m_id = _id(val);
		}
	}
}

const char* html_tag::get_attr( const char* name, const char* def ) const
{
	auto attr = m_attrs.find(name);
	if(attr != m_attrs.end())
	{
		return attr->second.c_str();
	}
	return def;
}

litehtml::elements_list litehtml::html_tag::select_all(const string& selector )
{
	css_selector sel;
	sel.parse(selector, get_document()->mode());

	return select_all(sel);
}

litehtml::elements_list litehtml::html_tag::select_all(const css_selector& selector )
{
	litehtml::elements_list res;
	select_all(selector, res);
	return res;
}

void litehtml::html_tag::select_all(const css_selector& selector, elements_list& res)
{
	if(select(selector))
	{
		res.push_back(shared_from_this());
	}

	for(auto& el : m_children)
	{
		el->select_all(selector, res);
	}
}


litehtml::element::ptr litehtml::html_tag::select_one( const string& selector )
{
	css_selector sel;
	sel.parse(selector, get_document()->mode());

	return select_one(sel);
}

litehtml::element::ptr litehtml::html_tag::select_one( const css_selector& selector )
{
	if(select(selector))
	{
		return shared_from_this();
	}

	for(auto& el : m_children)
	{
		element::ptr res = el->select_one(selector);
		if(res)
		{
			return res;
		}
	}
	return nullptr;
}

void litehtml::html_tag::apply_stylesheet( const litehtml::css& stylesheet )
{
	for(const auto& sel : stylesheet.selectors())
	{
		// optimization
		{
			const auto& r = sel->m_right;
			if (r.m_tag != star_id && r.m_tag != m_tag)
				continue;

			if (!r.m_attrs.empty())
			{
				const auto& attr = r.m_attrs[0];
				if (attr.type == select_class && !(attr.name in m_classes))
					continue;
			}
		}

		int apply = select(*sel, false);

		if(apply != select_no_match)
		{
			used_selector::ptr us = std::make_unique<used_selector>(sel, false);

			if(sel->is_media_valid())
			{
				auto apply_generated = [&]()
				{
					element::ptr el;
					if (apply & select_match_with_marker)
						el = get_element_marker(true);
					else if (apply & select_match_with_after)
						el = get_element_after(*sel->m_style, true);
					else if (apply & select_match_with_before)
						el = get_element_before(*sel->m_style, true);

					if (el) static_cast<html_tag&>(*el).m_style.combine(*sel->m_style, sel->m_layer);
					us->m_used = true;
				};

				if(apply & select_match_pseudo_class)
				{
					if(select(*sel, true))
					{
						if((apply & select_match_with_generated))
						{
							apply_generated();
						} else
						{
							m_style.combine(*sel->m_style, sel->m_layer);
							us->m_used = true;
						}
					}
				} else if((apply & select_match_with_generated))
				{
					apply_generated();
				} else
				{
					m_style.combine(*sel->m_style, sel->m_layer);
					us->m_used = true;
				}
			}
			m_used_styles.push_back(std::move(us));
		}
	}

	for(auto& el : m_children)
	{
		if(is_selector_tree_element(*el))
		{
			el->apply_stylesheet(stylesheet);
		}
	}
}

void litehtml::html_tag::get_content_size( size& sz, pixel_t max_width )
{
	sz.height	= 0;
	if(m_css.get_display() == display_block)
	{
		sz.width	= max_width;
	} else
	{
		sz.width	= 0;
	}
}

void litehtml::html_tag::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri)
{
	position pos = ri->pos();
	pos.x	+= x;
	pos.y	+= y;

	draw_background(hdc, x, y, clip, ri);
}

bool html_tag::get_custom_property(string_id name, css_token_vector& result) const
{
	for (auto tag = this; tag;)
	{
		bool local = false;
		if (const auto* value = tag->m_style.get_custom_property(name, local))
		{
			result = *value;
			return true;
		}
		if (local) return false;
		tag = dynamic_cast<const html_tag*>(tag->parent().get());
	}
	return false;
}

void html_tag::apply_inline_style()
{
	if (const char* style = get_attr("style"))
		m_style.add(style, "", get_document()->container(), this);
}

void litehtml::html_tag::compute_styles(bool recursive)
{
	document::ptr doc = get_document();
	m_style.subst_vars(this);

	if (const auto* diagnostics = m_style.get_diagnostics())
	{
		for (const auto& diagnostic : *diagnostics)
			doc->container()->report_css_diagnostic(diagnostic, this);
	}
	for (const auto& diagnostic : m_style.get_computed_diagnostics())
		doc->container()->report_css_diagnostic(diagnostic, this);

	m_css.compute(this, doc);
	if (!is_generated_content(*this)) handle_counter_properties();

	if (recursive)
	{
		compute_child_styles();
	}
}

bool litehtml::html_tag::is_white_space() const
{
	return false;
}

int	html_tag::select(const css_selector::vector& selector_list, bool apply_pseudo)
{
	for (auto sel : selector_list)
	{
		if (int result = select(*sel, apply_pseudo))
			return result;
	}
	return select_no_match;
}

int litehtml::html_tag::select(const string& selector)
{
	css_selector sel;
	sel.parse(selector, get_document()->mode());
	return select(sel, true);
}

int litehtml::html_tag::select(const css_selector& selector, bool apply_pseudo)
{
	int right_res = select(selector.m_right, apply_pseudo);
	if(right_res == select_no_match)
	{
		return select_no_match;
	}
	element::ptr el_parent = parent();
	if(selector.m_left)
	{
		if (!el_parent)
		{
			return select_no_match;
		}
		switch(selector.m_combinator)
		{
		case combinator_descendant:
			{
				bool is_pseudo = false;
				element::ptr res = find_ancestor(*selector.m_left, apply_pseudo, &is_pseudo);
				if(!res)
				{
					return select_no_match;
				} else
				{
					if(is_pseudo)
					{
						right_res |= select_match_pseudo_class;
					}
				}
			}
			break;
		case combinator_child:
			{
				int res = el_parent->select(*selector.m_left, apply_pseudo);
				if(res == select_no_match)
				{
					return select_no_match;
				} else
				{
					if(right_res != select_match_pseudo_class)
					{
						right_res |= res;
					}
				}
			}
			break;
		case combinator_adjacent_sibling:
			{
				bool is_pseudo = false;
				element::ptr res = el_parent->find_adjacent_sibling(shared_from_this(), *selector.m_left, apply_pseudo, &is_pseudo);
				if(!res)
				{
					return select_no_match;
				} else
				{
					if(is_pseudo)
					{
						right_res |= select_match_pseudo_class;
					}
				}
			}
			break;
		case combinator_general_sibling:
			{
				bool is_pseudo = false;
				element::ptr res =  el_parent->find_sibling(shared_from_this(), *selector.m_left, apply_pseudo, &is_pseudo);
				if(!res)
				{
					return select_no_match;
				} else
				{
					if(is_pseudo)
					{
						right_res |= select_match_pseudo_class;
					}
				}
			}
			break;
		default:
			right_res = select_no_match;
		}
	}
	return right_res;
}

int litehtml::html_tag::select(const css_element_selector& selector, bool apply_pseudo)
{
	if(selector.m_tag != star_id && selector.m_tag != m_tag)
	{
		return select_no_match;
	}

	int res = select_match;

	for(const auto& attr : selector.m_attrs)
	{
		switch(attr.type)
		{
		case select_class:
			if (!(attr.name in m_classes))
			{
				return select_no_match;
			}
			break;
		case select_id:
			if (attr.name != m_id)
			{
				return select_no_match;
			}
			break;
		case select_pseudo_element:
			if(attr.name == _after_)
			{
				res |= select_match_with_after;
			} else if(attr.name == _before_)
			{
				res |= select_match_with_before;
			} else if(attr.name == _id("marker"))
			{
				res |= select_match_with_marker;
			} else
			{
				return select_no_match;
			}
			break;
		case select_pseudo_class:
			if(apply_pseudo)
			{
				if (select_pseudoclass(attr) == select_no_match)
				{
					return select_no_match;
				}
			} else
			{
				res |= select_match_pseudo_class;
			}
			break;
		default:
			if (select_attribute(attr) == select_no_match)
			{
				return select_no_match;
			}
		}
	}
	return res;
}

int html_tag::select_pseudoclass(const css_attribute_selector& sel)
{
	element::ptr el_parent = parent();

	if (sel.name == _id("scope"))
		return matches_selector_scope(*this) ? select_match : select_no_match;
	if (sel.name == _id("any-link") || sel.name == _link_)
		return is_one_of(m_tag, _a_, _id("area")) && get_attr("href") ? select_match : select_no_match;
	if (sel.name == _id("focus-within"))
	{
		std::vector<const element*> pending{ this };
		while (!pending.empty())
		{
			const auto* node = pending.back();
			pending.pop_back();
			if (const auto* tag = dynamic_cast<const html_tag*>(node); tag && _id("focus") in tag->m_pseudo_classes)
				return select_match;
			for (const auto& child : node->children())
				if (is_selector_tree_element(*child)) pending.push_back(child.get());
		}
		return select_no_match;
	}
	if (sel.name == _id("has"))
		return matches_relative_selector(*this, sel.selector_list) ? select_match : select_no_match;
	if (sel.name == _id("where"))
		return select(sel.selector_list, true);
	if (sel.name == _id("empty"))
	{
		for (const auto& child : m_children)
		{
			if (is_selector_tree_element(*child) || (child->is_text() && !child->is_space()))
				return select_no_match;
		}
		return select_match;
	}

	switch (sel.name)
	{
	case _only_child_:
		if (el_parent && !el_parent->is_only_child(shared_from_this(), false))
		{
			return select_no_match;
		}
		break;
	case _only_of_type_:
		if (el_parent && !el_parent->is_only_child(shared_from_this(), true))
		{
			return select_no_match;
		}
		break;
	case _first_child_:
		if (el_parent && !el_parent->is_nth_child(shared_from_this(), 0, 1, false))
		{
			return select_no_match;
		}
		break;
	case _first_of_type_:
		if (el_parent && !el_parent->is_nth_child(shared_from_this(), 0, 1, true))
		{
			return select_no_match;
		}
		break;
	case _last_child_:
		if (el_parent && !el_parent->is_nth_last_child(shared_from_this(), 0, 1, false))
		{
			return select_no_match;
		}
		break;
	case _last_of_type_:
		if (el_parent && !el_parent->is_nth_last_child(shared_from_this(), 0, 1, true))
		{
			return select_no_match;
		}
		break;
	case _nth_child_:
	case _nth_of_type_:
	case _nth_last_child_:
	case _nth_last_of_type_:
	{
		int num = sel.a;
		int off = sel.b;
		if (!el_parent)
		{
			return matches_sibling_index(1, num, off) && (sel.selector_list.empty() || select(sel.selector_list, true)) ?
				select_match : select_no_match;
		}

		switch (sel.name)
		{
		case _nth_child_:
			if (!el_parent->is_nth_child(shared_from_this(), num, off, false, sel.selector_list))
			{
				return select_no_match;
			}
			break;
		case _nth_of_type_:
			if (!el_parent->is_nth_child(shared_from_this(), num, off, true))
			{
				return select_no_match;
			}
			break;
		case _nth_last_child_:
			if (!el_parent->is_nth_last_child(shared_from_this(), num, off, false, sel.selector_list))
			{
				return select_no_match;
			}
			break;
		case _nth_last_of_type_:
			if (!el_parent->is_nth_last_child(shared_from_this(), num, off, true))
			{
				return select_no_match;
			}
			break;
		default:
			break;
		}

	}
	break;
	case _is_:
		if (!select(sel.selector_list, true))
		{
			return select_no_match;
		}
		break;
	case _not_:
		if (select(sel.selector_list, true))
		{
			return select_no_match;
		}
		break;
	case _lang_:
		if (!matches_element_language(*this, sel.language_ranges))
		{
			return select_no_match;
		}
		break;
	default:
		if (!(sel.name in m_pseudo_classes))
		{
			return select_no_match;
		}
		break;
	}
	return select_match;
}

// https://www.w3.org/TR/selectors-4/#attribute-selectors
int html_tag::select_attribute(const css_attribute_selector& sel)
{
	const char* sz_attr_value = get_attr(_s(sel.name).c_str());

	if (!sz_attr_value) return select_no_match;

	string attr_value = sel.caseless_match ? lowcase(sz_attr_value) : sz_attr_value;

	switch (sel.matcher)
	{
	case attribute_exists:
		return select_match;

	case attribute_equals:
		if (attr_value == sel.value)
		{
			return select_match;
		}
		break;

	case attribute_contains_string: // *=
		if (sel.value != "" && contains(attr_value, sel.value))
		{
			return select_match;
		}
		break;

	// Attribute value is a whitespace-separated list of words, one of which is exactly sel.value
	case attribute_contains_word: // ~=
		if (sel.value != "" && contains(split_string(attr_value), sel.value))
		{
			return select_match;
		}
		break;

	case attribute_starts_with_string: // ^=
		if (sel.value != "" && match(attr_value, 0, sel.value))
		{
			return select_match;
		}
		break;

	// Attribute value is either equals sel.value or begins with sel.value immediately followed by "-".
	case attribute_starts_with_string_hyphen: // |=
		// Note: no special treatment for sel.value == ""
		if (attr_value == sel.value || match(attr_value, 0, sel.value + '-'))
		{
			return select_match;
		}
		break;

	case attribute_ends_with_string: // $=
		if (sel.value != "" && match(attr_value, -(int)sel.value.size(), sel.value))
		{
			return select_match;
		}
		break;
	}
	return select_no_match;
}

element::ptr html_tag::find_ancestor(const css_selector& selector, bool apply_pseudo, bool* is_pseudo)
{
	element::ptr el_parent = parent();
	if (!el_parent)
	{
		return nullptr;
	}
	int res = el_parent->select(selector, apply_pseudo);
	if(res != select_no_match)
	{
		if(is_pseudo)
		{
			if(res & select_match_pseudo_class)
			{
				*is_pseudo = true;
			} else
			{
				*is_pseudo = false;
			}
		}
		return el_parent;
	}
	return el_parent->find_ancestor(selector, apply_pseudo, is_pseudo);
}

void litehtml::html_tag::parse_attributes()
{
	for(auto& el : m_children)
	{
		el->parse_attributes();
	}
}

void litehtml::html_tag::get_text( string& text ) const
{
	for (auto& el : m_children)
	{
		el->get_text(text);
	}
}

bool litehtml::html_tag::is_body()  const
{
	return false;
}

void litehtml::html_tag::set_data( const char* /*data*/ )
{

}

bool litehtml::html_tag::on_mouse_over()
{
	bool ret = false;

	element::ptr el = shared_from_this();
	while(el)
	{
		if(el->set_pseudo_class(_hover_, true))
		{
			ret = true;
		}
		el = el->parent();
	}

	return ret;
}

bool litehtml::html_tag::on_mouse_leave()
{
	bool ret = false;

	element::ptr el = shared_from_this();
	while(el)
	{
		if(el->set_pseudo_class(_hover_, false))
		{
			ret = true;
		}
		if(el->set_pseudo_class(_active_, false))
		{
			ret = true;
		}
		el = el->parent();
	}

	return ret;
}

bool litehtml::html_tag::on_lbutton_down()
{
	bool ret = false;

	element::ptr el = shared_from_this();
	while (el)
	{
		if (el->set_pseudo_class(_active_, true))
		{
			ret = true;
		}
		el = el->parent();
	}

	return ret;
}

bool litehtml::html_tag::on_lbutton_up(bool is_click)
{
	bool ret = false;

	element::ptr el = shared_from_this();
	while (el)
	{
		if (el->set_pseudo_class(_active_, false))
		{
			ret = true;
		}
		el = el->parent();
	}

	if (is_click) on_click();

	return ret;
}

void litehtml::html_tag::on_click()
{
	if (!is_root())
	{
		if(!get_document()->container()->on_element_click(shared_from_this()))
		{
			element::ptr el_parent = parent();
			if (el_parent)
			{
				el_parent->on_click();
			}
		}
	}
}

bool litehtml::html_tag::is_break() const
{
	return false;
}

void litehtml::html_tag::draw_background(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip,
										 const std::shared_ptr<render_item> &ri)
{

	const bool has_border_image = !m_css.get_border_image().source.value.is_empty();
	const auto paint_borders = [&](borders decoration, const position& box, bool root, const position* image_box, bool first, bool last) {
		const bool collapsed = m_css.get_border_collapse() == border_collapse_collapse &&
			is_one_of(m_css.get_display(), display_table, display_inline_table, display_table_cell, display_table_row,
				display_table_row_group, display_table_header_group, display_table_footer_group);
		if (has_border_image && !collapsed)
		{
			const auto& image_border = image_box ? m_css.get_borders() : decoration;
			if (get_document()->container()->draw_border_image(*this, image_border, image_box ? *image_box : box,
				image_box ? &box : nullptr, first, last)) return;
		}
		if (!decoration.is_visible()) return;
		for (auto* side : { &decoration.left, &decoration.right, &decoration.top, &decoration.bottom })
		{
			side->color = side->color.to_srgb(m_css.get_color());
			if (collapsed && side->style == border_style_inset)
				side->style = border_style_ridge;
			else if (collapsed && side->style == border_style_outset)
				side->style = border_style_groove;
		}
		get_document()->container()->draw_borders(hdc, decoration, box, root);
	};
	const auto paint_shadows = [&](position box, borders decoration, bool inset) {
		box.round();
		get_document()->container()->draw_box_shadows(*this, box, decoration, inset);
	};
	if(m_css.get_display() != display_inline && m_css.get_display() != display_table_row)
	{
		position pos = ri->pos();
		pos.x	+= x;
		pos.y	+= y;

		position border_box = pos;
		border_box += ri->get_paddings();
		border_box += ri->get_borders();


		borders shadow_box = m_css.get_borders();
		shadow_box.radius = m_css.get_borders().radius.calc_percents(border_box.width, border_box.height);
		paint_shadows(border_box, shadow_box, false);
		if(has_border_image || border_box.does_intersect(clip) || is_root())
		{
			auto v_offset = ri->get_draw_vertical_offset();
			pos.y += v_offset;
			pos.height -= v_offset;

			const background* bg = get_background();
			if(bg)
			{
				int num_layers = bg->get_layers_number();
				for(int i = num_layers - 1; i >= 0; i--)
				{
					background_layer layer;

					if(!bg->get_layer(i, pos, this, ri, layer)) continue;

					if(is_root() && (clip != nullptr))
					{
						layer.clip_box = *clip;
						layer.border_box = *clip;
					}

					layer.border_box.round();
					layer.clip_box.round();

					bg->draw_layer(hdc, i, layer, get_document()->container());
				}
			}

			paint_shadows(border_box, shadow_box, true);
			borders bdr = m_css.get_borders();
			if(has_border_image || bdr.is_visible())
			{
				border_box.round();
				bdr.radius = m_css.get_borders().radius.calc_percents(border_box.width, border_box.height);
				paint_borders(bdr, border_box, is_root(), nullptr, true, true);
			}
		}
	} else
	{
		const background* bg = get_background();

		position::vector boxes;
		ri->get_inline_boxes(boxes);

		// Slice a single horizontal decoration across an inline's line fragments.
		const bool split_image = has_border_image && m_css.get_display() == display_inline && boxes.size() > 1;
		pixel_t image_width = 0;
		if (split_image)
			for (const auto& fragment : boxes) image_width += fragment.width;
		pixel_t image_offset = 0;
		position content_box;

		for(auto box = boxes.begin(); box != boxes.end(); box++)
		{
			const auto fragment_width = box->width;
			box->x	+= x;
			box->y	+= y;

			if(has_border_image || box->does_intersect(clip))
			{
				content_box = *box;
				content_box -= ri->get_borders();
				content_box -= ri->get_paddings();

				css_borders bdr;

				// set left borders radius for the first box
				if(box == boxes.begin())
				{
					bdr.radius.bottom_left_x	= m_css.get_borders().radius.bottom_left_x;
					bdr.radius.bottom_left_y	= m_css.get_borders().radius.bottom_left_y;
					bdr.radius.top_left_x		= m_css.get_borders().radius.top_left_x;
					bdr.radius.top_left_y		= m_css.get_borders().radius.top_left_y;
				}

				// set right borders radius for the last box
				if(box == boxes.end() - 1)
				{
					bdr.radius.bottom_right_x	= m_css.get_borders().radius.bottom_right_x;
					bdr.radius.bottom_right_y	= m_css.get_borders().radius.bottom_right_y;
					bdr.radius.top_right_x		= m_css.get_borders().radius.top_right_x;
					bdr.radius.top_right_y		= m_css.get_borders().radius.top_right_y;
				}


				bdr.top		= m_css.get_borders().top;
				bdr.bottom	= m_css.get_borders().bottom;
				if(box == boxes.begin())
				{
					bdr.left	= m_css.get_borders().left;
				}
				if(box == boxes.end() - 1)
				{
					bdr.right	= m_css.get_borders().right;
				}


				borders shadow_box = bdr;
				shadow_box.radius = bdr.radius.calc_percents(box->width, box->height);
				paint_shadows(*box, shadow_box, false);
				if(bg)
				{
					int num_layers = bg->get_layers_number();
					for(int i = num_layers - 1; i >= 0; i--)
					{
						background_layer layer;

						if(!bg->get_layer(i, content_box, this, ri, layer)) continue;

						layer.border_radius = bdr.radius.calc_percents(box->width, box->height);

						layer.border_box.round();
						layer.clip_box.round();

						bg->draw_layer(hdc, i, layer, get_document()->container());
					}
				}
				paint_shadows(*box, shadow_box, true);
				if(has_border_image || bdr.is_visible())
				{
					borders b = bdr;
					b.radius = bdr.radius.calc_percents(box->width, box->height);
					box->round();
					position image_box = *box;
					image_box.x -= image_offset;
					image_box.width = image_width;
					paint_borders(b, *box, false, split_image ? &image_box : nullptr,
						box == boxes.begin(), box == boxes.end() - 1);
				}
			}
			image_offset += fragment_width;
		}
	}
}

bool litehtml::html_tag::set_pseudo_class( string_id cls, bool add )
{
	bool ret = false;
	if(add)
	{
		if(std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), cls) == m_pseudo_classes.end())
		{
			m_pseudo_classes.push_back(cls);
			ret = true;
		}
	} else
	{
		auto pi = std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), cls);
		if(pi != m_pseudo_classes.end())
		{
			m_pseudo_classes.erase(pi);
			ret = true;
		}
	}
	return ret;
}

bool litehtml::html_tag::set_class( const char* pclass, bool add )
{
	string_vector classes;
	bool changed = false;

	split_string( pclass, classes, " " );

	if(add)
	{
		for( auto & _class : classes )
		{
			if(std::find(m_str_classes.begin(), m_str_classes.end(), _class) == m_str_classes.end())
			{
				m_str_classes.push_back( std::move( _class ) );
				changed = true;
			}
		}
	} else
	{
		for( const auto & _class : classes )
		{
			auto end = std::remove(m_str_classes.begin(), m_str_classes.end(), _class);

			if(end != m_str_classes.end())
			{
				m_str_classes.erase(end, m_str_classes.end());
				changed = true;
			}
		}
	}

	if( changed )
	{
		string class_string;
		join_string(class_string, m_str_classes, " ");
		set_attr("class", class_string.c_str());

		return true;
	}
	else
	{
		return false;
	}

}

bool litehtml::html_tag::is_replaced() const
{
	return false;
}

bool html_tag::is_nth_child(const element::ptr& el, int num, int off, bool of_type, const css_selector::vector& selector_list) const
{
	int idx = 1;
	for(const auto& child : m_children)
	{
		if(is_selector_tree_element(*child))
		{
			if( (!of_type && selector_list.empty()) ||
				(of_type && child->tag() == el->tag()) || child->select(selector_list) )
			{
				if (el == child) return matches_sibling_index(idx, num, off);
				idx++;
			}
			if(el == child) break;
		}
	}
	return false;
}

bool html_tag::is_nth_last_child(const element::ptr& el, int num, int off, bool of_type, const css_selector::vector& selector_list) const
{
	int idx = 1;
	for(auto child = m_children.rbegin(); child != m_children.rend(); child++)
	{
		if(is_selector_tree_element(**child))
		{
			if( (!of_type && selector_list.empty()) ||
				(of_type && (*child)->tag() == el->tag()) || (*child)->select(selector_list) )
			{
				if (el == (*child)) return matches_sibling_index(idx, num, off);
				idx++;
			}
			if(el == (*child)) break;
		}
	}
	return false;
}

litehtml::element::ptr litehtml::html_tag::find_adjacent_sibling( const element::ptr& el, const css_selector& selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/ )
{
	element::ptr ret;
	for(auto& e : m_children)
	{
		if(is_selector_tree_element(*e))
		{
			if(e == el)
			{
				if(ret)
				{
					int res = ret->select(selector, apply_pseudo);
					if(res != select_no_match)
					{
						if(is_pseudo)
						{
							if(res & select_match_pseudo_class)
							{
								*is_pseudo = true;
							} else
							{
								*is_pseudo = false;
							}
						}
						return ret;
					}
				}
				return nullptr;
			} else
			{
				ret = e;
			}
		}
	}
	return nullptr;
}

litehtml::element::ptr litehtml::html_tag::find_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo /*= true*/, bool* is_pseudo /*= 0*/)
{
	element::ptr ret = nullptr;
	for(auto& e : m_children)
	{
		if(is_selector_tree_element(*e))
		{
			if(e == el)
			{
				return ret;
			} else if(!ret)
			{
				int res = e->select(selector, apply_pseudo);
				if(res != select_no_match)
				{
					if(is_pseudo)
					{
						if(res & select_match_pseudo_class)
						{
							*is_pseudo = true;
						} else
						{
							*is_pseudo = false;
						}
					}
					ret = e;
				}
			}
		}
	}
	return nullptr;
}

bool litehtml::html_tag::is_only_child(const element::ptr& el, bool of_type) const
{
	int child_count = 0;
	for(const auto& child : m_children)
	{
		if(is_selector_tree_element(*child))
		{
			if( !of_type || (of_type && el->tag() == child->tag()) )
			{
				child_count++;
			}
			if(child_count > 1) break;
		}
	}
	if(child_count > 1)
	{
		return false;
	}
	return true;
}

element::ptr html_tag::get_element_marker(bool create)
{
	if (!m_children.empty() && m_children.front()->tag() == marker_tag())
		return m_children.front();
	if (!create) return nullptr;
	auto element = std::make_shared<el_marker>(get_document());
	element->parent(shared_from_this());
	m_children.push_front(element);
	return element;
}

element::ptr html_tag::get_element_before(const style&, bool create)
{
	auto where = m_children.begin();
	if (where != m_children.end() && (*where)->tag() == marker_tag()) ++where;
	if (where != m_children.end() && (*where)->tag() == __tag_before_) return *where;
	if (!create) return nullptr;
	auto element = std::make_shared<el_before>(get_document());
	element->parent(shared_from_this());
	m_children.insert(where, element);
	return element;
}

litehtml::element::ptr litehtml::html_tag::get_element_after(const style& style, bool create)
{
	if(!m_children.empty())
	{
		if( m_children.back()->tag() == __tag_after_ )
		{
			return m_children.back();
		}
	}
	if(create)
	{
		return add_pseudo_after(style);
	}
	return nullptr;
}


void litehtml::html_tag::compute_child_styles()
{
	if (css().get_display() == display_list_item)
		get_element_marker(true);
	const element* preceding = nullptr;
	for (const auto& child : m_children)
	{
		child->inherit_counters(preceding);
		child->compute_styles();
		if (child->tag() != empty_id && child->css().get_display() != display_none)
			preceding = child.get();
	}
}

void litehtml::html_tag::handle_counter_properties()
{
	if (css().get_display() == display_none) return;
	for (auto ancestor = parent(); ancestor; ancestor = ancestor->parent())
	{
		if (ancestor->css().get_display() == display_none) return;
	}
	static const auto set = _id("counter-set");
	static const auto list_item = _id("list-item");
	static const css_counter_changes empty;
	const auto declarations = [&](string_id property) -> const css_counter_changes& {
		for (const auto* current = this; current;)
		{
			const auto& value = current->m_style.get_property(property);
			if (value.is<css_counter_changes>()) return value.get<css_counter_changes>();
			if (!value.is<inherit>()) break;
			current = dynamic_cast<const html_tag*>(current->parent().get());
		}
		return empty;
	};
	const auto& resets = declarations(_counter_reset_);
	const auto& increments = declarations(_counter_increment_);
	const auto& sets = declarations(set);
	const auto mentions_list_item = [&](const css_counter_change& change) { return change.name == list_item; };

	if (is_one_of(tag(), _ol_, _ul_, _id("menu"), _dir_) && !std::any_of(resets.begin(), resets.end(), mentions_list_item))
	{
		const bool reversed = tag() == _ol_ && get_attr("reversed");
		int start = 1;
		const char* start_attribute = tag() == _ol_ ? get_attr("start") : nullptr;
		if (reversed && (!start_attribute || !html_parse_integer(start_attribute, start)))
		{
			start = static_cast<int>(std::count_if(m_children.begin(), m_children.end(), [](const element::ptr& child) {
				return child->tag() == _li_;
			}));
		}
		else if (start_attribute)
			html_parse_integer(start_attribute, start);
		reset_list_counter(list_item, start, reversed);
	}
	for (const auto& change : resets) reset_counter(change.name, change.value);
	for (const auto& change : increments) increment_counter(change.name, change.value);

	if (css().get_display() == display_list_item)
	{
		if (!std::any_of(increments.begin(), increments.end(), mentions_list_item))
			increment_list_counter(list_item);
		if (!std::any_of(sets.begin(), sets.end(), mentions_list_item))
		{
			int value;
			const char* attribute = tag() == _li_ ? get_attr("value") : nullptr;
			if (attribute && html_parse_integer(attribute, value)) set_counter(list_item, value);
		}
	}
	for (const auto& change : sets) set_counter(change.name, change.value);
}

void litehtml::html_tag::add_style(const style& style)
{
	m_style.combine(style);
}

void litehtml::html_tag::refresh_styles()
{
	for (auto& el : m_children)
	{
		if(el->css().get_display() != display_inline_text)
		{
			el->refresh_styles();
		}
	}

	m_style.clear();

	for (auto& usel : m_used_styles)
	{
		usel->m_used = false;

		if(usel->m_selector->is_media_valid())
		{
			int apply = select(*usel->m_selector, false);

			if (apply == select_no_match || ((apply & select_match_pseudo_class) && !select(*usel->m_selector, true)))
				continue;
			const auto& selector = *usel->m_selector;
			element::ptr generated;
			if (apply & select_match_with_marker)
				generated = get_element_marker(false);
			else if (apply & select_match_with_after)
				generated = get_element_after(*selector.m_style, false);
			else if (apply & select_match_with_before)
				generated = get_element_before(*selector.m_style, false);
			if (generated)
				static_cast<html_tag&>(*generated).m_style.combine(*selector.m_style, selector.m_layer);
			else if (!(apply & select_match_with_generated))
				m_style.combine(*selector.m_style, selector.m_layer);
			usel->m_used = true;
		}
	}
	apply_inline_style();
}

const litehtml::background* litehtml::html_tag::get_background(bool own_only)
{
	if(own_only)
	{
		// return own background with check for empty one
		if(m_css.get_bg().is_empty())
		{
			return nullptr;
		}
		return &m_css.get_bg();
	}

	if(m_css.get_bg().is_empty())
	{
		// if this is root element (<html>) try to get background from body
		if (is_root())
		{
			for (const auto& el : m_children)
			{
				if( el->is_body() )
				{
					// return own body background
					return el->get_background(true);
				}
			}
		}
		return nullptr;
	}

	if(is_body())
	{
		element::ptr el_parent = parent();
		if (el_parent)
		{
			if (!el_parent->get_background(true))
			{
				// parent of body will draw background for body
				return nullptr;
			}
		}
	}

	return &m_css.get_bg();
}

string html_tag::dump_get_name()
{
	if(m_tag == empty_id)
	{
		return "anon [html_tag]";
	}
	return _s(m_tag) + " [html_tag]";
}

// https://html.spec.whatwg.org/multipage/rendering.html#maps-to-the-pixel-length-property
void html_tag::map_to_pixel_length_property(string_id prop_name, string attr_value)
{
	int n;
	if (html_parse_non_negative_integer(attr_value, n))
	{
		css_token tok(DIMENSION, (float)n, css_number_integer, "px");
		m_style.add_property(prop_name, {tok});
	}
}

// https://html.spec.whatwg.org/multipage/rendering.html#tables-2:attr-table-border
void html_tag::map_to_pixel_length_property_with_default_value(string_id prop_name, string attr_value, int default_value)
{
	int n = default_value;
	html_parse_non_negative_integer(attr_value, n);
	css_token tok(DIMENSION, (float)n, css_number_integer, "px");
	m_style.add_property(prop_name, {tok});
}

// https://html.spec.whatwg.org/multipage/rendering.html#maps-to-the-dimension-property
void html_tag::map_to_dimension_property(string_id prop_name, string attr_value)
{
	float x = 0;
	html_dimension_type type = html_length;
	if (!html_parse_dimension_value(attr_value, x, type))
		return;

	css_token tok;
	if (type == html_length)
		tok = {DIMENSION,  x, css_number_number, "px"};
	else
		tok = {PERCENTAGE, x, css_number_number};

	m_style.add_property(prop_name, {tok});
}

// https://html.spec.whatwg.org/multipage/rendering.html#maps-to-the-dimension-property-(ignoring-zero)
void html_tag::map_to_dimension_property_ignoring_zero(string_id prop_name, string attr_value)
{
	float x = 0;
	html_dimension_type type = html_length;
	if (!html_parse_nonzero_dimension_value(attr_value, x, type))
		return;

	css_token tok;
	if (type == html_length)
		tok = {DIMENSION,  x, css_number_number, "px"};
	else
		tok = {PERCENTAGE, x, css_number_number};

	m_style.add_property(prop_name, {tok});
}

} // namespace litehtml
