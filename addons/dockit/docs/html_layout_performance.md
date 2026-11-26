# HTML layout performance

`HTMLLabel` and `HTMLLayout` expose optional cumulative statistics:

```gdscript
html_label.set_layout_statistics_enabled(true)
html_label.reset_layout_statistics()
# Perform the operation to measure and wait for its deferred refresh
await get_tree().process_frame
await get_tree().process_frame
var statistics: Dictionary = html_label.get_layout_statistics()
print(statistics.layout_count, " layouts, ", statistics.layout_usec, " us")
```

Statistics are disabled by default. Switching them off retains existing counters;
`reset_layout_statistics()` clears them without invalidating the document. Reading
statistics does not flush queued work. Parsing and clearing a document retain the
statistics switch and counters.

Each phase has a `_count` and `_usec` entry. Times are cumulative microseconds;
nested phases overlap, so adding their times does not give a total frame time.

| Phase | Meaning |
| --- | --- |
| `refresh_requests` | Host refresh requests before deferred coalescing; count only |
| `refresh` | Actual HTMLLabel refresh calls |
| `style` | Style preparation, recomputation, or hover style checks |
| `render_tree` | Explicit render tree reconstruction, including restyling |
| `measurement` | Intrinsic inline size cache misses |
| `measurement_cache_hits` | Reused intrinsic inline sizes; count only |
| `layout` | Calls to litehtml document layout, including convergence passes |
| `geometry` | Visual geometry index reconstruction |
| `paint` | Full drawing command recording |
| `text_index` | Logical text, accessibility text, and selection range indexing |
| `anchors` | Anchor collection |
| `tab_order` | Focus navigation collection |
| `scrollbar` | Host scrollbar convergence iterations |
| `interaction` | Host interaction updates |
| `hover` | Hover chain and dependent style updates |

Host phases remain zero for standalone HTMLLayout. Parsing is not a separately
timed phase. Use a warm document when measuring layout rather than initial loading.

Unchanged layouts reuse their viewport results. When layout inputs change,
automatic scrollbars converge from unconditional gutters again, allowing bars to
disappear when content shrinks. Ordinary HTMLLabel documents prepare viewport
styles before measuring content; container queries and CSS animation/transition
states keep the complete layout path.

DOM edits conservatively invalidate styles and layout. Intrinsic measurements can
survive convergence passes when their content/style generation and constraints
match. Pure drawing changes reuse geometry and anchors. Logical text indexes also
reuse their cached ranges when the recorded text commands still match; embedded
objects and ellipsized content use complete text indexing.

Actual layout changes still reflow the whole document. These caches do not provide
subtree layout or partial drawing command recording.

Run `tests/markdown_pipeline/html_layout.gd` for geometry/text/hit-test comparisons
against fresh layouts and counter assertions. `tests/markdown_pipeline/run.gd`
covers the shared Markdown layout pipeline and streaming source mapping.

Debug builds also accept `DOCKIT_HTML_LAYOUT_REFERENCE=1` at process startup to
exercise the complete calculation path. The same regression script emits a layout
signature for comparison. This switch is ignored in release builds.

Counter comparisons for the 320 × 180 regression fixtures:

| Operation | Complete calculation reference | Cached path |
| --- | --- | --- |
| Stable overflowing document, one coalesced refresh: layouts | 2 | 0 |
| Automatic scrollbar convergence: intrinsic measurements | 2 | 1 |
| Reserved scrollbar gutter, initial layouts | 2 | 1 |
| Color hover: logical text index rebuilds | 1 | 0 |
| Color hover: anchor collections | 1 | 0 |

These counts describe the fixtures, not a speedup guarantee for other documents.
