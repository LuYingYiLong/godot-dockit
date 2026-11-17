extends Node

var _failures: int
@onready var _label: HTMLLabel = $HTMLLabel


func _ready() -> void:
	for _frame: int in range(6):
		await get_tree().process_frame
	_check_types()
	_check_temporal_values()
	_check_validation()
	await _check_forms()
	_check_color()
	_check_type_changes()
	if DisplayServer.get_name() != "headless":
		await _check_picker()
		await _check_image_pixels()
		if "--native-file-picker" not in OS.get_cmdline_user_args():
			await _check_embedded_file_picker()
	if "--native-file-picker" in OS.get_cmdline_user_args():
		await _check_file_picker()
	print("HTML inputs completed with %d failures" % _failures)
	get_tree().quit(0 if _failures == 0 else 1)


func _check_types() -> void:
	var types: PackedStringArray = [
		"text", "search", "tel", "url", "email", "password", "number", "range",
		"date", "month", "week", "time", "datetime-local", "color", "file",
		"checkbox", "radio", "button", "submit", "reset", "image", "hidden",
	]
	for kind: String in types:
		var input: HTMLInputElement = _field(kind)
		_check(input != null and input.type == kind, "input type " + kind)
		if input != null:
			_check(input.get_bounding_rect().has_area() == (kind != "hidden"), "input geometry " + kind)
	_check(_field("unknown").type == "text", "an unknown type uses the text state")


func _check_temporal_values() -> void:
	var date: HTMLInputElement = _field("date")
	_check(date.value == "2024-02-29", "leap date markup")
	var leap: Dictionary = date.get_value_as_date()
	_check(leap.get("year") == 2024 and leap.get("month") == 2 and leap.get("day") == 29, "date parts")
	date.value = "1900-02-29"
	_check(date.value.is_empty() and is_nan(date.value_as_number), "invalid date sanitizes to empty")
	_check(date.set_value_as_date({"year": 2000, "month": 2, "day": 29,}) == OK and date.value == "2000-02-29",
		"date setter preserves valid leap day")
	_check(date.step_up() == OK and date.value == "2000-03-01", "date step crosses month")

	var week: HTMLInputElement = _field("week")
	_check(week.value == "2020-W53" and week.step_up() == OK and week.value == "2021-W01", "ISO weeks cross year")
	week.value = "2021-W53"
	_check(week.value.is_empty(), "year without ISO week 53")

	var month: HTMLInputElement = _field("month")
	_check(month.value_as_number == -1.0 and month.step_up() == OK and month.value == "1970-01", "month numeric epoch")
	var time: HTMLInputElement = _field("time")
	_check(time.value_as_number == 86399999.0 and time.step_up() == OK and time.value == "00:00", "fractional time step")
	time.set_attribute("min", "23:00")
	time.set_attribute("max", "04:00")
	time.value = "02:00"
	_check(time.get_validity().get("valid") == true, "time can span midnight")
	time.value = "12:00"
	_check(time.get_validity().get("valid") == false, "midday falls outside a midnight range")
	var local: HTMLInputElement = _field("datetime-local")
	_check(local.value == "2024-03-31T03:12" and local.get_value_as_date().is_empty(), "local time has a normalized value")


func _check_validation() -> void:
	var text: HTMLInputElement = _field("text")
	text.value = "ABC"
	_check(text.get_validity().get("pattern_mismatch") == false, "pattern valid text")
	text.value = "ABC1"
	_check(text.get_validity().get("pattern_mismatch") == true, "pattern must match whole text")
	text.set_attribute("readonly", "")
	_check(text.get_validity().get("will_validate") == false, "readonly invalid text is not a validation candidate")
	var email: HTMLInputElement = _field("email")
	email.value = "one@example.com, two@example.org"
	_check(email.get_validity().get("valid") == true, "multiple email accepts comma-separated addresses")
	email.value = "one@example.com, example.org"
	_check(email.get_validity().get("type_mismatch") == true, "multiple email checks every address")
	email.value = ""
	var number: HTMLInputElement = _field("number")
	number.value = "3"
	_check(number.get_validity().get("step_mismatch") == true, "off-step number stays editable and invalid")
	_check(number.step_up() == OK and number.value_as_number == 4.0, "step advances to the next permitted number")
	number.value = "20"
	_check(number.get_validity().get("range_overflow") == true, "number overflow")
	number.set_attribute("step", "any")
	_check(number.step_down() == ERR_UNAVAILABLE, "step any rejects a scripted step")
	_check(_field("password").get_selection_start() >= 0 and number.get_selection_start() == -1,
		"text selection API applies only to HTML selection types")


func _check_forms() -> void:
	var form: HTMLFormElement = _label.get_element_by_id("form") as HTMLFormElement
	var file: HTMLInputElement = _field("file")
	file.value = "res://private.txt"
	_check(file.value.is_empty() and file.get_files().is_empty() and file.get_validity().get("value_missing") == true,
		"scripts cannot select arbitrary files by setting value")
	_check(_label.submit_form(form) == ERR_INVALID_DATA, "validation blocks an empty required upload")
	var named: Array[Dictionary] = form.get_form_data(_field("image"))
	_check(_entry(named, "photo.x") == "0" and _entry(named, "photo.y") == "0", "image submit uses default coordinates")
	var empty: HTMLInputFile = _entry(named, "upload") as HTMLInputFile
	_check(empty != null and empty.size == 0 and empty.name.is_empty(), "form data contains the empty upload file")
	_field("image").remove_attribute("name")
	var unnamed: Array[Dictionary] = form.get_form_data(_field("image"))
	_check(_entry(unnamed, "x") == "0" and _entry(unnamed, "y") == "0", "unnamed image emits coordinate names")
	var checkbox: HTMLInputElement = _field("checkbox")
	checkbox.indeterminate = true
	_check(checkbox.matches(":indeterminate"), "checkbox sets its indeterminate pseudo-class")
	await get_tree().process_frame
	await get_tree().process_frame
	_check(_label.activate_element(checkbox) and checkbox.checked and not checkbox.indeterminate,
		"user activation toggles and clears indeterminate")
	_check(form.reset() == OK and not checkbox.checked and _field("hidden").value == "token", "form reset restores input defaults")


func _check_color() -> void:
	var color: HTMLInputElement = _field("color")
	color.value = "red"
	_check(color.value == "#ff0000", "color serializes CSS names to limited sRGB")
	color.value = "not-a-color"
	_check(color.value == "#000000", "invalid color defaults to black")
	color.set_attribute("alpha", "")
	color.set_attribute("colorspace", "display-p3")
	color.value = "color(display-p3 1 0 0 / 0.5)"
	_check(color.value.begins_with("color(display-p3 1") and color.value.contains("/ 0.5"), "P3 and alpha are preserved")


func _check_type_changes() -> void:
	var input: HTMLInputElement = _field("unknown")
	input.value = "edited"
	input.type = "hidden"
	_check(input.value == "edited" and input.get_attribute("value") == "edited", "value is transferred to reflected state")
	input.type = "text"
	_check(input.value == "edited", "value is restored from attribute when entering text mode")
	input.type = "file"
	_check(input.value.is_empty(), "changing to filename mode resets the state")
	input.type = "search"
	_check(input.type == "search" and input.is_valid(), "changing type preserves the element handle")


func _check_picker() -> void:
	for _frame: int in range(4):
		await get_tree().process_frame
	var date: HTMLInputElement = _field("date")
	var result: Error = date.show_picker()
	_check(result == OK, "opens the date picker (%s)" % error_string(result))
	await get_tree().process_frame
	var popup: PopupPanel = _label.get_node_or_null(^"_HTMLInputPicker") as PopupPanel
	_check(popup != null and popup.visible, "date popup is visible")
	if popup != null:
		popup.hide()
	await get_tree().process_frame
	await get_tree().process_frame
	_check(date.value == "2024-02-29", "cancel leaves date unchanged")


func _check_image_pixels() -> void:
	await RenderingServer.frame_post_draw
	var bounds: Rect2 = _field("image").get_bounding_rect()
	var pixels: Image = get_viewport().get_texture().get_image()
	var point: Vector2i = Vector2i(bounds.get_center())
	_check(get_viewport().get_visible_rect().has_point(Vector2(point)), "the image lies within the viewport")
	var pixel: Color = pixels.get_pixelv(point)
	_check(pixel.g > 0.6 and pixel.r < 0.35, "input image paints the imported green image")
	var received: Array[Dictionary]
	_label.form_submitted.connect(
		func(_form: HTMLFormElement, entries: Array[Dictionary], _submitter: HTMLElement) -> void:
			received.assign(entries),
		CONNECT_ONE_SHOT,
	)
	for state: bool in [true, false]:
		var click: InputEventMouseButton = InputEventMouseButton.new()
		click.button_index = MOUSE_BUTTON_LEFT
		click.pressed = state
		click.position = bounds.position + Vector2(15.0, 9.0)
		get_viewport().push_input(click, true)
	_check(_entry(received, "x") == "10" and _entry(received, "y") == "6",
		"image click submission uses inverse-transformed CSS coordinates")


func _check_file_picker() -> void:
	print("Pick fixture ", _write_fixture("input-fixture.txt", "sample"))
	var field: HTMLInputElement = _field("file")
	var result: Error = field.show_picker()
	_check(result == OK, "opens a native file picker")
	if result != OK:
		return
	var timeout: SceneTreeTimer = get_tree().create_timer(90.0)
	while field.get_files().is_empty() and timeout.time_left > 0.0:
		await get_tree().process_frame
	var picked: Array[HTMLInputFile] = field.get_files()
	_check(picked.size() == 1, "picker returns the selected file")
	if picked.size() != 1:
		return
	var file: HTMLInputFile = picked[0]
	_check(file.name == "input-fixture.txt" and file.size == 6 and file.read_bytes(1, 3).get_string_from_utf8() == "amp",
		"selected file can be read by range")
	var form: HTMLFormElement = _label.get_element_by_id("form") as HTMLFormElement
	_check(_entry(form.get_form_data(), "upload") == file and not field.get_validity().get("value_missing"),
		"form data carries the selected file")
	_check((field.clone_node() as HTMLInputElement).get_files().is_empty(), "cloning does not copy file access")
	form.reset()
	_check(field.get_files().is_empty() and file.read_bytes(0, 6).get_string_from_utf8() == "sample",
		"form reset clears the selection and retained handles stay readable")


func _check_embedded_file_picker() -> void:
	get_tree().root.gui_embed_subwindows = true
	var window: Window = Window.new()
	window.size = Vector2i(750, 490)
	add_child(window)
	_label.reparent(window, false)
	for _frame: int in range(6):
		await get_tree().process_frame
	var field: HTMLInputElement = _field("file")
	var fixture: String = _write_fixture("input-fixture.txt", "sample")
	var second: String = _write_fixture("second-input.txt", "other")
	_check(field.show_picker() == OK, "opens the embedded file picker")
	var dialog: FileDialog = _label.get_node_or_null(^"_HTMLFilePicker") as FileDialog
	_check(dialog != null and dialog.file_mode == FileDialog.FILE_MODE_OPEN_FILE, "single file picker mode")
	if dialog == null:
		return
	dialog.emit_signal("file_selected", fixture)
	var files: Array[HTMLInputFile] = field.get_files()
	_check(files.size() == 1 and files[0].read_bytes(1, 3).get_string_from_utf8() == "amp", "selected file bytes")
	_check(field.value == "C:\\fakepath\\input-fixture.txt", "file value exposes only a fake path")
	field.set_attribute("multiple", "")
	for _frame: int in range(4):
		await get_tree().process_frame
	_check(field.show_picker() == OK, "opens a second embedded picker")
	dialog = _label.get_node_or_null(^"_HTMLFilePicker") as FileDialog
	_check(dialog != null and dialog.file_mode == FileDialog.FILE_MODE_OPEN_FILES, "multiple file picker mode")
	if dialog == null:
		return
	dialog.emit_signal("files_selected", PackedStringArray([fixture, second,]))
	files = field.get_files()
	_check(files.size() == 2 and files[1].name == "second-input.txt", "multiple files preserve selection order")
	var form: HTMLFormElement = _label.get_element_by_id("form") as HTMLFormElement
	var entries: Array[HTMLInputFile]
	for entry: Dictionary in form.get_form_data():
		if entry.get("name") == "upload":
			entries.append(entry.get("value") as HTMLInputFile)
	_check(entries == files, "form data preserves all selected file handles")
	_check((field.clone_node() as HTMLInputElement).get_files().is_empty(), "cloning does not copy file access")
	await get_tree().process_frame
	_check(field.show_picker() == OK, "reopens a file picker")
	dialog = _label.get_node_or_null(^"_HTMLFilePicker") as FileDialog
	if dialog != null:
		dialog.emit_signal("canceled")
	_check(field.get_files() == files, "cancel preserves the selected file handles")
	await get_tree().process_frame
	_check(field.show_picker() == OK, "opens a picker before the type is changed")
	dialog = _label.get_node_or_null(^"_HTMLFilePicker") as FileDialog
	if dialog != null:
		field.type = "text"
		field.type = "file"
		dialog.emit_signal("files_selected", PackedStringArray([fixture,]))
	_check(field.get_files().is_empty(), "a type change discards a stale file picker result")
	_check(not files.is_empty() and files[0].read_bytes(0, 6).get_string_from_utf8() == "sample",
		"retained file handles remain readable")


func _write_fixture(name: String, contents: String) -> String:
	var path: String = "user://" + name
	var output: FileAccess = FileAccess.open(path, FileAccess.WRITE)
	output.store_string(contents)
	output.close()
	return ProjectSettings.globalize_path(path)


func _field(id: String) -> HTMLInputElement:
	return _label.get_element_by_id(id) as HTMLInputElement


func _entry(entries: Array[Dictionary], name: String) -> Variant:
	for entry: Dictionary in entries:
		if entry.get("name") == name:
			return entry.get("value")
	return null


func _check(condition: bool, message: String) -> void:
	if condition:
		return
	_failures += 1
	push_error(message)
