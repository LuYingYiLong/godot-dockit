#pragma once

#include <litehtml/html_tag.h>

namespace docrender {
	class HTMLStylesheetNode : public litehtml::html_tag {
	public:
		using html_tag::html_tag;
		bool style_disabled = false;
		litehtml::string linked_path;
		litehtml::string linked_text;

		bool accepts_style_text() const;
		void parse_attributes() override;
		void compute_styles(bool p_recursive) override;
		bool removeChild(const litehtml::element::ptr& p_child) override;
	};
}
