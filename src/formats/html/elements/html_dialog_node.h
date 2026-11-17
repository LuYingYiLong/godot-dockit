#pragma once

#include "html_node_copy_state.h"

#include <godot_cpp/variant/string.hpp>
#include <litehtml/html_tag.h>

namespace docrender {
	class HTMLDialogNode : public litehtml::html_tag, public HTMLNodeCopyState {
	public:
		using html_tag::html_tag;
		godot::String return_value;

		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override {
			auto* dialog = dynamic_cast<HTMLDialogNode*>(&p_target);
			if (!dialog) return false;
			if (p_mode == HTMLNodeCopyMode::ADOPT) dialog->return_value = return_value;
			return true;
		}
	};
}
