#ifndef LH_EL_TEXT_H
#define LH_EL_TEXT_H

#include "element.h"
#include "document.h"

namespace litehtml
{
	class el_text : public element
	{
	protected:
		string			m_text;
		string			m_transformed_text;
		size			m_size;
		bool			m_use_transformed;
		bool			m_draw_spaces;
		bool			m_whole_text_node;
		bool m_hyphenation_allowed = false;
	public:
		el_text(const char* text, const document::ptr& doc, bool whole_text_node = true);

		bool is_preserved_tab() const override
		{
			return m_text == "\t" && (css().get_white_space_collapse() == white_space_preserve ||
				css().get_white_space_collapse() == white_space_break_spaces);
		}
		const string& get_source_text() const { return m_text; }
		const string& get_rendered_text() const { return m_use_transformed ? m_transformed_text : m_text; }
		bool allows_hyphenation() const { return m_hyphenation_allowed; }
		void set_hyphenation_allowed(bool allowed) { m_hyphenation_allowed = allowed; }

		void				get_text(string& text) const override;
		void				compute_styles(bool recursive) override;
		bool				is_text() const override { return true; }

		void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri) override;
		string				dump_get_name() override;
		std::vector<std::tuple<string, string>> dump_get_attrs() override;
	protected:
		void				get_content_size(size& sz, pixel_t max_width) override;
	};
}

#endif  // LH_EL_TEXT_H
