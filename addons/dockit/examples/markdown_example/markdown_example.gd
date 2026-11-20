extends Control


const DEFAULT_MARKDOWN: String = """# Markdown Preview Lab

Edit the source and watch the preview update. Scroll either pane to follow the matching source block.

[![MarkdownLabel icon](uid://babn27hd7j422)](https://markdown.com.cn)

## Markdown basics

### Emphasis
*Italic*, **bold**, ***bold italic***, ~~strikethrough~~, and ==highlight==.

### Lists
- First item
- Second item
  - Nested item
- [x] A completed task
- [ ] A task to do

1. First step
2. Second step
   1. Nested step

### Quote
> A useful preview makes long documents easier to edit.
>
> - Lists work inside block quotes
> - So does **emphasis**

### Table
| Feature | Syntax | Preview |
| :-- | :--: | --: |
| Math | LaTeX | Yes |
| Diagrams | Mermaid | Yes |
| Editing | Markdown | Live |

## LaTeX

Inline math: $E = mc^2$ and $a^2 + b^2 = c^2$.

$$
\\int_{0}^{\\infty} e^{-x^2}\\,dx = \\frac{\\sqrt{\\pi}}{2}
$$

A second display formula:

$$
\\frac{d}{dx}\\left(\\sin x\\right)=\\cos x
$$

## Mermaid

~~~mermaid
flowchart LR
    A["Edit Markdown"] --> B{"Choose a block"}
    B --> C["LaTeX formula"]
    B --> D["Mermaid diagram"]
    C --> E["Live preview"]
    D --> E
~~~

## Code

~~~gdscript
func greet(name: String) -> void:
    print("Hello, ", name)
~~~

## HTML in Markdown

<details open>
  <summary>HTML block inside the preview</summary>
  <div style="padding: 10px; border: 1px solid #696969; border-radius: 6px; background-color: #292929;">
    <strong>Embedded HTML</strong><br />
    <span style="color: #8be9a8;">This content is rendered by the HTML layout.</span>
  </div>
</details>

The paragraph after this block is Markdown again.

## Links and footnotes

[Godot Dockit](https://godotengine.org/) renders this document locally.

A footnote reference.[^preview]

[^preview]: The source and preview panes track the corresponding document position.

---

Keep editing to try different Markdown, LaTeX, and Mermaid syntax.
"""


@onready var _code_edit: CodeEdit = %CodeEdit
@onready var _markdown_label: MarkdownLabel = %MarkdownLabel
@onready var _preview_timer: Timer = %PreviewTimer


var _code_scroll_bar: VScrollBar
var _preview_scroll_bar: VScrollBar
var _preview_theme: Theme
var _updating_preview: bool
var _syncing_scroll: bool


func _ready() -> void:
	_markdown_label.parse_latex = true
	_markdown_label.parse_html = true
	_markdown_label.parse_mermaid = true
	_preview_theme = _markdown_label.theme.duplicate() as Theme
	_merge_theme_type(preload("uid://fuqrhbfft60e"), &"LatexLabel")
	_merge_theme_type(preload("uid://dh02l0kc5xwy3"), &"MermaidLabel")
	_markdown_label.theme = _preview_theme

	_code_scroll_bar = _code_edit.get_v_scroll_bar()
	_preview_scroll_bar = _markdown_label.get_v_scroll_bar()
	if _code_scroll_bar != null:
		_code_scroll_bar.value_changed.connect(_on_code_scroll_changed)
	if _preview_scroll_bar != null:
		_preview_scroll_bar.value_changed.connect(_on_preview_scroll_changed)
	_preview_timer.timeout.connect(_update_preview)

	_code_edit.text = DEFAULT_MARKDOWN
	_preview_timer.stop()
	_update_preview()


func _merge_theme_type(p_source: Theme, p_theme_type: StringName) -> void:
	for color_text: String in p_source.get_color_list(p_theme_type):
		var color_name: StringName = StringName(color_text)
		_preview_theme.set_color(color_name, p_theme_type, p_source.get_color(color_name, p_theme_type))
	for font_size_text: String in p_source.get_font_size_list(p_theme_type):
		var font_size_name: StringName = StringName(font_size_text)
		_preview_theme.set_font_size(font_size_name, p_theme_type, p_source.get_font_size(font_size_name, p_theme_type))
	for stylebox_text: String in p_source.get_stylebox_list(p_theme_type):
		var stylebox_name: StringName = StringName(stylebox_text)
		_preview_theme.set_stylebox(stylebox_name, p_theme_type, p_source.get_stylebox(stylebox_name, p_theme_type))


func _on_code_edit_text_changed() -> void:
	_preview_timer.start()


func _on_preview_timer_timeout() -> void:
	_update_preview()


func _update_preview() -> void:
	_updating_preview = true
	_markdown_label.text = _code_edit.text
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

	var line_count: int = maxi(_code_edit.get_line_count(), 1)
	var source_line: float = clampf(_code_scroll_bar.value, 0.0, float(line_count - 1))
	var target_offset: float
	if _markdown_label.has_method("get_scroll_offset_for_source_line"):
		target_offset = float(_markdown_label.call("get_scroll_offset_for_source_line", source_line, line_count))
	else:
		target_offset = _scroll_fraction(_code_scroll_bar) * _scrollable_range(_preview_scroll_bar)

	_syncing_scroll = true
	_preview_scroll_bar.value = target_offset
	_syncing_scroll = false


func _sync_code_from_preview() -> void:
	if _code_scroll_bar == null or _preview_scroll_bar == null:
		return

	var line_count: int = maxi(_code_edit.get_line_count(), 1)
	var target_line: float
	if _markdown_label.has_method("get_source_line_at_scroll_offset"):
		target_line = float(_markdown_label.call("get_source_line_at_scroll_offset", _preview_scroll_bar.value, line_count))
	else:
		target_line = _scroll_fraction(_preview_scroll_bar) * float(line_count - 1)

	_syncing_scroll = true
	_code_scroll_bar.value = clampf(target_line, 0.0, float(line_count - 1))
	_syncing_scroll = false


func _scroll_fraction(p_scroll_bar: VScrollBar) -> float:
	var scroll_range: float = _scrollable_range(p_scroll_bar)
	if scroll_range <= 0.0:
		return 0.0
	return clampf(p_scroll_bar.value / scroll_range, 0.0, 1.0)


func _scrollable_range(p_scroll_bar: VScrollBar) -> float:
	return maxf(0.0, p_scroll_bar.max_value - p_scroll_bar.page)
