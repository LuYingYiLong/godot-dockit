#include "html.h"
#include "el_before_after.h"
#include "document.h"
#include "document_container.h"
#include "el_image.h"
#include "el_space.h"
#include "el_text.h"
#include "formats/html/css/css_litehtml_dom.h"
#include "formats/html/litehtml_document_adapter.h"

namespace litehtml
{
namespace
{
	const string_vector& quote_pairs(const element& node)
	{
		static const string_vector english{ "\xe2\x80\x9c", "\xe2\x80\x9d", "\xe2\x80\x98", "\xe2\x80\x99" };
		static const string_vector french{ "\xc2\xab\xc2\xa0", "\xc2\xa0\xc2\xbb", "\xe2\x80\xb9\xc2\xa0", "\xc2\xa0\xe2\x80\xba" };
		static const string_vector german{ "\xe2\x80\x9e", "\xe2\x80\x9c", "\xe2\x80\x9a", "\xe2\x80\x98" };
		static const string_vector japanese{ "\xe3\x80\x8c", "\xe3\x80\x8d", "\xe3\x80\x8e", "\xe3\x80\x8f" };
		static const string_vector none;
		static const string_vector fr{ "fr" }, de{ "de" }, ja{ "ja" };

		const css_quotes* quotes = &node.css().get_quotes();
		auto language = node.parent();
		while (quotes->kind == css_quotes_kind::match_parent && language)
		{
			quotes = &language->css().get_quotes();
			const auto ancestor = language->parent();
			if (!ancestor) break;
			language = ancestor;
		}
		if (quotes->kind == css_quotes_kind::pairs) return quotes->pairs;
		if (quotes->kind == css_quotes_kind::none) return none;
		const auto& language_node = language ? *language : node;
		if (matches_element_language(language_node, fr)) return french;
		if (matches_element_language(language_node, de)) return german;
		if (matches_element_language(language_node, ja)) return japanese;
		return english;
	}

}

el_before_after_base::el_before_after_base(const document::ptr& doc, string_id tag) : html_tag(doc)
{
	m_tag = tag;
}

el_marker::el_marker(const document::ptr& doc) : el_before_after_base(doc, marker_tag())
{
}

void el_marker::compute_styles(bool recursive)
{
	html_tag::compute_styles(false);
	const auto list_item = parent();
	css_w().set_display(display_none);
	if (!list_item || list_item->css().get_display() != display_list_item) return;
	for (auto ancestor = list_item->parent(); ancestor; ancestor = ancestor->parent())
		if (ancestor->css().get_display() == display_none) return;

	const auto& content = css().get_content();
	css_generated_content fallback;
	if (content.normal)
	{
		const auto& list_css = list_item->css();
		const auto& url = list_css.get_list_style_image();
		size extent;
		if (!url.empty())
			get_document()->container()->get_image_size(url.c_str(), list_css.get_list_style_image_baseurl().c_str(), extent,
				decorative_image_orientation(css().get_image_orientation()));
		if (extent.width > 0 && extent.height > 0)
		{
			fallback.base_url = list_css.get_list_style_image_baseurl();
			fallback.items.push_back({ css_content_kind::image, url, {} });
			fallback.items.push_back({ css_content_kind::text, " ", {} });
		}
		else
		{
			const auto& marker = list_css.get_list_marker_type();
			if (marker.text)
				fallback.items.push_back({ css_content_kind::text, *marker.text, {} });
			else if (marker.counter.name != "none")
				fallback.items.push_back({ css_content_kind::marker_counter, "list-item", {}, marker.counter });
		}
	}
	const auto& effective = content.normal ? fallback : content;
	if (effective.items.empty()) return;
	css_w().prepare_marker(list_item->css().get_list_style_position() == list_style_position_outside);
	resolve_content(effective);
	if (recursive) compute_child_styles();
}

void el_before_after_base::compute_styles(bool recursive)
{
	html_tag::compute_styles(false);
	const auto& content = css().get_content();
	if (content.items.empty())
		css_w().set_display(display_none);

	if (css().get_display() == display_none) return;
	for (auto ancestor = parent(); ancestor; ancestor = ancestor->parent())
	{
		if (ancestor->css().get_display() == display_none || ancestor->is_replaced()) return;
	}
	handle_counter_properties();
	resolve_content(content);
	if (recursive) compute_child_styles();
}

void el_before_after_base::resolve_content(const css_generated_content& content)
{
	using kind = css_content_kind;
	const auto doc = get_document();
	const auto origin = parent();
	const auto lookup = [&](const string& name) { return doc->find_counter_style(name); };
	std::vector<std::pair<kind, string>> parts;
	string text;
	for (const auto& item : content.items)
	{
		switch (item.kind)
		{
		case kind::text:
			text += item.value;
			break;
		case kind::attribute:
		{
			const char* value = origin ? origin->get_attr(item.value.c_str()) : nullptr;
			text += value ? value : item.fallback_or_separator;
			break;
		}
		case kind::image:
			if (!text.empty()) parts.emplace_back(kind::text, std::move(text));
			text.clear();
			parts.emplace_back(kind::image, item.value);
			break;
		case kind::counter:
		case kind::marker_counter:
			text += format_css_counter(get_counter_value(_id(item.value)), item.counter_style, lookup, item.kind == kind::marker_counter);
			break;
		case kind::counters:
		{
			const auto values = get_counter_values(_id(item.value));
			for (size_t i = 0; i < values.size(); ++i)
			{
				if (i != 0) text += item.fallback_or_separator;
				text += format_css_counter(values[i], item.counter_style, lookup);
			}
			break;
		}
		case kind::open_quote:
		case kind::close_quote:
		case kind::no_open_quote:
		case kind::no_close_quote:
		{
			const bool open = item.kind == kind::open_quote || item.kind == kind::no_open_quote;
			const std::optional<size_t> depth = open ? std::optional<size_t>(doc->open_quote()) : doc->close_quote();
			if (depth && (item.kind == kind::open_quote || item.kind == kind::close_quote))
			{
				const auto& pairs = quote_pairs(*this);
				if (!pairs.empty())
					text += pairs[std::min(*depth, pairs.size() / 2 - 1) * 2 + (open ? 0 : 1)];
			}
			break;
		}
		}
	}
	if (!text.empty()) parts.emplace_back(kind::text, std::move(text));

	if (parts == materialized_content && content.base_url == materialized_base) return;
	materialized_content = std::move(parts);
	materialized_base = content.base_url;
	clearRecursive();
	for (const auto& [type, value] : materialized_content)
	{
		if (type == kind::image) add_image(value, materialized_base);
		else add_text(value);
	}
}

void el_before_after_base::add_text(const string& text)
{
	for (const auto& run : docrender_editor::create_text(get_document(), text.c_str(), true))
		appendChild(run);
}

void el_before_after_base::add_image(const string& url, const string& base)
{
	const bool list_style_image = tag() == marker_tag() && css().get_content().normal;
	const auto image = std::make_shared<el_image>(get_document(), base, list_style_image);
	image->set_attr("src", url.c_str());
	image->set_attr("style", "display:inline-block");
	image->set_tagName("img");
	appendChild(image);
	image->parse_attributes();
}
} // namespace litehtml
