# StyleBox and CSS conversion

`StyleBoxCSSHelper` converts a single Godot StyleBox to a CSS declaration block, or computes a declaration block and creates a StyleBox. It supports Flat, Empty, Line, and Texture. Inspect both `error` and `diagnostics`: a successful conversion can still lose unsupported drawing properties.

```gdscript
var panel: StyleBoxFlat = StyleBoxFlat.new()
panel.bg_color = Color.DARK_SLATE_GRAY
panel.set_border_width_all(2)
panel.set_content_margin_all(12.0)

var encoded: Dictionary = StyleBoxCSSHelper.stylebox_to_css(panel)
var decoded: Dictionary = StyleBoxCSSHelper.css_to_stylebox(
	encoded.css,
	{"textures": encoded.textures,},
)
```

Effective content margins include the border. Conversion subtracts the border when generating CSS padding and adds it back when restoring the StyleBox. Negative CSS padding is not used. The original `-1` margin fallback setting is not preserved.

Texture StyleBoxes use standard `border-image` declarations. Slice margins, draw-center, expand margins, and stretch/tile/tile-fit are mapped to CSS. Cropped and tinted textures are baked in memory and returned in the texture dictionary. Keep that dictionary with the CSS; its generated resource URIs are not disk files. Reverse conversion requires the referenced textures in `context.textures`.

CSS and Godot do not share identical shadow and antialiasing algorithms. Godot border blending, Flat expand margins and skew, line grow offsets, per-side border colors, elliptical radii, independent border-image widths, and multiple or inset shadows can require approximation or omission. Diagnostics describe supported conversion losses. CSS layout and font properties belong to the containing Control rather than a StyleBox.

# Markdown layout

MarkdownLabel compiles its blocks and inline markup into one shared HTML DOM and uses HTMLLayout for measurement, drawing, selection, search, and link hit testing. Markdown tables contain real table cells; inline HTML containers retain their attributes and CSS box layout. Enable `parse_html` to interpret HTML from the Markdown source.

Godot Theme items become default CSS. HTML style elements and linked stylesheets apply throughout the Markdown document. `MarkdownLabel.stylesheet` adds a shared CSSStyleSheet after document styles. Author CSS overrides theme defaults through Dockit's normal cascade. HTML scripts and behavior scripts are not executed.

Streaming retains unchanged DOM blocks and replaces changed blocks. CSS selectors may cause an append to restyle earlier content, so shared-document CSS can require a wider layout update than the Markdown parse tail. Source blocks retain their line ranges for preview scrolling; selection uses the shared DOM's visible-text index.
