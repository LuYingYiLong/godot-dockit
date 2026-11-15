#pragma once

#include "css_property_value.h"

namespace litehtml {
	enum class css_declaration_status { unknown, invalid, deferred, parsed };

	struct css_parsed_declaration {
		css_declaration_status status;
		props_map properties;
	};

	css_parsed_declaration parse_css_declaration(string_id name, const css_token_vector& tokens,
		const string& baseurl, bool important, document_container* container, const css_source& source);
	css_parsed_declaration reparse_css_declaration(const property_value::deferred_declaration& declaration,
		const css_token_vector& tokens, bool important, document_container* container);
	bool supports_css_declaration(const css_token_vector& tokens);
	std::optional<css_diagnostic> check_css_declaration(css_declaration_status status, string_id name,
		const css_token_vector& tokens, const css_source& source);
}
