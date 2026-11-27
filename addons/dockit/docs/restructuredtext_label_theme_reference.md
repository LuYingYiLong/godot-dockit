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
| Constant | `margin_left`, `margin_top`, `margin_right`, `margin_bottom` | 16 | Insets around the document. Each side can be set independently. |
| Font | `normal_font` | Godot fallback | Body text and admonition font. |
| Font | `bold_font` | `normal_font` | Strong text font. |
| Font | `italics_font` or `italic_font` | `normal_font` | Emphasis font. `italics_font` takes precedence if both are set. |
| Font | `bold_italics_font` or `bold_italic_font` | `bold_font` | Bold emphasis font. `bold_italics_font` takes precedence if both are set. |
| Font Size | `normal_font_size` | 16 | Body font size in pixels. |
| Color | `font_color` | White | Body text color. |
| Color | `font_outline_color` | `Color(0, 0, 0, 1)` | Glyph outline color. |
| Color | `font_shadow_color` | Transparent black | Shadow and shadow-outline color. |
| Color | `link_color` | `Color(0.36, 0.62, 1.0)` | Link text and underline color. |
| Color | `selection_color` | `Color(0.22, 0.45, 0.95, 0.45)` | Selected text background. |
| Color | `search_result_color` | `Color(1.0, 0.72, 0.16, 0.42)` | Search match background. |
| Color | `font_separator_color` | `Color(0.45, 0.47, 0.52)` | Horizontal separator color. |
| Constant | `line_separation` | 2 | Extra vertical space between lines. |
| Constant | `paragraph_separation` | 10 | Default gap after a block. |
| Constant | `text_space_before`, `text_space_after` | 0, `paragraph_separation` | Spacing around regular paragraphs. |

### Text Effects

Text effects use Godot's shaped-text drawing API, so they do not create extra shader drawing items. The shadow is disabled by the default transparent `font_shadow_color`.

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Constant | `outline_size` | 0 | Glyph outline thickness in pixels, clamped to `[0, 16]`. |
| Constant | `shadow_outline_size` | 1 | Shadow glyph outline thickness in pixels, clamped to `[0, 16]`. |
| Constant | `shadow_offset_x`, `shadow_offset_y` | 1, 1 | Shadow offset in pixels. |

### Footnotes

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Font | `footnote_ref_font` | Body font | Font for the superscript reference number. |
| Font Size | `footnote_ref_font_size` | About 75% of `normal_font_size` (12 px at 16 px) | Size of the superscript reference number. |
| Color | `footnote_ref_font_color` | `Color(0.809, 0.809, 0.809)` | Reference number color. |
| StyleBox | `footnote_ref` | Empty | Optional background and margins around a reference number. |
| Font | `footnote_text_font` | Body font | Font for footnote definitions. |
| Font Size | `footnote_text_font_size` | `normal_font_size` | Size of footnote definition text. |
| Color | `footnote_text_font_color` | `font_color` | Color of footnote definition text. |

`extra_font_size` is added to both footnote font sizes. Footnote references remain clickable when a reference StyleBox is assigned.

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

The shared items below provide the default style for every admonition directive. Supported directives are `admonition`, `attention`, `caution`, `danger`, `error`, `hint`, `important`, `note`, `tip`, `seealso`, and `warning`.

| Category | Item | Default | Description |
| --- | --- | --- | --- |
| Font Size | `admonition_font_size` | Body size | Admonition text size. |
| Color | `admonition_font_color` | Body text color | Admonition body text color. |
| Color | `admonition_header_font_color` | White | Title text color. |
| Color | `admonition_accent_color` | Directive-specific | Accent used by the built-in panel and title strip. |
| StyleBox | `admonition_panel` | Built-in panel | Shared body background, border, and content margins. |
| StyleBox | `admonition_header_panel` | Built-in title strip | Shared title background, border, and content margins. |
| Constant | `admonition_space_before`, `admonition_space_after` | 0, `paragraph_separation` | Spacing around admonitions. |

Each directive can override any shared item with an item named `admonition_<directive>_<item>`. Use the lower-case directive name from the supported list. The generic `admonition` directive uses `custom` as its name because its title is supplied by the document.

| Item suffix | Example | Description |
| --- | --- | --- |
| `font_size` | `admonition_warning_font_size` | Body and title text size. |
| `font_color` | `admonition_warning_font_color` | Body text color. |
| `header_font_color` | `admonition_warning_header_font_color` | Title text color. |
| `accent_color` | `admonition_warning_accent_color` | Accent for generated panels when no matching StyleBox is set. |
| `panel` | `admonition_warning_panel` | Directive-specific body StyleBox. |
| `header_panel` | `admonition_warning_header_panel` | Directive-specific title StyleBox. |
| `space_before`, `space_after` | `admonition_warning_space_after` | Directive-specific spacing. |

For example, `admonition_note_panel` overrides the shared body panel for `note`, while `admonition_custom_header_panel` styles generic `admonition` directives. For each item, lookup order is directive-specific, shared, then built-in fallback. Accent lookup follows the same order: directive-specific accent, `admonition_accent_color`, then the built-in accent for that directive. The built-in accents are green for `note` and generic admonitions, amber for `attention`, `caution`, and `warning`, red for `danger` and `error`, blue for `important`, teal for `hint` and `tip`, and purple for `seealso`.

A custom StyleBox uses its own colors, so the accent color does not recolor a configured shared or directive-specific StyleBox. If neither StyleBox is set, the built-in panels use the resolved accent color. The example theme sets a shared style and adds a warning-specific override.

## Customize the Theme in a Script

Assign the example theme in the Inspector first. Then duplicate the active theme and set a directive-specific StyleBox:

```gdscript
extends Control


func _ready() -> void:
	var label: RestructuredtextLabel = %RestructuredtextLabel
	var custom_theme: Theme = label.get_theme().duplicate() as Theme
	var warning_panel: StyleBoxFlat = StyleBoxFlat.new()
	warning_panel.bg_color = Color(0.16, 0.12, 0.08, 1.0)
	warning_panel.border_color = Color(0.95, 0.61, 0.14, 1.0)
	warning_panel.set_border_width(SIDE_LEFT, 4)
	custom_theme.set_stylebox(&"admonition_warning_panel", &"RestructuredtextLabel", warning_panel)
	custom_theme.set_color(&"admonition_warning_header_font_color", &"RestructuredtextLabel", Color(0.08, 0.09, 0.11, 1.0))
	label.theme = custom_theme
```

The bundled theme uses system fonts, so it does not require font files in the project.
