#include "html_asset_collector.h"

#include "formats/html/css/css_stylesheet_rules.h"
#include "formats/html/css/css_value_helpers.h"
#include "html_image_sources.h"
#include "html_resource_paths.h"
#include "html_resources.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <gumbo.h>
#include <litehtml/css_parser.h>
#include <litehtml/html.h>

#include <memory>
#include <string>
#include <vector>

using namespace godot;

namespace docrender {
	namespace {
		constexpr int MAX_STYLESHEETS = 128;

		struct GumboDeleter {
			void operator()(GumboOutput* p_output) const {
				gumbo_destroy_output(&kGumboDefaultOptions, p_output);
			}
		};

		using GumboTree = std::unique_ptr<GumboOutput, GumboDeleter>;

		const GumboNode* child(const GumboElement& p_element, size_t p_index) {
			return static_cast<const GumboNode*>(p_element.children.data[p_index]);
		}

		void add_image_path(HashSet<String>& r_paths, const char* p_src, const String& p_base_dir) {
			const String path = resolve_html_image_path(String::utf8(p_src), p_base_dir);
			if (!path.is_empty()) {
				r_paths.insert(path);
			}
		}

		void collect_css_paths(const char* p_css, const String& p_base_dir, HashSet<String>& r_images,
			HashSet<String>* r_fonts = nullptr) {
			using namespace litehtml;
			const auto add = [&](const char* p_url) {
				add_image_path(r_images, p_url, p_base_dir);
				if (r_fonts) {
					const String path = resolve_html_font_path(String::utf8(p_url), p_base_dir);
					if (!path.is_empty()) r_fonts->insert(path);
				}
			};
			const auto add_string_source = [&](const css_token& p_source) {
				const css_token* source = &p_source;
				while (source->type == CV_FUNCTION && equal_i(source->name, "var") &&
					source->value.size() >= 3 && source->value[1].type == ',') {
					source = &source->value[2];
				}
				if (source->type == STRING) add_image_path(r_images, source->str.c_str(), p_base_dir);
			};
			const auto tokens = normalize(std::string(p_css), f_componentize | f_remove_whitespace);
			std::vector<const css_token_vector*> pending{ &tokens };
			while (!pending.empty()) {
				const auto& components = *pending.back();
				pending.pop_back();
				for (const auto& token : components) {
					std::string url;
					if (parse_url(token, url)) {
						add(url.c_str());
					}
					else if (token.is_component_value()) {
						pending.push_back(&token.value);
						if (token.type != CV_FUNCTION || token.value.empty()) continue;
						if (equal_i(token.name, "image-set")) {
							for (size_t index = 0; index < token.value.size(); ++index) {
								if (index == 0 || token.value[index - 1].type == ',') add_string_source(token.value[index]);
							}
						}
						else if (equal_i(token.name, "image")) {
							const auto tag = token.value.front().ident();
							const size_t index = is_one_of(tag, "ltr", "rtl") ? 1 : 0;
							if (index < token.value.size()) add_string_source(token.value[index]);
						}
					}
				}
			}
		}

		class AssetCollector {
			HashSet<String> checked_stylesheets;

		public:
			HashSet<String> images;
			HashSet<String> fonts;
			Dictionary stylesheets;

			void add_stylesheet(const String& p_url, const String& p_dir) {
				const String path = resolve_html_stylesheet_path(p_url, p_dir);
				if (path.is_empty() || checked_stylesheets.has(path) || checked_stylesheets.size() >= MAX_STYLESHEETS) {
					return;
				}
				checked_stylesheets.insert(path);
				if (const auto text = read_html_stylesheet(path)) {
					stylesheets[path] = *text;
					add_css(text->utf8().get_data(), path.get_base_dir());
				}
			}

			void add_css(const char* p_css, const String& p_dir) {
				using namespace litehtml;
				const std::string text(p_css);
				collect_css_paths(p_css, p_dir, images, &fonts);
				for (const auto& rule : css_parser::parse_stylesheet(text, true)) {
					if (rule->type != raw_rule::at || String::utf8(rule->name.c_str()).nocasecmp_to("import") != 0) {
						continue;
					}
					int index = 0;
					skip_whitespace(rule->prelude, index);
					if (index >= static_cast<int>(rule->prelude.size())) {
						continue;
					}
					const auto& token = rule->prelude[index];
					std::string url;
					if (!css::parse_import_url(token, url)) {
						continue;
					}
					add_stylesheet(String::utf8(url.c_str()), p_dir);
				}
			}
		};
	}

	HTMLAssets collect_html_assets(const String& p_html, const String& p_base_dir) {
		if (p_html.is_empty()) {
			return {};
		}
		AssetCollector assets;
		const CharString bytes = p_html.utf8();
		const GumboTree tree(gumbo_parse_with_options(&kGumboDefaultOptions, bytes.get_data(), bytes.length()));
		if (!tree || !tree->root) {
			return {};
		}
		std::vector<const GumboNode*> nodes{ tree->root };
		while (!nodes.empty()) {
			const GumboNode* node = nodes.back();
			nodes.pop_back();
			if (node->type != GUMBO_NODE_ELEMENT && node->type != GUMBO_NODE_TEMPLATE) {
				continue;
			}
			const GumboElement& element = node->v.element;
			if (element.tag == GUMBO_TAG_IMG) {
				if (const GumboAttribute* src = gumbo_get_attribute(&element.attributes, "src")) {
					add_image_path(assets.images, src->value, p_base_dir);
				}
			}
			if (element.tag == GUMBO_TAG_IMG || (element.tag == GUMBO_TAG_SOURCE &&
				node->parent && node->parent->type == GUMBO_NODE_ELEMENT &&
				node->parent->v.element.tag == GUMBO_TAG_PICTURE)) {
				if (const auto* srcset = gumbo_get_attribute(&element.attributes, "srcset")) {
					for (const auto& candidate : parse_html_srcset(srcset->value)) {
						add_image_path(assets.images, candidate.url.c_str(), p_base_dir);
					}
				}
			}
			if (element.tag == GUMBO_TAG_LINK) {
				const GumboAttribute* rel = gumbo_get_attribute(&element.attributes, "rel");
				const GumboAttribute* href = gumbo_get_attribute(&element.attributes, "href");
				if (href && is_html_stylesheet_link(rel ? rel->value : nullptr)) {
					assets.add_stylesheet(String::utf8(href->value), p_base_dir);
				}
			}
			if (const GumboAttribute* style = gumbo_get_attribute(&element.attributes, "style")) {
				collect_css_paths(style->value, p_base_dir, assets.images);
			}
			for (size_t i = 0; i < element.children.length; ++i) {
				const GumboNode* nested = child(element, i);
				if (element.tag == GUMBO_TAG_STYLE && (nested->type == GUMBO_NODE_TEXT || nested->type == GUMBO_NODE_CDATA)) {
					assets.add_css(nested->v.text.text, p_base_dir);
				}
				nodes.push_back(nested);
			}
		}

		Dictionary textures;
		Dictionary image_dpi;
		Dictionary image_orientations;
		ResourceLoader* loader = ResourceLoader::get_singleton();
		for (const String& path : assets.images) {
			if (loader->exists(path, "Texture2D")) {
				const Ref<Texture2D> texture = loader->load(path, "Texture2D");
				if (texture.is_valid()) {
					textures[path] = texture;
					const auto metadata = read_html_image_metadata(path);
					if (metadata.dpi > 0) image_dpi[path] = metadata.dpi;
					if (metadata.orientation != 1) image_orientations[path] = metadata.orientation;
				}
			}
		}
		Dictionary fonts;
		for (const String& path : assets.fonts) {
			if (loader->exists(path, "FontFile")) {
				const Ref<FontFile> font = loader->load(path, "FontFile");
				if (font.is_valid()) fonts[path] = font;
			}
		}
		return { textures, assets.stylesheets, fonts, image_dpi, image_orientations };
	}
}
