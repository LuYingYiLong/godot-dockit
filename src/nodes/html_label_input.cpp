#include "html_label.h"

#include "formats/html/elements/html_input_node.h"
#include "formats/html/elements/html_textarea_node.h"
#include "formats/html/dom/html_dom.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/window.hpp>

#include <algorithm>
#include <iterator>
#include <utility>

using namespace godot;
using docrender::HTMLInputNode;
using docrender::HTMLTextAreaNode;

namespace {
	enum InputCommand {
		INPUT_UNDO,
		INPUT_REDO,
		INPUT_CUT,
		INPUT_COPY,
		INPUT_PASTE,
		INPUT_SELECT_ALL,
	};

	struct InputMenuItem {
		const char* label;
		const char* action;
	};

	constexpr InputMenuItem INPUT_MENU_ITEMS[] = {
		{"Undo", "ui_undo"},
		{"Redo", "ui_redo"},
		{"Cut", "ui_cut"},
		{"Copy", "ui_copy"},
		{"Paste", "ui_paste"},
		{"Select All", "ui_text_select_all"},
	};

	Key input_menu_accelerator(const StringName& p_action) {
		for (const Ref<InputEvent>& event : InputMap::get_singleton()->action_get_events(p_action)) {
			const Ref<InputEventKey> key = event;
			if (key.is_valid()) {
				return key->get_physical_keycode() != KEY_NONE ?
					key->get_physical_keycode_with_modifiers() : key->get_keycode_with_modifiers();
			}
		}
		return KEY_NONE;
	}

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
		const auto mode = input->user_selection_mode();
		if (mode == litehtml::css_user_select::none) return;
		if (mode == litehtml::css_user_select::all) input->editor.set_selection(0, input->editor.get_value().length());
		else input->editor.hit_caret(*local, p_extend);
		restart_caret_blink();
		label->layout->repaint_input();
		queue_redraw();
	}
}

void HTMLLabelCanvas::change_input(const Ref<HTMLElement>& p_input, bool p_update_suggestions) {
	if (const auto input = label->layout->text_control(p_input)) {
		label->layout->update_input(*input);
		queue_redraw();
		if (p_update_suggestions && focused_element == p_input && input->get_attr("list")) label->open_suggestion_menu(p_input);
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

void HTMLLabelCanvas::move_range(const Ref<HTMLInputElement>& p_input, const Vector2& p_point, bool p_commit) {
	if (p_input.is_null() || !label->layout->is_focusable(p_input)) return;
	const auto node = p_input->get_input();
	const auto local = label->layout->input_local_point(*p_input.ptr(), p_point);
	if (!node || node->type() != HTMLInputNode::Type::RANGE || !local) return;
	const ObjectID id(label->get_instance_id());
	if (node->set_range_point(*local)) change_input(p_input);
	if (p_commit) {
		if (auto* owner = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id))) owner->canvas->commit_input(p_input);
	}
}

bool HTMLLabelCanvas::handle_input_key(const InputEventKey& p_key) {
	const Ref<HTMLElement> handle = focused_element;
	const auto input = label->layout->text_control(handle);
	if (!input || !label->layout->is_element_connected(handle)) return false;
	const Key key = p_key.get_keycode() != KEY_NONE ? p_key.get_keycode() : p_key.get_physical_keycode();
	if (auto* numeric = dynamic_cast<HTMLInputNode*>(input.get()); numeric && p_key.is_pressed() &&
		!p_key.is_alt_pressed() && !p_key.is_ctrl_pressed() && !p_key.is_meta_pressed() && !input->editor.has_preedit() &&
		!docrender::forms::is_disabled(*input)) {
		const bool range = numeric->type() == HTMLInputNode::Type::RANGE;
		const bool number = numeric->type() == HTMLInputNode::Type::NUMBER;
		const bool vertical = key == KEY_UP || key == KEY_DOWN;
		const bool horizontal = key == KEY_LEFT || key == KEY_RIGHT;
		const bool endpoint = key == KEY_HOME || key == KEY_END;
		if ((range && (vertical || horizontal || endpoint)) || (number && vertical && input->is_editable())) {
			int step = key == KEY_DOWN || key == KEY_LEFT ? -1 : 1;
			if (horizontal && input->css().is_right_to_left()) step = -step;
			const ObjectID id(label->get_instance_id());
			label->accept_event();
			if (numeric->step_number(step, endpoint ? std::optional<bool>(key == KEY_END) : std::nullopt)) {
				change_input(handle);
				if (range) if (auto* owner = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id))) owner->canvas->commit_input(handle);
			}
			return true;
		}
	}
	if (const auto* toggle = dynamic_cast<const HTMLInputNode*>(input.get()); toggle && toggle->is_toggle()) {
		if (!label->layout->is_focusable(handle)) return false;
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
	// One IME result can arrive as several key events. The first character can
	// invalidate the layout through :placeholder-shown while the field still has focus.
	if (!input->is_text_control() || !p_key.is_pressed() || docrender::forms::is_disabled(*input) ||
		input->get_attr("hidden")) return false;
	restart_caret_blink();
	label->layout->flush_input_paint();
	auto& editor = input->editor;

	if (label->context_menu_enabled && !p_key.is_echo() && p_key.is_action("ui_menu", true)) {
		const auto caret = label->layout->input_caret_point(*handle.ptr());
		if (open_input_menu(handle, caret.value_or(label->get_visible_rect().position), false)) {
			label->accept_event();
			return true;
		}
	}

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

	if (!inserts_text) editor.finish_ime_edit();

	bool changed = false;
	const auto selection_mode = input->user_selection_mode();
	const bool extend = p_key.is_shift_pressed() && selection_mode != litehtml::css_user_select::none;
	const char* maxlength = input->get_attr("maxlength", "");
	if (enter && !inserts_text && !editor.is_multiline()) {
		label->accept_event();
		if (!p_key.is_echo()) {
			const ObjectID id(label->get_instance_id());
			commit_input(handle);
			if (auto* owner = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id))) owner->submit_from_text(handle);
		}
		return true;
	}
	for (int command = 0; command < static_cast<int>(std::size(INPUT_MENU_ITEMS)); ++command) {
		if (p_key.is_action(INPUT_MENU_ITEMS[command].action, true)) {
			label->accept_event();
			run_input_command(handle, command);
			return true;
		}
	}
	if (inserts_text) {
		// IME character events can retain the key code of the confirmation key.
		if (editable) {
			changed = editor.insert_character(unicode, maxlength, Engine::get_singleton()->get_process_frames(),
				key == KEY_NONE || enter);
		}
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
	} else {
		return false;
	}
	if (extend && selection_mode == litehtml::css_user_select::all && editor.selection_start() != editor.selection_end()) {
		editor.set_selection(0, editor.get_value().length());
	}
	label->accept_event();
	if (changed) change_input(handle);
	else {
		label->layout->repaint_input();
		queue_redraw();
	}
	return true;
}

bool HTMLLabelCanvas::open_input_menu(const Ref<HTMLElement>& p_element, const Vector2& p_position, bool p_pointer) {
	const auto input = label->layout->text_control(p_element);
	if (!input || !input->is_text_control() || !label->layout->is_focusable(p_element)) return false;
	const Rect2 visible = label->get_visible_rect();
	if (!visible.has_area()) return false;

	cancel_press();
	const Ref<HTMLElement> active_element = focused_element;
	if (const auto active = label->layout->text_control(active_element);
		active && active->is_editable() && active->editor.has_preedit() && label->layout->is_element_connected(active_element)) {
		// Keep the marked text before closing the native composition, as LineEdit does.
		const String marked = active->editor.get_preedit();
		close_ime();
		if (active->editor.insert(marked, active->get_attr("maxlength", ""))) change_input(active_element);
	}

	label->grab_focus();
	if (!label->context_menu_enabled || !label->layout->is_element_connected(p_element) ||
		label->layout->text_control(p_element) != input || !input->is_text_control() ||
		docrender::forms::is_disabled(*input) || input->get_attr("hidden")) return false;
	if (focused_element != p_element) set_focused_element(p_element);
	auto& editor = input->editor;
	const bool has_selection = editor.selection_start() != editor.selection_end();
	if (p_pointer && !has_selection && input->is_editable()) set_input_caret(p_element, p_position, false);

	if (!input_menu) {
		input_menu = memnew(PopupMenu);
		input_menu->set_name("_HTMLInputMenu");
		input_menu->set_hide_on_item_selection(false);
		input_menu->connect("id_pressed", callable_mp(this, &HTMLLabelCanvas::on_input_menu_selected));
		input_menu->connect("popup_hide", callable_mp(this, &HTMLLabelCanvas::on_input_menu_hidden));
		add_child(input_menu, false, INTERNAL_MODE_BACK);
	}

	input_menu->clear();
	const bool editable = input->is_editable();
	const auto* field = dynamic_cast<const HTMLInputNode*>(input.get());
	const bool password = field && field->is_password();
	const bool disabled[] = {
		!editable || !editor.can_undo(),
		!editable || !editor.can_redo(),
		password || !editable || !has_selection,
		password || !has_selection,
		!editable || DisplayServer::get_singleton()->clipboard_get().is_empty(),
		editor.get_value().is_empty() || input->user_selection_mode() == litehtml::css_user_select::none,
	};
	for (int command = 0; command < static_cast<int>(std::size(INPUT_MENU_ITEMS)); ++command) {
		if (command == INPUT_CUT || command == INPUT_SELECT_ALL) input_menu->add_separator();
		const auto& item = INPUT_MENU_ITEMS[command];
		input_menu->add_item(tr(item.label), command, input_menu_accelerator(item.action));
		input_menu->set_item_disabled(input_menu->get_item_index(command), disabled[command]);
	}

	input_menu_target = p_element;
	close_ime();
	input_menu->set_position(get_screen_transform().xform(p_position.clamp(visible.position, visible.get_end())));
	input_menu->reset_size();
	input_menu->popup();
	if (input_menu->is_visible()) return true;
	on_input_menu_hidden();
	return false;
}

void HTMLLabelCanvas::close_input_menu() {
	if (input_menu && input_menu->is_visible()) input_menu->hide();
}

void HTMLLabelCanvas::on_input_menu_hidden() {
	input_menu_target.unref();
	callable_mp(this, &HTMLLabelCanvas::update_ime).call_deferred();
}

void HTMLLabelCanvas::on_input_menu_selected(int p_command) {
	const Ref<HTMLElement> target = input_menu_target;
	close_input_menu();
	if (target.is_valid() && focused_element == target) run_input_command(target, p_command);
}

void HTMLLabelCanvas::run_input_command(const Ref<HTMLElement>& p_element, int p_command) {
	const auto input = label->layout->text_control(p_element);
	if (!input || !input->is_text_control() || !label->layout->is_element_connected(p_element) ||
		docrender::forms::is_disabled(*input) || input->get_attr("hidden")) return;

	auto& editor = input->editor;
	const bool editable = input->is_editable() && !editor.has_preedit();
	const char* maxlength = input->get_attr("maxlength", "");
	bool changed = false;
	switch (p_command) {
		case INPUT_UNDO:
			if (editable) changed = editor.undo();
			break;
		case INPUT_REDO:
			if (editable) changed = editor.redo();
			break;
		case INPUT_CUT:
		case INPUT_COPY: {
			const auto* field = dynamic_cast<const HTMLInputNode*>(input.get());
			if (field && field->is_password()) break;
			const int64_t start = editor.selection_start();
			const int64_t length = editor.selection_end() - start;
			if (length > 0 && (p_command == INPUT_COPY || editable)) {
				DisplayServer::get_singleton()->clipboard_set(editor.get_value().substr(start, length));
				if (p_command == INPUT_CUT) changed = editor.insert({});
			}
			break;
		}
		case INPUT_PASTE:
			if (editable) {
				const String clipboard = DisplayServer::get_singleton()->clipboard_get();
				if (!clipboard.is_empty()) changed = editor.insert(clipboard, maxlength);
			}
			break;
		case INPUT_SELECT_ALL:
			if (input->user_selection_mode() != litehtml::css_user_select::none) editor.set_selection(0, editor.get_value().length());
			break;
		default:
			return;
	}
	restart_caret_blink();
	if (changed) change_input(p_element);
	else {
		label->layout->repaint_input();
		queue_redraw();
	}
}

void HTMLLabelCanvas::set_input_caret_visible(docrender::HTMLTextEditor& p_editor, bool p_visible) {
	if (p_editor.set_caret_visible(p_visible)) {
		label->layout->repaint_input();
		queue_redraw();
	}
}

void HTMLLabelCanvas::restart_caret_blink() {
	const auto input = label->layout->text_control(focused_element);
	if (!input || !input->is_text_control() || !is_inside_tree() || !caret_window_active || !label->has_focus()) return;

	set_input_caret_visible(input->editor, true);
	if (!label->caret_blink) {
		if (caret_blink_timer) caret_blink_timer->stop();
		return;
	}

	if (!caret_blink_timer) {
		caret_blink_timer = memnew(Timer);
		caret_blink_timer->set_ignore_time_scale(true);
		caret_blink_timer->connect("timeout", callable_mp(this, &HTMLLabelCanvas::blink_caret));
		add_child(caret_blink_timer, false, INTERNAL_MODE_BACK);
	}
	caret_blink_timer->start(label->caret_blink_interval);
}

void HTMLLabelCanvas::stop_caret_blink() {
	if (caret_blink_timer) caret_blink_timer->stop();
	if (const auto input = label->layout->text_control(focused_element)) {
		set_input_caret_visible(input->editor, false);
	}
}

void HTMLLabelCanvas::blink_caret() {
	const auto input = label->layout->text_control(focused_element);
	if (!input || !input->is_text_control() || !label->layout->is_element_connected(focused_element)) {
		stop_caret_blink();
		return;
	}
	set_input_caret_visible(input->editor, !input->editor.is_caret_visible());
}

void HTMLLabelCanvas::close_ime() {
	if (ime_window) {
		const auto window = *ime_window;
		ime_window.reset();
		ime_position.reset();
		label->update_internal_processing();
		auto* display = DisplayServer::get_singleton();
		display->window_set_ime_position(Vector2i(), window);
		display->window_set_ime_active(false, window);
	}
	const auto handle = focused_element;
	if (const auto input = label->layout->text_control(handle); input && input->editor.cancel_preedit()) {
		label->layout->repaint_input();
		queue_redraw();
	}
}

void HTMLLabelCanvas::update_ime() {
	if (input_menu_target.is_valid()) return;

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
		label->update_internal_processing();
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
	input->editor.update_preedit(display->ime_get_text(), display->ime_get_selection(),
		Engine::get_singleton()->get_process_frames());
	restart_caret_blink();
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
	if (!local.is_finite()) return false;
	const auto axes = textarea->resize_axes();
	const auto scrollbar = textarea->editor.scrollbar_at(local);
	const bool resize = axes != Vector2i() && textarea->get_resize_handle().has_point(local);
	if (!resize && !scrollbar) return false;

	cancel_press();
	textarea_drag.emplace();
	auto& drag = *textarea_drag;
	drag.element = p_element;
	drag.inverse = *inverse * Transform2D(0, label->get_visible_rect().position);
	drag.start_point = local;
	if (resize) {
		drag.axes = axes;
		drag.start_size = textarea->get_border_size();
		if (textarea->css().get_box_sizing() == litehtml::box_sizing_content_box) {
			drag.start_size -= textarea->get_border_extras();
		}
	} else {
		drag.vertical_scrollbar = *scrollbar;
		drag.scrollbar_grab = textarea->editor.scrollbar_grab_offset(*scrollbar, local);
		label->layout->set_scrollbar_pressed(docrender::HTMLScrollbarDrag{node, *scrollbar ? 1 : 0, 0});
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
		label->layout->set_scrollbar_pressed(std::nullopt);
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
	const auto current = drag.inverse.xform(p_point - label->get_visible_rect().position);
	const auto delta = (current - drag.start_point) * Vector2(drag.axes);
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
