#pragma once

#include "formats/html/elements/html_element.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <litehtml/element.h>

#include <functional>
#include <memory>
#include <vector>

namespace litehtml {
	class document;
}

struct HTMLNode::DetachedTree {
	std::weak_ptr<docrender::HTMLDOM> owner;
	std::weak_ptr<litehtml::element> host;
	litehtml::element::ptr root;
	bool public_fragment = false;

	DetachedTree(std::weak_ptr<docrender::HTMLDOM> p_owner, const litehtml::element::ptr& p_root);
	~DetachedTree();
};

namespace docrender {
	class HTMLTemplateNode;

	class HTMLDOM : public std::enable_shared_from_this<HTMLDOM> {
		friend struct HTMLNode::DetachedTree;

		std::shared_ptr<litehtml::document> document;
		godot::ObjectID owner_id;
		uint64_t generation = 0;
		mutable godot::HashMap<const litehtml::element*, godot::ObjectID> node_handles;
		mutable godot::ObjectID document_handle;
		godot::HashMap<const litehtml::element*, std::weak_ptr<HTMLNode::DetachedTree>> detached_trees;
		struct SourceAttributes {
			std::weak_ptr<litehtml::element> element;
			litehtml::string_map values;
		};
		godot::HashMap<const litehtml::element*, SourceAttributes> source_attributes;
		std::function<void()> on_change;

		struct AdoptedHandle {
			const litehtml::element* source;
			litehtml::element::ptr target;
			godot::Ref<HTMLNode> handle;
		};
		struct Adoption {
			struct Fragment {
				godot::Ref<HTMLDocumentFragment> source;
				HTMLTemplateNode* target;
			};
			std::vector<AdoptedHandle> handles;
			std::vector<Fragment> fragments;
		};

		enum class SelectorQuery {
			FIRST,
			ALL,
			SELF,
			CLOSEST
		};

		godot::TypedArray<HTMLElement> select_elements(const godot::String& p_selector, SelectorQuery p_query,
			const litehtml::element::ptr& p_scope = {}) const;

		std::shared_ptr<HTMLNode::DetachedTree> get_detached_tree(const litehtml::element& p_element) const;
		std::shared_ptr<HTMLNode::DetachedTree> register_detached_tree(const litehtml::element::ptr& p_root);
		godot::Ref<HTMLDocumentFragment> create_document_fragment(const std::shared_ptr<litehtml::document>& p_document);
		bool is_host_including_ancestor(const litehtml::element& p_ancestor, litehtml::element::ptr p_node) const;
		void update_detached_handles(const litehtml::element::ptr& p_root,
			const std::shared_ptr<HTMLNode::DetachedTree>& p_tree);
		void retain_detached_subtree(const litehtml::element::ptr& p_root);
		void release_detached_root(const std::shared_ptr<HTMLNode::DetachedTree>& p_tree, const litehtml::element::ptr& p_node);
		godot::Error place_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child,
			const litehtml::element::ptr& p_reference, bool p_replace_reference, bool p_notify = true, bool* r_changed = nullptr);
		godot::Error detach_child(const litehtml::element::ptr& p_element, bool p_notify);
		void forget_subtree(const litehtml::element& p_element);
		godot::Ref<HTMLNode> find_node_handle(const litehtml::element& p_element) const;
		void bind_handle(HTMLNode& r_handle, const litehtml::element::ptr& p_node) const;
		void transfer_adoption(HTMLDOM& p_source, const litehtml::element& p_root, const Adoption& p_adoption);
		godot::String serialize(const litehtml::element& p_element, bool p_outer) const;
		godot::Ref<HTMLElement> make_detached_element(const litehtml::element::ptr& p_element);
		litehtml::element::ptr clone_subtree(const HTMLDOM& p_source_dom, const litehtml::element& p_source, bool p_deep,
			Adoption* r_adoption = nullptr);
		bool clone_children(const HTMLDOM& p_source_dom, const litehtml::element& p_source, litehtml::element& r_target,
			Adoption* r_adoption = nullptr);
		void set_source_attribute(const litehtml::element::ptr& p_element, const char* p_name, const char* p_value);
		void forget_source_attributes(const litehtml::element& p_element);

	public:
		enum class TreeEdit { APPEND, PREPEND, BEFORE, AFTER, REPLACE, REPLACE_CHILDREN };
		enum class Lookup { TAG, CLASS, NAME };
		explicit HTMLDOM(std::function<void()> p_on_change);

		void set_document(std::shared_ptr<litehtml::document> p_document, godot::ObjectID p_owner_id, uint64_t p_generation);
		const std::shared_ptr<litehtml::document>& get_document() const { return document; }
		void clear();
		void normalize_details(const litehtml::element::ptr& p_tree, const std::vector<litehtml::element::ptr>& p_moved = {}, bool p_prefer_moved = false, bool p_notify = true);
		void form_state_changed(const litehtml::element::ptr& p_node);

		static godot::CharString normalize_tag_name(const godot::String& p_tag_name);
		static bool is_element(const litehtml::element& p_element);
		template <typename Visitor>
		static bool visit_elements(const litehtml::element::ptr& p_node, const Visitor& p_visit) {
			if (!p_node || litehtml::is_generated_tag(p_node->tag())) {
				return false;
			}
			if (is_element(*p_node) && p_visit(p_node)) {
				return true;
			}
			for (const auto& child : p_node->children()) {
				if (visit_elements(child, p_visit)) {
					return true;
				}
			}
			return false;
		}

		godot::Ref<HTMLElement> make_element(const litehtml::element::ptr& p_element) const;
		godot::Ref<HTMLNode> make_node(const litehtml::element::ptr& p_node) const;
		godot::Ref<HTMLDocumentNode> get_document_node() const;
		godot::Ref<HTMLNode> create_character_node(const godot::String& p_data, bool p_comment);
		godot::String character_data(const litehtml::element& p_node) const;
		godot::Error set_character_data(const litehtml::element::ptr& p_node, const godot::String& p_data, bool p_notify = true);
		godot::Error normalize(const litehtml::element::ptr& p_node);
		godot::Error edit_nodes(const litehtml::element::ptr& p_target, TreeEdit p_edit, const godot::Array& p_items,
			bool p_notify = true);
		godot::Ref<HTMLDocumentFragment> parse_fragment(const godot::String& p_html, const litehtml::element::ptr& p_context);
		godot::Ref<HTMLDocumentFragment> import_fragment(const HTMLDocumentFragment& p_source);
		bool adopt_fragment(HTMLDocumentFragment& r_source);
		godot::TypedArray<HTMLElement> get_children(const litehtml::element& p_element) const;
		godot::TypedArray<HTMLElement> get_elements(const godot::String& p_value, Lookup p_lookup,
			const litehtml::element::ptr& p_scope = {}) const;
		godot::Ref<HTMLElement> create_element(const godot::CharString& p_tag);
		godot::Ref<HTMLDocumentFragment> create_document_fragment();
		litehtml::element::ptr create_template_node(const std::shared_ptr<litehtml::document>& p_document);
		godot::Ref<HTMLDocumentFragment> copy_fragment(const litehtml::element& p_root, bool p_deep);
		godot::Ref<HTMLElement> copy_node(const HTMLDOM& p_source_dom, const litehtml::element& p_source, bool p_deep);
		bool adopt_node(HTMLDOM& p_source_dom, const litehtml::element::ptr& p_source);
		godot::Ref<HTMLElement> query_selector(const godot::String& p_selector, const litehtml::element::ptr& p_scope = {}) const;
		godot::TypedArray<HTMLElement> query_selector_all(const godot::String& p_selector, const litehtml::element::ptr& p_scope = {}) const;
		bool matches(const godot::String& p_selector, const litehtml::element::ptr& p_scope) const;
		godot::Ref<HTMLElement> closest(const godot::String& p_selector, const litehtml::element::ptr& p_scope) const;
		godot::Ref<HTMLElement> get_element_by_id(const godot::String& p_id, const litehtml::element::ptr& p_root = {}) const;
		bool is_element_connected(const litehtml::element& p_element) const;
		void remember_source_attributes(const litehtml::element::ptr& p_element, const litehtml::string_map& p_values);
		const litehtml::string_map* get_source_attributes(const litehtml::element& p_element) const;
		godot::Dictionary get_element_attributes(const litehtml::element& p_element) const;
		const char* get_element_attribute(const litehtml::element& p_element, const char* p_name) const;
		godot::String serialize_children(const litehtml::element& p_element) const;
		godot::String serialize_element(const litehtml::element& p_element) const;
		godot::Error edit_attribute(const litehtml::element::ptr& p_element, const char* p_name, const char* p_value);
		godot::Error replace_children(const litehtml::element::ptr& p_element, const godot::String& p_content, bool p_parse_html);
		godot::Error insert_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child,
			const litehtml::element::ptr& p_reference);
		godot::Error set_table_child(const litehtml::element::ptr& p_table, const litehtml::element::ptr& p_new_child,
			const litehtml::element::ptr& p_old_child, const litehtml::element::ptr& p_reference);
		godot::Error insert_fragment(const litehtml::element::ptr& p_parent, const HTMLDocumentFragment& p_fragment,
			const litehtml::element::ptr& p_reference);
		godot::Error replace_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_new_child,
			const litehtml::element::ptr& p_old_child);
		godot::Error remove_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child);
		godot::Error detach_element(const litehtml::element::ptr& p_element);
		godot::Error remove_element(const litehtml::element::ptr& p_element);
	};
}
