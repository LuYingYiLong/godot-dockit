extends SceneTree


var _failures: Array[String]


func _initialize() -> void:
	_run.call_deferred()


func _check(p_passed: bool, p_message: String) -> void:
	if not p_passed:
		_failures.append(p_message)
		push_error(p_message)


func _run() -> void:
	var label: RestructuredtextLabel = RestructuredtextLabel.new()
	label.size = Vector2(620.0, 520.0)
	get_root().add_child(label)
	label.rst_text = """.. _intro:

RST Heading
===========

A paragraph with **bold**, *italic*, ``literal``, and `Link <https://example.org>`_.

.. note:: Important note body.

.. code-block:: gdscript

    func greet():
        print("hello")

.. _table:

+---+---+
| A | B |
+===+===+
| C | D |
+---+---+

Footnote reference [1]_.

.. [1] Footnote definition.
"""
	await process_frame
	await process_frame
	label.select_all()
	var selected_text: String = label.get_selected_text()
	_check(label.get_rendered_block_count() >= 7, "RST blocks did not reach the shared HTML renderer")
	_check(selected_text.contains("RST Heading") and selected_text.contains("Important note body"), "RST block text is missing from selection")
	_check(selected_text.contains("bold") and selected_text.contains("literal") and selected_text.contains("Link"), "RST inline markup was not converted to selectable DOM text")
	_check(selected_text.contains("    print(\"hello\")"), "RST code block indentation was lost")
	_check(selected_text.contains("A") and selected_text.contains("B") and selected_text.contains("C") and selected_text.contains("D"), "RST table cell text is missing")
	_check(label.has_anchor("intro") and label.has_anchor("table"), "RST target anchors were not exposed by HTMLLayout")
	_check(label.scroll_to_anchor("table"), "RST table anchor did not have a layout offset")
	label.size.y = 120.0
	await process_frame
	await process_frame
	var signal_scroll_values: Array[float] = []
	label.link_pressed.connect(func(_uri: String) -> void: signal_scroll_values.append(label.get_v_scroll_bar().value))
	label.get_v_scroll_bar().value = 0.0
	label.activate_link_uri("#rst-footnote-def-1")
	_check(not signal_scroll_values.is_empty() and signal_scroll_values[0] > 0.0, "RST footnote anchor was not scrolled into view before link_pressed")
	var base_height: float = label.get_content_height()
	var stylesheet: CSSStyleSheet = CSSStyleSheet.new()
	stylesheet.css_text = ".rst-admonition-note { margin-bottom: 64px; }"
	label.stylesheet = stylesheet
	await process_frame
	await process_frame
	var styled_height: float = label.get_content_height()
	_check(styled_height > base_height + 30.0, "RST author stylesheet did not affect internal HTML layout")
	_check(label.has_anchor("intro") and label.get_rendered_block_count() >= 7, "RST DOM queries became stale after stylesheet changes")
	stylesheet.css_text = ".rst-admonition-note { margin-bottom: 96px; }"
	await process_frame
	await process_frame
	var reloaded_style_height: float = label.get_content_height()
	_check(reloaded_style_height > styled_height + 20.0, "Editing an assigned CSSStyleSheet did not reapply its rules")
	var theme: Theme = Theme.new()
	var header_panel: StyleBoxFlat = StyleBoxFlat.new()
	header_panel.set_content_margin(SIDE_TOP, 72.0)
	theme.set_stylebox(&"admonition_note_header_panel", &"RestructuredtextLabel", header_panel)
	label.theme = theme
	await process_frame
	await process_frame
	_check(label.get_content_height() > reloaded_style_height + 50.0, "RST admonition header StyleBox did not reach its HTML title element")
	label.rst_text = "x".repeat(180)
	label.autowrap_mode = TextServer.AUTOWRAP_ARBITRARY
	await process_frame
	await process_frame
	var wrapped_height: float = label.get_content_height()
	label.autowrap_mode = TextServer.AUTOWRAP_OFF
	await process_frame
	await process_frame
	_check(wrapped_height > label.get_content_height() + 10.0, "RST autowrap mode was not mapped to HTML layout")
	label.clear_document()
	await process_frame
	_check(label.get_selected_text().is_empty() and label.get_rendered_block_count() == 0, "Clearing RST left stale HTML content")
	label.queue_free()
	await process_frame
	print("Restructuredtext layout regression: %d failures" % _failures.size())
	quit(0 if _failures.is_empty() else 1)
