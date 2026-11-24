#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <optional>
#include <vector>

namespace dockit {

struct WordBoundary {
	int64_t start = 0;
	int64_t end = 0;
};

enum WordSelectionMode : int64_t {
	WORD_SELECTION_MODE_AUTO,
	WORD_SELECTION_MODE_TEXT_SERVER,
	WORD_SELECTION_MODE_JIEBA,
};

inline bool is_word_whitespace(char32_t p_character) {
	return (p_character >= 0x0009 && p_character <= 0x000D) || p_character == 0x0020 ||
		p_character == 0x0085 || p_character == 0x00A0 || p_character == 0x1680 ||
		(p_character >= 0x2000 && p_character <= 0x200A) || p_character == 0x2028 ||
		p_character == 0x2029 || p_character == 0x202F || p_character == 0x205F ||
		p_character == 0x3000;
}

inline godot::String get_project_word_language() {
	if (godot::TranslationServer* server = godot::TranslationServer::get_singleton()) {
		const godot::String locale = server->get_locale();
		if (!locale.is_empty()) return locale;
	}
	return "en";
}

inline godot::String normalize_word_language(godot::String p_language) {
	return p_language.strip_edges().replace("_", "-").to_lower();
}

inline bool uses_simplified_chinese_words(const godot::String& p_language) {
	const godot::String language = normalize_word_language(p_language);
	const godot::PackedStringArray parts = language.split("-");
	if (parts.is_empty() || parts[0] != "zh") return false;
	for (int64_t index = 1; index < parts.size(); index++) {
		const godot::String part = parts[index];
		if (part == "hant" || part == "tw" || part == "hk" || part == "mo") return false;
	}
	for (int64_t index = 1; index < parts.size(); index++) {
		const godot::String part = parts[index];
		if (part == "hans" || part == "cn" || part == "sg" || part == "my") return true;
	}
	return parts.size() == 1;
}

inline bool is_jieba_segmenter_available() {
	static const godot::StringName class_name("JiebaSegment");
	return godot::ClassDB::class_exists(class_name) && godot::ClassDB::can_instantiate(class_name) &&
		godot::ClassDB::class_has_method(class_name, "cut_with_offsets") &&
		godot::ClassDB::class_has_method(class_name, "initialize");
}

inline godot::String get_word_selection_mode_hint() {
	return is_jieba_segmenter_available() ? "Auto:0,TextServer:1,Jieba:2" : "Auto:0,TextServer:1,Jieba (unavailable):2";
}

inline godot::Ref<godot::RefCounted> get_optional_jieba_segmenter() {
	godot::Engine* engine = godot::Engine::get_singleton();
	if (engine == nullptr) return {};
	static const godot::StringName metadata_key("_dockit_word_boundary_jieba_segmenter");
	if (engine->has_meta(metadata_key)) {
		godot::Object* cached = engine->get_meta(metadata_key);
		if (godot::RefCounted* segmenter = godot::Object::cast_to<godot::RefCounted>(cached)) {
			return godot::Ref<godot::RefCounted>(segmenter);
		}
		return {};
	}
	static const godot::StringName class_name("JiebaSegment");
	if (!is_jieba_segmenter_available()) return {};
	godot::Ref<godot::RefCounted> segmenter(godot::ClassDB::instantiate(class_name));
	if (segmenter.is_null()) return segmenter;
	if (godot::ClassDB::class_has_method(class_name, "is_initialized") &&
		static_cast<bool>(segmenter->call("is_initialized"))) {
		engine->set_meta(metadata_key, segmenter);
		return segmenter;
	}
	const godot::Variant initialized = segmenter->call("initialize");
	if (initialized.get_type() != godot::Variant::BOOL || !static_cast<bool>(initialized)) {
		engine->set_meta(metadata_key, godot::Variant());
		return {};
	}
	engine->set_meta(metadata_key, segmenter);
	return segmenter;
}

inline bool get_jieba_word_boundaries(const godot::String& p_text, std::vector<WordBoundary>& r_boundaries) {
	const godot::Ref<godot::RefCounted> segmenter = get_optional_jieba_segmenter();
	if (segmenter.is_null()) return false;
	const godot::Variant result = segmenter->call("cut_with_offsets", p_text, true);
	if (result.get_type() != godot::Variant::ARRAY) return false;
	const godot::Array words = result;
	if (!p_text.is_empty() && words.is_empty()) return false;
	int64_t previous_end = 0;
	for (int64_t index = 0; index < words.size(); index++) {
		if (words[index].get_type() != godot::Variant::DICTIONARY) return false;
		const godot::Dictionary word = words[index];
		const godot::Variant start_value = word.get("start", godot::Variant());
		const godot::Variant end_value = word.get("end", godot::Variant());
		const godot::Variant text_value = word.get("word", godot::Variant());
		if (start_value.get_type() != godot::Variant::INT || end_value.get_type() != godot::Variant::INT ||
			text_value.get_type() != godot::Variant::STRING) return false;
		const int64_t start = start_value;
		const int64_t end = end_value;
		if (start < previous_end || start < 0 || end <= start || end > p_text.length()) return false;
		const godot::String token = p_text.substr(start, end - start);
		if (token != static_cast<godot::String>(text_value)) return false;
		if (token.strip_edges().is_empty()) {
			previous_end = end;
			continue;
		}
		r_boundaries.push_back({ start, end });
		previous_end = end;
	}
	return true;
}

inline std::vector<WordBoundary> get_text_server_word_boundaries(const godot::String& p_text, const godot::String& p_language) {
	std::vector<WordBoundary> boundaries;
	const godot::TextServerManager* manager = godot::TextServerManager::get_singleton();
	if (manager == nullptr) return boundaries;
	const godot::Ref<godot::TextServer> server = manager->get_primary_interface();
	if (server.is_null()) return boundaries;
	const godot::PackedInt32Array breaks = server->string_get_word_breaks(p_text, normalize_word_language(p_language));
	if ((breaks.size() & 1) != 0) return boundaries;
	for (int64_t index = 0; index + 1 < breaks.size(); index += 2) {
		const int64_t start = breaks[index];
		const int64_t end = breaks[index + 1];
		if (start >= 0 && end > start && end <= p_text.length() &&
			!p_text.substr(start, end - start).strip_edges().is_empty()) {
			boundaries.push_back({ start, end });
		}
	}
	return boundaries;
}

inline std::optional<WordBoundary> word_boundary_at(
		const godot::String& p_text,
		int64_t p_offset,
		const godot::String& p_language,
		int64_t p_scope_start = 0,
		int64_t p_scope_end = -1,
		int64_t p_selection_mode = WORD_SELECTION_MODE_AUTO) {
	const int64_t length = p_text.length();
	const int64_t scope_start = std::clamp<int64_t>(p_scope_start, 0, length);
	const int64_t requested_end = p_scope_end < 0 ? length : p_scope_end;
	const int64_t scope_end = std::clamp<int64_t>(requested_end, scope_start, length);
	if (p_offset < scope_start || p_offset > scope_end || scope_start == scope_end) return std::nullopt;
	if (p_offset < length && is_word_whitespace(p_text[p_offset])) return std::nullopt;
	const auto is_scope_separator = [&](int64_t p_index) {
		const char32_t character = p_text[p_index];
		return character == '\n' || character == '\r' || character == '\t';
	};
	int64_t start = p_offset;
	int64_t end = p_offset;
	while (start > scope_start && !is_scope_separator(start - 1)) start--;
	while (end < scope_end && !is_scope_separator(end)) end++;
	if (start == end || (p_offset < length && is_scope_separator(p_offset))) return std::nullopt;
	const godot::String segment = p_text.substr(start, end - start);
	const int64_t local_offset = p_offset - start;
	std::vector<WordBoundary> boundaries;
	const bool use_jieba = p_selection_mode == WORD_SELECTION_MODE_JIEBA ||
		(p_selection_mode == WORD_SELECTION_MODE_AUTO && uses_simplified_chinese_words(p_language));
	if (p_selection_mode != WORD_SELECTION_MODE_TEXT_SERVER && use_jieba &&
		!get_jieba_word_boundaries(segment, boundaries)) {
		boundaries.clear();
	}
	if (boundaries.empty()) boundaries = get_text_server_word_boundaries(segment, p_language);
	for (const WordBoundary& boundary : boundaries) {
		if (local_offset >= boundary.start && local_offset < boundary.end) {
			return WordBoundary{ start + boundary.start, start + boundary.end };
		}
		if (local_offset == boundary.end && local_offset > 0 && p_offset < scope_end &&
			!is_scope_separator(p_offset - 1) && !is_word_whitespace(p_text[p_offset])) {
			return WordBoundary{ start + boundary.start, start + boundary.end };
		}
	}
	return std::nullopt;
}

} // namespace dockit
