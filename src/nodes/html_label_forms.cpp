#include "html_label.h"

#include "formats/html/dom/html_dom.h"
#include "formats/html/elements/html_form_state.h"
#include "formats/html/elements/html_input_node.h"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>

using namespace godot;
using docrender::HTMLInputNode;
namespace forms = docrender::forms;

Error HTMLLabel::submit_form(const Ref<HTMLFormElement>& p_form, const Ref<HTMLButtonElement>& p_submitter) {
	if (source_dirty || !layout->is_element_connected(p_form)) return ERR_INVALID_PARAMETER;
	const auto form = p_form->lock_element();
	const auto submitter = p_form->lock_related_element(p_submitter);
	if (p_submitter.is_valid() && (!submitter || forms::button_action(*submitter) != forms::ButtonAction::SUBMIT ||
		forms::owner(*submitter) != form || forms::is_disabled(*submitter))) return ERR_INVALID_PARAMETER;
	if (!form->get_attr("novalidate") && !(submitter && submitter->get_attr("formnovalidate"))) {
		if (const auto invalid = forms::first_invalid(*form)) {
			const auto control = layout->get_dom().make_element(invalid);
			const ObjectID id(get_instance_id());
			focus_element(control);
			if (auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id))) {
				if (label->layout->is_element_connected(p_form) && label->layout->is_element_connected(control)) {
					label->emit_signal("form_validation_failed", p_form, control);
				}
			}
			return ERR_INVALID_DATA;
		}
	}
	emit_signal("form_submitted", p_form, p_form->get_form_data(p_submitter), p_submitter);
	return OK;
}

void HTMLLabel::submit_from_text(const Ref<HTMLElement>& p_element) {
	if (!layout->is_element_connected(p_element)) return;
	const auto node = p_element->lock_element();
	const auto form = node ? forms::owner(*node) : nullptr;
	if (!form) return;
	int blocking = 0;
	const Ref<HTMLFormElement> form_handle = layout->get_dom().make_element(form);
	for (const auto& control : forms::controls(*form)) {
		if (forms::button_action(*control) == forms::ButtonAction::SUBMIT) {
			if (!forms::is_disabled(*control)) submit_form(form_handle, Ref<HTMLButtonElement>(layout->get_dom().make_element(control)));
			return;
		}
		if (const auto* input = dynamic_cast<const HTMLInputNode*>(control.get()); input && input->type() == HTMLInputNode::Type::TEXT) ++blocking;
	}
	if (blocking <= 1) submit_form(form_handle);
}

bool HTMLLabel::activate_form_control(const Ref<HTMLElement>& p_element) {
	if (!layout->is_focusable(p_element)) {
		if (!layout->is_element_connected(p_element)) return false;
		const Ref<HTMLLabelElement> label = p_element;
		if (label.is_null() || !layout->get_element_scroll_bounds(label)) return false;
		return activate_form_control(label->get_control());
	}
	if (const Ref<HTMLLabelElement> label = p_element; label.is_valid()) return activate_form_control(label->get_control());
	const auto node = p_element->lock_element();
	if (!node) return false;
	if (const Ref<HTMLSelectElement> select = p_element; select.is_valid()) return open_select_menu(select);
	if (const auto input = layout->text_control(p_element)) {
		const ObjectID id(get_instance_id());
		if (!focus_element(p_element)) return false;
		if (auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id))) {
			if (const Ref<HTMLInputElement> toggle = p_element; toggle.is_valid() && !input->is_text_control()) label->canvas->toggle_checkbox(toggle);
		}
		return true;
	}
	const auto action = forms::button_action(*node);
	if (action == forms::ButtonAction::NONE) return false;
	if (action == forms::ButtonAction::BUTTON) {
		emit_signal("button_pressed", p_element);
		return true;
	}
	const Ref<HTMLFormElement> form = layout->get_dom().make_element(forms::owner(*node));
	if (action == forms::ButtonAction::SUBMIT) {
		submit_form(form, Ref<HTMLButtonElement>(p_element));
	} else if (form.is_valid() && form->reset() == OK) {
		emit_signal("form_reset", form);
	}
	return true;
}

Ref<HTMLElement> HTMLLabelCanvas::get_form_click_target(const Ref<HTMLElement>& p_element) const {
	if (p_element.is_null()) return {};
	for (auto node = p_element->lock_element(); node; node = node->parent()) {
		const auto tag = node->tag();
		if (tag == litehtml::_label_ || tag == litehtml::_select_) return label->layout->get_dom().make_element(node);
		if (docrender::forms::is_listed(*node) || (tag == litehtml::_a_ && node->get_attr("href"))) return {};
	}
	return {};
}

bool HTMLLabel::open_select_menu(const Ref<HTMLSelectElement>& p_element) {
	const auto rows = layout->select_menu_rows(p_element);
	if (rows.empty()) return false;
	if (open_select == p_element) {
		close_select_menu();
		return true;
	}
	close_select_menu();
	if (!has_focus()) grab_focus();
	canvas->set_focused_element(p_element);
	if (!select_menu) {
		select_menu = memnew(Control);
		select_menu->set_name("_HTMLSelectMenu");
		select_menu->set_mouse_filter(MOUSE_FILTER_STOP);
		select_menu->set_force_pass_scroll_events(false);
		select_menu->set_z_index(1);
		select_menu->hide();
		add_child(select_menu, false, INTERNAL_MODE_BACK);
		select_menu->connect("draw", callable_mp(this, &HTMLLabel::draw_select_menu));
		select_menu->connect("gui_input", callable_mp(this, &HTMLLabel::select_menu_input));
	}
	open_select = p_element;
	select_rows = rows;
	update_select_menu();
	if (open_select.is_null()) return false;
	const auto selected = p_element->get_selected_index();
	int first = -1;
	for (int i = 0; i < static_cast<int>(select_rows.size()); ++i) {
		if (!select_rows[i].enabled) continue;
		if (first < 0) first = i;
		if (select_rows[i].option_index == selected) { first = i; break; }
	}
	if (first >= 0) highlight_select_row(first);
	return true;
}

void HTMLLabel::close_select_menu() {
	open_select.unref();
	select_rows.clear();
	select_highlight = -1;
	select_pressed = -1;
	select_scroll = 0;
	if (select_menu) select_menu->hide();
}

void HTMLLabel::update_select_menu() {
	if (open_select.is_null()) return;
	const auto bounds = layout->get_element_scroll_bounds(open_select);
	const auto visible = get_visible_rect();
	if (!is_visible_in_tree() || !layout->is_focusable(open_select) || !bounds || !visible.intersects(*bounds)) {
		close_select_menu();
		return;
	}
	const auto offset = get_global_transform_with_canvas().affine_inverse().xform(canvas->get_global_transform_with_canvas().get_origin());
	const Rect2 window(visible.position + offset, visible.size);
	const Rect2 anchor(bounds->position + offset, bounds->size);
	float row_width = anchor.size.x;
	select_row_height = 20;
	for (const auto& row : select_rows) {
		select_row_height = std::max(select_row_height, row.text.height() + 8.0f);
		row_width = std::max(row_width, row.text.width() + 28.0f);
	}
	const float above = anchor.position.y - window.position.y;
	const float below = window.get_end().y - anchor.get_end().y;
	const float desired = select_row_height * std::min<size_t>(select_rows.size(), 8);
	const bool open_below = below >= desired || below >= above;
	const int capacity = static_cast<int>(std::max(0.0f, open_below ? below : above) / select_row_height);
	select_visible_rows = std::min({capacity, static_cast<int>(select_rows.size()), 8});
	if (!select_visible_rows) { close_select_menu(); return; }
	select_scroll = std::clamp(select_scroll, 0, static_cast<int>(select_rows.size()) - select_visible_rows);
	const float height = select_row_height * select_visible_rows;
	const real_t width = std::min<real_t>(row_width, window.size.x);
	select_menu->set_position(Vector2(std::clamp(anchor.position.x, window.position.x, window.get_end().x - width),
		open_below ? anchor.get_end().y : anchor.position.y - height));
	select_menu->set_size(Vector2(width, height));
	select_menu->show();
	select_menu->queue_redraw();
}

void HTMLLabel::draw_select_menu() {
	if (open_select.is_null()) return;
	const auto size = select_menu->get_size();
	const auto server = TextServerManager::get_singleton()->get_primary_interface();
	const int64_t selected = open_select->get_selected_index();
	select_menu->draw_rect(Rect2(Vector2(), size), Color(0.96f, 0.97f, 0.99f));
	for (int i = 0; i < select_visible_rows; ++i) {
		const int index = select_scroll + i;
		const auto& row = select_rows[index];
		const float y = i * select_row_height;
		const Rect2 box(Vector2(0, y), Vector2(size.x, select_row_height));
		if (row.background.a > 0) select_menu->draw_rect(box, row.background);
		if (index == select_highlight) select_menu->draw_rect(box, Color(0.29f, 0.51f, 0.89f, 0.32f));
		const bool checked = row.option_index >= 0 && row.option_index == selected;
		if (checked) select_menu->draw_circle(Vector2(9, y + select_row_height * 0.5f), 3, row.color);
		if (row.text.is_valid() && server.is_valid()) {
			const float inset = row.option_index >= 0 ? 18 : 6;
			const float baseline = y + (select_row_height + row.text.line_ascent() - row.text.line_descent()) * 0.5f;
			Color color = row.color;
			if (!row.enabled && row.option_index >= 0) color.a *= 0.45f;
			server->shaped_text_draw(row.text.get_rid(), select_menu->get_canvas_item(), Vector2(inset, baseline), 0,
				std::max<real_t>(0, size.x - inset - 5), color);
		}
	}
	if (static_cast<int>(select_rows.size()) > select_visible_rows) {
		const float scale = size.y / select_rows.size();
		select_menu->draw_rect(Rect2(size.x - 3, select_scroll * scale, 3, select_visible_rows * scale), Color(0.39f, 0.45f, 0.54f));
	}
	select_menu->draw_rect(Rect2(Vector2(), size), Color(0.39f, 0.45f, 0.54f), false);
}

void HTMLLabel::highlight_select_row(int p_row) {
	if (p_row < 0 || p_row >= static_cast<int>(select_rows.size()) || !select_rows[p_row].enabled) return;
	select_highlight = p_row;
	select_scroll = std::clamp(select_scroll, std::max(0, p_row + 1 - select_visible_rows), p_row);
	select_menu->queue_redraw();
}

void HTMLLabel::commit_select_row(int p_row) {
	if (p_row < 0 || p_row >= static_cast<int>(select_rows.size()) || !select_rows[p_row].enabled) return;
	const auto select = open_select;
	const int64_t index = select_rows[p_row].option_index;
	close_select_menu();
	if (!layout->is_focusable(select) || index == select->get_selected_index()) return;
	select->set_selected_index(index);
	emit_signal("select_changed", select);
}

void HTMLLabel::select_menu_input(const Ref<InputEvent>& p_event) {
	if (open_select.is_null()) return;
	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid()) {
		if (Rect2(Vector2(), select_menu->get_size()).has_point(motion->get_position())) {
			highlight_select_row(select_scroll + static_cast<int>(motion->get_position().y / select_row_height));
		}
		return;
	}
	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_null()) return;
	select_menu->accept_event();
	const auto key = button->get_button_index();
	if (button->is_pressed() && (key == MOUSE_BUTTON_WHEEL_UP || key == MOUSE_BUTTON_WHEEL_DOWN)) {
		select_scroll = std::clamp(select_scroll + (key == MOUSE_BUTTON_WHEEL_UP ? -1 : 1), 0,
			static_cast<int>(select_rows.size()) - select_visible_rows);
		select_menu->queue_redraw();
	} else if (key == MOUSE_BUTTON_LEFT) {
		const auto point = button->get_position();
		const int row = Rect2(Vector2(), select_menu->get_size()).has_point(point) ?
			select_scroll + static_cast<int>(point.y / select_row_height) : -1;
		if (button->is_pressed()) select_pressed = row;
		else if (select_pressed == row && !button->is_canceled()) commit_select_row(row);
	}
}

bool HTMLLabel::handle_select_key(const InputEventKey& p_key) {
	if (!p_key.is_pressed() || p_key.is_ctrl_pressed() || p_key.is_meta_pressed() || p_key.is_alt_pressed()) return false;
	const Ref<HTMLSelectElement> select = canvas->focused_element;
	if (select.is_null() || !layout->is_focusable(select)) return false;
	const Key key = p_key.get_keycode() != KEY_NONE ? p_key.get_keycode() : p_key.get_physical_keycode();
	const bool accept = key == KEY_SPACE || key == KEY_ENTER || key == KEY_KP_ENTER;
	if (accept || key == KEY_ESCAPE) {
		if (open_select.is_null() && !accept) return false;
		accept_event();
		if (p_key.is_echo()) return true;
		if (key == KEY_ESCAPE) close_select_menu();
		else if (open_select.is_valid()) commit_select_row(select_highlight);
		else open_select_menu(select);
		return true;
	}
	if (key != KEY_UP && key != KEY_DOWN && key != KEY_HOME && key != KEY_END) return false;
	accept_event();
	if (open_select.is_null()) { open_select_menu(select); return true; }
	const bool backwards = key == KEY_UP || key == KEY_END;
	const int size = static_cast<int>(select_rows.size());
	const int step = backwards ? -1 : 1;
	const int start = key == KEY_HOME ? 0 : (key == KEY_END ? size - 1 :
		(select_highlight < 0 ? (backwards ? size - 1 : 0) : select_highlight + step));
	for (int i = start; i >= 0 && i < size; i += step) {
		if (select_rows[i].enabled) { highlight_select_row(i); break; }
	}
	return true;
}
