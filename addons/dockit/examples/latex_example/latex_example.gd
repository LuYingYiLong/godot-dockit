extends Control


const DEFAULT_LATEX: String = "\\int_{0}^{\\infty} e^{-x^2} \\, dx = \\frac{\\sqrt{\\pi}}{2}"
const ORBIT_SENSITIVITY: float = 0.01
const CAMERA_ZOOM_STEP: float = 0.9
const CAMERA_MIN_DISTANCE: float = 0.4
const CAMERA_MAX_DISTANCE: float = 8.0
const MAX_ORBIT_PITCH: float = 1.48353

@onready var _code_edit: CodeEdit = %CodeEdit
@onready var _latex_label: LatexLabel = %LatexLabel
@onready var _latex_label_3d: LatexLabel3D = %LatexLabel3D
@onready var _orbit_pivot: Node3D = %OrbitPivot
@onready var _camera: Camera3D = %Camera3D
@onready var _tab_container: TabContainer = %TabContainer
@onready var _box_container: VBoxContainer = %VBoxContainer
@onready var _subviewport_container: SubViewportContainer = %SubViewportContainer
@onready var _preview_timer: Timer = %PreviewTimer

var _dragging: bool
var _three_d_tab_index: int = -1


func _ready() -> void:
	_three_d_tab_index = _tab_container.get_tab_idx_from_control(_box_container)
	_orbit_pivot.position = _latex_label_3d.position
	_camera.position = Vector3(0.0, 0.0, 1.5)
	_code_edit.caret_blink = true
	_code_edit.text = DEFAULT_LATEX
	_update_preview()


func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		var mouse_event: InputEventMouseButton = event as InputEventMouseButton
		if mouse_event.button_index == MOUSE_BUTTON_WHEEL_UP or mouse_event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			if mouse_event.pressed and _is_three_d_preview_active() and _is_over_three_d_preview():
				_zoom_camera(mouse_event.button_index == MOUSE_BUTTON_WHEEL_UP, mouse_event.factor)
				get_viewport().set_input_as_handled()
			return
		if mouse_event.pressed:
			if _is_three_d_preview_active() and _is_over_three_d_preview():
				_dragging = true
				get_viewport().set_input_as_handled()
		elif _dragging:
			_dragging = false
			get_viewport().set_input_as_handled()
			
	elif event is InputEventMouseMotion and _dragging and _is_three_d_preview_active():
		var motion_event: InputEventMouseMotion = event as InputEventMouseMotion
		var orbit: Vector3 = _orbit_pivot.rotation
		orbit.y -= motion_event.relative.x * ORBIT_SENSITIVITY
		orbit.x = clampf(orbit.x - motion_event.relative.y * ORBIT_SENSITIVITY, -MAX_ORBIT_PITCH, MAX_ORBIT_PITCH)
		_orbit_pivot.rotation = orbit
		get_viewport().set_input_as_handled()


func _on_code_edit_text_changed() -> void:
	_preview_timer.start()


func _on_preview_timer_timeout() -> void:
	_update_preview()


func _on_tab_container_tab_changed(_tab: int) -> void:
	_dragging = false


func _update_preview() -> void:
	var latex: String = _code_edit.text
	_latex_label.text = latex
	_latex_label_3d.text = latex


func _zoom_camera(zoom_in: bool, factor: float) -> void:
	var zoom_factor: float = pow(CAMERA_ZOOM_STEP, maxf(factor, 0.01))
	var zoom_scale: float = zoom_factor if zoom_in else 1.0 / zoom_factor
	_camera.position.z = clampf(_camera.position.z * zoom_scale, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE)


func _is_three_d_preview_active() -> bool:
	return _three_d_tab_index >= 0 and _tab_container.current_tab == _three_d_tab_index


func _is_over_three_d_preview() -> bool:
	return _subviewport_container.get_global_rect().has_point(get_global_mouse_position())
