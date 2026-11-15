#ifndef LH_STYLESHEET_H
#define LH_STYLESHEET_H

#include "css_selector.h"
#include "formats/html/css/css_parser.h"
#include "formats/html/css/css_font_face.h"

namespace docrender::css { struct StylesheetImport; }

namespace litehtml
{

using docrender::css::raw_rule;
using docrender::css::raw_declaration;

class css
{
	css_selector::vector m_selectors;
	struct layer
	{
		std::vector<size_t> children;
		std::map<string, size_t> named_children;
		int rank = css_unlayered;
	};
	std::vector<layer> m_layers{ layer{} };
	std::map<string, std::pair<int, css_counter_style>> m_counter_styles;
	struct counter_style_rule
	{
		string name;
		css_counter_style style;
		size_t layer_id;
	};
	std::vector<counter_style_rule> m_counter_style_rules;
	struct font_face_rule
	{
		css_font_face face;
		size_t layer_id;
	};
	std::vector<font_face_rule> m_font_face_rules;
	std::vector<css_font_face> m_font_faces;
	struct font_feature_values_rule
	{
		css_font_feature_values_rule values;
		size_t layer_id;
	};
	std::vector<font_feature_values_rule> m_font_feature_values_rules;
	std::vector<css_font_feature_values_rule> m_font_feature_values;
public:

	const css_selector::vector& selectors() const
	{
		return m_selectors;
	}

	template<class Input>
	void	parse_css_stylesheet(const Input& input, string baseurl, shared_ptr<document> doc, media_query_list_list::ptr media = nullptr, bool top_level = true, size_t layer_id = 0);

	void	sort_selectors(bool author = true);
	const css_counter_style* find_counter_style(const string& name) const
	{
		const auto found = m_counter_styles.find(name);
		return found == m_counter_styles.end() ? nullptr : &found->second.second;
	}
	const std::vector<css_font_face>& font_faces() const { return m_font_faces; }
	const std::vector<css_font_feature_values_rule>& font_feature_values() const { return m_font_feature_values; }
	bool	has_layers() const { return m_layers.size() > 1; }

private:
	bool	parse_style_rule(const raw_rule& rule, const string& baseurl, shared_ptr<document> doc, media_query_list_list::ptr media, size_t layer_id);
	bool	apply_import(const docrender::css::StylesheetImport& rule, const string& baseurl, shared_ptr<document> doc, media_query_list_list::ptr media, size_t layer_id);
	size_t register_layer(size_t parent, const string_vector& names);
	void	add_selector(const css_selector::ptr& selector);
};

inline void css::add_selector(const css_selector::ptr& selector)
{
	selector->m_order = (int)m_selectors.size();
	m_selectors.push_back(selector);
}


} // namespace litehtml

#endif  // LH_STYLESHEET_H
