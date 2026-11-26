extends Control


const DEFAULT_MARKDOWN: String = """# Streaming mixed-layout preview

This scene sends the source to [MarkdownLabel] in small chunks. Try a tiny chunk size, pause and resume, then compare deferred layout on and off.

## Markdown and inline HTML

A stream can cross **formatting markers**, `inline code`, links, and a footnote.[^stream]

<div style="padding: 12px; border: 2px solid #60a5fa; border-radius: 8px; background-color: #172033;">
  <h3 style="color: #93c5fd;">An HTML block in the stream</h3>
  <p>Its layout arrives a few characters at a time, including <strong>nested markup</strong>.</p>
</div>

## Mixed table cells

| Cell content | Rendered as |
| :-- | :-- |
| **Markdown** plus <span style="color: #fbbf24;">styled HTML</span> | Inline mixed content |
| <div style="padding: 8px; border: 1px solid #34d399; background-color: #10251f;"><b>Block HTML</b><br/>inside a table cell</div> | A separate HTML layout object |

## Display math

Inline: $e^{i\\pi} + 1 = 0$

$$
\\int_0^1 x^2\\,dx = \\frac{1}{3}
$$

## Mermaid diagram

~~~mermaid
flowchart LR
    A["Receive chunk"] --> B{"Complete block?"}
    B --> C["Update Markdown layout"]
    B --> D["Keep parsing tail"]
    C --> E["Mixed preview"]
    D --> E
~~~

## Code block

~~~gdscript
func stream_message(label: MarkdownLabel, message: String) -> void:
    label.append_text(message)
~~~

[^stream]: The footnote is parsed while later chunks are still arriving.
"""


@onready var _code_edit: CodeEdit = %CodeEdit
@onready var _markdown_label: MarkdownLabel = %MarkdownLabel
@onready var _start_button: Button = %StartButton
@onready var _pause_button: Button = %PauseButton
@onready var _reset_button: Button = %ResetButton
@onready var _chunk_size: SpinBox = %ChunkSizeSpinBox
@onready var _interval: SpinBox = %IntervalSpinBox
@onready var _max_unstable_lines: SpinBox = %MaxUnstableLinesSpinBox
@onready var _flush_interval: SpinBox = %FlushIntervalSpinBox
@onready var _deferred_layout: CheckBox = %DeferredLayoutCheckBox
@onready var _scroll_follow: CheckBox = %ScrollFollowCheckBox
@onready var _progress_bar: ProgressBar = %ProgressBar
@onready var _stream_timer: Timer = %StreamTimer


var _source_text: String = ""
var _next_character: int = 0
var _chunk_count: int = 0
var _started_at_msec: int = 0
var _is_streaming: bool = false
var _is_paused: bool = false


func _ready() -> void:
	_code_edit.text = DEFAULT_MARKDOWN
	_stream_timer.wait_time = _interval.value / 1000.0
	_markdown_label.streaming_enabled = true
	_chunk_size.get_line_edit().caret_blink = true
	_interval.get_line_edit().caret_blink = true
	_max_unstable_lines.get_line_edit().caret_blink = true
	_flush_interval.get_line_edit().caret_blink = true
	_apply_layout_options()


func _on_start_button_pressed() -> void:
	_stream_timer.stop()
	_source_text = _code_edit.text
	_next_character = 0
	_chunk_count = 0
	_started_at_msec = Time.get_ticks_msec()
	_is_streaming = true
	_is_paused = false
	_markdown_label.clear()
	_apply_layout_options()
	_start_button.disabled = true
	_pause_button.disabled = false
	_pause_button.text = "Pause"
	_update_progress()
	if _source_text.is_empty():
		_finish_stream()
		return
	_stream_timer.start()


func _on_pause_button_pressed() -> void:
	if not _is_streaming:
		return
	_is_paused = not _is_paused
	if _is_paused:
		_stream_timer.stop()
		_markdown_label.flush_pending_layout()
		_pause_button.text = "Resume"
	else:
		_stream_timer.start()
		_pause_button.text = "Pause"


func _on_reset_button_pressed() -> void:
	_stream_timer.stop()
	_markdown_label.clear()
	_source_text = _code_edit.text
	_next_character = 0
	_chunk_count = 0
	_is_streaming = false
	_is_paused = false
	_start_button.disabled = false
	_pause_button.disabled = true
	_pause_button.text = "Pause"
	_progress_bar.value = 0.0


func _on_stream_timer_timeout() -> void:
	if not _is_streaming or _is_paused:
		return
	
	var chunk_length: int = mini(int(_chunk_size.value), _source_text.length() - _next_character)
	var chunk: String = _source_text.substr(_next_character, chunk_length)
	_markdown_label.append_text(chunk)
	_next_character += chunk.length()
	_chunk_count += 1
	_update_progress()
	if _next_character >= _source_text.length():
		_finish_stream()


func _on_interval_changed(value: float) -> void:
	_stream_timer.wait_time = maxf(value / 1000.0, 0.001)


func _on_layout_option_changed(_value: float) -> void:
	_apply_layout_options()


func _on_deferred_layout_toggled(enabled: bool) -> void:
	_markdown_label.deferred_layout_enabled = enabled


func _on_scroll_follow_toggled(enabled: bool) -> void:
	_markdown_label.scroll_following = enabled


func _apply_layout_options() -> void:
	_markdown_label.deferred_layout_enabled = _deferred_layout.button_pressed
	_markdown_label.max_unstable_lines = int(_max_unstable_lines.value)
	_markdown_label.layout_flush_interval_msec = int(_flush_interval.value)
	_markdown_label.scroll_following = _scroll_follow.button_pressed


func _finish_stream() -> void:
	_stream_timer.stop()
	_markdown_label.finish_stream()
	_is_streaming = false
	_is_paused = false
	_start_button.disabled = false
	_pause_button.disabled = true
	_pause_button.text = "Pause"
	_update_progress()


func _update_progress() -> void:
	var total_characters: int = _source_text.length()
	_progress_bar.value = 100.0 if total_characters == 0 else 100.0 * float(_next_character) / float(total_characters)
	var elapsed_msec: int = maxi(Time.get_ticks_msec() - _started_at_msec, 1)
	var characters_per_second: float = 1000.0 * float(_next_character) / float(elapsed_msec)
