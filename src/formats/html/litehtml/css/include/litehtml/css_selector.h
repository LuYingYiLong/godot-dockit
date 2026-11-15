#ifndef LH_CSS_SELECTOR_H
#define LH_CSS_SELECTOR_H

#include "string_id.h"
#include "style.h"
#include "media_query.h"
#include "css_tokenizer.h"
#include "formats/html/css/css_selectors.h"

namespace litehtml
{
	//////////////////////////////////////////////////////////////////////////

	using docrender::css::selector_specificity;
	using docrender::css::attr_select_type;
	using docrender::css::attr_matcher;
	using docrender::css::css_combinator;

	inline constexpr auto select_class = docrender::css::select_class;
	inline constexpr auto select_id = docrender::css::select_id;
	inline constexpr auto select_attr = docrender::css::select_attr;
	inline constexpr auto select_pseudo_class = docrender::css::select_pseudo_class;
	inline constexpr auto select_pseudo_element = docrender::css::select_pseudo_element;
	inline constexpr auto attribute_exists = docrender::css::attribute_exists;
	inline constexpr auto attribute_equals = docrender::css::attribute_equals;
	inline constexpr auto attribute_contains_string = docrender::css::attribute_contains_string;
	inline constexpr auto attribute_contains_word = docrender::css::attribute_contains_word;
	inline constexpr auto attribute_starts_with_string = docrender::css::attribute_starts_with_string;
	inline constexpr auto attribute_starts_with_string_hyphen = docrender::css::attribute_starts_with_string_hyphen;
	inline constexpr auto attribute_ends_with_string = docrender::css::attribute_ends_with_string;
	inline constexpr auto combinator_descendant = docrender::css::combinator_descendant;
	inline constexpr auto combinator_child = docrender::css::combinator_child;
	inline constexpr auto combinator_adjacent_sibling = docrender::css::combinator_adjacent_sibling;
	inline constexpr auto combinator_general_sibling = docrender::css::combinator_general_sibling;
	inline constexpr auto strict_mode = docrender::css::strict_mode;
	inline constexpr auto forgiving_mode = docrender::css::forgiving_mode;
	inline constexpr auto forbid_pseudo_elements = docrender::css::forbid_pseudo_elements;
	inline constexpr auto strict_arguments = docrender::css::strict_arguments;
	inline constexpr auto relative_selectors = docrender::css::relative_selectors;
	inline constexpr auto forbid_has = docrender::css::forbid_has;

	class css_selector;
	class html_tag;

	// <subclass-selector> | <pseudo-element-selector>
	// = <id-selector> | <class-selector> | <attribute-selector> | <pseudo-class-selector> | <pseudo-element-selector>
	struct css_attribute_selector
	{
		using vector = std::vector<css_attribute_selector>;

		attr_select_type	type;
		string_id			prefix;   // [prefix|name]
		string_id			name;     // .name, #name, [name], :name
		string				value;    // [name=value]
		string_vector language_ranges; // :lang(...)

		attr_matcher		matcher;         // <attr-matcher>   = ~= |= ^= $= *=
		bool				caseless_match;  // value is matched ASCII case-insensitively

		std::vector<shared_ptr<css_selector>> selector_list; // :not(selector_list)
		int a, b; // :nth-child(an+b of selector_list)

		css_attribute_selector(attr_select_type type = select_class, string name = "")
			: type(type), prefix(empty_id), name(_id(name)), matcher(), caseless_match(0), a(0), b(0) {}

		operator bool() const { return name != empty_id; }
	};

	//////////////////////////////////////////////////////////////////////////

	class css_element_selector // compound selector: div.class:hover
	{
	public:
		using ptr = shared_ptr<css_element_selector>;
	public:
		string_id						m_prefix;
		string_id						m_tag;
		css_attribute_selector::vector	m_attrs; // subclass and pseudo-element selectors
	};

	//////////////////////////////////////////////////////////////////////////

	class css_selector // complex selector: div + p
	{
	public:
		using ptr    = shared_ptr<css_selector>;
		using vector = std::vector<css_selector::ptr>;

	public:
		selector_specificity		m_specificity;
		int							m_order = 0;
		size_t						m_layer_id = 0;
		int							m_layer = css_default_layer;
		css_selector::ptr			m_left;
		css_element_selector		m_right;
		css_combinator				m_combinator = combinator_descendant;
		// On the first compound of a relative selector, the combinator from its anchor.
		css_combinator				m_anchor_combinator = combinator_descendant;
		media_query_list_list::ptr	m_media_query;
		style::ptr					m_style;

	public:
		bool parse(const string& text, document_mode mode);
		bool is_media_valid() const;
	};

	inline bool css_selector::is_media_valid() const
	{
		if(!m_media_query)
		{
			return true;
		}
		return m_media_query->is_used();
	}


	//////////////////////////////////////////////////////////////////////////

	inline bool operator > (const css_selector& v1, const css_selector& v2)
	{
		if(v1.m_specificity == v2.m_specificity)
		{
			return (v1.m_order > v2.m_order);
		}
		return (v1.m_specificity > v2.m_specificity);
	}

	inline bool operator < (const css_selector& v1, const css_selector& v2)
	{
		if(v1.m_specificity == v2.m_specificity)
		{
			return (v1.m_order < v2.m_order);
		}
		return (v1.m_specificity < v2.m_specificity);
	}

	inline bool operator > (const css_selector::ptr& v1, const css_selector::ptr& v2)
	{
		return (*v1 > *v2);
	}

	inline bool operator < (const css_selector::ptr& v1, const css_selector::ptr& v2)
	{
		return (*v1 < *v2);
	}

	//////////////////////////////////////////////////////////////////////////

	class used_selector
	{
	public:
		typedef std::unique_ptr<used_selector>	ptr;
		typedef std::vector<used_selector::ptr>	vector;

		css_selector::ptr	m_selector;
		bool				m_used;

		used_selector(const css_selector::ptr& selector, bool used)
		{
			m_used		= used;
			m_selector	= selector;
		}
	};


	bool supports_css_selector(const css_token_vector& tokens, document_mode mode);
	css_selector::vector parse_selector_list(const css_token_vector& tokens, int options, document_mode mode);
}

#endif  // LH_CSS_SELECTOR_H
