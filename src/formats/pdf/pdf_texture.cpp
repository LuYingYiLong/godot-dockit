#include "pdf_texture.h"

using namespace godot;

void PDFTexture::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_page"), &PDFTexture::get_page);
	ClassDB::bind_method(D_METHOD("get_dpi"), &PDFTexture::get_dpi);
	ClassDB::bind_method(D_METHOD("get_page_count"), &PDFTexture::get_page_count);
	ClassDB::bind_method(D_METHOD("get_page_size_pt"), &PDFTexture::get_page_size_pt);
	ClassDB::bind_method(D_METHOD("get_page_rotation"), &PDFTexture::get_page_rotation);
	ClassDB::bind_method(D_METHOD("get_page_label"), &PDFTexture::get_page_label);
	ClassDB::bind_method(D_METHOD("get_has_transparency"), &PDFTexture::get_has_transparency);
	ClassDB::bind_method(D_METHOD("get_page_boxes"), &PDFTexture::get_page_boxes);
	ClassDB::bind_method(D_METHOD("get_rotation"), &PDFTexture::get_rotation);
	ClassDB::bind_method(D_METHOD("get_background_color"), &PDFTexture::get_background_color);
	ClassDB::bind_method(D_METHOD("get_render_flags"), &PDFTexture::get_render_flags);
	ClassDB::bind_method(D_METHOD("_set_page", "page"), &PDFTexture::set_page);
	ClassDB::bind_method(D_METHOD("_set_dpi", "dpi"), &PDFTexture::set_dpi);
	ClassDB::bind_method(D_METHOD("_set_page_count", "page_count"), &PDFTexture::set_page_count);
	ClassDB::bind_method(D_METHOD("_set_page_size_pt", "page_size_pt"), &PDFTexture::set_page_size_pt);
	ClassDB::bind_method(D_METHOD("_set_page_rotation", "page_rotation"), &PDFTexture::set_page_rotation);
	ClassDB::bind_method(D_METHOD("_set_page_label", "page_label"), &PDFTexture::set_page_label);
	ClassDB::bind_method(D_METHOD("_set_has_transparency", "has_transparency"), &PDFTexture::set_has_transparency);
	ClassDB::bind_method(D_METHOD("_set_page_boxes", "page_boxes"), &PDFTexture::set_page_boxes);
	ClassDB::bind_method(D_METHOD("_set_rotation", "rotation"), &PDFTexture::set_rotation);
	ClassDB::bind_method(D_METHOD("_set_background_color", "background_color"), &PDFTexture::set_background_color);
	ClassDB::bind_method(D_METHOD("_set_render_flags", "render_flags"), &PDFTexture::set_render_flags);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "_page", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_page", "get_page");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_dpi", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_dpi", "get_dpi");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_page_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_page_count", "get_page_count");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "_page_size_pt", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_page_size_pt", "get_page_size_pt");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_page_rotation", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_page_rotation", "get_page_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "_page_label", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_page_label", "get_page_label");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "_has_transparency", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_has_transparency", "get_has_transparency");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "_page_boxes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_page_boxes", "get_page_boxes");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_rotation", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_rotation", "get_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "_background_color", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_background_color", "get_background_color");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_render_flags", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_render_flags", "get_render_flags");

	const uint32_t usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY;
	ADD_PROPERTY(PropertyInfo(Variant::INT, "page", PROPERTY_HINT_NONE, "", usage), "", "get_page");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dpi", PROPERTY_HINT_NONE, "", usage), "", "get_dpi");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "page_count", PROPERTY_HINT_NONE, "", usage), "", "get_page_count");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "page_size_pt", PROPERTY_HINT_NONE, "", usage), "", "get_page_size_pt");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "page_rotation", PROPERTY_HINT_NONE, "", usage), "", "get_page_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "page_label", PROPERTY_HINT_NONE, "", usage), "", "get_page_label");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "has_transparency", PROPERTY_HINT_NONE, "", usage), "", "get_has_transparency");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "page_boxes", PROPERTY_HINT_NONE, "", usage), "", "get_page_boxes");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rotation", PROPERTY_HINT_NONE, "", usage), "", "get_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color", PROPERTY_HINT_NONE, "", usage), "", "get_background_color");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "render_flags", PROPERTY_HINT_FLAGS,
		"Annotations,Grayscale,Form Widgets,Printing,LCD Text,No Native Text,Limited Image Cache,Force Halftone,No Smooth Text,No Smooth Image,No Smooth Path", usage),
		"", "get_render_flags");
}

int32_t PDFTexture::get_page() const {
	return page;
}

int32_t PDFTexture::get_dpi() const {
	return dpi;
}

int32_t PDFTexture::get_page_count() const {
	return page_count;
}

Vector2 PDFTexture::get_page_size_pt() const {
	return page_size_pt;
}

int32_t PDFTexture::get_page_rotation() const {
	return page_rotation;
}

String PDFTexture::get_page_label() const {
	return page_label;
}

bool PDFTexture::get_has_transparency() const {
	return has_transparency;
}

Dictionary PDFTexture::get_page_boxes() const {
	return page_boxes.duplicate(true);
}

int32_t PDFTexture::get_rotation() const {
	return rotation;
}

Color PDFTexture::get_background_color() const {
	return background_color;
}

int32_t PDFTexture::get_render_flags() const {
	return render_flags;
}

void PDFTexture::set_page(int32_t p_page) {
	page = p_page;
}

void PDFTexture::set_dpi(int32_t p_dpi) {
	dpi = p_dpi;
}

void PDFTexture::set_page_count(int32_t p_page_count) {
	page_count = p_page_count;
}

void PDFTexture::set_page_size_pt(const Vector2& p_page_size_pt) {
	page_size_pt = p_page_size_pt;
}

void PDFTexture::set_page_rotation(int32_t p_page_rotation) {
	page_rotation = p_page_rotation;
}

void PDFTexture::set_page_label(const String& p_page_label) {
	page_label = p_page_label;
}

void PDFTexture::set_has_transparency(bool p_has_transparency) {
	has_transparency = p_has_transparency;
}

void PDFTexture::set_page_boxes(const Dictionary& p_page_boxes) {
	page_boxes = p_page_boxes;
}

void PDFTexture::set_rotation(int32_t p_rotation) {
	rotation = p_rotation;
}

void PDFTexture::set_background_color(const Color& p_background_color) {
	background_color = p_background_color;
}

void PDFTexture::set_render_flags(int32_t p_render_flags) {
	render_flags = p_render_flags;
}
