extends Control


@onready var search_line_edit: LineEdit = %SearchLineEdit
@onready var tree: Tree = %Tree
@onready var title_label: Label = %TitleLabel
@onready var reset_button: Button = %ResetButton
@onready var tab_container: TabContainer = %TabContainer
@onready var html_code_edit: CodeEdit = %HtmlCodeEdit
@onready var css_code_edit: CodeEdit = %CssCodeEdit
@onready var gdscript_code_edit: CodeEdit = %GdscriptCodeEdit
@onready var preview_html_label: HTMLLabel = %PreviewHTMLLabel
@onready var preview_timer: Timer = %PreviewTimer

var html_file_list: PackedStringArray
var tree_item_cache: Array[TreeItem]
var selected_html_path: String
var original_html_source: String
var original_css_source: String
var original_gdscript_source: String
var preview_script_path: String
var is_loading_source: bool


func _ready() -> void:
	preview_html_label.document = null
	var directory: DirAccess = DirAccess.open("res://addons/dockit/examples/html_example/html/")
	if directory == null:
		push_error("Failed to open the HTML examples directory.")
		return

	directory.list_dir_begin()
	var file_name: String = directory.get_next()
	while not file_name.is_empty():
		if not directory.current_is_dir() and file_name.get_extension().to_lower() == "html":
			html_file_list.append("res://addons/dockit/examples/html_example/html/" + file_name)
		file_name = directory.get_next()
	directory.list_dir_end()
	html_file_list.sort()

	var root: TreeItem = tree.create_item()
	for html_file: String in html_file_list:
		var item: TreeItem = tree.create_item(root)
		item.set_text(0, html_file.get_file().get_basename())
		item.set_metadata(0, html_file)
		tree_item_cache.append(item)

	if not tree_item_cache.is_empty():
		tree.set_selected(tree_item_cache[0], 0)
		_on_tree_item_selected()
	
	html_code_edit.caret_blink = true
	css_code_edit.caret_blink = true
	gdscript_code_edit.caret_blink = true


func _exit_tree() -> void:
	if not preview_script_path.is_empty():
		_remove_preview_script(preview_script_path)


func _on_tree_item_selected() -> void:
	var item: TreeItem = tree.get_selected()
	if item == null:
		return

	var html_path: String = str(item.get_metadata(0))
	if html_path.is_empty():
		return

	_load_example(html_path, item.get_text(0))
	tab_container.current_tab = 0


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


func _on_code_edit_text_changed() -> void:
	if is_loading_source or selected_html_path.is_empty():
		return
	preview_timer.start()


func _on_preview_timer_timeout() -> void:
	_refresh_preview()


func _on_reset_button_pressed() -> void:
	if selected_html_path.is_empty():
		return

	preview_timer.stop()
	is_loading_source = true
	html_code_edit.text = original_html_source
	css_code_edit.text = original_css_source
	gdscript_code_edit.text = original_gdscript_source
	is_loading_source = false
	_refresh_preview()


func _load_example(p_html_path: String, p_title: String) -> void:
	preview_timer.stop()
	selected_html_path = p_html_path
	title_label.text = p_title
	var source_html: String = _read_file(p_html_path)
	original_html_source = _remove_stylesheet_blocks(source_html)
	original_css_source = _extract_stylesheet(source_html)
	var script_info: Dictionary = _read_gdscript_info(source_html, p_html_path)
	original_gdscript_source = script_info.get("source", "")

	is_loading_source = true
	html_code_edit.text = original_html_source
	css_code_edit.text = original_css_source
	gdscript_code_edit.text = original_gdscript_source
	is_loading_source = false
	_refresh_preview()


func _refresh_preview() -> void:
	if selected_html_path.is_empty():
		return

	var html_source: String = html_code_edit.text
	var script_info: Dictionary = _read_gdscript_info(html_source, selected_html_path)
	var previous_script_path: String = preview_script_path
	var component_name: String = str(script_info.get("component", ""))
	var runtime_script: GDScript
	if not script_info.is_empty():
		runtime_script = _compile_preview_script(gdscript_code_edit.text)
	var behavior_scripts: Dictionary[StringName, GDScript] = {}
	var component_scripts: Dictionary[StringName, GDScript] = {}
	var preview_html_source: String = _remove_stylesheet_blocks(html_source)
	if runtime_script != null:
		preview_script_path = _write_preview_script(gdscript_code_edit.text)
		if not preview_script_path.is_empty():
			preview_html_source = _replace_gdscript_source(preview_html_source, preview_script_path)
			if component_name.is_empty():
				behavior_scripts[StringName(preview_script_path)] = runtime_script
		else:
			preview_html_source = _remove_gdscript_script(preview_html_source)
	else:
		preview_script_path = ""
		preview_html_source = _remove_gdscript_script(preview_html_source)

	preview_html_label.behavior_scripts = behavior_scripts
	preview_html_label.component_scripts = component_scripts

	var preview_document: HTMLDocument = HTMLDocument.new()
	preview_document.text = preview_html_source
	preview_document.root_subfolder = selected_html_path.get_base_dir() + "/"
	preview_document.stylesheet = css_code_edit.text
	preview_html_label.document = preview_document
	if not previous_script_path.is_empty() and previous_script_path != preview_script_path:
		_remove_preview_script(previous_script_path)


func _compile_preview_script(p_source: String) -> GDScript:
	if p_source.is_empty():
		return null

	var script: GDScript = GDScript.new()
	script.source_code = p_source
	var error: Error = script.reload()
	if error != OK:
		push_error("The edited GDScript could not be compiled for this preview.")
		return null
	return script


func _read_gdscript_info(p_html: String, p_html_path: String) -> Dictionary:
	var script_pattern: RegEx = RegEx.new()
	if script_pattern.compile("(?is)<script\\b([^>]*)>(.*?)</script\\s*>") != OK:
		return {}

	for regex_match: RegExMatch in script_pattern.search_all(p_html):
		var attributes: String = regex_match.get_string(1)
		if _read_attribute(attributes, "type").nocasecmp_to("text/gdscript") != 0:
			continue

		var source_path: String = _read_attribute(attributes, "src")
		var component_name: String = _read_attribute(attributes, "component")
		var source: String = regex_match.get_string(2).strip_edges()
		if not source_path.is_empty():
			var resolved_path: String = _resolve_resource_path(source_path, p_html_path)
			if resolved_path.is_empty():
				push_warning("Could not resolve GDScript source: %s" % source_path)
			else:
				source = _read_file(resolved_path)
		return {
			"src": source_path,
			"component": component_name,
			"source": source,
		}

	return {}


func _write_preview_script(p_source: String) -> String:
	var directory_path: String = "user://dockit_html_preview"
	var absolute_directory_path: String = ProjectSettings.globalize_path(directory_path)
	var directory_error: Error = DirAccess.make_dir_recursive_absolute(absolute_directory_path)
	if directory_error != OK and directory_error != ERR_ALREADY_EXISTS:
		push_error("Could not create the temporary preview-script directory.")
		return ""

	var path: String = directory_path.path_join(selected_html_path.sha256_text() + ".gd")
	var file: FileAccess = FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		push_error("Could not write the temporary preview script.")
		return ""
	file.store_string(p_source)
	file.close()
	return path


func _replace_gdscript_source(p_html: String, p_source: String) -> String:
	var script_pattern: RegEx = RegEx.new()
	if script_pattern.compile("(?is)<script\\b([^>]*)>(.*?)</script\\s*>") != OK:
		return p_html
	var source_pattern: RegEx = RegEx.new()
	if source_pattern.compile("(?is)(?<![\\w:-])src\\s*=\\s*(?:\"[^\"]*\"|'[^']*'|[^\\s>]+)") != OK:
		return p_html

	for regex_match: RegExMatch in script_pattern.search_all(p_html):
		var attributes: String = regex_match.get_string(1)
		if _read_attribute(attributes, "type").nocasecmp_to("text/gdscript") != 0:
			continue

		var source_match: RegExMatch = source_pattern.search(attributes)
		var source_start: int = regex_match.get_start(1)
		if source_match == null:
			source_start += attributes.length()
			return p_html.substr(0, source_start) + " src=\"" + p_source + "\"" + p_html.substr(source_start)

		var start: int = source_start + source_match.get_start()
		var end: int = source_start + source_match.get_end()
		return p_html.substr(0, start) + "src=\"" + p_source + "\"" + p_html.substr(end)
	return p_html


func _remove_gdscript_script(p_html: String) -> String:
	var script_pattern: RegEx = RegEx.new()
	if script_pattern.compile("(?is)<script\\b([^>]*)>(.*?)</script\\s*>") != OK:
		return p_html

	var result: String
	var source_index: int
	for regex_match: RegExMatch in script_pattern.search_all(p_html):
		if _read_attribute(regex_match.get_string(1), "type").nocasecmp_to("text/gdscript") != 0:
			continue
		result += p_html.substr(source_index, regex_match.get_start() - source_index)
		source_index = regex_match.get_end()
	result += p_html.substr(source_index)
	return result


func _remove_preview_script(p_path: String) -> void:
	if FileAccess.file_exists(p_path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(p_path))


func _read_attribute(p_attributes: String, p_name: String) -> String:
	var attribute_pattern: RegEx = RegEx.new()
	var pattern: String = "(?is)(?<![\\w:-])" + p_name + "\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s>]+))"
	if attribute_pattern.compile(pattern) != OK:
		return ""

	var regex_match: RegExMatch = attribute_pattern.search(p_attributes)
	if regex_match == null:
		return ""
	for capture_index: int in range(1, 4):
		var value: String = regex_match.get_string(capture_index)
		if not value.is_empty():
			return value
	return ""


func _resolve_resource_path(p_source: String, p_html_path: String) -> String:
	if p_source.begins_with("uid://"):
		var uid: int = ResourceUID.text_to_id(p_source)
		if not ResourceUID.has_id(uid):
			return ""
		return ResourceUID.get_id_path(uid)
	if p_source.contains("://"):
		return p_source
	return p_html_path.get_base_dir().path_join(p_source).simplify_path()


func _extract_stylesheet(p_html: String) -> String:
	var style_pattern: RegEx = RegEx.new()
	if style_pattern.compile("(?is)<style\\b[^>]*>(.*?)</style\\s*>") != OK:
		return ""

	var blocks: PackedStringArray
	for regex_match: RegExMatch in style_pattern.search_all(p_html):
		blocks.append(regex_match.get_string(1).strip_edges())
	return "\n\n".join(blocks)


func _remove_stylesheet_blocks(p_html: String) -> String:
	var style_pattern: RegEx = RegEx.new()
	if style_pattern.compile("(?is)<style\\b[^>]*>.*?</style\\s*>(?:\\r?\\n){0,2}") != OK:
		return p_html
	return style_pattern.sub(p_html, "", true)


func _read_file(p_path: String) -> String:
	if not FileAccess.file_exists(p_path):
		push_warning("Could not read example source: %s" % p_path)
		return ""
	return FileAccess.get_file_as_string(p_path)
