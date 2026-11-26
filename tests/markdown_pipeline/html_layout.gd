extends SceneTree


var _failures: Array[String]
var _snapshots: Array[Dictionary]
var _reference: bool = OS.get_environment("DOCKIT_HTML_LAYOUT_REFERENCE") == "1"


func _initialize() -> void:
	create_timer(20.0).timeout.connect(_timed_out)
	_run.call_deferred()


func _timed_out() -> void:
	push_error("HTML layout regression timed out")
	quit(1)


func _check(p_passed: bool, p_message: String) -> void:
	if not p_passed:
		_failures.append(p_message)
		push_error(p_message)


func _serve_images(p_server: TCPServer, p_images: Array[PackedByteArray]) -> void:
	for png: PackedByteArray in p_images:
		while not p_server.is_connection_available():
			await process_frame
		var peer: StreamPeerTCP = p_server.take_connection()
		while peer.get_available_bytes() == 0:
			peer.poll()
			await process_frame
		peer.get_data(peer.get_available_bytes())
		var response: PackedByteArray = ("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: %d\r\nConnection: close\r\n\r\n" % png.size()).to_utf8_buffer()
		response.append_array(png)
		peer.put_data(response)
		peer.disconnect_from_host()
	p_server.stop()


func _wait_for_image(p_image: HTMLImageElement, p_size: Vector2) -> void:
	for frame: int in range(120):
		await process_frame
		if p_image.get_intrinsic_size().is_equal_approx(p_size):
			return
	_check(false, "Asynchronous image dimensions did not update")


func _make_layout(p_html: String, p_size: Vector2i) -> HTMLLayout:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.set_layout_statistics_enabled(true)
	_check(layout.parse(p_html) == OK, "HTML parse failed")
	_check(layout.layout(p_size) == OK, "HTML layout failed")
	return layout


func _compare(p_layout: HTMLLayout, p_html: String, p_size: Vector2i, p_ids: PackedStringArray) -> void:
	var reference: HTMLLayout = _make_layout(p_html, p_size)
	_check(p_layout.get_content_size() == reference.get_content_size(), "Fresh layout content size differs")
	_check(p_layout.get_plain_text() == reference.get_plain_text(), "Fresh layout text differs")
	_check(p_layout.get_accessible_text() == reference.get_accessible_text(), "Fresh accessible text differs")
	for element_id: String in p_ids:
		var actual: HTMLElement = p_layout.get_element_by_id(element_id)
		var expected: HTMLElement = reference.get_element_by_id(element_id)
		_check(actual.get_bounding_rect().is_equal_approx(expected.get_bounding_rect()), "Fresh element geometry differs: " + element_id)
		var point: Vector2 = expected.get_bounding_rect().get_center()
		var hit: HTMLElement = p_layout.get_element_at(point)
		var reference_hit: HTMLElement = reference.get_element_at(point)
		_check((hit == null) == (reference_hit == null), "Fresh hit availability differs")
		if hit != null and reference_hit != null:
			_check(hit.get_attribute("id") == reference_hit.get_attribute("id"), "Fresh hit target differs")
	var rectangles: Array[Rect2]
	for element_id: String in p_ids:
		rectangles.append(p_layout.get_element_by_id(element_id).get_bounding_rect())
	_snapshots.append({"size": p_layout.get_content_size(), "text": p_layout.get_plain_text(), "rectangles": rectangles,})
	var count: int = p_layout.get_plain_text().length()
	_check(p_layout.get_selection_rects(0, count) == reference.get_selection_rects(0, count), "Fresh selection geometry differs")


func _run() -> void:
	get_root().size = Vector2i(800, 600)
	var viewport_size: Vector2i = Vector2i(320, 180)
	var source: String = "<style>body{margin:0}.row{display:flex}.cell{width:max-content;max-width:160px}table{border-collapse:collapse}</style><div class='row'><div id='a' class='cell'>中文 alpha beta</div><div id='b'>Right</div></div><table><tr><td>One</td><td id='c'>Two</td></tr></table>"
	var layout: HTMLLayout = _make_layout(source, viewport_size)
	layout.reset_layout_statistics()
	for index: int in range(12):
		_check(layout.layout(viewport_size) == OK, "Repeated layout failed")
	var stats: Dictionary = layout.get_layout_statistics()
	_check(stats.layout_count == 0 and stats.style_count == 0 and stats.geometry_count == 0 and stats.paint_count == 0, "Unchanged layout performed work")
	layout.get_element_by_id("b").set_text_content("Changed content wraps across the available width")
	_check(layout.layout(viewport_size) == OK, "Mutated layout failed")
	_compare(layout, source.replace("Right", "Changed content wraps across the available width"), viewport_size, ["a", "b", "c",])
	layout.set_layout_statistics_enabled(false)
	stats = layout.get_layout_statistics()
	layout.layout(Vector2i(300, 180))
	_check(layout.get_layout_statistics() == stats, "Disabled statistics changed counters")
	layout.reset_layout_statistics()
	_check(layout.get_layout_statistics().layout_count == 0, "Reset did not clear counters")
	layout.set_layout_statistics_enabled(true)
	layout.clear()
	_check(layout.is_layout_statistics_enabled(), "Clear disabled statistics")
	_check(layout.layout(viewport_size) != OK, "Layout before parse succeeded")

	var fixtures: PackedStringArray = [
		"<style>body{margin:0}#a{width:50vw;height:20vh}#b{display:grid;grid-template-columns:min-content 1fr}</style><div id='a'>Viewport</div><div id='b'><span>Grid min content</span><span>Cell</span></div>",
		"<style>#a{container-type:inline-size;width:220px}@container(min-width:200px){#b{font-size:28px}}</style><div id='a'><div id='b'>Container query</div></div>",
		"<style>#a{display:grid;grid-template-columns:1fr 1fr}#b{display:flex}</style><div id='a'><span>Grid</span><div id='b'>Flex content</div></div>",
		"<style>#a{height:60px;overflow:auto}#b{position:sticky;top:0;transform:translateX(3px)}</style><div id='a'><div id='b'>Sticky</div><div style='height:300px'>Scroll</div></div>",
	]
	for fixture: String in fixtures:
		var resized: HTMLLayout = _make_layout(fixture, viewport_size)
		resized.layout(Vector2i(260, 160))
		_compare(resized, fixture, Vector2i(260, 160), ["a", "b",])
		resized.layout(viewport_size)
		_compare(resized, fixture, viewport_size, ["a", "b",])

	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport_size)
	label.set_layout_statistics_enabled(true)
	get_root().add_child(label)
	label.text = "<style>body{margin:0}#measure{width:max-content}#hover:hover{color:red}</style><div id='measure'>Intrinsic measurement</div><div id='hover'>Hover text</div><div id='tall' style='height:900px'>Tail</div>"
	await process_frame
	await process_frame
	stats = label.get_layout_statistics()
	print("HTML initial overflow statistics: ", stats)
	_check(stats.layout_count <= 2, "Initial automatic scrollbar used an extra base layout")
	if not _reference:
		_check(stats.measurement_cache_hits_count > 0, "Scrollbar convergence did not reuse intrinsic measurement")
	label.reset_layout_statistics()
	for index: int in range(8):
		label.notification(Control.NOTIFICATION_RESIZED)
	await process_frame
	await process_frame
	stats = label.get_layout_statistics()
	print("HTML repeated refresh statistics: ", stats)
	if not _reference:
		_check(stats.layout_count == 0 and stats.style_count == 0 and stats.geometry_count == 0, "Stable host refresh did not reuse its final viewport")
	var overflow_width: float = label.get_element_by_id("tall").get_bounding_rect().size.x
	label.get_element_by_id("tall").set_style_property("height", "20px")
	await process_frame
	await process_frame
	var no_overflow_width: float = label.get_element_by_id("tall").get_bounding_rect().size.x
	_check(no_overflow_width > overflow_width, "Scrollbar stayed after shrinking content")
	label.select_all()
	var selected: String = label.get_selected_text()
	var exit_motion: InputEventMouseMotion = InputEventMouseMotion.new()
	exit_motion.position = Vector2(700.0, 500.0)
	exit_motion.global_position = exit_motion.position
	get_root().push_input(exit_motion, true)
	await process_frame
	label.reset_layout_statistics()
	var motion: InputEventMouseMotion = InputEventMouseMotion.new()
	motion.position = label.get_element_by_id("hover").get_bounding_rect().get_center()
	motion.global_position = motion.position
	motion.relative = Vector2(1.0, 1.0)
	get_root().push_input(motion, true)
	await process_frame
	await process_frame
	RenderingServer.force_draw()
	stats = label.get_layout_statistics()
	_check(label.get_computed_style_property(label.get_element_by_id("hover"), "color") == "rgb(255, 0, 0)", "Color hover did not run")
	_check(stats.paint_count > 0, "Hover did not repaint")
	_check(stats.layout_count == 0 and stats.geometry_count == 0, "Color hover caused layout or geometry rebuild")
	if not _reference:
		_check(stats.text_index_count == 0 and stats.anchors_count == 0, "Color hover rebuilt text or anchors")
	_check(label.get_selected_text() == selected, "Hover changed selection text")
	print("HTML paint-only statistics: ", stats)
	label.get_element_by_id("hover").set_style_property("font-size", "24px")
	await process_frame
	await process_frame
	_check(label.get_layout_statistics().layout_count > 0, "Font change did not invalidate layout")
	label.text = "<style>body{margin:0}</style><input id='range' type='range' min='0' max='100' value='20'><dialog id='dialog'><button id='confirm'>Confirm</button></dialog>"
	await process_frame
	await process_frame
	var range_input: HTMLInputElement = label.get_element_by_id("range") as HTMLInputElement
	var original_input_rect: Rect2 = range_input.get_bounding_rect()
	for value: int in [35, 60, 85,]:
		range_input.value = str(value)
		await process_frame
		_check(range_input.value == str(value), "Range state was lost")
		_check(range_input.get_bounding_rect().is_equal_approx(original_input_rect), "Range value changed layout bounds")
	var dialog: HTMLDialogElement = label.get_element_by_id("dialog") as HTMLDialogElement
	_check(dialog.show_modal() == OK, "Dialog did not open")
	await process_frame
	await process_frame
	var confirm: HTMLElement = label.get_element_by_id("confirm")
	_check(confirm.get_bounding_rect().has_area(), "Dialog child has no geometry")
	_check(label.get_element_at(confirm.get_bounding_rect().get_center()) == confirm, "Dialog hit target differs")
	_check(dialog.close() == OK, "Dialog did not close")
	await process_frame
	await process_frame
	_check(not dialog.is_modal(), "Closed dialog remained in top layer")
	label.text = "<style>@keyframes expand{from{width:30px}to{width:70px}}#animated{height:20px;animation:expand 0.03s forwards}</style><div id='animated'>Animation</div>"
	await create_timer(0.15).timeout
	_check(is_equal_approx(label.get_element_by_id("animated").get_bounding_rect().size.x, 70.0), "Animation did not reach its final layout")
	var server: TCPServer = TCPServer.new()
	var listening: Error = server.listen(0, "127.0.0.1")
	_check(listening == OK, "Local image server did not start")
	if listening == OK:
		var small_image: Image = Image.create(40, 24, false, Image.FORMAT_RGBA8)
		small_image.fill(Color.GREEN)
		var large_image: Image = Image.create(80, 32, false, Image.FORMAT_RGBA8)
		large_image.fill(Color.BLUE)
		_serve_images(server, [small_image.save_png_to_buffer(), large_image.save_png_to_buffer(),])
		var origin: String = "http://127.0.0.1:%d/" % server.get_local_port()
		label.text = "<style>body{margin:0}</style><img id='remote' src='%ssmall.png'>" % origin
		await process_frame
		var remote_image: HTMLImageElement = label.get_element_by_id("remote") as HTMLImageElement
		await _wait_for_image(remote_image, Vector2(40.0, 24.0))
		remote_image.src = origin + "large.png"
		await _wait_for_image(remote_image, Vector2(80.0, 32.0))
		_check(remote_image.get_bounding_rect().size.is_equal_approx(Vector2(80.0, 32.0)), "New image dimensions did not invalidate layout")
	label.queue_free()
	await process_frame
	var shadow_layout: HTMLLayout = _make_layout("<div id='host'><span id='assigned'>Assigned text</span></div>", viewport_size)
	var shadow_root: HTMLShadowRoot = shadow_layout.get_element_by_id("host").attach_shadow()
	_check(shadow_root != null, "Shadow root attachment failed")
	if shadow_root != null:
		shadow_root.replace_children([shadow_layout.parse_fragment("<div><slot>Fallback text</slot></div>"),])
		shadow_layout.layout(viewport_size)
		_check(shadow_layout.get_plain_text().contains("Assigned text"), "Slot lost assigned text")
		shadow_layout.get_element_by_id("host").set_inner_html("")
		shadow_layout.layout(viewport_size)
		_check(shadow_layout.get_plain_text().contains("Fallback text"), "Slot fallback did not invalidate layout")
	var reserved: HTMLLabel = HTMLLabel.new()
	reserved.size = Vector2(viewport_size)
	reserved.set_layout_statistics_enabled(true)
	get_root().add_child(reserved)
	reserved.text = "<style>html{overflow-y:scroll;scrollbar-gutter:stable}body{margin:0}</style><p>Reserved gutter</p>"
	await process_frame
	await process_frame
	print("HTML reserved gutter statistics: ", reserved.get_layout_statistics())
	if not _reference:
		_check(reserved.get_layout_statistics().layout_count == 1, "Reserved gutter performed an unnecessary base layout")
	reserved.queue_free()
	var printable: HTMLDocument = HTMLDocument.new()
	printable.text = "<style>@media print {p{font-size:24px}} @page {size:300px 200px;margin:10px}</style><p>Print layout</p>"
	var screen_layout: HTMLLayout = printable.create_layout()
	screen_layout.layout(viewport_size)
	var screen_size: Vector2i = screen_layout.get_content_size()
	var print_layout: HTMLPrintLayout = printable.create_print_layout()
	_check(print_layout.get_error() == OK and print_layout.get_page_count() > 0, "Print layout failed")
	_check(screen_layout.get_content_size() == screen_size, "Print layout changed screen geometry")
	await process_frame
	print("HTML layout signature: ", JSON.stringify(_snapshots))
	print("HTML layout regression: %d failures" % _failures.size())
	quit(0 if _failures.is_empty() else 1)
