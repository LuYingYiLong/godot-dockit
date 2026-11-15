#include <limits>

#include "html.h"
#include "element.h"
#include "document.h"
#include "render_item.h"
#include "formats/html/layout/grid/litehtml_render_grid.h"
#include "render_flex.h"
#include "render_inline.h"
#include "render_table.h"
#include "el_before_after.h"

namespace litehtml
{

#define LITEHTML_EMPTY_FUNC			{}
#define LITEHTML_RETURN_FUNC(ret)	{return ret;}

element::element(const document::ptr& doc) : m_doc(doc)
{
}

position element::get_placement() const
{
	position pos;
	bool is_first = true;
	for(const auto& ri_el : m_renders)
	{
		auto ri = ri_el.lock();
		if(ri)
		{
			position ri_pos = ri->get_placement();
			if(is_first)
			{
				is_first = false;
				pos = ri_pos;
			} else
			{
				if(pos.x < ri_pos.x)
				{
					pos.x = ri_pos.x;
				}
				if(pos.y < ri_pos.y)
				{
					pos.y = ri_pos.y;
				}
			}
		}
	}
	return pos;
}

bool element::is_inline() const
{
	if(	css().get_display() == display_inline ||
		   css().get_display() == display_inline_table ||
		   css().get_display() == display_inline_block ||
		   css().get_display() == display_inline_text ||
		   css().get_display() == display_inline_flex ||
		   css().get_display() == display_inline_grid)
	{
		return true;
	}
	return false;
}

bool element::is_inline_box() const
{
	if(	css().get_display() == display_inline_table ||
		   css().get_display() == display_inline_block ||
		   css().get_display() == display_inline_flex ||
		   css().get_display() == display_inline_grid)
	{
		return true;
	}
	return false;
}

bool element::is_ancestor(const ptr &el) const
{
	element::ptr el_parent = parent();
	while(el_parent && el_parent != el)
	{
		el_parent = el_parent->parent();
	}
	if(el_parent)
	{
		return true;
	}
	return false;
}

bool element::is_table_skip() const
{
	return is_space() || is_comment() || css().get_display() == display_none;
}

string element::dump_get_name()
{
	return "element";
}

std::vector<std::tuple<string, string>> element::dump_get_attrs()
{
	return m_css.dump_get_attrs();
}

void element::dump(dumper& cout)
{
	cout.begin_node(dump_get_name());

	auto attrs = dump_get_attrs();
	if(!attrs.empty())
	{
		cout.begin_attrs_group("attributes");
		for (const auto &attr: attrs)
		{
			cout.add_attr(std::get<0>(attr), std::get<1>(attr));
		}
		cout.end_attrs_group();
	}

	if(!m_children.empty())
	{
		cout.begin_attrs_group("children");
		for (const auto &el: m_children)
		{
			el->dump(cout);
		}
		cout.end_attrs_group();
	}

	cout.end_node();
}

std::shared_ptr<render_item> element::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	std::shared_ptr<render_item> ret;

	if(css().get_display() == display_table_column ||
	   css().get_display() == display_table_column_group ||
	   css().get_display() == display_table_footer_group ||
	   css().get_display() == display_table_header_group ||
	   css().get_display() == display_table_row_group)
	{
		ret = std::make_shared<render_item_table_part>(shared_from_this());
	} else if(css().get_display() == display_table_row)
	{
		ret = std::make_shared<render_item_table_row>(shared_from_this());
	} else if(css().get_display() == display_block ||
				css().get_display() == display_table_cell ||
				css().get_display() == display_table_caption ||
				css().get_display() == display_list_item ||
				css().get_display() == display_inline_block)
	{
		ret = std::make_shared<render_item_block>(shared_from_this());
	} else if(css().get_display() == display_table || css().get_display() == display_inline_table)
	{
		ret = std::make_shared<render_item_table>(shared_from_this());
	} else if(css().get_display() == display_inline || css().get_display() == display_inline_text)
	{
		ret = std::make_shared<render_item_inline>(shared_from_this());
	} else if(css().get_display() == display_flex || css().get_display() == display_inline_flex)
	{
		ret = std::make_shared<render_item_flex>(shared_from_this());
	}
	if (is_grid_display(css().get_display()))
		ret = std::make_shared<render_item_grid>(shared_from_this());
	if(ret)
	{
		if (css().get_display() == display_table ||
			css().get_display() == display_inline_table ||
			css().get_display() == display_table_caption ||
			css().get_display() == display_table_cell ||
			css().get_display() == display_table_column ||
			css().get_display() == display_table_column_group ||
			css().get_display() == display_table_footer_group ||
			css().get_display() == display_table_header_group ||
			css().get_display() == display_table_row ||
			css().get_display() == display_table_row_group)
		{
			get_document()->add_tabular(ret);
		}

		ret->parent(parent_ri);
		for(const auto& el : m_children)
		{
			auto ri = el->create_render_item(ret);
			if(ri)
			{
				ret->add_child(ri);
			}
		}
	}
	return ret;
}

bool element::requires_styles_update()
{
	for (const auto& used_style : m_used_styles)
	{
		if(used_style->m_selector->is_media_valid())
		{
			int res = select(*(used_style->m_selector), true);
			if( (res == select_no_match && used_style->m_used) || (res == select_match && !used_style->m_used) )
			{
				return true;
			}
		}
	}
	return false;
}

void element::add_render(const std::shared_ptr<render_item>& ri)
{
	m_renders.push_back(ri);
}

bool element::find_styles_changes( position::vector& redraw_boxes)
{
	if(css().get_display() == display_inline_text)
	{
		return false;
	}

	bool ret = false;

	if(requires_styles_update())
	{
		auto fetch_boxes = [&](const std::shared_ptr<element>& el)
			{
				for(const auto& weak_ri : el->m_renders)
				{
					auto ri = weak_ri.lock();
					if(ri)
					{
						position::vector boxes;
						ri->get_rendering_boxes(boxes);
						for (auto &box: boxes)
						{
							redraw_boxes.push_back(box);
						}
					}
				}
			};
		fetch_boxes(shared_from_this());
		for (auto& el : m_children)
		{
			fetch_boxes(el);
		}

		refresh_styles();
		compute_styles();
		ret = true;
	}
	for (auto& el : m_children)
	{
		if(el->find_styles_changes(redraw_boxes))
		{
			ret = true;
		}
	}
	return ret;
}

element::ptr element::_add_before_after(int type, const style& /*style*/)
{
	element::ptr el;
	if(type == 0)
	{
		el = std::make_shared<el_before>(get_document());
		m_children.insert(m_children.begin(), el);
	} else
	{
		el = std::make_shared<el_after>(get_document());
		m_children.insert(m_children.end(), el);
	}
	el->parent(shared_from_this());
	return el;
}

bool element::is_block_formatting_context() const
{
	if(m_css.get_display() == display_block)
	{
		auto par = parent();
		if(par && (par->css().get_display() == display_inline_flex || par->css().get_display() == display_flex))
		{
			return true;
		}
	}
	if (is_grid_display(m_css.get_display()))
		return true;
	if (const auto p = parent(); p && is_grid_display(p->css().get_display()) && m_css.get_display() != display_inline_text)
		return true;
	if(	m_css.get_display() == display_inline_block ||
		   m_css.get_display() == display_table_cell ||
		   m_css.get_display() == display_inline_flex ||
		   m_css.get_display() == display_flex ||
		   m_css.get_display() == display_table_caption ||
		   is_root() ||
		   m_css.get_float() != float_none ||
		   m_css.get_position() == element_position_absolute ||
		   m_css.get_position() == element_position_fixed ||
		   m_css.get_overflow() > overflow_visible)
	{
		return true;
	}
	return false;
}

void element::inherit_counters(const element* preceding_sibling)
{
	const auto ancestor = parent();
	const auto* source = preceding_sibling ? preceding_sibling : ancestor.get();
	if (source)
		m_counter_values = source->m_counter_values;
	else
		m_counter_values.clear();
}

element::scoped_counter::state& element::get_counter(string_id name)
{
	auto& stack = m_counter_values[name];
	if (stack.empty())
		stack.push_back({ parent().get(), std::make_shared<scoped_counter::state>(scoped_counter::state{ 0, false }) });
	return *stack.back().counter;
}

int element::get_counter_value(string_id name)
{
	return static_cast<int>(std::clamp<int64_t>(get_counter(name).value,
		std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
}

std::vector<int> element::get_counter_values(string_id name)
{
	get_counter(name);
	std::vector<int> values;
	for (const auto& counter : m_counter_values.at(name))
		values.push_back(static_cast<int>(std::clamp<int64_t>(counter.counter->value,
			std::numeric_limits<int>::min(), std::numeric_limits<int>::max())));
	return values;
}

void element::increment_counter(string_id name, int increment)
{
	auto& value = get_counter(name).value;
	const auto sum = static_cast<int64_t>(value) + increment;
	value = static_cast<int>(std::clamp<int64_t>(sum, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
}

void element::set_counter(string_id name, int value)
{
	get_counter(name).value = value;
}

void element::reset_counter(string_id name, int value)
{
	reset_counter_state(name, value, false);
}

void element::reset_list_counter(string_id name, int start, bool reversed)
{
	reset_counter_state(name, static_cast<int64_t>(start) + (reversed ? 1 : -1), reversed);
}

void element::increment_list_counter(string_id name)
{
	increment_counter(name, get_counter(name).reversed ? -1 : 1);
}

void element::reset_counter_state(string_id name, int64_t value, bool reversed)
{
	auto& stack = m_counter_values[name];
	const auto scope = parent();
	scoped_counter counter{ scope.get(), std::make_shared<scoped_counter::state>(scoped_counter::state{ value, reversed }) };
	// Reset replaces an earlier counter at this sibling level and nests under ancestor counters.
	if (!stack.empty() && stack.back().scope_parent == scope.get())
		stack.back() = std::move(counter);
	else
		stack.push_back(std::move(counter));
}

pixel_t litehtml::element::v_scroll(pixel_t dy) const
{
	if(m_renders.empty())
		return 0;
	auto ri_el = m_renders.front().lock();
	if(!ri_el)
		return 0;
	return ri_el->v_scroll(dy);
}

pixel_t litehtml::element::h_scroll(pixel_t dx) const
{
	if(m_renders.empty())
		return 0;
	auto ri_el = m_renders.front().lock();
	if(!ri_el)
		return 0;
	return ri_el->h_scroll(dx);
}

void litehtml::element::run_on_renderers(const std::function<bool(const std::shared_ptr<render_item>&)>& func)
{
	for(const auto& weak_ri : m_renders)
	{
		auto ri = weak_ri.lock();
		if(ri)
		{
			if(!func(ri))
			{
				break;
			}
		}
	}
}

const background* element::get_background(bool /*own_only*/)						LITEHTML_RETURN_FUNC(nullptr)
void element::add_style( const style& /*style*/)									LITEHTML_EMPTY_FUNC
void element::select_all(const css_selector& /*selector*/, elements_list& /*res*/)	LITEHTML_EMPTY_FUNC
elements_list element::select_all(const css_selector& /*selector*/)					LITEHTML_RETURN_FUNC(elements_list())
elements_list element::select_all(const string& /*selector*/)						LITEHTML_RETURN_FUNC(elements_list())
element::ptr element::select_one( const css_selector& /*selector*/ )				LITEHTML_RETURN_FUNC(nullptr)
element::ptr element::select_one( const string& /*selector*/ )						LITEHTML_RETURN_FUNC(nullptr)
element::ptr element::find_adjacent_sibling(const element::ptr& /*el*/, const css_selector& /*selector*/, bool /*apply_pseudo*/ /*= true*/, bool* /*is_pseudo*/ /*= 0*/) LITEHTML_RETURN_FUNC(nullptr)
element::ptr element::find_sibling(const element::ptr& /*el*/, const css_selector& /*selector*/, bool /*apply_pseudo*/ /*= true*/, bool* /*is_pseudo*/ /*= 0*/) LITEHTML_RETURN_FUNC(nullptr)
bool element::is_nth_last_child(const element::ptr& /*el*/, int /*num*/, int /*off*/, bool /*of_type*/, const css_selector::vector& /*selector_list*/) const LITEHTML_RETURN_FUNC(false)
bool element::is_nth_child(const element::ptr&, int /*num*/, int /*off*/, bool /*of_type*/, const css_selector::vector& /*selector_list*/) const LITEHTML_RETURN_FUNC(false)
bool element::is_only_child(const element::ptr& /*el*/, bool /*of_type*/) const		LITEHTML_RETURN_FUNC(false)
void element::get_content_size( size& /*sz*/, pixel_t /*max_width*/ )				LITEHTML_EMPTY_FUNC
bool element::appendChild(const ptr &/*el*/)										LITEHTML_RETURN_FUNC(false)
bool element::removeChild(const ptr &/*el*/)										LITEHTML_RETURN_FUNC(false)
void element::clearRecursive()														LITEHTML_EMPTY_FUNC
string_id element::id() const														LITEHTML_RETURN_FUNC(empty_id)
string_id element::tag() const														LITEHTML_RETURN_FUNC(empty_id)
const char* element::get_tagName() const											LITEHTML_RETURN_FUNC("")
void element::set_tagName( const char* /*tag*/ )									LITEHTML_EMPTY_FUNC
void element::set_data( const char* /*data*/ )										LITEHTML_EMPTY_FUNC
void element::set_attr( const char* /*name*/, const char* /*val*/ )					LITEHTML_EMPTY_FUNC
void element::apply_stylesheet( const litehtml::css& /*stylesheet*/ )				LITEHTML_EMPTY_FUNC
void element::refresh_styles()														LITEHTML_EMPTY_FUNC
void element::on_click()															LITEHTML_EMPTY_FUNC
void element::compute_styles( bool /*recursive*/ )									LITEHTML_EMPTY_FUNC
const char* element::get_attr( const char* /*name*/, const char* def /*= 0*/ ) const LITEHTML_RETURN_FUNC(def)
bool element::is_white_space() const												LITEHTML_RETURN_FUNC(false)
bool element::is_space() const														LITEHTML_RETURN_FUNC(false)
bool element::is_comment() const													LITEHTML_RETURN_FUNC(false)
bool element::is_body() const														LITEHTML_RETURN_FUNC(false)
bool element::is_break() const														LITEHTML_RETURN_FUNC(false)
bool element::is_text() const														LITEHTML_RETURN_FUNC(false)

bool element::on_mouse_over()														LITEHTML_RETURN_FUNC(false)
bool element::on_mouse_leave()														LITEHTML_RETURN_FUNC(false)
bool element::on_lbutton_down()														LITEHTML_RETURN_FUNC(false)
bool element::on_lbutton_up(const bool /*is_click*/)								LITEHTML_RETURN_FUNC(false)
bool element::set_pseudo_class( string_id /*cls*/, bool /*add*/ )					LITEHTML_RETURN_FUNC(false)
bool element::set_class( const char* /*pclass*/, bool /*add*/ )						LITEHTML_RETURN_FUNC(false)
bool element::is_replaced() const													LITEHTML_RETURN_FUNC(false)
void element::draw(uint_ptr /*hdc*/, pixel_t /*x*/, pixel_t /*y*/, const position */*clip*/, const std::shared_ptr<render_item> &/*ri*/) LITEHTML_EMPTY_FUNC
void element::draw_background(uint_ptr /*hdc*/, pixel_t /*x*/, pixel_t /*y*/, const position */*clip*/, const std::shared_ptr<render_item> &/*ri*/) LITEHTML_EMPTY_FUNC
void element::get_text( string& /*text*/ ) const									LITEHTML_EMPTY_FUNC
void element::parse_attributes()													LITEHTML_EMPTY_FUNC
int	element::select(const css_selector::vector& /*selector_list*/, bool /*apply_pseudo*/) LITEHTML_RETURN_FUNC(select_no_match)
int element::select(const string& /*selector*/)										LITEHTML_RETURN_FUNC(select_no_match)
int element::select(const css_selector& /*selector*/, bool /*apply_pseudo*/)		LITEHTML_RETURN_FUNC(select_no_match)
int element::select(const css_element_selector& /*selector*/, bool /*apply_pseudo*/) LITEHTML_RETURN_FUNC(select_no_match)
element::ptr element::find_ancestor(const css_selector& /*selector*/, bool /*apply_pseudo*/, bool* /*is_pseudo*/) LITEHTML_RETURN_FUNC(nullptr)

} // namespace litehtml
