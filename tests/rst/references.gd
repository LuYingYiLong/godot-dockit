extends Node

var failures: int
@onready var viewer: RestructuredtextLabel = $Viewer


func _ready() -> void:
	await get_tree().process_frame
	var resource: Resource = load("res://docs/index.rst")
	var document: RSTDocument = resource as RSTDocument
	_check(document != null, "the .rst importer loads an RSTDocument")
	if document == null:
		_finish()
		return

	viewer.set_document(document)
	viewer.refresh()
	for _frame: int in range(4):
		await get_tree().process_frame
	_double_click_at(Vector2(48.0, 20.0))
	_check(not viewer.get_selected_text().is_empty(), "RestructuredtextLabel selects a word on double-click")
	viewer.clear_selection()
	viewer.set_rst_text(".. code-block:: gdscript\n\n  func greet(name: String) -> void:\n      print(\"Hello, \", name)")
	viewer.refresh()
	viewer.select_all()
	_check(viewer.get_selected_text() == "func greet(name: String) -> void:\n    print(\"Hello, \", name)", "code-block rendering preserves relative indentation")
	viewer.clear_selection()
	viewer.set_rst_text(".. code-block:: gdscript\n\n  func greet(name: String) -> void:\n  \tprint(\"Hello, \", name)")
	viewer.refresh()
	viewer.select_all()
	_check(viewer.get_selected_text() == "func greet(name: String) -> void:\n\tprint(\"Hello, \", name)", "RST code blocks retain tab characters")
	viewer.clear_selection()
	viewer.set_document(document)
	viewer.refresh()
	var index: Resource = document.get_reference_index()
	_check(index != null, "the imported document includes the generated reference index")
	var generated_index: Resource = load("res://dockit_generated/rst_reference_index.tres")
	_check(generated_index != null, "the project-wide reference index is loadable as a resource")
	if index == null:
		_finish()
		return

	var document_uri: String = viewer.resolve_link_uri("doc", "guide#guide-section")
	_check(document_uri.ends_with("#guide-section"), ":doc: resolves a relative document and fragment")
	_check(document_uri.begins_with("uid://") or document_uri.begins_with("res://"), ":doc: resolves to a UID or resource path")

	var reference_uri: String = viewer.resolve_link_uri("ref", "guide-section")
	_check(reference_uri.ends_with("#guide-section"), ":ref: resolves a label to its document anchor")

	var image_path: String = viewer.resolve_document_path("assets/picture.svg")
	_check(FileAccess.file_exists(image_path), "relative image paths resolve from the source document")
	var download_uri: String = viewer.resolve_link_uri("download", "downloads/notes.txt")
	_check(FileAccess.file_exists(download_uri), ":download: resolves a relative file path")

	var document_targets: Dictionary = index.call("get_document_targets")
	var guide_record: Dictionary = document_targets.get("res://docs/guide.rst", {})
	_check(guide_record.get("path", "") == "res://docs/guide.rst", "the generated index retains the fallback resource path")
	var original_targets: Dictionary = document_targets
	var fallback_targets: Dictionary = {
		"res://docs/fallback.rst": {
			"uid": "",
			"path": "res://docs/fallback.rst",
			"anchor": "",
		},
	}
	index.call("set_document_targets", fallback_targets)
	_check(index.call("resolve_document", "res://docs/fallback.rst") == "res://docs/fallback.rst", "a missing UID falls back to the stored resource path")
	index.call("set_document_targets", original_targets)

	_finish()


func _check(condition: bool, message: String) -> void:
	if condition:
		return
	failures += 1
	push_error("RST reference check failed: " + message)


func _finish() -> void:
	if failures == 0:
		print("RST reference checks passed")
	get_tree().quit(0 if failures == 0 else 1)


func _double_click_at(point: Vector2) -> void:
	for click_index: int in range(2):
		for state: bool in [true, false]:
			var click: InputEventMouseButton = InputEventMouseButton.new()
			click.button_index = MOUSE_BUTTON_LEFT
			click.pressed = state
			click.double_click = state and click_index == 1
			click.position = point
			get_viewport().push_input(click, true)
