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
	_finish()


func _check(condition: bool, message: String) -> void:
	if condition:
		return
	failures += 1
	push_error("Markdown code block check failed: " + message)


func _finish() -> void:
	if failures == 0:
		print("Markdown code block checks passed")
	get_tree().quit(0 if failures == 0 else 1)
