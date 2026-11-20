extends HTMLComponent


var _title_slot: HTMLSlotElement
var _title: HTMLElement
var _status: HTMLElement
var _toggle_button: HTMLButtonElement


func _mount() -> void:
	_title_slot = shadow_root.query_selector("#title-slot") as HTMLSlotElement
	_title = host.query_selector("[slot='title']")
	_status = shadow_root.query_selector("#status")
	_toggle_button = shadow_root.query_selector("#toggle-title") as HTMLButtonElement

	if _title_slot != null:
		_title_slot.add_event_listener(&"slotchange", _on_title_slot_changed)

	if _toggle_button != null:
		_toggle_button.add_event_listener(&"click", _on_toggle_button_clicked)
		_toggle_button.disabled = _title == null

	_update_status()


func _unmount() -> void:
	if _title_slot != null and _title_slot.is_valid():
		_title_slot.remove_event_listener(&"slotchange", _on_title_slot_changed)

	if _toggle_button != null and _toggle_button.is_valid():
		_toggle_button.remove_event_listener(&"click", _on_toggle_button_clicked)


func _on_toggle_button_clicked(_event: HTMLEvent) -> void:
	if _title == null or not _title.is_valid() or _title_slot == null or not _title_slot.is_valid():
		return

	var title_is_assigned: bool = _title_slot.assigned_nodes().size() > 0
	_title.set_attribute("slot", "unassigned-title" if title_is_assigned else "title")


func _on_title_slot_changed(_event: HTMLEvent) -> void:
	_update_status()


func _update_status() -> void:
	if _status == null or not _status.is_valid() or _title_slot == null or not _title_slot.is_valid():
		return

	var title_is_assigned: bool = _title_slot.assigned_nodes().size() > 0
	_status.set_text_content(
		"Title is assigned to the named slot." if title_is_assigned else "The title slot fallback is visible."
	)
