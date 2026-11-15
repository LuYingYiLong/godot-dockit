#include "html_document_fragment.h"

#include "html_dom.h"

using namespace godot;

void HTMLDocumentFragment::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_child", "child"), &HTMLDocumentFragment::append_child);
	ClassDB::bind_method(D_METHOD("get_children"), &HTMLDocumentFragment::get_children);
}

Error HTMLDocumentFragment::append_child(const Ref<HTMLElement>& p_child) {
	const auto dom = tree && tree->root ? tree->owner.lock() : nullptr;
	const auto node = p_child.is_valid() ? p_child->lock_element() : nullptr;
	if (!dom || !node || &p_child->get_dom() != dom.get()) {
		return ERR_INVALID_PARAMETER;
	}
	return dom->insert_child(tree->root, node, {});
}

TypedArray<HTMLElement> HTMLDocumentFragment::get_children() const {
	if (const auto dom = tree && tree->root ? tree->owner.lock() : nullptr) {
		return dom->get_children(*tree->root);
	}
	return {};
}
