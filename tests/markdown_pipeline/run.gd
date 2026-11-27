extends SceneTree


var _failures: Array[String]


func _initialize() -> void:
	_run.call_deferred()


func _check(p_passed: bool, p_message: String) -> void:
	if not p_passed:
		_failures.append(p_message)
		push_error(p_message)


func _run() -> void:
	var flat: StyleBoxFlat = StyleBoxFlat.new()
	flat.bg_color = Color(0.2, 0.6, 0.3, 0.8)
	flat.border_color = Color.RED
	flat.set_border_width_all(3)
	flat.set_content_margin_all(10.0)
	flat.set_corner_radius_all(8)
	var converted: Dictionary = StyleBoxCSSHelper.stylebox_to_css(flat)
	_check(converted.error == OK, "Flat CSS conversion failed")
	var restored: Dictionary = StyleBoxCSSHelper.css_to_stylebox(converted.css)
	_check(restored.error == OK, "Flat restoration failed")
	if restored.error == OK:
		var roundtrip: StyleBoxFlat = restored.stylebox
		_check((roundtrip.bg_color - flat.bg_color).r ** 2 + (roundtrip.bg_color - flat.bg_color).g ** 2 + (roundtrip.bg_color - flat.bg_color).b ** 2 < 0.0001, "Flat background changed")
		_check(roundtrip.get_border_width(SIDE_LEFT) == 3, "Flat border width changed")
		_check(is_equal_approx(roundtrip.get_content_margin(SIDE_LEFT), 10.0), "Content margins double-counted the border")

	var empty: Dictionary = StyleBoxCSSHelper.css_to_stylebox("padding: 6px;", {}, &"empty")
	_check(empty.error == OK and empty.stylebox is StyleBoxEmpty, "Empty StyleBox conversion failed")
	var line: Dictionary = StyleBoxCSSHelper.css_to_stylebox("height: 4px; background-color: blue;", {}, &"line")
	_check(line.error == OK and line.stylebox is StyleBoxLine and line.stylebox.thickness == 4, "Line StyleBox conversion failed")
	var relative: Dictionary = StyleBoxCSSHelper.css_to_stylebox("padding: 10%;")
	_check(relative.error != OK and not relative.diagnostics.is_empty(), "Unresolved relative sizes were silently guessed")
	flat.bg_color = Color.BLUE
	converted = StyleBoxCSSHelper.stylebox_to_css(flat)
	_check(converted.css.contains("#0000ff"), "StyleBox changed did not invalidate conversion cache")

	var image: Image = Image.create(12, 12, false, Image.FORMAT_RGBA8)
	image.fill(Color.GREEN)
	var nine_patch: StyleBoxTexture = StyleBoxTexture.new()
	nine_patch.texture = ImageTexture.create_from_image(image)
	nine_patch.set_texture_margin_all(3.0)
	nine_patch.axis_stretch_horizontal = StyleBoxTexture.AXIS_STRETCH_MODE_TILE_FIT
	converted = StyleBoxCSSHelper.stylebox_to_css(nine_patch)
	restored = StyleBoxCSSHelper.css_to_stylebox(converted.css, {"textures": converted.textures,})
	_check(restored.error == OK, "Nine-patch restoration failed")
	_check(restored.get("stylebox") is StyleBoxTexture, "Auto did not select a texture StyleBox")
	if restored.error == OK and restored.stylebox is StyleBoxTexture:
		var texture_style: StyleBoxTexture = restored.stylebox
		_check(is_equal_approx(texture_style.get_texture_margin(SIDE_TOP), 3.0), "Nine-patch slice changed")
		_check(texture_style.axis_stretch_horizontal == StyleBoxTexture.AXIS_STRETCH_MODE_TILE_FIT, "Nine-patch repeat changed")

	var label: MarkdownLabel = MarkdownLabel.new()
	label.size = Vector2(640.0, 500.0)
	label.parse_html = true
	label.parse_latex = true
	get_root().add_child(label)
	var emitted_link_uris: Array[String] = []
	label.link_pressed.connect(func(uri: String) -> void: emitted_link_uris.append(uri))
	label.activate_link_uri("dockit-test://document")
	_check(emitted_link_uris == ["dockit-test://document"], "Link activation did not emit link_pressed")
	label.text = "# Heading\n\n| One | Two |\n| -- | -- |\n| <div style=\"height:90px;padding:12px;background:red\">**Strong** <a href=\"https://example.com\">Link</a></div> | Text |\n\n~~~gdscript\nfunc greet():\n\tprint(\"Hello\")\n~~~\n\n$E=mc^2$"
	await process_frame
	await process_frame
	label.select_all()
	var selected: String = label.get_selected_text()
	_check(selected.contains("Heading") and selected.contains("Strong") and selected.contains("Link"), "Mixed table text is missing")
	_check(not selected.contains("<div") and not selected.contains("**"), "Source markup leaked into copied text")
	_check(selected.contains("E=mc^2"), "Formula source is missing from copied text")
	_check(selected.contains("\tprint"), "Code tab indentation was lost")
	var found: Vector2i = label.search("Strong", 0, 0, 0)
	_check(found.x >= 0, "Search did not find text inside a table HTML container")
	var found_link: bool
	for row: int in range(0, 400, 8):
		for column: int in range(0, 640, 8):
			if label.get_link_uri_at_point(Vector2(column, row)) == "https://example.com":
				found_link = true
				break
		if found_link:
			break
	_check(found_link, "Link hit testing did not find the table link")
	var before_height: float = label.get_content_height()
	_check(before_height > 150.0, "HTML block height was ignored")
	var sheet: CSSStyleSheet = CSSStyleSheet.new()
	sheet.css_text = "td div {min-height:190px;}"
	label.stylesheet = sheet
	await process_frame
	_check(label.get_content_height() > before_height + 70.0, "Author CSS did not affect the Markdown table")
	label.clear_selection()
	label.streaming_enabled = true
	_check(label.deferred_layout_enabled, "Streaming updates were not coalesced by default")
	label.text = "# Stream\n\n"
	for character: String in "**Bold** and <span style=\"color:red\">HTML</span>\n\n|One|\n|--|\n|Cell|":
		label.append_text(character)
	var batch_lines: PackedStringArray = []
	for line_index: int in range(48):
		batch_lines.append("Batch line %d" % line_index)
	label.append_text("\n\n" + "\n\n".join(batch_lines))
	_check(label.flush_pending_layout(), "Batched stream update was not pending")
	_check(not label.flush_pending_layout(), "Batched stream update was flushed more than once")
	label.finish_stream()
	await process_frame
	label.select_all()
	selected = label.get_selected_text()
	_check(selected.contains("Bold") and selected.contains("HTML") and selected.contains("Cell") and selected.contains("Batch line 47"), "Streaming lost content from a coalesced append batch")
	var snapshot: MarkdownLabel = MarkdownLabel.new()
	snapshot.size = label.size
	snapshot.parse_html = true
	get_root().add_child(snapshot)
	snapshot.text = label.text
	await process_frame
	snapshot.select_all()
	_check(selected == snapshot.get_selected_text(), "Streaming output differs from parsing the same source at once")
	var paragraph_stream: MarkdownLabel = MarkdownLabel.new()
	paragraph_stream.size = Vector2(640.0, 500.0)
	paragraph_stream.streaming_enabled = true
	get_root().add_child(paragraph_stream)
	var paragraph_lines: PackedStringArray = []
	for line_index: int in range(40):
		paragraph_lines.append("Paragraph continuation %d" % line_index)
	paragraph_stream.text = "\n".join(paragraph_lines)
	await process_frame
	await process_frame
	paragraph_stream.append_text("\nfinal continuation")
	_check(paragraph_stream.flush_pending_layout(), "Long paragraph stream update was not pending")
	await process_frame
	await process_frame
	paragraph_stream.select_all()
	var streamed_paragraph: String = paragraph_stream.get_selected_text()
	var paragraph_snapshot: MarkdownLabel = MarkdownLabel.new()
	paragraph_snapshot.size = paragraph_stream.size
	get_root().add_child(paragraph_snapshot)
	paragraph_snapshot.text = paragraph_stream.text
	await process_frame
	paragraph_snapshot.select_all()
	_check(streamed_paragraph == paragraph_snapshot.get_selected_text(), "Streaming split a long continuing paragraph into separate blocks")
	paragraph_stream.queue_free()
	paragraph_snapshot.queue_free()
	snapshot.text = "<style>.sample {height:30px;}</style>\n\n<div class=\"sample\">CSS order</div>"
	sheet = CSSStyleSheet.new()
	sheet.css_text = ".sample {height:150px;}"
	snapshot.stylesheet = sheet
	await process_frame
	_check(snapshot.get_content_height() >= 150.0, "Assigned stylesheet did not follow document styles")
	var map_label: MarkdownLabel = MarkdownLabel.new()
	map_label.size = Vector2(260.0, 600.0)
	map_label.parse_html = true
	map_label.parse_latex = true
	map_label.parse_mermaid = true
	get_root().add_child(map_label)
	map_label.text = "# Mapping[^note]\n\n" + "A long source line that wraps in the preview and still needs to map by its actual rendered characters. ".repeat(4) + "\n\n| First | Second |\n| --- | --- |\n| table text | more text |\n\n~~~gdscript\nfunc greet():\n\tprint(\"Hello\")\n~~~\n\n<div style=\"height:72px;padding:8px\">Raw HTML text</div>\n\n$$\nx^2 + y^2\n$$\n\n[^note]: Footnote content mapped to its source line"
	await process_frame
	await process_frame
	var source_line_count: int = map_label.text.split("\n", true).size()
	var previous_source_y: float = -1.0
	for source_line: int in range(source_line_count):
		var source_y: float = map_label.get_scroll_offset_for_source_line(float(source_line), source_line_count)
		_check(source_y + 0.01 >= previous_source_y, "Source-to-preview mapping moved backwards at line %d" % source_line)
		_check(source_y >= 0.0 and source_y <= map_label.get_content_height() + 1.0, "Source line mapped outside preview bounds")
		previous_source_y = source_y
	var source_lines: PackedStringArray = map_label.text.split("\n", true)
	var wrapped_line: int = source_lines.find("A long source line that wraps in the preview and still needs to map by its actual rendered characters. ".repeat(4))
	var wrapped_start_y: float = map_label.get_scroll_offset_for_source_line(float(wrapped_line) + 0.05, source_line_count)
	var wrapped_middle_y: float = map_label.get_scroll_offset_for_source_line(float(wrapped_line) + 0.5, source_line_count)
	var wrapped_end_y: float = map_label.get_scroll_offset_for_source_line(float(wrapped_line) + 0.95, source_line_count)
	_check(wrapped_start_y < wrapped_end_y and wrapped_start_y <= wrapped_middle_y and wrapped_middle_y <= wrapped_end_y, "Wrapped source line did not follow rendered character geometry")
	var wrapped_roundtrip: float = map_label.get_source_line_at_scroll_offset(wrapped_middle_y, source_line_count)
	_check(absf(wrapped_roundtrip - wrapped_line) <= 1.0, "Wrapped line did not round-trip through preview geometry")
	var table_source_line: int = source_lines.find("| table text | more text |")
	var footnote_source_line: int = source_lines.find("[^note]: Footnote content mapped to its source line")
	var table_source_y: float = map_label.get_scroll_offset_for_source_line(float(table_source_line), source_line_count)
	var footnote_source_y: float = map_label.get_scroll_offset_for_source_line(float(footnote_source_line), source_line_count)
	_check(table_source_y > wrapped_end_y, "Table row did not map after the wrapped paragraph")
	_check(absf(map_label.get_source_line_at_scroll_offset(table_source_y, source_line_count) - table_source_line) <= 1.0, "Table row did not round-trip through preview geometry")
	_check(footnote_source_y > table_source_y, "Footnote source did not map to its rendered location")
	var previous_preview_line: float = -1.0
	for preview_y: int in range(0, int(map_label.get_content_height()), 12):
		var preview_line: float = map_label.get_source_line_at_scroll_offset(float(preview_y), source_line_count)
		_check(preview_line + 0.05 >= previous_preview_line, "Preview-to-source mapping moved backwards at y=%d: %.2f after %.2f" % [preview_y, preview_line, previous_preview_line])
		_check(preview_line >= 0.0 and preview_line <= source_line_count, "Preview offset mapped outside source bounds")
		previous_preview_line = preview_line
	map_label.streaming_enabled = true
	var first_line_y: float = map_label.get_scroll_offset_for_source_line(0.0, source_line_count)
	map_label.append_text("\n\n# Streamed tail\n\nTail mapping text")
	_check(map_label.flush_pending_layout(), "Source map stream update was not pending")
	await process_frame
	await process_frame
	var extended_line_count: int = map_label.text.split("\n", true).size()
	_check(is_equal_approx(map_label.get_scroll_offset_for_source_line(0.0, extended_line_count), first_line_y), "Appending a tail changed the cached source map for the prefix")
	var streamed_tail_line: int = map_label.text.split("\n", true).find("# Streamed tail")
	var streamed_tail_y: float = map_label.get_scroll_offset_for_source_line(float(streamed_tail_line), extended_line_count)
	_check(streamed_tail_y > table_source_y, "Incremental source map did not include the appended tail")
	map_label.select_all()
	_check(map_label.get_selected_text().contains("Footnote content mapped to its source line"), "Appending after a footnote discarded its definition")
	map_label.queue_free()
	var effect_label: MarkdownLabel = MarkdownLabel.new()
	effect_label.size = Vector2(640.0, 520.0)
	effect_label.add_theme_constant_override("outline_size", 16)
	effect_label.add_theme_constant_override("shadow_outline_size", 16)
	effect_label.add_theme_constant_override("shadow_offset_x", -1)
	effect_label.add_theme_constant_override("shadow_offset_y", 2)
	effect_label.add_theme_color_override("font_outline_color", Color.RED)
	effect_label.add_theme_color_override("font_shadow_color", Color.BLUE)
	get_root().add_child(effect_label)
	var effect_lines: PackedStringArray = PackedStringArray()
	for line_index in range(24):
		effect_lines.append("Theme text effects line %d" % line_index)
	effect_label.text = "\n".join(effect_lines)
	await process_frame
	_check(effect_label.get_rendered_block_count() > 0, "Large text effect overrides prevented Markdown rendering")
	effect_label.queue_free()
	snapshot.queue_free()
	label.queue_free()
	await process_frame
	print("Markdown pipeline regression: %d failures" % _failures.size())
	quit(0 if _failures.is_empty() else 1)
