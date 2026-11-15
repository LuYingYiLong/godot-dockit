#include "html_layout.h"

#include "elements/html_input_node.h"
#include "elements/html_select_node.h"
#include "elements/html_textarea_node.h"

#include "formats/html/paint/html_canvas_renderer.h"
#include "formats/html/dom/html_dom.h"
#include "formats/html/dom/html_document_fragment.h"
#include "formats/html/resources/html_image_sources.h"
#include "formats/html/resources/html_resource_paths.h"
#include "layout/html_geometry.h"
#include "formats/html/backend/litehtml_container.h"
#include "formats/html/backend/litehtml_document_adapter.h"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <litehtml/css_parser.h>
#include <litehtml/el_image.h>
#include <litehtml/render_item.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <string_view>
#include <unordered_set>

using namespace godot;
using docrender::HTMLInputNode;
using docrender::HTMLTextAreaNode;
using docrender::HTMLTextControlNode;
using docrender::HTMLCanvasRenderer;
using docrender::HTMLDOM;
using docrender::HTMLGeometryIndex;
using docrender::HTMLHighlight;
using docrender::LiteHTMLContainer;
using DocumentAdapter = litehtml::docrender_editor;

namespace {
	litehtml::element::ptr find_html_ancestor(litehtml::element::ptr p_element) {
		while (p_element && !HTMLDOM::is_element(*p_element)) {
			p_element = p_element->parent();
		}
		return p_element;
	}

	const char* get_link_href(const litehtml::element& p_element) {
		const char* href = p_element.tag() == litehtml::_a_ ? p_element.get_attr("href") : nullptr;
		return href && *href ? href : nullptr;
	}

	litehtml::element::ptr find_link_element(litehtml::element::ptr p_element) {
		for (auto element = std::move(p_element); element; element = element->parent()) {
			if (element->tag() == litehtml::_a_) {
				return get_link_href(*element) ? element : nullptr;
			}
		}
		return {};
	}

	void collect_anchors(const litehtml::element& p_element, const HTMLGeometryIndex& p_geometry, HashMap<String, Rect2>& r_anchors) {
		if (p_element.css().get_display() == litehtml::display_none) {
			return;
		}
		const char* id = p_element.get_attr("id");
		if (id && *id) {
			const String name = String::utf8(id);
			const auto* renderers = p_geometry.find_element(&p_element);
			if (renderers && !r_anchors.has(name)) {
				std::optional<Rect2> bounds;
				const auto include = [&](const Rect2& p_rect) {
					bounds = bounds ? bounds->merge(p_rect) : p_rect;
				};
				for (const auto* renderer : *renderers) {
					if (!renderer->item->is_visible() || !renderer->inverse) {
						continue;
					}
					for (const Rect2& box : renderer->visual_boxes) {
						include(box);
					}
					if (renderer->anchor_fallback) {
						include(*renderer->anchor_fallback);
					}
				}
				if (bounds) {
					r_anchors.insert(name, *bounds);
				}
			}
		}
		for (const auto& child : p_element.children()) {
			collect_anchors(*child, p_geometry, r_anchors);
		}
	}

} // namespace

struct HTMLLayout::State {
	enum class Update { NONE, LAYOUT, STYLE };

	// Render items are cleared before the document releases its fonts through the container.
	std::shared_ptr<HTMLDOM> dom;
	std::unique_ptr<LiteHTMLContainer> container;
	Ref<HTMLCanvasRenderer> canvas_renderer;
	HTMLGeometryIndex geometry;
	HashMap<String, Rect2> anchors;
	std::vector<std::weak_ptr<litehtml::element>> hovered_elements;
	Vector2i viewport_size;
	Vector2i content_size;
	std::string master_styles;
	std::string user_styles;
	Callable edit_callback;
	String error;
	Error parse_error = OK;
	Update pending_update = Update::LAYOUT;
	bool implicit_empty_document = false;
	bool reflow_queued = false;
	bool input_paint_pending = false;
	bool has_hover_styles = false;
	bool css_diagnostics_enabled = false;

	bool has_layout() const {
		return pending_update == Update::NONE && viewport_size.x > 0;
	}
};

void HTMLLayout::_bind_methods() {
	ClassDB::bind_method(D_METHOD("parse", "html", "base_dir", "stylesheet"), &HTMLLayout::parse,
		DEFVAL(String("res://")), DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("layout", "viewport_size"), &HTMLLayout::layout);
	ClassDB::bind_method(D_METHOD("get_content_size"), &HTMLLayout::get_content_size);
	ClassDB::bind_method(D_METHOD("get_plain_text"), &HTMLLayout::get_plain_text);
	ClassDB::bind_method(D_METHOD("hit_test_text", "position"), &HTMLLayout::hit_test_text);
	ClassDB::bind_method(D_METHOD("get_selection_rects", "from", "to"), &HTMLLayout::get_selection_rects);
	ClassDB::bind_method(D_METHOD("draw", "canvas", "position", "clip"), &HTMLLayout::draw,
		DEFVAL(Vector2()), DEFVAL(Rect2()));
	ClassDB::bind_method(D_METHOD("get_link_at", "position"), &HTMLLayout::get_link_at);
	ClassDB::bind_method(D_METHOD("get_anchor_rect", "id"), &HTMLLayout::get_anchor_rect);
	ClassDB::bind_method(D_METHOD("create_element", "tag_name"), &HTMLLayout::create_element);
	ClassDB::bind_method(D_METHOD("create_document_fragment"), &HTMLLayout::create_document_fragment);
	ClassDB::bind_method(D_METHOD("import_node", "element", "deep"), &HTMLLayout::import_node, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("adopt_node", "element"), &HTMLLayout::adopt_node);
	ClassDB::bind_method(D_METHOD("query_selector", "selector"), &HTMLLayout::query_selector);
	ClassDB::bind_method(D_METHOD("query_selector_all", "selector"), &HTMLLayout::query_selector_all);
	ClassDB::bind_method(D_METHOD("get_element_by_id", "id"), &HTMLLayout::get_element_by_id);
	ClassDB::bind_method(D_METHOD("get_element_at", "position"), &HTMLLayout::get_element_at);
	ClassDB::bind_method(D_METHOD("get_render_error"), &HTMLLayout::get_render_error);
	ClassDB::bind_method(D_METHOD("set_css_diagnostics_enabled", "enabled"), &HTMLLayout::set_css_diagnostics_enabled);
	ClassDB::bind_method(D_METHOD("is_css_diagnostics_enabled"), &HTMLLayout::is_css_diagnostics_enabled);
	ClassDB::bind_method(D_METHOD("get_css_diagnostics"), &HTMLLayout::get_css_diagnostics);
	ClassDB::bind_method(D_METHOD("clear"), &HTMLLayout::clear);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "css_diagnostics_enabled"), "set_css_diagnostics_enabled", "is_css_diagnostics_enabled");
}

HTMLLayout::HTMLLayout() :
	state(std::make_unique<State>()) {
	state->dom = std::make_shared<HTMLDOM>([this] { invalidate_styles(); });
}

HTMLDOM& HTMLLayout::get_dom() const {
	return *state->dom;
}

HTMLLayout::~HTMLLayout() {
	clear();
}

Error HTMLLayout::parse(const String& p_html, const String& p_base_dir, const String& p_stylesheet) {
	return parse_source(p_html, p_base_dir, p_stylesheet, Dictionary());
}

Error HTMLLayout::parse_source(const String& p_html, const String& p_base_dir, const String& p_stylesheet,
	const Dictionary& p_textures, bool p_fit_images, const String& p_default_stylesheet, const docrender::HTMLFontConfig& p_fonts,
	const Dictionary& p_local_stylesheets, const Dictionary& p_local_fonts, const Dictionary& p_image_dpi,
	const Dictionary& p_image_orientations) {
	clear();
	const std::optional<String> base_dir = docrender::normalize_html_resource_path(p_base_dir);
	if (!base_dir) {
		state->error = "The base directory must be a project resource path (res://).";
		return state->parse_error = ERR_INVALID_PARAMETER;
	}
	try {
		state->container = std::make_unique<LiteHTMLContainer>(*state->dom, *base_dir, p_textures, p_fit_images, p_fonts, p_local_stylesheets,
			p_local_fonts, p_image_dpi, p_image_orientations);
		state->container->set_css_diagnostics_enabled(state->css_diagnostics_enabled);
		state->master_styles = litehtml::master_css;
		if (!p_default_stylesheet.is_empty()) {
			state->master_styles += '\n';
			state->master_styles += p_default_stylesheet.utf8().get_data();
		}
		const CharString stylesheet = p_stylesheet.utf8();
		state->user_styles.assign(stylesheet.get_data(), stylesheet.length());
		if (!p_html.strip_edges().is_empty() && !initialize_document(p_html.utf8())) {
			clear();
			state->error = "litehtml could not create the document.";
			return state->parse_error = ERR_PARSE_ERROR;
		}
	}
	catch (const std::exception& p_error) {
		clear();
		state->error = "Could not parse HTML. " + String::utf8(p_error.what());
		return state->parse_error = ERR_PARSE_ERROR;
	}
	return OK;
}

bool HTMLLayout::initialize_document(const CharString& p_html) {
	state->dom->set_document(DocumentAdapter::parse(p_html, *state->container, state->master_styles, state->user_styles),
		ObjectID(get_instance_id()), parse_version);
	state->has_hover_styles = state->dom->get_document() && DocumentAdapter::has_hover_styles(*state->dom->get_document());
	return bool(state->dom->get_document());
}

bool HTMLLayout::ensure_document_for_editing() {
	if (!state->container || state->parse_error != OK) {
		return false;
	}
	if (state->dom->get_document()) {
		return true;
	}
	if (!initialize_document(String("<!doctype html>").utf8())) {
		return false;
	}
	state->implicit_empty_document = true;
	return true;
}

Error HTMLLayout::layout(const Vector2i& p_viewport_size) {
	if (state->parse_error != OK) {
		return state->parse_error;
	}
	if (p_viewport_size.x <= 0 || p_viewport_size.y <= 0) {
		state->error = "The viewport width and height must be positive.";
		return ERR_INVALID_PARAMETER;
	}
	if (!state->container) {
		state->error = "Call parse() before laying out the document.";
		return ERR_UNCONFIGURED;
	}
	state->error = String();
	if (state->has_layout() && state->viewport_size == p_viewport_size) {
		return OK;
	}
	using Update = State::Update;
	if (state->pending_update == Update::NONE) {
		state->pending_update = Update::LAYOUT;
	}
	if (state->dom->get_document() && !state->implicit_empty_document) {
		try {
			const Error result = reflow(p_viewport_size);
			if (result != OK) {
				return result;
			}
		}
		catch (const std::exception& p_error) {
			state->anchors.clear();
			state->container->clear_recording();
			state->geometry.clear();
			state->error = "Could not lay out HTML. " + String::utf8(p_error.what());
			return ERR_INVALID_DATA;
		}
	}
	state->viewport_size = p_viewport_size;
	state->pending_update = Update::NONE;
	return OK;
}

Error HTMLLayout::reflow(const Vector2i& p_viewport_size) {
	using Update = State::Update;
	const auto& document = state->dom->get_document();
	state->container->set_viewport(p_viewport_size);
	litehtml::media_features image_media;
	document->container()->get_media_features(image_media);
	const docrender::HTMLImageSourceResolver resolve_image = [this](std::string_view p_source) {
		return state->container->resolve_image_source(p_source);
	};
	std::vector<litehtml::el_image*> images;
	bool image_hints_changed = false;
	std::vector<std::pair<litehtml::element::ptr, Vector2>> scroll_offsets;
	HTMLDOM::visit_elements(document->root(), [&](const litehtml::element::ptr& p_element) {
		if (p_element->tag() == litehtml::_img_) {
			if (auto* image = dynamic_cast<litehtml::el_image*>(p_element.get())) {
				image_hints_changed |= image->select_source(docrender::select_html_image(*image, resolve_image, image_media));
				images.push_back(image);
			}
		}
		if (const auto* renderers = state->geometry.find_element(p_element.get())) {
			for (const auto* renderer : *renderers) {
				const auto& item = renderer->item;
				const Vector2 offset(item->get_scroll_left(), item->get_scroll_top());
				if (offset != Vector2()) {
					scroll_offsets.emplace_back(p_element, offset);
					break;
				}
			}
		}
		return false;
	});
	state->anchors.clear();
	state->container->clear_recording();
	state->geometry.clear();
	if (state->pending_update == Update::STYLE || image_hints_changed || DocumentAdapter::media_changed(*document)) {
		state->pending_update = Update::STYLE;
		DocumentAdapter::restyle(document, *state->container, state->master_styles, state->user_styles);
		state->has_hover_styles = DocumentAdapter::has_hover_styles(*document);
		state->pending_update = Update::LAYOUT;
	}
	else {
		// Viewport units also change when all media queries keep matching.
		DocumentAdapter::compute_styles(*document);
	}
	document->render(p_viewport_size.x);
	state->geometry.rebuild(document->root_render());
	const Vector2 original_size(document->width(), document->height());
	const Vector2 overflow = state->geometry.has_transforms() ? state->geometry.extend_overflow(original_size) : original_size;
	// Style recomputation can replace render items while keeping their DOM elements.
	bool restored_scroll = false;
	for (const auto& [element, offset] : scroll_offsets) {
		if (const auto* renderers = state->geometry.find_element(element.get())) {
			for (const auto* renderer : *renderers) {
				const auto& item = renderer->item;
				if (!item->scrollable_content_size()) continue;
				restored_scroll |= item->h_scroll(offset.x - item->get_scroll_left()) != 0;
				restored_scroll |= item->v_scroll(offset.y - item->get_scroll_top()) != 0;
				break;
			}
		}
	}
	if (restored_scroll) state->geometry.rebuild(document->root_render());
	const double width = std::ceil(overflow.x);
	const double height = std::ceil(overflow.y);
	constexpr double MAX_SIZE = std::numeric_limits<int32_t>::max();
	if (!std::isfinite(width) || !std::isfinite(height) || width > MAX_SIZE || height > MAX_SIZE) {
		state->geometry.clear();
		state->error = "The HTML content size is outside the supported range.";
		return ERR_INVALID_DATA;
	}
	const Vector2i size(static_cast<int32_t>(std::max(0.0, width)), static_cast<int32_t>(std::max(0.0, height)));
	state->content_size = size;
	record_paint();
	for (auto* image : images) image->commit_source();
	return OK;
}

void HTMLLayout::record_paint() {
	state->input_paint_pending = false;
	state->anchors.clear();
	collect_anchors(*state->dom->get_document()->root(), state->geometry, state->anchors);
	state->container->record(*state->dom->get_document(), state->content_size, state->geometry);
	record_css_diagnostics();
}

Vector2i HTMLLayout::get_content_size() const {
	return state->content_size;
}

void HTMLLayout::draw(CanvasItem* p_canvas, const Vector2& p_position, const Rect2& p_clip) {
	draw_with_highlights(p_canvas, p_position, p_clip, {}, nullptr, Color());
}

String HTMLLayout::get_plain_text() const {
	return state->has_layout() ? state->container->get_paint_recorder().get_plain_text() : String();
}

int64_t HTMLLayout::hit_test_text(const Vector2& p_position) const {
	return state->has_layout() ? state->container->get_paint_recorder().hit_test_text(p_position) : 0;
}

TypedArray<Rect2> HTMLLayout::get_selection_rects(int64_t p_from, int64_t p_to) const {
	if (!state->has_layout()) {
		return {};
	}
	const int64_t length = state->container->get_paint_recorder().get_plain_text().length();
	const int64_t from = CLAMP(std::min(p_from, p_to), int64_t(0), length);
	const int64_t to = CLAMP(std::max(p_from, p_to), int64_t(0), length);
	return state->container->get_paint_recorder().get_selection_rects(from, to);
}

void HTMLLayout::draw_with_highlights(CanvasItem* p_canvas, const Vector2& p_position, const Rect2& p_clip,
	const Vector2i& p_selection, const StyleBox* p_selection_style, const Color& p_selection_color,
	const Vector2i& p_search, const StyleBox* p_search_style, const Vector2& p_viewport_origin) {
	if (!state->has_layout() || !p_canvas || !p_canvas->is_inside_tree()) {
		return;
	}
	if (state->input_paint_pending) record_paint();
	const auto& runs = state->container->get_paint_recorder().get_paint_runs();
	const auto& diagnostics = state->container->get_paint_recorder().get_diagnostic_runs();
	if (runs.empty() && diagnostics.empty()) {
		return;
	}
	if (state->canvas_renderer.is_null()) {
		state->canvas_renderer.instantiate();
	}
	const HTMLHighlight selection{ p_selection.x, p_selection.y, p_selection_style, p_selection_color };
	const HTMLHighlight search{ p_search.x, p_search.y, p_search_style, Color(0, 0, 0, 0) };
	state->canvas_renderer->submit(*p_canvas, p_position, p_clip, runs, diagnostics, selection, search, p_viewport_origin);
}

String HTMLLayout::get_link_at(const Vector2& p_position) const {
	return find_link_at(p_position).uri;
}

Rect2 HTMLLayout::get_anchor_rect(const String& p_id) const {
	const Rect2* bounds = state->anchors.getptr(p_id);
	return bounds ? *bounds : Rect2();
}

const HashMap<String, Rect2>& HTMLLayout::get_anchor_rects() const {
	return state->anchors;
}

Ref<HTMLElement> HTMLLayout::create_element(const String& p_tag_name) {
	const CharString tag = HTMLDOM::normalize_tag_name(p_tag_name);
	if (tag.length() == 0) {
		return {};
	}
	try {
		if (ensure_document_for_editing()) {
			return state->dom->create_element(tag);
		}
	}
	catch (const std::exception&) {
		return {};
	}
	return {};
}

Ref<HTMLDocumentFragment> HTMLLayout::create_document_fragment() {
	try {
		if (ensure_document_for_editing()) {
			return state->dom->create_document_fragment();
		}
	}
	catch (const std::exception&) {
		return {};
	}
	return {};
}

Ref<HTMLElement> HTMLLayout::import_node(const Ref<HTMLElement>& p_element, bool p_deep) {
	const auto node = p_element.is_valid() ? p_element->lock_element() : nullptr;
	if (!node) {
		return {};
	}
	try {
		if (ensure_document_for_editing()) {
			return state->dom->copy_node(p_element->get_dom(), *node, p_deep);
		}
	}
	catch (const std::exception&) {
		return {};
	}
	return {};
}

Ref<HTMLElement> HTMLLayout::adopt_node(const Ref<HTMLElement>& p_element) {
	const auto node = p_element.is_valid() ? p_element->lock_element() : nullptr;
	if (!node) {
		return {};
	}
	auto& source = p_element->get_dom();
	const auto& source_document = source.get_document();
	if (!source_document || source_document->root() == node) {
		return {};
	}
	try {
		if (ensure_document_for_editing() && state->dom->adopt_node(source, node)) {
			return p_element;
		}
	}
	catch (const std::exception&) {
		return {};
	}
	return {};
}

void HTMLLayout::set_edit_callback(const Callable& p_callback) {
	state->edit_callback = p_callback;
}

void HTMLLayout::invalidate_styles() {
	mark_styles_pending();
	state->implicit_empty_document = false;
	if (state->edit_callback.is_valid()) {
		state->edit_callback.call();
	}
	else {
		queue_reflow();
	}
}

void HTMLLayout::mark_styles_pending() {
	state->pending_update = State::Update::STYLE;
	state->anchors.clear();
}

void HTMLLayout::queue_reflow() {
	if (state->viewport_size != Vector2i() && !state->reflow_queued) {
		state->reflow_queued = true;
		callable_mp_static(&HTMLLayout::reflow_after_edit).call_deferred(get_instance_id(), parse_version);
	}
}

void HTMLLayout::reflow_after_edit(uint64_t p_id, uint64_t p_version) {
	auto* owner = Object::cast_to<HTMLLayout>(ObjectDB::get_instance(ObjectID(p_id)));
	if (!owner || owner->parse_version != p_version) {
		return;
	}
	State& current = *owner->state;
	current.reflow_queued = false;
	if (!current.has_layout() && current.viewport_size != Vector2i()) {
		owner->layout(current.viewport_size);
	}
	if (current.canvas_renderer.is_valid()) {
		current.canvas_renderer->queue_redraws();
	}
}

Ref<HTMLElement> HTMLLayout::query_selector(const String& p_selector) const {
	return state->dom->query_selector(p_selector);
}

TypedArray<HTMLElement> HTMLLayout::query_selector_all(const String& p_selector) const {
	return state->dom->query_selector_all(p_selector);
}

Ref<HTMLElement> HTMLLayout::get_element_by_id(const String& p_id) const {
	return state->dom->get_element_by_id(p_id);
}

TypedArray<Rect2> HTMLLayout::get_element_rects(const litehtml::element::ptr& p_element) const {
	TypedArray<Rect2> result;
	if (const auto* renderers = state->geometry.find_element(p_element.get()); state->has_layout() && renderers) {
		for (const auto* renderer : *renderers) {
			if (!renderer->item->skip()) {
				for (const Rect2& box : renderer->visual_boxes) {
					result.append(box);
				}
			}
		}
	}
	return result;
}

Ref<HTMLElement> HTMLLayout::get_element_at(const Vector2& p_position) const {
	return wrap_hit_element(hit_test_element(p_position));
}

Ref<HTMLElement> HTMLLayout::wrap_hit_element(litehtml::element::ptr p_element) const {
	for (auto ancestor = p_element; ancestor; ancestor = ancestor->parent()) {
		if (litehtml::is_generated_tag(ancestor->tag())) {
			p_element = ancestor->parent();
		}
	}
	return state->dom->make_element(find_html_ancestor(std::move(p_element)));
}

litehtml::element::ptr HTMLLayout::hit_test_element(const Vector2& p_position) const {
	const Rect2 bounds(Vector2(), Vector2(state->content_size.max(state->viewport_size)));
	if (!state->has_layout() || !state->dom->get_document() || !p_position.is_finite() || !bounds.has_point(p_position)) {
		return {};
	}
	const auto root = state->dom->get_document()->root_render();
	if (!root) {
		return {};
	}
	if (state->geometry.has_transforms()) {
		return state->container->get_paint_recorder().hit_test_element(p_position);
	}
	const auto accepts_geometry = [&](const docrender::RenderGeometry* p_geometry) {
		return p_geometry && p_geometry->item->is_visible() && p_geometry->contains_visible_point(p_position);
	};
	const auto accepts = [&](const std::shared_ptr<litehtml::render_item>& p_item) {
		// Some litehtml fallback paths offer another fragment of the same element.
		return accepts_geometry(state->geometry.find_renderer(p_item.get()));
	};
	struct SkippedItems {
		std::vector<litehtml::render_item*> items;
		~SkippedItems() {
			for (auto* item : items) {
				item->skip(false);
			}
		}
	};
	SkippedItems skipped;
	for (;;) {
		auto element = root->get_element_by_point(p_position.x, p_position.y, p_position.x, p_position.y, accepts);
		if (!element) {
			return {};
		}
		const auto* renderers = state->geometry.find_element(element.get());
		if (!renderers) {
			return {};
		}
		if (std::any_of(renderers->begin(), renderers->end(), accepts_geometry)) {
			return element;
		}

		// litehtml's float fallback bypasses the filter. Exclude its rejected boxes and retry in the same order.
		const size_t previous_size = skipped.items.size();
		for (const auto* renderer : *renderers) {
			auto* item = renderer->item.get();
			if (item->is_visible()) {
				skipped.items.push_back(item);
				item->skip(true);
			}
		}
		if (skipped.items.size() == previous_size) {
			return {};
		}
	}
}

Vector2 HTMLLayout::wheel_delta(const InputEventMouseButton& p_event) {
	if (!p_event.is_pressed()) return {};
	Vector2 delta;
	switch (p_event.get_button_index()) {
		case MOUSE_BUTTON_WHEEL_UP: delta.y = -1; break;
		case MOUSE_BUTTON_WHEEL_DOWN: delta.y = 1; break;
		case MOUSE_BUTTON_WHEEL_LEFT: delta.x = -1; break;
		case MOUSE_BUTTON_WHEEL_RIGHT: delta.x = 1; break;
		default: return {};
	}
	if (p_event.is_shift_pressed() && delta.y != 0) delta = Vector2(delta.y, 0);
	const float factor = p_event.get_factor();
	if (!std::isfinite(factor)) return {};
	delta *= 36.0f * (factor > 0 ? factor : 1.0f);
	if (!delta.is_finite()) return {};

	return delta;
}

bool HTMLLayout::scroll_overflow_from_wheel(const Vector2& p_position, const InputEventMouseButton& p_event) {
	return scroll_overflow(p_position, wheel_delta(p_event)) != Vector2();
}

Vector2 HTMLLayout::scroll_overflow(const Vector2& p_position, const Vector2& p_delta) {
	if (p_delta == Vector2() || !state->has_layout()) return {};
	flush_input_paint();

	std::shared_ptr<litehtml::render_item> target;
	const auto hit = hit_test_element(p_position);
	for (auto element = hit; element && !target; element = element->parent()) {
		const auto* renderers = state->geometry.find_element(element.get());
		if (!renderers) continue;
		for (const auto* renderer : *renderers) {
			if (renderer->item->is_visible() && renderer->contains_visible_point(p_position)) {
				target = renderer->item;
				break;
			}
		}
	}

	Vector2 remaining = p_delta;
	if (auto* textarea = dynamic_cast<HTMLTextAreaNode*>(hit.get())) {
		const auto overflow = textarea->css().get_overflow();
		if (overflow == litehtml::overflow_auto || overflow == litehtml::overflow_scroll) {
			remaining -= textarea->editor.scroll_by(remaining);
		}
	}
	const Vector2 after_field = remaining;
	for (auto item = target; item && remaining != Vector2(); item = item->parent()) {
		if (item->is_h_scrollable(remaining.x)) remaining.x -= item->h_scroll(remaining.x);
		if (item->is_v_scrollable(remaining.y)) remaining.y -= item->v_scroll(remaining.y);
	}
	if (remaining == p_delta) return {};
	if (remaining != after_field) state->geometry.rebuild(state->dom->get_document()->root_render());
	record_paint();
	if (state->canvas_renderer.is_valid()) state->canvas_renderer->queue_redraws();
	return p_delta - remaining;
}

std::optional<Rect2> HTMLLayout::get_element_scroll_bounds(const Ref<HTMLElement>& p_element) const {
	if (!state->has_layout() || p_element.is_null() || p_element->get_owner() != this) {
		return std::nullopt;
	}
	const auto element = p_element->element.lock();
	const auto* renderers = state->geometry.find_element(element.get());
	std::optional<Rect2> bounds;
	if (renderers) {
		for (const auto* renderer : *renderers) {
			if (renderer->item->is_visible()) {
				for (const auto& fragment : renderer->fragments) {
					if (renderer->has_visible_area(fragment.rect)) {
						const Rect2 box = renderer->matrix.xform(fragment.rect);
						bounds = bounds ? bounds->merge(box) : box;
					}
				}
			}
		}
	}
	return bounds;
}

HTMLLayout::LinkHit HTMLLayout::find_link_at(const Vector2& p_position, Ref<HTMLElement>* r_element) const {
	auto element = hit_test_element(p_position);
	if (r_element) {
		*r_element = wrap_hit_element(element);
	}
	if (const auto link = find_link_element(std::move(element))) {
		return { String::utf8(link->get_attr("href")), reinterpret_cast<uintptr_t>(link.get()) };
	}
	return {};
}

String HTMLLayout::get_activatable_link_uri(const Ref<HTMLElement>& p_element) const {
	if (p_element.is_null() || p_element->get_owner() != this) return {};
	const auto link = p_element->lock_element();
	const char* href = link ? get_link_href(*link) : nullptr;
	return href && get_element_scroll_bounds(p_element) ? String::utf8(href) : String();
}

std::string_view HTMLLayout::get_cursor_at(const Vector2& p_position, bool& r_over_link) const {
	const auto element = find_html_ancestor(hit_test_element(p_position));
	r_over_link = bool(find_link_element(element));
	return element ? std::string_view(element->css().get_cursor()) : std::string_view();
}

Ref<HTMLElement> HTMLLayout::get_button_ancestor(const Ref<HTMLElement>& p_element) const {
	if (p_element.is_valid() && p_element->get_owner() == this) {
		for (auto element = p_element->element.lock(); element; element = element->parent()) {
			if (DocumentAdapter::is_command_button(*element)) {
				return state->dom->make_element(element);
			}
		}
	}
	return {};
}

bool HTMLLayout::is_enabled_button(const Ref<HTMLElement>& p_element) const {
	if (p_element.is_null() || p_element->get_owner() != this) {
		return false;
	}
	const auto element = p_element->element.lock();
	return element && DocumentAdapter::is_command_button(*element) && !docrender::forms::is_disabled(*element) &&
		get_element_scroll_bounds(p_element).has_value();
}

std::vector<docrender::HTMLSelectMenuRow> HTMLLayout::select_menu_rows(const Ref<HTMLElement>& p_element) const {
	std::vector<docrender::HTMLSelectMenuRow> result;
	if (!is_focusable(p_element) || !state->container) return result;
	const auto select = std::dynamic_pointer_cast<docrender::HTMLSelectNode>(p_element->lock_element());
	if (!select) return result;
	const auto to_color = [](const litehtml::web_color& p_color) {
		return Color(p_color.red / 255.0f, p_color.green / 255.0f, p_color.blue / 255.0f, p_color.alpha / 255.0f);
	};
	const auto make_row = [&](const litehtml::element& p_node, const String& p_text, int64_t p_index, bool p_enabled) {
		const auto& style = p_node.css();
		const auto font = style.get_font() ? style.get_font() : select->css().get_font();
		const auto& bg = style.get_bg().m_color;
		result.push_back({state->container->shape_input(font, p_text), to_color(style.get_color()),
			to_color(bg.alpha ? bg : select->css().get_bg().m_color), p_index, p_enabled});
	};
	litehtml::element::ptr previous_group;
	int64_t index = 0;
	for (const auto& option : select->get_options()) {
		const int64_t option_index = index++;
		const auto parent = option->parent();
		const auto group = parent.get() != select.get() ? parent : nullptr;
		if (option->get_attr("hidden") || (group && group->get_attr("hidden"))) continue;
		if (group && group != previous_group) make_row(*group, String::utf8(group->get_attr("label", "")), -1, false);
		previous_group = group;
		make_row(*option, option->get_label(), option_index, !docrender::forms::is_disabled(*option));
	}
	return result;
}

Vector2 HTMLLayout::get_image_intrinsic_size(const HTMLElement& p_element) const {
	if (!state->has_layout()) return {};
	const auto element = p_element.lock_element();
	auto* image = dynamic_cast<litehtml::el_image*>(element.get());
	if (!image || !state->dom->is_element_connected(*image)) return {};
	litehtml::size size{};
	image->get_content_size(size, 0);
	return Vector2(size.width, size.height);
}

String HTMLLayout::get_image_current_src(const HTMLElement& p_element) const {
	const auto element = p_element.lock_element();
	const auto* image = dynamic_cast<const litehtml::el_image*>(element.get());
	return image && state->dom->is_element_connected(*image) ? String::utf8(image->get_current_src().c_str()) : String();
}

namespace {
	std::optional<int64_t> explicit_tab_index(const litehtml::element& p_element) {
		const String value = String::utf8(p_element.get_attr("tabindex", "")).strip_edges();
		return value.is_valid_int() ? std::optional<int64_t>(value.to_int()) : std::nullopt;
	}

	bool can_focus_node(const litehtml::element& p_node) {
		const auto tag = p_node.tag();
		if (p_node.get_attr("hidden")) return false;
		if (docrender::forms::is_listed(p_node) && docrender::forms::is_disabled(p_node)) return false;
		if (tag == litehtml::_textarea_ || tag == litehtml::_select_) return true;
		if (tag == litehtml::_input_) {
			const auto* input = dynamic_cast<const HTMLInputNode*>(&p_node);
			return input && input->type() != HTMLInputNode::Type::OTHER && input->type() != HTMLInputNode::Type::HIDDEN;
		}
		return explicit_tab_index(p_node).has_value() || DocumentAdapter::is_command_button(p_node) ||
			(tag == litehtml::_a_ && p_node.get_attr("href"));
	}
}

bool HTMLLayout::is_focusable(const Ref<HTMLElement>& p_element) const {
	if (p_element.is_null() || p_element->get_owner() != this) return false;
	const auto element = p_element->element.lock();
	return element && can_focus_node(*element) && get_element_scroll_bounds(p_element).has_value();
}

bool HTMLLayout::is_element_connected(const Ref<HTMLElement>& p_element) const {
	if (p_element.is_null() || p_element->get_owner() != this) {
		return false;
	}
	const auto element = p_element->element.lock();
	return element && state->dom->is_element_connected(*element);
}

Ref<HTMLElement> HTMLLayout::get_focus_ancestor(const Ref<HTMLElement>& p_element) const {
	if (!p_element.is_valid() || p_element->get_owner() != this) return {};
	for (auto node = p_element->element.lock(); node; node = node->parent()) {
		if (!can_focus_node(*node)) continue;
		const auto handle = state->dom->make_element(node);
		if (get_element_scroll_bounds(handle)) return handle;
	}
	return {};
}

std::vector<Ref<HTMLElement>> HTMLLayout::get_tab_elements(bool& r_has_focusable_elements) const {
	struct TabElement {
		Ref<HTMLElement> element;
		int64_t index;
	};
	std::vector<TabElement> elements;
	r_has_focusable_elements = false;
	if (state->has_layout() && state->dom->get_document()) {
		HTMLDOM::visit_elements(state->dom->get_document()->root(), [&](const litehtml::element::ptr& p_element) {
			if (!can_focus_node(*p_element)) return false;
			const auto handle = state->dom->make_element(p_element);
			if (get_element_scroll_bounds(handle)) {
				r_has_focusable_elements = true;
				const int64_t index = explicit_tab_index(*p_element).value_or(0);
				if (const auto* radio = dynamic_cast<const HTMLInputNode*>(p_element.get()); radio && radio->type() == HTMLInputNode::Type::RADIO) {
					litehtml::element::ptr tab_stop;
					for (const auto& peer : docrender::forms::radio_group(*radio)) {
						if (!can_focus_node(*peer) || explicit_tab_index(*peer).value_or(0) < 0 ||
							!get_element_scroll_bounds(state->dom->make_element(peer))) continue;
						if (!tab_stop) tab_stop = peer;
						if (static_cast<const HTMLInputNode&>(*peer).is_checked()) {
							tab_stop = peer;
							break;
						}
					}
					if (tab_stop != p_element) return false;
				}
				if (index >= 0) elements.push_back({handle, index});
			}
			return false;
		});
	}
	std::stable_sort(elements.begin(), elements.end(), [](const TabElement& p_left, const TabElement& p_right) {
		if (p_left.index == 0 || p_right.index == 0) return p_left.index > p_right.index;
		return p_left.index < p_right.index;
	});
	std::vector<Ref<HTMLElement>> result;
	result.reserve(elements.size());
	for (auto& entry : elements) result.push_back(std::move(entry.element));
	return result;
}

std::shared_ptr<HTMLTextControlNode> HTMLLayout::text_control(const Ref<HTMLElement>& p_element) const {
	if (p_element.is_null() || p_element->get_owner() != this) return {};
	return std::dynamic_pointer_cast<HTMLTextControlNode>(p_element->element.lock());
}

std::optional<Transform2D> HTMLLayout::input_inverse(const HTMLElement& p_input) const {
	if (!state->has_layout() || p_input.get_owner() != this) return std::nullopt;
	const auto element = p_input.element.lock();
	const auto* renderers = state->geometry.find_element(element.get());
	if (!renderers || renderers->empty()) return std::nullopt;
	return renderers->front()->inverse;
}

std::optional<Vector2> HTMLLayout::input_local_point(const HTMLElement& p_input, const Vector2& p_point) const {
	const auto inverse = input_inverse(p_input);
	return inverse ? std::optional<Vector2>(inverse->xform(p_point)) : std::nullopt;
}

std::optional<Vector2> HTMLLayout::input_caret_point(const HTMLElement& p_input) const {
	const auto inverse = input_inverse(p_input);
	const auto node = std::dynamic_pointer_cast<HTMLTextControlNode>(p_input.lock_element());
	if (!inverse || !node) return std::nullopt;
	const auto caret = node->editor.get_caret_rect();
	if (!caret.has_area()) return std::nullopt;
	const auto viewport = node->editor.get_viewport();
	if (!viewport.has_area()) return std::nullopt;
	const Vector2 point = Vector2(caret.position.x, caret.get_end().y).clamp(viewport.position, viewport.get_end());
	return inverse->affine_inverse().xform(point);
}

void HTMLLayout::flush_input_paint() {
	if (state->has_layout() && state->input_paint_pending) record_paint();
}

bool HTMLLayout::resize_textarea(const Ref<HTMLElement>& p_element, const Vector2& p_size, const Vector2i& p_axes) {
	if (!p_size.is_finite() || p_axes == Vector2i()) return false;
	const auto node = text_control(p_element);
	if (!node || !node->editor.is_multiline()) return false;

	litehtml::raw_declaration::vector declarations;
	litehtml::raw_rule::vector rules;
	litehtml::css_parser(litehtml::normalize(node->get_attr("style", ""), litehtml::f_componentize))
		.consume_style_block_contents(declarations, rules);
	litehtml::string style;
	for (const auto& declaration : declarations) {
		const auto name = litehtml::lowcase(declaration.name);
		if ((p_axes.x && name == "width") || (p_axes.y && name == "height")) continue;
		style += declaration.name + ": " + litehtml::get_repr(declaration.value);
		if (declaration.important) style += " !important";
		style += "; ";
	}
	if (p_axes.x) style += "width: " + std::to_string(std::max(0.0f, p_size.x)) + "px; ";
	if (p_axes.y) style += "height: " + std::to_string(std::max(0.0f, p_size.y)) + "px;";
	return state->dom->edit_attribute(node, "style", style.c_str()) == OK;
}

void HTMLLayout::repaint_input() {
	state->input_paint_pending = true;
	if (state->canvas_renderer.is_valid()) state->canvas_renderer->queue_redraws();
}

void HTMLLayout::update_input(HTMLTextControlNode& p_input) {
	const bool style_changed = p_input.update_pseudos();
	if (!state->dom->is_element_connected(p_input)) return;
	if (style_changed) invalidate_styles();
	else repaint_input();
}

bool HTMLLayout::set_interaction_pseudo(const Ref<HTMLElement>& p_element, const char* p_name, bool p_enabled) {
	if (p_element.is_null() || p_element->get_owner() != this) {
		return false;
	}
	const auto element = p_element->element.lock();
	if (auto* input = dynamic_cast<HTMLTextControlNode*>(element.get()); input && std::string_view(p_name) == "focus") {
		input->set_focus_style(p_enabled);
	}
	return element && element->set_pseudo_class(litehtml::_id(p_name), p_enabled);
}

bool HTMLLayout::update_hover(const std::optional<Vector2>& p_position) {
	const auto target = p_position ? find_html_ancestor(hit_test_element(*p_position)) : nullptr;
	std::vector<std::weak_ptr<litehtml::element>> next;
	for (auto element = target; element; element = find_html_ancestor(element->parent())) {
		next.emplace_back(element);
	}
	auto& previous = state->hovered_elements;
	if (std::equal(previous.begin(), previous.end(), next.begin(), next.end(),
			[](const auto& p_left, const auto& p_right) { return p_left.lock() == p_right.lock(); })) {
		return false;
	}

	std::unordered_set<const litehtml::element*> hovered;
	for (const auto& entry : next) {
		hovered.insert(entry.lock().get());
	}
	for (const auto& entry : previous) {
		if (const auto element = entry.lock(); element && !hovered.erase(element.get())) {
			DocumentAdapter::set_hover(*element, false);
		}
	}
	for (auto element = target; element; element = find_html_ancestor(element->parent())) {
		if (hovered.count(element.get())) {
			DocumentAdapter::set_hover(*element, true);
		}
	}
	previous = std::move(next);
	if (!state->has_hover_styles) {
		return false;
	}
	mark_styles_pending();
	return true;
}

String HTMLLayout::get_render_error() const {
	return state->error;
}

void HTMLLayout::set_css_diagnostics_enabled(bool p_enabled) {
	if (state->css_diagnostics_enabled == p_enabled) {
		return;
	}
	state->css_diagnostics_enabled = p_enabled;
	if (!state->container) {
		return;
	}
	state->container->set_css_diagnostics_enabled(p_enabled);
	if (p_enabled && state->dom->get_document()) {
		state->pending_update = State::Update::STYLE;
		if (!state->edit_callback.is_valid()) {
			queue_reflow();
		}
	}
	else if (!p_enabled && state->canvas_renderer.is_valid()) {
		state->canvas_renderer->queue_redraws();
	}
}

bool HTMLLayout::is_css_diagnostics_enabled() const {
	return state->css_diagnostics_enabled;
}

TypedArray<Dictionary> HTMLLayout::get_css_diagnostics() const {
	return state->container ? state->container->get_css_diagnostics() : TypedArray<Dictionary>();
}

TypedArray<Dictionary> HTMLLayout::get_css_diagnostics_for(const litehtml::element& p_element) const {
	TypedArray<Dictionary> result;
	if (state->container && state->pending_update != State::Update::STYLE) {
		if (const auto* entries = state->container->get_element_diagnostics(p_element)) {
			for (const auto index : *entries) {
				result.append(state->container->get_css_diagnostic(index).duplicate());
			}
		}
	}
	return result;
}

void HTMLLayout::record_css_diagnostics() {
	if (!state->css_diagnostics_enabled) {
		return;
	}
	HTMLDOM::visit_elements(state->dom->get_document()->root(), [&](const litehtml::element::ptr& p_element) {
		const auto* renderers = state->geometry.find_element(p_element.get());
		if (renderers && state->container->get_element_diagnostics(*p_element)) {
			for (const auto* renderer : *renderers) {
				if (!renderer->item->is_visible()) {
					continue;
				}
				for (const auto& fragment : renderer->fragments) {
					if (fragment.rect.has_area()) {
						state->container->record_css_diagnostic(*renderer, fragment.rect);
					}
				}
			}
		}
		return false;
	});
}

String HTMLLayout::get_css_diagnostic_tooltip(const Vector2& p_position) const {
	if (!state->css_diagnostics_enabled) {
		return {};
	}
	for (auto node = find_html_ancestor(hit_test_element(p_position)); node; node = find_html_ancestor(node->parent())) {
		const auto* entries = state->container->get_element_diagnostics(*node);
		if (!entries) {
			continue;
		}
		String element = String::utf8(node->get_tagName());
		if (const char* id = state->dom->get_element_attribute(*node, "id"); id && *id) {
			element += "#" + String::utf8(id);
		}
		String html = "<div class='element'>" + element.xml_escape() + "</div>";
		for (const auto index : *entries) {
			const Dictionary& entry = state->container->get_css_diagnostic(index);
			const String path = entry["path"];
			const String source = path.is_empty() ? String(entry["source"]) : path;
			const String kind = entry["kind"];
			const String property = entry["property"];
			const String value = entry["value"];
			html += "<div class='diagnostic'><div class='kind'>" + kind.xml_escape() + "</div>" +
				"<div>" + property.xml_escape() + ": " + value.xml_escape() + "</div>" +
				"<div class='source'>" + source.xml_escape() + "</div></div>";
		}
		return html;
	}
	return {};
}

void HTMLLayout::clear() {
	++parse_version;
	state->canvas_renderer.unref();
	if (state->container) {
		state->container->clear_recording();
	}
	state->geometry.clear();
	state->dom->clear();
	state->hovered_elements.clear();
	state->has_hover_styles = false;
	state->container.reset();
	state->anchors.clear();
	state->viewport_size = Vector2i();
	state->content_size = Vector2i();
	state->error = String();
	state->parse_error = OK;
	state->pending_update = State::Update::LAYOUT;
	state->implicit_empty_document = false;
	state->reflow_queued = false;
	state->master_styles.clear();
	state->user_styles.clear();
}
