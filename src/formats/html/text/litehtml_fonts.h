#pragma once

#include "formats/html/css/css_font_face.h"
#include "html_font_config.h"
#include "html_shaped_text.h"

#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <litehtml/font_description.h>

#include <array>
#include <optional>

namespace docrender {
	class HTMLResources;

	class LiteHTMLFonts {
		struct FontChoice {
			godot::Ref<godot::Font> font;
			litehtml::css_unicode_ranges ranges;
			bool has_face_features = false;

			bool covers(char32_t p_character) const;
		};

		struct FontStack {
			godot::Ref<godot::Font> primary;
			godot::Ref<godot::Font> unrestricted;
			std::vector<FontChoice> choices;
			godot::String language;
			bool size_dependent = false;

			godot::Ref<godot::Font> match(const godot::String& p_text, int64_t p_start, int64_t p_end) const;
			void add_text(godot::TextLine& r_line, const godot::String& p_text, int32_t p_size) const;
			godot::Vector2 char_size(char32_t p_character, int32_t p_size) const;
		};

		struct FontData {
			std::shared_ptr<const FontStack> stack;
			int32_t size;
			float ascent;
			litehtml::font_description description;
			godot::Ref<godot::TextLine> emphasis;
			godot::HashMap<godot::String, HTMLTextLayout> lines;
			godot::HashMap<godot::String, std::vector<float>> emphasis_centers;
			std::vector<float> fragment_emphasis_centers;
			std::array<godot::HashMap<godot::String, HTMLTextLayout>, 3> punctuation_lines;

			HTMLTextLayout shape(const godot::String& p_text);
			HTMLTextLayout shape_punctuation(const godot::String& p_text, litehtml::punctuation_trim p_trim);
			const std::vector<float>* get_emphasis_centers(const godot::String& p_text, const HTMLTextLayout& p_layout);
		};

		HTMLFontConfig fonts;
		HTMLResources& resources;
		godot::Ref<godot::Font> default_font;
		std::vector<litehtml::css_font_face> font_faces;
		std::vector<litehtml::css_font_feature_values_rule> font_feature_rules;
		godot::HashMap<godot::String, std::vector<size_t>> faces_by_family;
		godot::HashMap<godot::String, litehtml::css_font_feature_names> feature_names_by_family;
		godot::HashMap<godot::String, std::shared_ptr<const FontStack>> font_stacks;
		godot::HashMap<godot::String, godot::Ref<godot::Font>> installed_fonts;
		godot::HashMap<godot::String, godot::Ref<godot::FontFile>> local_faces;
		std::optional<godot::HashMap<godot::String, godot::String>> installed_font_names;

		FontStack resolve_font_stack(const litehtml::font_description& p_description);
		godot::Ref<godot::Font> find_installed_font(const godot::String& p_family, int p_weight, bool p_italic, float p_width);
		godot::Ref<godot::FontFile> find_local_face(const godot::String& p_name);
		litehtml::css_font_features resolve_feature_names(const godot::String& p_folded_family,
			const litehtml::css_font_alternates& p_alternates) const;
		void append_authored_fonts(std::vector<FontChoice>& r_choices, const std::vector<size_t>& p_faces,
			const godot::String& p_folded_family, const litehtml::font_description& p_description, bool& r_size_dependent);

	public:
		static constexpr int32_t DEFAULT_SIZE = 16;
		LiteHTMLFonts(const HTMLFontConfig& p_fonts, HTMLResources& p_resources);
		const char* default_family() const;
		int32_t default_fixed_size() const;
		bool set_font_rules(std::vector<litehtml::css_font_face> p_faces,
			std::vector<litehtml::css_font_feature_values_rule> p_feature_values);

		litehtml::uint_ptr create_font(const litehtml::font_description& p_description, litehtml::font_metrics* r_metrics);
		void delete_font(litehtml::uint_ptr p_font);
		litehtml::pixel_t text_width(const char* p_text, litehtml::uint_ptr p_font);
		litehtml::pixel_t trimmed_punctuation_width(const char* p_text, litehtml::uint_ptr p_font, litehtml::punctuation_trim p_trim);
		HTMLTextLayout shape_text(litehtml::uint_ptr p_font, const godot::String& p_text);
		HTMLTextLayout shape_text_for_outline(litehtml::uint_ptr p_font, const godot::String& p_text,
			bool p_right_to_left, float& r_scale); // 保留字形索引和分数像素字号的轮廓排版
		HTMLShapedText shape_styled_text(litehtml::uint_ptr p_font, const godot::String& p_text,
			litehtml::text_justify_adjustment p_justification, float p_advance, int64_t p_start = 0, int64_t p_end = -1,
			litehtml::punctuation_trim p_trim = litehtml::punctuation_trim::none);
	};
}
