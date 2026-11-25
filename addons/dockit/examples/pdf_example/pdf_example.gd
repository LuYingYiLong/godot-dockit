extends Control


const PDF_DIRECTORY: String = "res://addons/dockit/examples/pdf_example/files/"
const SCALE_FIT_PAGE: int = 0
const SCALE_FIT_WIDTH: int = 1


@onready var _file_option_button: OptionButton = %FileOptionButton
@onready var _page_number_spin_box: SpinBox = %PageNumberSpinBox
@onready var _scale_option_button: OptionButton = %ScaleOptionButton
@onready var _back_button: Button = %BackButton
@onready var _forward_button: Button = %ForwardButton
@onready var _scroll_container: ScrollContainer = %ScrollContainer
@onready var _page_container: VBoxContainer = %PageContainer


var _file_list: Array[String]
var _page_texture_rects: Array[TextureRect]
var _page_numbers: Array[int]
var _pdf_texture: PDFTexture
var _pdf_reader: PDFReader
var _page_count: int
var _current_page: int
var _scale_mode: int
var _is_loading: bool
var _is_syncing_page_number: bool
var _can_render_pages: bool


func _ready() -> void:
	_page_number_spin_box.get_line_edit().caret_blink = true
	_scroll_container.get_v_scroll_bar().value_changed.connect(_on_vertical_scroll_value_changed)
	_pdf_reader = PDFReader.new()
	_scale_option_button.select(0)
	_scale_mode = SCALE_FIT_PAGE
	_load_pdf_list()


func _exit_tree() -> void:
	if _pdf_reader != null:
		_pdf_reader.close()


func _load_pdf_list() -> void:
	var directory: DirAccess = DirAccess.open(PDF_DIRECTORY)
	if directory == null:
		push_error("Failed to open the PDF examples directory.")
		return

	directory.list_dir_begin()
	var file_name: String = directory.get_next()
	while not file_name.is_empty():
		if not directory.current_is_dir() and file_name.get_extension().to_lower() == "pdf":
			_file_list.append(PDF_DIRECTORY.path_join(file_name))
		file_name = directory.get_next()
	directory.list_dir_end()
	_file_list.sort()

	if _file_list.is_empty():
		push_error("No PDF examples were found.")
		return

	_file_option_button.clear()
	for file_index: int in range(_file_list.size()):
		_file_option_button.add_item(_file_list[file_index].get_file().get_basename(), file_index)
	_file_option_button.select(0)
	_load_pdf(_file_list[0])


func _load_pdf(file_path: String) -> void:
	var loaded_resource: Resource = ResourceLoader.load(file_path)
	if not loaded_resource is PDFTexture:
		push_error("Failed to load the imported PDF texture: " + file_path)
		return

	_pdf_texture = loaded_resource as PDFTexture
	_pdf_reader.close()
	var error: Error = _pdf_reader.open_file(file_path)
	_can_render_pages = error == OK and _pdf_reader.is_open()
	_page_count = _pdf_reader.get_page_count() if _can_render_pages else _pdf_texture.page_count
	if _page_count < 1:
		push_error("The PDF does not contain any pages: " + file_path)
		return

	var initial_page: int = clampi(_pdf_texture.page, 1, _page_count)
	_is_loading = true
	_page_number_spin_box.min_value = 1.0
	_page_number_spin_box.max_value = _page_count
	_page_number_spin_box.step = 1.0
	_page_number_spin_box.suffix = "/%d" % _page_count
	_page_number_spin_box.value = initial_page
	_current_page = initial_page
	_scroll_container.scroll_vertical = 0

	if _can_render_pages:
		if not _create_pdf_pages():
			_can_render_pages = false
			_create_imported_page_fallback()
	else:
		push_warning("PDFReader could not open the source PDF. Showing its imported page only: " + _pdf_reader.get_last_error_message())
		_create_imported_page_fallback()

	_apply_scale_mode()
	_is_loading = false
	call_deferred("_scroll_to_page", initial_page)
	_update_pagination_controls()


func _create_pdf_pages() -> bool:
	_clear_pdf_pages()
	for page_number: int in range(1, _page_count + 1):
		var page_texture: Texture2D
		if page_number == _pdf_texture.page:
			page_texture = _pdf_texture
		else:
			var image: Image = _pdf_reader.render_page(
				page_number - 1,
				_pdf_texture.dpi,
				_pdf_texture.render_flags,
				_pdf_texture.rotation,
				_pdf_texture.background_color,
			)
			if image == null:
				push_error("Failed to render PDF page %d: %s" % [page_number, _pdf_reader.get_last_error_message()])
				_clear_pdf_pages()
				return false
			page_texture = ImageTexture.create_from_image(image)
		_add_page_texture_rect(page_number, page_texture)
	return true


func _create_imported_page_fallback() -> void:
	_clear_pdf_pages()
	_add_page_texture_rect(_pdf_texture.page, _pdf_texture)
	_current_page = _pdf_texture.page


func _add_page_texture_rect(page_number: int, texture: Texture2D) -> void:
	var texture_rect: TextureRect = TextureRect.new()
	texture_rect.name = "Page%d" % page_number
	texture_rect.texture = texture
	texture_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	texture_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	texture_rect.size_flags_vertical = Control.SIZE_SHRINK_BEGIN
	texture_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_page_container.add_child(texture_rect)
	_page_texture_rects.append(texture_rect)
	_page_numbers.append(page_number)


func _clear_pdf_pages() -> void:
	for child: Node in _page_container.get_children():
		_page_container.remove_child(child)
		child.queue_free()
	_page_texture_rects.clear()
	_page_numbers.clear()


func _apply_scale_mode() -> void:
	if _page_texture_rects.is_empty():
		return

	var vertical_scroll_bar: VScrollBar = _scroll_container.get_v_scroll_bar()
	var horizontal_scroll_bar: HScrollBar = _scroll_container.get_h_scroll_bar()
	var available_width: float = maxf(_scroll_container.size.x - vertical_scroll_bar.get_combined_minimum_size().x, 1.0)
	var available_height: float = maxf(_scroll_container.size.y - horizontal_scroll_bar.get_combined_minimum_size().y, 1.0)
	for texture_rect: TextureRect in _page_texture_rects:
		if texture_rect.texture == null:
			continue
		var texture_size: Vector2 = Vector2(texture_rect.texture.get_size())
		if texture_size.x <= 0.0 or texture_size.y <= 0.0:
			continue
		var scale_factor: float = available_width / texture_size.x
		if _scale_mode == SCALE_FIT_PAGE:
			scale_factor = minf(scale_factor, available_height / texture_size.y)
		texture_rect.custom_minimum_size = texture_size * scale_factor
		texture_rect.size_flags_horizontal = Control.SIZE_SHRINK_CENTER if _scale_mode == SCALE_FIT_PAGE else Control.SIZE_EXPAND_FILL
	if not _is_loading:
		call_deferred("_update_current_page_from_scroll")


func _scroll_to_page(page_number: int) -> void:
	var page_index: int = _page_numbers.find(page_number)
	if page_index < 0:
		return
	_scroll_container.scroll_vertical = maxi(roundi(_page_texture_rects[page_index].position.y), 0)
	_update_current_page_from_scroll()


func _update_current_page_from_scroll() -> void:
	if _is_loading or _page_texture_rects.is_empty():
		return

	var viewport_center_y: float = _scroll_container.scroll_vertical + _scroll_container.size.y * 0.5
	var closest_page_index: int = 0
	var closest_distance: float = INF
	for page_index: int in range(_page_texture_rects.size()):
		var texture_rect: TextureRect = _page_texture_rects[page_index]
		var page_center_y: float = texture_rect.position.y + texture_rect.size.y * 0.5
		var distance: float = absf(page_center_y - viewport_center_y)
		if distance < closest_distance:
			closest_distance = distance
			closest_page_index = page_index

	var visible_page: int = _page_numbers[closest_page_index]
	if visible_page != _current_page:
		_current_page = visible_page
		_is_syncing_page_number = true
		_page_number_spin_box.value = visible_page
		_is_syncing_page_number = false
	_update_pagination_controls()


func _update_pagination_controls() -> void:
	_page_number_spin_box.editable = _can_render_pages
	_back_button.disabled = not _can_render_pages or _current_page <= 1
	_forward_button.disabled = not _can_render_pages or _current_page >= _page_count


func _on_file_option_button_item_selected(index: int) -> void:
	if index < 0 or index >= _file_list.size():
		return
	_load_pdf(_file_list[index])


func _on_page_number_spin_box_value_changed(value: float) -> void:
	if _is_loading or _is_syncing_page_number:
		return
	call_deferred("_scroll_to_page", roundi(value))


func _on_scale_option_button_item_selected(index: int) -> void:
	_scale_mode = _scale_option_button.get_item_id(index)
	_apply_scale_mode()


func _on_vertical_scroll_value_changed(_value: float) -> void:
	_update_current_page_from_scroll()


func _on_scroll_container_resized() -> void:
	_apply_scale_mode()


func _on_back_button_pressed() -> void:
	if _can_render_pages:
		_page_number_spin_box.value = maxi(_current_page - 1, 1)


func _on_forward_button_pressed() -> void:
	if _can_render_pages:
		_page_number_spin_box.value = mini(_current_page + 1, _page_count)
