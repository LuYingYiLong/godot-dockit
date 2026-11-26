# Markdown pipeline regression

Build the debug extension first. Make `addons/dockit` available under this test project (a local directory junction to the repository's `addons/dockit` is sufficient), and create `.godot/extension_list.cfg` containing `res://addons/dockit/dockit.gdextension`.

Run Godot 4.7.2 with:

```powershell
godot --headless --path tests/markdown_pipeline --script run.gd
```

The suite checks StyleBox conversions, nine-patch resources, Markdown/HTML tables, shared stylesheets, code indentation, and streaming. A nonzero exit code indicates a failed check.

Run `--script html_layout.gd` in the same project to check HTML geometry, text,
selection and hit-test equivalence against fresh layouts, scrollbar disappearance,
intrinsic measurement reuse, and paint-only hover statistics. This suite sets an
explicit root viewport size so synthetic input is routed in headless mode.
It also covers range values, modal dialogs, slot fallback text, CSS animation,
asynchronous images served over loopback, and screen/print layout isolation.

In debug builds, set `DOCKIT_HTML_LAYOUT_REFERENCE=1` before launching the HTML
suite to use the complete calculation reference path. Its `HTML layout signature`
must match the normal run. Optimization-specific counter assertions are disabled
for the reference run; behavior assertions remain enabled.
