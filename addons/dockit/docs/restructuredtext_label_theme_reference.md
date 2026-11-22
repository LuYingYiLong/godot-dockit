# RestructuredtextLabel Theme Reference

`RestructuredtextLabel` uses Godot's theme system to style reStructuredText content. All RST styling items use one theme type: `RestructuredtextLabel`. Assign a `Theme` to the viewer or an ancestor `Control`, then edit that type in the Theme editor. The bundled example is [`restructuredtext_theme.tres`](../examples/restructuredtext_theme.tres).

## Apply the Example Theme

1. Add a `RestructuredtextLabel` node to a scene.
2. Assign `restructuredtext_theme.tres` to **Theme → Theme** on the viewer, or on a parent `Control` to share it with several viewers.
3. Open the theme resource and select `RestructuredtextLabel` in the Theme editor. All margins, fonts, colors, spacing, and StyleBoxes are in this type.

## Theme Items

The item category is shown in the **RestructuredtextLabel** theme type. For example, `fonts/normal_font` is the body font, and `constants/list_indent` controls list indentation.

### Page and Text

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Constant | `margin_left`, `margin_top`, `margin_right`, `margin_bottom` | `content_margin` (16) | Insets around the document. Each side can be set independently. |
| Font | `normal_font` | Godot fallback | Body text and admonition font. |
| Font | `bold_font` | `normal_font` | Strong text font. |
| Font | `italics_font` or `italic_font` | `normal_font` | Emphasis font. `italics_font` takes precedence if both are set. |
| Font | `bold_italics_font` or `bold_italic_font` | `bold_font` | Bold emphasis font. `bold_italics_font` takes precedence if both are set. |
| Font Size | `normal_font_size` | 16 | Body font size in pixels. |
| Color | `font_color` | White | Body text color. |
| Color | `link_color` | `Color(0.36, 0.62, 1.0)` | Link text and underline color. |
| Color | `selection_color` | `Color(0.22, 0.45, 0.95, 0.45)` | Selected text background. |
| Color | `search_result_color` | `Color(1.0, 0.72, 0.16, 0.42)` | Search match background. |
| Color | `font_separator_color` | `Color(0.45, 0.47, 0.52)` | Horizontal separator color. |
| Constant | `line_separation` | 2 | Extra vertical space between lines. |
| Constant | `paragraph_separation` | 10 | Default gap after a block. |
| Constant | `text_space_before`, `text_space_after` | 0, `paragraph_separation` | Spacing around regular paragraphs. |

### Headings

Headings use numbered item names in the same `RestructuredtextLabel` type. Each level supports `heading_N_font`, `heading_N_font_size`, `heading_N_font_color`, `heading_N_space_before`, and `heading_N_space_after`, where `N` is 1–5.

| Level | Font Size Fallback | Before / After Fallback |
| --- | ---: | --- |
| 1 | 28 px | 0 / `paragraph_separation` |
| 2 | 24 px | 0 / `paragraph_separation` |
| 3 | 21 px | 0 / `paragraph_separation` |
| 4 | 19 px | 0 / `paragraph_separation` |
| 5 | 17 px | 0 / `paragraph_separation` |

Heading font falls back to the body font, and heading color falls back to `font_color`. Assign bold fonts to the heading items if you want bold headings; the bundled theme does this. `extra_font_size` is added to the configured text sizes.

### Lists

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Font | `list_marker_font` | Body font | Bullet and numbered marker font. |
| Font Size | `list_marker_font_size` | 16 | Marker size in pixels. |
| Color | `list_marker_color` | Body text color | Marker color. |
| Constant | `list_indent` | 26 | Left inset reserved for markers. |
| Constant | `list_marker_gap` | 6 | Gap between a marker and its text. |
| Constant | `list_marker_min_width` | 14 | Minimum marker width. |
| Constant | `list_space_before`, `list_space_after` | 0, `paragraph_separation` | Spacing around list blocks. |

### Inline and Block Code

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Font | `inline_code_font` | Body font | Font for inline literals. |
| Font Size | `inline_code_font_size` | 16 | Inline literal size. |
| Color | `inline_code_font_color` | `Color(0.86, 0.88, 0.91)` | Inline literal text color. |
| StyleBox | `inline_code_style` | Built-in dark rounded box | Inline literal background and content margins. |
| Font | `code_block_font` | Inline code font | Font for literal and code blocks. |
| Font Size | `code_block_font_size` | 16 | Code block size. |
| Color | `code_block_font_color` | `Color(0.86, 0.88, 0.91)` | Code block text color. |
| StyleBox | `code_block_panel` | Built-in dark rounded panel | Code block background, border, and content margins. |
| Constant | `code_block_space_before`, `code_block_space_after` | 0, `paragraph_separation` | Spacing around code blocks. |

StyleBox content margins affect the rendered inset and layout size.

### Tables

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Font | `table_font` | Body font | Regular cell font. |
| Font Size | `table_font_size` | 16 | Regular cell size. |
| Color | `table_font_color` | Body text color | Regular cell text color. |
| Font | `table_header_font` | Body font | Header cell font. |
| Font Size | `table_header_font_size` | `table_font_size` | Header cell size. |
| Color | `table_header_font_color` | `table_font_color` | Header cell text color. |
| StyleBox | `table_panel` | Transparent | Optional outer table frame; its content margins inset the cells. |
| StyleBox | `table_header_panel` | Dark filled bordered cell | Header background, border, and content margins. |
| StyleBox | `table_cell_panel` | Transparent bordered cell | Regular cell background, border, and content margins. |
| StyleBox | `table_striped_panel` | Dark filled bordered cell | Alternating row style when striping is enabled. |
| Constant | `table_striped` | 0 | Set to a non-zero value to alternate row backgrounds. |
| Constant | `table_space_before`, `table_space_after` | 0, `paragraph_separation` | Spacing around tables. |

### Admonitions

All admonition directives share these items. Supported directives are `admonition`, `attention`, `caution`, `danger`, `error`, `hint`, `important`, `note`, `tip`, `seealso`, and `warning`.

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Font Size | `admonition_font_size` | Body size | Admonition text size. |
| Color | `admonition_font_color` | Body text color | Admonition body text color. |
| Color | `admonition_header_font_color` | White | Title text color. |
| StyleBox | `admonition_panel` | Built-in panel | Admonition body background, border, and content margins. |
| StyleBox | `admonition_header_panel` | Built-in title strip | Admonition title background, border, and content margins. |
| Constant | `admonition_space_before`, `admonition_space_after` | 0, `paragraph_separation` | Spacing around admonitions. |

When StyleBoxes are unset, the built-in accent color varies by directive. A configured `admonition_panel` and `admonition_header_panel` provide one shared style for all directives.

## Customize the Theme in a Script

Assign the example theme in the Inspector first. Then duplicate the active theme and edit an item in the shared type:

```gdscript
extends Control


func _ready() -> void:
	var label: RestructuredtextLabel = %RestructuredtextLabel
	var custom_theme: Theme = label.get_theme().duplicate() as Theme
	custom_theme.set_color(&"link_color", &"RestructuredtextLabel", Color(0.42, 0.72, 1.0))
	label.theme = custom_theme
```

The bundled theme uses system fonts, so it does not require font files in the project.
