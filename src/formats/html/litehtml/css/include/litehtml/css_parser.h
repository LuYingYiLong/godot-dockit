#pragma once

#include "css_tokenizer.h"
#include "stylesheet.h"
#include "formats/html/css/css_parser.h"

namespace litehtml {

using docrender::css::raw_rule;
using docrender::css::raw_declaration;
using docrender::css::keep_whitespace_fn;
using docrender::css::remove_whitespace;
using docrender::css::normalize;
using docrender::css::parse_comma_separated_list;
using docrender::css::is_declaration_value;
using docrender::css::is_any_value;
using docrender::css::skip_whitespace;

inline constexpr auto f_componentize = docrender::css::f_componentize;
inline constexpr auto f_remove_whitespace = docrender::css::f_remove_whitespace;

class css_parser : public docrender::css::css_parser {
public:
	using docrender::css::css_parser::css_parser;
	using docrender::css::css_parser::parse_stylesheet;

	static raw_rule::vector parse_stylesheet(const string& input, bool top_level);
};

} // namespace litehtml
