#pragma once

#include "css_tokenizer.h"
#include "css_types.h"

namespace litehtml {

using docrender::css::css_token_type;
using docrender::css::css_number_type;
using docrender::css::css_hash_type;
using docrender::css::css_token;
using docrender::css::css_token_vector;
using docrender::css::css_tokenizer;
using docrender::css::get_repr;
using docrender::css::css_parse_error;
using docrender::css::tokenize;

inline constexpr auto WHITESPACE = docrender::css::WHITESPACE;
inline constexpr auto _EOF = docrender::css::_EOF;
inline constexpr auto LEFT_BRACE = docrender::css::LEFT_BRACE;
inline constexpr auto RIGHT_BRACE = docrender::css::RIGHT_BRACE;
inline constexpr auto LEFT_BRACKET = docrender::css::LEFT_BRACKET;
inline constexpr auto RIGHT_BRACKET = docrender::css::RIGHT_BRACKET;
inline constexpr auto LEFT_PAREN = docrender::css::LEFT_PAREN;
inline constexpr auto RIGHT_PAREN = docrender::css::RIGHT_PAREN;
inline constexpr auto COLON = docrender::css::COLON;
inline constexpr auto SEMICOLON = docrender::css::SEMICOLON;
inline constexpr auto COMMA = docrender::css::COMMA;
inline constexpr auto BANG = docrender::css::BANG;
inline constexpr auto DOT = docrender::css::DOT;
inline constexpr auto AMPERSAND = docrender::css::AMPERSAND;
inline constexpr auto IDENT = docrender::css::IDENT;
inline constexpr auto FUNCTION = docrender::css::FUNCTION;
inline constexpr auto AT_KEYWORD = docrender::css::AT_KEYWORD;
inline constexpr auto HASH = docrender::css::HASH;
inline constexpr auto STRING = docrender::css::STRING;
inline constexpr auto BAD_STRING = docrender::css::BAD_STRING;
inline constexpr auto URL = docrender::css::URL;
inline constexpr auto BAD_URL = docrender::css::BAD_URL;
inline constexpr auto NUMBER = docrender::css::NUMBER;
inline constexpr auto PERCENTAGE = docrender::css::PERCENTAGE;
inline constexpr auto DIMENSION = docrender::css::DIMENSION;
inline constexpr auto CDO = docrender::css::CDO;
inline constexpr auto CDC = docrender::css::CDC;
inline constexpr auto CV_FUNCTION = docrender::css::CV_FUNCTION;
inline constexpr auto CURLY_BLOCK = docrender::css::CURLY_BLOCK;
inline constexpr auto ROUND_BLOCK = docrender::css::ROUND_BLOCK;
inline constexpr auto SQUARE_BLOCK = docrender::css::SQUARE_BLOCK;
inline constexpr auto css_number_integer = docrender::css::css_number_integer;
inline constexpr auto css_number_number = docrender::css::css_number_number;
inline constexpr auto css_hash_unrestricted = docrender::css::css_hash_unrestricted;
inline constexpr auto css_hash_id = docrender::css::css_hash_id;

} // namespace litehtml
