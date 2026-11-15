#include "html_dom.h"
#include "html_document_fragment.h"

#include "formats/html/css/css_litehtml_dom.h"

#include "formats/html/elements/html_anchor_element.h"
#include "formats/html/elements/html_button_element.h"
#include "formats/html/elements/html_element_types.h"
#include "formats/html/elements/html_heading_element.h"
#include "formats/html/elements/html_form_elements.h"
#include "formats/html/elements/html_form_state.h"
#include "formats/html/elements/html_select_node.h"
#include "formats/html/elements/html_image_element.h"
#include "formats/html/elements/html_input_element.h"
#include "formats/html/elements/html_template_element.h"
#include "formats/html/elements/html_template_node.h"
#include "formats/html/elements/html_textarea_element.h"
#include "formats/html/elements/html_input_node.h"
#include "formats/html/elements/html_node_copy_state.h"
#include "formats/html/elements/html_textarea_node.h"
#include "formats/html/backend/litehtml_document_adapter.h"

#include <litehtml/css_parser.h>
#include <litehtml/css_selector.h>
#include <litehtml/document.h>
#include <litehtml/el_cdata.h>
#include <litehtml/el_comment.h>
#include <litehtml/el_space.h>
#include <litehtml/html_tag.h>

#include <algorithm>
#include <exception>
#include <iterator>
#include <string_view>
#include <utility>

using namespace godot;
using docrender::HTMLDOM;
using DocumentAdapter = litehtml::docrender_editor;

namespace {
	bool is_valid_tag_name(std::string_view p_name) {
		const auto is_letter = [](unsigned char p_byte) {
			return (p_byte >= 'A' && p_byte <= 'Z') || (p_byte >= 'a' && p_byte <= 'z');
		};
		if (p_name.empty() || !is_letter(p_name.front())) {
			return false;
		}
		return std::all_of(p_name.begin(), p_name.end(), [&](unsigned char p_byte) {
			return is_letter(p_byte) || (p_byte >= '0' && p_byte <= '9') ||
				p_byte == '-' || p_byte == '_' || p_byte == '.' || p_byte == ':' || p_byte >= 0x80;
		});
	}

	bool is_void_element(const litehtml::element& p_element) {
		const std::string_view tag = p_element.get_tagName();
		static constexpr std::string_view void_tags[] = {
			"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param",
			"source", "track", "wbr", "basefont", "bgsound", "frame", "keygen",
		};
		return std::find(std::begin(void_tags), std::end(void_tags), tag) != std::end(void_tags);
	}

	bool can_contain_elements(const litehtml::element& p_element) {
		const std::string_view tag = p_element.get_tagName();
		return !is_void_element(p_element) && tag != "style" && tag != "script" &&
			tag != "title" && tag != "template" && tag != "textarea" && tag != "xmp" && tag != "iframe" &&
			tag != "noembed" && tag != "noframes" && tag != "plaintext";
	}

	bool has_raw_text(const litehtml::element& p_element) {
		const std::string_view tag = p_element.get_tagName();
		return tag == "style" || tag == "script" || tag == "xmp" || tag == "iframe" ||
			tag == "noembed" || tag == "noframes" || tag == "plaintext";
	}

	void append_escaped(std::string& r_html, std::string_view p_value, bool p_attribute) {
		for (const char byte : p_value) {
			switch (byte) {
			case '&': r_html += "&amp;"; break;
			case '<': r_html += "&lt;"; break;
			case '>': r_html += "&gt;"; break;
			case '"': r_html += p_attribute ? "&quot;" : "\""; break;
			default: r_html += byte; break;
			}
		}
	}

	const litehtml::element* tree_root(const litehtml::element& p_element) {
		const litehtml::element* root = &p_element;
		for (auto parent = root->parent(); parent; parent = parent->parent()) {
			root = parent.get();
		}
		return root;
	}

	litehtml::element::ptr clone_character_data(const litehtml::element& p_source, const litehtml::document::ptr& p_document) {
		if (const auto* space = dynamic_cast<const litehtml::el_space*>(&p_source)) {
			return std::make_shared<litehtml::el_space>(space->get_source_text().c_str(), p_document);
		}
		if (const auto* text = dynamic_cast<const litehtml::el_text*>(&p_source)) {
			return std::make_shared<litehtml::el_text>(text->get_source_text().c_str(), p_document, text->is_whole_text_node());
		}
		litehtml::element::ptr copy;
		if (p_source.is_comment()) {
			copy = std::make_shared<litehtml::el_comment>(p_document);
		}
		else if (dynamic_cast<const litehtml::el_cdata*>(&p_source)) {
			copy = std::make_shared<litehtml::el_cdata>(p_document);
		}
		if (copy) {
			litehtml::string text;
			p_source.get_text(text);
			copy->set_data(text.c_str());
		}
		return copy;
	}

	template <typename Element>
	Ref<HTMLElement> instantiate_element_handle() {
		Ref<Element> handle;
		handle.instantiate();
		return handle;
	}

	Ref<HTMLElement> create_element_handle(litehtml::string_id p_tag) {
		switch (p_tag) {
			case litehtml::_a_: return instantiate_element_handle<HTMLAnchorElement>();
			case litehtml::_button_: return instantiate_element_handle<HTMLButtonElement>();
			case litehtml::_form_: return instantiate_element_handle<HTMLFormElement>();
			case litehtml::_label_: return instantiate_element_handle<HTMLLabelElement>();
			case litehtml::_fieldset_: return instantiate_element_handle<HTMLFieldSetElement>();
			case litehtml::_legend_: return instantiate_element_handle<HTMLLegendElement>();
			case litehtml::_select_: return instantiate_element_handle<HTMLSelectElement>();
			case litehtml::_option_: return instantiate_element_handle<HTMLOptionElement>();
			case litehtml::_optgroup_: return instantiate_element_handle<HTMLOptGroupElement>();
			case litehtml::_div_: return instantiate_element_handle<HTMLDivElement>();
			case litehtml::_p_: return instantiate_element_handle<HTMLParagraphElement>();
			case litehtml::_span_: return instantiate_element_handle<HTMLSpanElement>();
			case litehtml::_h1_:
			case litehtml::_h2_:
			case litehtml::_h3_:
			case litehtml::_h4_:
			case litehtml::_h5_:
			case litehtml::_h6_: return instantiate_element_handle<HTMLHeadingElement>();
			case litehtml::_img_: return instantiate_element_handle<HTMLImageElement>();
			case litehtml::_input_: return instantiate_element_handle<HTMLInputElement>();
			case litehtml::_template_: return instantiate_element_handle<HTMLTemplateElement>();
			case litehtml::_textarea_: return instantiate_element_handle<HTMLTextAreaElement>();
			default: return instantiate_element_handle<HTMLElement>();
		}
	}

} // namespace

HTMLElement::DetachedTree::DetachedTree(std::weak_ptr<HTMLDOM> p_owner, const litehtml::element::ptr& p_root) :
	owner(std::move(p_owner)), root(p_root) {}

HTMLElement::DetachedTree::~DetachedTree() {
	if (const auto dom = owner.lock(); root && dom) {
		dom->detached_trees.erase(root.get());
		HTMLDOM::visit_elements(root, [&](const litehtml::element::ptr& p_element) {
			dom->element_handles.erase(p_element.get());
			dom->forget_source_attributes(*p_element);
			return false;
		});
	}
}

HTMLDOM::HTMLDOM(std::function<void()> p_on_change) :
	on_change(std::move(p_on_change)) {}

void HTMLDOM::set_document(litehtml::document::ptr p_document, ObjectID p_owner_id, uint64_t p_generation) {
	document = std::move(p_document);
	owner_id = p_owner_id;
	generation = p_generation;
	for (const auto& entry : detached_trees) {
		if (const auto tree = entry.value.lock()) docrender::forms::synchronize(tree->root);
	}
}

void HTMLDOM::form_state_changed(const litehtml::element::ptr& p_node) {
	docrender::forms::synchronize(p_node);
	if (is_element_connected(*p_node)) on_change();
}

void HTMLDOM::clear() {
	element_handles.clear();
	// Releasing one root can destroy nested template fragments and unregister them.
	std::vector<std::shared_ptr<HTMLElement::DetachedTree>> trees;
	for (const auto& entry : detached_trees) {
		if (auto tree = entry.value.lock()) {
			trees.push_back(std::move(tree));
		}
	}
	detached_trees.clear();
	for (const auto& tree : trees) {
		tree->root.reset();
	}
	source_attributes.clear();
	document.reset();
}

bool HTMLDOM::is_element(const litehtml::element& p_element) {
	const auto tag = p_element.tag();
	return tag != litehtml::empty_id && !litehtml::is_generated_tag(tag) && *p_element.get_tagName();
}

std::shared_ptr<HTMLElement::DetachedTree> HTMLDOM::get_detached_tree(const litehtml::element& p_element) const {
	const auto* tree = detached_trees.getptr(tree_root(p_element));
	return tree ? tree->lock() : nullptr;
}

std::shared_ptr<HTMLElement::DetachedTree> HTMLDOM::register_detached_tree(const litehtml::element::ptr& p_root) {
	auto tree = std::make_shared<HTMLElement::DetachedTree>(weak_from_this(), p_root);
	detached_trees.insert(p_root.get(), tree);
	return tree;
}

void HTMLDOM::update_detached_handles(const litehtml::element::ptr& p_root,
	const std::shared_ptr<HTMLElement::DetachedTree>& p_tree) {
	visit_elements(p_root, [&](const litehtml::element::ptr& p_element) {
		if (const ObjectID* id = element_handles.getptr(p_element.get())) {
			if (auto* handle = Object::cast_to<HTMLElement>(ObjectDB::get_instance(*id))) {
				handle->detached_tree = p_tree;
			}
		}
		return false;
	});
}

void HTMLDOM::retain_detached_subtree(const litehtml::element::ptr& p_root) {
	update_detached_handles(p_root, register_detached_tree(p_root));
}

bool HTMLDOM::is_element_connected(const litehtml::element& p_element) const {
	return document && document->root().get() == tree_root(p_element);
}

Ref<HTMLElement> HTMLDOM::find_element_handle(const litehtml::element& p_element) const {
	if (const ObjectID* id = element_handles.getptr(&p_element)) {
		auto* existing = Object::cast_to<HTMLElement>(ObjectDB::get_instance(*id));
		if (existing && existing->element.lock().get() == &p_element) {
			return Ref<HTMLElement>(existing);
		}
	}
	return {};
}

Ref<HTMLElement> HTMLDOM::make_element(const litehtml::element::ptr& p_element) const {
	if (!p_element || !is_element(*p_element)) {
		return {};
	}
	if (const auto existing = find_element_handle(*p_element); existing.is_valid()) {
		return existing;
	}
	Ref<HTMLElement> handle = create_element_handle(p_element->tag());
	handle->layout_id = owner_id;
	handle->parse_version = generation;
	handle->element = p_element;
	handle->detached_tree = get_detached_tree(*p_element);
	element_handles.insert(p_element.get(), ObjectID(handle->get_instance_id()));
	return handle;
}

TypedArray<HTMLElement> HTMLDOM::get_children(const litehtml::element& p_element) const {
	TypedArray<HTMLElement> children;
	for (const auto& child : p_element.children()) {
		if (const auto handle = make_element(child); handle.is_valid()) {
			children.append(handle);
		}
	}
	return children;
}

CharString HTMLDOM::normalize_tag_name(const String& p_tag_name) {
	CharString tag = p_tag_name.utf8();
	if (!is_valid_tag_name(std::string_view(tag.get_data(), tag.length()))) {
		return {};
	}
	for (char* byte = tag.ptrw(); *byte; ++byte) {
		if (*byte >= 'A' && *byte <= 'Z') {
			*byte += 'a' - 'A';
		}
	}
	return tag;
}

Ref<HTMLElement> HTMLDOM::create_element(const CharString& p_tag) {
	return make_detached_element(document->create_element(p_tag.get_data(), {}));
}

Ref<HTMLDocumentFragment> HTMLDOM::create_document_fragment() {
	return create_document_fragment(document);
}

Ref<HTMLDocumentFragment> HTMLDOM::create_document_fragment(const litehtml::document::ptr& p_document) {
	Ref<HTMLDocumentFragment> fragment;
	fragment.instantiate();
	// An unnamed native tag can hold children without exposing an HTMLElement handle.
	fragment->tree = register_detached_tree(std::make_shared<litehtml::html_tag>(p_document));
	return fragment;
}

litehtml::element::ptr HTMLDOM::create_template_node(const litehtml::document::ptr& p_document) {
	// Initial parsing uses a document that HTMLDOM has yet to install.
	const auto fragment = create_document_fragment(p_document);
	const auto node = std::make_shared<HTMLTemplateNode>(p_document, fragment);
	fragment->tree->host = node;
	return node;
}

Ref<HTMLDocumentFragment> HTMLDOM::copy_fragment(const litehtml::element& p_root, bool p_deep) {
	const auto fragment = create_document_fragment();
	if (p_deep && !clone_children(*this, p_root, *fragment->tree->root)) {
		return {};
	}
	docrender::forms::synchronize(fragment->tree->root);
	return fragment;
}

Ref<HTMLElement> HTMLDOM::make_detached_element(const litehtml::element::ptr& p_element) {
	Ref<HTMLElement> handle = make_element(p_element);
	if (handle.is_valid()) {
		handle->detached_tree = register_detached_tree(p_element);
	}
	return handle;
}

Ref<HTMLElement> HTMLDOM::copy_node(const HTMLDOM& p_source_dom, const litehtml::element& p_source, bool p_deep) {
	const auto copy = clone_subtree(p_source_dom, p_source, p_deep);
	docrender::forms::synchronize(copy);
	return make_detached_element(copy);
}

bool HTMLDOM::adopt_node(HTMLDOM& p_source_dom, const litehtml::element::ptr& p_source) {
	const auto source_tree = p_source_dom.get_detached_tree(*p_source);
	const auto parent = p_source->parent();
	if (!parent && !source_tree) {
		return false;
	}
	const bool source_connected = p_source_dom.is_element_connected(*p_source);
	Adoption adoption;
	const auto copy = clone_subtree(p_source_dom, *p_source, true, &adoption);
	if (!copy) {
		return false;
	}
	// Stage the detached target before editing the source. Its owner cleans up on failure.
	const auto target_tree = register_detached_tree(copy);
	for (const auto& entry : adoption.handles) {
		element_handles.insert(entry.target.get(), ObjectID(entry.handle->get_instance_id()));
	}
	if (parent && !parent->removeChild(p_source)) {
		return false;
	}
	p_source_dom.release_detached_root(source_tree, p_source);
	for (const auto& entry : adoption.handles) {
		p_source_dom.element_handles.erase(entry.source);
		entry.handle->layout_id = owner_id;
		entry.handle->parse_version = generation;
		++entry.handle->adoption_version;
		entry.handle->element = entry.target;
		entry.handle->detached_tree = get_detached_tree(*entry.target);
	}
	p_source_dom.forget_subtree(*p_source);
	for (const auto& entry : adoption.fragments) {
		// Keep the public fragment object and transfer its ownership to the copied host.
		entry.source->tree = std::move(entry.target->content->tree);
		entry.target->content = entry.source;
	}
	docrender::forms::synchronize(copy, copy);
	docrender::forms::synchronize(parent);
	if (source_connected) {
		p_source_dom.on_change();
	}
	return true;
}

litehtml::element::ptr HTMLDOM::clone_subtree(const HTMLDOM& p_source_dom, const litehtml::element& p_source, bool p_deep,
	Adoption* r_adoption) {
	if (!is_element(p_source)) {
		return clone_character_data(p_source, document);
	}
	const auto* source_attributes = p_source_dom.get_source_attributes(p_source);
	const litehtml::string_map attributes = source_attributes ? *source_attributes : litehtml::string_map();
	const auto copy = document->create_element(p_source.get_tagName(), attributes);
	if (!copy) {
		return {};
	}
	if (p_deep) {
		const auto* source_template = dynamic_cast<const HTMLTemplateNode*>(&p_source);
		const auto& source = source_template ? *source_template->content->tree->root : p_source;
		if (!clone_children(p_source_dom, source, *copy, r_adoption)) {
			forget_subtree(*copy);
			return {};
		}
	}
	const auto mode = r_adoption ? HTMLNodeCopyMode::ADOPT : HTMLNodeCopyMode::COPY;
	if (const auto* state = dynamic_cast<const HTMLNodeCopyState*>(&p_source); state && !state->copy_state_to(*copy, mode)) {
		forget_subtree(*copy);
		return {};
	}
	if (r_adoption) {
		if (const auto handle = p_source_dom.find_element_handle(p_source); handle.is_valid()) {
			r_adoption->handles.push_back({ &p_source, copy, handle });
		}
		if (const auto* source_template = dynamic_cast<const HTMLTemplateNode*>(&p_source)) {
			r_adoption->fragments.push_back({ source_template->content, static_cast<HTMLTemplateNode*>(copy.get()) });
		}
	}
	return copy;
}

bool HTMLDOM::clone_children(const HTMLDOM& p_source_dom, const litehtml::element& p_source, litehtml::element& r_target,
	Adoption* r_adoption) {
	for (const auto& child : p_source.children()) {
		if (litehtml::is_generated_tag(child->tag())) {
			continue;
		}
		const auto copy = clone_subtree(p_source_dom, *child, true, r_adoption);
		if (!copy || !r_target.appendChild(copy)) {
			if (copy) {
				forget_subtree(*copy);
			}
			return false;
		}
	}
	return true;
}

bool HTMLDOM::is_host_including_ancestor(const litehtml::element& p_ancestor, litehtml::element::ptr p_node) const {
	while (p_node) {
		if (p_node.get() == &p_ancestor) {
			return true;
		}
		if (auto parent = p_node->parent()) {
			p_node = std::move(parent);
		} else {
			const auto tree = get_detached_tree(*p_node);
			p_node = tree ? tree->host.lock() : nullptr;
		}
	}
	return false;
}

void HTMLDOM::release_detached_root(const std::shared_ptr<HTMLElement::DetachedTree>& p_tree, const litehtml::element::ptr& p_node) {
	if (p_tree && p_tree->root == p_node) {
		detached_trees.erase(p_node.get());
		p_tree->root.reset();
	}
}

void HTMLDOM::forget_subtree(const litehtml::element& p_element) {
	if (const ObjectID* id = element_handles.getptr(&p_element)) {
		if (auto* handle = Object::cast_to<HTMLElement>(ObjectDB::get_instance(*id))) {
			handle->element.reset();
			handle->detached_tree.reset();
		}
		element_handles.erase(&p_element);
	}
	forget_source_attributes(p_element);
	for (const auto& child : p_element.children()) {
		forget_subtree(*child);
	}
	if (const auto* node = dynamic_cast<const HTMLTemplateNode*>(&p_element)) {
		const auto& tree = node->content->tree;
		if (const auto root = tree->root) {
			forget_subtree(*root);
			release_detached_root(tree, root);
		}
	}
}

TypedArray<HTMLElement> HTMLDOM::select_elements(const String& p_selector, SelectorQuery p_query,
	const litehtml::element::ptr& p_scope) const {
	TypedArray<HTMLElement> matches;
	const CharString utf8 = p_selector.utf8();
	const auto tokens = litehtml::tokenize(std::string(utf8.get_data(), utf8.length()));
	ERR_FAIL_COND_V_MSG(!docrender::css::has_balanced_selector_tokens(tokens), matches, "Invalid HTML selector: " + p_selector);
	namespace syntax = docrender::css;
	const auto selectors = syntax::parse_selector_list(syntax::normalize(tokens, syntax::f_componentize),
		syntax::strict_mode | syntax::forbid_pseudo_elements,
		litehtml::selector_mode(document ? document->mode() : litehtml::no_quirks_mode));
	ERR_FAIL_COND_V_MSG(selectors.empty(), matches, "Invalid or unsupported HTML selector: " + p_selector);
	if (!document) {
		return matches;
	}
	const auto root = p_scope ? p_scope : document->root();
	const syntax::MatchContext context{root.get(), syntax::MatchMode::complete, p_scope && !is_element(*p_scope)};
	const auto& dom_view = litehtml::selector_dom_view();
	const auto collect = [&](const litehtml::element::ptr& p_element) {
		if (syntax::match_selector_list(dom_view, p_element.get(), selectors, context) == syntax::match_none) {
			return false;
		}
		matches.append(make_element(p_element));
		return p_query != SelectorQuery::ALL;
	};

	if (p_query == SelectorQuery::SELF || p_query == SelectorQuery::CLOSEST) {
		for (auto element = root; element; element = element->parent()) {
			if ((HTMLDOM::is_element(*element) && collect(element)) || p_query == SelectorQuery::SELF) {
				break;
			}
		}
	}
	else if (p_scope) {
		for (const auto& child : p_scope->children()) {
			if (HTMLDOM::visit_elements(child, collect)) {
				break;
			}
		}
	}
	else {
		HTMLDOM::visit_elements(root, collect);
	}
	return matches;
}

Ref<HTMLElement> HTMLDOM::query_selector(const String& p_selector, const litehtml::element::ptr& p_scope) const {
	const auto matches = select_elements(p_selector, SelectorQuery::FIRST, p_scope);
	return matches.is_empty() ? Ref<HTMLElement>() : Ref<HTMLElement>(matches[0]);
}

TypedArray<HTMLElement> HTMLDOM::query_selector_all(const String& p_selector, const litehtml::element::ptr& p_scope) const {
	return select_elements(p_selector, SelectorQuery::ALL, p_scope);
}

bool HTMLDOM::matches(const String& p_selector, const litehtml::element::ptr& p_scope) const {
	return !select_elements(p_selector, SelectorQuery::SELF, p_scope).is_empty();
}

Ref<HTMLElement> HTMLDOM::closest(const String& p_selector, const litehtml::element::ptr& p_scope) const {
	const auto matches = select_elements(p_selector, SelectorQuery::CLOSEST, p_scope);
	return matches.is_empty() ? Ref<HTMLElement>() : Ref<HTMLElement>(matches[0]);
}

Ref<HTMLElement> HTMLDOM::get_element_by_id(const String& p_id, const litehtml::element::ptr& p_root) const {
	if (!document || p_id.is_empty()) {
		return {};
	}
	const CharString id = p_id.utf8();
	litehtml::element::ptr match;
	HTMLDOM::visit_elements(p_root ? p_root : document->root(), [&](const litehtml::element::ptr& p_element) {
		const char* candidate = get_element_attribute(*p_element, "id");
		if (candidate && std::string_view(candidate) == std::string_view(id.get_data(), id.length())) {
			match = p_element;
		}
		return match != nullptr;
	});
	return make_element(match);
}

void HTMLDOM::remember_source_attributes(const litehtml::element::ptr& p_element, const litehtml::string_map& p_values) {
	source_attributes.insert(p_element.get(), SourceAttributes{ p_element, p_values });
}

const litehtml::string_map* HTMLDOM::get_source_attributes(const litehtml::element& p_element) const {
	const auto* source = source_attributes.getptr(&p_element);
	return source && !source->element.expired() ? &source->values : nullptr;
}

void HTMLDOM::set_source_attribute(const litehtml::element::ptr& p_element, const char* p_name, const char* p_value) {
	auto* source = source_attributes.getptr(p_element.get());
	if (p_value) {
		if (!source || source->element.expired()) {
			source_attributes.insert(p_element.get(), SourceAttributes{ p_element, {} });
			source = source_attributes.getptr(p_element.get());
		}
		source->values[p_name] = p_value;
	}
	else if (source) {
		source->values.erase(p_name);
	}
}

void HTMLDOM::forget_source_attributes(const litehtml::element& p_element) {
	source_attributes.erase(&p_element);
}

Dictionary HTMLDOM::get_element_attributes(const litehtml::element& p_element) const {
	Dictionary result;
	if (const auto* attributes = get_source_attributes(p_element)) {
		for (const auto& [name, value] : *attributes) {
			result[String::utf8(name.c_str())] = String::utf8(value.c_str());
		}
	}
	return result;
}

const char* HTMLDOM::get_element_attribute(const litehtml::element& p_element, const char* p_name) const {
	if (const auto* attributes = get_source_attributes(p_element)) {
		const auto found = attributes->find(p_name);
		if (found != attributes->end()) {
			return found->second.c_str();
		}
	}
	return p_element.get_attr(p_name);
}

Error HTMLDOM::edit_attribute(const litehtml::element::ptr& p_element, const char* p_name, const char* p_value) {
	const auto* source = get_source_attributes(*p_element);
	const char* previous = nullptr;
	if (source) {
		const auto found = source->find(p_name);
		if (found != source->end()) {
			previous = found->second.c_str();
		}
	}
	if ((!previous && !p_value) || (previous && p_value && std::string_view(previous) == p_value)) {
		return OK;
	}
	if (!DocumentAdapter::set_attribute(*p_element, p_name, p_value)) {
		return ERR_UNAVAILABLE;
	}
	set_source_attribute(p_element, p_name, p_value);
	if (auto* input = dynamic_cast<HTMLInputNode*>(p_element.get())) {
		input->attribute_changed(p_name);
	} else if (auto* textarea = dynamic_cast<HTMLTextAreaNode*>(p_element.get())) {
		textarea->update_pseudos();
	}
	const std::string_view name(p_name);
	if (name == "selected") {
		if (auto* option = dynamic_cast<docrender::HTMLOptionNode*>(p_element.get())) option->attribute_changed();
	}
	if (name == "checked" || name == "type" || name == "name" || name == "form" || name == "id" || name == "disabled" || name == "selected") {
		docrender::forms::synchronize(p_element, p_element);
	}
	if (is_element_connected(*p_element)) {
		on_change();
	}
	return OK;
}

String HTMLDOM::serialize_children(const litehtml::element& p_element) const {
	std::string html;
	const auto append_children = [&](const auto& p_append_children, const litehtml::element& p_parent) -> void {
		const auto* node = dynamic_cast<const HTMLTemplateNode*>(&p_parent);
		const auto& contents = node ? *node->content->tree->root : p_parent;
		for (const auto& child : contents.children()) {
			if (litehtml::is_generated_tag(child->tag())) {
				continue;
			}
			if (HTMLDOM::is_element(*child)) {
				const char* tag = child->get_tagName();
				html += '<';
				html += tag;
				if (const auto* attributes = get_source_attributes(*child)) {
					for (const auto& [name, value] : *attributes) {
						html += ' ';
						html += name;
						html += "=\"";
						append_escaped(html, value, true);
						html += '"';
					}
				}
				html += '>';
				if (!is_void_element(*child)) {
					p_append_children(p_append_children, *child);
					html += "</";
					html += tag;
					html += '>';
				}
			}
			else {
				std::string text;
				child->get_text(text);
				if (child->is_comment()) {
					html += "<!--" + text + "-->";
				}
				else if (has_raw_text(p_parent)) {
					html += text;
				}
				else {
					append_escaped(html, text, false);
				}
			}
		}
	};
	append_children(append_children, p_element);
	return String::utf8(html.data(), html.size());
}

Error HTMLDOM::replace_children(const litehtml::element::ptr& p_element, const String& p_content, bool p_parse_html) {
	const auto* node = dynamic_cast<const HTMLTemplateNode*>(p_element.get());
	if (is_void_element(*p_element) || (node && !p_parse_html)) {
		return p_content.is_empty() ? OK : ERR_INVALID_PARAMETER;
	}
	const CharString content = p_content.utf8();
	if (std::string_view(content.get_data(), content.length()).find('\0') != std::string_view::npos) {
		return ERR_INVALID_PARAMETER;
	}
	litehtml::elements_list children;
	try {
		const bool split = p_element->tag() != litehtml::_textarea_ && !has_raw_text(*p_element);
		children = p_parse_html ? DocumentAdapter::parse_fragment(*document, *p_element, content) :
			DocumentAdapter::create_text(document, content.get_data(), split);
	}
	catch (const std::exception&) {
		return ERR_PARSE_ERROR;
	}
	const auto& target = node ? node->content->tree->root : p_element;
	for (const auto& child : target->children()) {
		forget_subtree(*child);
	}
	DocumentAdapter::replace_children(*target, std::move(children));
	if (auto* textarea = dynamic_cast<HTMLTextAreaNode*>(p_element.get())) {
		textarea->sync_default_value();
	}
	docrender::forms::synchronize(target, target);
	if (!node && is_element_connected(*p_element)) {
		on_change();
	}
	return OK;
}

Error HTMLDOM::insert_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child,
	const litehtml::element::ptr& p_reference) {
	return place_child(p_parent, p_child, p_reference, false);
}

Error HTMLDOM::insert_fragment(const litehtml::element::ptr& p_parent, const HTMLDocumentFragment& p_fragment,
	const litehtml::element::ptr& p_reference) {
	const auto& tree = p_fragment.tree;
	if (!tree || !tree->root || tree->owner.lock().get() != this || !can_contain_elements(*p_parent) ||
		(p_reference && p_reference->parent() != p_parent)) {
		return ERR_INVALID_PARAMETER;
	}
	const auto& root = tree->root;
	if (is_host_including_ancestor(*root, p_parent)) {
		return ERR_INVALID_PARAMETER;
	}

	const auto& children = root->children();
	if (children.empty()) {
		return OK;
	}
	const auto target_tree = get_detached_tree(*p_parent);
	auto child = children.begin();
	const size_t count = children.size();
	if (!DocumentAdapter::insert_children(*root, *p_parent, p_reference)) {
		return ERR_INVALID_PARAMETER;
	}
	// Splice preserves the iterators to the moved children, now in the target list.
	for (size_t i = 0; i < count; ++i, ++child) {
		update_detached_handles(*child, target_tree);
	}
	docrender::forms::synchronize(p_parent, p_parent);
	if (!target_tree) {
		on_change();
	}
	return OK;
}

Error HTMLDOM::replace_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_new_child,
	const litehtml::element::ptr& p_old_child) {
	return place_child(p_parent, p_new_child, p_old_child, true);
}

Error HTMLDOM::place_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child,
	const litehtml::element::ptr& p_reference, bool p_replace_reference) {
	if (!can_contain_elements(*p_parent) || (p_replace_reference && !p_reference) ||
		(p_reference && p_reference->parent() != p_parent)) {
		return ERR_INVALID_PARAMETER;
	}
	if (is_host_including_ancestor(*p_child, p_parent)) {
		return ERR_INVALID_PARAMETER;
	}

	const auto source_tree = get_detached_tree(*p_child);
	const auto old_parent = p_child->parent();
	if (!source_tree && !p_child->parent()) {
		return ERR_INVALID_PARAMETER;
	}
	const auto target_tree = get_detached_tree(*p_parent);
	using EditResult = DocumentAdapter::child_edit_result;
	const auto result = DocumentAdapter::place_child(p_child, *p_parent, p_reference, p_replace_reference);
	if (result != EditResult::changed) {
		return result == EditResult::unchanged ? OK : ERR_INVALID_PARAMETER;
	}
	if (source_tree != target_tree) {
		release_detached_root(source_tree, p_child);
		update_detached_handles(p_child, target_tree);
	}
	if (p_replace_reference) {
		retain_detached_subtree(p_reference);
		docrender::forms::synchronize(p_reference);
	}
	docrender::forms::synchronize(p_parent, p_child);
	if (old_parent && tree_root(*old_parent) != tree_root(*p_parent)) docrender::forms::synchronize(old_parent);
	if (!source_tree || !target_tree) {
		on_change();
	}
	return OK;
}

Error HTMLDOM::remove_child(const litehtml::element::ptr& p_parent, const litehtml::element::ptr& p_child) {
	return p_child->parent() == p_parent ? detach_element(p_child) : ERR_INVALID_PARAMETER;
}

Error HTMLDOM::detach_element(const litehtml::element::ptr& p_element) {
	const auto parent = p_element->parent();
	const auto old_tree = get_detached_tree(*p_element);
	if (!parent) {
		return old_tree ? OK : ERR_INVALID_PARAMETER;
	}
	if (!parent->removeChild(p_element)) {
		return ERR_INVALID_PARAMETER;
	}
	retain_detached_subtree(p_element);
	docrender::forms::synchronize(parent);
	docrender::forms::synchronize(p_element);
	if (!old_tree) {
		on_change();
	}
	return OK;
}

Error HTMLDOM::remove_element(const litehtml::element::ptr& p_element) {
	const auto parent = p_element->parent();
	const auto old_tree = get_detached_tree(*p_element);
	if (!parent) {
		return old_tree ? OK : ERR_INVALID_PARAMETER;
	}
	if (!parent->removeChild(p_element)) {
		return ERR_INVALID_PARAMETER;
	}
	forget_subtree(*p_element);
	docrender::forms::synchronize(parent);
	if (!old_tree) {
		on_change();
	}
	return OK;
}
