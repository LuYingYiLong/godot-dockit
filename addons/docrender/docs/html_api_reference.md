# HTML API and Web standards

DocRender exposes an HTML element tree and layout geometry to GDScript. This guide maps its public API to the corresponding Web concepts and describes the Godot contracts that matter when porting browser code. [WHATWG DOM](https://dom.spec.whatwg.org/) · [CSSOM View](https://www.w3.org/TR/cssom-view-1/) · [CSS support](html_css_support.md)

## Documents and handles

| Web concept | DocRender API and semantics |
| --- | --- |
| `Document` | `HTMLLayout` owns the live tree. Parse the source with `parse()` before creating, importing, or adopting elements. `HTMLDocument` is a Godot `Resource` for source text, base path, stylesheet, and imported asset references. `HTMLDocument.create_layout()` creates an independent runtime view. |
| `Element` and `HTMLElement` | `HTMLElement` is a `RefCounted` handle. Queries and hit tests return the existing handle for an element while it is held. A handle survives reflow and tree moves. Reparse, `clear()`, or disposal of its current layout invalidates it. Check `is_valid()` before using a retained handle. |
| `Node.isConnected` | `is_connected()` reports current membership in the layout's document tree. A detached element can be valid and editable. Holding a handle to any element of a detached subtree keeps it available for reinsertion. |
| `parentElement`, `children` | `get_parent()` returns an element or `null`. `get_children()` returns an `Array[HTMLElement]` snapshot in tree order. Text, comments, and generated pseudo-elements have no public handles. |
| `getAttribute()`, `classList`, `dataset` | `get_attribute(name, default_value = "")` returns a string. Use `has_attribute()` to distinguish a missing attribute from an empty one. `get_class_list()` returns a `PackedStringArray` and `get_dataset()` a `Dictionary`. Both are snapshots. Use the class helpers and attribute setters to edit the element. |
| `textContent`, `innerHTML` | `get_text_content()` includes parsed text even when hidden and excludes comments and generated content. `get_inner_html()` serializes child markup, including text and comments. Replacing the contents with `set_text_content()` or `set_inner_html()` invalidates handles to the replaced descendants. |

`HTMLLabel` owns an independent layout for display and interaction. Runtime edits apply to that layout. They leave the shared `HTMLDocument` resource and its other views unchanged. Use the HTML APIs on the main thread.

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

Clones and imports preserve form controls' current value, checkedness, and corresponding dirty flags. They reset text selection to offset zero. A shallow `<textarea>` copy keeps its current value with empty default text. Adoption preserves selection and ends focus and IME composition in the source view. Adopting a template also transfers its `content` fragment and the existing handles inside it. DocRender restricts template tree edits to the fragment and does not support declarative shadow roots or template patching. [DOM tree mutation and cloning](https://dom.spec.whatwg.org/#interface-node) · [DOM adoption and import](https://dom.spec.whatwg.org/#dom-document-adoptnode) · [HTML template element](https://html.spec.whatwg.org/multipage/scripting.html#the-template-element)

## Selectors

`HTMLLayout`, `HTMLLabel`, and `HTMLElement` expose `query_selector()` and `query_selector_all()`. The single-result method returns the first match or `null`. The multiple-result method returns an `Array[HTMLElement]` snapshot in tree order, with each matching element included once. Hidden elements can match. `get_element_by_id()` compares the raw ID case-sensitively and returns the first match, including when IDs repeat. [DOM selector APIs](https://dom.spec.whatwg.org/#dom-parentnode-queryselectorall)

`HTMLDocumentFragment` also exposes these three methods. `template.content.query_selector_all(":scope > .row")` selects the fragment's direct children with the `row` class. `query_selector(":scope")` returns `null`. Queries can find nested template elements. Query each nested element's `content` to search its contents. [Selectors Level 4 virtual scoping root](https://www.w3.org/TR/selectors-4/#the-scope-pseudo)

An element query searches its descendants. `:scope` refers to the element that started the query. For example, `container.query_selector_all(":scope > .item")` selects its direct children with the `item` class. `matches()` tests the current element, and `closest()` starts with that element before walking up its ancestors. Layout-wide queries scope to the document root. Invalid or unsupported selector syntax reports a Godot error and yields `null`, an empty array, or `false` according to the method. Pseudo-elements are excluded from query syntax. See the [CSS support reference](html_css_support.md#selectors-and-rules) for the supported selector subset.

## Geometry, scrolling, and interaction

| Web concept | DocRender API and semantics |
| --- | --- |
| `getClientRects()` and `getBoundingClientRect()` | `get_client_rects()` returns one `Rect2` per laid-out border-box fragment, including multiple fragments for a wrapped inline element. Bounds include CSS transforms and use document pixels before clipping and `HTMLLabel` scrolling. `get_bounding_rect()` merges those bounds or returns an empty `Rect2` when no box exists. CSSOM View reports geometry relative to the browser viewport. |
| `elementFromPoint()` | `HTMLLayout.get_element_at(point)` accepts document coordinates. For standalone drawing, subtract the `draw()` offset from the canvas-local point and apply the caller's clip. `HTMLLabel.get_element_at(point)` accepts label-local coordinates and accounts for the scroll offset and visible content area. |
| `scrollIntoView()` | `HTMLLabel.scroll_to_element(element, align = 0.0)` takes a laid-out element from that label's layout. `align` positions its top within the visible height between `0.0` at the top and `1.0` at the bottom, subject to the scroll range. `scroll_to_anchor(id, align = 0.0)` targets an HTML ID. Both return `false` when the target is unavailable or layout is pending. |
| `Range.getClientRects()` | `HTMLLayout.get_selection_rects(from, to)` uses character offsets into `get_plain_text()`. It returns visible highlights in document coordinates, accounting for transforms and clipping. The public API uses a pair of offsets where the DOM `Range` API uses node and offset pairs. |
| Focus, activation, and text input | `HTMLLabel.focus_element()` and `activate_element()` use the control's DOM focus and activation handling. Subscribe to Godot signals such as `link_pressed`, `button_pressed`, and `input_changed`. The caller handles external navigation. Text inputs and textareas use Godot string offsets snapped to grapheme boundaries. The Web text control API uses UTF-16 code units. |

DOM queries reflect successful edits immediately. Geometry requires layout. After changing a standalone document, call `layout(viewport_size)` to measure immediately. Layouts with a previous viewport can also reflow on a deferred call. For `HTMLLabel`, wait for `layout_changed` before reading geometry or scrolling to a newly inserted element. The label batches connected tree edits, including insertion of a nonempty fragment, into a layout update. [CSSOM View element geometry and scrolling](https://www.w3.org/TR/cssom-view-1/#extensions-to-the-element-interface) · [HTML text control selections](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#textFieldSelection)

## Forms

The form, label, fieldset, legend, select, option, and optgroup elements expose typed handles. Forms expose `get_controls()`, `get_form_data()`, and `reset()`. For a connected form, `HTMLLabel.submit_form()` checks `required` and emits either `form_submitted` with an ordered array of name and value dictionaries or `form_validation_failed` with the first invalid control. The game handles submission. Reset buttons emit `form_reset`. [WHATWG form control infrastructure](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html)

Text inputs, checkboxes, radios, hidden inputs, textareas, and single-select dropdowns are supported. Dropdowns use a CSS-styled trigger and a label-local custom menu. Use `select_changed` for user selections. Form transport, browser validation UI, multiple selection, and list boxes are outside the current API.

The [GDScript WPT ports](../../../tests/html/wpt/dom.gd) exercise selected tree and selector behaviors through the public API. Each case identifies its upstream source. [Building and running the HTML tests](../../../CONTRIBUTING.md#build-and-test)
