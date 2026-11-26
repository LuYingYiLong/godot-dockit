#include "style_box_css_helper.h"

#include "formats/html/html_layout.h"
#include "formats/html/elements/html_element.h"
#include "formats/html/css/css_shadows.h"
#include "formats/html/css/css_parser.h"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <algorithm>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/style_box_line.hpp>
#include <godot_cpp/classes/style_box_texture.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

namespace dockit {
	namespace {
		HashMap<uint64_t, Dictionary> conversion_cache;
		void invalidate_cached_stylebox(int64_t p_id) { conversion_cache.erase(uint64_t(p_id)); }
		const char* sides[] = { "left", "top", "right", "bottom" };
		const char* corners[] = { "top-left", "top-right", "bottom-right", "bottom-left" };
		String px(double p_value) { return String::num(p_value) + "px"; }
		String color_css(const Color& p_color) { return "#" + p_color.to_html(true); }
		void declaration(String& r_css, const String& p_name, const String& p_value) {
			r_css += p_name + String(": ") + p_value + String(";\n");
		}
		Dictionary result(Error p_error, const PackedStringArray& p_diagnostics) {
			Dictionary value;
			value["error"] = p_error;
			value["diagnostics"] = p_diagnostics;
			return value;
		}
		String escape_attribute(String p_value) {
			return p_value.replace("&", "&amp;").replace("\"", "&quot;").replace("<", "&lt;").replace(">", "&gt;");
		}
		String texture_repeat(StyleBoxTexture::AxisStretchMode p_mode) {
			return p_mode == StyleBoxTexture::AXIS_STRETCH_MODE_TILE ? "repeat" :
				p_mode == StyleBoxTexture::AXIS_STRETCH_MODE_TILE_FIT ? "round" : "stretch";
		}
		StyleBoxTexture::AxisStretchMode stretch_mode(const String& p_value) {
			return p_value == "repeat" ? StyleBoxTexture::AXIS_STRETCH_MODE_TILE :
				p_value == "round" ? StyleBoxTexture::AXIS_STRETCH_MODE_TILE_FIT : StyleBoxTexture::AXIS_STRETCH_MODE_STRETCH;
		}
	}

	void StyleBoxCSSHelper::_bind_methods() {
		ClassDB::bind_static_method("StyleBoxCSSHelper", D_METHOD("stylebox_to_css", "stylebox"), &StyleBoxCSSHelper::stylebox_to_css);
		ClassDB::bind_static_method("StyleBoxCSSHelper", D_METHOD("css_to_stylebox", "css", "context", "target_type"),
			&StyleBoxCSSHelper::css_to_stylebox, DEFVAL(Dictionary()), DEFVAL(StringName("auto")));
	}

	Dictionary StyleBoxCSSHelper::stylebox_to_css(const Ref<StyleBox>& p_stylebox) {
		PackedStringArray diagnostics;
		if (p_stylebox.is_null()) return result(ERR_INVALID_PARAMETER, diagnostics);
		const uint64_t cache_id = p_stylebox->get_instance_id();
		if (const auto* cached = conversion_cache.getptr(cache_id)) return cached->duplicate(true);
		String css;
		Dictionary textures;
		declaration(css, "box-sizing", "border-box");
		const auto* flat = Object::cast_to<StyleBoxFlat>(p_stylebox.ptr());
		const auto* texture = Object::cast_to<StyleBoxTexture>(p_stylebox.ptr());
		const auto* line = Object::cast_to<StyleBoxLine>(p_stylebox.ptr());
		if (!flat && !texture && !line && !Object::cast_to<StyleBoxEmpty>(p_stylebox.ptr())) {
			diagnostics.push_back("Unsupported StyleBox subclass: " + p_stylebox->get_class());
			return result(ERR_UNAVAILABLE, diagnostics);
		}
		for (int i = 0; i < 4; ++i) {
			const double border = flat ? flat->get_border_width(Side(i)) : 0;
			declaration(css, String("padding-") + sides[i], px(MAX(0.0, p_stylebox->get_content_margin(Side(i)) - border)));
			if (p_stylebox->get_content_margin(Side(i)) < border)
				diagnostics.push_back("Content margin smaller than border cannot be represented by CSS padding.");
		}
		if (flat) {
			declaration(css, "background-color", color_css(flat->is_draw_center_enabled() ? flat->get_bg_color() : Color(0, 0, 0, 0)));
			declaration(css, "border-style", "solid");
			declaration(css, "border-color", color_css(flat->get_border_color()));
			for (int i = 0; i < 4; ++i) {
				declaration(css, String("border-") + sides[i] + "-width", px(flat->get_border_width(Side(i))));
				declaration(css, String("border-") + corners[i] + "-radius", px(flat->get_corner_radius(Corner(i))));
				if (flat->get_expand_margin(Side(i)) != 0) diagnostics.push_back("StyleBoxFlat expand margins have no CSS box-decoration equivalent.");
			}
			if (flat->get_shadow_color().a > 0 && flat->get_shadow_size() > 0) {
				declaration(css, "box-shadow", px(flat->get_shadow_offset().x) + " " + px(flat->get_shadow_offset().y) + " " +
					px(flat->get_shadow_size()) + " " + color_css(flat->get_shadow_color()));
				diagnostics.push_back("Godot and CSS shadow falloff differ; shadow size is mapped to CSS blur.");
			}
			if (flat->get_skew() != Vector2()) diagnostics.push_back("StyleBoxFlat skew is not converted.");
			if (flat->get_border_blend()) diagnostics.push_back("StyleBoxFlat border blending has no CSS equivalent.");
			diagnostics.push_back("Godot antialiasing and corner tessellation settings are not encoded in CSS.");
		}
		if (texture) {
			Ref<Texture2D> source = texture->get_texture();
			if (source.is_null()) {
				diagnostics.push_back("StyleBoxTexture has no texture.");
				return result(ERR_INVALID_DATA, diagnostics);
			}
			if (texture->get_region_rect().has_area() || texture->get_modulate() != Color(1, 1, 1, 1)) {
				Ref<Image> image = source->get_image();
				if (image.is_null()) {
					diagnostics.push_back("Texture cannot be read for region cropping or modulation.");
					return result(ERR_UNAVAILABLE, diagnostics);
				}
				image = image->duplicate();
				if (image->is_compressed() && image->decompress() != OK) return result(ERR_UNAVAILABLE, diagnostics);
				image->convert(Image::FORMAT_RGBA8);
				if (texture->get_region_rect().has_area()) {
					const Rect2i region = Rect2i(texture->get_region_rect()).intersection(Rect2i(Vector2i(), image->get_size()));
					if (!region.has_area()) return result(ERR_INVALID_DATA, diagnostics);
					image = image->get_region(region);
				}
				const Color modulate = texture->get_modulate();
				if (modulate != Color(1, 1, 1, 1)) {
					for (int y = 0; y < image->get_height(); ++y)
						for (int x = 0; x < image->get_width(); ++x) image->set_pixel(x, y, image->get_pixel(x, y) * modulate);
				}
				source = ImageTexture::create_from_image(image);
				diagnostics.push_back("Texture region and modulation were baked into a generated texture.");
			}
			const String uri = "res://__dockit_runtime/stylebox-" + String::num_uint64(p_stylebox->get_instance_id()) + ".png";
			textures[uri] = source;
			declaration(css, "border-image-source", "url(\"" + uri + "\")");
			String slice, width, outset;
			for (int i : { 1, 2, 3, 0 }) {
				const Side side = Side(i);
				if (!slice.is_empty()) { slice += " "; width += " "; outset += " "; }
				slice += String::num(texture->get_texture_margin(side));
				width += px(texture->get_texture_margin(side));
				outset += px(texture->get_expand_margin(side));
			}
			declaration(css, "border-image-slice", slice + (texture->is_draw_center_enabled() ? " fill" : ""));
			declaration(css, "border-image-width", width);
			declaration(css, "border-image-outset", outset);
			declaration(css, "border-image-repeat", texture_repeat(texture->get_h_axis_stretch_mode()) + " " + texture_repeat(texture->get_v_axis_stretch_mode()));
		}
		if (line) {
			declaration(css, "background-color", color_css(line->get_color()));
			declaration(css, line->is_vertical() ? "width" : "height", px(line->get_thickness()));
			if (line->get_grow_begin() != 0 || line->get_grow_end() != 0)
				diagnostics.push_back("StyleBoxLine grow offsets require a containing-box layout and are not converted.");
		}
		Dictionary output = result(OK, diagnostics);
		output["css"] = css;
		output["textures"] = textures;
		if (conversion_cache.size() >= 128) conversion_cache.clear();
		const Callable invalidated = callable_mp_static(&invalidate_cached_stylebox).bind(int64_t(cache_id));
		if (!p_stylebox->is_connected("changed", invalidated)) p_stylebox->connect("changed", invalidated);
		if (texture && texture->get_texture().is_valid() && !texture->get_texture()->is_connected("changed", invalidated))
			texture->get_texture()->connect("changed", invalidated);
		conversion_cache.insert(cache_id, output);
		return output.duplicate(true);
	}

	Dictionary StyleBoxCSSHelper::css_to_stylebox(const String& p_css, const Dictionary& p_context, const StringName& p_target_type) {
		PackedStringArray diagnostics;
		const String target = p_target_type;
		if (target != "auto" && target != "flat" && target != "empty" && target != "texture" && target != "line")
			return result(ERR_INVALID_PARAMETER, diagnostics);
		css::css_parser parser(css::tokenize(p_css.utf8().get_data()));
		css::raw_declaration::vector declarations;
		css::raw_rule::vector rules;
		parser.consume_style_block_contents(declarations, rules);
		if (!rules.empty()) {
			diagnostics.push_back("Expected a declaration block without rules or selectors.");
			return result(ERR_PARSE_ERROR, diagnostics);
		}
		for (const auto& declaration : declarations) {
			const String name = String::utf8(declaration.name.c_str());
			const bool length_property = name.begins_with("padding") || name.begins_with("border") || name.begins_with("margin") ||
				name == "width" || name == "height" || name == "box-shadow";
			if (!length_property) {
				if (name != "background" && name != "background-color" && name != "background-image" && name != "box-sizing")
					diagnostics.push_back("Property is not represented by a StyleBox: " + name);
				continue;
			}
			const String value = String::utf8(css::get_repr(declaration.value).c_str());
			if ((value.contains("%") && !p_context.has("box_size")) ||
				(value.contains("rem") && !p_context.has("root_font_size")) ||
				(value.contains("em") && !value.contains("rem") && !p_context.has("font_size")) ||
				((value.contains("vw") || value.contains("vh")) && !p_context.has("viewport_size")) || value.contains("var(")) {
				diagnostics.push_back("Unresolved relative length or variable in " + name + "; supply an explicit context.");
				return result(ERR_INVALID_PARAMETER, diagnostics);
			}
		}
		Ref<HTMLLayout> layout;
		layout.instantiate();
		layout->set_css_diagnostics_enabled(true);
		const Vector2i viewport = p_context.get("viewport_size", Vector2i(1024, 768));
		const double font_size = p_context.get("font_size", 16.0);
		const double root_size = p_context.get("root_font_size", font_size);
		String defaults = "html {font-size:" + px(root_size) + ";} body {margin:0;} #box {display:block;border:0 solid transparent;background:transparent;padding:0;font-size:" + px(font_size) + ";}";
		String parent;
		if (p_context.has("box_size")) {
			const Vector2 size = p_context["box_size"];
			parent = "width:" + px(size.x) + ";height:" + px(size.y) + ";";
		}
		const Error error = layout->parse_source("<div style=\"" + parent + "\"><div id=\"box\" style=\"" + escape_attribute(p_css) + "\"></div></div>",
			p_context.get("base_dir", "res://"), String(), p_context.get("textures", Dictionary()), String(), false, defaults);
		if (error != OK || layout->layout(viewport) != OK) return result(ERR_PARSE_ERROR, diagnostics);
		const Ref<HTMLElement> element = layout->get_element_by_id("box");
		const auto read = [&](const String& p_name) { return layout->get_computed_style_property(element, p_name); };
		const auto number = [&](const String& p_name) { return read(p_name).to_float(); };
		const auto color = [&](const String& p_name) {
			css::css_parser color_parser(css::tokenize((String("color:") + read(p_name) + String(";")).utf8().get_data()));
			css::raw_declaration::vector parsed_colors;
			css::raw_rule::vector color_rules;
			color_parser.consume_style_block_contents(parsed_colors, color_rules);
			if (parsed_colors.empty()) return Color(0, 0, 0, 0);
			auto tokens = parsed_colors.front().value;
			css::remove_whitespace(tokens);
			litehtml::web_color value;
			if (tokens.size() != 1 || !litehtml::parse_color(tokens.front(), value, nullptr)) return Color(0, 0, 0, 0);
			value = value.to_srgb();
			return Color(value.red / 255.0f, value.green / 255.0f, value.blue / 255.0f, value.alpha / 255.0f);
			};
		const auto css_diagnostics = layout->get_css_diagnostics();
		for (int i = 0; i < css_diagnostics.size(); ++i) diagnostics.push_back(String(css_diagnostics[i].stringify()));
		Ref<StyleBox> style;
		const String image_source = read("border-image-source");
		if (target == "texture" || (target == "auto" && !image_source.is_empty() && image_source != "none")) {
			Ref<StyleBoxTexture> texture;
			texture.instantiate();
			String uri = image_source.trim_prefix("url(").trim_suffix(")").strip_edges().trim_prefix("\"").trim_suffix("\"").trim_prefix("'").trim_suffix("'");
			const Dictionary textures = p_context.get("textures", Dictionary());
			Ref<Texture2D> source = textures.get(uri, Variant());
			if (source.is_null()) {
				diagnostics.push_back("border-image-source must be provided in context.textures: " + uri);
				return result(ERR_FILE_NOT_FOUND, diagnostics);
			}
			texture->set_texture(source);
			const auto expand = [](const PackedStringArray& p_values, int p_index) -> String {
				if (p_values.is_empty()) return "0";
				const int count = p_values.size();
				return p_values[count == 1 ? 0 : count == 2 ? p_index % 2 : count == 3 && p_index == 3 ? 1 : p_index];
				};
			const String slice_text = read("border-image-slice");
			const PackedStringArray slices = slice_text.replace("fill", "").strip_edges().split(" ", false);
			const PackedStringArray widths = read("border-image-width").split(" ", false);
			const PackedStringArray outsets = read("border-image-outset").split(" ", false);
			int index = 0;
			for (int i : { 1, 2, 3, 0 }) {
				String slice = expand(slices, index);
				const double dimension = (i % 2 == 0) ? source->get_width() : source->get_height();
				const double amount = slice.to_float() * (slice.ends_with("%") ? dimension / 100.0 : 1.0);
				texture->set_texture_margin(Side(i), amount);
				const String width = expand(widths, index);
				if (width.ends_with("px") && !Math::is_equal_approx(width.to_float(), amount)) diagnostics.push_back("Independent border-image width cannot be represented by StyleBoxTexture texture margins.");
				const String outset = expand(outsets, index++);
				if (!outset.ends_with("px") && outset.to_float() != 0) diagnostics.push_back("Relative border-image outsets require a resolved border width.");
				texture->set_expand_margin(Side(i), outset.to_float());
			}
			texture->set_draw_center(slice_text.contains("fill"));
			const PackedStringArray repeats = read("border-image-repeat").split(" ", false);
			if (!repeats.is_empty()) {
				texture->set_h_axis_stretch_mode(stretch_mode(repeats[0]));
				texture->set_v_axis_stretch_mode(stretch_mode(repeats[repeats.size() > 1 ? 1 : 0]));
				if (repeats.has("space")) diagnostics.push_back("border-image-repeat: space is approximated as stretch.");
			}
			style = texture;
		}
		else if (target == "empty") {
			Ref<StyleBoxEmpty> empty;
			empty.instantiate();
			style = empty;
		}
		else if (target == "line") {
			Ref<StyleBoxLine> line;
			line.instantiate();
			const bool vertical = p_context.get("vertical", false);
			line->set_vertical(vertical);
			line->set_thickness(MAX(0, int(number(vertical ? "width" : "height"))));
			line->set_color(color("background-color"));
			style = line;
		}
		else {
			Ref<StyleBoxFlat> flat;
			flat.instantiate();
			flat->set_bg_color(color("background-color"));
			const Color border = color("border-top-color");
			flat->set_border_color(border);
			for (int i = 0; i < 4; ++i) {
				flat->set_border_width(Side(i), MAX(0, int(number(String("border-") + sides[i] + "-width"))));
				const String radius = read(String("border-") + corners[i] + "-radius");
				flat->set_corner_radius(Corner(i), MAX(0, int(radius.to_float())));
				if (color(String("border-") + sides[i] + "-color") != border) diagnostics.push_back("Per-side border colors are approximated by the top border color.");
				const String border_style = read(String("border-") + sides[i] + "-style");
				if (border_style != "solid" && border_style != "none" && border_style != "hidden") diagnostics.push_back("Non-solid border is approximated by a solid StyleBoxFlat border.");
				if (radius.contains(" ") || radius.contains("%")) diagnostics.push_back("Elliptical or percentage border radius requires resolution or approximation.");
			}
			const String shadow_css = read("box-shadow");
			if (!shadow_css.is_empty() && shadow_css != "none") {
				css::css_parser shadow_parser(css::tokenize((String("box-shadow:") + shadow_css + String(";")).utf8().get_data()));
				css::raw_declaration::vector parsed_shadows;
				css::raw_rule::vector shadow_rules;
				shadow_parser.consume_style_block_contents(parsed_shadows, shadow_rules);
				auto tokens = parsed_shadows.empty() ? css::css_token_vector() : parsed_shadows.front().value;
				css::remove_whitespace(tokens);
				std::vector<litehtml::box_shadow> shadows;
				if (litehtml::parse_box_shadows(tokens, nullptr, shadows) && !shadows.empty()) {
					const auto& shadow = shadows.front();
					if (shadow.inset) diagnostics.push_back("Inset shadows cannot be represented by StyleBoxFlat.");
					else {
						const auto shadow_color = shadow.color.to_srgb();
						flat->set_shadow_color(Color(shadow_color.red / 255.0f, shadow_color.green / 255.0f, shadow_color.blue / 255.0f, shadow_color.alpha / 255.0f));
						flat->set_shadow_offset(Vector2(shadow.x.val(), shadow.y.val()));
						flat->set_shadow_size(MAX(0, int(shadow.blur.val())));
					}
					if (shadows.size() > 1 || shadow.spread.val() != 0) diagnostics.push_back("Additional shadows and shadow spread cannot be represented by StyleBoxFlat.");
					diagnostics.push_back("Godot and CSS shadow falloff differ.");
				}
				else diagnostics.push_back("Shadow could not be converted.");
			}
			if (read("background-image") != "none" && !read("background-image").is_empty()) diagnostics.push_back("Background images and gradients cannot be represented by StyleBoxFlat.");
			style = flat;
		}
		for (int i = 0; i < 4; ++i) style->set_content_margin(Side(i), number(String("padding-") + sides[i]) + number(String("border-") + sides[i] + "-width"));
		Dictionary output = result(OK, diagnostics);
		output["stylebox"] = style;
		return output;
	}
}

void dockit::StyleBoxCSSHelper::clear_cache() { conversion_cache.clear(); }
