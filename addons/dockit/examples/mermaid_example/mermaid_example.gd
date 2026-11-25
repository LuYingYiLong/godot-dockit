extends Control


@onready var search_line_edit: LineEdit = %SearchLineEdit
@onready var tree: Tree = %Tree
@onready var title_label: Label = %TitleLabel
@onready var code_edit: CodeEdit = %CodeEdit
@onready var scroll_container: ScrollContainer = %ScrollContainer
@onready var mermaid_label: MermaidLabel = %MermaidLabel
@onready var error_hint_label: Label = %ErrorHintLabel
@onready var preview_timer: Timer = %PreviewTimer

var diagram_file_list: PackedStringArray
var tree_item_cache: Array[TreeItem]
var selected_mermaid_path: String
var is_loading_source: bool
var original_mermaid_source: String


func _ready() -> void:
	mermaid_label.document = null
	var directory: DirAccess = DirAccess.open("res://addons/dockit/examples/mermaid_example/diagrams/")
	if directory == null:
		push_error("Failed to open the Diagram examples directory.")
		return

	directory.list_dir_begin()
	var file_name: String = directory.get_next()
	while not file_name.is_empty():
		if not directory.current_is_dir() and file_name.get_extension().to_lower() == "mmd":
			diagram_file_list.append("res://addons/dockit/examples/mermaid_example/diagrams/" + file_name)
		file_name = directory.get_next()
	directory.list_dir_end()
	diagram_file_list.sort()

	var root: TreeItem = tree.create_item()
	for html_file: String in diagram_file_list:
		var item: TreeItem = tree.create_item(root)
		item.set_text(0, _snake_to_title(html_file.get_file().get_basename()))
		item.set_metadata(0, html_file)
		tree_item_cache.append(item)

	if not tree_item_cache.is_empty():
		tree.set_selected(tree_item_cache[0], 0)
		_on_tree_item_selected()
	
	code_edit.caret_blink = true


func _snake_to_title(snake: String) -> String:
	var pascal: String = snake.to_pascal_case()
	var result: String
	for i: int in pascal.length():
		var c: String = pascal[i]
		if i > 0 and c == c.to_upper() and c != c.to_lower():
			result += " "
		result += c
	return result


func _on_tree_item_selected() -> void:
	var item: TreeItem = tree.get_selected()
	if item == null:
		return

	var html_path: String = str(item.get_metadata(0))
	if html_path.is_empty():
		return

	_load_example(html_path, item.get_text(0))


func _load_example(mermaid_path: String, title: String) -> void:
	preview_timer.stop()
	selected_mermaid_path = mermaid_path
	title_label.text = title
	var source_mermaid: String = _read_file(mermaid_path)
	original_mermaid_source = source_mermaid

	is_loading_source = true
	code_edit.text = original_mermaid_source
	is_loading_source = false
	_refresh_preview()


func _refresh_preview() -> void:
	if selected_mermaid_path.is_empty():
		return

	var mermaid_source: String = code_edit.text
	mermaid_label.text = mermaid_source


func _read_file(path: String) -> String:
	if not FileAccess.file_exists(path):
		push_warning("Could not read example source: %s" % path)
		return ""
	return FileAccess.get_file_as_string(path)


func _on_code_edit_text_changed() -> void:
	if is_loading_source or selected_mermaid_path.is_empty():
		return
	preview_timer.start()


func _on_preview_timer_timeout() -> void:
	_refresh_preview()


func _on_mermaid_label_render_finished() -> void:
	var error: String = mermaid_label.get_render_error()
	scroll_container.visible = error == ""
	error_hint_label.visible = error != ""
	error_hint_label.text = "Render error: " + error


func _on_reset_button_pressed() -> void:
	if selected_mermaid_path.is_empty():
		return

	preview_timer.stop()
	is_loading_source = true
	code_edit.text = original_mermaid_source
	is_loading_source = false
	_refresh_preview()


func _on_search_line_edit_text_changed(_new_text: String) -> void:
	var query: String = search_line_edit.text
	var first_result: TreeItem
	for item: TreeItem in tree_item_cache:
		item.visible = query.is_empty() or item.get_text(0).containsn(query)
		if item.visible and first_result == null:
			first_result = item

	if first_result != null:
		tree.set_selected(first_result, 0)
		_on_tree_item_selected()
