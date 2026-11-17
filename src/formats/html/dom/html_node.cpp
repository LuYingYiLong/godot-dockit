#include "html_node.h"

#include "html_dom.h"
#include "formats/html/html_layout.h"

#include <litehtml/el_text.h>

#include <vector>

using namespace godot;

void HTMLDocumentNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_base_uri"), &HTMLDocumentNode::get_base_uri);
}

String HTMLDocumentNode::get_base_uri() const {
	const auto* layout = get_owner();
	return layout ? layout->get_base_uri() : String();
}

void HTMLNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_node_type"), &HTMLNode::get_node_type);
	ClassDB::bind_method(D_METHOD("is_valid"), &HTMLNode::is_valid);
	ClassDB::bind_method(D_METHOD("is_connected"), &HTMLNode::is_connected);
	ClassDB::bind_method(D_METHOD("get_parent_node"), &HTMLNode::get_parent_node);
	ClassDB::bind_method(D_METHOD("get_child_nodes"), &HTMLNode::get_child_nodes);
	ClassDB::bind_method(D_METHOD("get_first_child"), &HTMLNode::get_first_child);
	ClassDB::bind_method(D_METHOD("get_next_sibling"), &HTMLNode::get_next_sibling);
	ClassDB::bind_method(D_METHOD("get_root_node"), &HTMLNode::get_root_node);
	ClassDB::bind_method(D_METHOD("contains", "node"), &HTMLNode::contains);
	ClassDB::bind_method(D_METHOD("compare_document_position", "node"), &HTMLNode::compare_document_position);
	ClassDB::bind_method(D_METHOD("is_equal_node", "node"), &HTMLNode::is_equal_node);
	ClassDB::bind_method(D_METHOD("normalize"), &HTMLNode::normalize);
	BIND_ENUM_CONSTANT(ELEMENT_NODE);
	BIND_ENUM_CONSTANT(TEXT_NODE);
	BIND_ENUM_CONSTANT(COMMENT_NODE);
	BIND_ENUM_CONSTANT(DOCUMENT_NODE);
	BIND_ENUM_CONSTANT(DOCUMENT_FRAGMENT_NODE);
	BIND_ENUM_CONSTANT(POSITION_DISCONNECTED);
	BIND_ENUM_CONSTANT(POSITION_PRECEDING);
	BIND_ENUM_CONSTANT(POSITION_FOLLOWING);
	BIND_ENUM_CONSTANT(POSITION_CONTAINS);
	BIND_ENUM_CONSTANT(POSITION_CONTAINED_BY);
	BIND_ENUM_CONSTANT(POSITION_IMPLEMENTATION_SPECIFIC);
}

HTMLLayout* HTMLNode::get_owner() const {
	auto* layout = Object::cast_to<HTMLLayout>(ObjectDB::get_instance(layout_id));
	return layout && layout->parse_version == parse_version ? layout : nullptr;
}

docrender::HTMLDOM& HTMLNode::get_dom() const {
	return get_owner()->get_dom();
}

std::shared_ptr<litehtml::element> HTMLNode::lock_element() const {
	return get_owner() ? element.lock() : nullptr;
}

std::shared_ptr<litehtml::element> HTMLNode::lock_related_node(const Ref<HTMLNode>& p_node) const {
	return p_node.is_valid() && p_node->get_owner() == get_owner() ? p_node->lock_element() : nullptr;
}

bool HTMLNode::is_valid() const {
	return get_owner() && !element.expired();
}

bool HTMLNode::is_connected() const {
	const auto node = lock_element();
	return node && get_dom().is_element_connected(*node);
}

Ref<HTMLNode> HTMLNode::get_parent_node() const {
	if (get_node_type() == DOCUMENT_NODE) return {};
	if (const auto node = lock_element()) {
		auto& dom = get_dom();
		if (const auto parent = node->parent()) return dom.make_node(parent);
		if (dom.is_element_connected(*node)) return dom.get_document_node();
	}
	return {};
}

TypedArray<HTMLNode> HTMLNode::get_child_nodes() const {
	TypedArray<HTMLNode> children;
	const auto node = lock_element();
	if (!node) return children;
	auto& dom = get_dom();
	if (get_node_type() == DOCUMENT_NODE) {
		children.append(dom.make_node(node));
		return children;
	}
	const litehtml::string* group = nullptr;
	for (const auto& child : node->children()) {
		const auto* text = dynamic_cast<const litehtml::el_text*>(child.get());
		const auto* current = text ? text->dom_text().get() : nullptr;
		if ((!current || current != group)) {
			if (const auto handle = dom.make_node(child); handle.is_valid()) children.append(handle);
		}
		group = current;
	}
	return children;
}

Ref<HTMLNode> HTMLNode::get_first_child() const {
	const auto children = get_child_nodes();
	return children.is_empty() ? Ref<HTMLNode>() : Ref<HTMLNode>(children[0]);
}

Ref<HTMLNode> HTMLNode::get_next_sibling() const {
	if (const auto parent = get_parent_node(); parent.is_valid()) {
		bool found = false;
		for (const Ref<HTMLNode>& child : parent->get_child_nodes()) {
			if (found) return child;
			found = child.ptr() == this;
		}
	}
	return {};
}

Ref<HTMLNode> HTMLNode::get_root_node() const {
	if (!is_valid()) return {};
	Ref<HTMLNode> root(const_cast<HTMLNode*>(this));
	for (auto parent = root->get_parent_node(); parent.is_valid(); parent = parent->get_parent_node()) root = parent;
	return root;
}

bool HTMLNode::contains(const Ref<HTMLNode>& p_node) const {
	if (!is_valid()) return false;
	for (auto node = p_node; node.is_valid(); node = node->get_parent_node()) {
		if (node.ptr() == this) return true;
	}
	return false;
}

int32_t HTMLNode::compare_document_position(const Ref<HTMLNode>& p_node) const {
	if (!is_valid() || p_node.is_null() || !p_node->is_valid()) return POSITION_DISCONNECTED;
	if (p_node.ptr() == this) return 0;
	const auto path = [](Ref<HTMLNode> p_start) {
		std::vector<Ref<HTMLNode>> result;
		while (p_start.is_valid()) {
			result.push_back(p_start);
			p_start = p_start->get_parent_node();
		}
		return result;
	};
	const auto self = path(Ref<HTMLNode>(const_cast<HTMLNode*>(this)));
	const auto other = path(p_node);
	if (self.back() != other.back()) {
		return POSITION_DISCONNECTED | POSITION_IMPLEMENTATION_SPECIFIC |
			(get_instance_id() < p_node->get_instance_id() ? POSITION_FOLLOWING : POSITION_PRECEDING);
	}
	auto a = self.rbegin();
	auto b = other.rbegin();
	while (a != self.rend() && b != other.rend() && *a == *b) {
		++a;
		++b;
	}
	if (a == self.rend()) return POSITION_CONTAINED_BY | POSITION_FOLLOWING;
	if (b == other.rend()) return POSITION_CONTAINS | POSITION_PRECEDING;
	for (const Ref<HTMLNode>& child : (*(a - 1))->get_child_nodes()) {
		if (child == *a) return POSITION_FOLLOWING;
		if (child == *b) return POSITION_PRECEDING;
	}
	return POSITION_DISCONNECTED;
}

bool HTMLNode::is_equal_node(const Ref<HTMLNode>& p_node) const {
	const auto a = lock_element();
	const auto b = p_node.is_valid() ? p_node->lock_element() : nullptr;
	if (!a || !b || get_node_type() != p_node->get_node_type()) return false;
	if (get_node_type() == TEXT_NODE || get_node_type() == COMMENT_NODE) {
		return get_dom().character_data(*a) == p_node->get_dom().character_data(*b);
	}
	if (get_node_type() == ELEMENT_NODE && (a->tag() != b->tag() ||
		get_dom().get_element_attributes(*a) != p_node->get_dom().get_element_attributes(*b))) return false;
	const auto left = get_child_nodes();
	const auto right = p_node->get_child_nodes();
	if (left.size() != right.size()) return false;
	for (int64_t i = 0; i < left.size(); ++i) {
		const Ref<HTMLNode> child = left[i];
		if (!child->is_equal_node(right[i])) return false;
	}
	return true;
}

Error HTMLNode::normalize() {
	const auto node = lock_element();
	return node ? get_dom().normalize(node) : ERR_INVALID_PARAMETER;
}

void HTMLTextNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_data"), &HTMLTextNode::get_data);
	ClassDB::bind_method(D_METHOD("set_data", "data"), &HTMLTextNode::set_data);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "data"), "set_data", "get_data");
}

String HTMLTextNode::get_data() const {
	const auto node = lock_element();
	return node ? get_dom().character_data(*node) : String();
}

Error HTMLTextNode::set_data(const String& p_data) {
	const auto node = lock_element();
	return node ? get_dom().set_character_data(node, p_data) : ERR_INVALID_PARAMETER;
}

void HTMLCommentNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_data"), &HTMLCommentNode::get_data);
	ClassDB::bind_method(D_METHOD("set_data", "data"), &HTMLCommentNode::set_data);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "data"), "set_data", "get_data");
}

String HTMLCommentNode::get_data() const {
	const auto node = lock_element();
	return node ? get_dom().character_data(*node) : String();
}

Error HTMLCommentNode::set_data(const String& p_data) {
	const auto node = lock_element();
	return node ? get_dom().set_character_data(node, p_data) : ERR_INVALID_PARAMETER;
}
