#include "markdown_label.h"
#include "formats/remote_images.h"

#include "formats/html/markup/html_markup.h"
#include "formats/html/resources/html_resource_paths.h"
#include "formats/html/elements/html_element.h"
#include "formats/html/layout/html_geometry.h"
#include "formats/html/paint/html_paint_run.h"
#include "formats/word_boundaries.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/style_box_line.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <climits>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

using namespace godot;

namespace {
	static bool packed_string_array_has(const PackedStringArray& p_array, const StringName& p_name) {
		const String name = String(p_name);
		for (int32_t i = 0; i < p_array.size(); i++) {
			if (p_array[i] == name) {
				return true;
			}
		}

		return false;
	}

	static float get_theme_base_scale_or_one(Control* p_owner) {
		const float base_scale = p_owner == nullptr ? 1.0f : p_owner->get_theme_default_base_scale();
		return base_scale > 0.0f ? base_scale : 1.0f;
	}

	static int32_t scale_theme_fallback(int32_t p_value, float p_base_scale) {
		return static_cast<int32_t>(Math::round(static_cast<double>(p_value) * static_cast<double>(p_base_scale)));
	}

	static float scale_theme_fallback(float p_value, float p_base_scale) {
		return p_value * p_base_scale;
	}

	static int32_t get_explicit_theme_font_size(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		if (p_owner != nullptr && p_owner->has_theme_font_size_override(p_name)) {
			const int32_t value = p_owner->get_theme_font_size(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}

		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && packed_string_array_has(theme->get_font_size_list(String(p_type)), p_name)) {
			const int32_t value = theme->get_font_size(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}

		return -1;
	}

	static int32_t get_explicit_theme_constant(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		if (p_owner != nullptr && p_owner->has_theme_constant_override(p_name)) {
			return p_owner->get_theme_constant(p_name, p_type);
		}

		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && packed_string_array_has(theme->get_constant_list(String(p_type)), p_name)) {
			return theme->get_constant(p_name, p_type);
		}

		if (p_owner != nullptr && p_owner->has_theme_constant(p_name, p_type)) {
			return p_owner->get_theme_constant(p_name, p_type);
		}

		return INT_MIN;
	}

	static int32_t get_theme_font_size_or(Control* p_owner, const StringName& p_type, const StringName& p_name, int32_t p_default) {
		const int32_t explicit_font_size = get_explicit_theme_font_size(p_owner, p_type, p_name);
		if (explicit_font_size > 0) {
			return explicit_font_size;
		}

		return p_default;
	}

	static int32_t get_theme_constant_or(Control* p_owner, const StringName& p_type, const StringName& p_name, int32_t p_default) {
		const int32_t explicit_constant = get_explicit_theme_constant(p_owner, p_type, p_name);
		if (explicit_constant != INT_MIN) {
			return explicit_constant;
		}

		return p_default;
	}

	static int32_t get_scaled_theme_constant_or(Control* p_owner, const StringName& p_type, const StringName& p_name, int32_t p_default, float p_base_scale) {
		const int32_t explicit_constant = get_explicit_theme_constant(p_owner, p_type, p_name);
		if (explicit_constant != INT_MIN) {
			return explicit_constant;
		}

		return scale_theme_fallback(p_default, p_base_scale);
	}

	static Color get_theme_color_or(Control* p_owner, const StringName& p_type, const StringName& p_name, const Color& p_default) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_color(p_name, p_type)) {
			return theme->get_color(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_color(p_name, p_type)) {
			return p_owner->get_theme_color(p_name, p_type);
		}

		return p_default;
	}

	static Ref<Font> get_theme_font_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		if (p_owner != nullptr && p_owner->has_theme_font_override(p_name)) {
			return p_owner->get_theme_font(p_name, p_type);
		}
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_font(p_name, p_type)) {
			return theme->get_font(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_font(p_name, p_type)) {
			return p_owner->get_theme_font(p_name, p_type);
		}
		if (p_owner != nullptr) {
			return p_owner->get_theme_default_font();
		}

		return Ref<Font>();
	}

	static Ref<StyleBox> get_theme_stylebox_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_stylebox(p_name, p_type)) {
			return theme->get_stylebox(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_stylebox(p_name, p_type)) {
			return p_owner->get_theme_stylebox(p_name, p_type);
		}

		return Ref<StyleBox>();
	}

	static Color get_theme_stylebox_color_or(Control* p_owner, const StringName& p_type, const StringName& p_name,
		const Color& p_default) {
		const Ref<StyleBox> stylebox = get_theme_stylebox_or(p_owner, p_type, p_name);
		const auto* flat = stylebox.is_valid() ? Object::cast_to<StyleBoxFlat>(stylebox.ptr()) : nullptr;
		return flat == nullptr ? p_default : flat->get_bg_color();
	}

	static Ref<Texture2D> get_theme_icon_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_icon(p_name, p_type)) {
			return theme->get_icon(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_icon(p_name, p_type)) {
			return p_owner->get_theme_icon(p_name, p_type);
		}
		return Ref<Texture2D>();
	}

	static Ref<StyleBoxFlat> create_internal_stylebox(const Color& p_background_color, const Color& p_border_color, int32_t p_border_left, int32_t p_border_top, int32_t p_border_right, int32_t p_border_bottom, float p_margin_left, float p_margin_top, float p_margin_right, float p_margin_bottom, int32_t p_corner_radius = 0, bool p_draw_center = true, int32_t p_corner_detail = 8, float p_base_scale = 1.0f) {
		Ref<StyleBoxFlat> stylebox;
		stylebox.instantiate();
		stylebox->set_bg_color(p_background_color);
		stylebox->set_border_color(p_border_color);
		stylebox->set_border_width(SIDE_LEFT, scale_theme_fallback(p_border_left, p_base_scale));
		stylebox->set_border_width(SIDE_TOP, scale_theme_fallback(p_border_top, p_base_scale));
		stylebox->set_border_width(SIDE_RIGHT, scale_theme_fallback(p_border_right, p_base_scale));
		stylebox->set_border_width(SIDE_BOTTOM, scale_theme_fallback(p_border_bottom, p_base_scale));
		stylebox->set_content_margin(SIDE_LEFT, scale_theme_fallback(p_margin_left, p_base_scale));
		stylebox->set_content_margin(SIDE_TOP, scale_theme_fallback(p_margin_top, p_base_scale));
		stylebox->set_content_margin(SIDE_RIGHT, scale_theme_fallback(p_margin_right, p_base_scale));
		stylebox->set_content_margin(SIDE_BOTTOM, scale_theme_fallback(p_margin_bottom, p_base_scale));
		stylebox->set_corner_radius_all(scale_theme_fallback(p_corner_radius, p_base_scale));
		stylebox->set_draw_center(p_draw_center);
		stylebox->set_corner_detail(p_corner_detail);
		return stylebox;
	}

	static float get_stylebox_margin_or(const Ref<StyleBox>& p_stylebox, Side p_side, float p_default) {
		if (p_stylebox.is_valid()) {
			return p_stylebox->get_margin(p_side);
		}

		return p_default;
	}

	struct EditorColorPair {
		uint32_t from_rgb;
		uint32_t to_rgb;
	};

	struct EditorLightThemeInfo {
		bool enabled = false;
		Ref<Theme> editor_theme;
		Color error_color = Color(1.0f, 0.161f, 0.161f, 1.0f);
		Color success_color = Color(0.0f, 0.6f, 0.2f, 1.0f);
		Color warning_color = Color(0.84f, 0.55f, 0.04f, 1.0f);
	};

	static uint32_t color_to_rgb_key(const Color& p_color) {
		return (static_cast<uint32_t>(p_color.get_r8()) << 16) | (static_cast<uint32_t>(p_color.get_g8()) << 8) | static_cast<uint32_t>(p_color.get_b8());
	}

	static Color color_from_rgb_key(uint32_t p_rgb, float p_alpha) {
		const float red = static_cast<float>((p_rgb >> 16) & 0xff) / 255.0f;
		const float green = static_cast<float>((p_rgb >> 8) & 0xff) / 255.0f;
		const float blue = static_cast<float>(p_rgb & 0xff) / 255.0f;
		return Color(red, green, blue, p_alpha);
	}

	static Color get_editor_theme_color_or(const Ref<Theme>& p_theme, const StringName& p_name, const Color& p_default) {
		if (p_theme.is_valid() && p_theme->has_color(p_name, "Editor")) {
			return p_theme->get_color(p_name, "Editor");
		}

		return p_default;
	}

	static bool is_light_editor_base_color(const Color& p_base_color) {
		return p_base_color.get_luminance() >= 0.5f;
	}

	static EditorLightThemeInfo get_editor_light_theme_info(Control* p_owner) {
		EditorLightThemeInfo info;
		Engine* engine = Engine::get_singleton();
		EditorInterface* editor_interface = engine != nullptr && engine->is_editor_hint() ? EditorInterface::get_singleton() : nullptr;
		if (editor_interface != nullptr) {
			info.editor_theme = editor_interface->get_editor_theme();
			if (info.editor_theme.is_valid()) {
				info.error_color = get_editor_theme_color_or(info.editor_theme, "error_color", info.error_color);
				info.success_color = get_editor_theme_color_or(info.editor_theme, "success_color", info.success_color);
				info.warning_color = get_editor_theme_color_or(info.editor_theme, "warning_color", info.warning_color);
				if (info.editor_theme->has_color("base_color", "Editor")) {
					info.enabled = is_light_editor_base_color(info.editor_theme->get_color("base_color", "Editor"));
					return info;
				}
				if (info.editor_theme->has_color("font_color", "Editor")) {
					info.enabled = info.editor_theme->get_color("font_color", "Editor").get_luminance() < 0.5f;
					return info;
				}
			}

			const Ref<EditorSettings> editor_settings = editor_interface->get_editor_settings();
			if (editor_settings.is_valid() && editor_settings->has_setting("interface/theme/base_color")) {
				const Variant base_color_value = editor_settings->get_setting("interface/theme/base_color");
				if (base_color_value.get_type() == Variant::COLOR) {
					info.enabled = is_light_editor_base_color(base_color_value);
					return info;
				}
			}
		}

		if (p_owner != nullptr && p_owner->has_theme_color("base_color", "Editor")) {
			info.enabled = is_light_editor_base_color(p_owner->get_theme_color("base_color", "Editor"));
		}

		return info;
	}

	static Color convert_editor_light_color(const Color& p_color, const EditorLightThemeInfo& p_theme_info) {
		if (!p_theme_info.enabled || p_color.a <= 0.0f) {
			return p_color;
		}

		const uint32_t rgb = color_to_rgb_key(p_color);
		if (rgb == 0xff5f5f) {
			return Color(p_theme_info.error_color.r, p_theme_info.error_color.g, p_theme_info.error_color.b, p_color.a);
		}
		if (rgb == 0x5fff97) {
			return Color(p_theme_info.success_color.r, p_theme_info.success_color.g, p_theme_info.success_color.b, p_color.a);
		}
		if (rgb == 0xffdd65) {
			return Color(p_theme_info.warning_color.r, p_theme_info.warning_color.g, p_theme_info.warning_color.b, p_color.a);
		}

		static constexpr EditorColorPair editor_color_map[] = {
			{ 0x478cbf, 0x478cbf },
			{ 0x414042, 0x414042 },
			{ 0xffffff, 0x414141 },
			{ 0xfefefe, 0xfefefe },
			{ 0x000000, 0xbfbfbf },
			{ 0x010101, 0x010101 },
			{ 0xff0000, 0xff0000 },
			{ 0x00ff00, 0x00ff00 },
			{ 0x0000ff, 0x0000ff },
			{ 0xe0e0e0, 0x5a5a5a },
			{ 0x808080, 0x808080 },
			{ 0xb3b3b3, 0x363636 },
			{ 0x699ce8, 0x699ce8 },
			{ 0xf9f9f9, 0x606060 },
			{ 0x5fb2ff, 0x0079f0 },
			{ 0x003e7a, 0x2b74bb },
			{ 0xf7f5cf, 0x615f3a },
			{ 0xc38ef1, 0xa85de9 },
			{ 0x8da5f3, 0x3d64dd },
			{ 0x7582a8, 0x6d83c8 },
			{ 0xfc7f7f, 0xcd3838 },
			{ 0xb56d6d, 0xbe6a6a },
			{ 0x99c4ff, 0x4589e6 },
			{ 0x869ebf, 0x7097cd },
			{ 0xffa6bd, 0xe65c7f },
			{ 0xbf909c, 0xcd8b9c },
			{ 0x8eef97, 0x2fa139 },
			{ 0x76ad7b, 0x64a66a },
			{ 0xf0caa0, 0x844b0e },
			{ 0xff4545, 0xff2929 },
			{ 0xffe345, 0xffe337 },
			{ 0x80ff45, 0x74ff34 },
			{ 0x45ffa2, 0x2cff98 },
			{ 0x45d7ff, 0x22ccff },
			{ 0x8045ff, 0x702aff },
			{ 0xff4596, 0xff2781 },
			{ 0xe1da5b, 0xd6cf4b },
			{ 0x62aeff, 0x1678e0 },
			{ 0x75d1e6, 0x41acc5 },
			{ 0x84ffee, 0x49ccba },
			{ 0xf70000, 0xc91616 },
			{ 0xeec315, 0xd58c0b },
			{ 0xdbee15, 0xb7d10a },
			{ 0x288027, 0x218309 },
			{ 0xffca5f, 0xfea900 },
			{ 0x2998ff, 0x68b6ff },
			{ 0xa2d2ff, 0x4998e3 },
			{ 0x69c4d4, 0x29a3cc },
			{ 0xea7940, 0xbd5e2c },
			{ 0xff2b88, 0xbd165f },
			{ 0xeac840, 0xbd9d1f },
			{ 0x3cf34e, 0x16a827 },
			{ 0x2877f6, 0x236be6 },
			{ 0xeae440, 0x9f9722 },
			{ 0xa448f0, 0x9853ce },
			{ 0x5ad5c4, 0x0a9c88 },
			{ 0xd6d6d6, 0x474747 },
			{ 0x474747, 0xd6d6d6 },
			{ 0x919191, 0x6e6e6e },
			{ 0xfce00e, 0xaa8d24 },
			{ 0x0e71fc, 0x0350bd },
			{ 0xc6ced4, 0x828f9b },
			{ 0x41ecad, 0x25e3a0 },
			{ 0x6f91f0, 0x6d8eeb },
			{ 0x5abbef, 0x4fb2e9 },
			{ 0x35d4f4, 0x27ccf0 },
			{ 0x4593ec, 0x4690e7 },
			{ 0xee5677, 0xee7991 },
			{ 0xe1ec41, 0xb2bb19 },
			{ 0x54ed9e, 0x57e99f },
			{ 0x417aec, 0x6993ec },
			{ 0x55f3e3, 0x12d5c3 },
			{ 0xf74949, 0xf77070 },
			{ 0x44bd44, 0x46b946 },
			{ 0xec418e, 0xec69a3 },
			{ 0xf1738f, 0xee758e },
			{ 0x41ec80, 0x2ce573 },
			{ 0xb9ec41, 0x96ce1a },
			{ 0xf68f45, 0xf49047 },
			{ 0xac73f1, 0xad76ee },
			{ 0xde66f0, 0xdc6aed },
			{ 0xf066bd, 0xed6abd },
			{ 0x77ce57, 0x67c046 },
			{ 0xea686c, 0xd95256 },
			{ 0xeac968, 0xd9b64f },
			{ 0xcf68ea, 0xc050dd },
		};

		for (const EditorColorPair& pair : editor_color_map) {
			if (rgb == pair.from_rgb) {
				return color_from_rgb_key(pair.to_rgb, p_color.a);
			}
		}

		const float max_channel = std::max<float>(p_color.r, std::max<float>(p_color.g, p_color.b));
		const float min_channel = std::min<float>(p_color.r, std::min<float>(p_color.g, p_color.b));
		const float luminance = p_color.get_luminance();
		if (max_channel - min_channel <= 0.025f && luminance > 0.64f && rgb != 0xfefefe) {
			const float weight = CLAMP((luminance - 0.64f) / 0.36f, 0.0f, 1.0f);
			const float gray = Math::lerp(0.52f, 0.28f, weight);
			return Color(gray, gray, gray, p_color.a);
		}

		return p_color;
	}

	static Ref<StyleBox> convert_editor_light_stylebox(const Ref<StyleBox>& p_stylebox, const EditorLightThemeInfo& p_theme_info) {
		if (!p_theme_info.enabled || p_stylebox.is_null()) {
			return p_stylebox;
		}

		const Ref<StyleBoxFlat> source_flat = p_stylebox;
		if (source_flat.is_valid()) {
			Ref<StyleBoxFlat> converted;
			converted.instantiate();
			converted->set_bg_color(convert_editor_light_color(source_flat->get_bg_color(), p_theme_info));
			converted->set_border_color(convert_editor_light_color(source_flat->get_border_color(), p_theme_info));
			converted->set_border_blend(source_flat->get_border_blend());
			converted->set_draw_center(source_flat->is_draw_center_enabled());
			converted->set_skew(source_flat->get_skew());
			converted->set_shadow_color(convert_editor_light_color(source_flat->get_shadow_color(), p_theme_info));
			converted->set_shadow_size(source_flat->get_shadow_size());
			converted->set_shadow_offset(source_flat->get_shadow_offset());
			converted->set_anti_aliased(source_flat->is_anti_aliased());
			converted->set_aa_size(source_flat->get_aa_size());
			converted->set_corner_detail(source_flat->get_corner_detail());

			for (int32_t i = 0; i < 4; i++) {
				const Side side = static_cast<Side>(i);
				converted->set_content_margin(side, source_flat->get_content_margin(side));
				converted->set_border_width(side, source_flat->get_border_width(side));
				converted->set_expand_margin(side, source_flat->get_expand_margin(side));
			}
			for (int32_t i = 0; i < 4; i++) {
				const Corner corner = static_cast<Corner>(i);
				converted->set_corner_radius(corner, source_flat->get_corner_radius(corner));
			}

			return converted;
		}

		const Ref<StyleBoxLine> source_line = p_stylebox;
		if (source_line.is_valid()) {
			Ref<StyleBoxLine> converted;
			converted.instantiate();
			converted->set_color(convert_editor_light_color(source_line->get_color(), p_theme_info));
			converted->set_thickness(source_line->get_thickness());
			converted->set_grow_begin(source_line->get_grow_begin());
			converted->set_grow_end(source_line->get_grow_end());
			converted->set_vertical(source_line->is_vertical());

			for (int32_t i = 0; i < 4; i++) {
				const Side side = static_cast<Side>(i);
				converted->set_content_margin(side, source_line->get_content_margin(side));
			}

			return converted;
		}

		return p_stylebox;
	}

	static void apply_editor_light_theme(MarkdownThemeCache& p_cache, const EditorLightThemeInfo& p_theme_info) {
		if (!p_theme_info.enabled) {
			return;
		}

		// 统一在主题缓存阶段转换，绘制代码不用关心编辑器亮暗主题。
		p_cache.color.text = convert_editor_light_color(p_cache.color.text, p_theme_info);
		p_cache.color.link = convert_editor_light_color(p_cache.color.link, p_theme_info);
		p_cache.color.code_text = convert_editor_light_color(p_cache.color.code_text, p_theme_info);
		p_cache.color.code_block = convert_editor_light_color(p_cache.color.code_block, p_theme_info);
		p_cache.color.code_block_background = convert_editor_light_color(p_cache.color.code_block_background, p_theme_info);
		p_cache.color.highlight = convert_editor_light_color(p_cache.color.highlight, p_theme_info);
		p_cache.color.highlight_font = convert_editor_light_color(p_cache.color.highlight_font, p_theme_info);
		p_cache.color.font_outline = convert_editor_light_color(p_cache.color.font_outline, p_theme_info);
		p_cache.color.font_shadow = convert_editor_light_color(p_cache.color.font_shadow, p_theme_info);
		p_cache.color.strikethrough = convert_editor_light_color(p_cache.color.strikethrough, p_theme_info);
		p_cache.color.selection_color = convert_editor_light_color(p_cache.color.selection_color, p_theme_info);
		p_cache.color.list_marker = convert_editor_light_color(p_cache.color.list_marker, p_theme_info);
		p_cache.color.table_header_font = convert_editor_light_color(p_cache.color.table_header_font, p_theme_info);
		p_cache.color.table_cell_font = convert_editor_light_color(p_cache.color.table_cell_font, p_theme_info);
		p_cache.color.footnote_ref = convert_editor_light_color(p_cache.color.footnote_ref, p_theme_info);
		p_cache.color.footnote_text = convert_editor_light_color(p_cache.color.footnote_text, p_theme_info);
		for (int32_t i = 0; i < 6; i++) {
			p_cache.color.heading[i] = convert_editor_light_color(p_cache.color.heading[i], p_theme_info);
		}

		p_cache.stylebox.inline_code = convert_editor_light_stylebox(p_cache.stylebox.inline_code, p_theme_info);
		p_cache.stylebox.code_block_panel = convert_editor_light_stylebox(p_cache.stylebox.code_block_panel, p_theme_info);
		p_cache.stylebox.blockquote = convert_editor_light_stylebox(p_cache.stylebox.blockquote, p_theme_info);
		p_cache.stylebox.blockquote_nested = convert_editor_light_stylebox(p_cache.stylebox.blockquote_nested, p_theme_info);
		p_cache.stylebox.selection = convert_editor_light_stylebox(p_cache.stylebox.selection, p_theme_info);
		p_cache.stylebox.search_result = convert_editor_light_stylebox(p_cache.stylebox.search_result, p_theme_info);
		p_cache.stylebox.separator = convert_editor_light_stylebox(p_cache.stylebox.separator, p_theme_info);
		p_cache.stylebox.highlight = convert_editor_light_stylebox(p_cache.stylebox.highlight, p_theme_info);
		p_cache.stylebox.table_panel = convert_editor_light_stylebox(p_cache.stylebox.table_panel, p_theme_info);
		p_cache.stylebox.table_header_panel = convert_editor_light_stylebox(p_cache.stylebox.table_header_panel, p_theme_info);
		p_cache.stylebox.table_cell_panel = convert_editor_light_stylebox(p_cache.stylebox.table_cell_panel, p_theme_info);
		p_cache.stylebox.table_striped_panel = convert_editor_light_stylebox(p_cache.stylebox.table_striped_panel, p_theme_info);
		p_cache.stylebox.footnote_ref = convert_editor_light_stylebox(p_cache.stylebox.footnote_ref, p_theme_info);
	}

	static Ref<Font> first_valid_font(const Ref<Font>& p_primary, const Ref<Font>& p_secondary, const Ref<Font>& p_fallback) {
		if (p_primary.is_valid()) {
			return p_primary;
		}
		if (p_secondary.is_valid()) {
			return p_secondary;
		}

		return p_fallback;
	}

	static Ref<Font> get_span_font(const MarkdownInlineSpan& p_span, const MarkdownThemeCache& p_cache, const Ref<Font>& p_normal_font) {
		if (p_span.code) {
			return p_cache.font.code.is_valid() ? p_cache.font.code : p_normal_font;
		}
		if (p_span.bold && p_span.italic) {
			return first_valid_font(p_cache.font.bold_italic, Ref<Font>(), p_normal_font);
		}
		if (p_span.bold) {
			return first_valid_font(p_cache.font.bold, Ref<Font>(), p_normal_font);
		}
		if (p_span.italic) {
			return first_valid_font(p_cache.font.italic, Ref<Font>(), p_normal_font);
		}

		return p_normal_font;
	}


	enum MarkdownImageLoadStatus {
		MARKDOWN_IMAGE_READY,
		MARKDOWN_IMAGE_LOADING,
		MARKDOWN_IMAGE_FAILED,
	};

	struct MarkdownImageCacheEntry {
		MarkdownImageLoadStatus status = MARKDOWN_IMAGE_FAILED;
		Ref<Texture2D> texture;
		String error;
		double gif_elapsed = 0.0;
	};

	static HashMap<String, MarkdownImageCacheEntry> s_image_cache;
	static HashMap<String, std::vector<uint64_t>> s_image_watchers;
	static std::vector<String> s_pending_local_image_uris;
	static int32_t s_live_image_canvas_count = 0;

	static String image_uri_without_query(const String& p_uri) {
		String clean_uri = p_uri;
		const int64_t query_pos = clean_uri.find("?");
		if (query_pos >= 0) {
			clean_uri = clean_uri.substr(0, query_pos);
		}
		const int64_t hash_pos = clean_uri.find("#");
		if (hash_pos >= 0) {
			clean_uri = clean_uri.substr(0, hash_pos);
		}
		return clean_uri;
	}

	static bool is_gif_image_uri(const String& p_uri) {
		return image_uri_without_query(p_uri).to_lower().ends_with(".gif");
	}

	static bool has_gif_plugin_support() {
		static int32_t gif_support = -1;
		if (gif_support < 0) {
			gif_support = (ClassDB::class_exists("GIFTexture") || ClassDB::class_exists("GIFReader")) ? 1 : 0;
		}
		return gif_support == 1;
	}

	static void register_image_watcher(const String& p_uri, MarkdownLabelCanvas* p_canvas) {
		if (p_canvas == nullptr) {
			return;
		}

		std::vector<uint64_t>& watchers = s_image_watchers[p_uri];
		const uint64_t instance_id = p_canvas->get_instance_id();
		if (std::find(watchers.begin(), watchers.end(), instance_id) == watchers.end()) {
			watchers.push_back(instance_id);
		}
	}

	static void notify_image_watchers(const String& p_uri) {
		if (!s_image_watchers.has(p_uri)) {
			return;
		}

		std::vector<uint64_t> watchers = s_image_watchers[p_uri];
		s_image_watchers.erase(p_uri);
		for (uint64_t instance_id : watchers) {
			Object* object = ObjectDB::get_instance(instance_id);
			MarkdownLabelCanvas* canvas = Object::cast_to<MarkdownLabelCanvas>(object);
			if (canvas != nullptr) {
				canvas->refresh_image_content();
			}
		}
	}

	static bool has_pending_local_image_uri(const String& p_uri) {
		return std::find(s_pending_local_image_uris.begin(), s_pending_local_image_uris.end(), p_uri) != s_pending_local_image_uris.end();
	}

	static void add_pending_local_image_uri(const String& p_uri) {
		if (!has_pending_local_image_uri(p_uri)) {
			s_pending_local_image_uris.push_back(p_uri);
		}
	}

	static bool is_animated_gif_texture(const Ref<Texture2D>& p_texture) {
		if (p_texture.is_null()) {
			return false;
		}

		Object* texture_object = p_texture.ptr();
		if (texture_object == nullptr || !texture_object->has_method("get_frame_count") || !texture_object->has_method("get_frame") || !texture_object->has_method("set_frame") || !texture_object->has_method("get_frame_delay")) {
			return false;
		}

		return static_cast<int32_t>(texture_object->call("get_frame_count")) > 1;
	}

	static MarkdownImageCacheEntry load_image_texture(const String& p_uri, MarkdownLabelCanvas* p_canvas) {
		if (p_uri.is_empty()) {
			MarkdownImageCacheEntry entry;
			entry.status = MARKDOWN_IMAGE_FAILED;
			entry.error = "empty_uri";
			return entry;
		}

		if (dockit::is_remote_image_url(p_uri)) {
			const auto key = dockit::remote_image_request_url(p_uri);
			if (const auto* cached = s_image_cache.getptr(key)) return *cached;
			const auto callback = p_canvas ?
				callable_mp_static(&MarkdownLabelCanvas::remote_image_completed).bind(p_canvas->get_instance_id()) : Callable();
			const auto image = dockit::request_remote_image(p_uri, callback);
			using Status = dockit::RemoteImageStatus;
			MarkdownImageCacheEntry entry;
			entry.status = image.status == Status::ready ? MARKDOWN_IMAGE_READY :
				image.status == Status::loading ? MARKDOWN_IMAGE_LOADING : MARKDOWN_IMAGE_FAILED;
			entry.texture = image.texture;
			entry.error = image.error;
			if (entry.texture.is_valid()) s_image_cache.insert(key, entry);
			return entry;
		}

		if (s_image_cache.has(p_uri)) {
			MarkdownImageCacheEntry entry = s_image_cache[p_uri];
			if (entry.status == MARKDOWN_IMAGE_LOADING) {
				register_image_watcher(p_uri, p_canvas);
			}
			return entry;
		}

		if (p_uri.begins_with("uid://")) {
			ResourceUID* resource_uid = ResourceUID::get_singleton();
			if (!resource_uid->has_id(resource_uid->text_to_id(p_uri))) {
				// Editor imports can register the UID after this render.
				MarkdownImageCacheEntry entry;
				entry.status = MARKDOWN_IMAGE_FAILED;
				entry.error = "unknown_resource_uid";
				return entry;
			}
		}

		if (p_uri.begins_with("user://") || p_uri.is_absolute_path()) {
			MarkdownImageCacheEntry entry;
			entry.texture = dockit::load_html_image_file(p_uri);
			entry.status = entry.texture.is_valid() ? MARKDOWN_IMAGE_READY : MARKDOWN_IMAGE_FAILED;
			if (entry.texture.is_valid()) s_image_cache[p_uri] = entry;
			return entry;
		}


		MarkdownImageCacheEntry entry;
		if (is_gif_image_uri(p_uri) && !has_gif_plugin_support()) {
			entry.status = MARKDOWN_IMAGE_FAILED;
			entry.error = "gif_plugin_missing";
			s_image_cache[p_uri] = entry;
			return entry;
		}

		entry.status = MARKDOWN_IMAGE_LOADING;
		s_image_cache[p_uri] = entry;
		register_image_watcher(p_uri, p_canvas);
		if (p_canvas != nullptr) {
			p_canvas->request_local_image(p_uri);
		}
		return entry;
	}

	static Vector2 fit_texture_size(const Ref<Texture2D>& p_texture, float p_max_width,
		const Vector2i& p_requested_size = {}, float p_max_height = std::numeric_limits<float>::max()) {
		if (p_texture.is_null() || p_texture->get_width() <= 0 || p_texture->get_height() <= 0) {
			return Vector2(1.0f, 1.0f);
		}

		Vector2 size = p_texture->get_size();
		const float aspect = size.x / size.y;
		if (p_requested_size.x > 0) {
			size = Vector2(p_requested_size.x, p_requested_size.x / aspect);
		}
		if (p_requested_size.y > 0) {
			size = Vector2(p_requested_size.x > 0 ? size.x : p_requested_size.y * aspect, p_requested_size.y);
		}
		const float max_width = std::max<float>(1.0f, p_max_width);
		const float scale = std::min({ 1.0f, max_width / size.x, p_max_height / size.y });
		return (size * scale).max(Vector2(1.0f, 1.0f));
	}

	static bool is_mermaid_block(const MarkdownBlock& p_block) {
		return p_block.type == MARKDOWN_BLOCK_CODE && p_block.end_line > 0 &&
			p_block.argument.to_lower() == "mermaid" && !p_block.text.is_empty();
	}

	static void collect_mermaid_sources(const std::vector<MarkdownBlock>& p_blocks, HashSet<String>& r_sources) {
		for (const MarkdownBlock& block : p_blocks) {
			if (is_mermaid_block(block)) {
				r_sources.insert(block.text);
			}
			if (!block.children.empty()) {
				collect_mermaid_sources(block.children, r_sources);
			}
			for (const MarkdownListItem& item : block.list_items) {
				collect_mermaid_sources(item.children, r_sources);
			}
		}
	}

	static bool add_mermaid_to_paragraph(const Ref<TextParagraph>& p_paragraph, const MermaidRenderCache& p_cache, const MarkdownBlock& p_block, float p_width, MarkdownParagraphObjects& r_objects) {
		const MermaidRenderCache::Result* result = is_mermaid_block(p_block) ? p_cache.get_result(p_block.text) : nullptr;
		if (result == nullptr || result->texture.is_null()) {
			return false;
		}
		const String key = "mermaid";
		const float render_scale = MermaidRenderCache::RASTER_SCALE;
		const Vector2 size = fit_texture_size(result->texture, p_width * render_scale) / render_scale;
		if (!p_paragraph->add_object(key, size, INLINE_ALIGNMENT_CENTER, p_block.text.length())) {
			return false;
		}
		r_objects.images.insert(key, result->texture);
		p_paragraph->set_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		return true;
	}

	static BitField<TextServer::LineBreakFlag> get_autowrap_trim_mask() {
		return TextServer::BREAK_TRIM_INDENT | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	}

	static BitField<TextServer::LineBreakFlag> get_autowrap_break_flags(const MarkdownLabel* p_label, bool p_preserve_whitespace = false) {
		const TextServer::AutowrapMode mode = p_label == nullptr ? TextServer::AUTOWRAP_WORD_SMART : p_label->get_autowrap_mode();
		BitField<TextServer::LineBreakFlag> flags = TextServer::BREAK_MANDATORY;

		switch (mode) {
		case TextServer::AUTOWRAP_WORD_SMART:
			flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_ADAPTIVE | TextServer::BREAK_MANDATORY;
			break;
		case TextServer::AUTOWRAP_WORD:
			flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_MANDATORY;
			break;
		case TextServer::AUTOWRAP_ARBITRARY:
			flags = TextServer::BREAK_GRAPHEME_BOUND | TextServer::BREAK_MANDATORY;
			break;
		case TextServer::AUTOWRAP_OFF:
			break;
		}

		if (p_preserve_whitespace) {
			return flags;
		}

		const BitField<TextServer::LineBreakFlag> trim_flags = p_label == nullptr
			? BitField<TextServer::LineBreakFlag>(TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES)
			: p_label->get_autowrap_trim_flags();
		return flags | trim_flags;
	}

	static void configure_paragraph_autowrap(const Ref<TextParagraph>& p_paragraph, const MarkdownLabel* p_label, bool p_preserve_whitespace = false) {
		if (p_paragraph.is_null()) {
			return;
		}

		p_paragraph->set_break_flags(get_autowrap_break_flags(p_label, p_preserve_whitespace));
	}

	static void configure_code_block_tabs(const Ref<TextParagraph>& p_paragraph, const Ref<Font>& p_font, int32_t p_font_size) {
		if (p_paragraph.is_null() || p_font.is_null()) {
			return;
		}

		const float space_advance = p_font->get_char_size(' ', std::max<int32_t>(1, p_font_size)).x + static_cast<float>(p_font->get_spacing(TextServer::SPACING_SPACE));
		PackedFloat32Array tab_stops;
		tab_stops.push_back(std::max<float>(1.0f, 4.0f * space_advance));
		p_paragraph->tab_align(tab_stops);
	}

	static Color get_span_color(const MarkdownInlineSpan& p_span, const MarkdownThemeCache& p_cache, const Color& p_default_color) {
		if (p_span.custom_color) {
			return p_span.color;
		}
		if (!p_span.link_uri.is_empty()) {
			return p_cache.color.link;
		}
		if (p_span.footnote_ref) {
			return p_cache.color.footnote_ref;
		}
		if (p_span.highlight) {
			return p_cache.color.highlight_font;
		}
		if (p_span.code) {
			return p_cache.color.code_text;
		}
		return p_default_color;
	}

	static MarkdownInlineSpan make_display_math_span(const MarkdownBlock& p_block) {
		MarkdownInlineSpan span;
		span.text = p_block.text;
		span.latex = p_block.argument;
		span.end = span.text.length();
		span.display_math = true;
		return span;
	}

	static bool add_formula_to_paragraph(const Ref<TextParagraph>& p_paragraph, const MarkdownInlineSpan& p_span, int32_t p_font_size, const Color& p_color, float p_max_width, MarkdownParagraphObjects& r_objects) {
		Ref<TeXRender> formula;
		formula.instantiate();
		const TeXRender::Style style = p_span.display_math ? TeXRender::STYLE_DISPLAY : TeXRender::STYLE_TEXT;
		if (formula->parse(p_span.latex, p_font_size, style, p_color) != OK || !formula->is_valid()) {
			return false;
		}
		formula->set_insets(Vector4i(), true);
		if (formula->get_width() > p_max_width) {
			formula->set_text_size(p_font_size * p_max_width / formula->get_width());
		}
		const Vector2 size = Vector2(formula->get_size()).max(Vector2(1.0f, 1.0f));
		const String key = String("latex_") + String::num_int64(r_objects.formulas.size());
		const InlineAlignment alignment = static_cast<InlineAlignment>(INLINE_ALIGNMENT_BASELINE_TO | INLINE_ALIGNMENT_TO_BASELINE);
		const int32_t object_length = static_cast<int32_t>(p_span.end - p_span.start);
		if (!p_paragraph->add_object(key, size, alignment, object_length, formula->get_baseline_offset())) {
			return false;
		}
		r_objects.formulas.insert(key, formula);
		return true;
	}

	static void add_spans_to_paragraph(const Ref<TextParagraph>& p_paragraph, const String& p_text, const std::vector<MarkdownInlineSpan>& p_spans, const MarkdownThemeCache& p_cache, const Ref<Font>& p_normal_font, int32_t p_font_size, const Color& p_default_color, MarkdownParagraphObjects& r_objects, float p_max_width, MarkdownLabelCanvas* p_canvas) {
		int64_t pos = 0;
		int32_t image_object_index = 0;
		for (const MarkdownInlineSpan& span : p_spans) {
			if (span.start > pos) {
				p_paragraph->add_string(p_text.substr(pos, span.start - pos), p_normal_font, std::max<int32_t>(1, p_font_size));
			}
			if (!span.latex.is_empty() && add_formula_to_paragraph(p_paragraph, span, p_font_size, get_span_color(span, p_cache, p_default_color), p_max_width, r_objects)) {
				pos = span.end;
				continue;
			}
			if (span.html_image || !span.image_uri.is_empty()) {
				const String image_uri = p_canvas->resolve_image_uri(span.image_uri, span.html_image);
				MarkdownImageCacheEntry image_entry = load_image_texture(image_uri, p_canvas);
				const bool image_ready = image_entry.status == MARKDOWN_IMAGE_READY;
				const bool image_failed = image_entry.status == MARKDOWN_IMAGE_FAILED;
				Ref<Texture2D> tex = image_ready ? image_entry.texture : (image_failed ? p_cache.icon.file_broken : Ref<Texture2D>());
				if (span.html_image && image_failed && span.text != String::chr(0xfffc)) {
					const Ref<Font> font = get_span_font(span, p_cache, p_normal_font);
					p_paragraph->add_string(span.text, font, std::max<int32_t>(1, p_font_size));
					pos = span.end;
					continue;
				}
				if (tex.is_valid() || image_entry.status == MARKDOWN_IMAGE_LOADING) {
					String name = String("img_") + String::num_int64(image_object_index++);
					if (tex.is_valid()) {
						r_objects.images[name] = tex;
					}
					int32_t object_length = std::max<int32_t>(1, static_cast<int32_t>(span.end - span.start));
					const int32_t viewport_height = p_canvas->get_layout_height();
					const float max_height = viewport_height > 0 ? viewport_height : std::numeric_limits<float>::max();
					const float placeholder_size = static_cast<float>(std::max<int32_t>(1, p_font_size));
					Vector2 object_size = image_ready ? fit_texture_size(tex, p_max_width, span.image_size, max_height) :
						Vector2(std::min(p_max_width, placeholder_size), std::min(max_height, placeholder_size));
					if (!image_ready) {
						if (!span.html_image) {
							object_length = 1;
						}
					}
					p_paragraph->add_object(name, object_size, INLINE_ALIGNMENT_CENTER, object_length);
					const String object_placeholder = String::chr(0xfffc);
					if (image_failed && !span.text.is_empty() && span.text != object_placeholder) {
						MarkdownInlineSpan fallback_span = span;
						fallback_span.italic = true;
						const Ref<Font> font = get_span_font(fallback_span, p_cache, p_normal_font);
						p_paragraph->add_string(" " + span.text, font, std::max<int32_t>(1, p_font_size));
					}
				}
				else if (image_entry.status == MARKDOWN_IMAGE_FAILED && !span.text.is_empty() && span.text != String::chr(0xfffc)) {
					MarkdownInlineSpan fallback_span = span;
					fallback_span.italic = true;
					const Ref<Font> font = get_span_font(fallback_span, p_cache, p_normal_font);
					p_paragraph->add_string(span.text, font, std::max<int32_t>(1, p_font_size));
				}
			}
			else {
				const Ref<Font> font = get_span_font(span, p_cache, p_normal_font);
				const int32_t font_size = span.footnote_ref ? p_cache.font_size.footnote_ref : (span.code ? p_cache.font_size.code : p_font_size);
				const Ref<Font> actual_font = span.footnote_ref && p_cache.font.footnote_ref.is_valid() ? p_cache.font.footnote_ref : font;
				p_paragraph->add_string(span.text, actual_font, std::max<int32_t>(1, font_size));
			}
			pos = span.end;
		}
		if (pos < static_cast<int64_t>(p_text.length())) {
			p_paragraph->add_string(p_text.substr(pos), p_normal_font, std::max<int32_t>(1, p_font_size));
		}
	}

	static Vector2 get_inline_code_vertical_padding(const std::vector<MarkdownInlineSpan>& p_spans, const MarkdownThemeCache& p_cache) {
		bool has_code_span = false;
		for (const MarkdownInlineSpan& span : p_spans) {
			if (span.code) {
				has_code_span = true;
				break;
			}
		}
		if (!has_code_span) {
			return Vector2();
		}

		const float margin_top = get_stylebox_margin_or(p_cache.stylebox.inline_code, SIDE_TOP, 1.0f);
		const float margin_bottom = get_stylebox_margin_or(p_cache.stylebox.inline_code, SIDE_BOTTOM, 1.0f);
		return Vector2(std::max<float>(0.0f, margin_top), std::max<float>(0.0f, margin_bottom));
	}

	static bool is_search_word_character(char32_t p_character) {
		return (p_character >= 'a' && p_character <= 'z') || (p_character >= 'A' && p_character <= 'Z') || (p_character >= '0' && p_character <= '9') || p_character == '_';
	}
	static bool is_whole_word_match(const String& p_text, int64_t p_start, int64_t p_length) {
		if (p_start > 0 && is_search_word_character(p_text[p_start - 1])) {
			return false;
		}

		const int64_t end = p_start + p_length;
		if (end < p_text.length() && is_search_word_character(p_text[end])) {
			return false;
		}

		return true;
	}

	static const char* block_spacing_type(const MarkdownBlock& p_block) {
		switch (p_block.type) {
		case MARKDOWN_BLOCK_HEADING:
			return "heading";
		case MARKDOWN_BLOCK_CODE:
			return "code_block";
		case MARKDOWN_BLOCK_TABLE:
			return "table";
		case MARKDOWN_BLOCK_FOOTNOTES:
			return "footnotes";
		case MARKDOWN_BLOCK_BLOCKQUOTE:
			return "blockquote";
		case MARKDOWN_BLOCK_TASK_LIST:
		case MARKDOWN_BLOCK_UNORDERED_LIST:
		case MARKDOWN_BLOCK_ORDERED_LIST:
			return "list";
		default:
			return "text";
		}
	}

} // !namespace

void MarkdownThemeCache::build(Control* p_owner, int32_t p_extra_font_size) {
	base_scale = get_theme_base_scale_or_one(p_owner);
	const int32_t default_font_size = std::max<int32_t>(1, p_owner == nullptr ? scale_theme_fallback(16, base_scale) : p_owner->get_theme_default_font_size());
	const auto scale_font_size_from_text = [&](int32_t p_base_font_size) -> int32_t {
		return std::max<int32_t>(1, static_cast<int32_t>(Math::round(static_cast<double>(default_font_size) * static_cast<double>(p_base_font_size) / 16.0)));
		};

	// Font sizes
	font_size.text = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "text_font_size", default_font_size) + p_extra_font_size);
	font_size.code = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "code_text_font_size", default_font_size) + p_extra_font_size);
	font_size.code_block = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "code_block_font_size", default_font_size) + p_extra_font_size);
	font_size.list_marker = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "list_marker_font_size", default_font_size) + p_extra_font_size);
	font_size.table_header = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "table_header_font_size", default_font_size) + p_extra_font_size);
	font_size.table_cell = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "table_cell_font_size", default_font_size) + p_extra_font_size);
	font_size.footnote_text = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "footnote_text_font_size", font_size.text) + p_extra_font_size);
	font_size.footnote_ref = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "footnote_ref_font_size", scale_font_size_from_text(12)) + p_extra_font_size);

	static const int32_t default_heading_sizes[] = { 36, 26, 20, 18, 12, 10 };
	for (int32_t i = 0; i < 6; i++) {
		const String name = String("h") + String::num_int64(i + 1) + "_font_size";
		font_size.heading[i] = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", StringName(name), scale_font_size_from_text(default_heading_sizes[i])) + p_extra_font_size);
	}

	// Constants
	constant.line_separation = get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "line_separation", 2, base_scale);
	constant.paragraph_separation = get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "paragraph_separation", 10, base_scale);
	constant.list_indent = std::max<int32_t>(0, get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "list_indent", 26, base_scale));
	constant.list_marker_gap = std::max<int32_t>(0, get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "list_marker_gap", 6, base_scale));
	constant.table_striped = std::max<int32_t>(0, get_theme_constant_or(p_owner, "MarkdownLabel", "table_striped", 1));
	constant.footnote_space_before = std::max<int32_t>(0, get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "footnote_space_before", 16, base_scale));
	constant.footnote_line_separation = std::max<int32_t>(0, get_theme_constant_or(p_owner, "MarkdownLabel", "footnote_line_separation", constant.line_separation));
	constant.outline_size = std::clamp(get_theme_constant_or(p_owner, "MarkdownLabel", "outline_size", 0), 0, 16);
	constant.shadow_outline_size = std::clamp(get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "shadow_outline_size", 1, base_scale), 0, 16);
	constant.shadow_offset_x = get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "shadow_offset_x", 1, base_scale);
	constant.shadow_offset_y = get_scaled_theme_constant_or(p_owner, "MarkdownLabel", "shadow_offset_y", 1, base_scale);

	// Fonts
	font.text = get_theme_font_or(p_owner, "MarkdownLabel", "text_font");
	font.bold = get_theme_font_or(p_owner, "MarkdownLabel", "bold_font");
	font.bold_italic = get_theme_font_or(p_owner, "MarkdownLabel", "bold_italic_font");
	if (!font.bold_italic.is_valid()) {
		font.bold_italic = font.text;
	}
	font.italic = get_theme_font_or(p_owner, "MarkdownLabel", "italic_font");
	if (!font.italic.is_valid()) {
		font.italic = font.text;
	}
	font.code = get_theme_font_or(p_owner, "MarkdownLabel", "code_text_font");
	if (!font.code.is_valid()) {
		font.code = font.text;
	}
	font.code_block = get_theme_font_or(p_owner, "MarkdownLabel", "code_block_font");
	if (!font.code_block.is_valid()) {
		font.code_block = font.code;
	}
	font.list_marker = get_theme_font_or(p_owner, "MarkdownLabel", "list_marker_font");
	if (!font.list_marker.is_valid()) {
		font.list_marker = font.text;
	}
	font.table_header = get_theme_font_or(p_owner, "MarkdownLabel", "table_header_font");
	if (!font.table_header.is_valid()) {
		font.table_header = font.text;
	}
	font.table_cell = get_theme_font_or(p_owner, "MarkdownLabel", "table_cell_font");
	if (!font.table_cell.is_valid()) {
		font.table_cell = font.text;
	}
	font.footnote_ref = get_theme_font_or(p_owner, "MarkdownLabel", "footnote_ref_font");
	if (!font.footnote_ref.is_valid()) {
		font.footnote_ref = font.text;
	}
	font.footnote_text = get_theme_font_or(p_owner, "MarkdownLabel", "footnote_text_font");
	if (!font.footnote_text.is_valid()) {
		font.footnote_text = font.text;
	}

	for (int32_t i = 0; i < 6; i++) {
		const String name = String("h") + String::num_int64(i + 1) + "_font";
		font.heading[i] = get_theme_font_or(p_owner, "MarkdownLabel", StringName(name));
		if (!font.heading[i].is_valid()) {
			font.heading[i] = font.text;
		}
	}

	// Colors
	color.text = get_theme_color_or(p_owner, "MarkdownLabel", "text_font_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	color.link = get_theme_color_or(p_owner, "MarkdownLabel", "link_color", Color(0.36f, 0.62f, 1.0f, 1.0f));
	color.selection_color = get_theme_color_or(p_owner, "MarkdownLabel", "selection_color", Color(0.102f, 0.102f, 1.0f, 0.8f));
	stylebox.selection = get_theme_stylebox_or(p_owner, "MarkdownLabel", "selection_stylebox");
	if (!stylebox.selection.is_valid()) {
		stylebox.selection = create_internal_stylebox(color.selection_color, Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, true, 8, base_scale);
	}
	color.code_text = get_theme_color_or(p_owner, "MarkdownLabel", "code_text_font_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	color.code_block = get_theme_color_or(p_owner, "MarkdownLabel", "code_block_font_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	color.code_block_background = get_theme_color_or(p_owner, "MarkdownLabel", "code_block_background_color", Color(0.08f, 0.09f, 0.11f, 1.0f));
	color.scrollbar_track = get_theme_stylebox_color_or(p_owner, "VScrollBar", "scroll", Color(0.1f, 0.1f, 0.1f, 0.6f));
	color.scrollbar_thumb = get_theme_stylebox_color_or(p_owner, "VScrollBar", "grabber", Color(1.0f, 1.0f, 1.0f, 0.4f));
	color.scrollbar_hover = get_theme_stylebox_color_or(p_owner, "VScrollBar", "grabber_highlight", Color(1.0f, 1.0f, 1.0f, 0.75f));
	color.scrollbar_pressed = get_theme_stylebox_color_or(p_owner, "VScrollBar", "grabber_pressed", Color(0.75f, 0.75f, 0.75f, 0.75f));
	color.highlight = get_theme_color_or(p_owner, "MarkdownLabel", "highlight_color", Color(1.0f, 0.92f, 0.35f, 0.5f));
	color.highlight_font = get_theme_color_or(p_owner, "MarkdownLabel", "highlight_font_color", Color(1.0f, 0.700f, 0.211f, 1.0f));
	color.font_outline = get_theme_color_or(p_owner, "MarkdownLabel", "font_outline_color", Color(0.0f, 0.0f, 0.0f, 1.0f));
	color.font_shadow = get_theme_color_or(p_owner, "MarkdownLabel", "font_shadow_color", Color(0.0f, 0.0f, 0.0f, 0.0f));
	color.strikethrough = get_theme_color_or(p_owner, "MarkdownLabel", "strikethrough_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	color.list_marker = get_theme_color_or(p_owner, "MarkdownLabel", "list_marker_color", color.text);
	color.table_header_font = get_theme_color_or(p_owner, "MarkdownLabel", "table_header_font_color", color.text);
	color.table_cell_font = get_theme_color_or(p_owner, "MarkdownLabel", "table_cell_font_color", color.text);
	color.footnote_ref = get_theme_color_or(p_owner, "MarkdownLabel", "footnote_ref_font_color", Color(0.809f, 0.809f, 0.809f, 1.0f));
	color.footnote_text = get_theme_color_or(p_owner, "MarkdownLabel", "footnote_text_font_color", Color(0.875f, 0.875f, 0.875f, 0.502f));

	for (int32_t i = 0; i < 6; i++) {
		const String name = String("h") + String::num_int64(i + 1) + "_font_color";
		color.heading[i] = get_theme_color_or(p_owner, "MarkdownLabel", StringName(name), color.text);
	}

	// StyleBoxes
	stylebox.inline_code = get_theme_stylebox_or(p_owner, "MarkdownLabel", "inline_code");
	if (!stylebox.inline_code.is_valid()) {
		stylebox.inline_code = create_internal_stylebox(Color(0.6f, 0.6f, 0.6f, 0.392f), Color(0, 0, 0, 0), 0, 0, 0, 0, 4.0f, 0.0f, 4.0f, 0.0f, 2, true, 8, base_scale);
	}

	Ref<StyleBoxEmpty> empty;
	empty.instantiate();
	stylebox.footnote_ref = empty;

	stylebox.code_block_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "code_block_panel");
	if (!stylebox.code_block_panel.is_valid()) {
		stylebox.code_block_panel = create_internal_stylebox(Color(0.0f, 0.0f, 0.0f, 0.6f), Color(0, 0, 0, 0), 0, 0, 0, 0, 8.0f, 8.0f, 8.0f, 8.0f, 3, true, 8, base_scale);
	}

	stylebox.blockquote = get_theme_stylebox_or(p_owner, "MarkdownLabel", "blockquote_panel");
	if (!stylebox.blockquote.is_valid()) {
		stylebox.blockquote = create_internal_stylebox(Color(0.0f, 0.0f, 0.0f, 0.196f), Color(0.458f, 0.458f, 0.458f, 1.0f), 2, 0, 0, 0, 16.0f, 16.0f, 8.0f, 16.0f, 0, false, 8, base_scale);
	}

	stylebox.blockquote_nested = get_theme_stylebox_or(p_owner, "MarkdownLabel", "blockquote_nested");
	if (!stylebox.blockquote_nested.is_valid()) {
		stylebox.blockquote_nested = stylebox.blockquote;
	}

	stylebox.search_result = get_theme_stylebox_or(p_owner, "MarkdownLabel", "search_result");
	if (!stylebox.search_result.is_valid()) {
		stylebox.search_result = create_internal_stylebox(Color(1.0f, 0.72f, 0.16f, 0.42f), Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, true, 8, base_scale);
	}

	stylebox.highlight = get_theme_stylebox_or(p_owner, "MarkdownLabel", "highlight");
	if (!stylebox.highlight.is_valid()) {
		stylebox.highlight = create_internal_stylebox(Color(0.6f, 0.6f, 0.6f, 0.392f), Color(0, 0, 0, 0), 0, 0, 0, 0, 4.0f, 0.0f, 4.0f, 0.0f, 2, true, 8, base_scale);
	}

	stylebox.separator = get_theme_stylebox_or(p_owner, "MarkdownLabel", "separator");
	if (!stylebox.separator.is_valid()) {
		stylebox.separator = create_internal_stylebox(Color(0.537f, 0.537f, 0.537f, 1.0f), Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, true, 8, base_scale);
	}

	stylebox.table_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_panel");
	if (!stylebox.table_panel.is_valid()) {
		stylebox.table_panel = create_internal_stylebox(Color(0.0f, 0.0f, 0.0f, 0.6f), Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 3, true, 8, base_scale);
	}

	stylebox.table_header_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_header_panel");
	if (!stylebox.table_header_panel.is_valid()) {
		stylebox.table_header_panel = create_internal_stylebox(Color(1.0f, 1.0f, 1.0f, 0.196f), Color(0, 0, 0, 0), 0, 0, 0, 1, 4.0f, 4.0f, 4.0f, 4.0f, 0, false, 8, base_scale);
	}

	stylebox.table_cell_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_cell_panel");
	if (!stylebox.table_cell_panel.is_valid()) {
		stylebox.table_cell_panel = create_internal_stylebox(Color(1.0f, 1.0f, 1.0f, 0.196f), Color(0, 0, 0, 0), 0, 0, 0, 0, 4.0f, 4.0f, 4.0f, 4.0f, 0, false, 8, base_scale);
	}

	stylebox.table_striped_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_striped_panel");
	if (!stylebox.table_striped_panel.is_valid()) {
		stylebox.table_striped_panel = create_internal_stylebox(Color(1.0f, 1.0f, 1.0f, 0.024f), Color(0, 0, 0, 0), 0, 0, 0, 0, 4.0f, 4.0f, 4.0f, 4.0f, 0, true, 8, base_scale);
	}

	// Texture
	icon.task_checked = get_theme_icon_or(p_owner, "MarkdownLabel", "task_checked");
	if (!icon.task_checked.is_valid()) {
		icon.task_checked = ResourceLoader::get_singleton()->load("uid://diq14yv8brh7m");
	}
	icon.task_unchecked = get_theme_icon_or(p_owner, "MarkdownLabel", "task_unchecked");
	if (!icon.task_unchecked.is_valid()) {
		icon.task_unchecked = ResourceLoader::get_singleton()->load("uid://wgx35yuv1sh3");
	}
	icon.file_broken = get_theme_icon_or(p_owner, "MarkdownLabel", "file_broken");
	if (!icon.file_broken.is_valid()) {
		icon.file_broken = ResourceLoader::get_singleton()->load("uid://cn51pvobw1gxd");
	}

	apply_editor_light_theme(*this, get_editor_light_theme_info(p_owner));
}

// MarkdownLabelCanvas
MarkdownLabelCanvas::MarkdownLabelCanvas() {
	s_live_image_canvas_count++;
}

MarkdownLabelCanvas::~MarkdownLabelCanvas() {
	s_live_image_canvas_count = std::max<int32_t>(0, s_live_image_canvas_count - 1);
	if (s_live_image_canvas_count == 0) {
		s_image_watchers.clear();
		s_pending_local_image_uris.clear();
		s_image_cache.clear();
	}
}

void MarkdownLabelCanvas::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_markdown_text", "text"), &MarkdownLabelCanvas::set_markdown_text);
	ClassDB::bind_method(D_METHOD("append_text_incremental", "delta"), &MarkdownLabelCanvas::append_text_incremental);
	ClassDB::bind_method(D_METHOD("finish_stream"), &MarkdownLabelCanvas::finish_stream);
	ClassDB::bind_method(D_METHOD("clear_document"), &MarkdownLabelCanvas::clear_document);
	ClassDB::bind_method(D_METHOD("set_streaming_enabled", "enabled"), &MarkdownLabelCanvas::set_streaming_enabled);
	ClassDB::bind_method(D_METHOD("set_max_unstable_lines", "lines"), &MarkdownLabelCanvas::set_max_unstable_lines);
	ClassDB::bind_method(D_METHOD("set_deferred_layout_enabled", "enabled"), &MarkdownLabelCanvas::set_deferred_layout_enabled);
	ClassDB::bind_method(D_METHOD("is_deferred_layout_enabled"), &MarkdownLabelCanvas::is_deferred_layout_enabled);
	ClassDB::bind_method(D_METHOD("set_layout_flush_interval_msec", "msec"), &MarkdownLabelCanvas::set_layout_flush_interval_msec);
	ClassDB::bind_method(D_METHOD("get_layout_flush_interval_msec"), &MarkdownLabelCanvas::get_layout_flush_interval_msec);
	ClassDB::bind_method(D_METHOD("flush_pending_layout"), &MarkdownLabelCanvas::flush_pending_layout);
	ClassDB::bind_method(D_METHOD("select_all"), &MarkdownLabelCanvas::select_all);
	ClassDB::bind_method(D_METHOD("select_between_points", "from", "to"), &MarkdownLabelCanvas::select_between_points);
	ClassDB::bind_method(D_METHOD("get_link_uri_at_point", "position"), &MarkdownLabelCanvas::get_link_uri_at_point);
	ClassDB::bind_method(D_METHOD("clear_selection"), &MarkdownLabelCanvas::clear_selection);
	ClassDB::bind_method(D_METHOD("search", "text", "flags", "from_line", "from_column"), &MarkdownLabelCanvas::search);
	ClassDB::bind_method(D_METHOD("get_selected_text"), &MarkdownLabelCanvas::get_selected_text);
	ClassDB::bind_method(D_METHOD("get_rendered_block_count"), &MarkdownLabelCanvas::get_rendered_block_count);
	ClassDB::bind_method(D_METHOD("get_rendered_html"), &MarkdownLabelCanvas::get_rendered_html);
	ClassDB::bind_method(D_METHOD("get_content_height"), &MarkdownLabelCanvas::get_content_height);
	ClassDB::bind_method(D_METHOD("has_anchor", "anchor"), &MarkdownLabelCanvas::has_anchor);
	ClassDB::bind_method(D_METHOD("get_anchors"), &MarkdownLabelCanvas::get_anchors);
	ClassDB::bind_method(D_METHOD("get_anchor_offset", "anchor"), &MarkdownLabelCanvas::get_anchor_offset);
	ClassDB::bind_method(D_METHOD("get_search_result_y"), &MarkdownLabelCanvas::get_search_result_y);
	ClassDB::bind_method(D_METHOD("get_bottom_position"), &MarkdownLabelCanvas::get_bottom_position);
}

void MarkdownLabelCanvas::set_label(MarkdownLabel* p_label) {
	label = p_label;
	mark_layout_dirty();
}

void MarkdownLabelCanvas::set_markdown_text(const String& p_text) {
	layout_document.clear();
	raw_text = p_text;
	reset_source_line_starts();
	selection_anchor = 0;
	selection_caret = 0;
	search_match_start = -1;
	search_match_end = -1;
	dragging_selection = false;
	selection_dragged = false;
	selection_drag_attempt = false;
	pressed_markdown = false;
	parser_state.reset();
	cached_blocks.clear();
	parsed_source_line_count = 0;
	// A streaming append before the next redraw must start with a fresh layout.
	items.clear();
	pending_stream_parse = false;
	pending_layout_elapsed_msec = 0.0;
	internal_processing_enabled = false;
	set_process_internal(false);
	update_internal_processing();
	mark_layout_dirty();
}

void MarkdownLabelCanvas::reset_source_line_starts() {
	source_line_starts.clear();
	source_line_starts.push_back(0);
	for (int64_t i = 0; i < raw_text.length(); ++i) {
		if (raw_text[i] != '\n' && raw_text[i] != '\r') continue;
		if (raw_text[i] == '\r' && i + 1 < raw_text.length() && raw_text[i + 1] == '\n') ++i;
		source_line_starts.push_back(i + 1);
	}
}

void MarkdownLabelCanvas::append_source_line_starts(int64_t p_previous_length) {
	if (raw_text.length() <= p_previous_length) return;
	int64_t scan_start = p_previous_length;
	if (p_previous_length > 0 && raw_text[p_previous_length - 1] == '\r') {
		if (source_line_starts.size() > 1 && source_line_starts.back() == p_previous_length) source_line_starts.pop_back();
		scan_start = p_previous_length - 1;
	}
	for (int64_t i = scan_start; i < raw_text.length(); ++i) {
		if (raw_text[i] != '\n' && raw_text[i] != '\r') continue;
		if (raw_text[i] == '\r' && i + 1 < raw_text.length() && raw_text[i + 1] == '\n') ++i;
		source_line_starts.push_back(i + 1);
	}
}

String MarkdownLabelCanvas::resolve_image_uri(const String& p_uri, bool p_html) const {
	const String resource_base = label != nullptr ? label->get_resource_base_dir() : String("res://");
	if (p_html) {
		if (dockit::is_remote_image_url(p_uri)) return p_uri;
		const String path = dockit::resolve_html_image_path(p_uri, resource_base);
		if (path.is_empty()) return {};
		const bool available = ResourceLoader::get_singleton()->exists(path, "Texture2D") ||
			((path.begins_with("user://") || path.is_absolute_path()) && FileAccess::file_exists(path));
		return available ? path : String();
	}
	if (p_uri.begins_with("uid://")) return p_uri;
	if (p_uri.is_absolute_path() || p_uri.begins_with("res://") || p_uri.begins_with("user://")) {
		return dockit::normalize_html_resource_path(p_uri).value_or(String());
	}
	if (p_uri.contains(":")) {
		return p_uri;
	}
	if (resource_base.is_empty()) return {};
	return dockit::normalize_html_resource_path(resource_base.path_join(p_uri)).value_or(String());
}

int32_t MarkdownLabelCanvas::get_layout_height() const {
	return layout_height;
}

MarkdownParseOptions MarkdownLabelCanvas::get_parse_options() const {
	return label != nullptr ? MarkdownParseOptions{ label->is_parsing_latex(), label->is_parsing_gfm(), label->is_parsing_html(), label->is_parsing_mermaid() } : MarkdownParseOptions();
}

void MarkdownLabelCanvas::parse_current_text_incremental() {
	const MarkdownParseOptions options = get_parse_options();
	if (streaming_enabled) {
		const int32_t line_count = static_cast<int32_t>(source_line_starts.size());
		int32_t reparse_line = find_reparse_line(cached_blocks, line_count, parser_state, max_unstable_lines);
		if (parsed_source_line_count < line_count) {
			const int32_t unparsed_prefix = std::max<int32_t>(0, parsed_source_line_count - max_unstable_lines);
			reparse_line = std::min(reparse_line, unparsed_prefix);
		}
		if (options.parse_html && !cached_blocks.empty()) {
			// The append can complete the previous last line and add more lines in the same chunk.
			reparse_line = std::min(reparse_line, std::max(0, cached_blocks.back().line - 1));
		}
		rebuild_tail(raw_text, source_line_starts, parsed_source_line_count, reparse_line, cached_blocks, parser_state, options);
		parsed_source_line_count = line_count;
	}
	else {
		parser_state.reset();
		MarkdownParseResult result = parse_markdown_document(raw_text, nullptr, INT_MAX, options);
		cached_blocks = result.blocks;
		parser_state = result.state;
		parsed_source_line_count = static_cast<int32_t>(source_line_starts.size());
	}
}

void MarkdownLabelCanvas::append_text_incremental(const String& p_delta) {
	const int64_t previous_length = raw_text.length();
	raw_text += p_delta;
	append_source_line_starts(previous_length);

	if (deferred_layout_enabled && streaming_enabled) {
		pending_stream_parse = true;
		update_internal_processing();
		return;
	}

	parse_current_text_incremental();

	mark_layout_dirty();
}

void MarkdownLabelCanvas::finish_stream() {
	const bool had_pending_parse = pending_stream_parse;
	pending_stream_parse = false;
	pending_layout_elapsed_msec = 0.0;
	if (!had_pending_parse) return;
	parse_current_text_incremental();
	mark_layout_dirty();
}

void MarkdownLabelCanvas::clear_document() {
	layout_document.clear();
	raw_text = String();
	source_line_starts.assign(1, 0);
	plain_text = String();
	items.clear();
	lines.clear();
	cached_blocks.clear();
	parser_state.reset();
	parsed_source_line_count = 0;
	mermaid_cache.update({}, {});
	pending_stream_parse = false;
	pending_layout_elapsed_msec = 0.0;
	rendered_block_count = 0;
	selection_anchor = 0;
	selection_caret = 0;
	search_match_start = -1;
	search_match_end = -1;
	dragging_selection = false;
	selection_dragged = false;
	selection_drag_attempt = false;
	pressed_markdown = false;
	pressed_blank = false;
	internal_processing_enabled = false;
	set_process_internal(false);
	content_height = 1.0f;
	layout_dirty = false;
	anchor_offsets.clear();
	html_anchor_sources.clear();
	set_custom_minimum_size(Vector2(1.0, content_height));
	update_minimum_size();
	queue_redraw();
	update_internal_processing();
}

void MarkdownLabelCanvas::set_streaming_enabled(bool p_enabled) {
	streaming_enabled = p_enabled;
}

void MarkdownLabelCanvas::set_max_unstable_lines(int32_t p_lines) {
	max_unstable_lines = std::max<int32_t>(1, std::min<int32_t>(p_lines, 256));
}

void MarkdownLabelCanvas::set_deferred_layout_enabled(bool p_enabled) {
	if (deferred_layout_enabled == p_enabled) {
		return;
	}

	if (!p_enabled) {
		flush_pending_layout();
	}
	deferred_layout_enabled = p_enabled;
	update_internal_processing();
}

bool MarkdownLabelCanvas::is_deferred_layout_enabled() const {
	return deferred_layout_enabled;
}

void MarkdownLabelCanvas::set_layout_flush_interval_msec(int32_t p_msec) {
	layout_flush_interval_msec = std::max<int32_t>(1, std::min<int32_t>(p_msec, 1000));
}

int32_t MarkdownLabelCanvas::get_layout_flush_interval_msec() const {
	return layout_flush_interval_msec;
}

bool MarkdownLabelCanvas::flush_pending_layout() {
	if (!pending_stream_parse) {
		return false;
	}

	pending_stream_parse = false;
	pending_layout_elapsed_msec = 0.0;
	parse_current_text_incremental();
	mark_layout_dirty();
	update_internal_processing();
	if (label != nullptr) {
		label->notify_canvas_layout_changed();
	}
	return true;
}

void MarkdownLabelCanvas::request_local_image(const String& p_uri) {
	if (p_uri.is_empty() || !s_image_cache.has(p_uri) || s_image_cache[p_uri].status != MARKDOWN_IMAGE_LOADING) {
		return;
	}

	if (has_pending_local_image_uri(p_uri)) {
		update_internal_processing();
		return;
	}

	ResourceLoader* loader = ResourceLoader::get_singleton();
	if (loader == nullptr) {
		MarkdownImageCacheEntry entry;
		entry.status = MARKDOWN_IMAGE_FAILED;
		entry.error = "resource_loader_missing";
		s_image_cache[p_uri] = entry;
		notify_image_watchers(p_uri);
		return;
	}

	const Error error = loader->load_threaded_request(p_uri, "Texture2D", true);
	if (error != OK) {
		MarkdownImageCacheEntry entry;
		entry.status = MARKDOWN_IMAGE_FAILED;
		entry.error = "threaded_request_failed";
		s_image_cache[p_uri] = entry;
		notify_image_watchers(p_uri);
		return;
	}

	add_pending_local_image_uri(p_uri);
	update_internal_processing();
}

void MarkdownLabelCanvas::refresh_image_content() {
	mark_layout_dirty();
}

void MarkdownLabelCanvas::remote_image_completed(const String& p_url, const Ref<Texture2D>& p_texture, uint64_t p_canvas) {
	auto* canvas = Object::cast_to<MarkdownLabelCanvas>(ObjectDB::get_instance(ObjectID(p_canvas)));
	if (!canvas) return;
	if (p_texture.is_valid() && !s_image_cache.has(p_url)) {
		s_image_cache.insert(p_url, { MARKDOWN_IMAGE_READY, p_texture,{}, 0.0 });
	}
	canvas->refresh_image_content();
}

void MarkdownLabelCanvas::refresh_after_html_resource(uint64_t p_canvas) {
	if (auto* canvas = Object::cast_to<MarkdownLabelCanvas>(ObjectDB::get_instance(ObjectID(p_canvas)))) {
		canvas->refresh_image_content();
	}
}

bool MarkdownLabelCanvas::process_pending_local_images() {
	if (s_pending_local_image_uris.empty()) {
		return false;
	}

	ResourceLoader* loader = ResourceLoader::get_singleton();
	if (loader == nullptr) {
		return false;
	}

	std::vector<String> remaining;
	for (const String& uri : s_pending_local_image_uris) {
		if (!s_image_cache.has(uri) || s_image_cache[uri].status != MARKDOWN_IMAGE_LOADING) {
			continue;
		}

		const ResourceLoader::ThreadLoadStatus status = loader->load_threaded_get_status(uri);
		if (status == ResourceLoader::THREAD_LOAD_IN_PROGRESS) {
			remaining.push_back(uri);
			continue;
		}

		MarkdownImageCacheEntry entry;
		if (status == ResourceLoader::THREAD_LOAD_LOADED) {
			Ref<Resource> resource = loader->load_threaded_get(uri);
			Ref<Texture2D> texture = resource;
			if (texture.is_valid()) {
				entry.status = MARKDOWN_IMAGE_READY;
				entry.texture = texture;
			}
			else {
				entry.status = MARKDOWN_IMAGE_FAILED;
				entry.error = "resource_not_texture";
			}
		}
		else {
			entry.status = MARKDOWN_IMAGE_FAILED;
			entry.error = "threaded_load_failed";
		}

		s_image_cache[uri] = entry;
		notify_image_watchers(uri);
	}

	s_pending_local_image_uris = remaining;
	return !s_pending_local_image_uris.empty();
}

bool MarkdownLabelCanvas::update_gif_animations(double p_delta) {
	const auto shared_html = layout_document.get_layout();
	bool has_animated_gif = shared_html.is_valid() && shared_html->advance_embedded_images(p_delta);
	if (has_animated_gif && p_delta > 0) queue_redraw();
	for (const KeyValue<String, MarkdownImageCacheEntry>& key_value : s_image_cache) {
		if (key_value.value.status != MARKDOWN_IMAGE_READY || !is_animated_gif_texture(key_value.value.texture)) {
			continue;
		}

		Object* texture_object = key_value.value.texture.ptr();
		if (texture_object == nullptr) {
			continue;
		}

		const int32_t frame_count = static_cast<int32_t>(texture_object->call("get_frame_count"));
		if (frame_count <= 1) {
			continue;
		}

		has_animated_gif = true;
		MarkdownImageCacheEntry& entry = s_image_cache[key_value.key];
		const int32_t frame = static_cast<int32_t>(texture_object->call("get_frame"));
		const double delay = std::max<double>(0.01, static_cast<double>(texture_object->call("get_frame_delay", frame)));
		entry.gif_elapsed += p_delta;
		if (entry.gif_elapsed >= delay) {
			entry.gif_elapsed = 0.0;
			texture_object->call("set_frame", (frame + 1) % frame_count);
			queue_redraw();
		}
	}

	return has_animated_gif;
}

void MarkdownLabelCanvas::update_internal_processing() {
	const bool should_process = pending_stream_parse || mermaid_cache.has_pending() || !s_pending_local_image_uris.empty() || dragging_selection || update_gif_animations(0.0);
	if (internal_processing_enabled != should_process) {
		internal_processing_enabled = should_process;
		set_process_internal(should_process);
	}
}

void MarkdownLabelCanvas::update_html_scrollbar_hover(const Ref<HTMLLayout>& p_layout, const Vector2& p_point) {
	if (hovered_html_scrollbar_layout != p_layout) {
		if (hovered_html_scrollbar_layout.is_valid()) hovered_html_scrollbar_layout->update_scrollbar_hover(std::nullopt);
		hovered_html_scrollbar_layout = p_layout;
	}
	if (p_layout.is_valid()) p_layout->update_scrollbar_hover(p_point);
}

void MarkdownLabelCanvas::mark_layout_dirty() {
	update_html_scrollbar_hover({});
	if (html_scrollbar_drag) html_scrollbar_drag->layout->set_scrollbar_pressed(std::nullopt);
	html_scrollbar_drag.reset();
	pressed_blank = false;
	layout_dirty = true;
	queue_redraw();
}

bool MarkdownLabelCanvas::has_selection() const {
	return selection_anchor != selection_caret;
}

bool MarkdownLabelCanvas::is_character_selected(int64_t p_character) const {
	const int64_t from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t to = std::max<int64_t>(selection_anchor, selection_caret);
	return from < to && p_character >= from && p_character <= to;
}

bool MarkdownLabelCanvas::is_position_inside_selection(const Vector2& p_position) {
	if (!has_selection()) {
		return false;
	}
	flush_pending_layout();
	rebuild_layout();
	return is_character_selected(hit_test_document(p_position));
}

void MarkdownLabelCanvas::select_all() {
	flush_pending_layout();
	rebuild_layout();
	selection_anchor = 0;
	selection_caret = plain_text.length();
	queue_redraw();
}

void MarkdownLabelCanvas::select_between_points(const Vector2& p_from, const Vector2& p_to) {
	flush_pending_layout();
	rebuild_layout();
	selection_origin = selection_point_at(p_from);
	if (selection_origin.excluded) return;
	extend_document_selection(p_to);
	queue_redraw();
}

String MarkdownLabelCanvas::get_link_uri_at_point(const Vector2& p_position) {
	flush_pending_layout();
	rebuild_layout();
	return link_uri_at_position(p_position);
}


void MarkdownLabelCanvas::clear_selection() {
	selection_drag_attempt = false;
	selection_anchor = selection_caret;
	queue_redraw();
}

Vector2i MarkdownLabelCanvas::search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column) {
	flush_pending_layout();
	rebuild_layout();

	const String needle = p_text;
	if (needle.is_empty() || plain_text.is_empty()) {
		search_match_start = -1;
		search_match_end = -1;
		queue_redraw();
		return Vector2i(-1, -1);
	}

	const bool match_case = (p_flags & 1) != 0;
	const bool whole_words = (p_flags & 2) != 0;
	const bool backwards = (p_flags & 4) != 0;
	const String haystack = match_case ? plain_text : plain_text.to_lower();
	const String search_needle = match_case ? needle : needle.to_lower();
	const int64_t needle_length = search_needle.length();
	int64_t start_offset = line_column_to_offset(p_from_line, p_from_column);

	if (backwards) {
		int64_t index = std::min<int64_t>(start_offset, std::max<int64_t>(0, haystack.length() - needle_length));
		while (index >= 0) {
			if (haystack.substr(static_cast<int32_t>(index), static_cast<int32_t>(needle_length)) == search_needle && (!whole_words || is_whole_word_match(plain_text, index, needle_length))) {
				search_match_start = index;
				search_match_end = index + needle_length;
				queue_redraw();
				return offset_to_line_column(index);
			}
			index--;
		}
	}
	else {
		int64_t index = std::max<int64_t>(0, std::min<int64_t>(start_offset, haystack.length()));
		while (index <= haystack.length() - needle_length) {
			const int64_t found = haystack.find(search_needle, static_cast<int32_t>(index));
			if (found < 0) {
				break;
			}
			if (!whole_words || is_whole_word_match(plain_text, found, needle_length)) {
				search_match_start = found;
				search_match_end = found + needle_length;
				queue_redraw();
				return offset_to_line_column(found);
			}
			index = found + 1;
		}
	}

	search_match_start = -1;
	search_match_end = -1;
	queue_redraw();
	return Vector2i(-1, -1);
}

String MarkdownLabelCanvas::get_selected_text() const {
	const_cast<MarkdownLabelCanvas*>(this)->flush_pending_layout();
	const_cast<MarkdownLabelCanvas*>(this)->rebuild_layout();
	const int64_t from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t to = std::max<int64_t>(selection_anchor, selection_caret);
	if (from >= to || from < 0 || to > plain_text.length()) {
		return String();
	}

	if (!label) return plain_text.substr(from, to - from);
	String text;
	int64_t offset = from;
	const auto append_html = [&](const HTMLLayout& p_layout, int64_t p_start, int64_t p_end) {
		if (p_end <= offset || p_start >= to) return;
		if (offset < p_start) text += plain_text.substr(offset, p_start - offset);
		const int64_t end = std::min(to, p_end);
		text += p_layout.get_copyable_text(std::max(offset, p_start) - p_start, end - p_start);
		offset = end;
		};
	for (const MarkdownCanvasItem& item : items) {
		if (item.global_start >= to) break;
		if (item.global_end <= offset) continue;
		if (item.html_layout.is_valid()) {
			append_html(*item.html_layout.ptr(), item.global_start, item.global_end);
		}
		else {
			for (const MarkdownContainerLine& line : item.container_lines) {
				if (line.html_layout.is_valid()) append_html(*line.html_layout.ptr(), line.global_start, line.global_end);
			}
		}
	}
	if (offset < to) text += plain_text.substr(offset, to - offset);
	return text;
}

Variant MarkdownLabelCanvas::_get_drag_data(const Vector2& p_at_position) {
	Variant drag_data = Control::_get_drag_data(p_at_position);
	if (drag_data.get_type() != Variant::NIL) {
		return drag_data;
	}

	if (label == nullptr || !label->is_drag_and_drop_selection_enabled() || !selection_drag_attempt) {
		return Variant();
	}

	const String selected_text = get_selected_text();
	if (selected_text.is_empty()) {
		return Variant();
	}

	Label* preview = memnew(Label);
	preview->set_text(selected_text);
	preview->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	preview->set_auto_translate_mode(Node::AUTO_TRANSLATE_MODE_DISABLED);
	set_drag_preview(preview);
	return selected_text;
}

void MarkdownLabelCanvas::_generate_context_menu() {
	menu = memnew(PopupMenu);
	add_child(menu, false, Node::INTERNAL_MODE_FRONT);
	menu->connect("id_pressed", callable_mp(this, &MarkdownLabelCanvas::menu_option));

	menu->add_item("Copy", MarkdownLabel::MENU_COPY);
	menu->add_item("Select All", MarkdownLabel::MENU_SELECT_ALL);
}

void MarkdownLabelCanvas::_update_context_menu() {
	if (menu == nullptr) {
		_generate_context_menu();
	}

	int32_t idx = menu->get_item_index(MarkdownLabel::MENU_COPY);
	if (idx >= 0) {
		menu->set_item_accelerator(idx, label != nullptr && label->is_shortcut_keys_enabled() ? _get_menu_action_accelerator("ui_copy") : KEY_NONE);
		menu->set_item_disabled(idx, label == nullptr || !label->is_selection_enabled());
	}

	idx = menu->get_item_index(MarkdownLabel::MENU_SELECT_ALL);
	if (idx >= 0) {
		menu->set_item_accelerator(idx, label != nullptr && label->is_shortcut_keys_enabled() ? _get_menu_action_accelerator("ui_text_select_all") : KEY_NONE);
		menu->set_item_disabled(idx, label == nullptr || !label->is_selection_enabled());
	}
}

Key MarkdownLabelCanvas::_get_menu_action_accelerator(const String& p_action) {
	const TypedArray<Ref<InputEvent>> events = InputMap::get_singleton()->action_get_events(p_action);
	if (events.is_empty()) {
		return KEY_NONE;
	}

	const Ref<InputEvent> first_event = events[0];
	if (first_event.is_null()) {
		return KEY_NONE;
	}

	const Ref<InputEventKey> key_event = first_event;
	if (key_event.is_null()) {
		return KEY_NONE;
	}

	if (key_event->get_physical_keycode() != KEY_NONE) {
		return key_event->get_physical_keycode_with_modifiers();
	}
	else {
		return key_event->get_keycode_with_modifiers();
	}
}

PopupMenu* MarkdownLabelCanvas::get_menu() {
	if (menu == nullptr) {
		_generate_context_menu();
	}
	return menu;
}

bool MarkdownLabelCanvas::is_menu_visible() const {
	return menu != nullptr && menu->is_visible();
}

void MarkdownLabelCanvas::menu_option(int p_option) {
	switch (p_option) {
	case MarkdownLabel::MENU_COPY: {
		String txt = get_selected_text();
		if (txt.is_empty()) {
			txt = plain_text;
		}
		if (!txt.is_empty()) {
			DisplayServer::get_singleton()->clipboard_set(txt);
		}
	} break;
	case MarkdownLabel::MENU_SELECT_ALL: {
		select_all();
	} break;
	}
}

int32_t MarkdownLabelCanvas::get_rendered_block_count() {
	flush_pending_layout();
	rebuild_layout();
	return rendered_block_count;
}

String MarkdownLabelCanvas::get_rendered_html() {
	flush_pending_layout();
	rebuild_layout();
	const Ref<HTMLLayout> html = layout_document.get_layout();
	if (html.is_null()) return String();
	const Ref<HTMLElement> body = html->get_body();
	return body.is_valid() ? body->get_inner_html() : String();
}

float MarkdownLabelCanvas::get_content_height() {
	flush_pending_layout();
	rebuild_layout();
	return content_height;
}

bool MarkdownLabelCanvas::has_anchor(const String& p_anchor) const {
	const_cast<MarkdownLabelCanvas*>(this)->flush_pending_layout();
	const_cast<MarkdownLabelCanvas*>(this)->rebuild_layout();
	return anchor_offsets.has(p_anchor);
}

PackedStringArray MarkdownLabelCanvas::get_anchors() const {
	const_cast<MarkdownLabelCanvas*>(this)->flush_pending_layout();
	const_cast<MarkdownLabelCanvas*>(this)->rebuild_layout();
	PackedStringArray result;
	const Array keys = anchor_offsets.keys();
	for (int32_t i = 0; i < keys.size(); i++) {
		result.append(keys[i]);
	}
	return result;
}

float MarkdownLabelCanvas::get_anchor_offset(const String& p_anchor) const {
	const_cast<MarkdownLabelCanvas*>(this)->flush_pending_layout();
	const_cast<MarkdownLabelCanvas*>(this)->rebuild_layout();
	if (!anchor_offsets.has(p_anchor)) {
		return -1.0f;
	}
	return anchor_offsets[p_anchor];
}

std::optional<float> MarkdownLabelCanvas::scroll_to_html_anchor(const String& p_anchor, float p_align) {
	const auto* source = html_anchor_sources.getptr(p_anchor);
	if (!source) return std::nullopt;
	const auto target = source->layout->scroll_anchor_into_view(p_anchor, p_align, *this,
		callable_mp(this, &MarkdownLabelCanvas::queue_pointer_update));
	if (!target) return std::nullopt;
	queue_pointer_update();
	return source->top + target->area.position.y;
}

float MarkdownLabelCanvas::get_search_result_y() const {
	const_cast<MarkdownLabelCanvas*>(this)->flush_pending_layout();
	const_cast<MarkdownLabelCanvas*>(this)->rebuild_layout();
	if (search_match_start < 0 || search_match_end <= search_match_start) {
		return -1.0f;
	}

	for (const MarkdownCanvasItem& item : items) {
		if (item.html_layout.is_valid() && search_match_start >= item.global_start && search_match_start < item.global_end) {
			const auto rects = item.html_layout->get_selection_rects(search_match_start - item.global_start, search_match_end - item.global_start);
			return item.text_rect.position.y + (rects.is_empty() ? 0.0f : Rect2(rects[0]).position.y);
		}
		if (!item.container_lines.empty() || item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
			for (const MarkdownContainerLine& line : item.container_lines) {
				if (search_match_start >= line.global_start && search_match_start < line.global_end) {
					if (line.html_layout.is_valid()) {
						const auto rects = line.html_layout->get_selection_rects(search_match_start - line.global_start, search_match_end - line.global_start);
						return line.text_rect.position.y + (rects.is_empty() ? 0.0f : Rect2(rects[0]).position.y);
					}
					return item.rect.position.y;
				}
			}
		}
		if (search_match_start >= item.paragraph_global_start && search_match_start < item.paragraph_global_end) {
			return item.rect.position.y;
		}
		if (search_match_start >= item.header_global_start && search_match_start < item.header_global_end) {
			return item.rect.position.y;
		}
	}

	return -1.0f;
}

float MarkdownLabelCanvas::get_bottom_position() const {
	const_cast<MarkdownLabelCanvas*>(this)->flush_pending_layout();
	const_cast<MarkdownLabelCanvas*>(this)->rebuild_layout();
	return content_height;
}

Vector2 MarkdownLabelCanvas::_get_minimum_size() const {
	return Vector2(1.0, content_height);
}

void MarkdownLabelCanvas::build_theme_cache() {
	const int32_t extra = label == nullptr ? 0 : label->get_extra_font_size();
	theme_cache.build(label, extra);
}

void MarkdownLabelCanvas::rebuild_layout() {
	const float width = std::max<float>(1.0f, get_size().x);
	const int32_t height = is_inside_tree() ? std::max(1, int(get_viewport_rect().size.y)) : 1;
	if (!layout_dirty && std::abs(width - layout_width) < 0.01f && height == layout_html_height) return;
	layout_dirty = false;
	layout_width = width;
	layout_html_height = height;
	build_theme_cache();
	const MarkdownParseOptions options = get_parse_options();
	if (!streaming_enabled || cached_blocks.empty()) {
		MarkdownParseResult parsed = parse_markdown_document(raw_text, nullptr, INT_MAX, options);
		cached_blocks = std::move(parsed.blocks);
		parser_state = parsed.state;
		parsed_source_line_count = static_cast<int32_t>(source_line_starts.size());
	}
	HashSet<String> mermaid_sources;
	if (options.parse_mermaid && MermaidRender::is_available()) collect_mermaid_sources(cached_blocks, mermaid_sources);
	MermaidRenderCache::Palette palette;
	palette.light = theme_cache.color.text.get_luminance() < 0.5f;
	mermaid_cache.update(mermaid_sources, palette);
	const Ref<HTMLLayout> previous_html = layout_document.get_layout();
	const dockit::HTMLBoundary previous_anchor = previous_html.is_valid() ? previous_html->text_boundary_at(selection_anchor) : dockit::HTMLBoundary();
	const dockit::HTMLBoundary previous_caret = previous_html.is_valid() ? previous_html->text_boundary_at(selection_caret) : dockit::HTMLBoundary();
	const Error error = layout_document.update(cached_blocks, raw_text, source_line_starts, options, theme_cache, label, mermaid_cache,
		Vector2i(std::max(1, int(width)), height), label ? label->get_resource_base_dir() : String("res://"),
		label ? label->get_stylesheet() : Ref<CSSStyleSheet>());
	items.clear();
	lines.clear();
	anchor_offsets.clear();
	html_anchor_sources.clear();
	footnote_reference_offsets.clear();
	footnote_definition_offsets.clear();
	rendered_block_count = int32_t(cached_blocks.size());
	const Ref<HTMLLayout> html = layout_document.get_layout();
	plain_text = error == OK && html.is_valid() ? html->get_plain_text() : String();
	content_height = error == OK && html.is_valid() ? std::max(1, html->get_content_size().y) : 1;
	if (html.is_valid()) {
		html->set_resource_layout_callback(callable_mp_static(&MarkdownLabelCanvas::refresh_after_html_resource).bind(get_instance_id()));
		MarkdownCanvasItem item;
		item.type = MARKDOWN_BLOCK_HTML;
		item.html_layout = html;
		item.rect = Rect2(0, 0, width, content_height);
		item.text_rect = item.rect;
		item.global_start = 0;
		item.global_end = plain_text.length();
		items.push_back(std::move(item));
		for (const auto& anchor : html->get_anchors()) {
			anchor_offsets[anchor.key] = anchor.value.bounds.position.y;
			html_anchor_sources.insert(anchor.key, { html, 0.0f });
		}
	}
	if (html == previous_html && html.is_valid()) {
		if (previous_anchor.node.is_valid() && previous_anchor.node->is_connected()) selection_anchor = html->document_text_offset(previous_anchor);
		if (previous_caret.node.is_valid() && previous_caret.node->is_connected()) selection_caret = html->document_text_offset(previous_caret);
	}
	selection_anchor = std::min<int64_t>(selection_anchor, plain_text.length());
	selection_caret = std::min<int64_t>(selection_caret, plain_text.length());
	if (search_match_end > plain_text.length() || search_match_end <= search_match_start) {
		search_match_start = -1;
		search_match_end = -1;
	}
	set_custom_minimum_size(Vector2(1.0, content_height));
	update_minimum_size();
	update_internal_processing();
	queue_pointer_update();
	if (label) label->notify_canvas_layout_changed();
}

double MarkdownLabelCanvas::get_offset_for_source_line(double p_line, int32_t p_line_count) {
	rebuild_layout();
	return layout_document.source_line_to_y(p_line, p_line_count);
}

double MarkdownLabelCanvas::get_source_line_at_offset(double p_offset, int32_t p_line_count) {
	rebuild_layout();
	return layout_document.y_to_source_line(p_offset, p_line_count);
}


Rect2 MarkdownLabelCanvas::get_visible_content_rect() const {
	const float width = std::max<float>(1.0f, get_size().x);
	if (label == nullptr) {
		return Rect2(0.0f, 0.0f, width, content_height);
	}

	VScrollBar* v_scroll = label->get_v_scroll_bar();
	const float visible_y = v_scroll == nullptr ? 0.0f : static_cast<float>(v_scroll->get_value());
	const float visible_height = std::max<float>(1.0f, label->get_size().y);
	const float buffer = 256.0f;
	return Rect2(0.0f, std::max<float>(0.0f, visible_y - buffer), width, visible_height + buffer * 2.0f);
}

Rect2 MarkdownLabelCanvas::get_html_visible_rect() const {
	const Rect2 bounds(Vector2(), get_size());
	if (!is_inside_tree() || label == nullptr || label->scroll_container == nullptr) {
		return bounds;
	}
	const Transform2D transform = get_global_transform_with_canvas().affine_inverse() * label->scroll_container->get_global_transform_with_canvas();
	return bounds.intersection(transform.xform(Rect2(Vector2(), label->scroll_container->get_size())));
}

void MarkdownLabelCanvas::_draw() {
	if (label == nullptr) {
		return;
	}
	const Rect2 visible = get_html_visible_rect();
	const int64_t selection_from = std::min(selection_anchor, selection_caret);
	const int64_t selection_to = std::max(selection_anchor, selection_caret);
	dockit::HTMLTextRenderEffects text_effects;
	text_effects.outline_size = theme_cache.constant.outline_size;
	text_effects.outline_color = theme_cache.color.font_outline;
	text_effects.shadow_outline_size = theme_cache.constant.shadow_outline_size;
	text_effects.shadow_offset = Vector2(theme_cache.constant.shadow_offset_x, theme_cache.constant.shadow_offset_y);
	text_effects.shadow_color = theme_cache.color.font_shadow;
	auto draw_html = [&](const Ref<HTMLLayout>& p_layout, const Rect2& p_rect, int64_t p_from, int64_t p_to) {
		if (p_layout.is_valid()) {
			const Rect2 clip = visible.intersection(p_rect);
			if (clip.has_area()) {
				const auto local_range = [&](int64_t p_start, int64_t p_end) {
					return Vector2i(CLAMP(p_start, p_from, p_to) - p_from, CLAMP(p_end, p_from, p_to) - p_from);
					};
				p_layout->draw_with_text_effects(this, p_rect.position, clip, local_range(selection_from, selection_to),
					theme_cache.stylebox.selection.ptr(), theme_cache.color.selection_color,
					local_range(search_match_start, search_match_end), theme_cache.stylebox.search_result.ptr(), {}, text_effects);
			}
		}
		};
	for (const MarkdownCanvasItem& item : items) {
		if (item.rect.position.y > visible.get_end().y) {
			break;
		}
		if (item.rect.get_end().y < visible.position.y) {
			continue;
		}
		draw_html(item.html_layout, item.text_rect, item.global_start, item.global_end);
		for (const MarkdownContainerLine& line : item.container_lines) {
			draw_html(line.html_layout, line.text_rect, line.global_start, line.global_end);
		}
	}
}

bool MarkdownLabelCanvas::is_rect_visible(const Rect2& p_rect, const Rect2& p_visible_rect) const {
	return p_rect.intersects(p_visible_rect) || p_visible_rect.encloses(p_rect) || p_rect.encloses(p_visible_rect);
}

void MarkdownLabelCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		layout_document.clear();
	}
	if (p_what == NOTIFICATION_RESIZED || p_what == NOTIFICATION_THEME_CHANGED) {
		mark_layout_dirty();
	}

	if (p_what == NOTIFICATION_MOUSE_EXIT) {
		update_html_scrollbar_hover({});
		pressed_html_link = 0;
		pressed_markdown = false;
		pressed_blank = false;
		if (!dragging_selection || !selection_dragged) {
			set_default_cursor_shape(CURSOR_ARROW);
		}
	}

	if (p_what == NOTIFICATION_EXIT_TREE || (p_what == NOTIFICATION_VISIBILITY_CHANGED && !is_visible_in_tree())) {
		update_html_scrollbar_hover({});
		pressed_blank = false;
		if (html_scrollbar_drag) html_scrollbar_drag->layout->set_scrollbar_pressed(std::nullopt);
		for (const auto& anchor : html_anchor_sources) anchor.value.layout->cancel_scroll_activity();
	}

	if (p_what == NOTIFICATION_FOCUS_EXIT || p_what == NOTIFICATION_EXIT_TREE) {
		pressed_html_link = 0;
		pressed_html_layout.unref();
		pressed_markdown = false;
		pressed_blank = false;
		if (html_scrollbar_drag) html_scrollbar_drag->layout->set_scrollbar_pressed(std::nullopt);
		html_scrollbar_drag.reset();
		dragging_selection = false;
	}

	if (p_what == NOTIFICATION_FOCUS_EXIT) {
		if (label != nullptr && label->is_deselect_on_focus_loss_enabled()) {
			clear_selection();
		}
		selection_drag_attempt = false;
		queue_pointer_update();
	}

	if (p_what == NOTIFICATION_DRAG_END) {
		selection_drag_attempt = false;
	}

	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		const double delta = get_process_delta_time();
		if (mermaid_cache.poll()) {
			mark_layout_dirty();
		}
		if (pending_stream_parse) {
			pending_layout_elapsed_msec += delta * 1000.0;
			if (pending_layout_elapsed_msec >= static_cast<double>(layout_flush_interval_msec)) {
				flush_pending_layout();
			}
		}

		const bool has_pending_images = process_pending_local_images();
		const bool has_animated_gifs = update_gif_animations(delta);
		const bool should_keep_processing = pending_stream_parse || mermaid_cache.has_pending() || has_pending_images || has_animated_gifs;
		if (!dragging_selection || !selection_dragged || label == nullptr) {
			internal_processing_enabled = should_keep_processing;
			set_process_internal(internal_processing_enabled);
			return;
		}

		VScrollBar* v_scroll = label->get_v_scroll_bar();
		if (v_scroll == nullptr) {
			internal_processing_enabled = should_keep_processing;
			set_process_internal(internal_processing_enabled);
			return;
		}

		const Size2 label_size = label->get_size();
		const Vector2 label_mouse = label->get_local_mouse_position();

		const float scroll_margin = 24.0f;
		float scroll_delta = 0.0f;
		const float above = scroll_margin - label_mouse.y;
		const float below = label_mouse.y - (label_size.y - scroll_margin);

		if (above > 0.0f) {
			const float base_speed = 80.0f;
			scroll_delta = -base_speed * (above / scroll_margin) * static_cast<float>(delta);
		}
		else if (below > 0.0f) {
			const float base_speed = 80.0f;
			scroll_delta = base_speed * (below / scroll_margin) * static_cast<float>(delta);
		}

		if (scroll_delta != 0.0f) {
			const float max_scroll = v_scroll->get_max() - v_scroll->get_page();
			const float new_value = std::max<float>(0.0f, std::min<float>(max_scroll, v_scroll->get_value() + scroll_delta));
			v_scroll->set_value(new_value);

			const Vector2 canvas_mouse = get_local_mouse_position();
			extend_document_selection(canvas_mouse);
			queue_redraw();
		}

		internal_processing_enabled = true;
		set_process_internal(true);
		return;
	}

	if (p_what != NOTIFICATION_DRAW) {
		return;
	}

	rebuild_layout();
}

static float get_alignment_offset(const Ref<TextParagraph>& p_paragraph, int32_t p_line, float p_width) {
	if (p_paragraph.is_null()) return 0;
	const float remaining = std::max(0.0f, p_width - float(p_paragraph->get_line_size(p_line).x));
	const auto alignment = p_paragraph->get_alignment();
	return alignment == HORIZONTAL_ALIGNMENT_CENTER ? remaining / 2.0f : alignment == HORIZONTAL_ALIGNMENT_RIGHT ? remaining : 0.0f;
}

int64_t MarkdownLabelCanvas::hit_test_document(const Vector2& p_position) const {
	if (items.empty()) {
		return 0;
	}

	if (p_position.y < items.front().rect.position.y) {
		return items.front().global_start;
	}
	if (p_position.y > items.back().rect.get_end().y) {
		return plain_text.length();
	}

	int32_t low = 0;
	int32_t high = static_cast<int32_t>(items.size()) - 1;
	int32_t item_index = high;
	while (low <= high) {
		const int32_t middle = low + (high - low) / 2;
		if (p_position.y < items[middle].rect.position.y) {
			item_index = middle;
			high = middle - 1;
		}
		else if (p_position.y > items[middle].rect.get_end().y) {
			low = middle + 1;
		}
		else {
			item_index = middle;
			break;
		}
	}

	const MarkdownCanvasItem& item = items[item_index];
	if (p_position.y < item.rect.position.y) {
		return item.global_start;
	}
	if (p_position.y <= item.rect.get_end().y) {
		if (item.html_layout.is_valid()) {
			return item.global_start + item.html_layout->hit_test_text(dockit::to_local_point(p_position, item.text_rect.position));
		}
		if (item.type == MARKDOWN_BLOCK_TABLE) {
			for (const MarkdownTableCell& cell : item.cells) {
				if (cell.rect.has_point(p_position)) {
					Vector2 local_position = p_position - cell.text_rect.position;
					if (!cell.paragraph.is_null() && cell.paragraph->get_line_count() > 0) {
						float line_y = 0.0f;
						for (int32_t li = 0; li < cell.paragraph->get_line_count(); li++) {
							const float line_h = cell.paragraph->get_line_size(li).y + cell.paragraph->get_line_spacing();
							if (local_position.y < line_y + line_h || li == cell.paragraph->get_line_count() - 1) {
								local_position.x -= get_alignment_offset(cell.paragraph, li, cell.text_rect.size.x);
								break;
							}
							line_y += line_h;
						}
					}
					const int32_t local_character = std::min<int32_t>(std::max<int32_t>(cell.paragraph->hit_test(local_position), 0), static_cast<int32_t>(cell.global_end - cell.global_start));
					return cell.global_start + local_character;
				}
			}
			return item.cells.empty() ? item.global_start : item.cells[0].global_start;
		}
		if (!item.container_lines.empty() || item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
			for (const MarkdownContainerLine& line : item.container_lines) {
				if (p_position.y <= line.rect.get_end().y) {
					if (line.html_layout.is_valid()) {
						return line.global_start + line.html_layout->hit_test_text(dockit::to_local_point(p_position, line.text_rect.position));
					}
					if (line.paragraph.is_null()) {
						return p_position.x < line.rect.get_center().x ? line.global_start : line.global_end;
					}
					const Vector2 local_position = p_position - line.text_rect.position;
					const int32_t local_character = std::min<int32_t>(std::max<int32_t>(line.paragraph->hit_test(local_position), 0), static_cast<int32_t>(line.global_end - line.global_start));
					return line.global_start + local_character;
				}
			}
			return p_position.x < item.rect.get_center().x ? item.global_start : item.global_end;
		}
		if (!item.paragraph.is_valid()) {
			return p_position.x < item.rect.get_center().x ? item.global_start : item.global_end;
		}
		const Vector2 local_position = p_position - item.text_rect.position;
		const int32_t local_character = std::min<int32_t>(std::max<int32_t>(item.paragraph->hit_test(local_position), 0), static_cast<int32_t>(item.paragraph_global_end - item.paragraph_global_start));
		return item.paragraph_global_start + local_character;
	}

	return plain_text.length();
}

String MarkdownLabelCanvas::link_uri_at_position(const Vector2& p_position) const {
	Vector2 local_position;
	const Ref<HTMLLayout> html = html_at_position(p_position, local_position);
	if (html.is_valid()) {
		return get_html_visible_rect().has_point(p_position) ? html->get_link_at(local_position) : String();
	}
	return link_uri_at_character(hit_test_document(p_position));
}

Ref<HTMLLayout> MarkdownLabelCanvas::html_at_position(const Vector2& p_position, Vector2& r_local_position, int64_t* r_offset) const {
	if (label == nullptr) {
		return {};
	}
	for (const MarkdownCanvasItem& item : items) {
		if (item.rect.position.y > p_position.y) {
			break;
		}
		if (!item.rect.has_point(p_position)) {
			continue;
		}
		if (item.html_layout.is_valid() && item.text_rect.has_point(p_position)) {
			r_local_position = dockit::to_local_point(p_position, item.text_rect.position);
			if (r_offset) *r_offset = item.global_start;
			return item.html_layout;
		}
		for (const MarkdownContainerLine& line : item.container_lines) {
			if (line.html_layout.is_valid() && line.text_rect.has_point(p_position)) {
				r_local_position = dockit::to_local_point(p_position, line.text_rect.position);
				if (r_offset) *r_offset = line.global_start;
				return line.html_layout;
			}
		}
		break;
	}
	return {};
}

dockit::HTMLSelectionPoint MarkdownLabelCanvas::selection_point_at(const Vector2& p_position) const {
	Vector2 local;
	int64_t offset = 0;
	const Ref<HTMLLayout> html = html_at_position(p_position, local, &offset);
	if (html.is_null()) return { hit_test_document(p_position) };
	auto point = html->selection_point_at(local);
	point.translate(offset);
	return point;
}

void MarkdownLabelCanvas::extend_document_selection(const Vector2& p_position) {
	const Vector2i selection = dockit::html_selection_between(selection_origin, selection_point_at(p_position));
	selection_anchor = selection.x;
	selection_caret = selection.y;
}

int64_t MarkdownLabelCanvas::line_column_to_offset(int32_t p_line, int32_t p_column) const {
	const int32_t target_line = std::max<int32_t>(0, p_line);
	const int32_t target_column = std::max<int32_t>(0, p_column);
	int32_t current_line = 0;
	int32_t current_column = 0;

	for (int64_t index = 0; index < plain_text.length(); index++) {
		if (current_line == target_line && current_column >= target_column) {
			return index;
		}

		if (plain_text[index] == '\n') {
			if (current_line == target_line) {
				return index;
			}
			current_line++;
			current_column = 0;
		}
		else {
			current_column++;
		}
	}

	return plain_text.length();
}

Vector2i MarkdownLabelCanvas::offset_to_line_column(int64_t p_offset) const {
	const int64_t target_offset = std::max<int64_t>(0, std::min<int64_t>(p_offset, plain_text.length()));
	int32_t line = 0;
	int32_t column = 0;

	for (int64_t index = 0; index < target_offset; index++) {
		if (plain_text[index] == '\n') {
			line++;
			column = 0;
		}
		else {
			column++;
		}
	}

	return Vector2i(line, column);
}

String MarkdownLabelCanvas::link_uri_at_character(int64_t p_character) const {
	for (const MarkdownCanvasItem& item : items) {
		if (p_character < item.global_start || p_character > item.global_end) {
			continue;
		}
		if (item.type == MARKDOWN_BLOCK_TABLE) {
			for (const MarkdownTableCell& cell : item.cells) {
				if (p_character >= cell.global_start && p_character <= cell.global_end) {
					const int64_t local_character = p_character - cell.global_start;
					for (const MarkdownInlineSpan& span : cell.spans) {
						if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
							return span.link_uri;
						}
					}
					return String();
				}
			}
			continue;
		}
		if (!item.container_lines.empty() || item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
			for (const MarkdownContainerLine& line : item.container_lines) {
				if (p_character >= line.global_start && p_character <= line.global_end) {
					const int64_t local_character = p_character - line.global_start;
					for (const MarkdownInlineSpan& span : line.spans) {
						if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
							return span.link_uri;
						}
					}
					return String();
				}
			}
			continue;
		}
		if (p_character >= item.paragraph_global_start && p_character <= item.paragraph_global_end) {
			const int64_t local_character = p_character - item.paragraph_global_start;
			for (const MarkdownInlineSpan& span : item.spans) {
				if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
					return span.link_uri;
				}
			}
		}
	}

	return String();
}

String MarkdownLabelCanvas::image_tooltip_at_position(const Vector2& p_position) const {
	const int64_t ch = hit_test_document(p_position);
	for (const MarkdownCanvasItem& item : items) {
		if (ch >= item.global_start && ch <= item.global_end) {
			if (ch >= item.paragraph_global_start && ch <= item.paragraph_global_end) {
				const int64_t local_ch = ch - item.paragraph_global_start;
				for (const MarkdownInlineSpan& span : item.spans) {
					if (!span.image_uri.is_empty() && local_ch >= span.start && local_ch <= span.end) {
						if (!span.image_title.is_empty()) return span.image_title;
						return String();
					}
				}
			}
		}
	}
	return String();
}

String MarkdownLabelCanvas::footnote_id_at_character(int64_t p_character, bool p_reference) const {
	for (const MarkdownCanvasItem& item : items) {
		if (p_character < item.global_start || p_character > item.global_end) {
			continue;
		}

		if (!p_reference && item.type == MARKDOWN_BLOCK_FOOTNOTES) {
			for (int32_t footnote_index = 0; footnote_index < static_cast<int32_t>(item.footnote_ids.size()); footnote_index++) {
				if (footnote_index < static_cast<int32_t>(item.footnote_starts.size()) && footnote_index < static_cast<int32_t>(item.footnote_ends.size())) {
					const int64_t start = item.paragraph_global_start + item.footnote_starts[footnote_index];
					const int64_t end = item.paragraph_global_start + item.footnote_ends[footnote_index];
					if (p_character >= start && p_character <= end) {
						return item.footnote_ids[footnote_index];
					}
				}
			}
		}

		if (item.type == MARKDOWN_BLOCK_TABLE) {
			for (const MarkdownTableCell& cell : item.cells) {
				if (p_character >= cell.global_start && p_character <= cell.global_end) {
					const int64_t local_character = p_character - cell.global_start;
					for (const MarkdownInlineSpan& span : cell.spans) {
						if (span.footnote_ref == p_reference && !span.footnote_id.is_empty() && local_character >= span.start && local_character <= span.end) {
							return span.footnote_id;
						}
					}
				}
			}
			continue;
		}

		if (!item.container_lines.empty() || item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
			for (const MarkdownContainerLine& line : item.container_lines) {
				if (p_character >= line.global_start && p_character <= line.global_end) {
					const int64_t local_character = p_character - line.global_start;
					for (const MarkdownInlineSpan& span : line.spans) {
						if (span.footnote_ref == p_reference && !span.footnote_id.is_empty() && local_character >= span.start && local_character <= span.end) {
							return span.footnote_id;
						}
					}
				}
			}
			continue;
		}

		if (p_character >= item.paragraph_global_start && p_character <= item.paragraph_global_end) {
			const int64_t local_character = p_character - item.paragraph_global_start;
			for (const MarkdownInlineSpan& span : item.spans) {
				if (span.footnote_ref == p_reference && !span.footnote_id.is_empty() && local_character >= span.start && local_character <= span.end) {
					return span.footnote_id;
				}
			}
		}
	}
	return String();
}

float MarkdownLabelCanvas::footnote_target_at_position(const Vector2& p_position) const {
	const int64_t character = hit_test_document(p_position);
	const String reference_id = footnote_id_at_character(character, true);
	if (!reference_id.is_empty() && footnote_definition_offsets.has(reference_id)) {
		return footnote_definition_offsets[reference_id];
	}

	const String definition_id = footnote_id_at_character(character, false);
	if (!definition_id.is_empty() && footnote_reference_offsets.has(definition_id)) {
		return footnote_reference_offsets[definition_id];
	}

	return -1.0f;
}

void MarkdownLabelCanvas::scroll_html_input(const Vector2& p_position, const Ref<InputEvent>& p_event) {
	if (!get_html_visible_rect().has_point(p_position)) return;
	Vector2 html_position;
	const auto html = html_at_position(p_position, html_position);
	if (html.is_null()) return;
	const auto result = html->scroll_overflow_from_input(html_position, p_event, *this,
		callable_mp(this, &MarkdownLabelCanvas::queue_pointer_update));
	if (result == dockit::HTMLScrollInput::ignored) return;
	pressed_html_link = 0;
	pressed_blank = false;
	queue_pointer_update();
	if (result == dockit::HTMLScrollInput::handled) accept_event();
}

namespace {
	constexpr float DRAG_DEADZONE_SQUARED = 4.0f * 4.0f;

	enum class ParagraphHit { OUTSIDE, TEXT, OBJECT };

	ParagraphHit hit_test_paragraph(const Ref<TextParagraph>& p_paragraph, const Rect2& p_rect, const Vector2& p_position) {
		if (p_paragraph.is_null() || !p_rect.has_point(p_position)) return ParagraphHit::OUTSIDE;
		const Vector2 local = p_position - p_rect.position;
		float y = 0.0f;
		for (int32_t line = 0; line < p_paragraph->get_line_count(); ++line) {
			const Vector2 size = p_paragraph->get_line_size(line);
			if (local.y < y + size.y) {
				const float x = get_alignment_offset(p_paragraph, line, p_rect.size.x);
				if (!Rect2(Vector2(x, y), size).has_point(local)) return ParagraphHit::OUTSIDE;
				const Array objects = p_paragraph->get_line_objects(line);
				for (int64_t index = 0; index < objects.size(); ++index) {
					const Rect2 object = p_paragraph->get_line_object_rect(line, objects[index]);
					if (local.x >= object.position.x && local.x < object.get_end().x) {
						return object.has_point(local) ? ParagraphHit::OBJECT : ParagraphHit::OUTSIDE;
					}
				}
				return ParagraphHit::TEXT;
			}
			y += size.y + p_paragraph->get_line_spacing();
		}
		return ParagraphHit::OUTSIDE;
	}

	ParagraphHit hit_test_markdown_text(const std::vector<MarkdownCanvasItem>& p_items, const Vector2& p_position) {
		const auto next = std::upper_bound(p_items.begin(), p_items.end(), p_position.y,
			[](real_t p_y, const MarkdownCanvasItem& p_item) { return p_y < p_item.rect.position.y; });
		if (next == p_items.begin()) return ParagraphHit::OUTSIDE;
		const auto& item = *std::prev(next);
		if (!item.rect.has_point(p_position)) return ParagraphHit::OUTSIDE;
		for (const MarkdownTableCell& cell : item.cells) {
			if (cell.rect.has_point(p_position)) return hit_test_paragraph(cell.paragraph, cell.text_rect, p_position);
		}
		for (const MarkdownContainerLine& line : item.container_lines) {
			if (line.rect.has_point(p_position)) return hit_test_paragraph(line.paragraph, line.text_rect, p_position);
		}
		return hit_test_paragraph(item.paragraph, item.text_rect, p_position);
	}
}

void MarkdownLabelCanvas::update_pointer(const Vector2& p_position) {
	if (html_scrollbar_drag || selection_drag_attempt) return;
	if (dragging_selection) {
		if (selection_dragged && label->is_selection_enabled()) {
			set_default_cursor_shape(CURSOR_IBEAM);
			set_tooltip_text(String());
		}
		return;
	}

	const bool visible = get_html_visible_rect().has_point(p_position);
	Vector2 html_position;
	const auto html = visible ? html_at_position(p_position, html_position) : Ref<HTMLLayout>();
	update_html_scrollbar_hover(html, html_position);

	String tooltip;
	bool over_link = false;
	bool over_text = false;
	if (html.is_valid()) {
		if (!html->scrollbar_at(html_position)) {
			tooltip = html->get_link_at(html_position);
			over_link = !tooltip.is_empty();
			over_text = !over_link && label->is_selection_enabled() && html->hit_test_selectable_text(html_position);
		}
	}
	else if (visible) {
		const auto hit = hit_test_markdown_text(items, p_position);
		if (hit != ParagraphHit::OUTSIDE) {
			tooltip = link_uri_at_character(hit_test_document(p_position));
			if (tooltip.is_empty()) tooltip = image_tooltip_at_position(p_position);
			over_link = !tooltip.is_empty() || footnote_target_at_position(p_position) >= 0.0f;
			over_text = label->is_selection_enabled() && hit == ParagraphHit::TEXT;
		}
	}
	set_default_cursor_shape(over_link ? CURSOR_POINTING_HAND : (over_text ? CURSOR_IBEAM : CURSOR_ARROW));
	set_tooltip_text(tooltip);
}

void MarkdownLabelCanvas::queue_pointer_update() {
	if (is_inside_tree() && !pointer_update_queued) {
		pointer_update_queued = true;
		callable_mp(this, &MarkdownLabelCanvas::refresh_pointer).call_deferred();
	}
}

void MarkdownLabelCanvas::refresh_pointer() {
	pointer_update_queued = false;
	if (is_inside_tree() && is_visible_in_tree() && get_viewport()->gui_get_hovered_control() == this) {
		update_pointer(get_local_mouse_position());
	}
}

void MarkdownLabelCanvas::on_margin_gui_input(const Ref<InputEvent>& p_event) {
	const Transform2D to_canvas = get_transform().affine_inverse();
	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid() && pressed_blank &&
		to_canvas.xform(motion->get_position()).distance_squared_to(press_position) > DRAG_DEADZONE_SQUARED) {
		pressed_blank = false;
	}
	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_null() || button->get_button_index() != MOUSE_BUTTON_LEFT) return;
	const Vector2 point = to_canvas.xform(button->get_position());
	if (button->is_pressed()) {
		press_position = point;
		pressed_blank = true;
		return;
	}
	if (std::exchange(pressed_blank, false) && !button->is_canceled() &&
		point.distance_squared_to(press_position) <= DRAG_DEADZONE_SQUARED && !get_html_visible_rect().has_point(point)) {
		clear_selection();
	}
}

void MarkdownLabelCanvas::_gui_input(const Ref<InputEvent>& p_event) {
	if (label == nullptr) {
		return;
	}

	const Ref<InputEventPanGesture> pan = p_event;
	if (pan.is_valid()) {
		scroll_html_input(pan->get_position(), p_event);
		return;
	}
	const Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid()) {
		const auto index = mouse_button->get_button_index();
		if (index == MOUSE_BUTTON_WHEEL_UP || index == MOUSE_BUTTON_WHEEL_DOWN ||
			index == MOUSE_BUTTON_WHEEL_LEFT || index == MOUSE_BUTTON_WHEEL_RIGHT) {
			if (mouse_button->is_ctrl_pressed() && (index == MOUSE_BUTTON_WHEEL_UP || index == MOUSE_BUTTON_WHEEL_DOWN)) {
				const int32_t delta = index == MOUSE_BUTTON_WHEEL_UP ? 1 : -1;
				label->set_extra_font_size(std::max<int32_t>(0, label->get_extra_font_size() + delta));
				accept_event();
				return;
			}
			scroll_html_input(mouse_button->get_position(), p_event);
			return;
		}

		if (mouse_button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (!mouse_button->is_pressed()) queue_pointer_update();
			if (!mouse_button->is_pressed() && html_scrollbar_drag) {
				const auto drag = std::move(*html_scrollbar_drag);
				html_scrollbar_drag.reset();
				if (!mouse_button->is_canceled()) {
					drag.layout->drag_scrollbar(drag.drag, mouse_button->get_position() - drag.origin);
					drag.layout->finish_scrollbar_drag(drag.drag);
				}
				drag.layout->set_scrollbar_pressed(std::nullopt);
				Vector2 html_position;
				const auto html = get_html_visible_rect().has_point(mouse_button->get_position())
					? html_at_position(mouse_button->get_position(), html_position) : Ref<HTMLLayout>();
				update_html_scrollbar_hover(html, html_position);
				accept_event();
				return;
			}
			if (mouse_button->is_pressed()) {
				press_position = mouse_button->get_position();
				const bool double_click = mouse_button->is_double_click();
				pressed_blank = false;
				selection_dragged = false;
				selection_drag_attempt = false;
				dragging_selection = false;
				const bool visible = get_html_visible_rect().has_point(press_position);
				Vector2 html_position;
				int64_t html_text_offset = 0;
				pressed_html_layout = visible ? html_at_position(press_position, html_position, &html_text_offset) : Ref<HTMLLayout>();
				pressed_markdown = visible && pressed_html_layout.is_null();
				bool over_selectable_text = false;
				if (pressed_html_layout.is_valid()) {
					if (const auto drag = pressed_html_layout->scrollbar_at(html_position)) {
						html_scrollbar_drag = HTMLScrollbarGesture{ pressed_html_layout, *drag, press_position - html_position };
						pressed_html_link = 0;
						pressed_html_layout.unref();
						update_internal_processing();
						html_scrollbar_drag->layout->drag_scrollbar(*drag, html_position);
						set_default_cursor_shape(CURSOR_ARROW);
						set_tooltip_text(String());
						accept_event();
						return;
					}
					pressed_html_link = pressed_html_layout->find_link_at(html_position).id;
					over_selectable_text = pressed_html_layout->hit_test_selectable_text(html_position, &pressed_blank);
					pressed_blank = pressed_blank && pressed_html_link == 0;
				}
				else if (pressed_markdown) {
					const auto content = hit_test_markdown_text(items, press_position);
					over_selectable_text = content == ParagraphHit::TEXT;
					pressed_blank = content == ParagraphHit::OUTSIDE;
				}
				const bool selectable = label->is_selection_enabled() && over_selectable_text;
				if (selectable && pressed_markdown && !double_click && label->is_drag_and_drop_selection_enabled() && is_position_inside_selection(press_position)) {
					selection_drag_attempt = true;
					pressed_markdown = false;
					if (!has_focus()) {
						grab_focus();
					}
					accept_event();
					return;
				}
				if (selectable) {
					selection_origin = selection_point_at(press_position);
					dragging_selection = !selection_origin.excluded;
					if (dragging_selection) {
						const Vector2i selection = dockit::html_selection_between(selection_origin, selection_origin);
						selection_anchor = selection.x;
						selection_caret = selection.y;
						if (double_click) {
							std::optional<dockit::WordBoundary> boundary;
							if (pressed_html_layout.is_valid()) {
								const int64_t local_offset = pressed_html_layout->selection_point_at(html_position).offset;
								const String text = pressed_html_layout->get_plain_text();
								const dockit::HTMLTextLanguageRange language = pressed_html_layout->get_text_language_range_at(
									local_offset, dockit::get_project_word_language());
								boundary = dockit::word_boundary_at(text, local_offset, language.language, language.start, language.end,
									label->get_word_selection_mode());
								if (boundary) {
									selection_anchor = html_text_offset + boundary->start;
									selection_caret = html_text_offset + boundary->end;
								}
							}
							else {
								boundary = dockit::word_boundary_at(plain_text, selection_origin.offset, dockit::get_project_word_language(),
									0, -1, label->get_word_selection_mode());
								if (boundary) {
									selection_anchor = boundary->start;
									selection_caret = boundary->end;
								}
							}
						}
						queue_redraw();
						if (!has_focus()) grab_focus();
					}
				}
				accept_event();
				return;
			}

			const Vector2 up_position = mouse_button->get_position();
			const bool clicked = !selection_dragged && !mouse_button->is_canceled() &&
				up_position.distance_squared_to(press_position) <= DRAG_DEADZONE_SQUARED && get_html_visible_rect().has_point(up_position);
			const bool blank_clicked = std::exchange(pressed_blank, false) && clicked;
			if (blank_clicked) clear_selection();

			if (std::exchange(dragging_selection, false)) {
				if (selection_dragged && !mouse_button->is_canceled()) {
					extend_document_selection(mouse_button->get_position());
					queue_redraw();
				}
				update_internal_processing();
				accept_event();
			}

			if (pressed_html_layout.is_valid()) {
				Vector2 html_position;
				const Ref<HTMLLayout> html = clicked ? html_at_position(up_position, html_position) : Ref<HTMLLayout>();
				const HTMLLayout::LinkHit hit = html == pressed_html_layout
					? html->find_link_at(html_position) : HTMLLayout::LinkHit{};
				const bool link_clicked = pressed_html_link != 0 && pressed_html_link == hit.id;
				pressed_html_link = 0;
				pressed_html_layout.unref();
				accept_event();
				if (link_clicked) {
					label->activate_link_uri(hit.uri);
				}
				return;
			}

			if (selection_drag_attempt) {
				selection_drag_attempt = false;
				if (is_position_inside_selection(mouse_button->get_position())) {
					clear_selection();
				}
				accept_event();
				return;
			}

			if (std::exchange(pressed_markdown, false) && clicked && !blank_clicked) {
				accept_event();
				Vector2 html_position;
				if (html_at_position(up_position, html_position).is_valid()) return;
				const String uri = link_uri_at_position(up_position);
				if (!uri.is_empty()) {
					label->activate_link_uri(uri);
					return;
				}
				const float footnote_target = footnote_target_at_position(up_position);
				VScrollBar* v_scroll = label->get_v_scroll_bar();
				if (footnote_target >= 0.0f && v_scroll != nullptr) {
					const float max_scroll = std::max<float>(0.0f, v_scroll->get_max() - v_scroll->get_page());
					v_scroll->set_value(std::min<float>(std::max<float>(0.0f, footnote_target - 8.0f), max_scroll));
				}
			}
			return;
		}

		if (mouse_button->get_button_index() == MOUSE_BUTTON_RIGHT && mouse_button->is_pressed() && label->is_context_menu_enabled()) {
			get_menu();
			_update_context_menu();
			menu->set_position(get_screen_transform().xform(mouse_button->get_position()));
			menu->reset_size();
			menu->popup();
			menu->grab_focus();
			accept_event();
			return;
		}
		return;
	}

	const Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid()) {
		if (html_scrollbar_drag) {
			const auto& drag = *html_scrollbar_drag;
			drag.layout->drag_scrollbar(drag.drag, mouse_motion->get_position() - drag.origin);
			accept_event();
			return;
		}
		const Vector2 current_position = mouse_motion->get_position();
		const bool moved = current_position.distance_squared_to(press_position) > DRAG_DEADZONE_SQUARED;
		if (moved) {
			pressed_blank = false;
			pressed_html_link = 0;
			pressed_markdown = false;
		}
		if (selection_drag_attempt) {
			return;
		}

		if (dragging_selection) {
			selection_dragged |= moved;
			if (selection_dragged || pressed_html_layout.is_null()) {
				extend_document_selection(current_position);
			}
			update_pointer(current_position);
			internal_processing_enabled = true;
			set_process_internal(true);
			queue_redraw();
			accept_event();
			return;
		}

		update_pointer(mouse_motion->get_position());
		return;
	}

	const Ref<InputEventKey> key_event = p_event;
	if (key_event.is_valid() && key_event->is_pressed()) {
		if (label->is_shortcut_keys_enabled() && key_event->is_action("ui_text_select_all", true)) {
			select_all();
			accept_event();
			return;
		}
		if (label->is_shortcut_keys_enabled() && key_event->is_action("ui_copy", true)) {
			const String selected = get_selected_text();
			if (!selected.is_empty()) {
				DisplayServer::get_singleton()->clipboard_set(selected);
			}
			accept_event();
			return;
		}
		if (key_event->is_action("ui_menu", true)) {
			if (label->is_context_menu_enabled()) {
				get_menu();
				_update_context_menu();
				menu->set_position(get_screen_position());
				menu->reset_size();
				menu->popup();
				menu->grab_focus();
			}
			accept_event();
			return;
		}
		return;
	}
}

// MarkdownLabel
void MarkdownLabel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("append_text", "delta"), &MarkdownLabel::append_text);
	ClassDB::bind_method(D_METHOD("finish_stream"), &MarkdownLabel::finish_stream);
	ClassDB::bind_method(D_METHOD("clear"), &MarkdownLabel::clear);
	ClassDB::bind_method(D_METHOD("flush_pending_layout"), &MarkdownLabel::flush_pending_layout);
	ClassDB::bind_method(D_METHOD("select_all"), &MarkdownLabel::select_all);
	ClassDB::bind_method(D_METHOD("clear_selection"), &MarkdownLabel::clear_selection);
	ClassDB::bind_method(D_METHOD("get_selected_text"), &MarkdownLabel::get_selected_text);
	ClassDB::bind_method(D_METHOD("search", "text", "flags", "from_line", "from_column"), &MarkdownLabel::search);
	ClassDB::bind_method(D_METHOD("get_link_uri_at_point", "position"), &MarkdownLabel::get_link_uri_at_point);
	ClassDB::bind_method(D_METHOD("activate_link_uri", "uri"), &MarkdownLabel::activate_link_uri);
	ClassDB::bind_method(D_METHOD("has_anchor", "anchor"), &MarkdownLabel::has_anchor);
	ClassDB::bind_method(D_METHOD("get_anchors"), &MarkdownLabel::get_anchors);
	ClassDB::bind_method(D_METHOD("scroll_to_anchor", "anchor", "align"), &MarkdownLabel::scroll_to_anchor, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("scroll_to_search_result", "align"), &MarkdownLabel::scroll_to_search_result, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("get_scroll_offset_for_source_line", "line", "line_count"), &MarkdownLabel::get_scroll_offset_for_source_line);
	ClassDB::bind_method(D_METHOD("get_source_line_at_scroll_offset", "offset", "line_count"), &MarkdownLabel::get_source_line_at_scroll_offset);
	ClassDB::bind_method(D_METHOD("load_markdown_file", "path"), &MarkdownLabel::load_markdown_file);
	ClassDB::bind_method(D_METHOD("refresh"), &MarkdownLabel::refresh);
	ClassDB::bind_method(D_METHOD("get_rendered_block_count"), &MarkdownLabel::get_rendered_block_count);
	ClassDB::bind_method(D_METHOD("get_rendered_html"), &MarkdownLabel::get_rendered_html);
	ClassDB::bind_method(D_METHOD("get_content_height"), &MarkdownLabel::get_content_height);
	ClassDB::bind_method(D_METHOD("get_h_scroll_bar"), &MarkdownLabel::get_h_scroll_bar);
	ClassDB::bind_method(D_METHOD("get_v_scroll_bar"), &MarkdownLabel::get_v_scroll_bar);
	ClassDB::bind_method(D_METHOD("get_menu"), &MarkdownLabel::get_menu);
	ClassDB::bind_method(D_METHOD("is_menu_visible"), &MarkdownLabel::is_menu_visible);
	ClassDB::bind_method(D_METHOD("menu_option", "option"), &MarkdownLabel::menu_option);

	ClassDB::bind_method(D_METHOD("set_markdown_text", "text"), &MarkdownLabel::set_markdown_text);
	ClassDB::bind_method(D_METHOD("get_markdown_text"), &MarkdownLabel::get_markdown_text);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_markdown_text", "get_markdown_text");

	ClassDB::bind_method(D_METHOD("set_access", "access"), &MarkdownLabel::set_access);
	ClassDB::bind_method(D_METHOD("get_access"), &MarkdownLabel::get_access);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "access", PROPERTY_HINT_ENUM, "Resources,User Data,File System"), "set_access", "get_access");

	ClassDB::bind_method(D_METHOD("set_root_subfolder", "root_subfolder"), &MarkdownLabel::set_root_subfolder);
	ClassDB::bind_method(D_METHOD("get_root_subfolder"), &MarkdownLabel::get_root_subfolder);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "root_subfolder", PROPERTY_HINT_DIR), "set_root_subfolder", "get_root_subfolder");

	ClassDB::bind_method(D_METHOD("set_parse_latex", "enabled"), &MarkdownLabel::set_parse_latex);
	ClassDB::bind_method(D_METHOD("is_parsing_latex"), &MarkdownLabel::is_parsing_latex);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "parse_latex"), "set_parse_latex", "is_parsing_latex");

	ClassDB::bind_method(D_METHOD("set_parse_html", "enabled"), &MarkdownLabel::set_parse_html);
	ClassDB::bind_method(D_METHOD("is_parsing_html"), &MarkdownLabel::is_parsing_html);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "parse_html"), "set_parse_html", "is_parsing_html");

	ClassDB::bind_method(D_METHOD("set_parse_gfm", "enabled"), &MarkdownLabel::set_parse_gfm);
	ClassDB::bind_method(D_METHOD("is_parsing_gfm"), &MarkdownLabel::is_parsing_gfm);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "parse_gfm"), "set_parse_gfm", "is_parsing_gfm");

	ClassDB::bind_method(D_METHOD("set_parse_mermaid", "enabled"), &MarkdownLabel::set_parse_mermaid);
	ClassDB::bind_method(D_METHOD("is_parsing_mermaid"), &MarkdownLabel::is_parsing_mermaid);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "parse_mermaid"), "set_parse_mermaid", "is_parsing_mermaid");

	ClassDB::bind_method(D_METHOD("set_stylesheet", "stylesheet"), &MarkdownLabel::set_stylesheet);
	ClassDB::bind_method(D_METHOD("get_stylesheet"), &MarkdownLabel::get_stylesheet);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stylesheet", PROPERTY_HINT_RESOURCE_TYPE, "CSSStyleSheet"), "set_stylesheet", "get_stylesheet");

	ClassDB::bind_method(D_METHOD("set_extra_font_size", "size"), &MarkdownLabel::set_extra_font_size);
	ClassDB::bind_method(D_METHOD("get_extra_font_size"), &MarkdownLabel::get_extra_font_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "extra_font_size", PROPERTY_HINT_RANGE, "0,16"), "set_extra_font_size", "get_extra_font_size");

	ClassDB::bind_method(D_METHOD("set_selection_enabled", "enabled"), &MarkdownLabel::set_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_selection_enabled"), &MarkdownLabel::is_selection_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "selection_enabled"), "set_selection_enabled", "is_selection_enabled");

	ClassDB::bind_method(D_METHOD("set_word_selection_mode", "mode"), &MarkdownLabel::set_word_selection_mode);
	ClassDB::bind_method(D_METHOD("get_word_selection_mode"), &MarkdownLabel::get_word_selection_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "word_selection_mode", PROPERTY_HINT_ENUM, "Auto:0,TextServer:1,Jieba:2"),
		"set_word_selection_mode", "get_word_selection_mode");

	ClassDB::bind_method(D_METHOD("set_deselect_on_focus_loss_enabled", "enabled"), &MarkdownLabel::set_deselect_on_focus_loss_enabled);
	ClassDB::bind_method(D_METHOD("is_deselect_on_focus_loss_enabled"), &MarkdownLabel::is_deselect_on_focus_loss_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deselect_on_focus_loss_enabled"), "set_deselect_on_focus_loss_enabled", "is_deselect_on_focus_loss_enabled");

	ClassDB::bind_method(D_METHOD("set_drag_and_drop_selection_enabled", "enabled"), &MarkdownLabel::set_drag_and_drop_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_drag_and_drop_selection_enabled"), &MarkdownLabel::is_drag_and_drop_selection_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "drag_and_drop_selection_enabled"), "set_drag_and_drop_selection_enabled", "is_drag_and_drop_selection_enabled");

	ClassDB::bind_method(D_METHOD("set_context_menu_enabled", "enabled"), &MarkdownLabel::set_context_menu_enabled);
	ClassDB::bind_method(D_METHOD("is_context_menu_enabled"), &MarkdownLabel::is_context_menu_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "context_menu_enabled"), "set_context_menu_enabled", "is_context_menu_enabled");

	ClassDB::bind_method(D_METHOD("set_shortcut_keys_enabled", "enabled"), &MarkdownLabel::set_shortcut_keys_enabled);
	ClassDB::bind_method(D_METHOD("is_shortcut_keys_enabled"), &MarkdownLabel::is_shortcut_keys_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shortcut_keys_enabled"), "set_shortcut_keys_enabled", "is_shortcut_keys_enabled");

	ClassDB::bind_method(D_METHOD("set_streaming_enabled", "enabled"), &MarkdownLabel::set_streaming_enabled);
	ClassDB::bind_method(D_METHOD("is_streaming_enabled"), &MarkdownLabel::is_streaming_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "streaming_enabled"), "set_streaming_enabled", "is_streaming_enabled");

	ClassDB::bind_method(D_METHOD("set_max_unstable_lines", "lines"), &MarkdownLabel::set_max_unstable_lines);
	ClassDB::bind_method(D_METHOD("get_max_unstable_lines"), &MarkdownLabel::get_max_unstable_lines);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_unstable_lines", PROPERTY_HINT_RANGE, "1,256"), "set_max_unstable_lines", "get_max_unstable_lines");

	ClassDB::bind_method(D_METHOD("set_deferred_layout_enabled", "enabled"), &MarkdownLabel::set_deferred_layout_enabled);
	ClassDB::bind_method(D_METHOD("is_deferred_layout_enabled"), &MarkdownLabel::is_deferred_layout_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deferred_layout_enabled"), "set_deferred_layout_enabled", "is_deferred_layout_enabled");

	ClassDB::bind_method(D_METHOD("set_layout_flush_interval_msec", "msec"), &MarkdownLabel::set_layout_flush_interval_msec);
	ClassDB::bind_method(D_METHOD("get_layout_flush_interval_msec"), &MarkdownLabel::get_layout_flush_interval_msec);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "layout_flush_interval_msec", PROPERTY_HINT_RANGE, "1,1000,1"), "set_layout_flush_interval_msec", "get_layout_flush_interval_msec");

	ClassDB::bind_method(D_METHOD("set_fit_content", "enabled"), &MarkdownLabel::set_fit_content);
	ClassDB::bind_method(D_METHOD("is_fit_content_enabled"), &MarkdownLabel::is_fit_content_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fit_content"), "set_fit_content", "is_fit_content_enabled");

	ClassDB::bind_method(D_METHOD("set_scroll_active", "active"), &MarkdownLabel::set_scroll_active);
	ClassDB::bind_method(D_METHOD("is_scroll_active"), &MarkdownLabel::is_scroll_active);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_active"), "set_scroll_active", "is_scroll_active");

	ClassDB::bind_method(D_METHOD("set_scroll_follow", "follow"), &MarkdownLabel::set_scroll_follow);
	ClassDB::bind_method(D_METHOD("is_scroll_following"), &MarkdownLabel::is_scroll_following);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_following"), "set_scroll_follow", "is_scroll_following");

	ClassDB::bind_method(D_METHOD("set_scroll_follow_visible_characters", "follow"), &MarkdownLabel::set_scroll_follow_visible_characters);
	ClassDB::bind_method(D_METHOD("is_scroll_following_visible_characters"), &MarkdownLabel::is_scroll_following_visible_characters);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_following_visible_characters"), "set_scroll_follow_visible_characters", "is_scroll_following_visible_characters");

	ClassDB::bind_method(D_METHOD("set_autowrap_mode", "autowrap_mode"), &MarkdownLabel::set_autowrap_mode);
	ClassDB::bind_method(D_METHOD("get_autowrap_mode"), &MarkdownLabel::get_autowrap_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "autowrap_mode", PROPERTY_HINT_ENUM, "Off,Arbitrary,Word,Word (Smart)"), "set_autowrap_mode", "get_autowrap_mode");

	ClassDB::bind_method(D_METHOD("set_autowrap_trim_flags", "autowrap_trim_flags"), &MarkdownLabel::set_autowrap_trim_flags);
	ClassDB::bind_method(D_METHOD("get_autowrap_trim_flags"), &MarkdownLabel::get_autowrap_trim_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "autowrap_trim_flags", PROPERTY_HINT_FLAGS, "Trim Spaces After Break:64,Trim Spaces Before Break:128"), "set_autowrap_trim_flags", "get_autowrap_trim_flags");

	ADD_SIGNAL(MethodInfo("link_pressed", PropertyInfo(Variant::STRING, "uri")));
	ADD_SIGNAL(MethodInfo("text_changed"));
	ADD_SIGNAL(MethodInfo("render_warning", PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "line")));
	ADD_SIGNAL(MethodInfo("extra_font_size_changed", PropertyInfo(Variant::INT, "font_size")));

	BIND_ENUM_CONSTANT(MENU_COPY);
	BIND_ENUM_CONSTANT(MENU_SELECT_ALL);
	BIND_ENUM_CONSTANT(MENU_MAX);
	BIND_ENUM_CONSTANT(ACCESS_RESOURCES);
	BIND_ENUM_CONSTANT(ACCESS_USER_DATA);
	BIND_ENUM_CONSTANT(ACCESS_FILE_SYSTEM);
	BIND_ENUM_CONSTANT(WORD_SELECTION_MODE_AUTO);
	BIND_ENUM_CONSTANT(WORD_SELECTION_MODE_TEXT_SERVER);
	BIND_ENUM_CONSTANT(WORD_SELECTION_MODE_JIEBA);
}

void MarkdownLabel::_validate_property(PropertyInfo& p_property) const {
	if (p_property.name == StringName("word_selection_mode")) {
		p_property.hint_string = dockit::get_word_selection_mode_hint();
	}
}

void MarkdownLabel::ensure_controls() {
	if (scroll_container != nullptr) {
		return;
	}

	set_theme_type_variation("MarkdownLabel");

	scroll_container = memnew(ScrollContainer);
	scroll_container->set_name("_MarkdownScroll");
	scroll_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(scroll_container, false, Node::INTERNAL_MODE_BACK);
	VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
	if (v_scroll != nullptr) {
		v_scroll->connect("value_changed", callable_mp(this, &MarkdownLabel::on_scroll_value_changed));
	}
	HScrollBar* h_scroll = scroll_container->get_h_scroll_bar();
	if (h_scroll != nullptr) {
		h_scroll->connect("value_changed", callable_mp(this, &MarkdownLabel::on_scroll_value_changed));
	}

	margin_container = memnew(MarginContainer);
	margin_container->set_name("_MarkdownMargin");
	margin_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	margin_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	margin_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->add_child(margin_container, false, Node::INTERNAL_MODE_BACK);

	canvas = memnew(MarkdownLabelCanvas);
	canvas->set_name("_MarkdownCanvas");
	canvas->set_label(this);
	canvas->set_focus_mode(Control::FOCUS_CLICK);
	canvas->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas->set_custom_minimum_size(Vector2(0.0, 1.0));
	canvas->set_streaming_enabled(streaming_enabled);
	canvas->set_max_unstable_lines(max_unstable_lines);
	canvas->set_deferred_layout_enabled(deferred_layout_enabled);
	canvas->set_layout_flush_interval_msec(layout_flush_interval_msec);
	canvas->set_markdown_text(raw_text);
	margin_container->add_child(canvas, false, Node::INTERNAL_MODE_BACK);
	margin_container->connect("gui_input", callable_mp(canvas, &MarkdownLabelCanvas::on_margin_gui_input));

	apply_theme_settings();
	apply_scroll_settings();
}

void MarkdownLabel::on_scroll_value_changed(double p_value) {
	(void)p_value;
	update_scroll_following();
	if (canvas != nullptr) {
		canvas->pressed_html_link = 0;
		canvas->pressed_blank = false;
		canvas->queue_pointer_update();
		canvas->queue_redraw();
	}
}

void MarkdownLabel::apply_theme_settings() {
	if (margin_container == nullptr) {
		return;
	}

	margin_container->add_theme_constant_override("margin_left", get_theme_constant_or(this, "MarkdownLabel", "margin_left", 16));
	margin_container->add_theme_constant_override("margin_top", get_theme_constant_or(this, "MarkdownLabel", "margin_top", 16));
	margin_container->add_theme_constant_override("margin_right", get_theme_constant_or(this, "MarkdownLabel", "margin_right", 16));
	margin_container->add_theme_constant_override("margin_bottom", get_theme_constant_or(this, "MarkdownLabel", "margin_bottom", 0));
	if (fit_content) {
		update_minimum_size();
	}
}

void MarkdownLabel::apply_scroll_settings() {
	if (scroll_container == nullptr) {
		return;
	}

	scroll_container->set_vertical_scroll_mode(scroll_active ? ScrollContainer::SCROLL_MODE_AUTO : ScrollContainer::SCROLL_MODE_DISABLED);
}

void MarkdownLabel::update_scroll_following() {
	if (!scroll_follow) {
		scroll_following = false;
		return;
	}

	if (scroll_container == nullptr) {
		scroll_following = true;
		return;
	}

	VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
	if (v_scroll == nullptr || !v_scroll->is_visible_in_tree()) {
		scroll_following = true;
		return;
	}

	scroll_following = v_scroll->get_value() > (v_scroll->get_max() - v_scroll->get_page() - 1.0);
}

void MarkdownLabel::scroll_to_following() {
	if (!scroll_follow || !scroll_following || scroll_container == nullptr || canvas == nullptr) {
		return;
	}

	const float target_height = canvas->get_content_height();
	scroll_container->call_deferred("set_deferred", "scroll_vertical", static_cast<int32_t>(target_height));
}

void MarkdownLabel::_notification(int p_what) {
	if (p_what == NOTIFICATION_POSTINITIALIZE || p_what == NOTIFICATION_POST_ENTER_TREE) {
		ensure_controls();
	}

	if (p_what == NOTIFICATION_READY) {
		refresh();
	}
	if (p_what == NOTIFICATION_RESIZED && canvas != nullptr && !fit_content) {
		canvas->mark_layout_dirty();
	}

	if (p_what == NOTIFICATION_THEME_CHANGED) {
		apply_theme_settings();
		if (canvas != nullptr) {
			refresh();
		}
	}
}

Vector2 MarkdownLabel::_get_minimum_size() const {
	if (!fit_content) {
		return Vector2();
	}

	Control* self = const_cast<MarkdownLabel*>(this);
	const int32_t top = get_theme_constant_or(self, "MarkdownLabel", "margin_top", 16);
	const int32_t bottom = get_theme_constant_or(self, "MarkdownLabel", "margin_bottom", 0);
	const float text_height = canvas == nullptr ? 1.0f : canvas->get_content_height();
	return Vector2(0.0f, text_height + top + bottom);
}

void MarkdownLabel::set_markdown_text(const String& p_text) {
	set_markdown_source(p_text, String());
}

void MarkdownLabel::set_markdown_source(const String& p_text, const String& p_source_path) {
	source_path = p_source_path;
	update_scroll_following();
	raw_text = p_text;
	if (canvas != nullptr) {
		canvas->set_markdown_text(p_text);
	}
	notify_canvas_layout_changed();
	emit_signal("text_changed");
}

String MarkdownLabel::get_resource_base_dir() const {
	return dockit::resolve_html_resource_base_dir(access, root_subfolder, source_path);
}

void MarkdownLabel::set_stylesheet(const Ref<CSSStyleSheet>& p_stylesheet) {
	if (stylesheet == p_stylesheet) return;
	const Callable changed = callable_mp(this, &MarkdownLabel::refresh);
	if (stylesheet.is_valid() && stylesheet->is_connected("changed", changed)) stylesheet->disconnect("changed", changed);
	stylesheet = p_stylesheet;
	if (stylesheet.is_valid()) stylesheet->connect("changed", changed);
	refresh();
}

Ref<CSSStyleSheet> MarkdownLabel::get_stylesheet() const { return stylesheet; }

void MarkdownLabel::set_parse_html(bool p_enabled) {
	if (parse_html == p_enabled) {
		return;
	}
	parse_html = p_enabled;
	if (canvas != nullptr) {
		canvas->set_markdown_text(raw_text);
	}
	notify_canvas_layout_changed();
}

bool MarkdownLabel::is_parsing_html() const {
	return parse_html;
}

String MarkdownLabel::get_markdown_text() const {
	return raw_text;
}

void MarkdownLabel::set_parse_latex(bool p_enabled) {
	if (parse_latex == p_enabled) {
		return;
	}
	parse_latex = p_enabled;
	if (canvas != nullptr) {
		canvas->set_markdown_text(raw_text);
	}
	notify_canvas_layout_changed();
}

bool MarkdownLabel::is_parsing_latex() const {
	return parse_latex;
}

void MarkdownLabel::set_parse_gfm(bool p_enabled) {
	if (parse_gfm == p_enabled) {
		return;
	}
	parse_gfm = p_enabled;
	if (canvas != nullptr) {
		canvas->set_markdown_text(raw_text);
	}
	notify_canvas_layout_changed();
}

bool MarkdownLabel::is_parsing_gfm() const {
	return parse_gfm;
}

void MarkdownLabel::set_parse_mermaid(bool p_enabled) {
	if (parse_mermaid == p_enabled) {
		return;
	}
	parse_mermaid = p_enabled;
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
	notify_canvas_layout_changed();
}

bool MarkdownLabel::is_parsing_mermaid() const {
	return parse_mermaid;
}


void MarkdownLabel::set_access(int32_t p_access) {
	if (p_access < ACCESS_RESOURCES || p_access > ACCESS_FILE_SYSTEM || access == p_access) return;
	access = p_access;
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
	notify_canvas_layout_changed();
}

int32_t MarkdownLabel::get_access() const {
	return access;
}

void MarkdownLabel::set_root_subfolder(const String& p_root_subfolder) {
	if (root_subfolder == p_root_subfolder) return;
	root_subfolder = p_root_subfolder;
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
	notify_canvas_layout_changed();
}

String MarkdownLabel::get_root_subfolder() const {
	return root_subfolder;
}

void MarkdownLabel::append_text(const String& p_delta) {
	ensure_controls();
	update_scroll_following();

	raw_text += p_delta;
	canvas->set_streaming_enabled(streaming_enabled);
	canvas->set_max_unstable_lines(max_unstable_lines);
	canvas->set_deferred_layout_enabled(deferred_layout_enabled);
	canvas->set_layout_flush_interval_msec(layout_flush_interval_msec);
	canvas->append_text_incremental(p_delta);
	if (!deferred_layout_enabled || !streaming_enabled) {
		notify_canvas_layout_changed();
	}
	emit_signal("text_changed");
}

void MarkdownLabel::finish_stream() {
	update_scroll_following();

	if (canvas != nullptr) {
		canvas->finish_stream();
	}
	notify_canvas_layout_changed();
}

void MarkdownLabel::clear() {
	raw_text = String();
	if (canvas != nullptr) {
		canvas->clear_document();
	}
	if (scroll_follow) {
		scroll_following = true;
	}
	notify_canvas_layout_changed();
	emit_signal("text_changed");
}

void MarkdownLabel::set_streaming_enabled(bool p_enabled) {
	streaming_enabled = p_enabled;
	if (canvas != nullptr) {
		canvas->set_streaming_enabled(p_enabled);
	}
}

bool MarkdownLabel::is_streaming_enabled() const {
	return streaming_enabled;
}

void MarkdownLabel::set_max_unstable_lines(int32_t p_lines) {
	max_unstable_lines = p_lines;
	if (canvas != nullptr) {
		canvas->set_max_unstable_lines(p_lines);
	}
}

int32_t MarkdownLabel::get_max_unstable_lines() const {
	return max_unstable_lines;
}

void MarkdownLabel::set_deferred_layout_enabled(bool p_enabled) {
	if (deferred_layout_enabled == p_enabled) {
		return;
	}

	deferred_layout_enabled = p_enabled;
	if (canvas != nullptr) {
		canvas->set_deferred_layout_enabled(p_enabled);
	}
}

bool MarkdownLabel::is_deferred_layout_enabled() const {
	return deferred_layout_enabled;
}

void MarkdownLabel::set_layout_flush_interval_msec(int32_t p_msec) {
	layout_flush_interval_msec = std::max<int32_t>(1, std::min<int32_t>(p_msec, 1000));
	if (canvas != nullptr) {
		canvas->set_layout_flush_interval_msec(layout_flush_interval_msec);
	}
}

int32_t MarkdownLabel::get_layout_flush_interval_msec() const {
	return layout_flush_interval_msec;
}

bool MarkdownLabel::flush_pending_layout() {
	if (canvas == nullptr) {
		return false;
	}
	return canvas->flush_pending_layout();
}

void MarkdownLabel::set_fit_content(bool p_enabled) {
	if (fit_content == p_enabled) {
		return;
	}

	fit_content = p_enabled;
	refresh();
	update_minimum_size();
}

bool MarkdownLabel::is_fit_content_enabled() const {
	return fit_content;
}

void MarkdownLabel::set_scroll_active(bool p_active) {
	if (scroll_active == p_active) {
		return;
	}

	scroll_active = p_active;
	apply_scroll_settings();
	queue_redraw();
	if (canvas != nullptr) {
		canvas->queue_redraw();
	}
}

bool MarkdownLabel::is_scroll_active() const {
	return scroll_active;
}

void MarkdownLabel::set_scroll_follow(bool p_follow) {
	scroll_follow = p_follow;
	if (scroll_follow) {
		if (scroll_container == nullptr) {
			scroll_following = true;
		}
		else {
			VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
			scroll_following = v_scroll == nullptr || !v_scroll->is_visible_in_tree() || v_scroll->get_value() > (v_scroll->get_max() - v_scroll->get_page() - 1.0);
		}
	}
	else {
		scroll_following = false;
	}
}

bool MarkdownLabel::is_scroll_following() const {
	return scroll_follow;
}

void MarkdownLabel::set_scroll_follow_visible_characters(bool p_follow) {
	scroll_follow_visible_characters = p_follow;
}

bool MarkdownLabel::is_scroll_following_visible_characters() const {
	return scroll_follow_visible_characters;
}

void MarkdownLabel::set_autowrap_mode(TextServer::AutowrapMode p_mode) {
	if (autowrap_mode == p_mode) {
		return;
	}

	autowrap_mode = p_mode;
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
	update_minimum_size();
}

TextServer::AutowrapMode MarkdownLabel::get_autowrap_mode() const {
	return autowrap_mode;
}

void MarkdownLabel::set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags) {
	const BitField<TextServer::LineBreakFlag> trimmed_flags = p_flags & get_autowrap_trim_mask();
	if (autowrap_flags_trim == trimmed_flags) {
		return;
	}

	autowrap_flags_trim = trimmed_flags;
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
	update_minimum_size();
}

BitField<TextServer::LineBreakFlag> MarkdownLabel::get_autowrap_trim_flags() const {
	return autowrap_flags_trim;
}

void MarkdownLabel::set_extra_font_size(int32_t p_extra) {
	extra_font_size = std::max<int32_t>(0, std::min<int32_t>(p_extra, 16));
	emit_signal("extra_font_size_changed", extra_font_size);
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
}

int32_t MarkdownLabel::get_extra_font_size() const {
	return extra_font_size;
}

void MarkdownLabel::set_selection_enabled(bool p_enabled) {
	selection_enabled = p_enabled;
	if (canvas != nullptr) canvas->queue_pointer_update();
}

bool MarkdownLabel::is_selection_enabled() const {
	return selection_enabled;
}

void MarkdownLabel::set_word_selection_mode(WordSelectionMode p_mode) {
	if (p_mode < WORD_SELECTION_MODE_AUTO || p_mode > WORD_SELECTION_MODE_JIEBA) p_mode = WORD_SELECTION_MODE_AUTO;
	if (word_selection_mode == p_mode) return;
	word_selection_mode = p_mode;
	notify_property_list_changed();
}

MarkdownLabel::WordSelectionMode MarkdownLabel::get_word_selection_mode() const {
	return word_selection_mode;
}

void MarkdownLabel::set_deselect_on_focus_loss_enabled(bool p_enabled) {
	deselect_on_focus_loss_enabled = p_enabled;
}

bool MarkdownLabel::is_deselect_on_focus_loss_enabled() const {
	return deselect_on_focus_loss_enabled;
}

void MarkdownLabel::set_drag_and_drop_selection_enabled(bool p_enabled) {
	drag_and_drop_selection_enabled = p_enabled;
}

bool MarkdownLabel::is_drag_and_drop_selection_enabled() const {
	return drag_and_drop_selection_enabled;
}

void MarkdownLabel::set_context_menu_enabled(bool p_enabled) {
	context_menu_enabled = p_enabled;
}

bool MarkdownLabel::is_context_menu_enabled() const {
	return context_menu_enabled;
}

void MarkdownLabel::set_shortcut_keys_enabled(bool p_enabled) {
	shortcut_keys_enabled = p_enabled;
}

bool MarkdownLabel::is_shortcut_keys_enabled() const {
	return shortcut_keys_enabled;
}

PopupMenu* MarkdownLabel::get_menu() {
	ensure_controls();
	return canvas->get_menu();
}

bool MarkdownLabel::is_menu_visible() const {
	if (canvas == nullptr) {
		return false;
	}
	return canvas->is_menu_visible();
}

void MarkdownLabel::menu_option(int p_option) {
	if (canvas == nullptr) {
		return;
	}
	canvas->menu_option(p_option);
}

void MarkdownLabel::select_all() {
	ensure_controls();
	canvas->select_all();
}

void MarkdownLabel::clear_selection() {
	if (canvas != nullptr) {
		canvas->clear_selection();
	}
}

String MarkdownLabel::get_selected_text() const {
	if (canvas == nullptr) {
		return String();
	}
	return canvas->get_selected_text();
}

Vector2i MarkdownLabel::search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column) {
	ensure_controls();
	return canvas->search(p_text, p_flags, p_from_line, p_from_column);
}

String MarkdownLabel::get_link_uri_at_point(const Vector2& p_position) {
	ensure_controls();
	return canvas->get_link_uri_at_point(p_position);
}

void MarkdownLabel::activate_link_uri(const String& p_uri) {
	if (p_uri.begins_with("#")) {
		scroll_to_anchor(dockit::decode_html_fragment(p_uri.substr(1)));
	}
	emit_signal("link_pressed", p_uri);
}

bool MarkdownLabel::has_anchor(const String& p_anchor) const {
	if (canvas == nullptr) {
		return false;
	}
	return canvas->has_anchor(p_anchor);
}

PackedStringArray MarkdownLabel::get_anchors() const {
	if (canvas == nullptr) {
		return PackedStringArray();
	}
	return canvas->get_anchors();
}

bool MarkdownLabel::scroll_to_anchor(const String& p_anchor, float p_align) {
	ensure_controls();
	if (scroll_container == nullptr || canvas == nullptr) {
		return false;
	}

	const float offset = canvas->get_anchor_offset(p_anchor);
	if (offset < 0.0f) {
		return false;
	}

	ScrollContainer* target_scroll = scroll_container;
	float target_offset = canvas->scroll_to_html_anchor(p_anchor, p_align).value_or(offset);
	if (is_inside_tree() && (fit_content || !scroll_active)) {
		Control* child = this;
		for (Control* parent = get_parent_control(); parent; parent = parent->get_parent_control()) {
			auto* ancestor = Object::cast_to<ScrollContainer>(parent);
			if (ancestor && ancestor->get_vertical_scroll_mode() != ScrollContainer::SCROLL_MODE_DISABLED) {
				target_scroll = ancestor;
				// Measure from the scrollable child so a deferred container sort cannot affect the offset.
				const Vector2 target = canvas->get_global_transform_with_canvas().xform(Vector2(0.0f, target_offset));
				const Vector2 origin = child->get_global_transform_with_canvas().get_origin();
				target_offset = ancestor->get_global_transform_with_canvas().affine_inverse().basis_xform(target - origin).y;
				break;
			}
			child = parent;
		}
	}
	VScrollBar* v_scroll = target_scroll->get_v_scroll_bar();
	const float align = std::isfinite(p_align) ? std::clamp(p_align, 0.0f, 1.0f) : 0.0f;
	v_scroll->set_value(target_offset - v_scroll->get_page() * align);
	return true;
}

bool MarkdownLabel::scroll_to_search_result(float p_align) {
	ensure_controls();
	if (scroll_container == nullptr || canvas == nullptr) {
		return false;
	}

	const float y = canvas->get_search_result_y();
	if (y < 0.0f) {
		return false;
	}

	VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
	if (v_scroll == nullptr) {
		return false;
	}

	const float viewport_height = scroll_container->get_size().y;
	const float target = std::max<float>(0.0f, y - viewport_height * p_align);
	v_scroll->set_value(std::min<float>(target, v_scroll->get_max() - v_scroll->get_page()));
	return true;
}

double MarkdownLabel::get_scroll_offset_for_source_line(double p_line, int32_t p_line_count) {
	return canvas ? canvas->get_offset_for_source_line(p_line, p_line_count) : 0.0;
}

double MarkdownLabel::get_source_line_at_scroll_offset(double p_offset, int32_t p_line_count) {
	return canvas ? canvas->get_source_line_at_offset(p_offset, p_line_count) : 0.0;
}

Error MarkdownLabel::load_markdown_file(const String& p_path) {
	const String path = ProjectSettings::get_singleton()->localize_path(ResourceUID::ensure_path(p_path));
	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	if (file.is_null()) {
		return ERR_FILE_CANT_OPEN;
	}

	const String content = file->get_as_text();
	file->close();

	set_markdown_source(content, path);
	return OK;
}

void MarkdownLabel::refresh() {
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
}

void MarkdownLabel::notify_canvas_layout_changed() {
	if (fit_content) {
		update_minimum_size();
	}
	scroll_to_following();
}

int32_t MarkdownLabel::get_rendered_block_count() const {
	if (canvas == nullptr) {
		return 0;
	}
	return canvas->get_rendered_block_count();
}

String MarkdownLabel::get_rendered_html() const {
	return canvas == nullptr ? String() : canvas->get_rendered_html();
}

float MarkdownLabel::get_content_height() const {
	if (canvas == nullptr) {
		return 1.0f;
	}
	return canvas->get_content_height();
}

HScrollBar* MarkdownLabel::get_h_scroll_bar() const {
	if (scroll_container == nullptr) {
		return nullptr;
	}
	return scroll_container->get_h_scroll_bar();
}

VScrollBar* MarkdownLabel::get_v_scroll_bar() const {
	if (scroll_container == nullptr) {
		return nullptr;
	}
	return scroll_container->get_v_scroll_bar();
}
