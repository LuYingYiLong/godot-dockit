extends SceneTree

const SOURCE: String = """
<!doctype html>
<style>
	html, body { margin: 0; background: white; }
	#outer { width: 168px; height: 118px; border: 0; padding: 6px; background: #f00000; }
	#outer::backdrop { background: #2040c0; }
	#inner { width: 108px; height: 54px; border: 0; padding: 6px; background: #f0c000; }
	#inner::backdrop { background: linear-gradient(90deg, #008000, #008000); opacity: 0.75; }
	#clipped { overflow: hidden; width: 5px; height: 5px; transform: translateX(2px); }
	#spacer { height: 1000px; }
</style>
<button id="opener" type="button" style="position:fixed;inset:0 auto auto 0">Open</button>
<div id="clipped"><dialog id="outer"><input id="edit" autofocus><button id="tail" type="button">Tail</button>
	<select id="choice" tabindex="-1"><option>Alpha</option><option>Beta</option></select>
	<span>Outer text</span></dialog></div>
<dialog id="inner"><button id="confirm" type="button">Confirm</button></dialog>
<dialog id="markup" open><button id="regular" type="button">Regular</button></dialog>
<div id="spacer">Background text</div>
"""

var failures: int
var frame: SubViewport
var label: HTMLLabel
var closed_values: Array[String] = []
var restored_focus: Array[HTMLElement] = []


func _initialize() -> void:
	_run.call_deferred()


func _run() -> void:
	frame = SubViewport.new()
	frame.size = Vector2i(360, 260)
	frame.gui_disable_input = false
	frame.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(frame)
	var native: Button = Button.new()
	native.position = Vector2(0, 220)
	native.size = Vector2(42, 32)
	frame.add_child(native)
	label = HTMLLabel.new()
	label.size = Vector2(320, 200)
	label.text = SOURCE
	frame.add_child(label)
	await _settle()

	var outer: HTMLDialogElement = label.get_element_by_id("outer") as HTMLDialogElement
	var inner: HTMLDialogElement = label.get_element_by_id("inner") as HTMLDialogElement
	var regular: HTMLDialogElement = label.get_element_by_id("markup") as HTMLDialogElement
	var opener: HTMLElement = label.get_element_by_id("opener")
	var edit: HTMLElement = label.get_element_by_id("edit")
	var tail: HTMLElement = label.get_element_by_id("tail")
	var confirm: HTMLElement = label.get_element_by_id("confirm")
	_check(outer != null and inner != null and regular != null, "dialog handle factory")
	if failures:
		quit(1)
		return
	regular.open = false
	_check(outer.get_bounding_rect() == Rect2(), "closed dialog has no box")
	_check(regular.show() == OK and regular.matches(":open") and not regular.is_modal(), "regular show")
	_check(regular.show_modal() == ERR_INVALID_PARAMETER, "regular dialog rejects modal reopen")
	regular.open = false
	var detached: HTMLDialogElement = outer.clone_node(true) as HTMLDialogElement
	_check(detached.show_modal() == ERR_INVALID_PARAMETER, "detached modal rejected")

	native.grab_focus()
	label.set_scroll_offset(Vector2i(0, 270))
	await _settle()
	var size_before: Vector2i = label.get_content_size()
	outer.closed.connect(_on_closed)
	inner.closed.connect(_on_closed)
	_check(outer.show_modal() == OK and outer.show_modal() == OK, "modal show is idempotent")
	_check(outer.show() == ERR_INVALID_PARAMETER and outer.matches(":modal"), "modal state and mode guard")
	await _settle()
	_check(label.get_focused_element() == edit, "autofocus inside transformed and clipped ancestor")
	_check(label.get_content_size() == size_before and label.get_scroll_offset().y == 270, "modal preserves document size and scroll")
	var modal_center: Vector2 = outer.get_bounding_rect().get_center() - Vector2(label.get_scroll_offset())
	_check(modal_center.distance_to(Vector2(154, 100)) < 8.0, "viewport-centered modal at document scroll offset")
	_check(label.get_element_at(Vector2(7, 8)) == null, "background is excluded from hit testing")
	_check(not label.activate_element(opener) and not label.focus_element(opener), "background script interaction blocked")
	_check(_pixel(Vector2i(8, 10)).is_equal_approx(Color("#2040c0")), "backdrop paints over background")
	_check(not label.get_computed_style_property(outer, "background-color", "::backdrop").is_empty(),
		"backdrop has a computed style")
	_check(_pixel(Vector2i(125, 130)).r > 0.8, "modal paints above its backdrop")
	var wheel: InputEventMouseButton = InputEventMouseButton.new()
	wheel.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wheel.position = Vector2(8, 10)
	wheel.pressed = true
	frame.push_input(wheel, true)
	await _settle()
	_check(label.get_scroll_offset().y == 270, "wheel over backdrop cannot scroll the document")
	label.select_all()
	_check("Outer text" in label.get_selected_text() and "Background text" not in label.get_selected_text(),
		"select all is scoped to modal text")
	label.clear_selection()
	label.set_scroll_offset(Vector2i(0, 310))
	await _settle()
	_check((outer.get_bounding_rect().get_center() - Vector2(label.get_scroll_offset())).is_equal_approx(modal_center),
		"modal stays fixed after viewport scroll")
	_check(label.get_element_at(edit.get_bounding_rect().get_center() - Vector2(label.get_scroll_offset())) == edit,
		"hit testing follows the fixed modal after scroll")
	_key(KEY_TAB)
	_check(label.get_focused_element() == tail, "Tab advances in dialog")
	_key(KEY_TAB)
	_check(label.get_focused_element() == edit, "Tab wraps in dialog")
	_key(KEY_TAB, true)
	_check(label.get_focused_element() == tail, "Shift+Tab wraps backwards")
	var choice: HTMLElement = label.get_element_by_id("choice")
	_check(label.focus_element(choice), "script can focus a negative-tabindex control in the modal")
	_key(KEY_SPACE)
	_key(KEY_ESCAPE, false, true)
	await _settle()
	_check(outer.open and label.get_focused_element() == choice, "Escape dismisses an open select menu before the dialog")
	_check(label.focus_element(tail), "restore a known opener for the nested dialog")

	inner.return_value = "kept"
	_check(inner.show_modal() == OK, "nested modal opens")
	await _settle()
	_check(label.get_focused_element() == confirm, "top dialog gains focus")
	_check(not label.focus_element(edit), "lower modal is excluded from focus")
	_check(_pixel(Vector2i(8, 10)).g > _pixel(Vector2i(8, 10)).b, "top gradient backdrop composites above lower backdrop")
	_key(KEY_ESCAPE, false, true)
	await _settle()
	_check(not inner.open and outer.open and label.get_focused_element() == tail, "Escape closes only top dialog and restores its opener")
	_check(closed_values == ["kept"] and restored_focus == [tail], "closed signal follows focus restoration")
	_key(KEY_ESCAPE)
	await _settle()
	_check(not outer.open and frame.gui_get_focus_owner() == native, "closing final modal restores external Godot focus")

	_check(label.focus_element(opener), "background focus is available again")
	_check(outer.show_modal() == OK, "reopen after dismissal")
	await _settle()
	_check(inner.show_modal() == OK, "reopen nested modal")
	await _settle()
	outer.open = false
	await _settle()
	_check(not outer.is_modal() and inner.is_modal() and label.get_focused_element() == confirm,
		"removing a covered modal does not steal focus")
	_check(inner.close("") == OK and inner.close("ignored") == OK, "close is idempotent")
	await _settle()
	_check(label.get_focused_element() == opener and closed_values == ["kept", "", ""], "covered removal preserves the original restore point")
	_check(outer.show_modal() == OK, "open before DOM removal")
	await _settle()
	_check(outer.detach() == OK and not outer.is_modal(), "detaching clears the modal layer")
	await _settle()
	_check(label.get_focused_element() == opener, "detaching a modal restores focus")
	var standalone: HTMLLayout = HTMLLayout.new()
	_check(standalone.parse("<dialog id='empty'></dialog>") == OK, "standalone layout parses a dialog")
	var empty: HTMLDialogElement = standalone.get_element_by_id("empty") as HTMLDialogElement
	empty.return_value = "adopted"
	_check(empty.show_modal() == OK and standalone.layout(Vector2i(160, 100)) == OK,
		"standalone layout can open and lay out a modal")
	var clone: HTMLDialogElement = empty.clone_node(true) as HTMLDialogElement
	_check(clone.open and not clone.is_modal() and clone.return_value.is_empty(), "cloning copies the open attribute")
	var destination: HTMLLayout = HTMLLayout.new()
	_check(destination.parse("<main></main>") == OK and destination.adopt_node(empty) == empty, "adopt modal into another layout")
	_check(empty.open and not empty.is_modal() and not empty.is_connected() and empty.return_value == "adopted",
		"adoption removes modal membership and keeps the handle and result")
	label.text = "<button>Replacement</button>"
	await _settle()
	_check(not outer.is_valid() and not inner.is_valid(), "reparse invalidates previous dialog handles")
	if failures == 0:
		print("HTML dialog checks passed")
	quit(0 if failures == 0 else 1)


func _on_closed(value: String) -> void:
	closed_values.append(value)
	restored_focus.append(label.get_focused_element())


func _settle() -> void:
	for _iteration: int in range(4):
		await process_frame
	await RenderingServer.frame_post_draw


func _pixel(position: Vector2i) -> Color:
	return frame.get_texture().get_image().get_pixelv(position)


func _key(code: Key, shift: bool = false, repeat: bool = false) -> void:
	var event: InputEventKey = InputEventKey.new()
	event.keycode = code
	event.shift_pressed = shift
	event.pressed = true
	frame.push_input(event, true)
	if repeat:
		event.echo = true
		frame.push_input(event, true)
		event.echo = false
	event.pressed = false
	frame.push_input(event, true)


func _check(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error(description)
