#include "html_label.h"

#include "formats/html/elements/html_input_node.h"
#include "formats/html/elements/html_textarea_node.h"
#include "formats/html/dom/html_dom.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/window.hpp>

#include <algorithm>
#include <utility>

using namespace godot;
using docrender::HTMLInputNode;
using docrender::HTMLTextAreaNode;

namespace {
	Window* find_native_ime_window(Viewport* p_viewport) {
		while (p_viewport) {
			if (auto* window = Object::cast_to<Window>(p_viewport)) {
				if (!window->is_embedded()) return window;

				// Find the actual embedder, which can be above the parent viewport.
				do {
					Node* parent = p_viewport->get_parent();
					p_viewport = parent ? parent->get_viewport() : nullptr;
				} while (p_viewport && !p_viewport->is_embedding_subwindows());
			} else {
				auto* container = Object::cast_to<SubViewportContainer>(p_viewport->get_parent());
				// An offscreen texture has no unique screen position.
				p_viewport = container ? container->get_viewport() : nullptr;
			}
		}
		return nullptr;
	}
}

void HTMLLabelCanvas::set_input_caret(const Ref<HTMLElement>& p_input, const Vector2& p_point, bool p_extend) {
	label->layout->flush_input_paint();
	const auto input = label->layout->text_control(p_input);
	const auto local = label->layout->input_local_point(*p_input.ptr(), p_point);
	if (input && local) {
		input->editor.hit_caret(*local, p_extend);
		label->layout->repaint_input();
		queue_redraw();
	}
}

void HTMLLabelCanvas::change_input(const Ref<HTMLElement>& p_input) {
	if (const auto input = label->layout->text_control(p_input)) {
		label->layout->update_input(*input);
		queue_redraw();
		label->emit_signal(input->editor.is_multiline() ? "textarea_changed" : "input_changed", p_input);
	}
}

void HTMLLabelCanvas::commit_input(const Ref<HTMLElement>& p_input, bool p_deferred) {
	const auto input = label->layout->text_control(p_input);
	if (!input || !input->editor.take_user_change()) return;
	const StringName signal = input->editor.is_multiline() ? "textarea_committed" : "input_committed";
	if (p_deferred) {
		const ObjectID id(label->get_instance_id());
		callable_mp_static(+[](uint64_t p_id, const Ref<HTMLElement>& element, uint64_t p_adoption_version,
			uint64_t p_reset_version, const StringName& p_signal) {
			auto* owner = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(ObjectID(p_id)));
			if (owner && element->adoption_version == p_adoption_version) {
				const auto input = owner->layout->text_control(element);
				if (input && input->editor.get_reset_version() == p_reset_version) owner->emit_signal(p_signal, element);
			}
		}).call_deferred(uint64_t(id), p_input, p_input->adoption_version, input->editor.get_reset_version(), signal);
	} else {
		label->emit_signal(signal, p_input);
	}
}

void HTMLLabelCanvas::toggle_checkbox(const Ref<HTMLInputElement>& p_input) {
	const auto input = p_input.is_valid() ? p_input->get_input() : nullptr;
	if (!input || !input->is_toggle() || docrender::forms::is_disabled(*input) ||
		!input->set_checked(input->type() == HTMLInputNode::Type::RADIO || !input->is_checked())) return;
	const ObjectID id(label->get_instance_id());
	const uint64_t reset_version = input->editor.get_reset_version();
	change_input(p_input);
	if (auto* owner = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id)); owner && owner->layout->text_control(p_input) == input &&
		input->editor.get_reset_version() == reset_version) {
		owner->emit_signal("input_committed", p_input);
	}
}

bool HTMLLabelCanvas::move_radio(const Ref<HTMLInputElement>& p_input, int p_direction) {
	const auto input = p_input->get_input();
	if (!input) return false;
	std::vector<Ref<HTMLInputElement>> available;
	for (const auto& node : docrender::forms::radio_group(*input)) {
		const Ref<HTMLInputElement> peer = label->layout->get_dom().make_element(node);
		if (label->layout->is_focusable(peer)) available.push_back(peer);
	}
	const auto current = std::find(available.begin(), available.end(), p_input);
	if (current == available.end()) return false;
	const auto count = static_cast<ptrdiff_t>(available.size());
	const Ref<HTMLInputElement> next = available[(std::distance(available.begin(), current) + count + p_direction) % count];
	set_focused_element(next, true);
	toggle_checkbox(next);
	return true;
}

bool HTMLLabelCanvas::handle_input_key(const InputEventKey& p_key) {
	const Ref<HTMLElement> handle = focused_element;
	const auto input = label->layout->text_control(handle);
	if (!input || !label->layout->is_focusable(handle)) return false;
	const Key key = p_key.get_keycode() != KEY_NONE ? p_key.get_keycode() : p_key.get_physical_keycode();
	if (const auto* toggle = dynamic_cast<const HTMLInputNode*>(input.get()); toggle && toggle->is_toggle()) {
		if (p_key.is_pressed() && toggle->type() == HTMLInputNode::Type::RADIO && !p_key.is_alt_pressed() &&
			!p_key.is_meta_pressed() && !p_key.is_ctrl_pressed() &&
			(key == KEY_LEFT || key == KEY_UP || key == KEY_RIGHT || key == KEY_DOWN)) {
			label->accept_event();
			return move_radio(handle, key == KEY_LEFT || key == KEY_UP ? -1 : 1);
		}
		if (key != KEY_SPACE || p_key.is_alt_pressed() || p_key.is_meta_pressed() || p_key.is_ctrl_pressed()) return false;
		label->accept_event();
		if (p_key.is_echo()) return true;
		if (p_key.is_pressed()) {
			checkbox_key_pressed = true;
			set_element_pseudo(handle, "active", true);
		} else if (checkbox_key_pressed) {
			cancel_key_press();
			if (!p_key.is_canceled()) toggle_checkbox(handle);
		}
		return true;
	}
	if (!input->is_text_control() || !p_key.is_pressed()) return false;
	label->layout->flush_input_paint();
	auto& editor = input->editor;

	const bool enter = key == KEY_ENTER || key == KEY_KP_ENTER;
	const bool editable = input->is_editable();
	const char32_t unicode = p_key.get_unicode();
	const bool alt_gr_text = p_key.is_alt_pressed() && p_key.is_ctrl_pressed();
	const bool inserts_text = unicode >= 0x20 && unicode != 0x7f && !p_key.is_meta_pressed() &&
		(!p_key.is_ctrl_pressed() || alt_gr_text);
	if (input->editor.has_preedit() && !inserts_text) {
		if (key == KEY_ESCAPE) {
			label->accept_event();
			close_ime();
			update_ime();
			return true;
		}
		if (enter || key == KEY_BACKSPACE || key == KEY_DELETE || key == KEY_LEFT || key == KEY_RIGHT ||
			key == KEY_UP || key == KEY_DOWN || key == KEY_HOME || key == KEY_END ||
			p_key.is_ctrl_pressed() || p_key.is_meta_pressed()) {
			label->accept_event();
			return true;
		}
	}

	bool changed = false;
	const bool extend = p_key.is_shift_pressed();
	const char* maxlength = input->get_attr("maxlength", "");
	if (enter && !editor.is_multiline()) {
		label->accept_event();
		if (!p_key.is_echo()) {
			const ObjectID id(label->get_instance_id());
			commit_input(handle);
			if (auto* owner = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id))) owner->submit_from_text(handle);
		}
		return true;
	}
	if (p_key.is_action("ui_text_select_all", true)) {
		input->editor.set_selection(0, input->editor.get_value().length());
	} else if (p_key.is_action("ui_copy", true) || p_key.is_action("ui_cut", true)) {
		const int64_t start = input->editor.selection_start();
		const int64_t length = input->editor.selection_end() - start;
		if (length) {
			DisplayServer::get_singleton()->clipboard_set(input->editor.get_value().substr(start, length));
			if (editable && p_key.is_action("ui_cut", true)) changed = input->editor.insert({});
		}
	} else if (p_key.is_action("ui_paste", true)) {
		if (editable) changed = input->editor.insert(DisplayServer::get_singleton()->clipboard_get(), maxlength);
	} else if (key == KEY_LEFT || key == KEY_RIGHT) {
		input->editor.move_caret(key == KEY_LEFT ? -1 : 1, extend);
	} else if (key == KEY_HOME || key == KEY_END) {
		editor.move_to_edge(key == KEY_END, p_key.is_ctrl_pressed() || p_key.is_meta_pressed(), extend);
	} else if (editor.is_multiline() && (key == KEY_UP || key == KEY_DOWN)) {
		editor.move_vertical(key == KEY_UP ? -1 : 1, extend);
	} else if (enter) {
		if (editable) changed = editor.insert("\n", maxlength);
	} else if (key == KEY_BACKSPACE || key == KEY_DELETE) {
		if (editable) changed = input->editor.erase(key == KEY_BACKSPACE ? -1 : 1);
	} else if (inserts_text) {
		if (editable) changed = input->editor.insert(String::chr(unicode), maxlength);
	} else {
		return false;
	}
	label->accept_event();
	if (changed) change_input(handle);
	else {
		label->layout->repaint_input();
		queue_redraw();
	}
	return true;
}

void HTMLLabelCanvas::close_ime() {
	if (ime_window) {
		const auto window = *ime_window;
		ime_window.reset();
		ime_position.reset();
		label->set_process_internal(false);
		auto* display = DisplayServer::get_singleton();
		display->window_set_ime_position(Vector2i(), window);
		display->window_set_ime_active(false, window);
	}
	const auto handle = focused_element;
	if (const auto input = label->layout->text_control(handle); input && input->editor.has_preedit()) {
		input->editor.set_preedit({}, {});
		label->layout->repaint_input();
		queue_redraw();
	}
}

void HTMLLabelCanvas::update_ime() {
	const auto handle = focused_element;
	if (!label->layout->is_element_connected(handle)) {
		close_ime();
		return;
	}
	const auto input = label->layout->text_control(handle);
	auto* display = DisplayServer::get_singleton();
	if (!input || !input->is_editable() || !label->is_inside_tree() || !label->has_focus() ||
		!label->is_visible_in_tree() || !display->has_feature(DisplayServer::FEATURE_IME)) {
		if (ime_window) close_ime();
		return;
	}

	Viewport* viewport = get_viewport();
	Window* field_window = get_window();
	Window* native_window = find_native_ime_window(viewport);
	const int32_t id = native_window ? native_window->get_window_id() : -1;
	if (id < 0 || !field_window || !field_window->has_focus() || !native_window->has_focus()) {
		if (ime_window) close_ime();
		return;
	}
	if (ime_window != id) {
		if (ime_window) close_ime();
		ime_window = id;
		// A window stretch or embedded-window move can change the anchor without redrawing the field.
		label->set_process_internal(true);
		display->window_set_ime_active(true, id);
	}

	const auto point = label->layout->input_caret_point(*handle.ptr());
	const Rect2 visible = label->get_visible_rect();
	if (!point || !visible.has_area()) return;
	const Vector2 local = point->clamp(visible.position, visible.get_end());
	// Viewport screen transforms map into the native window's client area.
	const Vector2 client = (viewport->get_screen_transform() * get_global_transform_with_canvas()).xform(local);
	const Vector2i client_size = display->window_get_size(id);
	if (!client.is_finite() || client_size.x <= 0 || client_size.y <= 0) return;
	const Vector2i position(client.round().clamp(Vector2(), Vector2(client_size - Vector2i(1, 1))));
	if (ime_position != position) {
		ime_position = position;
		display->window_set_ime_position(position, id);
	}
}

void HTMLLabelCanvas::on_ime_update() {
	if (!ime_window || !label->has_focus()) return;
	const auto handle = focused_element;
	if (!label->layout->is_element_connected(handle)) {
		close_ime();
		return;
	}
	const auto input = label->layout->text_control(handle);
	if (!input || !input->is_editable()) return;
	auto* display = DisplayServer::get_singleton();
	input->editor.set_preedit(display->ime_get_text(), display->ime_get_selection());
	label->layout->repaint_input();
	queue_redraw();
}


bool HTMLLabelCanvas::begin_textarea_drag(const Ref<HTMLElement>& p_element, const Vector2& p_point) {
	const auto node = label->layout->text_control(p_element);
	const auto* textarea = dynamic_cast<const HTMLTextAreaNode*>(node.get());
	if (!textarea) return false;
	label->layout->flush_input_paint();
	const auto inverse = label->layout->input_inverse(*p_element.ptr());
	if (!inverse) return false;
	const Vector2 local = inverse->xform(p_point);
	const auto axes = textarea->resize_axes();
	const auto scrollbar = textarea->editor.scrollbar_at(local);
	const bool resize = axes != Vector2i() && textarea->get_resize_handle().has_point(local);
	if (!resize && !scrollbar) return false;

	cancel_press();
	textarea_drag.emplace();
	auto& drag = *textarea_drag;
	drag.element = p_element;
	drag.inverse = *inverse;
	drag.start_point = p_point - label->get_visible_rect().position;
	if (resize) {
		drag.axes = axes;
		drag.start_size = textarea->get_border_size();
		if (textarea->css().get_box_sizing() == litehtml::box_sizing_content_box) {
			drag.start_size -= textarea->get_border_extras();
		}
	} else {
		drag.vertical_scrollbar = *scrollbar;
		drag.scrollbar_grab = textarea->editor.scrollbar_grab_offset(*scrollbar, local);
		move_textarea_drag(p_point);
	}
	update_cursor(p_point);
	return true;
}

void HTMLLabelCanvas::move_textarea_drag(const Vector2& p_point) {
	if (!textarea_drag) return;
	const auto& drag = *textarea_drag;
	const auto node = label->layout->text_control(drag.element);
	const auto* textarea = dynamic_cast<const HTMLTextAreaNode*>(node.get());
	if (!textarea || docrender::forms::is_disabled(*textarea)) {
		textarea_drag.reset();
		resize_pending.reset();
		return;
	}
	if (drag.axes == Vector2i()) {
		if (const auto local = label->layout->input_local_point(*drag.element.ptr(), p_point)) {
			node->editor.drag_scrollbar(drag.vertical_scrollbar, *local, drag.scrollbar_grab);
			label->layout->repaint_input();
			queue_redraw();
		}
		return;
	}
	const auto delta = drag.inverse.basis_xform(p_point - label->get_visible_rect().position - drag.start_point) * Vector2(drag.axes);
	const auto size = (drag.start_size + delta).max(Vector2());
	if (!size.is_finite() || (delta == Vector2() && !dragged_resize)) return;
	dragged_resize = true;
	resize_pending = size;
	if (!resize_queued) {
		resize_queued = true;
		callable_mp(this, &HTMLLabelCanvas::flush_textarea_resize).call_deferred();
	}
}

void HTMLLabelCanvas::flush_textarea_resize() {
	resize_queued = false;
	const auto size = std::exchange(resize_pending, std::nullopt);
	if (!label || !textarea_drag || !size) return;
	const auto& drag = *textarea_drag;
	if (const auto node = label->layout->text_control(drag.element)) {
		const auto* textarea = dynamic_cast<const HTMLTextAreaNode*>(node.get());
		const auto axes = textarea ? textarea->resize_axes() : Vector2i();
		const auto allowed_axes = Vector2i(drag.axes.x && axes.x, drag.axes.y && axes.y);
		if (allowed_axes != Vector2i()) {
			applying_textarea_resize = true;
			label->layout->resize_textarea(drag.element, *size, allowed_axes);
			applying_textarea_resize = false;
		}
	}
}

void HTMLLabelCanvas::update_input_autoscroll() {
	const auto input = label->layout->text_control(dragging_input);
	if (!input) {
		dragging_input.unref();
		set_process_internal(false);
		return;
	}
	const auto point = get_local_mouse_position();
	const auto local = label->layout->input_local_point(*dragging_input.ptr(), point);
	if (!local || !input->editor.is_multiline()) return;
	const Rect2 viewport = input->editor.get_viewport();
	const Vector2 outside = *local - local->clamp(viewport.position, viewport.get_end());
	if (outside != Vector2() && input->editor.scroll_by(outside * std::min(1.0, get_process_delta_time() * 12.0)) != Vector2()) {
		set_input_caret(dragging_input, point, true);
	}
}

std::optional<Control::CursorShape> HTMLLabelCanvas::textarea_resize_cursor(const Vector2& p_point) const {
	Vector2i axes;
	if (textarea_drag) {
		axes = textarea_drag->axes;
	} else {
		const auto handle = label->layout->get_element_at(p_point);
		const auto node = label->layout->text_control(handle);
		const auto* textarea = dynamic_cast<const HTMLTextAreaNode*>(node.get());
		if (!textarea) return std::nullopt;
		const auto local = label->layout->input_local_point(*handle.ptr(), p_point);
		if (!local || !textarea->get_resize_handle().has_point(*local)) return std::nullopt;
		axes = textarea->resize_axes();
	}
	if (axes.x && axes.y) return CURSOR_FDIAGSIZE;
	if (axes.x) return CURSOR_HSIZE;
	if (axes.y) return CURSOR_VSIZE;
	return std::nullopt;
}
