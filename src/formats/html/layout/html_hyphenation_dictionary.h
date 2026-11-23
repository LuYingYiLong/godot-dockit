#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace dockit {
	struct HyphenationPatternData {
		std::string_view locale;
		const uint8_t* bytes;
		size_t size;
	};

	// 查找自动断词词典
	const HyphenationPatternData* find_hyphenation_patterns(std::string_view p_locale);			// 根据语言查找自动断词模式
	const HyphenationPatternData* hyphenation_patterns_for_language(
		std::string_view p_language
	);																							// 查找适用于指定语言的自动断词模式
	std::vector<int64_t> automatic_hyphenation_points(
		const HyphenationPatternData& p_patterns,
		std::u32string_view p_word
	);																							// 计算单词中可自动断开的字符位置
}
