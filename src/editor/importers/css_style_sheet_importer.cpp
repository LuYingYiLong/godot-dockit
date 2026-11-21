#include "css_style_sheet_importer.h"

#include "formats/dockit_source.h"
#include "formats/html/css/css_style_sheet.h"
#include "formats/html/resources/html_asset_collector.h"

#include <godot_cpp/classes/resource_saver.hpp>

namespace godot {
	namespace {
		PackedStringArray get_import_dependencies(const String& p_source_file) {
			String source;
			if (dockit::read_document_text(p_source_file, source) != OK) return {};
		return dockit::collect_css_stylesheet_imports(source, p_source_file);
		}
	}

	String CSSStyleSheetImporter::_get_importer_name() const {
		return "dockit.css_stylesheet";
	}

	String CSSStyleSheetImporter::_get_visible_name() const {
		return tr("CSS Style Sheet", "Godot Dockit");
	}

	PackedStringArray CSSStyleSheetImporter::_get_recognized_extensions() const {
		return { "css" };
	}

	TypedArray<Dictionary> CSSStyleSheetImporter::_get_import_options(const String&, int32_t) const {
		return {};
	}

	String CSSStyleSheetImporter::_get_save_extension() const {
		return "res";
	}

	String CSSStyleSheetImporter::_get_resource_type() const {
		return String(CSSStyleSheet::get_class_static());
	}

	int32_t CSSStyleSheetImporter::_get_import_order() const {
		return IMPORT_ORDER_DEFAULT;
	}

	int32_t CSSStyleSheetImporter::_get_format_version() const {
		return FORMAT_VERSION;
	}

	PackedStringArray CSSStyleSheetImporter::_get_build_dependencies(const String& p_path) const {
		return get_import_dependencies(p_path);
	}

	Error CSSStyleSheetImporter::_import(const String& p_source_file, const String& p_save_path, const Dictionary&,
		const TypedArray<String>&, const TypedArray<String>&) const {
		String source;
		const Error error = dockit::read_document_text(p_source_file, source);
		if (error != OK) return error;
		Ref<CSSStyleSheet> resource;
		resource.instantiate();
		resource->set_css_text(source);
		resource->set_source_path(p_source_file);
		return ResourceSaver::get_singleton()->save(resource, p_save_path + String(".res"));
	}
} // namespace godot
