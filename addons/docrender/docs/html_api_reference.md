# HTML API and Web standards

DocRender exposes an HTML element tree and layout geometry to GDScript. This guide maps its public API to the corresponding Web concepts and describes the Godot contracts that matter when porting browser code. [WHATWG DOM](https://dom.spec.whatwg.org/) · [CSSOM View](https://www.w3.org/TR/cssom-view-1/) · [CSS support](html_css_support.md)

## Documents and handles

| Web concept | DocRender API and semantics |
| --- | --- |
| `Document` | `HTMLLayout` owns the live tree. Parse the source with `parse()` before creating, importing, or adopting elements. `HTMLDocument` is a Godot `Resource` for source text, base path, stylesheet, and imported asset references. `HTMLDocument.create_layout()` creates an independent runtime view. |
| `Element` and `HTMLElement` | `HTMLElement` is a `RefCounted` handle. Queries and hit tests return the existing handle for an element while it is held. A handle survives reflow and tree moves. Reparse, `clear()`, or disposal of its current layout invalidates it. Check `is_valid()` before using a retained handle. |
| `Node.isConnected` | `is_connected()` reports current membership in the layout's document tree. A detached element can be valid and editable. Holding a handle to any element of a detached subtree keeps it available for reinsertion. |
| `parentElement`, `children`, and `childNodes` | `get_parent()` returns an element or `null`. `get_children()` returns an `Array[HTMLElement]` snapshot in tree order. `HTMLNode.get_parent_node()` and `get_child_nodes()` include the document, fragments, text, and comments where applicable. CSS pseudo-elements stay in the rendering tree. |
| `getAttribute()`, `classList`, `dataset` | `get_attribute(name, default_value = "")` returns a string. Use `has_attribute()` to distinguish a missing attribute from an empty one. `get_class_list()` returns a `PackedStringArray` and `get_dataset()` a `Dictionary`. Both are snapshots. Use the class helpers and attribute setters to edit the element. |
| `textContent`, `innerHTML` | `get_text_content()` includes parsed text even when hidden and excludes comments and generated content. `get_inner_html()` serializes child markup, including text and comments. Replacing the contents with `set_text_content()` or `set_inner_html()` invalidates handles to the replaced descendants. |

`HTMLLabel` owns an independent layout for display and interaction. Runtime edits apply to that layout. They leave the shared `HTMLDocument` resource and its other views unchanged. Use the HTML APIs on the main thread.

`HTMLLayout.get_document_node()` returns the live tree root. Use `get_document_element()`, `get_head()`, and `get_body()` for its HTML elements. `set_title()` edits that view's title. `HTMLTextNode.data` and `HTMLCommentNode.data` edit character data while retaining the handle. A parsed text node keeps its identity when laid-out across multiple inline runs. Use `get_root_node()`, `contains()`, and `compare_document_position()` to compare positions, or `is_equal_node()` to compare contents and descendants.

## Tree editing and fragments

| WHATWG DOM API | DocRender API and semantics |
| --- | --- |
| `document.createElement()` | `layout.create_element(tag_name)` returns a detached element. Invalid names and unparsed layouts return `null`. Supported element types use their registered `HTMLElement` subclass. |
| `appendChild()`, `insertBefore()` | `append_child()` and `insert_before()` move a same-layout element and preserve its subtree handles. A `null` reference appends. Tree edits return a Godot `Error`, with `OK` for success and `ERR_INVALID_PARAMETER` for invalid operations. |
| `removeChild()`, `replaceChild()`, `Element.remove()` | `remove_child()` and `replace_child()` detach the outgoing subtree for reuse. Use `detach()` for browser-style removal that keeps handles valid. DocRender's `remove()` invalidates the removed subtree's handles when the element has a parent. Calling it on a detached root leaves that root available for insertion. |
| `createDocumentFragment()` and fragment insertion | `layout.create_document_fragment()` and `label.create_document_fragment()` create a same-layout `HTMLDocumentFragment`. Add elements with `fragment.append_child()`. Insert them with `parent.append_fragment(fragment)` or `parent.insert_fragment_before(fragment, reference)`. Success moves the elements in order and empties the fragment for reuse. A rejected batch leaves both trees intact. Direct fragment children return `null` from `get_parent()` and `false` from `is_connected()`. The fragment exposes an element snapshot via `get_children()`. |
| `HTMLTemplateElement.content` | The read-only `content` property returns the same detached fragment on every access. Its contents are outside document queries, styling, and layout. `get_inner_html()` and `set_inner_html()` operate on this fragment. Use `content.clone_node(true)` and `append_fragment()` to instantiate the template without consuming its contents. A shallow copy of a template or its fragment has empty contents. |
| `cloneNode(deep)` and `importNode(node, deep)` | `element.clone_node(deep = false)` copies within the same layout. `target_layout.import_node(element, deep = false)` copies into a different layout or the same layout. Both return new detached handles and retain attributes, including IDs. Deep copies also include text and comments. Imported relative resources resolve from the target layout's base directory. |
| `adoptNode()` | `target_layout.adopt_node(element)` transfers the subtree and its existing handles. The result stays detached until inserted. Same-layout adoption also detaches. Cross-layout tree edits require an explicit import or adoption. Adopting the document root or adopting into an unparsed layout returns `null`. Clearing the old layout after a successful move leaves the transferred handles valid. |

The batch methods `append()`, `prepend()`, `before()`, `after()`, `replace_with()`, and `replace_children()` take arrays of nodes, fragments, and plain text strings. A successful edit consumes inserted fragments and keeps handles to detached nodes. `parse_fragment(html, context)` accepts HTML markup and uses the supplied element's parsing context. `import_fragment()` copies contents to another layout. `adopt_fragment()` moves an independent fragment and retains its existing handles. To move content with its template, adopt the template element.

Clones and imports preserve form controls' current value, checkedness, and corresponding dirty flags. They reset text selection to offset zero. A shallow `<textarea>` copy keeps its current value with empty default text. Adoption preserves selection and ends focus and IME composition in the source view. Adopting a template also transfers its `content` fragment and the existing handles inside it. DocRender restricts template tree edits to the fragment and does not support declarative shadow roots or template patching. [DOM tree mutation and cloning](https://dom.spec.whatwg.org/#interface-node) · [DOM adoption and import](https://dom.spec.whatwg.org/#dom-document-adoptnode) · [HTML template element](https://html.spec.whatwg.org/multipage/scripting.html#the-template-element)

## Selectors

`HTMLLayout`, `HTMLLabel`, and `HTMLElement` expose `query_selector()` and `query_selector_all()`. The single-result method returns the first match or `null`. The multiple-result method returns an `Array[HTMLElement]` snapshot in tree order, with each matching element included once. Hidden elements can match. `get_element_by_id()` compares the raw ID case-sensitively and returns the first match, including when IDs repeat. [DOM selector APIs](https://dom.spec.whatwg.org/#dom-parentnode-queryselectorall)

`HTMLDocumentFragment` also exposes these three methods. `template.content.query_selector_all(":scope > .row")` selects the fragment's direct children with the `row` class. `query_selector(":scope")` returns `null`. Queries can find nested template elements. Query each nested element's `content` to search its contents. [Selectors Level 4 virtual scoping root](https://www.w3.org/TR/selectors-4/#the-scope-pseudo)

An element query searches its descendants. `:scope` refers to the element that started the query. For example, `container.query_selector_all(":scope > .item")` selects its direct children with the `item` class. `matches()` tests the current element, and `closest()` starts with that element before walking up its ancestors. Layout-wide queries scope to the document root. Invalid or unsupported selector syntax reports a Godot error and yields `null`, an empty array, or `false` according to the method. Pseudo-elements are excluded from query syntax. See the [CSS support reference](html_css_support.md#selectors-and-rules) for the supported selector subset.

## Geometry, scrolling, and interaction

| Web concept | DocRender API and semantics |
| --- | --- |
| `getClientRects()` and `getBoundingClientRect()` | `get_client_rects()` returns one `Rect2` per laid-out border-box fragment, including multiple fragments for a wrapped inline element. Bounds include CSS transforms and use document pixels before clipping and `HTMLLabel` scrolling. `get_bounding_rect()` merges those bounds or returns an empty `Rect2` when no box exists. CSSOM View reports geometry relative to the browser viewport. |
| Element layout metrics | `get_client_size()` and `get_client_offset()` return the padding box size and the border inset. `get_offset_rect()` returns untransformed layout geometry relative to the `get_offset_parent()` padding edge. `get_scroll_size()` measures the scrollable extent. |
| `elementFromPoint()` | `HTMLLayout.get_element_at(point)` accepts document coordinates. For standalone drawing, subtract the `draw()` offset from the canvas-local point and apply the caller's clip. `HTMLLabel.get_element_at(point)` accepts label-local coordinates and accounts for the scroll offset and visible content area. |
| `elementsFromPoint()` | `get_elements_at(point)` uses the same coordinate spaces and returns the hit elements from front to back. The array includes each element once. |
| `scrollIntoView()` | `HTMLLabel.scroll_to_element(element, align = 0.0)` takes a laid-out element from that label's layout. `align` positions its top within the visible height between `0.0` at the top and `1.0` at the bottom, subject to the scroll range. `scroll_to_anchor(id, align = 0.0)` targets an HTML ID. Both return `false` when the target is unavailable or layout is pending. |
| Scroll alignment and inner offsets | `HTMLLabel.scroll_element_into_view()` accepts `block`, `inline`, and `behavior` options. Use `HTMLElement.get_scroll_offset()`, `set_scroll_offset()`, `scroll_to()`, and `scroll_by()` for an inner scroll container. Use `HTMLLabel`'s scroll offset for the outer viewport. |
| `Range.getClientRects()` | `HTMLRange.get_client_rects()` projects the live DOM endpoints onto the visible text rectangles in document coordinates. `HTMLLayout.get_selection_rects(from, to)` accepts offsets into `get_plain_text()` directly. Both use the last layout and account for transforms and clipping. |
| Document text selection | `HTMLLabel.find_text()` returns offsets into `get_plain_text()`. Pass those offsets to `set_selection_range()`, read current endpoints with `get_selection_range()`, and use `get_selection_rects()` or `copy_selection()` for the current selection. |
| Focus, activation, and text input | `HTMLLabel.focus_element()` and `activate_element()` use the control's DOM focus and activation handling. Subscribe to Godot signals such as `link_pressed`, `button_pressed`, and `input_changed`. The caller handles external navigation. Text inputs and textareas use Godot string offsets snapped to grapheme boundaries. The Web text control API uses UTF-16 code units. |

DOM queries reflect successful edits immediately. Geometry requires layout. After changing a standalone document, call `layout(viewport_size)` to measure immediately. Layouts with a previous viewport can also reflow on a deferred call. For `HTMLLabel`, wait for `layout_changed` before reading geometry or scrolling to a newly inserted element. The label batches connected tree edits, including insertion of a nonempty fragment, into a layout update. [CSSOM View element geometry and scrolling](https://www.w3.org/TR/cssom-view-1/#extensions-to-the-element-interface) · [HTML text control selections](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#textFieldSelection)

## Live ranges, selection, and mutation observers

Use `HTMLLayout.create_range()` or `HTMLLabel.create_range()` after parsing a document. An endpoint is a node and a zero-based offset. Text and comments count Godot string characters. Other nodes count logical children, including text and comments. A range tracks insertions, removals, text edits, text splitting, and `normalize()`. Removing a subtree relocates endpoints inside it to its former position in the parent. Extracting preserves the handles of fully selected nodes. `get_text()` returns source text. For a label, `get_selected_text()` returns the rendered, copyable text. [DOM live ranges](https://dom.spec.whatwg.org/#concept-live-range)

`get_selection()` returns the layout's single selection object. It holds a live range and a direction. `set_base_and_extent()` accepts a backwards selection, and `extend()` moves the focus while retaining the anchor. Dragging or using `set_selection_range()` in `HTMLLabel` also updates the logical selection where the rendered offsets map to source text. Selection stays anchored to the same nodes when text is inserted before it. Clearing or reparsing the layout invalidates its ranges and selection. [Selection API](https://w3c.github.io/selection-api/)

Instantiate `HTMLMutationObserver`, connect `mutations_delivered`, and call `observe(target, options)`. Options include `child_list`, `attributes`, `character_data`, `subtree`, `attribute_filter`, `attribute_old_value`, and `character_data_old_value`. Records report `type` (`childList`, `attributes`, or `characterData`), `target`, `added_nodes`, `removed_nodes`, `previous_sibling`, `next_sibling`, `attribute_name`, and `old_value`. Child changes use logical DOM nodes. A mixed batch emits one child-list record for each affected parent. `take_records()` drains pending records synchronously. In a deferred delivery, each observer gets a snapshot. Edits made by a callback go to a later batch. A removed subtree is observed by its former ancestors until delivery. The legacy `remove()` and `set_inner_html()` methods still invalidate their removed handles. [DOM mutation observers](https://dom.spec.whatwg.org/#mutation-observers)

## Forms

The form, label, fieldset, legend, select, option, and optgroup elements expose typed handles. Forms expose `get_controls()`, `get_form_data()`, and `reset()`. For a connected form, `HTMLLabel.submit_form()` checks `required` and emits either `form_submitted` with an ordered array of name and value dictionaries or `form_validation_failed` with the first invalid control. The game handles ordinary submissions through the signal. Reset buttons emit `form_reset`. [WHATWG form control infrastructure](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html)

`method="dialog"` submits a form and closes its nearest ancestor dialog. The submitter's `formmethod` can override the form method. A supplied `value` becomes the dialog's `return_value`, including when the value is empty. An omitted value or submitter preserves the result. The `form_submitted` signal is emitted when the submission succeeds. The dialog's `closed` signal follows focus restoration. [WHATWG form submission algorithm](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#form-submission-algorithm)

Inputs support single-line text modes, numbers, range sliders, checkboxes, radios, hidden fields, and buttons. Text input suggestions use `<datalist>` with a local `list` ID. Suggestions follow edits in an eligible field. Press Down to open the list with the keyboard, Enter to accept, and Escape to dismiss. An output element restores its default text on form reset.

A single-row select uses a dropdown. The `multiple` attribute or a `size` above one enables a list box. Multi-select submissions emit one ordered form entry per selected enabled option. Use `select_changed` for user actions. The `form_submitted` submitter argument is an `HTMLElement` handle. Scripts can check for `HTMLButtonElement` and `HTMLInputElement` to distinguish the two kinds of submitter.

### Input types

`HTMLInputElement` covers the 22 HTML input states. Date and time values use the HTML civil date formats. GDScript can read date parts with `get_value_as_date()`, step numeric or temporal fields with `step_up()` and `step_down()`, and inspect constraint flags with `get_validity()`. The `pattern` attribute uses Godot's RegEx syntax. [WHATWG input states](https://html.spec.whatwg.org/multipage/input.html#states-of-the-type-attribute)

The color input opens a canvas-drawn picker with saturation, brightness, hue, and RGB, HSL or hexadecimal entry. The swatch previews edits while the panel is open. Enter or a click outside applies the color, and Escape discards the preview. The eyedropper is available on display backends that support screen capture.

File entries use the same `{name, value}` structure as text entries. Each file value is an `HTMLInputFile` with `name`, `size`, and `read_bytes(offset, count)`. The picker exposes the file name and content without exposing its system path. An image submitter contributes two coordinate entries, prefixed with its name when present.

The [GDScript WPT ports](../../../tests/html/wpt/dom.gd) exercise selected tree and selector behaviors through the public API. Each case identifies its upstream source. [Building and running the HTML tests](../../../CONTRIBUTING.md#build-and-test)

## Lists and preformatted text

`HTMLPreElement` and `HTMLUListElement` expose the shared `HTMLElement` API. `HTMLOListElement` reflects `start`, `reversed`, and `type`. `HTMLLIElement` reflects `value` and the legacy `type` attribute. When `start` is absent, the property reads as `1`, and a reversed list starts numbering from its item count. `<aside>` uses `HTMLElement`. [WHATWG grouping content](https://html.spec.whatwg.org/multipage/grouping-content.html)

Document nodes expose `HTMLHtmlElement`, `HTMLHeadElement`, `HTMLBodyElement`, `HTMLTitleElement`, `HTMLMetaElement`, `HTMLBaseElement`, and `HTMLScriptElement` through the same queries and DOM creation API. `HTMLTitleElement.text` exposes the original title text. `HTMLLayout.get_title()` collapses ASCII whitespace. Script elements expose their original text and attributes. [WHATWG document metadata](https://html.spec.whatwg.org/multipage/semantics.html#document-metadata)

## Document behaviors

Assign a GDScript resource that extends `HTMLBehavior` to the `inventory` entry of the label's `behavior_scripts` dictionary and enable `behaviors_enabled` on that label. The dictionary is saved with the scene as resource references. In GDScript, assign the dictionary back after editing the returned copy.

```html
<script type="text/gdscript" src="inventory"></script>
<section id="inventory"><h2>Inventory</h2></section>
```

```gdscript
extends HTMLBehavior

func _attach_document(dom: HTMLDocumentNode) -> void:
	var inventory: HTMLElement = dom.get_element_by_id("inventory")
	if inventory == null:
		return
	var status: HTMLElement = dom.create_element("p")
	status.set_text_content("Inventory ready")
	inventory.append_child(status)
```

The [`HTMLDocumentNode`](#documents-and-handles) is the live DOM for this view. Use it to query elements and create nodes or fragments. The behavior's read-only `document` property exposes the same handle through `_detach()`. Override `_attach(label: HTMLLabel)` if you need host control methods or signals such as `button_pressed`. When both hooks are defined, `_attach_document()` takes precedence.

The label scans the connected document after its first completed layout. Each matching declaration gets a separate instance in document order. The host matches `src` to a registered name and constructs the corresponding script without arguments. Document replacement detaches behaviors in reverse order while the old DOM is still available. Disabling behaviors, replacing the registry, and exiting the scene tree also call `_detach()`. A new mount scans the DOM again. Mounting runs in the game, and ordinary DOM edits do not start a new mount.

`get_base_uri()` on `HTMLLayout`, `HTMLDocumentNode`, and `HTMLLabel` exposes the effective base. DocRender resolves the first connected `<base href>` in document order against the source file or the configured base directory. Supported bases are relative paths within a Godot resource scheme, `res://`, `user://`, and resolvable `uid://` paths. Invalid and blocked bases fall back to the source. Relative image, audio, video and inline CSS URLs use the effective base. External CSS and its nested imports use their own file locations. The HTML importer collects project dependencies for the active base at import time. [WHATWG document base URLs](https://html.spec.whatwg.org/multipage/urls-and-fetching.html#document-base-urls)

`HTMLAnchorElement.get_resolved_href()` exposes a link resolved against the effective base. `link_pressed` passes the original `href`, and fragment-only links scroll within their existing label.

## Sections, disclosure, and gauges

DocRender exposes typed `HTMLArticleElement` and `HTMLSectionElement` handles for block sectioning content. The HTML standard exposes both as `HTMLElement`. Description lists, menu lists, quotes, and line and thematic breaks expose their corresponding reflected properties.

Use `HTMLDetailsElement.open` to change disclosure state. A named group opens one disclosure at a time, and a closed disclosure keeps its DOM available to selectors. The first summary can be focused and activated by the pointer or keyboard. `HTMLProgressElement` and `HTMLMeterElement` expose normalized numeric values. A progress indicator with no value attribute is indeterminate and advances with the label's animation process mode.

## Tables

`HTMLTableElement`, `HTMLTableSectionElement`, `HTMLTableRowElement`, `HTMLTableCellElement`, `HTMLTableColElement`, and `HTMLTableCaptionElement` are typed element handles. The table exposes `caption`, `t_head`, `t_foot`, `get_t_bodies()`, and `get_rows()`. Sections expose `get_rows()`, and rows expose `get_cells()`. These methods return typed array snapshots. Table rows are ordered by head, body and direct rows, then foot. Hidden DOM elements count toward indices. Nested table rows and anonymous layout boxes are outside the collection. [WHATWG table elements](https://html.spec.whatwg.org/multipage/tables.html#the-table-element)

Use `insert_row()` and `insert_cell()` to create a row or a data cell. An empty table uses its last body or creates one. Insert a `th` using `create_element("th")` and `insert_before()` or `append_child()`. Deletion detaches the row or cell and retains its handle. Index `-1` appends on insertion and removes the last item on deletion. Deleting the last item of an empty collection returns `OK`. Invalid insertion returns `null` and invalid deletion returns `ERR_INVALID_PARAMETER`. A table part setter accepts a matching handle from the same layout or `null`. [WHATWG row and cell operations](https://html.spec.whatwg.org/multipage/tables.html#the-tr-element)

## CSSOM

Use `HTMLElement.get_style_property()`, `get_style_property_priority()`, and `get_style_properties()` to inspect inline declarations. `set_style_property(name, value, "important")` sets an important declaration. Use an empty priority for an ordinary declaration. `remove_style_property()` returns the old value. Shorthand expansion can preserve the other longhands when their values can be serialized separately. Use a complete shorthand edit for a pending shorthand such as `margin: var(--sides)`. [CSSOM declaration blocks](https://drafts.csswg.org/cssom/#css-declaration-blocks)

`HTMLLayout` and `HTMLLabel` expose `get_computed_style()` and `get_computed_style_property()`. They return serialized values, including sampled animation values. Percentages that depend on a layout box may still appear as percentages. The optional pseudo argument accepts `::before`, `::after`, `::marker`, `::placeholder`, `::selection`, and `::backdrop` for the supported properties of those pseudos. Computed properties without a serializer return an empty string. Use the element's rectangle methods to read layout geometry. [CSSOM computed styles](https://drafts.csswg.org/cssom/#dom-window-getcomputedstyle)

`HTMLDialogElement` opens a regular dialog with `show()` or a connected modal with `show_modal()`. Inside an `HTMLLabel`, the topmost modal limits pointer interaction and Tab navigation to its own content. `close()` optionally records a string result and emits `closed` after focus restoration. Direct changes to the `open` attribute update visibility and modal state. [HTML dialog](https://html.spec.whatwg.org/multipage/interactive-elements.html#the-dialog-element)

`closed_by` reports the effective `closedby` policy. `any` allows Escape and outside clicks within the label, `closerequest` allows Escape, and `none` requires an explicit call or form submission. An omitted or invalid policy defaults to `closerequest` for a modal and `none` for a regular dialog. Escape follows menu and IME handling. An outside press and release must target the same active dialog. [HTML dialog light dismiss](https://html.spec.whatwg.org/multipage/interactive-elements.html#dialog-light-dismiss)

`request_close()` emits `close_requested` regardless of the policy. The request reports whether a script, Escape, or an outside click initiated it. A synchronous listener can call `prevent_default()` for any of those sources. An accepted request uses the regular closing and focus restoration path. `close()` and `method="dialog"` submissions close directly. [HTML close requests](https://html.spec.whatwg.org/multipage/interactive-elements.html#dom-dialog-requestclose)

`get_stylesheets()` returns author stylesheet records with `id`, `owner`, `source`, `path`, `media`, and `disabled`. The source is `style`, `link`, or `stylesheet` for the additional stylesheet. Pass an ID to `get_css_rules()` to read a snapshot of the top-level rules, or use `insert_rule()` and `delete_rule()` with zero-based indices to edit them. Edits use the sheet's source order, base path, and media conditions. The edits are local to the layout and expire on a new parse. Use `supports_css()`, `supports_selector()`, and `matches_media()` to query the shared CSS parsers and the layout viewport. [CSSOM stylesheets](https://drafts.csswg.org/cssom/#css-style-sheets)

`HTMLLinkElement.sheet` and `HTMLStyleElement.sheet` return a read-only reference to an `HTMLStyleSheet`, or `null` when no sheet is associated. The handle reads and edits rules through the same CSSOM methods as the layout. Changes to `media` and `disabled` keep the handle and its edits. Replacing a link's `href` or a style element's text invalidates the old handle and clears those edits, even if the source is restored before the next frame. `get_stylesheet_id()` retains its existing numeric ID behavior. An initially disabled local stylesheet can be enabled at runtime, including when its text and dependencies are baked into an exported `HTMLDocument`.

## Animation playback

`HTMLLayout.get_animations()` and `HTMLLabel.get_animations()` return CSS animations, active transitions, and script animations. Pass an element to filter the result, or use `HTMLElement.get_animations()`. A handle identifies the target, the animation name or transition property, and any generated pseudo-element. Use `play()`, `pause()`, `cancel()`, `finish()`, `reverse()`, and `set_current_time()` to control it. Times are in seconds. [Web Animations playback](https://www.w3.org/TR/web-animations-1/#the-animation-interface)

`HTMLElement.animate()` accepts an array of keyframe dictionaries and a timing dictionary. Use CSS names such as `background-color` for the keys. A frame may also specify `offset` from `0.0` to `1.0` and a CSS `easing`. The timing dictionary accepts `duration`, `delay`, `iterations`, `easing`, `direction`, and `fill`. Script animations use the same property interpolators and styling priority as CSS animations. They follow CSS animations on the same element in composition order.

```gdscript
var frames: Array[Dictionary] = [
    {"opacity": 0.0},
    {"opacity": 1.0},
]
var animation: HTMLAnimation = element.animate(frames, {"duration": 0.4, "fill": "both"})
label.animation_process_mode = HTMLLabel.ANIMATION_PROCESS_MANUAL
label.advance_animation(0.2)
```

The label's Idle, Physics, and Manual modes share one timeline. A standalone `HTMLLayout` uses the scene's process frames. Removing a target or replacing the parsed document invalidates its handles.

## Audio

`<audio controls>` draws a player inside `HTMLLabel`. It starts at 300 by 54 CSS pixels and uses the document's color scheme. The controls support play and pause, seeking, muting, volume, and keyboard focus. Audio follows the playback clock in all three CSS animation modes. Editor previews start audio through the controls.

Use an audio asset imported by Godot or the optional FMOD plugin. For example, `<audio controls src="../media/voice.ogg"></audio>` resolves the path relative to the HTML source. Alternatively, omit `src` and provide child `<source src="…">` entries in preference order. `HTMLAudioElement.play()` returns a Godot error if no source can be loaded with the selected backend.

Set `HTMLLabel.bus` to the output bus. When the FMOD extension is loaded, enable `use_fmod_bus` to select the FMOD player and its bus list. Choose the respective importer for each candidate in Godot's Import dock. A candidate has the resource type produced by its importer, either `AudioStream` or `FmodAudioStream`.

The HTML importer records the audio resources as dependencies. Selected-scene exports can resolve their original `res://` paths through Godot's import remaps in the PCK. Include the DocRender extension descriptor in the preset's resource filter, along with the FMOD descriptor when that backend is selected. For audio paths assigned only at runtime, include the resources in the export preset or reference them from an exported resource. [Godot resource loading after export](https://docs.godotengine.org/en/stable/classes/class_fileaccess.html#description)

## Godot resource URLs

HTML and CSS asset URLs accept `res://`, `user://`, and `uid://`. A resource UID resolves to its registered Godot path before the asset type is checked. Relative URLs use the containing stylesheet's directory or the HTML base directory. Both `res://` and `user://` support relative traversal within their own root. [Godot resource UIDs](https://docs.godotengine.org/en/stable/classes/class_resourceuid.html)

Importing HTML records project assets, including those addressed by UID, as export dependencies. Assets under `user://` are loaded at runtime and belong to the running project's user data. This supports local stylesheets, PNG, JPEG, WebP and SVG images, fonts, and WAV, Ogg Vorbis and MP3 audio. The selected image or audio source reports its resolved path.

Absolute `http://` and `https://` URLs can supply PNG, JPEG and WebP images to `<img>`, `srcset`, `<picture>`, and CSS backgrounds. The HTML preview and exported projects load them asynchronously through the same in-memory request cache as Markdown. Pending or failed images can display `alt` text. A background arriving later updates its painted size without changing the element's box. Imported documents record local dependencies. Remote images require a network connection when they are first displayed.

`<link rel="stylesheet">` and `@import` can load HTTP(S) stylesheets asynchronously. Responses are limited to 1 MiB of UTF-8 text and use `text/css` when a Content-Type header is present. A linked `sheet` becomes available when its response is accepted. Styles and imports are applied in their original source order. CSSOM rule snapshots retain each `@import` statement. Each remote stylesheet resolves relative imports and image URLs against its own final response URL, including URLs in rules added through CSSOM. Remote references in imported HTML are fetched when the document is rendered.
