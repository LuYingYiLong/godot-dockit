#pragma once

#include "html_text_editor.h"
#include "html_form_state.h"

#include <litehtml/html_tag.h>

namespace docrender {
	class HTMLTextControlNode : public litehtml::html_tag {
	protected:
		bool focused = false;

	public:
		HTMLTextEditor editor;
		HTMLTextControlNode(const std::shared_ptr<litehtml::document>& p_document, bool p_multiline) :
			html_tag(p_document), editor(p_multiline) {}

		virtual bool is_text_control() const = 0;
		virtual bool update_pseudos() = 0;
		bool is_editable() const { return is_text_control() && !forms::is_disabled(*this) && !get_attr("readonly"); }
		void set_focus_style(bool p_focused) { focused = p_focused; }
	};
}
