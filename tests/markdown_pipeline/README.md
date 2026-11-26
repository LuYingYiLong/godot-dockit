# Markdown pipeline regression

Build the debug extension first. Make `addons/dockit` available under this test project (a local directory junction to the repository's `addons/dockit` is sufficient), and create `.godot/extension_list.cfg` containing `res://addons/dockit/dockit.gdextension`.

Run Godot 4.7.2 with:

```powershell
godot --headless --path tests/markdown_pipeline --script run.gd
```

The suite checks StyleBox conversions, nine-patch resources, Markdown/HTML tables, shared stylesheets, code indentation, and streaming. A nonzero exit code indicates a failed check.
