#include "html.h"
#include "style.h"
#include "css_parser.h"
#include "document_container.h"
#include "html_tag.h"
#include "formats/html/css/css_declarations.h"

namespace litehtml
{

void style::add(const string& txt, const string& baseurl, document_container* container, const html_tag* element)
{
	add(normalize(txt, f_componentize), baseurl, container, element);
}

void style::add(const css_token_vector& tokens, const string& baseurl, document_container* container, const html_tag* element)
{
	raw_declaration::vector declarations;
	raw_rule::vector rules;
	css_parser(tokens).consume_style_block_contents(declarations, rules);
	if (!rules.empty())
		css_parse_error("rule inside a style block");

	const auto source = container ? container->get_css_source(baseurl, element != nullptr) : css_source{};
	for (auto& declaration : declarations)
	{
		remove_whitespace(declaration.value);
		const auto& name = declaration.name;
		const auto id = _id(name.compare(0, 2, "--") == 0 ? name : lowcase(name));
		add_declaration(id, declaration.value, baseurl, declaration.important, container, source, element);
	}
}

void style::add_property(string_id name, const css_token_vector& tokens, const string& baseurl, bool important,
	document_container* container)
{
	add_declaration(name, tokens, baseurl, important, container, {}, nullptr);
}

void style::add_property(string_id name, const string& value, const string& baseurl, bool important, document_container* container)
{
	add_property(name, normalize(value, f_componentize | f_remove_whitespace), baseurl, important, container);
}

void style::add_declaration(string_id name, const css_token_vector& tokens, const string& baseurl, bool important,
	document_container* container, const css_source& source, const html_tag* element)
{
	auto result = parse_css_declaration(name, tokens, baseurl, important, container, source);
	if (auto diagnostic = check_css_declaration(result.status, name, tokens, source))
	{
		if (container) container->report_css_diagnostic(*diagnostic, element);
		if (!element) remember_diagnostic(*diagnostic);
	}
	for (auto& entry : result.properties)
	{
		if (element) entry.second.m_layer = css_inline_layer;
		add_parsed_property(entry.first, std::move(entry.second));
	}
}

void style::remember_diagnostic(const css_diagnostic& diagnostic)
{
	if (!diagnostics) diagnostics = std::make_shared<std::vector<css_diagnostic>>();
	const auto same_declaration = [&](const css_diagnostic& other) {
		return diagnostic.kind == other.kind && diagnostic.property == other.property && diagnostic.value == other.value &&
			diagnostic.source.kind == other.source.kind && diagnostic.source.path == other.source.path;
	};
	if (std::none_of(diagnostics->begin(), diagnostics->end(), same_declaration))
		diagnostics->push_back(diagnostic);
}

void style::add_parsed_property(string_id name, property_value value)
{
	if (value.m_layer >= 0 && value.m_layer != css_inline_layer)
	{
		auto& candidates = layer_candidates[name];
		auto [entry, inserted] = candidates.try_emplace(value.m_layer, value);
		if (!inserted && takes_cascade_precedence(value, entry->second))
			entry->second = value;
	}
	auto [entry, inserted] = m_properties.try_emplace(name, std::move(value));
	if (!inserted && takes_cascade_precedence(value, entry->second))
		entry->second = std::move(value);
}

void style::combine(const style& src, int layer)
{
	if (src.diagnostics)
	{
		for (const auto& diagnostic : *src.diagnostics)
			remember_diagnostic(diagnostic);
	}
	for (const auto& [name, property] : src.m_properties)
	{
		auto value = property;
		value.m_layer = layer;
		add_parsed_property(name, std::move(value));
	}
}

void style::snapshot_defaults()
{
	default_properties = m_properties;
}

const property_value& style::get_property(string_id name) const
{
	if (const auto it = computed_declarations.find(name); it != computed_declarations.end())
		return it->second;
	if (const auto it = m_properties.find(name); it != m_properties.end())
		return it->second;
	static const property_value unset;
	return unset;
}

const css_token_vector* style::get_custom_property(string_id name, bool& local) const
{
	const auto it = computed_variables.find(name);
	local = it != computed_variables.end();
	return local && it->second ? &*it->second : nullptr;
}

void style::subst_vars(const html_tag* element)
{
	resolve_css_declarations(m_properties, default_properties, layer_candidates, *element, computed_variables,
		computed_declarations, computed_diagnostics);
}

} // namespace litehtml
