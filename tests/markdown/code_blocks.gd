extends Node

var failures: int = 0


func _ready() -> void:
	await get_tree().process_frame
	var label: MarkdownLabel = MarkdownLabel.new()
	add_child(label)
	label.set_markdown_text("```gdscript\nfunc greet(name: String) -> void:\n    print(\"Hello, \", name)\n```")
	label.refresh()
	label.select_all()
	_check(
		label.get_selected_text() == "func greet(name: String) -> void:\n    print(\"Hello, \", name)",
		"Markdown fenced code preserves relative indentation",
	)
	label.set_markdown_text("~~~gdscript\nfunc greet(name: String) -> void:\n\tprint(\"Hello, \", name)\n~~~")
	label.refresh()
	label.select_all()
	_check(
		label.get_selected_text() == "func greet(name: String) -> void:\n\tprint(\"Hello, \", name)",
		"Markdown fenced code retains tab characters",
	)
	await _check_font_size_theme_category()
	_finish()


func _check_font_size_theme_category() -> void:
	var label: MarkdownLabel = MarkdownLabel.new()
	label.theme_type_variation = &"MarkdownLabel"
	label.size = Vector2(320.0, 160.0)
	add_child(label)
	label.set_markdown_text("# Theme size")
	await get_tree().process_frame

	var default_theme: Theme = Theme.new()
	label.theme = default_theme
	label.refresh()
	var default_height: float = label.get_content_height()

	var constant_theme: Theme = Theme.new()
	constant_theme.set_constant(&"h1_font_size", &"MarkdownLabel", 1)
	label.theme = constant_theme
	label.refresh()
	var constant_height: float = label.get_content_height()
	_check(is_equal_approx(default_height, constant_height), "Markdown font sizes ignore constant-list entries")

	var font_size_theme: Theme = Theme.new()
	font_size_theme.set_font_size(&"h1_font_size", &"MarkdownLabel", 40)
	label.theme = font_size_theme
	label.refresh()
	_check(label.get_content_height() > default_height, "Markdown font sizes are read from the font-size list")
	label.queue_free()


func _check(condition: bool, message: String) -> void:
	if condition:
		return
	failures += 1
	push_error("Markdown code block check failed: " + message)


func _finish() -> void:
	if failures == 0:
		print("Markdown code block checks passed")
	get_tree().quit(0 if failures == 0 else 1)
