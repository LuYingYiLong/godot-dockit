#include "litehtml_fonts.h"

#include "formats/html/resources/html_resources.h"
#include "html_system_fonts.h"

#include <godot_cpp/classes/font_variation.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/system_font.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <tuple>
#include <utility>

using namespace godot;
using namespace litehtml;

namespace docrender {
	namespace {
		constexpr int64_t WIDTH_AXIS = 0x77647468; // OpenType tag "wdth".
		constexpr uint32_t OPTICAL_SIZE_AXIS = 0x6f70737a; // OpenType tag "opsz".
		constexpr uint32_t PETITE_CAPS = 0x70636170; // OpenType tag "pcap".
		constexpr uint32_t UPPERCASE_PETITE_CAPS = 0x63327063; // OpenType tag "c2pc".

		int32_t font_pixel_size(pixel_t p_size) {
			const double size = std::round(static_cast<double>(p_size));
			return std::isfinite(size) && size <= std::numeric_limits<int32_t>::max()
				? static_cast<int32_t>(std::max(1.0, size)) : LiteHTMLFonts::DEFAULT_SIZE;
		}

		float glyph_height(const Ref<TextServer>& p_server, const RID& p_font, int32_t p_size, char32_t p_char, float p_fallback) {
			const int64_t glyph = p_server->font_get_glyph_index(p_font, p_size, p_char, 0);
			if (glyph == 0) {
				return p_fallback;
			}
			const Dictionary outline = p_server->font_get_glyph_contours(p_font, p_size, glyph);
			const PackedVector3Array points = outline.get("points", PackedVector3Array());
			float height = 0.0f;
			for (int64_t index = 0; index < points.size(); ++index) {
				height = std::max(height, -points[index].y);
			}
			return std::isfinite(height) && height > 0.0f ? height : p_fallback;
		}

		struct CaseFoldRange {
			char32_t first;
			char32_t last;
			int32_t offset;
			int step;
		};

		struct CaseFoldExpansion {
			char32_t codepoint;
			const char32_t* folded;
		};

		#include "unicode_case_folding.inc"

		struct CodepointRange {
			char32_t first;
			char32_t last;
		};

		#include "unicode_emphasis_skip.inc"

		bool skips_emphasis(char32_t p_value) {
			const auto* range = std::lower_bound(std::begin(EMPHASIS_SKIP_RANGES), std::end(EMPHASIS_SKIP_RANGES), p_value,
				[](const CodepointRange& p_range, char32_t p_codepoint) { return p_range.last < p_codepoint; });
			return range != std::end(EMPHASIS_SKIP_RANGES) && range->first <= p_value;
		}

		String fold_font_family(const String& p_name) {
			std::u32string folded;
			folded.reserve(p_name.length());
			for (int64_t index = 0; index < p_name.length(); ++index) {
				const char32_t codepoint = p_name[index];
				const auto* range = std::lower_bound(std::begin(CASE_FOLD_RANGES), std::end(CASE_FOLD_RANGES), codepoint,
					[](const CaseFoldRange& p_range, char32_t p_value) { return p_range.last < p_value; });
				if (range != std::end(CASE_FOLD_RANGES) && codepoint >= range->first && (codepoint - range->first) % range->step == 0) {
					folded += static_cast<char32_t>(static_cast<int32_t>(codepoint) + range->offset);
					continue;
				}
				const auto* expansion = std::lower_bound(std::begin(CASE_FOLD_EXPANSIONS), std::end(CASE_FOLD_EXPANSIONS), codepoint,
					[](const CaseFoldExpansion& p_expansion, char32_t p_value) { return p_expansion.codepoint < p_value; });
				if (expansion != std::end(CASE_FOLD_EXPANSIONS) && expansion->codepoint == codepoint) {
					folded += expansion->folded;
				}
				else {
					folded += codepoint;
				}
			}
			return String(folded.c_str());
		}

		std::pair<int, float> font_weight_rank(float p_requested, float p_selected) {
			const float distance = std::abs(p_requested - p_selected);
			if (p_requested < 400.0f) return { p_selected > p_requested, distance };
			if (p_requested > 500.0f) return { p_selected < p_requested, distance };
			if (p_selected >= p_requested && p_selected <= 500.0f) return { 0, distance };
			return { p_selected < p_requested ? 1 : 2, distance };
		}

		std::pair<bool, float> font_width_rank(float p_requested, float p_selected) {
			const bool wrong_side = p_requested <= 100.0f ? p_selected > p_requested : p_selected < p_requested;
			return { wrong_side, std::abs(p_requested - p_selected) };
		}

		std::pair<int, float> font_style_rank(const css_font_style& p_requested, const css_font_face& p_face,
			float p_selected_angle) {
			if (p_requested.kind == font_style_italic) {
				if (p_face.style == font_style_italic) return { 0, 0.0f };
				if (p_selected_angle >= 11.0f) return { 1, p_selected_angle - 11.0f };
				if (p_selected_angle > 0.0f) return { 2, 11.0f - p_selected_angle };
				return { p_face.style == font_style_normal ? 3 : 4, -p_selected_angle };
			}
			if (p_face.style == font_style_italic) {
				return { p_requested.kind == font_style_normal ? 2 : 3, 0.0f };
			}
			const float angle = p_requested.angle;
			if (angle == 0.0f && p_selected_angle == 0.0f) return { 0, 0.0f };
			const float direction = angle < 0.0f ? -1.0f : 1.0f;
			const float preferred = p_selected_angle * direction;
			const float requested = std::abs(angle);
			if (preferred == requested) return { 0, 0.0f };
			const float distance = std::abs(preferred - requested);
			if (preferred <= 0.0f && requested != 0.0f) return { 4, distance };
			if (preferred < 0.0f) return { 4, distance };
			const bool outward = requested >= 11.0f;
			return { (preferred > requested) == outward ? 1 : 2, distance };
		}

		Transform2D oblique_transform(float p_degrees) {
			constexpr double radians_per_degree = 3.14159265358979323846 / 180.0;
			// The tangent is undefined at either endpoint of the CSS angle range.
			const float shear = static_cast<float>(std::tan(std::clamp(p_degrees, -89.0f, 89.0f) * radians_per_degree));
			// TextServer passes the columns to FreeType as its y-up outline matrix rows.
			return Transform2D(1.0f, shear, 0.0f, 1.0f, 0.0f, 0.0f);
		}

		void merge_axis_settings(Dictionary& r_axes, const Dictionary& p_supported, const css_font_variations& p_settings) {
			if (p_settings.empty()) return;
			const auto server = TextServerManager::get_singleton()->get_primary_interface();
			for (const Variant& key : r_axes.keys()) {
				if (key.get_type() != Variant::STRING && key.get_type() != Variant::STRING_NAME) continue;
				const auto tag = static_cast<uint32_t>(server->name_to_tag(key));
				if (p_settings.count(tag) && p_supported.has(int64_t(tag))) r_axes.erase(key);
			}
			for (const auto& [tag, value] : p_settings) {
				if (p_supported.has(int64_t(tag))) r_axes[int64_t(tag)] = value;
			}
		}

		bool wants_optical_size(const font_description& p_description) {
			return p_description.optical_sizing == font_optical_sizing_auto && !p_description.variations.count(OPTICAL_SIZE_AXIS);
		}

		void apply_optical_size(Dictionary& r_axes, const Dictionary& p_supported, const font_description& p_description) {
			constexpr float points_per_css_pixel = 72.0f / 96.0f;
			merge_axis_settings(r_axes, p_supported, { { OPTICAL_SIZE_AXIS, font_pixel_size(p_description.size) * points_per_css_pixel } });
		}

		bool has_feature_settings(const font_description& p_description) {
			return p_description.kerning != font_kerning_auto || !p_description.features.empty() ||
				!p_description.variant_features.empty() || p_description.alternates.historical_forms || p_description.letter_spacing != 0;
		}

		Dictionary font_features(const Ref<Font>& p_font, const css_font_features& p_face,
			const css_font_features& p_named, const font_description& p_description) {
			Dictionary features = p_font->get_opentype_features();
			features = features.duplicate();
			constexpr uint32_t kern = 0x6b65726e;
			constexpr uint32_t palt = 0x70616c74;
			constexpr uint32_t apkn = 0x61706b6e;
			css_font_features overrides = p_face;
			if (p_description.kerning != font_kerning_auto) {
				const bool enable = p_description.kerning == font_kerning_normal;
				overrides[kern] = enable;
				if (enable) {
					const Dictionary supported = p_font->get_supported_feature_list();
					if (supported.has(int64_t(palt))) {
						overrides[palt] = 1;
						if (supported.has(int64_t(apkn))) overrides[apkn] = 1;
					}
				}
				else overrides[apkn] = 0;
			}
			for (const auto& [tag, value] : p_description.variant_features) overrides[tag] = value;
			if (p_description.variant_features.count(PETITE_CAPS)) {
				const Dictionary supported = p_font->get_supported_feature_list();
				const bool all_petite = p_description.variant_features.count(UPPERCASE_PETITE_CAPS);
				if (!supported.has(int64_t(PETITE_CAPS)) || (all_petite && !supported.has(int64_t(UPPERCASE_PETITE_CAPS)))) {
					overrides[PETITE_CAPS] = 0;
					overrides[0x736d6370] = 1; // smcp
					if (all_petite) {
						overrides[UPPERCASE_PETITE_CAPS] = 0;
						overrides[0x63327363] = 1; // c2sc
					}
				}
			}
			if (p_description.alternates.historical_forms) overrides[0x68697374] = 1; // hist
			for (const auto& [tag, value] : p_named) overrides[tag] = value;
			if (p_description.letter_spacing != 0) {
				for (const uint32_t optional : { 0x6c696761u, 0x636c6967u, 0x646c6967u, 0x686c6967u }) { // liga, clig, dlig, hlig
					overrides[optional] = 0;
				}
			}
			for (const auto& [tag, value] : p_description.features) overrides[tag] = value;
			const auto server = TextServerManager::get_singleton()->get_primary_interface();
			for (const Variant& key : features.keys()) {
				if (key.get_type() != Variant::STRING && key.get_type() != Variant::STRING_NAME) continue;
				const auto tag = static_cast<uint32_t>(server->name_to_tag(key));
				if (overrides.count(tag)) features.erase(key);
			}
			for (const auto& [tag, value] : overrides) features[int64_t(tag)] = int64_t(value);
			return features;
		}

		Ref<Font> combine_fonts(TypedArray<Font> p_fonts, const Ref<Font>& p_feature_source) {
			const Ref<Font> primary = p_fonts[0];
			if (p_fonts.size() == 1) return primary;
			const Ref<FontVariation> primary_variation = primary;
			Ref<FontVariation> combined;
			if (primary_variation.is_valid()) {
				combined = primary_variation->duplicate();
			}
			else {
				combined.instantiate();
				combined->set_base_font(primary);
			}
			p_fonts.remove_at(0);
			const TypedArray<Font> fallbacks = primary->get_fallbacks();
			for (int64_t index = 0; index < fallbacks.size(); ++index) {
				const Ref<Font> fallback = fallbacks[index];
				if (fallback.is_valid() && !p_fonts.has(fallback)) p_fonts.append(fallback);
			}
			combined->set_fallbacks(p_fonts);
			if (p_feature_source.is_valid()) combined->set_opentype_features(p_feature_source->get_opentype_features());
			return combined;
		}

	} // namespace

	LiteHTMLFonts::LiteHTMLFonts(const HTMLFontConfig& p_fonts, HTMLResources& p_resources) :
		fonts(p_fonts), resources(p_resources), default_font(p_fonts.text) {
		ThemeDB* themes = ThemeDB::get_singleton();
		const Ref<Theme> project_theme = themes->get_project_theme();
		if (default_font.is_null() && project_theme.is_valid()) {
			default_font = project_theme->get_default_font();
		}
		if (default_font.is_null()) {
			default_font = themes->get_fallback_font();
		}
	}

	bool LiteHTMLFonts::set_font_rules(std::vector<css_font_face> p_faces,
		std::vector<css_font_feature_values_rule> p_feature_values) {
		if (font_faces == p_faces && font_feature_rules == p_feature_values) return false;
		font_faces = std::move(p_faces);
		font_feature_rules = std::move(p_feature_values);
		font_stacks.clear();
		faces_by_family.clear();
		for (size_t index = 0; index < font_faces.size(); ++index) {
			const String name = fold_font_family(String::utf8(font_faces[index].family.c_str()));
			auto* faces = faces_by_family.getptr(name);
			if (!faces) faces = &faces_by_family.insert(name, {})->value;
			faces->push_back(index);
		}
		feature_names_by_family.clear();
		for (const auto& rule : font_feature_rules) {
			for (const auto& family : rule.families) {
				const String name = fold_font_family(String::utf8(family.c_str()));
				auto* definitions = feature_names_by_family.getptr(name);
				if (!definitions) definitions = &feature_names_by_family.insert(name, {})->value;
				for (size_t type = 0; type < rule.names.size(); ++type) {
					for (const auto& [alias, values] : rule.names[type]) (*definitions)[type].insert_or_assign(alias, values);
				}
			}
		}
		return true;
	}

	css_font_features LiteHTMLFonts::resolve_feature_names(const String& p_folded_family, const css_font_alternates& p_alternates) const {
		if (p_alternates.names.empty()) return {};
		const auto* definitions = feature_names_by_family.getptr(p_folded_family);
		return definitions ? resolve_font_feature_names(p_alternates, *definitions) : css_font_features{};
	}

	bool LiteHTMLFonts::FontChoice::covers(char32_t p_character) const {
		if (!ranges.empty()) {
			const auto range = std::lower_bound(ranges.begin(), ranges.end(), p_character,
				[](const auto& p_range, char32_t p_value) { return p_range.second < p_value; });
			if (range == ranges.end() || range->first > p_character) return false;
		}
		return font->has_char(p_character);
	}

	Ref<Font> LiteHTMLFonts::FontStack::match(const String& p_text, int64_t p_start, int64_t p_end) const {
		for (const auto& choice : choices) {
			int64_t index = p_start;
			while (index < p_end && choice.covers(p_text[index])) ++index;
			if (index == p_end) return choice.font;
		}
		return {};
	}

	void LiteHTMLFonts::FontStack::add_text(TextLine& r_line, const String& p_text, int32_t p_size) const {
		if (choices.empty() || p_text.is_empty()) {
			r_line.add_string(p_text, unrestricted, p_size);
			return;
		}
		const auto server = TextServerManager::get_singleton()->get_primary_interface();
		const auto breaks = server->string_get_character_breaks(p_text);
		int64_t start = 0;
		int64_t run_start = 0;
		Ref<Font> run;
		const auto append = [&](int64_t p_first, const Ref<Font>& p_font) {
			if (run != p_font) {
				if (run.is_valid()) r_line.add_string(p_text.substr(run_start, p_first - run_start), run, p_size);
				run_start = p_first;
				run = p_font;
			}
		};
		for (const auto end : breaks) {
			if (end <= start || end > p_text.length()) continue;
			const auto font = match(p_text, start, end);
			if (font.is_valid()) {
				append(start, font);
			}
			else {
				// Leave variation selectors and joined emoji sequences to the shaper.
				bool needs_sequence = false;
				for (int64_t index = start; index < end && !needs_sequence; ++index) {
					const char32_t character = p_text[index];
					needs_sequence = character == 0x200d || (character >= 0xfe00 && character <= 0xfe0f) ||
						(character >= 0xe0100 && character <= 0xe01ef);
				}
				if (needs_sequence) {
					append(start, unrestricted);
				}
				else {
					for (int64_t index = start; index < end; ++index) {
						const auto fallback = match(p_text, index, index + 1);
						append(index, fallback.is_valid() ? fallback : unrestricted);
					}
				}
			}
			start = end;
		}
		if (start < p_text.length()) {
			const auto remainder = match(p_text, start, p_text.length());
			append(start, remainder.is_valid() ? remainder : unrestricted);
		}
		if (run.is_valid()) r_line.add_string(p_text.substr(run_start), run, p_size);
	}

	Vector2 LiteHTMLFonts::FontStack::char_size(char32_t p_character, int32_t p_size) const {
		if (choices.empty()) return unrestricted->get_char_size(p_character, p_size);
		for (const auto& choice : choices) {
			if (choice.covers(p_character)) return choice.font->get_char_size(p_character, p_size);
		}
		return unrestricted->get_char_size(p_character, p_size);
	}

	HTMLTextLayout LiteHTMLFonts::FontData::shape(const String& p_text) {
		if (const auto* cached = lines.getptr(p_text)) {
			return *cached;
		}
		Ref<TextLine> line;
		line.instantiate();
		stack->add_text(*line.ptr(), p_text, size);
		HTMLTextLayout layout{ line, ascent };
		layout.spacing = HTMLTextSpacing::create(layout, p_text, description.letter_spacing, description.word_spacing);
		lines.insert(p_text, layout);
		return layout;
	}

	HTMLTextLayout LiteHTMLFonts::FontData::shape_punctuation(const String& p_text, punctuation_trim p_trim) {
		auto& cache = punctuation_lines[static_cast<size_t>(p_trim) - 1];
		if (const auto* cached = cache.getptr(p_text)) return *cached;
		auto layout = shape(p_text);
		const auto server = TextServerManager::get_singleton()->get_primary_interface();
		const auto glyphs = server->shaped_text_get_glyphs(layout.get_rid());
		const auto trimmed = [&]() -> HTMLTextLayout {
			if (glyphs.size() != 1) return layout;
			const Dictionary glyph = glyphs[0];
			const RID font = glyph["font_rid"];
			const int64_t index = glyph["index"];
			const int64_t font_size = glyph["font_size"];
			if (!font.is_valid() || index <= 0 || font_size <= 0) return layout;

			constexpr uint32_t halt = 0x68616c74;
			const auto choice = stack->match(p_text, 0, p_text.length());
			const auto& chosen = choice.is_valid() ? choice : stack->unrestricted;
			Dictionary features = chosen->get_opentype_features().duplicate();
			// Explicit font features already determine this glyph's advance.
			if (features.has(int64_t(halt)) || features.has("halt")) return layout;
			if (server->font_supported_feature_list(font).has(int64_t(halt))) {
				features[halt] = 1;
				Ref<TextLine> line;
				line.instantiate();
				if (server->shaped_text_add_string(line->get_rid(), p_text, chosen->get_rids(), font_size, features)) {
					HTMLTextLayout candidate{ line, ascent };
					if (candidate.width() > 0 && candidate.width() < layout.width()) return candidate;
				}
			}

			const auto han_advance = [&](char32_t p_character) {
				const auto id = server->font_get_glyph_index(font, font_size, p_character, 0);
				return id > 0 ? server->font_get_glyph_advance(font, font_size, id).x : real_t(0);
			};
			const float full = layout.width();
			const float ideograph = han_advance(U'水');
			if (ideograph <= 0 || std::abs(full - ideograph) > 0.5f ||
				std::abs(han_advance(U'卜') - ideograph) > 0.5f || std::abs(han_advance(U'一') - ideograph) > 0.5f) return layout;
			const Dictionary contours = server->font_get_glyph_contours(font, font_size, index);
			const PackedVector3Array points = contours.get("points", PackedVector3Array());
			if (points.is_empty()) return layout;
			const Vector2 offset = glyph["offset"];
			float left = std::numeric_limits<float>::infinity();
			float right = -left;
			for (const Vector3& point : points) {
				left = std::min(left, float(point.x + offset.x));
				right = std::max(right, float(point.x + offset.x));
			}
			const float trim_left = p_trim == punctuation_trim::left ? full / 2 : p_trim == punctuation_trim::center ? full / 4 : 0;
			const float trim_right = p_trim == punctuation_trim::right ? full / 2 : p_trim == punctuation_trim::center ? full / 4 : 0;
			if ((trim_left == 0 || left >= trim_left - 0.5f) && (trim_right == 0 || right <= full - trim_right + 0.5f)) {
				layout.trim_inline_advance(trim_left, trim_right);
			}
			return layout;
		}();
		cache.insert(p_text, trimmed);
		return trimmed;
	}

	const std::vector<float>* LiteHTMLFonts::FontData::get_emphasis_centers(const String& p_text, const HTMLTextLayout& p_layout) {
		if (emphasis.is_null()) return nullptr;
		if (!p_layout.is_subrange()) {
			if (const auto* cached = emphasis_centers.getptr(p_text)) return cached->empty() ? nullptr : cached;
		}
		const auto server = TextServerManager::get_singleton()->get_primary_interface();
		const auto line_rid = p_layout.get_rid();
		const auto offset = p_layout.text_start(*server.ptr());
		const auto breaks = server->string_get_character_breaks(p_text);
		std::vector<float> points;
		int32_t start = 0;
		for (const int32_t end : breaks) {
			if (end <= start || end > p_text.length()) continue;
			// A separator with combining marks is eligible when the cluster has visible content.
			bool eligible = false;
			for (int32_t index = start; index < end && !eligible; ++index) {
				eligible = !skips_emphasis(p_text[index]);
			}
			if (eligible) {
				const auto ranges = server->shaped_text_get_selection(line_rid, start + offset, end + offset);
				if (!ranges.is_empty()) {
					float left = ranges[0].x;
					float right = ranges[0].y;
					for (const Vector2& range : ranges) {
						left = std::min(left, range.x);
						right = std::max(right, range.y);
					}
					if (right > left) {
						points.push_back((left + right) / 2.0f);
					}
				}
			}
			start = end;
		}
		if (p_layout.is_subrange()) {
			fragment_emphasis_centers = std::move(points);
			return fragment_emphasis_centers.empty() ? nullptr : &fragment_emphasis_centers;
		}
		const auto& cached = emphasis_centers.insert(p_text, std::move(points))->value;
		return cached.empty() ? nullptr : &cached;
	}

	Ref<Font> LiteHTMLFonts::find_installed_font(const String& p_family, int p_weight, bool p_italic, float p_width) {
		if (p_family.is_empty()) {
			return {};
		}
		const String folded = fold_font_family(p_family);
		const int stretch = static_cast<int>(std::round(std::clamp(p_width, 50.0f, 200.0f)));
		const String key = vformat("%d/%d/%d/%s", p_weight, int(p_italic), stretch, folded);
		if (const auto* cached = installed_fonts.getptr(key)) {
			return *cached;
		}
		OS* os = OS::get_singleton();
		// Godot aliases these names on some platforms. A quoted CSS family must be a real family.
		const bool platform_alias = folded == "serif" || folded == "sans-serif" || folded == "monospace" ||
			folded == "cursive" || folded == "fantasy";
		if (!installed_font_names) {
			installed_font_names.emplace();
			for (const String& name : os->get_system_fonts()) {
				const String name_key = fold_font_family(name);
				if (!installed_font_names->has(name_key)) {
					installed_font_names->insert(name_key, name);
				}
			}
		}
		const String* listed_family = installed_font_names->getptr(folded);
		const bool exact_lookup = !platform_alias && os->get_name() == "Windows";
		Ref<Font> result;
		if (listed_family || !platform_alias) {
			Ref<SystemFont> font;
			font.instantiate();
			font->set_font_weight(std::clamp(p_weight, 100, 999));
			font->set_font_stretch(stretch);
			font->set_font_italic(p_italic);
			font->set_allow_system_fallback(false);
			PackedStringArray names;
			names.append(listed_family ? *listed_family : p_family);
			font->set_font_names(names);
			if (font->get_face_count() > 0 && (exact_lookup || (listed_family && !platform_alias) ||
				folded == fold_font_family(font->get_font_name()))) {
				result = font;
			}
		}
		installed_fonts.insert(key, result);
		return result;
	}

	Ref<FontFile> LiteHTMLFonts::find_local_face(const String& p_name) {
		const String key = fold_font_family(p_name);
		if (const auto* cached = local_faces.getptr(key)) return *cached;
		const auto font = load_system_font_face(p_name);
		local_faces.insert(key, font);
		return font;
	}

	void LiteHTMLFonts::append_authored_fonts(std::vector<FontChoice>& r_choices, const std::vector<size_t>& p_faces,
		const String& p_folded_family, const font_description& p_description, bool& r_size_dependent) {
		const auto named_features = resolve_feature_names(p_folded_family, p_description.alternates);
		struct Candidate {
			const css_font_face* face;
			float weight;
			float angle;
			std::pair<bool, float> width_rank;
			std::pair<int, float> style_rank;
			std::pair<int, float> weight_rank;
		};
		std::vector<Candidate> matches;
		matches.reserve(p_faces.size());
		for (auto index = p_faces.rbegin(); index != p_faces.rend(); ++index) {
			const auto& face = font_faces[*index];
			const float weight = std::clamp<float>(p_description.weight, face.min_weight, face.max_weight);
			const float angle = std::clamp(p_description.style.kind == font_style_italic ? 11.0f : p_description.style.angle,
				face.min_angle, face.max_angle);
			const auto [min_width, max_width] = face.width_range.value_or(std::make_pair(100.0f, 100.0f));
			const float width = std::clamp(p_description.width, min_width, max_width);
			matches.push_back({ &face, weight, angle, font_width_rank(p_description.width, width),
				font_style_rank(p_description.style, face, angle),
				font_weight_rank(p_description.weight, weight) });
		}
		const auto rank = [](const Candidate& p_candidate) {
			return std::tie(p_candidate.width_rank, p_candidate.style_rank, p_candidate.weight_rank);
		};
		std::stable_sort(matches.begin(), matches.end(), [&](const Candidate& a, const Candidate& b) {
			return rank(a) < rank(b);
		});
		const Candidate* best = nullptr;
		for (const auto& candidate : matches) {
			if (best && rank(candidate) != rank(*best)) break;
			const auto& face = *candidate.face;
			for (const auto& source : face.sources) {
				const Ref<FontFile> font = source.local ? find_local_face(String::utf8(source.value.c_str())) :
					resources.load_font(source.value.c_str(), face.base_url.c_str());
				if (font.is_null()) continue;
				constexpr int64_t weight_axis = 0x77676874; // OpenType tag "wght".
				constexpr int64_t slant_axis = 0x736c6e74; // OpenType tag "slnt".
				constexpr int64_t italic_axis = 0x6974616c; // OpenType tag "ital".
				const Dictionary axes_supported = font->get_supported_variation_list();
				if (source.needs_variations && axes_supported.is_empty()) continue;
				const bool variable_weight = axes_supported.has(weight_axis);
				Dictionary axes;
				if (variable_weight) axes[weight_axis] = candidate.weight;
				if (axes_supported.has(WIDTH_AXIS)) {
					float width = p_description.width;
					if (face.width_range) width = std::clamp(width, face.width_range->first, face.width_range->second);
					axes[WIDTH_AXIS] = width;
				}
				const bool oblique_face = face.style == font_style_oblique;
				const bool use_slant = axes_supported.has(slant_axis) && (oblique_face ||
					(p_description.style.kind == font_style_oblique && face.style == font_style_normal));
				if (use_slant) axes[slant_axis] = -(oblique_face ? candidate.angle : p_description.style.angle);
				if (face.style == font_style_italic && axes_supported.has(italic_axis)) axes[italic_axis] = 1;
				merge_axis_settings(axes, axes_supported, face.variations);
				if (wants_optical_size(p_description) && axes_supported.has(int64_t(OPTICAL_SIZE_AXIS))) {
					apply_optical_size(axes, axes_supported, p_description);
					r_size_dependent = true;
				}
				merge_axis_settings(axes, axes_supported, p_description.variations);
				const bool face_has_features = !face.features.empty() || !named_features.empty();
				const bool has_features = face_has_features || has_feature_settings(p_description);
				const bool embolden = !variable_weight && p_description.weight >= 600 &&
					candidate.weight < 600.0f && font->get_font_weight() < 600;
				const float angle = p_description.style.kind == font_style_italic ? default_oblique_angle : p_description.style.angle;
				const bool slant = angle != 0.0f && !use_slant && face.style == font_style_normal &&
					!(font->get_font_style() & TextServer::FONT_ITALIC);
				Ref<Font> selected = font;
				const int64_t face_index = font->get_face_index(0);
				if (!axes.is_empty() || has_features || embolden || slant || face_index != 0) {
					Ref<FontVariation> variation;
					variation.instantiate();
					variation->set_base_font(font);
					variation->set_variation_face_index(face_index);
					variation->set_variation_opentype(axes);
					if (has_features) variation->set_opentype_features(font_features(font, face.features, named_features, p_description));
					if (embolden) variation->set_variation_embolden(0.6f);
					if (slant) variation->set_variation_transform(oblique_transform(angle));
					selected = variation;
				}
				if (!best) best = &candidate;
				r_choices.push_back({ selected, face.unicode_ranges, face_has_features });
				break;
			}
		}
	}

	LiteHTMLFonts::FontStack LiteHTMLFonts::resolve_font_stack(const font_description& p_description) {
		const bool bold = p_description.weight >= 600;
		const bool italic = p_description.style.kind == font_style_italic;
		const bool oblique = p_description.style.kind == font_style_oblique;
		const float angle = italic ? default_oblique_angle : p_description.style.angle;
		const bool adjust_optical = wants_optical_size(p_description);
		bool size_dependent = false;
		bool has_named_features = false;
		const auto with_style = [&](const Ref<Font>& p_font, bool p_synthesize = true, const String& p_family = {}) -> Ref<Font> {
			const bool embolden = p_synthesize && bold && p_font->get_font_weight() < 600;
			const bool slant = p_synthesize && angle != 0.0f && !(p_font->get_font_style() & TextServer::FONT_ITALIC);
			const Ref<SystemFont> system_font = p_font;
			const bool adjust_width = p_description.width != 100.0f && system_font.is_null();
			const Dictionary supported_axes = adjust_width || adjust_optical || (slant && oblique) || !p_description.variations.empty() ?
				p_font->get_supported_variation_list() : Dictionary();
			const bool vary_width = adjust_width && supported_axes.has(WIDTH_AXIS);
			const bool vary_optical = adjust_optical && supported_axes.has(int64_t(OPTICAL_SIZE_AXIS));
			const bool vary_explicit = std::any_of(p_description.variations.begin(), p_description.variations.end(),
				[&](const auto& setting) { return supported_axes.has(int64_t(setting.first)); });
			css_font_features named_features;
			if (!p_description.alternates.names.empty() && !feature_names_by_family.is_empty()) {
				const String family = p_family.is_empty() ? fold_font_family(p_font->get_font_name()) : p_family;
				named_features = resolve_feature_names(family, p_description.alternates);
				has_named_features |= !named_features.empty();
			}
			const bool has_features = has_feature_settings(p_description) || !named_features.empty();
			if (!embolden && !slant && !vary_width && !vary_optical && !vary_explicit && !has_features) {
				return p_font;
			}
			const Ref<FontVariation> previous = p_font;
			Ref<FontVariation> variation;
			if (previous.is_valid()) {
				variation = previous->duplicate();
			}
			else {
				variation.instantiate();
				variation->set_base_font(p_font);
				const Ref<FontFile> file = p_font;
				if (file.is_valid()) variation->set_variation_face_index(file->get_face_index(0));
			}
			Dictionary axes = variation->get_variation_opentype().duplicate();
			if (vary_width) axes[WIDTH_AXIS] = p_description.width;
			if (embolden) {
				variation->set_variation_embolden(0.6f);
			}
			if (slant) {
				constexpr int64_t slant_axis = 0x736c6e74;
				if (oblique && supported_axes.has(slant_axis)) {
					axes[slant_axis] = -angle;
				}
				else {
					variation->set_variation_transform(oblique_transform(angle));
				}
			}
			if (vary_optical) {
				apply_optical_size(axes, supported_axes, p_description);
				size_dependent = true;
			}
			if (vary_explicit) merge_axis_settings(axes, supported_axes, p_description.variations);
			if (has_features) variation->set_opentype_features(font_features(p_font, {}, named_features, p_description));
			variation->set_variation_opentype(axes);
			variation->set_fallbacks(p_font->get_fallbacks());
			return variation;
		};
		const String text_name = fold_font_family(default_font->get_font_name());
		const Ref<Font>& themed_variant = bold && italic ? fonts.bold_italic : (bold ? fonts.bold : fonts.italic);
		const Ref<Font> text_font = (bold || italic) && themed_variant.is_valid() ?
			with_style(themed_variant, false, text_name) : with_style(default_font, true, text_name);
		if (p_description.families.empty()) {
			return { text_font, text_font, {}, size_dependent };
		}

		std::vector<FontChoice> choices;
		const auto append = [&](const Ref<Font>& p_font) {
			if (p_font.is_valid()) choices.push_back({ p_font, {} });
		};
		const auto append_installed = [&](const char* p_names) {
			for (const String& name : String(p_names).split("|")) {
				const auto font = find_installed_font(name, p_description.weight, italic, p_description.width);
				if (font.is_valid()) {
					append(with_style(font, true, fold_font_family(name)));
					break;
				}
			}
		};
		const String code_name = fonts.code.is_valid() ? fold_font_family(fonts.code->get_font_name()) : String();
		const String block_name = fonts.code_block.is_valid() ? fold_font_family(fonts.code_block->get_font_name()) : String();
		for (const auto& family : p_description.families) {
			const String name = String::utf8(family.name.c_str());
			if (!family.generic) {
				const String folded = fold_font_family(name);
				if (folded.is_empty()) {
					continue;
				}
				if (const auto* authored = faces_by_family.getptr(folded)) {
					append_authored_fonts(choices, *authored, folded, p_description, size_dependent);
				}
				else if (folded == "docrender-code-block" && fonts.code_block.is_valid()) {
					append(with_style(fonts.code_block, true, folded));
				}
				else if (folded == text_name) {
					append(text_font);
				}
				else if (folded == code_name) {
					append(with_style(fonts.code));
				}
				else if (folded == block_name) {
					append(with_style(fonts.code_block));
				}
				else if (const auto font = find_installed_font(name, p_description.weight, italic, p_description.width); font.is_valid()) {
					append(with_style(font, true, folded));
				}
				continue;
			}
			if (name == "sans-serif") {
				append(text_font);
			}
			else if (name == "system-ui" || name == "ui-sans-serif") {
				append_installed("Segoe UI|.AppleSystemUIFont|Helvetica Neue|Roboto|Cantarell|Noto Sans");
			}
			else if (name == "monospace" && fonts.code.is_valid()) {
				append(with_style(fonts.code));
			}
			else {
				static constexpr std::pair<const char*, const char*> generic_fonts[] = {
				{ "serif", "Times New Roman|Noto Serif|DejaVu Serif|Liberation Serif|Times|Droid Serif" },
				{ "monospace", "Courier New|Menlo|Noto Sans Mono|DejaVu Sans Mono|Liberation Mono|Roboto Mono|Droid Sans Mono" },
					{ "cursive", "Comic Sans MS|Apple Chancery|URW Chancery L|Dancing Script" },
					{ "fantasy", "Gabriola|Papyrus|Impact|Copperplate" },
					{ "math", "Cambria Math|STIX Two Math|Latin Modern Math|DejaVu Math TeX Gyre|Noto Sans Math" },
					{ "ui-serif", "New York" },
					{ "ui-monospace", "Cascadia Mono|SF Mono" },
					{ "ui-rounded", "SF Pro Rounded" },
					{ "generic(fangsong)", "FangSong|STFangsong|仿宋" },
					{ "generic(kai)", "KaiTi|STKaiti|Kaiti TC|楷体" },
					{ "generic(khmer-mul)", "Khmer OS Muol|Khmer OS Muol Light" },
					{ "generic(nastaliq)", "Noto Nastaliq Urdu|Jameel Noori Nastaleeq|Urdu Typesetting" },
				};
				for (const auto& [generic, candidates] : generic_fonts) {
					if (name == generic) {
						append_installed(candidates);
						break;
					}
				}
			}
		}
		append(text_font);
		TypedArray<Font> unrestricted;
		Ref<Font> primary;
		bool restricted = false;
		bool has_face_features = false;
		for (const auto& choice : choices) {
			has_face_features |= choice.has_face_features;
			if (choice.ranges.empty()) {
				if (!unrestricted.has(choice.font)) unrestricted.append(choice.font);
			}
			else {
				restricted = true;
			}
			// Font-relative metrics use the first face whose range includes space.
			if (primary.is_null() && (choice.ranges.empty() ||
				std::any_of(choice.ranges.begin(), choice.ranges.end(), [](const auto& range) {
					return range.first <= U' ' && U' ' <= range.second;
				}))) {
				primary = choice.font;
			}
		}
		// TextLine applies a span's feature dictionary to every font in its fallback list.
		// Split runs when feature defaults, kerning, or petite-cap fallback varies by face.
		const bool per_face_features = has_face_features || has_named_features || p_description.kerning == font_kerning_normal ||
			p_description.variant_features.count(PETITE_CAPS);
		if (!restricted && !per_face_features) choices.clear();
		return { primary, combine_fonts(unrestricted, per_face_features ? text_font : Ref<Font>()), std::move(choices), size_dependent };
	}

	uint_ptr LiteHTMLFonts::create_font(const font_description& p_description, font_metrics* r_metrics) {
		const int32_t size = font_pixel_size(p_description.size);
		String key = vformat("%d/%d/", p_description.weight, int(p_description.style.kind));
		key += String::num(p_description.style.angle, 9);
		key += String("/") + String::num(p_description.width, 9);
		for (const auto& family : p_description.families) {
			const String name = String::utf8(family.name.c_str());
			key += vformat("/%d/%d/%s", int(family.generic), name.length(), name);
		}
		key += String::utf8(p_description.opentype_key().c_str());
		String size_key;
		const auto* cached = font_stacks.getptr(key);
		if (!cached) {
			size_key = key + vformat("/size=%d", size);
			cached = font_stacks.getptr(size_key);
		}
		const auto stack = cached ? *cached : std::make_shared<const FontStack>(resolve_font_stack(p_description));
		if (!cached) font_stacks.insert(stack->size_dependent ? size_key : key, stack);
		const auto& font = stack->primary;

		const float base_ascent = font->get_ascent(size);
		float ascent = base_ascent;
		float descent = font->get_descent(size);
		r_metrics->emphasis_extent = 0;
		Ref<TextLine> emphasis;
		if (!p_description.emphasis_style.empty()) {
			const auto server = TextServerManager::get_singleton()->get_primary_interface();
			const String value = String::utf8(p_description.emphasis_style.c_str(), p_description.emphasis_style.size());
			const auto breaks = server->string_get_character_breaks(value);
			const String mark = breaks.is_empty() ? value : value.substr(0, breaks[0]);
			if (!mark.is_empty()) {
				Ref<Font> mark_font = stack->match(mark, 0, mark.length());
				if (mark_font.is_null()) mark_font = stack->unrestricted;
				const Ref<FontVariation> mark_variation = mark_font;
				Ref<FontVariation> variant;
				if (mark_variation.is_valid()) {
					variant = mark_variation->duplicate();
				}
				else {
					variant.instantiate();
					variant->set_base_font(mark_font);
					variant->set_fallbacks(mark_font->get_fallbacks());
				}
				Dictionary features = mark_font->get_opentype_features().duplicate();
				features["ruby"] = 1;
				variant->set_opentype_features(features);
				emphasis.instantiate();
				emphasis->add_string(mark, variant, std::max(1, size / 2));
				const bool under = p_description.emphasis_position & text_emphasis_position_under;
				const int adjacent_decoration = under ? text_decoration_line_underline : text_decoration_line_overline;
				r_metrics->emphasis_extent = emphasis->get_size().y + ((p_description.decoration_line & adjacent_decoration) ? 2.0f : 0.0f);
				if (under) descent += r_metrics->emphasis_extent;
				else ascent += r_metrics->emphasis_extent;
			}
		}
		auto* data = new FontData{ stack, size, ascent, p_description, emphasis, {}, {} };
		r_metrics->font_size = size;
		r_metrics->ascent = ascent;
		r_metrics->descent = descent;
		const float underline_position = font->get_underline_position(size);
		r_metrics->underline_position = std::isfinite(underline_position) ? underline_position : 0.0f;
		r_metrics->height = r_metrics->ascent + r_metrics->descent;
		const auto rids = font->get_rids();
		const auto server = TextServerManager::get_singleton()->get_primary_interface();
		r_metrics->x_height = size * 0.5f;
		r_metrics->cap_height = base_ascent;
		if (!rids.is_empty() && server.is_valid()) {
			const RID primary = rids[0];
			r_metrics->x_height = glyph_height(server, primary, size, U'x', r_metrics->x_height);
			r_metrics->cap_height = glyph_height(server, primary, size, U'H', r_metrics->cap_height);
		}
		const float zero_advance = stack->char_size(U'0', size).x;
		const float ideograph_advance = stack->char_size(U'\u6c34', size).x;
		r_metrics->ch_width = zero_advance > 0.0f ? zero_advance : size * 0.5f;
		r_metrics->ic_width = ideograph_advance > 0.0f ? ideograph_advance : static_cast<float>(size);
		r_metrics->draw_spaces = p_description.decoration_line != text_decoration_line_none;
		r_metrics->sub_shift = size * 0.2f;
		r_metrics->super_shift = size * 0.35f;
		return reinterpret_cast<uint_ptr>(data);
	}

	void LiteHTMLFonts::delete_font(uint_ptr p_font) {
		delete reinterpret_cast<FontData*>(p_font);
	}

	pixel_t LiteHTMLFonts::text_width(const char* p_text, uint_ptr p_font) {
		return reinterpret_cast<FontData*>(p_font)->shape(String::utf8(p_text)).width();
	}

	pixel_t LiteHTMLFonts::trimmed_punctuation_width(const char* p_text, uint_ptr p_font, punctuation_trim p_trim) {
		return reinterpret_cast<FontData*>(p_font)->shape_punctuation(String::utf8(p_text), p_trim).width();
	}

	HTMLTextLayout LiteHTMLFonts::shape_text(uint_ptr p_font, const String& p_text) {
		return reinterpret_cast<FontData*>(p_font)->shape(p_text == "\t" ? String(" ") : p_text);
	}

	HTMLTextLayout LiteHTMLFonts::shape_text_for_outline(uint_ptr p_font, const String& p_text,
		bool p_right_to_left, float& r_scale) {
		const auto& font = *reinterpret_cast<const FontData*>(p_font);
		if (font.description.size <= 0) return {};
		const int size = std::max(font.size, 32);
		r_scale = font.description.size / size;
		Ref<TextLine> line;
		line.instantiate();
		line->set_direction(p_right_to_left ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR);
		font.stack->add_text(*line.ptr(), p_text, size);
		HTMLTextLayout layout{ line };
		layout.spacing = HTMLTextSpacing::create(layout, p_text,
			font.description.letter_spacing / r_scale, font.description.word_spacing / r_scale);
		return layout;
	}

	HTMLShapedText LiteHTMLFonts::shape_styled_text(uint_ptr p_font, const String& p_run, text_justify_adjustment p_justification,
		float p_advance, int64_t p_start, int64_t p_end, punctuation_trim p_trim) {
		FontData& font = *reinterpret_cast<FontData*>(p_font);
		const String text = p_end < 0 ? p_run : p_run.substr(p_start, p_end - p_start);
		HTMLTextLayout layout = shape_text(p_font, p_run);
		const auto& description = font.description;
		if (p_trim != punctuation_trim::none) {
			layout = font.shape_punctuation(text, p_trim);
		}
		else if (p_end >= 0) {
			layout = layout.subrange(p_start, p_end);
			if (!layout.is_valid()) layout = shape_text(p_font, text);
			layout.spacing = HTMLTextSpacing::create(layout, text, description.letter_spacing,
				description.word_spacing, p_justification);
		}
		else if (text == "\t" || text == " ") {
			const float native_width = layout.native_width();
			if (p_advance != layout.width()) layout.spacing = HTMLTextSpacing::create(layout, " ", 0, p_advance - native_width);
		}
		else if (p_justification.spacing > 0.0f) {
			layout.spacing = HTMLTextSpacing::create(layout, text, description.letter_spacing,
				description.word_spacing, p_justification);
		}
		float thickness = 0.0f;
		if (description.decoration_line != text_decoration_line_none) {
			const auto& specified = description.decoration_thickness;
			thickness = layout.underline_thickness();
			if (!specified.is_predefined()) {
				thickness = specified.calc_percent(font.size);
			}
			else if (specified.predef() == text_decoration_thickness_from_font) {
				const float metric = font.stack->primary->get_underline_thickness(font.size);
				if (std::isfinite(metric) && metric > 0.0f) {
					thickness = metric;
				}
			}
			thickness = std::max(1.0f, thickness);
		}
		return { layout, description.decoration_line, description.decoration_style,
			description.decoration_color, thickness, font.emphasis, font.get_emphasis_centers(text, layout),
			description.emphasis_color, (description.emphasis_position & text_emphasis_position_under) != 0 };
	}
} // namespace docrender
