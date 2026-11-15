#pragma once

#include "elements/html_element.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <litehtml/element.h>

#include <functional>
#include <memory>

namespace litehtml {
	class document;
}

namespace docrender {
	class HTMLDOM : public std::enable_shared_from_this<HTMLDOM> {
		friend struct HTMLElement::DetachedTree;

		std::shared_ptr<litehtml::document> document;
		godot::ObjectID owner_id;
		uint64_t generation = 0;
		mutable godot::HashMap<const litehtml::element*, godot::ObjectID> element_handles;
		godot::HashMap<const litehtml::element*, std::weak_ptr<HTMLElement::DetachedTree>> detached_trees;
		struct SourceAttributes {
			std::weak_ptr<litehtml::element> element;
			litehtml::string_map values;
		};
		godot::HashMap<const litehtml::element*, SourceAttributes> source_attributes;
		std::function<void()> on_change;

		enum class SelectorQuery {
			FIRST,
			ALL,
			SELF,
			CLOSEST
		};

		godot::TypedArray<HTMLElement> select_elements(const godot::String& p_selector, SelectorQuery p_query,
			const litehtml::element::ptr& p_scope = {}) const;

		std::shared_ptr<HTMLElement::DetachedTree> get_detached_tree(const litehtml::element& p_element) const;
		std::shared_ptr<HTMLElement::DetachedTree> register_detached_tree(const litehtml::element::ptr& p_root);
		void update_detached_handles(const litehtml::element::ptr& p_root,
			const std::shared_ptr<HTMLElement::DetachedTree>& p_tree);
		void retain_detached_subtree(const litehtml::element::ptr& p_root);
		void release_detached_root(const std::shared_ptr<HTMLElement::DetachedTree>& p_tree, const litehtml::element::ptr& p_node);
		godot::Error place_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child,
			const litehtml::element::ptr& p_reference, bool p_replace_reference);
		void forget_subtree(const litehtml::element& p_element);
		godot::Ref<HTMLElement> make_detached_element(const litehtml::element::ptr& p_element);
		litehtml::element::ptr clone_subtree(const HTMLDOM& p_source_dom, const litehtml::element& p_source, bool p_deep);
		const litehtml::string_map* get_source_attributes(const litehtml::element& p_element) const;
		void set_source_attribute(const litehtml::element::ptr& p_element, const char* p_name, const char* p_value);
		void forget_source_attributes(const litehtml::element& p_element);

	public:
		explicit HTMLDOM(std::function<void()> p_on_change);

		void set_document(std::shared_ptr<litehtml::document> p_document, godot::ObjectID p_owner_id, uint64_t p_generation);
		const std::shared_ptr<litehtml::document>& get_document() const { return document; }
		void clear();

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
		godot::Ref<HTMLElement> create_element(const godot::CharString& p_tag);
		godot::Ref<HTMLElement> copy_node(const HTMLDOM& p_source_dom, const litehtml::element& p_source, bool p_deep);
		godot::Ref<HTMLElement> query_selector(const godot::String& p_selector, const litehtml::element::ptr& p_scope = {}) const;
		godot::TypedArray<HTMLElement> query_selector_all(const godot::String& p_selector, const litehtml::element::ptr& p_scope = {}) const;
		bool matches(const godot::String& p_selector, const litehtml::element::ptr& p_scope) const;
		godot::Ref<HTMLElement> closest(const godot::String& p_selector, const litehtml::element::ptr& p_scope) const;
		godot::Ref<HTMLElement> get_element_by_id(const godot::String& p_id) const;
		bool is_element_connected(const litehtml::element& p_element) const;
		void remember_source_attributes(const litehtml::element::ptr& p_element, const litehtml::string_map& p_values);
		godot::Dictionary get_element_attributes(const litehtml::element& p_element) const;
		const char* get_element_attribute(const litehtml::element& p_element, const char* p_name) const;
		godot::String serialize_children(const litehtml::element& p_element) const;
		godot::Error edit_attribute(const litehtml::element::ptr& p_element, const char* p_name, const char* p_value);
		godot::Error replace_children(const litehtml::element::ptr& p_element, const godot::String& p_content, bool p_parse_html);
		godot::Error insert_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child,
			const litehtml::element::ptr& p_reference);
		godot::Error replace_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_new_child,
			const litehtml::element::ptr& p_old_child);
		godot::Error remove_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child);
		godot::Error detach_element(const litehtml::element::ptr& p_element);
		godot::Error remove_element(const litehtml::element::ptr& p_element);
	};
}
