extends Control


const DEFAULT_RST: String = """Dockit reStructuredText
=======================

This is a live **reStructuredText** preview. Edit the source on the left and the rendered document updates after a short pause.

.. _inline-markup:

Inline markup
-------------

Use *emphasis*, **strong text**, and ``inline code``. Links use the standard reStructuredText form: `Godot <https://godotengine.org>`_.

You can also create an in-document link to `the list section <#lists>`_.

.. _lists:

Lists
-----

Unordered lists can contain multiple items:

* First item
* Second item with **strong text**
* Third item with ``code``

Enumerated lists are supported too:

1. Write the source.
2. Review the preview.
3. Adjust the theme.

Tables
------

.. list-table:: Supported block examples
   :header-rows: 1

   * - Block
     - Example
   * - Paragraph
     - Inline markup and links
   * - Code
     - Literal or code-block directive
   * - Table
     - Grid, simple, or list-table form

Code
----

.. code-block:: gdscript

   func greet(name: String) -> void:
       print("Hello, ", name)

Admonitions
-----------

.. note::

   The preview uses the bundled ``restructuredtext_theme.tres`` theme. Try changing the text or theme while this example is open.

.. warning::

   Relative images and downloads need a source ``.rst`` document. This editor preview uses raw text, so it does not resolve relative files.

Anchors and footnotes
---------------------

Explicit targets can be used for navigation within a document. The `inline markup section <#inline-markup>`_ is an example.

Footnotes are rendered at the location of their definition.[#preview-note]_

.. [#preview-note] The RST renderer is designed for lightweight documentation previews inside Godot.
"""

@onready var _code_edit: CodeEdit = %CodeEdit
@onready var _restructuredtext_label: RestructuredtextLabel = %RestructuredtextLabel
@onready var _preview_timer: Timer = %PreviewTimer

var _code_scroll_bar: VScrollBar
var _preview_scroll_bar: VScrollBar
var _updating_preview: bool
var _syncing_scroll: bool


func _ready() -> void:
	_restructuredtext_label.set_open_external_links(true)
	_code_scroll_bar = _code_edit.get_v_scroll_bar()
	_preview_scroll_bar = _restructuredtext_label.get_v_scroll_bar()

	_code_scroll_bar.value_changed.connect(_on_code_scroll_changed)
	_preview_scroll_bar.value_changed.connect(_on_preview_scroll_changed)

	_code_edit.text = DEFAULT_RST
	_update_preview()


func _on_code_edit_text_changed() -> void:
	_preview_timer.start()


func _update_preview() -> void:
	_updating_preview = true
	_restructuredtext_label.set_rst_text(_code_edit.text)
	_updating_preview = false
	call_deferred("_sync_preview_from_code")


func _on_code_scroll_changed(_value: float) -> void:
	if not _updating_preview and not _syncing_scroll:
		_sync_preview_from_code()


func _on_preview_scroll_changed(_value: float) -> void:
	if not _updating_preview and not _syncing_scroll:
		_sync_code_from_preview()


func _sync_preview_from_code() -> void:
	if _code_scroll_bar == null or _preview_scroll_bar == null:
		return

	_syncing_scroll = true
	_preview_scroll_bar.value = _scroll_fraction(_code_scroll_bar) * _scrollable_range(_preview_scroll_bar)
	_syncing_scroll = false


func _sync_code_from_preview() -> void:
	if _code_scroll_bar == null or _preview_scroll_bar == null:
		return

	_syncing_scroll = true
	_code_scroll_bar.value = _scroll_fraction(_preview_scroll_bar) * _scrollable_range(_code_scroll_bar)
	_syncing_scroll = false


func _scroll_fraction(p_scroll_bar: VScrollBar) -> float:
	var scroll_range: float = _scrollable_range(p_scroll_bar)
	if scroll_range <= 0.0:
		return 0.0
	return clampf(p_scroll_bar.value / scroll_range, 0.0, 1.0)


func _scrollable_range(p_scroll_bar: VScrollBar) -> float:
	return maxf(0.0, p_scroll_bar.max_value - p_scroll_bar.page)


func _on_restructuredtext_label_link_pressed(uri: String) -> void:
	print("reStructuredText link activated: ", uri)


func _on_restructuredtext_label_render_warning(message: String, line: int) -> void:
	push_warning("reStructuredText line %d: %s" % [line, message])
