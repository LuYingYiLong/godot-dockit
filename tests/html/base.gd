extends Node

var failures: int
@onready var frame: SubViewport = $Frame
@onready var label: HTMLLabel = $Frame/HTMLLabel


func _ready() -> void:
	await _settle()
	var from_pck: bool = "--pck" in OS.get_cmdline_user_args()
	if from_pck:
		_check(not FileAccess.file_exists("res://pages/index.html"), "export loads imported HTML from its resource remap")
		_check(not FileAccess.file_exists("res://assets/v1/styles/outer.css"), "raw CSS is excluded from the selected-scene export")
		_check(not ResourceLoader.exists("res://assets/v2/icon.svg"), "unused assets are outside the scene's export dependencies")
	var layout: HTMLLayout = label.document.create_layout()
	_check(layout.get_document_element() is HTMLHtmlElement and layout.get_head() is HTMLHeadElement
		and layout.get_body() is HTMLBodyElement, "document accessors return the three structural types")
	var title: HTMLTitleElement = layout.query_selector("title") as HTMLTitleElement
	_check(title.text == "  First  title  " and layout.get_title() == "First title", "title text preserves original whitespace")
	title.text = "  New\t title "
	_check(layout.get_title() == "New title", "changing typed title updates the document title")
	var meta: HTMLMetaElement = label.get_element_by_id("meta") as HTMLMetaElement
	_check(meta.name == "viewport" and meta.content == "test", "meta reflects name and content")
	meta.http_equiv = "refresh"
	meta.charset = "utf-8"
	meta.media = "screen"
	_check(meta.get_attribute("http-equiv") == "refresh" and meta.get_attribute("charset") == "utf-8"
		and meta.get_attribute("media") == "screen", "metadata setters reflect through DOM attributes")
	var script: HTMLScriptElement = label.get_element_by_id("config") as HTMLScriptElement
	_check(script.type == "application/json" and JSON.parse_string(script.text) == {"enabled": true},
		"script keeps the inline data available to GDScript")
	script.text = "{\"tag\":\"<p>\"}"
	script.src = "data.js"
	script.async = true
	script.defer = true
	script.no_module = true
	_check(script.text.contains("<p>") and script.get_attribute("src") == "data.js" and script.has_attribute("async")
		and script.has_attribute("defer") and script.has_attribute("nomodule"), "script setters preserve raw text and reflect attributes")
	await _settle()

	var first: HTMLBaseElement = label.get_element_by_id("first") as HTMLBaseElement
	var second: HTMLBaseElement = label.get_element_by_id("next") as HTMLBaseElement
	var icon: HTMLImageElement = label.get_element_by_id("icon") as HTMLImageElement
	var linked: HTMLElement = label.get_element_by_id("linked")
	var jump: HTMLAnchorElement = label.get_element_by_id("jump") as HTMLAnchorElement
	var tone: HTMLAudioElement = label.get_element_by_id("tone") as HTMLAudioElement
	_check(first.href == "../assets/v1/virtual.html" and first.get_resolved_href() == "res://assets/v1/virtual.html"
		and second.get_resolved_href() == "res://assets/v2/" and label.get_base_uri() == "res://assets/v1/virtual.html"
		and layout.get_document_node().get_base_uri() == label.get_base_uri(),
		"the first connected base with an href wins, even after a target-only base and template")
	_check(icon.current_src == "res://assets/v1/icon.svg" and icon.get_intrinsic_size().x == 14.0, "relative srcset resolves through the base")
	_check(tone.play() == OK and tone.current_src == "res://assets/v1/tone.wav" and is_equal_approx(tone.duration, 0.5),
		"relative audio plays through the base")
	tone.pause()
	_check(is_equal_approx(linked.get_bounding_rect().size.x, 40.0), "the external stylesheet loaded through the base")
	_check_pixel("icon", Color.RED)
	_check_pixel("inline", Color.GREEN)
	_check_pixel("linked", Color.BLUE)
	_check_pixel("nested", Color.MAGENTA)
	_check_pixel("inline-import", Color.CYAN)
	_check(jump.href == "#tail" and jump.get_resolved_href() == "res://assets/v1/virtual.html#tail", "script can read the resolved link")
	var clicked_uris: Array[String]
	label.link_pressed.connect(func(uri: String) -> void: clicked_uris.append(uri), CONNECT_ONE_SHOT)
	_check(label.activate_element(jump) and clicked_uris == ["#tail"] and label.get_scroll_offset().y > 0,
		"existing local anchors and link_pressed retain the original href")
	if not from_pck:
		await _check_base_edits(layout, first, second, icon, linked, tone)
	else:
		first.href = ""
		_check(label.get_base_uri() == "res://pages/index.html", "export retains the original source URL for an empty base")
	if failures == 0:
		print("HTML base export checks passed" if from_pck else "HTML base checks passed")
	get_tree().quit(0 if failures == 0 else 1)


func _check_base_edits(
	layout: HTMLLayout, first: HTMLBaseElement, second: HTMLBaseElement, icon: HTMLImageElement,
	linked: HTMLElement, tone: HTMLAudioElement,
) -> void:
	var detached: HTMLBaseElement = layout.create_element("base") as HTMLBaseElement
	detached.href = "res://unused/"
	_check(layout.get_base_uri() == "res://assets/v1/virtual.html", "a detached base does not affect the document")
	first.href = "../assets/v2/"
	_check(label.get_base_uri() == "res://assets/v2/", "base mutations update URL queries immediately")
	await _settle()
	_check(icon.current_src == "res://assets/v2/icon.svg" and icon.get_intrinsic_size().x == 36.0
		and is_equal_approx(linked.get_bounding_rect().size.x, 70.0), "base mutation reselects images and loads the new stylesheet")
	_check(tone.play() == OK and tone.current_src == "res://assets/v2/tone.wav" and is_equal_approx(tone.duration, 1.0),
		"audio resolves again when the document base changes")
	tone.pause()
	first.href = "https://example.invalid/"
	await _settle()
	_check(label.get_base_uri() == "res://pages/index.html" and icon.get_intrinsic_size().x == 24.0,
		"blocked first base falls back to the source file without picking a later base")
	_check(first.detach() == OK and second.is_connected() and label.get_base_uri() == "res://assets/v2/",
		"detaching the first base promotes the next")
	second.href = ""
	_check(label.get_base_uri() == "res://pages/index.html", "an empty href resolves to the source file")
	second.href = "user://skin/"
	_check(label.get_base_uri() == "user://skin/", "an explicit user resource directory is accepted")
	second.href = "../../../../escape/"
	_check(label.get_base_uri() == "res://pages/index.html", "the resolver rejects paths above the Godot scheme root")
	_check(layout.parse("<base href='assets/'><img src='icon.svg'>", "res://pages") == OK
		and layout.get_base_uri() == "res://pages/assets/", "inline layouts resolve from the supplied directory")
	_check(layout.parse("<svg><base href='ignored/'></base><title>SVG</title></svg><math><base href='ignored/'/></math>",
		"res://pages") == OK and layout.get_base_uri() == "res://pages/", "foreign base elements cannot change the HTML resource base")
	_check(not (layout.query_selector("svg title") is HTMLTitleElement) and layout.get_title().is_empty(),
		"foreign titles keep their generic handles and stay out of the document title")
	var svg: HTMLElement = layout.query_selector("svg")
	_check(svg.set_inner_html("<base href='ignored/'/>") == OK and layout.get_base_uri() == "res://pages/",
		"base elements inserted into a foreign fragment stay out of the resource context")


func _check_pixel(id: String, expected: Color) -> void:
	var element: HTMLElement = label.get_element_by_id(id)
	var sample: Vector2i = Vector2i(element.get_bounding_rect().position) + Vector2i(4, 4)
	var actual: Color = frame.get_texture().get_image().get_pixelv(sample)
	_check(actual.is_equal_approx(expected), "%s uses the baked relative resource (%s)" % [id, actual])


func _settle() -> void:
	for _step: int in range(4):
		await get_tree().process_frame
	await RenderingServer.frame_post_draw


func _check(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error(description)
