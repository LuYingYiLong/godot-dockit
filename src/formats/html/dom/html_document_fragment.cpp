#include "html_document_fragment.h"

#include "html_dom.h"

using namespace godot;

void HTMLDocumentFragment::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_child", "child"), &HTMLDocumentFragment::append_child);
	ClassDB::bind_method(D_METHOD("get_children"), &HTMLDocumentFragment::get_children);
	ClassDB::bind_method(D_METHOD("clone_node", "deep"), &HTMLDocumentFragment::clone_node, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("query_selector", "selector"), &HTMLDocumentFragment::query_selector);
	ClassDB::bind_method(D_METHOD("query_selector_all", "selector"), &HTMLDocumentFragment::query_selector_all);
	ClassDB::bind_method(D_METHOD("get_element_by_id", "id"), &HTMLDocumentFragment::get_element_by_id);
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

Ref<HTMLDocumentFragment> HTMLDocumentFragment::clone_node(bool p_deep) const {
	if (const auto dom = tree && tree->root ? tree->owner.lock() : nullptr) {
		return dom->copy_fragment(*tree->root, p_deep);
	}
	return {};
}

Ref<HTMLElement> HTMLDocumentFragment::query_selector(const String& p_selector) const {
	if (const auto dom = tree && tree->root ? tree->owner.lock() : nullptr) {
		return dom->query_selector(p_selector, tree->root);
	}
	return {};
}

TypedArray<HTMLElement> HTMLDocumentFragment::query_selector_all(const String& p_selector) const {
	if (const auto dom = tree && tree->root ? tree->owner.lock() : nullptr) {
		return dom->query_selector_all(p_selector, tree->root);
	}
	return {};
}

Ref<HTMLElement> HTMLDocumentFragment::get_element_by_id(const String& p_id) const {
	if (const auto dom = tree && tree->root ? tree->owner.lock() : nullptr) {
		return dom->get_element_by_id(p_id, tree->root);
	}
	return {};
}
