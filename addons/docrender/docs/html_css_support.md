# HTML CSS support

DocRender maintains CSS syntax, selectors, declarations, the cascade, value types, the default stylesheet, and computed styles in its C++ CSS modules. The litehtml v0.10 integration (`9bc84b8`) supplies DOM, viewport, font, and resource data and consumes the resulting computed style. Box-dependent percentages, intrinsic sizes, and transform origins are resolved during layout. This reference applies to `HTMLLayout`, `HTMLLabel`, and HTML blocks in `MarkdownLabel`.

For GDScript queries, element handles, and geometry, see [HTML API and Web standards](html_api_reference.md).

## Selectors and rules

| Feature | Support |
| --- | --- |
| Basic selectors | Type, class, ID, attribute, selector lists, descendant, child, and sibling combinators |
| Pseudo-classes | `:root`, `:scope`, `:empty`, `:is()`, `:where()`, `:not()`, `:has()`, `:nth-child()` and related structural selectors, `:lang()`, `:link`, and `:any-link`. `HTMLLabel` supplies `:hover`, `:focus`, `:focus-visible`, and `:focus-within`, plus `:active` for buttons, checkboxes, and radios. Form controls expose `:enabled` and `:disabled`. Checkboxes, radios, and dropdown options expose `:checked`. |
| Pseudo-elements | `::before`, `::after`, and `::marker` with generated content in the final compound selector. The legacy `:before` and `:after` spellings are equivalent. Fields support `::placeholder` and `::selection` through the stylesheet cascade. Sub-pseudo-elements, `::first-line`, and `::first-letter` are unsupported. |
| Cascade | Named and anonymous layers, specificity, source order, `!important`, custom properties, and `var()`. Supported declarations share the CSS-wide keywords `inherit`, `initial`, `unset`, `revert`, and `revert-layer`. |
| Style sources | Inline `style`, `<style>`, the extra `stylesheet` string, and local project stylesheets loaded through `<link>` or `@import`. |
| At-rules | `@media` supports viewport conditions such as width, height, and orientation. `@supports` tests declarations and selectors. `@layer` supports ordering statements and nested blocks. `@import` accepts local paths with optional layer, `supports(…)`, and media clauses. `@counter-style` defines document-local counter styles. `@font-face` loads project fonts. `@font-feature-values` names font-specific glyph alternates. `@keyframes` is unsupported. |

## Properties and values

| Area | Support |
| --- | --- |
| Layout | Block and inline layout, lists, tables, Flexbox, block and inline Grid, dimensions, margins, padding, and static, relative, absolute, and fixed positioning. The `inset` shorthand sets `top`, `right`, `bottom`, and `left`. Sticky positioning is unsupported. |
| Tables | Automatic and fixed column sizing, header and footer ordering, top and bottom captions, and row and column spans. Separated borders support `border-spacing` and `empty-cells`. Collapsed borders are resolved per grid-line segment. `visibility: collapse` removes row and column tracks and clips cells spanning the remaining tracks. Column and row groups support solid backgrounds. [CSS table model](https://www.w3.org/TR/CSS22/tables.html) |
| Form controls | `appearance: auto\|none` controls default decoration for buttons, text fields, checkboxes, radios, and selects. `accent-color` sets default checked colors on checkboxes and radios. Text inputs and textareas support `caret-color`, `::placeholder` color, opacity and font properties, and `::selection` foreground and background colors. [CSS user interface](https://www.w3.org/TR/css-ui-4/) · [CSS pseudo-elements](https://www.w3.org/TR/css-pseudo-4/) |
| Box sizing | `width`, `min-width`, and `max-width` accept `min-content`, `max-content`, `fit-content`, `stretch`, `fit-content()`, and `stretch()`. `aspect-ratio` sets a preferred width-to-height ratio. Logical size properties map to the horizontal writing mode. |
| 2D transforms | `transform` accepts `none` and lists of `matrix()`, `translate()`, `translateX()`, `translateY()`, `rotate()`, `scale()`, `scaleX()`, `scaleY()`, `skew()`, `skewX()`, and `skewY()`. `transform-origin` supports one or two position components. See the 2D transforms section below. |
| Grid | Track lists support intrinsic sizes, named lines, and `repeat()`, including `auto-fill` and `auto-fit`. Grid items support placement by name, line number, or span. Grid supports automatic placement, patterned implicit tracks, item alignment, and track distribution. |
| Flexbox spacing | `row-gap`, `column-gap`, and one- or two-value `gap` work with wrapping and reverse directions. The shorthand sets the row gap first and the column gap second. Values include `normal` (zero in Flexbox), non-negative lengths, and percentages. Percentage gaps use the container's inner size on their axis and resolve to zero when that size depends on the flex items. [CSS Gaps](https://www.w3.org/TR/css-gaps-1/#gaps) |
| Lengths | Absolute, font-relative, and viewport-relative lengths. See the length units and math functions sections below. |
| Colors | Named and hex colors, `rgb()` and `hsl()` variants, `hwb()`, `lab()`, `lch()`, `oklab()`, `oklch()`, predefined `color()` spaces, `color-mix()`, and `currentColor` where applicable. |
| Backgrounds | Solid colors, local images, and linear, radial, and conic gradients. Two-axis tiling supports `repeat`, `no-repeat`, `round`, and `space`. |
| Image fitting | `object-fit` sizes image pixels inside the element's content box. `object-position` aligns the fitted image using keywords, lengths, percentages, or four-component edge offsets. |
| Image sampling | `image-rendering` supports `auto`, `smooth`, `high-quality`, `crisp-edges`, and `pixelated`. |
| Image orientation | `image-orientation` reads EXIF by default, supports the encoded orientation with `none`, and accepts legacy quarter-turn and flip overrides for content images. |
| Image resolution | `image-resolution` sets the natural CSS size of raster content and background images using an explicit resolution or `from-image`, with optional `snap`. |
| Image candidates | `image-set()` works in `background-image` and the `background` shorthand. Options accept local URLs, path strings, gradients, resolutions, and MIME type hints. |
| Image fallbacks | `image()` works in background layers and `image-set()` options. It supports solid colors, fallback colors, `xywh` image fragments, and `ltr` and `rtl` annotations. |
| Image blending | `cross-fade()` mixes background images, gradients, and colors with percentage weights. It also works in `image-set()` options. |
| Border images | `border-image` and its five longhands support nine-slice images, center fill, overhang, and four tiling modes. |
| Outlines | `outline`, its three longhands, and `outline-offset` paint focus rings and other outlines around element boxes. |
| Borders and shadows | `solid`, `dashed`, `dotted`, `double`, `groove`, `ridge`, `inset`, and `outset`, with per-side widths and colors and `border-radius`. `box-shadow` supports `inset` and multiple layers. |
| Text | Theme fonts, font size and weight, italic text, wrapping, alignment, and line height. `font-family` accepts a prioritized list of installed family names and generic families, including in the `font` shorthand. `text-decoration` supports underline, overline, and line-through in five styles, with `auto`, `from-font`, length, and percentage thickness. `text-shadow` supports offsets, blur, and multiple layers on glyphs and decorations. Text emphasis paints keyword shapes and custom marks above or below horizontal text. |
| Cursor | `HTMLLabel` maps common CSS keywords to Godot cursor shapes. Image cursors and unmapped keywords use the control's fallback. |

## Relational selectors and feature queries

`:has()` matches relative to its subject. Use `:has(.icon)` for descendants, `:has(> .icon)` for children, and `:has(+ .item)` or `:has(~ .item)` for following siblings. A relative selector may continue with more combinators. The argument list contributes its highest specificity. `:where()` matches its selector list with zero specificity. `:empty` follows Selectors Level 4 and ignores comments, generated content, and document whitespace. [Selectors Level 4](https://www.w3.org/TR/selectors-4/#relational)


In a stylesheet, `:scope` selects the document root. In an element query, it selects the element that issued the query. `:nth-child(An+B of S)` and `:nth-last-child(An+B of S)` count matching element siblings, including hidden ones. An invalid selector in the `of` list invalidates that pseudo-class. [Selectors Level 4](https://www.w3.org/TR/selectors-4/#tree-structural-pseudos)

`:lang(en, "*-Hant")` checks the inherited HTML `lang` attribute using case-insensitive subtag matching. `:lang("")` matches untagged content. Language-tag aliases and extlang canonicalization are outside the supported set. Hyperlinks on `<a>` and `<area>` match both `:link` and `:any-link` when `href` is present. DocRender records no browsing history. [Selectors Level 4](https://www.w3.org/TR/selectors-4/#the-lang-pseudo)

Use `@supports (display: grid)` to test a declaration and `@supports selector(.card:has(> img))` to test a selector. Conditions accept `not`, `and`, `or`, and parentheses for grouping. Nest `@supports` and `@media` to require both conditions. Declaration probes use the CSS parser and the known rendering limits. A well-formed `var()` probe checks the declaration syntax before substitution. Selector probes reject unsupported arguments inside otherwise forgiving lists. [CSS Conditional Rules](https://www.w3.org/TR/css-conditional-3/#at-supports) · [Selector feature queries](https://www.w3.org/TR/css-conditional-4/#supports-selector-fn)

`font-format(woff2)`, `font-tech(variations)`, and `at-rule(@font-face)` query support for font formats, font technology, and at-rules. Format queries take a keyword. [CSS Conditional Rules Level 5](https://www.w3.org/TR/css-conditional-5/#extensions)

## Cascade

Document styles from `<style>` and enabled links with the `stylesheet` relation follow document order. Link relations are matched as case-insensitive tokens. Alternate stylesheets are skipped. Each `@import` contributes its rules at the import location. The additional `stylesheet` joins the same author rule set after the document's rules.

A `<style>` element contributes author rules when its `type` is omitted, empty, or a case-insensitive match for `text/css`. Changing the attribute recomputes the cascade. [HTML style block processing](https://html.spec.whatwg.org/multipage/semantics.html#update-a-style-block)

A stylesheet link accepts an omitted or empty `type` and the `text/css` MIME type with optional parameters. Changing the link's type or moving a stylesheet in the document recomputes the author cascade in the resulting document order. [HTML stylesheet links](https://html.spec.whatwg.org/multipage/links.html#link-type-stylesheet)

DocRender compares declarations for each longhand in this order.

1. Author declarations marked `!important` precede normal author declarations.
2. A `style` attribute precedes stylesheet rules of the same importance.
3. Among stylesheet rules with equal importance, determine layer precedence. Later layers win for normal declarations, and earlier layers win for important declarations. Unlayered rules form the final implicit layer.
4. Within a layer, higher selector specificity wins. For instance, a document rule for `#card` takes precedence over an additional rule for `.card`.
5. When the preceding criteria match, the later declaration wins, including repeated declarations within a single block. For equally specific stylesheet rules in the same layer, the additional stylesheet comes last.

For example, a matching stylesheet rule with `width: 12rem !important` overrides `style="width: var(--card-width)"`. Adding `!important` to the inline `width` makes the inline declaration win. A value rejected during parsing leaves earlier declarations eligible. A winning value rejected after variable substitution uses `unset` and the earlier stylesheet value stays overridden. The same distinction applies to a shorthand and the longhands it wins. [Invalid values after variable substitution](https://www.w3.org/TR/css-variables-1/#invalid-variables)

Pseudo-elements contribute to the type count. `:is()` and `:not()` take the highest specificity in their argument lists, including valid arguments that did not match. `:nth-child(… of …)` and `:nth-last-child(… of …)` add that highest specificity to their own pseudo-class specificity. [Selectors specificity](https://www.w3.org/TR/selectors-4/#specificity-rules)

Use `@layer base, theme;` to establish order before appending rules with `@layer theme { … }`. Dotted paths such as `theme.buttons` and nested blocks address the same sublayer. Direct rules in a parent form its final implicit sublayer. Each unnamed block or bare `@import "sheet.css" layer;` creates a separate anonymous layer. An import with `layer(name)` appends to the named layer. Layers inside viewport or feature conditions contribute to layer order when those conditions match.

The pre-author baseline contains litehtml defaults, control and theme defaults, and HTML presentational hints. Author declarations take precedence over this baseline. In a document without author layers, both `revert` and `revert-layer` restore the baseline for each affected longhand. When the document uses layers, `revert-layer` first looks in earlier layers in normal layer order, or in the author stylesheets when used in a `style` attribute. If no eligible declaration remains, it restores the baseline. A restored declaration can still use `var()`. [CSS Cascade](https://www.w3.org/TR/css-cascade-5/#cascade-sort)

If a custom property resolves to a CSS-wide keyword, DocRender applies it before substituting values that reference the property. For example, `--card-width: var(--missing, revert)` restores the baseline for `--card-width`, so `width: var(--card-width, 12rem)` sees the restored or inherited value and uses `12rem` if neither is available. [CSS Variables](https://www.w3.org/TR/css-variables-1/#using-variables)

## Tables and form controls

Fixed tables use a resolved table width, column declarations, and the first visual row to size tracks. Later cells can overflow their assigned tracks. In the separated border model, hidden empty cells leave the table background visible. Collapsed row and column tracks retain their DOM handles, while spanning cells are clipped to the remaining tracks.

Form decoration uses DocRender's painted controls. Use `appearance: none` to keep the control's interactions and author styling with custom chrome. `::placeholder` accepts color, opacity as a number or percentage, and font properties. `::selection` paints text fields and textareas. Element-wide opacity still produces a drawing diagnostic.

## Box sizes

`min-content` uses the available line-break opportunities. `max-content` measures the content without optional line breaks. `fit-content` constrains the available width between those two sizes. `fit-content(…)` replaces the available width with a non-negative length or percentage. `stretch` fills the available width with the margin box, and `stretch(…)` uses the argument as that margin-box size. Percentages in the functions use the containing block's width. For positioned boxes, insets reduce the available width used by the keywords. [CSS Box Sizing Level 3](https://www.w3.org/TR/css-sizing-3/#sizing-values) · [CSS Box Sizing Level 4](https://drafts.csswg.org/css-sizing-4/#sizing-values)

In the supported horizontal writing mode, `inline-size`, `min-inline-size`, and `max-inline-size` share the cascade with their `width` counterparts. `block-size`, `min-block-size`, and `max-block-size` share the cascade with their `height` counterparts and accept the corresponding height values. Box content sizing keywords and functions are currently available on the inline axis.

`aspect-ratio` accepts `auto`, a non-negative number, or a pair such as `16 / 9`. CSS math functions with finite numeric results and `var()` may supply ratio components. A zero component makes the ratio behave as `auto`. A ratio alone uses the dimensions selected by `box-sizing`. Combining `auto` with a ratio uses the content box and gives an image's natural ratio priority when available. An image with numeric HTML `width` and `height` attributes gets the corresponding fallback ratio as a presentational hint. A ratio can supply an automatic dimension as the other dimension changes through layout. An in-flow box with visible overflow can grow to fit its content when the minimum on the ratio-dependent axis is automatic. [CSS preferred aspect ratios](https://drafts.csswg.org/css-sizing-4/#aspect-ratio) · [HTML image dimensions](https://html.spec.whatwg.org/multipage/rendering.html#attributes-for-embedded-content-and-images)

## Image fitting

`object-fit` applies to `<img>`. The default `fill` scales to the content box. `contain` shows the whole image and preserves its natural proportions. `cover` fills the box and crops any excess. `none` uses the image's natural dimensions, and `scale-down` limits `contain` to the natural dimensions. The two-keyword forms `contain scale-down` and `cover scale-down` limit the corresponding fit mode to the natural dimensions. These values affect the painted image. The element's layout size comes from its sizing properties, including `aspect-ratio`. [CSS object fitting](https://www.w3.org/TR/css-images-4/#the-object-fit)

`object-position` defaults to `50% 50%`. Use one or two components such as `left`, `top right`, or `calc(50% + 4px) 25%`. Four-component positions such as `bottom 6px right 25%` set offsets from the named edges. Percentages use the difference between the box and the fitted image along each axis. The box background remains visible where the image has no coverage. The content box and its rounded corners clip the image. Flow-relative position keywords use the element's direction in horizontal writing mode. The three-component form is specific to backgrounds. [CSS object positioning](https://www.w3.org/TR/css-images-3/#the-object-position) · [CSS position values](https://drafts.csswg.org/css-values-5/#position)

## Image sampling

`image-rendering` inherits and controls how local raster textures are sampled in images, backgrounds, and markers. `auto` uses the Godot canvas default. `smooth` uses linear filtering, `high-quality` uses linear filtering with any available texture mipmaps, and `crisp-edges` uses nearest-neighbor filtering. The legacy `optimizeQuality` and `optimizeSpeed` values map to `smooth` and `crisp-edges`, respectively.

`pixelated` first enlarges each axis to its closest positive integer multiple with nearest-neighbor filtering, then linearly samples that intermediate image at the displayed size. This sharpens enlarged pixel art and limits blending to transitions between source pixels. Godot samples the virtual intermediate image directly without allocating a larger texture. [CSS image-rendering](https://www.w3.org/TR/css-images-3/#the-image-rendering)

## Background attachment

`background-attachment` accepts `scroll`, `local`, or `fixed` per layer. The default `scroll` moves with the element box. `local` follows the element's scrolled contents and sizes against its scrollable area. `fixed` uses the document's viewport for positioning and sizing. The image remains visible through the element's background clip. Non-root transformed elements and descendants of transformed elements treat `fixed` as `scroll`. [CSS Backgrounds attachment](https://www.w3.org/TR/css-backgrounds-3/#the-background-attachment) · [CSS Transforms rendering model](https://www.w3.org/TR/css-transforms-1/#transform-rendering)

Wheel input in HTMLLabel and MarkdownLabel's HTML blocks scrolls the nearest eligible `overflow: auto` or `overflow: scroll` box under the pointer. Inner scroll offsets are retained when the same DOM elements are laid out again. Standalone HTMLLayout uses the viewport passed to `layout()` with its origin at the document origin. Each Markdown HTML block has its own viewport.

In HTMLLabel, `textarea` wraps text by default. Use `wrap="off"` or `white-space: pre` for horizontal scrolling. `overflow: auto` shows internal scrollbars as needed. `resize` accepts `none`, `both`, `horizontal`, `vertical`, `inline`, and `block`. The default `both` draws a grip in the bottom-right corner when the textarea can scroll or clips overflow. Dragging updates the applicable inline `width` and `height`, subject to CSS sizing and cascade rules. `inline` and `block` correspond to horizontal and vertical resizing in the supported horizontal writing mode.

## Background positions

`background-position: right 12px bottom 8px` places the image 12 CSS pixels inward from the right and 8 inward from the bottom. Edge pairs can be reordered and one offset can be omitted in the three-component form. In a two-component value such as `right 8px`, the length is the vertical position. Negative offsets are allowed. Percentages, including those in CSS math, use the positioning area minus the final tile size on each axis, including when that difference is negative. [CSS Backgrounds positioning](https://www.w3.org/TR/css-backgrounds-3/#the-background-position)

`background-position-x` and `background-position-y` accept individual positions such as `right 10px` and `bottom 20%`. Their comma-separated lists cycle independently and can override the corresponding part of either background shorthand. Flow-relative positions and the `background-position-inline` and `background-position-block` properties use horizontal writing mode and respect the element's `direction`. [CSS Backgrounds position longhands](https://drafts.csswg.org/css-backgrounds-4/#propdef-background-position-x) · [CSS Values flow-relative positions](https://drafts.csswg.org/css-values-5/#position)

## Background tiling

`background-repeat` and the `background` shorthand accept one or two tiling keywords per layer. An omitted second keyword uses the first for both axes. `repeat-x` and `repeat-y` place a single tile on the other axis. In the current horizontal writing mode, `repeat-inline` and `repeat-block` select the horizontal and vertical axes, respectively.

`round` adjusts the tile size so a whole number fits in the background positioning area. When only one axis rounds and the other axis uses an automatic size, the other dimension follows the original tile ratio. `space` puts whole tiles against the two edges and distributes gaps equally. If fewer than two tiles fit, one tile is placed according to `background-position` on that axis. Both patterns extend through the painting area selected by `background-clip`.

`background-repeat-x` and `background-repeat-y` cascade as independent lists. `background-repeat-inline` and `background-repeat-block` map to them in horizontal writing mode. Textures, gradients, solid image fallbacks, and cross-fades share the tiling geometry. Background positions and sizes also accept CSS length and percentage math. [CSS Backgrounds 3 tiling and sizing](https://www.w3.org/TR/css-backgrounds-3/#the-background-repeat) · [CSS Backgrounds 4 repeat longhands](https://drafts.csswg.org/css-backgrounds-4/#the-background-repeat)

## Outlines

Use `outline: 2px solid currentColor` and `outline-offset: 3px` to draw a ring separated from the border. The shorthand sets `outline-width`, `outline-style`, and `outline-color`. Signed offsets cascade independently and a negative offset moves the ring inward. Very negative offsets are constrained on each axis to keep the ring visible. Outlines contribute ink only, with no change to box dimensions, pointer hit areas, or scrolling.

The width accepts the border width keywords and nonnegative lengths. All border line styles except `hidden` are supported. `outline-style: auto` draws a two-pixel solid ring. Its default color follows the operating system accent when available, with a blue fallback. Explicit CSS colors apply to automatic rings too. For other styles, the automatic color resolves to `currentColor`.

An outline follows the element's rounded border edges and encloses wrapped inline fragments with one ring per rendered box. Outlines paint above the contents of their drawing group and follow element transforms and ancestor overflow clips. The supported colors include the CSS color functions listed above. [CSS Basic User Interface outlines](https://www.w3.org/TR/css-ui-4/#outline)

## Border images

`border-image-source` accepts the supported CSS image values, including local URLs, gradients, `image()`, `image-set()`, and `cross-fade()`. URLs imported through local stylesheets become texture dependencies of the HTML resource. With `none` or an unavailable URL, the ordinary border paints.

Use `border-image: url(frame.png) 24 fill / 12px / 4px round stretch` to cut 24 source pixels from each edge, paint the middle, draw 12 CSS pixel wide slices, and extend the image area by 4 CSS pixels. Slice values can be numbers or percentages. Width accepts lengths, percentages, border-width multipliers, or `auto`. Outset accepts lengths or border-width multipliers. Both shorthands validate as a unit and `border` resets the five border-image longhands.

`border-image-repeat` accepts one or two values. `stretch` fills the region, `repeat` centers and tiles, `round` adjusts tile sizes for a whole number, and `space` keeps whole tiles with equal gaps before, between, and after them. Wrapped inline fragments share horizontal tiling. The rectangular image ignores the element's own border radius and follows ancestor overflow clips and element transforms. An outset affects painting only. Collapsed table borders use ordinary border painting. [CSS Border Images](https://www.w3.org/TR/css-backgrounds-3/#border-images)

## Image orientation

`image-orientation` inherits. The default `from-image` uses the EXIF orientation from PNG, JPEG, or WebP metadata preceding the pixel data. `none` uses the encoded pixels. Imported HTML stores the orientation alongside its texture dependencies so it is available when the original raster file is omitted from an export. Reimport existing HTML files to store this metadata for export.

For content images, the optional angle and `flip` can appear in either order. The angle rounds to the nearest quarter turn, with ties toward positive infinity, and the horizontal flip happens after rotation. Decorative images use EXIF when given a legacy angle or flip. Natural dimensions, fitting, image fragments, and background image sizing use the oriented image. [CSS image orientation](https://www.w3.org/TR/css-images-3/#the-image-orientation)

## Image resolution

Use `image-resolution: 192dpi` or `image-resolution: 2x` to make a raster image's natural CSS width and height half its pixel dimensions. The property inherits and applies to content images and raster sources in backgrounds, including fragments and cross-fades. It accepts the CSS resolution units and math functions. SVG uses its vector dimensions. [CSS image-resolution](https://www.w3.org/TR/css-images-4/#the-image-resolution)

`from-image` reads equal horizontal and vertical densities with physical units from PNG, JPEG, and WebP metadata before pixel data. The explicit resolution in `from-image 2x` supplies a fallback. A plain image defaults to one image pixel per CSS pixel. An `image-set()` selection supplies its declared density or its metadata as the natural density. An explicit `image-resolution` without `from-image` overrides that density. [CSS image metadata](https://www.w3.org/TR/css-images-3/#image-metadata) · [CSS image-set()](https://www.w3.org/TR/css-images-4/#image-set-notation)

`snap` chooses the nearest resolution with a whole-pixel sampling ratio against DocRender's `1dppx` output. The HTML importer stores the source DPI with the document so imported textures retain their natural CSS size in exports.

## Image candidates

Use `image-set("icon.png" 1x, url("icon-2x.png") 2x)` in a background layer. Omitted resolutions count as `1x`. Resolutions accept `x`, `dppx`, `dpi`, `dpcm`, and typed math functions such as `calc(96dpi + 1dppx)`. Resolution media queries use the same units and math. [CSS resolution values](https://www.w3.org/TR/css-values-4/#resolution)

`type()` accepts PNG, JPEG, WebP, and SVG MIME types. Unsupported MIME types are skipped before ties are resolved in declaration order. DocRender chooses the smallest supported density at or above its CSS output resolution of `1dppx`. If every density is lower, it chooses the highest available density. The selected resolution scales a raster image's natural CSS size. SVG and gradients use their usual sizing rules. [CSS image-set()](https://www.w3.org/TR/css-images-4/#image-set-notation)

The HTML importer embeds local URL and string candidates from each collected stylesheet, including direct string fallbacks in `var()`. Relative paths use the stylesheet's directory.

Content images support `srcset` width and pixel-density descriptors, `sizes` media conditions and length math, and `<picture>` sources with `media` and `type`. They use the current layout viewport and the same `1dppx` output density. Width candidates use the first matching source size, with `100vw` as the default. The selected source's dimensions become HTML sizing hints when provided. The importer records local candidates across media branches as texture dependencies. [HTML responsive images](https://html.spec.whatwg.org/multipage/images.html)

If the selected texture is unavailable, an image with nonempty `alt` draws the alternative text inside its existing box. The text uses the computed font, color, line height, alignment, and wrapping mode. It provides automatic box dimensions and follows the content clip and rounded corners. The intrinsic image size stays zero until a usable texture is selected. [HTML alternative text](https://html.spec.whatwg.org/multipage/embedded-content.html#the-img-element)

## Image fallbacks and fragments

Use `image("sprite.png", currentColor)` to paint a solid color if the source cannot load. Use `image(rgb(0 0 255 / 50%))` as an independent solid-color layer. Both forms use the background layer's size, position, clip, and repeat. A successfully loaded transparent image retains its transparency. [CSS image()](https://www.w3.org/TR/css-images-4/#image-notation)

Use a fragment such as `image("sprites.png#xywh=40,0,20,20")` to paint a cropped image with its own natural dimensions. The fragment accepts pixel coordinates, an explicit `pixel:` prefix, or `percent:` coordinates. Crop rectangles intersect the source image bounds. An unrecognized fragment or an empty intersection uses the fallback if specified. The importer records the source texture as a dependency. DocRender caches each cropped texture within the layout. [Media Fragments URI](https://www.w3.org/TR/media-frags/#naming-space)

An annotation such as `image(ltr "arrow.png")` flips the image horizontally in an RTL element. Background image direction follows inherited `direction: ltr | rtl` and HTML `dir="ltr"` or `dir="rtl"` hints. The directional flip is applied after the source's `image-orientation`. [CSS bidi-sensitive images](https://www.w3.org/TR/css-images-4/#image-notation)

## Image blending

Use `cross-fade(url("photo.png") 75%, currentColor)` in a background layer. Omitted weights share what remains up to 100%. If explicit weights sum above 100%, their contributions are normalized. Any remainder after all weights are assigned contributes transparency. The color and alpha channels are averaged in premultiplied sRGB. [CSS cross-fade()](https://www.w3.org/TR/css-images-4/#cross-fade-function)

Images with natural dimensions contribute to the weighted width and height. Colors and gradients fill the resulting tile. Nested cross-fades distribute their weights through the inputs. The background layer's size, position, repeat, and clip apply to the result. DocRender rasterizes composite tiles with up to 2048 pixels per side before drawing them through Godot.

## Length units

- Absolute lengths include `px`, `in`, `cm`, `mm`, `Q`, `pt`, and `pc`. DocRender uses 96 CSS pixels per inch.
- Font-relative lengths include `em`, `ex`, `cap`, `ch`, `ic`, and `lh`. The root equivalents are `rem`, `rex`, `rcap`, `rch`, `ric`, and `rlh`. `lh` uses the computed CSS line height, including a font-metrics value for `normal`. When `font-size` uses a local font-relative unit, it uses the parent's metrics. `lh` in `line-height` also uses the parent's line height. A self-reference at the root uses initial metrics.
- `vw`, `vh`, `vi`, `vb`, `vmin`, and `vmax` follow the layout viewport. DocRender uses horizontal writing, so `vi` follows width and `vb` follows height. The `sv*`, `lv*`, and `dv*` variants use the same viewport because DocRender has a single viewport size per layout pass.

Length-based media queries resolve font units against the initial font metrics. They reevaluate viewport units and length calculations when the viewport changes. [CSS Values and Units Level 4](https://www.w3.org/TR/css-values-4/#lengths)

## Generated content and counters

`::before` and `::after` accept strings, local `url()` images, `attr()` text, quotes, and counters in `content`. For example, `content: attr(data-title, "Untitled")` substitutes the element's attribute as literal text and uses the fallback for a missing attribute. `attr(data-title raw-string, "Untitled")` is also supported. Relative image URLs use the stylesheet's directory. Generated content is included in rendered text and stays outside DOM queries. [CSS Generated Content](https://www.w3.org/TR/css-content-3/#content-property)

The inherited `quotes` property accepts `auto`, `none`, `match-parent`, and pairs of strings. Automatic quote pairs follow English, French, German, or Japanese conventions, with English as the default. `open-quote` and `close-quote` track nesting in document order. Their `no-` variants adjust nesting without drawing a mark. [CSS quotation marks](https://www.w3.org/TR/css-content-3/#quotes)

Use `counter-reset`, `counter-increment`, and `counter-set` to create or change named counters. `counter()` reads the innermost value. `counters()` joins values across nested scopes. Both support `decimal`, `decimal-leading-zero`, Latin alphabetic styles, `lower-greek`, Roman numerals, and symbolic styles such as `disc`. The default style is `decimal`. Counters on elements with `display: none` and pseudo-elements without generated content have no effect. [CSS Lists and Counters](https://www.w3.org/TR/css-lists-3/#auto-numbering)

### List markers

`::marker` generates content before `::before` on a list item. Use `content: none` to suppress it, or `content: normal` to use the item's `list-style-image` or `list-style-type`. The type can be a keyword or a quoted string, including in the `list-style` shorthand. The marker accepts inherited text styling such as color, font, and text transformation. `list-style-position: inside` places it in the first line's inline flow. `outside` aligns it with the first line beside the item. [CSS list markers](https://www.w3.org/TR/css-lists-3/#marker-pseudo)

The `list-item` counter works with `counter()` and `counters()`. HTML ordered lists initialize it from `start` and `reversed`, and a list item's `value` sets its ordinal. The HTML `type` hint participates in the cascade. To control numbering in CSS, set `counter-reset`, `counter-increment`, or `counter-set` with the `list-item` name. [CSS list-item counters](https://www.w3.org/TR/css-lists-3/#list-item-counter)

### Custom counter styles

```css
@counter-style chapter {
  system: extends upper-roman;
  prefix: "[";
  suffix: "] ";
}

ol { list-style: chapter inside; }
```

Counter styles support `cyclic`, `fixed`, `symbolic`, `alphabetic`, `numeric`, `additive`, and `extends`. Text symbols can be strings or identifiers. Anonymous `symbols()` styles use quoted strings. Both kinds of style work with list markers, `counter()`, and `counters()`. The `prefix` and `suffix` descriptors apply to the default marker. The `negative`, `range`, `pad`, and `fallback` descriptors also apply to generated text. Styles defined in imports and conditional rules follow the author layer and source order. Built-in numeric styles include CJK, Arabic-Indic, and several other Unicode decimal systems. Image symbols and `speak-as` are outside the supported grammar. [CSS Counter Styles](https://www.w3.org/TR/css-counter-styles-3/)

## Variables and math functions

```css
:root { --gutter: 16px; }
.panel {
  width: min(100%, calc(48rem - 2 * var(--gutter, 16px)));
  padding: clamp(8px, 2vw, 24px);
}
```

Custom property names are case-sensitive. Values resolve before inheritance. `var()` supports repeated references, nested and empty fallbacks, and detects cycles, including references in fallback arguments. Invalid substitutions use the property's inherited or initial value. Declaration diagnostics identify the affected element and retain the original declaration. [CSS Custom Properties](https://www.w3.org/TR/css-variables-1/#using-variables)

Shorthands validate the full value before applying their longhands. Omitted components reset to their shorthand defaults. A shorthand with `var()` resolves once per element and updates the longhands it still wins. A winning declaration with an invalid substitution uses `unset`. A rollback without an eligible declaration uses `unset`. [CSS Cascading and Inheritance](https://www.w3.org/TR/css-cascade-5/)

Length expressions are supported in `width`, `height`, their `min-` and `max-` variants, `margin`, `padding`, `top`, `right`, `bottom`, `left`, `font-size`, `line-height`, `flex-basis`, Flexbox and Grid gaps, and length entries in Grid track lists. `line-height` also accepts math that produces a unitless number. Percentage arguments use the basis supplied by each layout property, including when the result depends on a nested `sign()` or `atan2()`.

- Arithmetic and comparisons use `calc()`, `min()`, `max()`, and `clamp()`. Put spaces around `+` and `-`. Multiplication takes a number on at least one side. Division accepts a number or a divisor of the same type as the dividend.
- `round()` accepts `nearest`, `up`, `down`, and `to-zero`, with an optional step of 1 for numbers. `mod()` follows the sign of the divisor. `rem()` follows the sign of the dividend.
- Trigonometric functions include `sin()`, `cos()`, `tan()`, `asin()`, `acos()`, `atan()`, and `atan2()`. Forward functions accept an angle or a number in radians. Inverse functions return angles. Other functions include `pow()`, `sqrt()`, `hypot()`, `log()`, `exp()`, `abs()`, and `sign()`.
- Math accepts `e`, `pi`, `infinity`, `-infinity`, and `NaN`. The renderer bounds layout values to its representable range. Transform scalars, color components, and color mixing weights support calculations that can resolve without layout lengths.

[CSS Values and Units](https://www.w3.org/TR/css-values-4/#math)

## Color functions

Use absolute `hwb()`, `lab()`, `lch()`, `oklab()`, and `oklch()` wherever DocRender accepts a color. Components support numbers and percentages where defined by the color space, and hue accepts degrees or an angle in `deg`, `grad`, `rad`, or `turn`. Use `/` to specify alpha. A component set to `none` is treated as zero when painting a color directly. [CSS Color 4](https://www.w3.org/TR/css-color-4/#color-syntax)

`color(display-p3 0.445 0.529 0.891)` selects a predefined space. Supported spaces are `srgb`, `srgb-linear`, `display-p3`, `display-p3-linear`, `a98-rgb`, `prophoto-rgb`, `rec2020`, `xyz-d50`, and `xyz-d65` (also spelled `xyz`). Components outside the nominal gamut are retained for interpolation. [Predefined color spaces](https://www.w3.org/TR/css-color-4/#predefined)

`color-mix(in oklch shorter hue, currentColor 30%, white)` mixes one or more colors in any of these predefined spaces, as well as `lab`, `lch`, `oklab`, `oklch`, `hsl`, and `hwb`. Omitting the interpolation method uses `oklab`. For polar spaces, hue can follow the `shorter`, `longer`, `increasing`, or `decreasing` arc. Missing components carry over when applicable, and non-hue components use premultiplied alpha. [CSS Color 4 interpolation](https://www.w3.org/TR/css-color-4/#interpolation)

Each input can include a weight from `0%` to `100%`. Omitted weights share the remainder. Polar mixes combine colors in source order. Explicit weights are normalized, and when the total is below `100%`, it also scales the output alpha. A total of zero yields a transparent color. Custom properties can supply the inputs or weights. Mixes that contain `currentColor` are resolved in the element that uses the property. In the `color` property itself, they use the inherited text color. Nested mixes retain floating-point colors until painting, when channels are clipped to DocRender's 8-bit sRGB output. Relative colors are outside the supported grammar. [CSS Color 5](https://www.w3.org/TR/css-color-5/#color-mix)

Markdown HTML blocks use the full HTML color pipeline. Markdown's inline `<span style>` supports constant mixes in `color` and `background-color`.

## 2D transforms

```css
.card {
  transform-origin: left top;
  transform: translate(10%, 12px) rotate(8deg) scale(1.1);
}
```

Translation and origin offsets accept lengths, percentages, and length expressions. Percentages use the border-box width or height. The default origin is the center of that box. Rotation accepts `deg`, `grad`, `rad`, `turn`, and unitless zero. Scale accepts finite factors, including negative factors. Angles and numeric factors also accept math functions. A singular transform such as `scale(0)` hides the subtree while preserving its place in the flow.

Transforms affect painting, element rectangles, links, selection, and scrollable overflow. Non-replaced inline boxes and table columns or column groups can inherit an ancestor's transform as part of its subtree. Use an inline-block for a transform on an inline element itself. In Markdown, downward overflow expands the HTML block and the column bounds clip horizontal overflow. 3D transforms, `transform-box`, standalone transform properties, and animation are outside the supported set. [CSS Transforms Level 1](https://www.w3.org/TR/css-transforms-1/)

Painting and hit testing use the exact clip shapes of up to 16 rounded or transformed overflow ancestors, starting with the closest ancestor. Farther clips use their document-space bounding rectangles. An element's own rounded shape is evaluated separately.

## Grid

```css
.cards {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
  grid-template-rows: auto;
  gap: 12px 16px;
}

.cards .featured {
  grid-column: 1 / -1;
  grid-row: 1;
}
```

`display: inline-grid` uses the same tracks and item placement as `grid`. An automatic width fits the content within the containing block, subject to intrinsic sizes and `min-width` and `max-width`. The whole grid moves to the next line if it cannot fit alongside preceding text. Text inside each item can wrap within its grid area. The surrounding line uses the grid's first baseline by default, and `vertical-align` can change how the box lines up with its neighbors. [CSS Grid container sizing and baselines](https://www.w3.org/TR/css-grid-2/#grid-container-sizing) · [CSS Inline baseline source](https://www.w3.org/TR/css-inline-3/#baseline-source)

Use a positive integer in `repeat()` to repeat one or more track sizes. For example, `repeat(2, 80px minmax(0, 1fr))` creates four tracks. An axis can declare up to 10,000 explicit tracks. `minmax()` accepts non-negative lengths, percentages, `auto`, `min-content`, and `max-content` on either side, with fractions also accepted as the maximum. [CSS Grid track sizing](https://www.w3.org/TR/css-grid-2/#track-sizes)

`min-content` follows the space needed at the available wrap opportunities, while `max-content` uses the preferred content size. `fit-content()` caps a track's max-content growth at a non-negative length or percentage, subject to the track's automatic minimum. A percentage cap resolves against the grid's final content size on that axis. Intrinsic measurements use the uncapped maximum until that size is known. These track sizes work in explicit templates, integer repeats, and implicit track patterns. A `fit-content()` track takes space up to its limit before content alignment distributes remaining space. [CSS Grid intrinsic track sizes](https://www.w3.org/TR/css-grid-2/#algo-content)

Each axis accepts one `repeat(auto-fill, …)` or `repeat(auto-fit, …)`. Every track in a template containing an automatic repeat needs a length or percentage as at least one of its sizing bounds. Each layout pass counts repetitions using the container's content box and gaps. With only a definite minimum size, it rounds the count up to meet that minimum. Without a definite size constraint, it repeats once. [CSS Grid automatic repetition](https://www.w3.org/TR/css-grid-2/#auto-repeat)

`auto-fill` keeps every repeated track. `auto-fit` collapses repeated tracks that no in-flow item occupies or spans. The remaining tracks have one gap between them, and the grid retains its original line numbers after collapse. [CSS Grid gutters](https://www.w3.org/TR/css-grid-2/#gutters)

Use brackets to name a grid line, as in `[outer content-start] 80px [content-end]`. Line names are case-sensitive. `repeat(3, [card] 80px [card])` produces four distinct `card` lines. `grid-column: 2 card / -1 card` uses the second matching line from the start and the last matching line. A definite edge can anchor a named span such as `grid-column: 1 / span 2 card`. `auto-fit` retains line names when it collapses empty tracks. [CSS Grid line names](https://www.w3.org/TR/css-grid-2/#named-lines) · [CSS Grid line placement](https://www.w3.org/TR/css-grid-2/#line-placement)

```css
.dashboard {
  display: grid;
  grid-template-columns: [frame] 120px minmax(0, 1fr);
  grid-template-areas: "title title" "nav main" "nav .";
}

.title { grid-area: title; }
.main { grid-area: 2 / main-start / span 2 / -1; }
```

Each string in `grid-template-areas` defines one row. Consecutive dots represent one empty cell. Rows must have equal lengths and each named area must fill a rectangle. An area adds `name-start` and `name-end` lines to both axes. The template can extend the explicit grid. `grid-area` sets row start, column start, row end, and column end, in that order. A name on its own applies to all four edges. [CSS Grid named areas](https://www.w3.org/TR/css-grid-2/#grid-template-areas-property) · [CSS Grid placement shorthands](https://www.w3.org/TR/css-grid-2/#placement-shorthands)

`grid-row`, `grid-column`, and their `-start` and `-end` longhands accept names, nonzero integer line numbers, `span`, and `auto`. Positive line numbers count from the start of the explicit grid. Negative line numbers count back from its end, so `grid-column: 1 / -1` spans the explicit grid even when implicit columns exist. Placement coordinates are limited to 10,000 tracks on each side of the explicit origin. An area entirely beyond the limit uses the nearest edge track. [CSS Grid placement](https://www.w3.org/TR/css-grid-2/#placement) · [Large grid limits](https://www.w3.org/TR/css-grid-2/#overlarge-grids)

Explicit areas may overlap. Automatic placement skips occupied areas and processes items in `order` and document order. `grid-auto-flow` defaults to `row`, which fills each row and then adds rows as needed. `column` fills each column and then adds columns. Add `dense` to either direction to revisit earlier holes, or use `dense` alone for `row dense`. Consecutive direct text forms an anonymous item. Whitespace-only runs and out-of-flow children take no cells. A numeric `z-index` establishes a stacking context on a Grid item, including one with `position: static`. [CSS Grid auto-placement](https://www.w3.org/TR/css-grid-2/#auto-placement-algo) · [CSS Grid painting order](https://www.w3.org/TR/css-grid-2/#z-order)

`grid-auto-rows` and `grid-auto-columns` accept a space-separated list of the supported track sizes and default to `auto`. For `grid-auto-columns: 60px 100px`, the first column after the sized tracks takes `60px`, and the closest implicit column before the explicit grid takes `100px`. The pattern repeats in each direction. The forward pattern also sizes any tracks added by `grid-template-areas` that have no size from `grid-template-columns` or `grid-template-rows`. [CSS Grid implicit track sizing](https://www.w3.org/TR/css-grid-2/#auto-tracks)

`row-gap`, `column-gap`, and the `gap` shorthand add spacing between tracks, including between tracks in a spanning item's area. `normal` resolves to zero. Percentage tracks use `auto` and the percentage portion of a gap resolves against zero during intrinsic sizing. Final layout resolves percentages against the content box. [CSS percentage gaps](https://www.w3.org/TR/css-gaps-1/#gap-percent)

`justify-items` and `align-items` set the horizontal and vertical alignment within each grid area. Individual items can override these defaults with `justify-self` and `align-self`. Positions include `start`, `end`, and `center`, with `left` and `right` available on the horizontal axis. Add `safe` before a position to use start alignment when the item overflows its area. `stretch` fills the area along the chosen axis when the item has an automatic size and no automatic margins on that axis. Automatic margins absorb positive free space before alignment. [CSS Grid item alignment](https://www.w3.org/TR/css-grid-2/#grid-align)

`justify-content` and `align-content` align or distribute the tracks within the content box. In addition to positional alignment and `stretch`, they support `space-between`, `space-around`, and `space-evenly`. Distribution adds to the declared gap, and spanning items include the added spacing inside their areas. Collapsed `auto-fit` tracks retain their line numbers and contribute no spacing. [CSS Grid content distribution](https://www.w3.org/TR/css-align-3/#distribution-grid)

`align-items`, `align-self`, `justify-items`, and `justify-self` support `baseline` (equivalent to `first baseline`) and `last baseline` in horizontal, left-to-right layout. Row alignment uses the first or last text baseline, with the bottom border edge supplying one for images and empty boxes. Column alignment synthesizes a baseline at the left border edge. Spanning items join the group at their first or last track, and the alignment adjustment contributes to the size needed across the span. Auto margins take precedence. Percentage sizes and limits that depend on content-sized tracks use safe start or end alignment. [CSS Grid baseline alignment](https://www.w3.org/TR/css-grid-2/#grid-align) · [Baseline grouping](https://www.w3.org/TR/css-align-3/#baseline-sharing-group)

Grid items with `align-content: first baseline` or `last baseline` can align their contents with neighboring items in the same row. Participation requires a content baseline and self-alignment that anchors the matching edge. Extra space is applied inside the box. When the content cannot join the group, the corresponding safe edge applies. DocRender implements this for its Grid layout. Browser behavior for these values varies. [CSS baseline content alignment](https://www.w3.org/TR/css-align-3/#baseline-align-content) · [CSS Working Group resolution](https://github.com/w3c/csswg-drafts/issues/13421)

`place-items`, `place-self`, and `place-content` set the corresponding `align-*` and `justify-*` properties in that order. A single value applies to both axes, as in `place-items: center`. With a single baseline value in `place-content`, the second axis defaults to `start`. Multi-keyword values such as `place-self: safe end unsafe center` use the grammar of the corresponding longhand. [CSS Box Alignment shorthands](https://www.w3.org/TR/css-align-3/#place-content)

`grid-template` sets the explicit rows, columns, and named areas. Use `none`, a pair of track lists such as `40px auto / repeat(3, 1fr)`, or quoted area rows such as `"head head" 40px "nav main" / 12rem 1fr`. In the quoted form, omitted row sizes are `auto`, the optional column list uses individual tracks, and line names on either side of a row boundary share that line. The implicit grid settings remain in effect. [CSS Grid template shorthand](https://www.w3.org/TR/css-grid-2/#grid-template-property)

`grid` accepts the same explicit forms and adds automatic placement forms such as `auto-flow dense 24px / repeat(3, 1fr)` and `40px 40px / auto-flow 100px`. Put `auto-flow` on the left of the slash to create rows or on the right to create columns. The optional `dense` keyword can appear immediately before or after it. Any sizes that follow form the repeating implicit track pattern on that axis. Omitted longhands reset to their initial values: `none` for the explicit grid, `auto` for implicit track sizes, and `row` for the flow direction. Both shorthands preserve the existing gaps. [CSS Grid definition shorthand](https://www.w3.org/TR/css-grid-2/#grid-shorthand)

`subgrid` remains unsupported.


## Underline placement

`text-underline-position` accepts `auto`, `from-font`, and `under`. The `left` and `right` keywords can be used alone or with `under`. In horizontal writing, only `under` selects the text's bottom edge. `text-underline-offset` takes `auto`, a signed length, a percentage of the decorating element's font size, or a length expression. For an explicit offset, the origin is the baseline with `auto`, the font metric with `from-font`, and the text's bottom edge with `under`. [CSS Text Decoration](https://www.w3.org/TR/css-text-decor-4/#underline-offset)

Both properties inherit independently of the `text-decoration` shorthand. An existing underline keeps the offset from the element that started it. The renderer positions its segments using the shaped run's baseline and descent. It does not yet resolve separately originating nested underlines or align their baselines across mixed font sizes.

## Text emphasis

Use `text-emphasis: open sesame currentColor` to set the mark and color, and `text-emphasis-position: under` to place the mark below horizontal text. The longhand `text-emphasis-style` accepts `none`, an open or filled dot, circle, double circle, triangle, or sesame, and custom strings. For custom strings, the first grapheme is used. The shorthand resets omitted style and color and keeps the position independent. [CSS Text Decoration](https://www.w3.org/TR/css-text-decor-3/#emphasis-marks)

Marks center on shaped graphemes, using half-size text and the OpenType ruby feature. CSS-defined spaces, punctuation, and controls are skipped. Marks participate in line spacing, clipping, and text shadows. Text selection and copying use the underlying document text. Vertical writing modes, ruby collision handling, and `text-emphasis-skip` are outside this support.

## Font families

```css
body { font-family: "Segoe UI", Arial, sans-serif; }
code { font-family: "Cascadia Mono", Consolas, monospace; }
```

The renderer tries the listed families in order and lets Godot shape text with the next font when a glyph is missing. Quoted names are literal, including punctuation and names such as `"monospace"`. Unquoted generic families use theme fonts or installed fonts, depending on the family and platform. The project or Markdown theme supplies the default text font and `sans-serif`. A configured Markdown code font supplies `monospace`. `initial` restores the default, `inherit` and `unset` inherit the parent's list, and `revert` rolls back to the default stylesheet and presentational hints.

Use `@font-face { font-family: "UI Copy"; src: url(fonts/ui.woff2); font-weight: 100 900; }` to name a project font. Paths are relative to the stylesheet or can use `res://`. Sources are tried in order. Supported extensions are TTF, OTF, TTC, OTC, WOFF, and WOFF2. The `format()` hint accepts `truetype`, `opentype`, `collection`, `woff`, and `woff2`. A source marked `tech(variations)` requires a variable font.

Font matching uses the declared style (`normal`, `italic`, or `oblique`) and weight. A weight range configures the OpenType `wght` axis when the font provides it. Legacy hints such as `format("truetype-variations")` also require a variable font. Rules respect media conditions, layer order, and source order. Imported HTML records font dependencies across media branches, including linked and nested stylesheets. [CSS Fonts](https://www.w3.org/TR/css-fonts-4/#font-face-rule)

`font-style: oblique 18deg` also works in the `font` shorthand. Omit the angle to request 14 degrees. In `@font-face`, use two angles to declare an inclusive range. Positive and negative angles select different slants. Variable faces with a `slnt` axis use the selected angle, within the face and axis limits. When DocRender synthesizes the slant of an upright font, it caps the shear at 89 degrees. [CSS Fonts style matching](https://www.w3.org/TR/css-fonts-4/#font-style-prop)

`font-width` and its legacy alias `font-stretch` share one cascaded value. They accept nonnegative percentages and the width keywords from `ultra-condensed` through `ultra-expanded`. The `font` shorthand accepts the width keywords and resets the width to `normal` when omitted. In `@font-face`, either descriptor accepts a single width, an inclusive range, or `auto`. Width is matched before style and weight. Variable faces use the OpenType `wdth` axis within the declared and font-supported ranges. Installed families use the widths available through Godot's system font lookup. [CSS Fonts width matching](https://www.w3.org/TR/css-fonts-4/#font-width-prop)

`font-feature-settings` and `font-variation-settings` are inherited and also work as `@font-face` descriptors. Both accept `normal` or comma-separated settings with quoted, case-sensitive four-character OpenType tags. Features accept an optional nonnegative integer, `on`, or `off`. Axes require a number, which can be fractional or negative. Math expressions follow the same numeric types. Repeating a tag uses its last value. The `font` shorthand resets both properties to `normal`, so any settings supplied by the chosen face still apply.

Element settings override the selected face's defaults for the same tag. Explicit axes take precedence over the coordinates derived from `font-weight`, `font-width`, and `font-style` after font selection. Coordinates are limited to the font's intrinsic axis bounds. Unsupported tags have no effect on shaping or font selection. [CSS Fonts feature and variation precedence](https://www.w3.org/TR/css-fonts-4/#feature-variation-precedence)

`font-kerning` accepts `auto`, `normal`, and `none`. `auto` uses the normal font and shaper defaults, including any feature settings on the selected face. `normal` enables kerning in horizontal text and applies proportional alternate spacing when the font supplies it. `none` disables kerning. This property is inherited and the `font` shorthand resets it to `auto`. An explicit `font-feature-settings` entry can override the features set by `font-kerning`. [CSS Fonts kerning](https://www.w3.org/TR/css-fonts-4/#font-kerning-prop)

`font-optical-sizing` is inherited and accepts `auto` or `none`. The `font` shorthand resets it to `auto`. For a font with an `opsz` axis, `auto` uses the rendered font size in points at 96 CSS pixels per inch, limited to the font's axis range. It takes precedence over the `@font-face` descriptor's `opsz` value. `none` skips the automatic adjustment, and an explicit element-level `font-variation-settings` entry for `opsz` takes precedence in either mode. Changes to layout font size update the automatic axis. [CSS Fonts optical sizing](https://www.w3.org/TR/css-fonts-4/#font-optical-sizing-def)

The inherited `font-variant-*` properties below select OpenType features during text shaping. They apply after the selected face's feature defaults and before explicit `font-feature-settings` on the element. Within a property, keywords from distinct categories can be combined in any order. Repeated or conflicting choices in one category invalidate the declaration.

| Property | Supported values |
| --- | --- |
| `font-variant-ligatures` | `normal`, `none`, and enable or disable common, discretionary, and historical ligatures and contextual alternates. Required ligatures stay enabled by the shaper. |
| `font-variant-numeric` | `normal`, `lining-nums` or `oldstyle-nums`, `proportional-nums` or `tabular-nums`, `diagonal-fractions` or `stacked-fractions`, `ordinal`, and `slashed-zero`. |
| `font-variant-east-asian` | `normal`, a JIS or simplified/traditional form, `full-width` or `proportional-width`, and `ruby`. |
| `font-variant-caps` | `normal`, `small-caps`, `all-small-caps`, `petite-caps`, `all-petite-caps`, `unicase`, and `titling-caps`. Petite caps fall back to the corresponding OpenType small-cap feature when unavailable. Synthetic small caps and the synthetic unicase fallback are unsupported. |
| `font-variant-alternates` | `normal`, `historical-forms`, `stylistic()`, `styleset()`, `character-variant()`, `swash()`, `ornaments()`, and `annotation()`. |

`font-variant` combines these groups and resets omitted groups. `font-variant: none` disables optional ligatures and resets the other groups. The `font` shorthand also resets these groups and accepts the CSS2 `small-caps` component. [CSS Fonts variants](https://www.w3.org/TR/css-fonts-4/#font-variant-prop)

`@font-feature-values` defines case-sensitive names for one or more named font families. The definitions combine across stylesheets, and later declarations in the winning cascade layer replace the same name for that family and feature type. The selected face's family determines which definitions apply, including when text falls back to another family. Undefined names leave the glyphs unchanged. `styleset()` and `character-variant()` accept comma-separated names. Stylistic-set definitions can enable multiple sets, and character-variant definitions can supply a selector index and an optional alternate value. For OpenType stylistic sets and character variants, index zero and indices above 99 enable no feature. [CSS Fonts feature values](https://www.w3.org/TR/css-fonts-4/#font-feature-values)

`letter-spacing` and `word-spacing` accept `normal`, lengths, percentages, and compatible CSS math expressions. Both properties are inherited. Percentages resolve against the receiving element's font size, while relative lengths such as `em` are computed where declared. Fractional and negative spacing participate in line breaking, painting, text shadows, and text selection.

`letter-spacing` adds space between shaped clusters and shares spacing across adjacent inline fragments. It suppresses optional ligatures unless an explicit `font-feature-settings` entry enables them. Connected-script handling applies within shaped fragments. `word-spacing` adjusts visible word separators such as ordinary and no-break spaces after white-space processing. Word separators with zero advance and fixed-width spaces such as U+3000 keep their intrinsic advance. [CSS Text spacing](https://www.w3.org/TR/css-text-4/#spacing)

`text-autospace` inherits and defaults to `normal`, which enables both `ideograph-alpha` and `ideograph-numeric`. DocRender uses the same modes for `auto`. These modes add one eighth of the containing font's ideographic advance between eligible adjacent characters on the same line. `no-autospace` disables the feature. The `punctuation` flag adds nonbreaking spacing around French punctuation according to the inherited HTML language. `insert` adds spacing at unspaced boundaries. `replace` also adjusts ordinary U+0020 spaces while preserving them in copied text. Shaping uses Godot's bidi handling inside each text run. Spacing between separate inline runs follows their layout order. [CSS automatic text spacing](https://drafts.csswg.org/css-text-4/#text-autospace)

`text-spacing-trim` inherits and controls fullwidth punctuation at line starts, at line ends, and between adjacent characters. It accepts `normal`, `auto`, `space-all`, `trim-all`, `trim-both`, and one keyword from each of the three position groups. DocRender uses normal behavior for `auto`. `allow-end` trims a closing glyph when its full advance would overflow. `space-first` keeps full width at the start of a paragraph and after a forced break. Punctuation follows the horizontal layout order. The renderer uses OpenType `halt` when available. It can also trim blank portions of a uniform fullwidth font if glyph contours allow it. Proportional glyphs without a trimming feature keep their native advances. [CSS punctuation spacing](https://drafts.csswg.org/css-text-4/#text-spacing-trim)

`text-spacing` sets both properties. `none` sets `space-all` and `no-autospace`. `normal` restores the initial values, and `auto` selects automatic behavior for both. When the shorthand specifies only one of the properties, the other resets to its initial value. [CSS text spacing shorthand](https://drafts.csswg.org/css-text-4/#text-spacing)

`text-transform` accepts `none`, `math-auto`, or one case transform with optional `full-width` and `full-size-kana`. The width and kana transforms can also be used on their own or together. The renderer applies case first, then width, then kana size. The width transform converts preserved spaces to ideographic spaces and leaves collapsed spaces at their ordinary width. `math-auto` applies the mathematical italic mapping to source text nodes that contain a single character. Selection and copy use the source characters, while measuring, drawing, and hit testing use the transformed glyphs. Case conversion uses Godot's simple Unicode mappings. Line-break opportunities still follow the original text runs. [CSS Text transforms](https://www.w3.org/TR/css-text-4/#text-transform-property) · [MathML italic mappings](https://www.w3.org/TR/mathml-core/#italic-mappings)

`text-align` sets `text-align-all` and resets `text-align-last` to `auto`. `justify-all` sets both to `justify`, and `match-parent` uses the parent's computed alignment for each longhand. `text-align-last` applies to the final line and before forced line breaks. With `auto`, a justified paragraph aligns those lines at the start. `start` and `end` follow the current horizontal left-to-right layout. Character-based table alignment requires a separate layout feature. [CSS Text alignment](https://www.w3.org/TR/css-text-4/#text-align-property)

`text-justify` accepts `auto`, `none`, `inter-word`, `inter-character` (also `distribute`), and `ruby`, with the optional `no-compress` modifier. Justification expands spaces or shaped cluster boundaries, including across inline elements. `auto` uses word separators and a script-based heuristic for scripts such as Han, Japanese, Korean, and Thai. `ruby` skips word separators and boundaries between Bopomofo characters. The implementation expands spacing, keeps cursive runs connected, and leaves preserved white-space runs at their original spacing. An unexpandable line uses the last-line alignment, falling back to center if that alignment is `justify`. [CSS justification](https://www.w3.org/TR/css-text-4/#text-justify-property)

`text-indent` accepts a length, percentage, or length expression, with optional `each-line` and `hanging` in any order. `each-line` also indents after forced breaks. `hanging` inverts which lines are indented. Percentages use the block's content width for layout and a zero basis for intrinsic measurement. Negative indentation moves the line start outward. [CSS line indentation](https://www.w3.org/TR/css-text-4/#text-indent-property)

`tab-size` accepts a non-negative number, length, or compatible math expression. The default is `8`. A number scales the block container's space advance, including letter and word spacing. Preserved tabs advance to the next stop measured from the block's content edge, with the CSS `0.5ch` minimum-distance rule. A zero interval produces a zero-width tab. In `pre-wrap`, trailing whitespace hangs at soft wraps and can hang if it overflows at a forced break. Selection and copy preserve each tab as one character. With collapsed whitespace, tabs follow the ordinary space rules. [CSS tab stops](https://www.w3.org/TR/css-text-4/#tab-size-property)

`white-space-collapse` supports `collapse`, `preserve`, `preserve-breaks`, `preserve-spaces`, `break-spaces`, and `discard`. `preserve-spaces` replaces tabs and source newlines with spaces. `discard` removes document whitespace during layout. In `break-spaces`, spaces, tabs, and Unicode space separators keep their advances at line ends. They can wrap after each separator, and the first space in a sequence stays with the preceding word. `text-wrap-mode: wrap` or `nowrap` controls soft breaks separately from whitespace. An inline `nowrap` group can move as a unit to a permitted break. Forced breaks still apply. [CSS white space](https://www.w3.org/TR/css-text-4/#white-space-collapse) · [CSS wrapping mode](https://www.w3.org/TR/css-text-4/#text-wrap-mode)

`text-wrap` accepts a mode and a style in either order. Omitted values reset to `wrap` and `auto`. The `text-wrap-style` values `auto` and `stable` use first-fit wrapping. `balance` evens out the space remaining on lines, `pretty` considers line lengths across the group and penalizes very short final lines, and `avoid-short-last-line` considers adjusting the final break. Forced breaks separate groups, and the number of lines in each group stays the same. Balancing applies to groups of up to ten lines and pretty wrapping to groups of up to 24 lines. Groups with preserved tabs, excessive break opportunities, or flow affected by floats use first-fit wrapping. [CSS wrapping styles](https://www.w3.org/TR/css-text-4/#text-wrap-style)

`word-break` accepts `normal`, `keep-all`, `break-all`, `manual`, and the legacy `break-word`. Normal wrapping recognizes Han, Kana, and Hangul boundaries. `keep-all` suppresses letter-to-letter CJK breaks in line layout and intrinsic sizing. `break-all` treats letters and eligible numeric and Southeast Asian character units like ideographs for wrapping. It adds ordinary break opportunities inside words, including in minimum content sizing, while honoring punctuation restrictions and preserving the original shaping and copy offsets. Graphemes that share a native glyph cluster stay together. The legacy `break-word` behaves as `word-break: normal` with `overflow-wrap: anywhere`. [CSS word breaking](https://www.w3.org/TR/css-text-4/#word-break-property)

Thai, Lao, Khmer, and Myanmar text uses the active Godot text server's dictionaries where available. Lookup can span adjacent inline elements. The script supplies a default dictionary locale, and an inherited `lang` for that language supplies the locale variant. Southeast Asian scripts without an available dictionary use grapheme boundaries. The resulting opportunities contribute to minimum content sizing. `word-break: manual` disables this lexical analysis and its fallback while preserving other normal wrapping rules. With wrapping enabled, `<wbr>` and U+200B ZERO WIDTH SPACE remain explicit opportunities. Phrase-based wrapping with `auto-phrase` remains pending. [CSS analytical word breaking](https://www.w3.org/TR/css-text-4/#analytical-word-breaking)

`line-break` accepts `auto`, `loose`, `normal`, `strict`, and `anywhere`. DocRender uses the normal rules for `auto`. Loose wrapping adds opportunities before small kana and iteration marks, between inseparable punctuation marks, and before U+2010 and U+2013 after ideographs. With a declared Chinese or Japanese writing system, normal and loose allow breaks before U+301C and U+30A0. Loose also permits breaks before selected centered punctuation and East Asian suffixes and after East Asian prefixes. An inherited HTML `lang` identifies the writing system, with an explicit script subtag taking precedence. For example, `ja-Latn` follows the Latin rules and `ko-Hani` follows the Chinese rules. [CSS line breaking strictness](https://www.w3.org/TR/css-text-4/#line-break-property)

With `line-break: anywhere`, wrapping can occur between graphemes inside a word and contributes those opportunities to the minimum content width. `nowrap` still suppresses soft breaks, and `break-spaces` permits wrapping before preserved spaces. DocRender draws line fragments from the original shaped text, preserving source offsets for selection and copying. Graphemes that share a native glyph cluster, such as some ligatures, are kept together. The full Unicode line-breaking algorithm is pending. [CSS line breaking strictness](https://www.w3.org/TR/css-text-4/#line-break-property)

`overflow-wrap` accepts `normal`, `anywhere`, and `break-word`. The latter two can break an unbreakable sequence at a grapheme boundary when the line would overflow and has no ordinary break available. Only `anywhere` includes the fallback opportunities in minimum content sizing. Both preserve maximum content width and reuse the same shaped fragments and text offsets as `line-break: anywhere`. The legacy `word-wrap` spelling maps to `overflow-wrap` in the cascade. [CSS overflow wrapping](https://www.w3.org/TR/css-text-4/#overflow-wrap-property)

`hyphens: manual` is the default and allows a break at an authored U+00AD soft hyphen (`&shy;`). `hyphens: none` suppresses that opportunity. The inherited `hyphenate-character` accepts `auto` (U+2010) or a string, including an empty string. A chosen replacement takes up space on its line and contributes to minimum content sizing. The source character stays in the document text and the replacement appears only in the drawing. `white-space: nowrap` suppresses these breaks, and `word-break: break-all` and `line-break: anywhere` use their own opportunities without painting a hyphen. [CSS hyphenation](https://www.w3.org/TR/css-text-4/#hyphenation)

`hyphens: auto` selects embedded patterns using the inherited HTML `lang`. The dictionaries cover US and British English, three German orthographies, Spanish, French, Irish, Galician, Italian, Portuguese, and Ukrainian. Unqualified `en` uses British rules, and unqualified `de` uses the modern orthography. Unknown languages and unsupported scripts use authored opportunities. Automatic points can span adjacent inline elements and are considered during minimum-content sizing. An authored soft hyphen takes priority. The dictionary can supply additional breaks in an authored segment wider than the available line. Lookup recognizes canonical spellings of the bundled alphabets and preserves the original text. Automatic points stay out of the copied text and respect grapheme and native shaped glyph cluster boundaries. [Hyphenation patterns and licenses](../LiteHTML-LICENSES/hyphenation-patterns.txt)

A soft hyphen separates shaping runs. Ligatures and cursive joining across the marker are pending.

`hyphenate-limit-zone` allows a specified amount of space to remain at an unhyphenated line end before splitting the following word. It accepts lengths, percentages of the current line width, and CSS math. A word at the start of a line can still use an eligible break. `hyphenate-limit-lines` accepts `no-limit` or a nonnegative integer. It limits successive hyphenated lines, including breaks with an empty replacement. An unhyphenated line resets the count. Both properties inherit and apply during balanced line wrapping. The zone and positive line limits leave minimum-content opportunities available. A zero line limit disables them. [CSS hyphenation limits](https://www.w3.org/TR/css-text-4/#hyphenate-limit-zone)

`hyphenate-limit-chars` sets the minimum counts for the whole word, before the break, and after the break. It accepts one to three nonnegative integers or `auto` values. The `auto` counts are `5 2 2`. With two components, the second applies on both sides. Godot supplies word and grapheme boundaries across inline markup, with Unicode word-boundary rules retaining punctuation such as apostrophes inside words. Nonspacing marks, punctuation inside a word, and formatting controls are excluded from the counts. A soft hyphen inside a grapheme is ignored for hyphenation. The limits inherit and affect both line wrapping and minimum-content sizing. [CSS hyphenation character limits](https://www.w3.org/TR/css-text-4/#hyphenate-limit-chars) · [Unicode word boundaries](https://www.unicode.org/reports/tr29/#Word_Boundaries)

`hyphenate-limit-last: always` tries to leave the last full line of a block container unhyphenated. It moves material to the next line when possible and retains a hyphen if the alternative adds overflow or keeps moving the same break down. It also applies to balanced wrapping and leaves intrinsic width contributions available. The initial value is `none`. The `column`, `page`, and `spread` values are accepted. Their effects require fragmentation boundaries, which the continuous HTML renderer currently lacks. [CSS last-line hyphenation limits](https://www.w3.org/TR/css-text-4/#hyphenate-limit-last)

`white-space-trim` accepts `none` or a combination of `discard-before`, `discard-after`, and `discard-inner`. The exterior options remove adjacent collapsible whitespace. `discard-inner` removes leading and trailing whitespace in inline boxes. In block containers it trims through the last leading source newline and from the first trailing source newline, preserving indentation and same-line spaces. Trimming uses the in-flow boxes for layout and selection. `HTMLElement.get_text_content()` still returns the source. [CSS white-space trimming](https://www.w3.org/TR/css-text-4/#white-space-trim)

`white-space` supports `normal`, `pre`, `pre-wrap`, and `pre-line`, or any combination of a collapse value, wrapping mode, and trim value. Omitted longhands reset to `collapse`, `wrap`, and `none`. [CSS white-space shorthand](https://www.w3.org/TR/css-text-4/#white-space-property)

`local("Arial Bold")` and `local(Arial-BoldMT)` select an installed face by its full or PostScript name. Windows uses DirectWrite. On other platforms, the lookup depends on the fonts exposed by Godot's OS and text server APIs. Authors can include a project URL as a fallback. A URL fragment, as in `url(fonts/type.ttc#Type-Bold)`, selects the face with that exact PostScript name. Without a fragment, the first face is used. [CSS Fonts source selection](https://www.w3.org/TR/css-fonts-4/#src-desc)

`unicode-range` accepts comma-separated code points, inclusive intervals, or wildcards such as `U+4??`. Faces with the selected style and weight can cover different ranges in the same family. Overlaps use layer and source order. A face must also contain the glyph, and a grapheme cluster uses one face when possible. The first face whose range includes space supplies line-height metrics. [CSS Fonts character ranges](https://www.w3.org/TR/css-fonts-4/#unicode-range-desc)

Family names use Unicode default case folding without normalization. Godot and the OS provide installed font discovery, localized aliases, and face matching. When a generic family has no available candidate, rendering proceeds through the rest of the list and the document default. [CSS Fonts Level 4](https://www.w3.org/TR/css-fonts-4/#font-family-prop)

## Declaration diagnostics

The editor's HTML source preview outlines visible elements with CSS declaration diagnostics. Hover over an outlined element to read the details in an HTML tooltip. For your own `HTMLLabel`, enable **Debug → CSS Diagnostics Enabled** in the Inspector or set `css_diagnostics_enabled` from GDScript. The same setting on a standalone `HTMLLayout` enables outlines in `draw()`.

`HTMLElement.get_css_diagnostics()` returns the declarations associated with that element. `HTMLLabel.get_css_diagnostics()` and `HTMLLayout.get_css_diagnostics()` include the full document's results, including declarations in rules that match no visible element. Each entry includes the property, value, and source. Local stylesheets also include their `res://` path.

| Kind | Example |
| --- | --- |
| `unknown_property` | `scrollbar-width: thin` |
| `unsupported_value` | `grid-template-areas: "head head" "head ."` |

Diagnostics cover declarations reached by the stylesheet parser. For a declaration using `var()`, the value is checked when DocRender resolves it on an element. Drawing diagnostics identify the winning declaration after shorthand expansion. Use `get_render_error()` for parse and layout failures.
