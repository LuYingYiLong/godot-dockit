#include "html_label.h"

#include "formats/html/elements/html_input_node.h"
#include "formats/html/paint/html_paint_recorder.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;
using docrender::HTMLInputNode;
namespace calendar = docrender::input_time;

namespace {
	PackedStringArray file_filters(const String& p_accept) {
		PackedStringArray filters;
		for (const String& item : p_accept.split(",", false)) {
			const String type = item.strip_edges().to_lower();
			if (type.contains(";") || type.contains("\n") || type.contains("\r")) continue;
			if (type.begins_with(".") && type.length() > 1 && !type.contains("/") && !type.contains("\\") &&
				!type.contains("*") && !type.contains("?")) {
				filters.append("*" + type + " ; " + type);
			} else if (type == "image/*") {
				filters.append("*.png,*.jpg,*.jpeg,*.webp,*.gif,*.bmp,*.svg,*.avif,*.heic ; Images ; image/*");
			} else if (type == "audio/*") {
				filters.append("*.mp3,*.wav,*.ogg,*.flac,*.aac,*.m4a,*.opus ; Audio ; audio/*");
			} else if (type == "video/*") {
				filters.append("*.mp4,*.webm,*.ogv,*.mov,*.mkv,*.avi ; Video ; video/*");
			} else if (type.contains("/")) {
				if (type == "text/plain") {
					filters.append("*.txt,*.text ; Plain text ; text/plain");
					continue;
				}
				if (type == "image/jpeg") {
					filters.append("*.jpg,*.jpeg,*.jpe ; JPEG ; image/jpeg");
					continue;
				}
				if (type == "audio/mpeg") {
					filters.append("*.mp3,*.mp2 ; MPEG audio ; audio/mpeg");
					continue;
				}
				const auto slash = type.find("/");
				const auto extension = type.substr(slash + 1).get_slice("+", 0);
				if (!extension.is_empty()) filters.append("*." + extension + " ; " + type + " ; " + type);
			}
		}
		return filters;
	}

	Button* picker_button(Control& p_parent, const String& p_text, const Callable& p_callback) {
		auto* button = memnew(Button);
		button->set_text(p_text);
		button->connect("pressed", p_callback);
		p_parent.add_child(button);
		return button;
	}
}

bool HTMLLabel::input_picker_valid() const {
	const auto input = picker_input.is_valid() ? picker_input->get_input() : nullptr;
	return input && picker_input->adoption_version == picker_adoption && input->type_name() == picker_type &&
		input->editor.get_reset_version() == picker_reset && is_inside_tree() && is_visible_in_tree() &&
		!source_dirty && layout->is_element_connected(picker_input) && layout->allows_modal_interaction(picker_input) &&
		!docrender::forms::is_disabled(*input) && !(input->supports_readonly() && input->get_attr("readonly"));
}

void HTMLLabel::close_input_picker() {
	const bool closes_color = picker_type == "color";
	if (closes_color) {
		if (const auto node = picker_input->get_input()) node->set_color_preview(std::nullopt);
		if (color_picker_canvas) color_picker_canvas->hide();
		color_picker = {};
		layout->repaint_input();
	}
	++picker_sequence;
	picker_input.unref();
	picker_type = String();
	if (closes_color && canvas) canvas->update_focus_mode();
	picker_content = nullptr;
	if (auto* popup = std::exchange(input_popup, nullptr)) {
		popup->hide();
		popup->queue_free();
	}
	if (auto* dialog = std::exchange(file_dialog, nullptr)) {
		dialog->hide();
		dialog->queue_free();
	}
}

void HTMLLabel::on_input_picker_closed(uint64_t p_sequence) {
	if (p_sequence == picker_sequence) close_input_picker();
}

Error HTMLLabel::open_input_picker(const Ref<HTMLInputElement>& p_element) {
	if (!is_inside_tree() || !is_visible_in_tree() || source_dirty || !layout->is_element_connected(p_element) ||
		!layout->is_focusable(p_element) || !layout->allows_modal_interaction(p_element)) return ERR_INVALID_PARAMETER;
	const auto node = p_element->get_input();
	if (!node || !node->has_picker() || (node->supports_readonly() && node->get_attr("readonly"))) return ERR_UNAVAILABLE;
	if (picker_type == "file" && input_picker_valid()) return picker_input == p_element ? OK : ERR_BUSY;
	const auto kind = node->type();
	const auto date_kind = node->time_kind();
	const ObjectID id(get_instance_id());
	if (!focus_element(p_element) || !ObjectDB::get_instance(id)) return ERR_UNAVAILABLE;
	if (!layout->is_element_connected(p_element) || p_element->get_input() != node || node->type() != kind) return ERR_UNAVAILABLE;
	if (kind != HTMLInputNode::Type::FILE && kind != HTMLInputNode::Type::COLOR && date_kind == calendar::Kind::none) {
		return open_suggestion_menu(p_element) ? OK : ERR_UNAVAILABLE;
	}
	close_select_menu();
	canvas->close_input_menu();
	close_input_picker();
	picker_input = p_element;
	picker_type = node->type_name();
	picker_adoption = p_element->adoption_version;
	picker_reset = node->editor.get_reset_version();
	if (kind == HTMLInputNode::Type::FILE) return open_file_picker(picker_sequence);
	if (kind == HTMLInputNode::Type::COLOR) return show_color_picker();

	const auto bounds = layout->get_element_scroll_bounds(p_element);
	if (!bounds) {
		close_input_picker();
		return ERR_UNAVAILABLE;
	}
	input_popup = memnew(PopupPanel);
	input_popup->set_name("_HTMLInputPicker");
	input_popup->connect("popup_hide", callable_mp(this, &HTMLLabel::on_input_picker_closed).bind(picker_sequence),
		Object::CONNECT_DEFERRED);
	add_child(input_popup, false, INTERNAL_MODE_BACK);
	const auto scheme = layout->get_element_color_scheme(p_element);
	Ref<StyleBoxFlat> panel;
	panel.instantiate();
	panel->set_bg_color(docrender::html_system_color(litehtml::system_color::menu_face, scheme));
	panel->set_border_color(docrender::html_system_color(litehtml::system_color::menu_border, scheme));
	panel->set_border_width_all(1);
	panel->set_corner_radius_all(8);
	for (const auto side : { SIDE_LEFT, SIDE_TOP, SIDE_RIGHT, SIDE_BOTTOM }) panel->set_content_margin(side, 8);
	input_popup->add_theme_stylebox_override("panel", panel);
	picker_content = memnew(VBoxContainer);
	Ref<Theme> theme;
	theme.instantiate();
	const auto ink = docrender::html_system_color(litehtml::system_color::canvas_text, scheme);
	for (const char* type : { "Button", "Label", "LineEdit" }) {
		theme->set_color("font_color", type, ink);
		theme->set_color("font_hover_color", type, ink);
		theme->set_color("font_pressed_color", type, ink);
		theme->set_color("font_focus_color", type, ink);
		theme->set_color("icon_normal_color", type, ink);
		theme->set_color("icon_hover_color", type, ink);
		theme->set_color("icon_pressed_color", type, ink);
		theme->set_color("caret_color", type, ink);
	}
	const auto button_style = [&](litehtml::system_color p_color) {
		Ref<StyleBoxFlat> box;
		box.instantiate();
		box->set_bg_color(docrender::html_system_color(p_color, scheme));
		box->set_corner_radius_all(4);
		for (const auto side : { SIDE_LEFT, SIDE_TOP, SIDE_RIGHT, SIDE_BOTTOM }) box->set_content_margin(side, 4);
		return box;
	};
	theme->set_stylebox("normal", "Button", button_style(litehtml::system_color::button_face));
	theme->set_stylebox("hover", "Button", button_style(litehtml::system_color::menu_hover));
	theme->set_stylebox("pressed", "Button", button_style(litehtml::system_color::list_selected));
	theme->set_stylebox("normal", "LineEdit", button_style(litehtml::system_color::field));
	picker_content->set_theme(theme);
	input_popup->add_child(picker_content);
	const double current = node->value_as_number();
	picker_date = std::isfinite(current) ? calendar::parts(date_kind, current) :
		Time::get_singleton()->get_datetime_dict_from_system();
	if (!std::isfinite(current) && date_kind == calendar::Kind::time) {
		picker_date["hour"] = 0;
		picker_date["minute"] = 0;
		picker_date["second"] = 0;
	}
	build_time_picker();
	const Vector2i minimum = Vector2i(picker_content->get_combined_minimum_size().ceil()) + Vector2i(16, 16);
	const bool embedded = get_viewport()->is_embedding_subwindows();
	const Transform2D transform = embedded ? canvas->get_global_transform_with_canvas() : canvas->get_screen_transform();
	const Vector2 base = transform.xform(bounds->get_end());
	const Vector2 top = transform.xform(bounds->position);
	const Rect2i visible = embedded ? Rect2i(get_viewport()->get_visible_rect()) :
		DisplayServer::get_singleton()->screen_get_usable_rect(get_window()->get_current_screen());
	Vector2i position(top.x, base.y);
	if (position.y + minimum.y > visible.get_end().y && top.y > visible.position.y) position.y = int(top.y) - minimum.y;
	position.x = std::clamp(position.x, visible.position.x, std::max(visible.position.x, visible.get_end().x - minimum.x));
	position.y = std::clamp(position.y, visible.position.y, std::max(visible.position.y, visible.get_end().y - minimum.y));
	input_popup->popup(Rect2i(position, minimum));
	return OK;
}

Error HTMLLabel::open_file_picker(uint64_t p_sequence) {
	const auto input = picker_input->get_input();
	auto* display = DisplayServer::get_singleton();
	if (display->get_name() == "headless") {
		close_input_picker();
		return ERR_UNAVAILABLE;
	}
	const bool multiple = input->get_attr("multiple");
	const auto filters = file_filters(String::utf8(input->get_attr("accept", "")));
	if (!get_window()->is_embedded() && display->has_feature(DisplayServer::FEATURE_NATIVE_DIALOG_FILE)) {
		const auto mode = multiple ? DisplayServer::FILE_DIALOG_MODE_OPEN_FILES : DisplayServer::FILE_DIALOG_MODE_OPEN_FILE;
		const auto result = display->file_dialog_show("Choose File", "", "", false, mode, filters,
			callable_mp(this, &HTMLLabel::native_files_selected).bind(p_sequence), get_window()->get_window_id());
		if (result == OK) return OK;
	}
	file_dialog = memnew(FileDialog);
	file_dialog->set_name("_HTMLFilePicker");
	file_dialog->set_title("Choose File");
	file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	file_dialog->set_file_mode(multiple ? FileDialog::FILE_MODE_OPEN_FILES : FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_filters(filters);
	file_dialog->connect("files_selected", callable_mp(this, &HTMLLabel::files_selected).bind(p_sequence));
	file_dialog->connect("file_selected", callable_mp(this, &HTMLLabel::file_selected).bind(p_sequence));
	file_dialog->connect("canceled", callable_mp(this, &HTMLLabel::on_input_picker_closed).bind(p_sequence));
	add_child(file_dialog, false, INTERNAL_MODE_BACK);
	file_dialog->popup_centered_ratio(0.68);
	return OK;
}

void HTMLLabel::native_files_selected(bool p_accepted, const PackedStringArray& p_paths, int, uint64_t p_sequence) {
	if (p_accepted) files_selected(p_paths, p_sequence);
	else on_input_picker_closed(p_sequence);
}

void HTMLLabel::file_selected(const String& p_path, uint64_t p_sequence) {
	files_selected(PackedStringArray(Array::make(p_path)), p_sequence);
}

void HTMLLabel::files_selected(const PackedStringArray& p_paths, uint64_t p_sequence) {
	if (p_sequence != picker_sequence) return;
	if (!input_picker_valid() || p_paths.is_empty()) {
		close_input_picker();
		return;
	}
	const auto element = picker_input;
	const auto input = element->get_input();
	TypedArray<HTMLInputFile> selected;
	const auto old_files = input->get_files();
	for (const String& path : p_paths) {
		const auto file = HTMLInputFile::open(path);
		if (file.is_null()) {
			close_input_picker();
			return;
		}
		selected.append(file);
		if (!input->get_attr("multiple")) break;
	}
	close_input_picker();
	if (old_files.size() == selected.size()) {
		bool same = true;
		for (int64_t i = 0; i < selected.size(); ++i) {
			const Ref<HTMLInputFile> old_file = old_files[i], new_file = selected[i];
			same &= old_file->file->get_path_absolute() == new_file->file->get_path_absolute();
		}
		if (same) return;
	}
	if (!input->set_files(selected)) return;
	const ObjectID id(get_instance_id());
	const auto reset = input->editor.get_reset_version();
	canvas->change_input(element, false);
	if (auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id)); label &&
		label->layout->is_element_connected(element) && element->get_input() == input &&
		input->type() == HTMLInputNode::Type::FILE && input->editor.get_reset_version() == reset) {
		label->emit_signal("input_committed", element);
	}
}

void HTMLLabel::navigate_time_picker(int p_delta) {
	if (!input_picker_valid()) return;
	const auto kind = picker_input->get_input()->time_kind();
	const int64_t year = picker_date["year"], month = picker_date["month"];
	const int64_t number = (year - 1) * 12 + month - 1 + p_delta * (kind == calendar::Kind::month ? 12 : 1);
	if (number < 0 || number >= 275760 * 12LL) return;
	picker_date["year"] = number / 12 + 1;
	picker_date["month"] = number % 12 + 1;
	picker_date["day"] = 1;
	build_time_picker();
}

void HTMLLabel::choose_time_picker_day(int p_day) {
	if (!input_picker_valid()) return;
	if (picker_input->get_input()->time_kind() == calendar::Kind::month) {
		picker_date["month"] = p_day;
		picker_date["day"] = 1;
	} else {
		picker_date["day"] = p_day;
	}
	if (picker_input->get_input()->time_kind() == calendar::Kind::local) build_time_picker();
	else commit_time_picker();
}

void HTMLLabel::change_time_picker_part(double p_value, const String& p_part) {
	picker_date[p_part] = static_cast<int64_t>(p_value);
}

void HTMLLabel::build_time_picker() {
	while (picker_content->get_child_count()) {
		auto* child = picker_content->get_child(0);
		picker_content->remove_child(child);
		child->queue_free();
	}
	const auto node = picker_input->get_input();
	const auto kind = node->time_kind();
	if (kind != calendar::Kind::time) {
		auto* header = memnew(HBoxContainer);
		picker_content->add_child(header);
		auto* prev = picker_button(*header, "", callable_mp(this, &HTMLLabel::navigate_time_picker).bind(-1));
		prev->set_button_icon(layout->get_control_icon(docrender::HTMLControlIcon::left));
		prev->set_tooltip_text("Previous");
		auto* caption = memnew(Label);
		const int64_t year = picker_date["year"], month = picker_date["month"];
		caption->set_text(kind == calendar::Kind::month ? String::num_int64(year) : vformat("%04d-%02d", year, month));
		caption->set_h_size_flags(SIZE_EXPAND_FILL);
		caption->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		header->add_child(caption);
		auto* next = picker_button(*header, "", callable_mp(this, &HTMLLabel::navigate_time_picker).bind(1));
		next->set_button_icon(layout->get_control_icon(docrender::HTMLControlIcon::right));
		next->set_tooltip_text("Next");
		auto* grid = memnew(GridContainer);
		grid->set_columns(kind == calendar::Kind::month ? 3 : 7);
		picker_content->add_child(grid);
		const int64_t selected_day = picker_date["day"];
		if (kind != calendar::Kind::month) {
			for (const char* weekday : { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" }) {
				auto* label = memnew(Label);
				label->set_text(weekday);
				label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
				grid->add_child(label);
			}
			const auto first = calendar::parse(calendar::Kind::date, vformat("%04d-%02d-01", year, month));
			if (first) {
				const int offset = (static_cast<int64_t>(*first / calendar::day) % 7 + 10) % 7;
				for (int i = 0; i < offset; ++i) grid->add_child(memnew(Control));
			}
		}
		Button* selected = nullptr;
		for (int day = 1; day <= (kind == calendar::Kind::month ? 12 : 31); ++day) {
			const auto date = calendar::parse(calendar::Kind::date, vformat("%04d-%02d-%02d", year, month, day));
			if (kind != calendar::Kind::month && !date) break;
			auto* cell = picker_button(*grid, String::num_int64(day), callable_mp(this, &HTMLLabel::choose_time_picker_day).bind(day));
			cell->set_custom_minimum_size(Vector2(30, 26));
			if (kind == calendar::Kind::week && date) cell->set_tooltip_text(calendar::format(kind, *date));
			if (day == (kind == calendar::Kind::month ? month : selected_day)) {
				cell->set_toggle_mode(true);
				cell->set_pressed_no_signal(true);
				selected = cell;
			}
		}
		if (selected) callable_mp(static_cast<Control*>(selected), &Control::grab_focus).call_deferred();
	}
	if (kind == calendar::Kind::time || kind == calendar::Kind::local) {
		auto* clock = memnew(HBoxContainer);
		picker_content->add_child(clock);
		const auto add_spin = [&](const String& key, const char* p_caption, int p_max, int p_step) {
			auto* spin = memnew(SpinBox);
			spin->set_min(0);
			spin->set_max(p_max);
			spin->set_step(p_step);
			spin->set_prefix(p_caption);
			spin->set_value(double(picker_date.get(key, 0)));
			spin->connect("value_changed", callable_mp(this, &HTMLLabel::change_time_picker_part).bind(key));
			clock->add_child(spin);
		};
		add_spin("hour", "h", 23, 1);
		add_spin("minute", "m", 59, 1);
		const double step = node->step_scale();
		const bool fractions = step < 1000 || std::fmod(step, 1000.) != 0 ||
			int64_t(picker_date.get("millisecond", 0)) != 0;
		if (fractions || std::fmod(step, 60000.) != 0 || int64_t(picker_date.get("second", 0))) add_spin("second", "s", 59, 1);
		if (fractions) add_spin("millisecond", "ms", 999, 1);
		picker_button(*picker_content, "OK", callable_mp(this, &HTMLLabel::commit_time_picker));
	}
	auto* actions = memnew(HBoxContainer);
	picker_content->add_child(actions);
	picker_button(*actions, "Clear", callable_mp(this, &HTMLLabel::commit_picker_value).bind(String()));
	picker_button(*actions, "Cancel", callable_mp(this, &HTMLLabel::close_input_picker));
}

void HTMLLabel::commit_time_picker() {
	if (!input_picker_valid()) {
		close_input_picker();
		return;
	}
	const auto kind = picker_input->get_input()->time_kind();
	const auto value = calendar::from_parts(kind, picker_date);
	if (value) commit_picker_value(calendar::format(kind, *value));
}

void HTMLLabel::commit_picker_value(const String& p_value) {
	if (!input_picker_valid()) {
		close_input_picker();
		return;
	}
	const auto element = picker_input;
	const auto input = element->get_input();
	close_input_picker();
	const String clean = input->sanitize_value(p_value);
	if (clean == input->get_value()) return;
	input->editor.set_selection(0, input->editor.get_value().length());
	if (!input->editor.insert(clean)) return;
	const ObjectID id(get_instance_id());
	canvas->change_input(element, false);
	if (auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id))) label->canvas->commit_input(element);
}
