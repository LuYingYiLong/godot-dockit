extends Node

var failures: int
@onready var label: HTMLLabel = $Label


func _ready() -> void:
	await _check_dispatch()
	await _check_delegation()
	await _check_click_and_context_menu()
	await _check_keyboard()
	await _check_form()
	if failures == 0:
		print("HTML event checks passed")
	get_tree().quit(0 if failures == 0 else 1)


func _check_dispatch() -> void:
	label.text = """<div id="outer"><div id="mid"><button id="inner" type="button">Inner</button></div></div>"""
	label.size = Vector2(420.0, 240.0)
	await _settle()
	var dom: HTMLDocumentNode = label.get_document_node()
	_check(dom != null, "the label exposes its document node")
	if dom == null:
		return
	var outer: HTMLElement = label.get_element_by_id("outer")
	var mid: HTMLElement = label.get_element_by_id("mid")
	var inner: HTMLElement = label.get_element_by_id("inner")
	var order: Array[String]
	var capture: Array[String]
	var on_capture: Callable = func(event: HTMLEvent) -> void:
		capture.append(event.current_target.get_attribute("id"))
	var on_bubble: Callable = func(event: HTMLEvent) -> void:
		order.append("bubble:" + event.current_target.get_attribute("id"))
	var on_target: Callable = func(event: HTMLEvent) -> void:
		order.append("target:" + event.current_target.get_attribute("id"))
	_check(outer.add_event_listener(&"click", on_capture, {"capture": true}) == OK, "capture listeners register")
	_check(mid.add_event_listener(&"click", on_capture, {"capture": true}) == OK, "nested capture listeners register")
	_check(mid.add_event_listener(&"click", on_bubble) == OK, "bubble listeners register")
	_check(inner.add_event_listener(&"click", on_target) == OK, "target listeners register")
	_check(inner.add_event_listener(&"click", on_target) == OK, "duplicate registrations are ignored")
	_check(inner.has_event_listener(&"click"), "registered listeners are reported")

	var event: HTMLEvent = _event()
	var not_cancelled: bool = inner.dispatch_event(event)
	_check(not_cancelled, "dispatch reports an untouched default action")
	_check(capture == ["outer", "mid"], "capture runs from the document inward")
	_check(order == ["target:inner", "bubble:mid"], "the target runs once and bubbles upward")
	_check(event.target == inner and event.event_phase == HTMLEvent.PHASE_NONE,
		"the target is stable and the phase resets after dispatch")

	order.clear()
	var stop: Callable = func(stopped: HTMLEvent) -> void:
		order.append("stop")
		stopped.stop_propagation()
	mid.add_event_listener(&"click", stop)
	inner.dispatch_event(_event())
	_check(order == ["target:inner", "stop"], "stop_propagation ends propagation after the current node")
	mid.remove_event_listener(&"click", stop)

	order.clear()
	var cancel: Callable = func(cancelled: HTMLEvent) -> void:
		cancelled.prevent_default()
	inner.add_event_listener(&"click", cancel)
	_check(not inner.dispatch_event(_event()), "prevent_default is reported by dispatch")
	inner.remove_event_listener(&"click", cancel)

	var hits: Array[int]
	var once: Callable = func(_cancelled: HTMLEvent) -> void:
		hits.append(1)
	inner.add_event_listener(&"click", once, {"once": true})
	inner.dispatch_event(_event())
	inner.dispatch_event(_event())
	_check(hits.size() == 1, "once listeners run a single time")

	var clone: HTMLElement = inner.clone_node()
	_check(not clone.has_event_listener(&"click"), "clones do not copy listeners")

	mid.detach()
	dom.body.append_child(mid)
	order.clear()
	inner.dispatch_event(_event())
	_check(order.has("target:inner"), "detaching and reinserting keeps listeners")
	inner.remove()
	_check(not inner.is_valid(), "removing an element invalidates its handle")


func _check_delegation() -> void:
	label.text = """<div id="list"><button id="one" data-action="a" type="button">A</button><button id="two" data-action="b" type="button">B</button></div>"""
	label.size = Vector2(420.0, 200.0)
	await _settle()
	var dom: HTMLDocumentNode = label.get_document_node()
	if dom == null:
		_check(false, "the document node is available for delegation")
		return
	var actions: Array[String]
	var delegate: Callable = func(event: HTMLEvent) -> void:
		var target: HTMLElement = event.target
		var owner: HTMLElement = target.closest("[data-action]") if target != null else null
		if owner != null:
			actions.append(owner.get_attribute("data-action"))
	dom.add_event_listener(&"click", delegate)
	_click("one")
	_click("two")
	_check(actions == ["a", "b"], "one document listener serves every descendant")
	dom.remove_event_listener(&"click", delegate)


func _check_click_and_context_menu() -> void:
	label.text = """<p><a id="link" href="#target">Link</a></p><p id="target">Target</p><p id="plain">Plain</p>"""
	label.size = Vector2(420.0, 240.0)
	await _settle()
	var pressed: Array[String]
	var on_link: Callable = func(uri: String) -> void:
		pressed.append(uri)
	label.link_pressed.connect(on_link)

	_click("link")
	_check(pressed.size() == 1, "a plain click still reports link_pressed")

	pressed.clear()
	var veto: Callable = func(event: HTMLEvent) -> void:
		event.prevent_default()
	var link: HTMLElement = label.get_element_by_id("link")
	link.add_event_listener(&"click", veto)
	_click("link")
	_check(pressed.is_empty(), "cancelling click suppresses link_pressed")
	link.remove_event_listener(&"click", veto)

	var menu: Array[int]
	var on_menu: Callable = func(event: HTMLEvent) -> void:
		menu.append(1)
		event.prevent_default()
	var plain: HTMLElement = label.get_element_by_id("plain")
	plain.add_event_listener(&"contextmenu", on_menu)
	var right: InputEventMouseButton = InputEventMouseButton.new()
	right.button_index = MOUSE_BUTTON_RIGHT
	right.pressed = true
	right.position = plain.get_bounding_rect().get_center()
	get_viewport().push_input(right, true)
	_check(menu.size() == 1, "a right click dispatches contextmenu")
	plain.remove_event_listener(&"contextmenu", on_menu)
	label.link_pressed.disconnect(on_link)


func _check_keyboard() -> void:
	label.text = """<input id="a"><input id="b">"""
	label.size = Vector2(420.0, 200.0)
	await _settle()
	var first: HTMLElement = label.get_element_by_id("a")
	var second: HTMLElement = label.get_element_by_id("b")
	label.focus_element(first)
	_check(label.get_focused_element() == first, "the first field takes focus")
	var hits: Array[int]
	var veto: Callable = func(event: HTMLEvent) -> void:
		hits.append(1)
		event.prevent_default()
	first.add_event_listener(&"keydown", veto)
	_key(KEY_TAB, true)
	_key(KEY_TAB, false)
	_check(hits.size() >= 1, "keydown reaches the focused element")
	_check(label.get_focused_element() == first, "cancelling keydown stops Tab traversal")
	first.remove_event_listener(&"keydown", veto)
	_key(KEY_TAB, true)
	_key(KEY_TAB, false)
	_check(label.get_focused_element() == second, "Tab still traverses without a listener")


func _check_form() -> void:
	label.text = """<form id="form"><input id="name" name="name" value="x"><button id="send" type="button">Send</button></form>"""
	label.size = Vector2(420.0, 220.0)
	await _settle()
	var submitted: Array[int]
	var on_submit: Callable = func(_form: HTMLFormElement, _entries: Array[Dictionary], _submitter: HTMLElement) -> void:
		submitted.append(1)
	label.form_submitted.connect(on_submit)
	var form: HTMLFormElement = label.get_element_by_id("form")
	_check(label.submit_form(form) == OK and submitted.size() == 1, "a plain submission reaches form_submitted")

	submitted.clear()
	var veto: Callable = func(event: HTMLEvent) -> void:
		event.prevent_default()
	form.add_event_listener(&"submit", veto)
	_check(label.submit_form(form) == OK, "a cancelled submission is not an error")
	_check(submitted.is_empty(), "cancelling submit suppresses form_submitted")
	form.remove_event_listener(&"submit", veto)
	label.form_submitted.disconnect(on_submit)


func _event() -> HTMLEvent:
	var event: HTMLEvent = HTMLEvent.new()
	event.type = &"click"
	event.bubbles = true
	event.cancelable = true
	return event


func _click(id: String) -> void:
	var element: HTMLElement = label.get_element_by_id(id)
	var point: Vector2 = element.get_bounding_rect().get_center()
	for state: bool in [true, false]:
		var click: InputEventMouseButton = InputEventMouseButton.new()
		click.button_index = MOUSE_BUTTON_LEFT
		click.pressed = state
		click.position = point
		get_viewport().push_input(click, true)


func _key(code: Key, pressed: bool) -> void:
	var key: InputEventKey = InputEventKey.new()
	key.keycode = code
	key.pressed = pressed
	get_viewport().push_input(key, true)


func _settle() -> void:
	for _frame: int in range(8):
		await get_tree().process_frame


func _check(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error(description)
