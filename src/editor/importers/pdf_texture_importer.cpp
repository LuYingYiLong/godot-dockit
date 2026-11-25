#include "pdf_texture_importer.h"

#include "formats/pdf/reader/pdf_reader.h"
#include "formats/pdf/pdf_texture.h"

#include <godot_cpp/classes/resource_saver.hpp>

#include <limits>

namespace godot {
	bool PDFTextureImporter::is_supported() {
		return PDFReader::is_supported();
	}

	String PDFTextureImporter::_get_importer_name() const {
		return "dockit.pdf_texture";
	}

	String PDFTextureImporter::_get_visible_name() const {
		return tr("PDF Texture", "Godot Dockit");
	}

	PackedStringArray PDFTextureImporter::_get_recognized_extensions() const {
		PackedStringArray extensions;
		extensions.append("pdf");
		return extensions;
	}

	TypedArray<Dictionary> PDFTextureImporter::_get_import_options(const String&, int32_t) const {
		Dictionary page;
		page["name"] = "page";
		page["default_value"] = 1;
		page["property_hint"] = PROPERTY_HINT_RANGE;
		page["hint_string"] = "1,100,1,or_greater";

		Dictionary dpi;
		dpi["name"] = "dpi";
		dpi["default_value"] = PDFTexture::DEFAULT_DPI;
		dpi["property_hint"] = PROPERTY_HINT_RANGE;
		dpi["hint_string"] = "1,600,1,or_greater,suffix:dpi";

		Dictionary rotation;
		rotation["name"] = "rotation";
		rotation["default_value"] = 0;
		rotation["property_hint"] = PROPERTY_HINT_ENUM;
		rotation["hint_string"] = "0°,90°,180°,270°";

		Dictionary background_color;
		background_color["name"] = "background_color";
		background_color["default_value"] = Color(1, 1, 1, 1);
		background_color["property_hint"] = PROPERTY_HINT_NONE;

		Dictionary render_annotations;
		render_annotations["name"] = "render_annotations";
		render_annotations["default_value"] = true;

		Dictionary render_form_widgets;
		render_form_widgets["name"] = "render_form_widgets";
		render_form_widgets["default_value"] = false;

		Dictionary grayscale;
		grayscale["name"] = "grayscale";
		grayscale["default_value"] = false;

		TypedArray<Dictionary> options;
		options.append(page);
		options.append(dpi);
		options.append(rotation);
		options.append(background_color);
		options.append(render_annotations);
		options.append(render_form_widgets);
		options.append(grayscale);
		return options;
	}

	String PDFTextureImporter::_get_save_extension() const {
		return "res";
	}

	String PDFTextureImporter::_get_resource_type() const {
		return PDFTexture::get_class_static();
	}

	int32_t PDFTextureImporter::_get_format_version() const {
		return 1;
	}

	bool PDFTextureImporter::_can_import_threaded() const {
		return false;
	}

	Error PDFTextureImporter::_import(const String& p_source_file, const String& p_save_path, const Dictionary& p_options,
		const TypedArray<String>&, const TypedArray<String>&) const {
		const int64_t page_number = p_options.get("page", 1);
		const int64_t dpi = p_options.get("dpi", PDFTexture::DEFAULT_DPI);
		const int64_t rotation_option = p_options.get("rotation", 0);
		const int64_t rotation = rotation_option * 90;
		const Color background_color = p_options.get("background_color", Color(1, 1, 1, 1));
		const bool render_annotations = p_options.get("render_annotations", true);
		const bool render_form_widgets = p_options.get("render_form_widgets", false);
		const bool grayscale = p_options.get("grayscale", false);

		ERR_FAIL_COND_V_MSG(page_number < 1, ERR_INVALID_PARAMETER, "PDF Texture: page numbers start at 1.");
		ERR_FAIL_COND_V_MSG(dpi < 1 || dpi > std::numeric_limits<int32_t>::max(), ERR_INVALID_PARAMETER,
			"PDF Texture: DPI must be a positive 32-bit integer.");
		ERR_FAIL_COND_V_MSG(rotation_option < 0 || rotation_option > 3, ERR_INVALID_PARAMETER,
			"PDF Texture: rotation must be 0, 90, 180, or 270 degrees.");

		int64_t render_flags = 0;
		if (render_annotations) {
			render_flags |= PDFReader::RENDER_ANNOTATIONS;
		}
		if (render_form_widgets) {
			render_flags |= PDFReader::RENDER_FORM_WIDGETS;
		}
		if (grayscale) {
			render_flags |= PDFReader::RENDER_GRAYSCALE;
		}

		Ref<PDFReader> reader;
		reader.instantiate();
		const Error error = reader->open_file(p_source_file);
		ERR_FAIL_COND_V_MSG(error != OK, error, vformat("PDF Texture: %s", reader->get_last_error_message()));
		const int64_t page_count = reader->get_page_count();
		ERR_FAIL_COND_V_MSG(page_number > page_count, ERR_INVALID_PARAMETER,
			vformat("PDF Texture: requested page %d, but %s has %d pages.", page_number, p_source_file, page_count));

		const Dictionary page_info = reader->get_page_info(page_number - 1);
		ERR_FAIL_COND_V_MSG(page_info.is_empty(), ERR_FILE_CORRUPT,
			vformat("PDF Texture: could not read metadata for page %d in %s.", page_number, p_source_file));
		const Vector2 page_size_pt = page_info.get("size_pt", Vector2());
		const int64_t page_rotation = page_info.get("rotation", 0);
		const String page_label = page_info.get("label", String());
		const bool has_transparency = page_info.get("has_transparency", false);
		const Dictionary page_boxes = page_info.get("boxes", Dictionary());

		const Ref<Image> image = reader->render_page(page_number - 1, dpi, render_flags, rotation, background_color);
		ERR_FAIL_COND_V_MSG(image.is_null(), reader->get_last_error(),
			vformat("PDF Texture: %s (file %s, page %d at %d DPI)", reader->get_last_error_message(), p_source_file, page_number, dpi));

		Ref<PDFTexture> texture;
		texture.instantiate();
		texture->set_image(image);
		texture->set_page(static_cast<int32_t>(page_number));
		texture->set_dpi(static_cast<int32_t>(dpi));
		texture->set_page_count(static_cast<int32_t>(page_count));
		texture->set_page_size_pt(page_size_pt);
		texture->set_page_rotation(static_cast<int32_t>(page_rotation));
		texture->set_page_label(page_label);
		texture->set_has_transparency(has_transparency);
		texture->set_page_boxes(page_boxes);
		texture->set_rotation(static_cast<int32_t>(rotation));
		texture->set_background_color(background_color);
		texture->set_render_flags(static_cast<int32_t>(render_flags));
		return ResourceSaver::get_singleton()->save(texture, p_save_path + String(".res"), ResourceSaver::FLAG_COMPRESS);
	}
} // namespace godot
