#ifndef LH_STYLE_H
#define LH_STYLE_H

#include <limits>

#include "background.h"
#include "formats/html/css/css_border_image.h"
#include "css_length.h"
#include "css_position.h"
#include "css_tokenizer.h"
#include "gradient.h"
#include "formats/html/litehtml_css_transform.h"
#include "formats/html/layout/grid/litehtml_grid_tracks.h"
#include "formats/html/litehtml_box_shadow.h"
#include "formats/html/litehtml_css.h"
#include "formats/html/litehtml_css_variables.h"
#include "web_color.h"

namespace litehtml
{
	struct invalid {}; // indicates "not found" condition in style::get_property
	struct inherit {}; // "inherit" was specified as the value of this property

	struct initial_value {};
	struct revert_value {};
	struct revert_layer_value {};

	// Layers use their normal cascade order. Special ranks sit outside the named layers.
	constexpr int css_default_layer = -1;
	constexpr int css_unlayered = std::numeric_limits<int>::max() - 1;
	constexpr int css_inline_layer = std::numeric_limits<int>::max();

	struct property_value : variant<
		invalid,
		initial_value,
		revert_value,
		revert_layer_value,
		inherit,
		int,
		int_vector,
		css_length,
		css_aspect_ratio,
		object_fit,
		css_image_position,
		vector<css_axis_position>,
		css_image_resolution,
		std::optional<web_color>,
		css_border_image_source,
		css_border_image_slice,
		css_border_image_offsets,
		css_border_image_repeat,
		css_image_orientation,
		length_vector,
		float,
		web_color,
		vector<image>,
		vector<background_repeat>,
		string,
		string_vector,
		size_vector,
		grid_line,
		css_alignment,
		css_transform_list,
		css_transform_origin,
		grid_track_template,
		grid_area_template,
		grid_track_list,
		grid_auto_flow,
		std::vector<box_shadow>,
		css_font_families,
		css_font_style,
		css_font_features,
		css_font_alternates,
		css_font_variations,
		css_generated_content,
		css_list_marker_type,
		css_quotes,
		css_counter_changes,
		css_text_indent,
		css_hyphenation_chars,
		css_token_vector
	>
	{
		bool m_important = false;
		int m_layer = css_default_layer;

		struct deferred_declaration
		{
			string_id name;
			css_token_vector tokens;
			string baseurl;
			css_source source;
		};
		std::shared_ptr<const deferred_declaration> deferred;

		struct declaration_origin
		{
			string_id name;
			string value;
			css_source source;
		};
		std::shared_ptr<const declaration_origin> origin;

		property_value() {}
		template<class T> property_value(const T& val, bool important)
			: base(val), m_important(important) {}
	};

	// Compare origins first. Within each layer, callers use increasing specificity and source order.
	inline bool takes_cascade_precedence(const property_value& candidate, const property_value& current)
	{
		if ((candidate.m_layer == css_default_layer) != (current.m_layer == css_default_layer))
			return current.m_layer == css_default_layer;
		if (candidate.m_important != current.m_important) return candidate.m_important;
		if (candidate.m_layer == current.m_layer) return true;
		if (current.m_layer == css_inline_layer) return false;
		if (candidate.m_layer == css_inline_layer) return true;
		return candidate.m_important ? candidate.m_layer < current.m_layer : candidate.m_layer > current.m_layer;
	}

	class html_tag;
	typedef std::map<string_id, property_value>	props_map;
	using css_layer_candidates = std::map<string_id, std::map<int, property_value>>;

	// represents a style block, eg. "color: black; display: inline"
	class style
	{
	public:
		typedef std::shared_ptr<style>		ptr;
		typedef std::vector<style::ptr>		vector;
	private:
		props_map							m_properties;
		css_computed_variables computed_variables;
		props_map computed_declarations;
		std::shared_ptr<std::vector<css_diagnostic>> diagnostics;
		std::vector<css_diagnostic> computed_diagnostics;
		props_map default_properties;
		css_layer_candidates layer_candidates;
	public:
		void add(const css_token_vector& tokens, const string& baseurl = "", document_container* container = nullptr, const html_tag* element = nullptr);
		void add(const string& txt,              const string& baseurl = "", document_container* container = nullptr, const html_tag* element = nullptr);

		void add_property(string_id name, const css_token_vector& tokens, const string& baseurl = "", bool important = false, document_container* container = nullptr);
		void add_property(string_id name, const string& val,              const string& baseurl = "", bool important = false, document_container* container = nullptr);

		const property_value& get_property(string_id name) const;

		void combine(const style& src, int layer = css_default_layer);
		void clear()
		{
			diagnostics.reset();
			computed_diagnostics.clear();
			default_properties.clear();
			layer_candidates.clear();
			computed_variables.clear();
			computed_declarations.clear();
			m_properties.clear();
		}

		const css_token_vector* get_custom_property(string_id name, bool& local) const;
		const std::vector<css_diagnostic>* get_diagnostics() const { return diagnostics.get(); }
		const std::vector<css_diagnostic>& get_computed_diagnostics() const { return computed_diagnostics; }
		void snapshot_defaults();
		void subst_vars(const html_tag* el);

	private:

		void add_declaration(string_id name, const css_token_vector& tokens, const string& baseurl, bool important,
			document_container* container, const css_source& source, const html_tag* element);
		void remember_diagnostic(const css_diagnostic& diagnostic);
		void add_parsed_property(string_id name, property_value value);
	};

	bool parse_url(const css_token& token, string& url);
	bool parse_length(const css_token& tok, css_length& length, int options, string keywords = "");
	bool parse_angle(const css_token& tok, float& angle, bool percents_allowed = false);
	bool parse_bg_position(const css_token_vector& tokens, int& index, css_length& x, css_length& y,
		bool convert_keywords_to_percents, int length_flags = f_length_percentage);

	template<typename Enum>
	bool parse_keyword(const css_token& tok, Enum& val, string keywords, int first_keyword_value = 0)
	{
		int	value_index(const string& val, const string& strings, int defValue = -1, char delim = ';');
		int idx = value_index(tok.ident(), keywords);
		if (idx == -1) return false;
		val = (Enum)(first_keyword_value + idx);
		return true;
	}

} // namespace litehtml

#endif  // LH_STYLE_H
