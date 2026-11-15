"""Check that editor tab restoration preserves imported source references."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory


PROBE = '''@tool
extends EditorPlugin

const SOURCE_MARKERS: Array[String] = [
	"Shared document",
	"LaTeX reference",
	"Mermaid MMD reference",
	"Mermaid long extension reference",
]


func _enter_tree() -> void:
	if OS.get_environment("DOCRENDER_REFERENCE_PROBE") == "1":
		_probe.call_deferred()


func _probe() -> void:
	var scene: Node
	var restored_sources: Dictionary[String, bool] = {}
	for frame: int in range(240):
		await get_tree().process_frame
		scene = EditorInterface.get_edited_scene_root()
		for editor: ScriptEditorBase in EditorInterface.get_script_editor().get_open_script_editors():
			var source: CodeEdit = editor.get_base_editor() as CodeEdit
			if editor.is_class("TextEditor") and source != null:
				for marker: String in SOURCE_MARKERS:
					if source.text.contains(marker):
						restored_sources[marker] = true
		if scene != null and scene.has_node("Second") and restored_sources.size() == SOURCE_MARKERS.size():
			break
	if scene == null or not scene.has_node("Second") or restored_sources.size() != SOURCE_MARKERS.size():
		push_error("Editor did not restore the fixture scene and source tabs: %s" % [restored_sources])
		get_tree().quit(1)
		return
	for frame: int in range(4):
		await get_tree().process_frame
	var first: HTMLLabel = scene.get_node("First") as HTMLLabel
	var second: HTMLLabel = scene.get_node("Second") as HTMLLabel
	var packed: PackedScene = PackedScene.new()
	if packed.pack(scene) != OK or ResourceSaver.save(packed, "res://roundtrip.tscn") != OK:
		push_error("Could not save the restored scene")
		get_tree().quit(1)
		return
	var saved: String = FileAccess.get_file_as_string("res://roundtrip.tscn")
	var embedded: bool = '[sub_resource type="HTMLDocument"' in saved and 'text = "Embedded document"' in saved
	if first.document != second.document or not embedded:
		push_error("Editor changed HTML resource sharing or its embedded resource classification")
		get_tree().quit(1)
		return
	var expected: Dictionary[String, Resource] = {
		"res://source.html": first.document,
		"res://source.tex": scene.get(&"latex_document") as Resource,
		"res://source.mmd": scene.get(&"mermaid_document") as Resource,
		"res://other.mermaid": scene.get(&"other_document") as Resource,
	}
	var passed: bool = true
	for path: String in expected:
		var document: Resource = expected[path]
		if document == null:
			push_error("Editor did not restore the document at %s" % path)
			passed = false
			continue
		var external: bool = false
		for line: String in saved.split("\\n", false):
			if line.begins_with('[ext_resource type="%s"' % document.get_class()) and 'path="%s"' % path in line:
				external = true
				break
		var cache_matches: bool = ResourceLoader.get_cached_ref(path) == document
		if document.resource_path != path or not cache_matches or not external:
			push_error("Changed reference for %s (resource_path=%s, cache_matches=%s, external=%s)" % [
				path, document.resource_path, cache_matches, external,
			])
			passed = false
	if not passed:
		get_tree().quit(1)
		return
	print("DocRender editor source resource references passed.")
	get_tree().quit()
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("godot", type=Path, help="path to the Godot 4.7.2 console executable")
    args = parser.parse_args()
    godot = args.godot.resolve()
    addon = Path(__file__).resolve().parents[2] / "addons/docrender"
    library = addon / "bin/docrender.windows.template_debug.x86_64.dll"
    if not godot.is_file() or not library.is_file():
        parser.error("Use the Godot console executable and build the Windows x64 template_debug target first.")

    with TemporaryDirectory(prefix="docrender-editor-reference-") as folder:
        root = Path(folder)
        project = root / "project"
        profile = root / "profile"
        for directory in (project / "addons/docrender/bin", project / "addons/probe", project / ".godot/editor", profile):
            directory.mkdir(parents=True)
        shutil.copyfile(library, project / "addons/docrender/bin" / library.name)
        shutil.copyfile(addon / "docrender.gdextension", project / "addons/docrender/docrender.gdextension")

        def write(path, contents):
            (project / path).write_text(contents, encoding="utf-8", newline="\n")

        write("project.godot", 'config_version=5\n[application]\nconfig/name="DocRender editor reference test"\n'
              '[editor_plugins]\nenabled=PackedStringArray("res://addons/probe/plugin.cfg")\n')
        write(".godot/extension_list.cfg", "res://addons/docrender/docrender.gdextension\n")
        write("addons/probe/plugin.cfg", '[plugin]\nname="Reference Probe"\nscript="probe.gd"\n')
        write("addons/probe/probe.gd", PROBE)
        write("source.html", "<p>Shared document</p>\n")
        write("source.tex", "% LaTeX reference\nx + y\n")
        write("source.mmd", "%% Mermaid MMD reference\ngraph TD\n  A --> B\n")
        write("other.mermaid", "%% Mermaid long extension reference\ngraph TD\n  C --> D\n")
        write("document_references.gd", '''extends Control

@export var latex_document: LatexDocument
@export var mermaid_document: MermaidDocument
@export var other_document: MermaidDocument
''')
        env = {**os.environ, "APPDATA": str(profile), "DOCRENDER_REFERENCE_PROBE": "0"}
        command = [str(godot), "--headless", "--path", str(project)]
        imported = subprocess.run([*command, "--import"], env=env, cwd=project, timeout=60, capture_output=True)
        sources = ("source.html", "source.tex", "source.mmd", "other.mermaid")
        if imported.returncode or not all((project / f"{source}.import").is_file() for source in sources):
            print(imported.stdout.decode(errors="replace"), imported.stderr.decode(errors="replace"))
            return imported.returncode or 1

        write("main.tscn", '''[gd_scene format=3]

[ext_resource type="HTMLDocument" path="res://source.html" id="1_external"]
[ext_resource type="Script" path="res://document_references.gd" id="2_script"]
[ext_resource type="LatexDocument" path="res://source.tex" id="3_latex"]
[ext_resource type="MermaidDocument" path="res://source.mmd" id="4_mermaid"]
[ext_resource type="MermaidDocument" path="res://other.mermaid" id="5_other"]

[sub_resource type="HTMLDocument" id="HTMLDocument_embedded"]
text = "Embedded document"

[node name="Root" type="Control"]
script = ExtResource("2_script")
latex_document = ExtResource("3_latex")
mermaid_document = ExtResource("4_mermaid")
other_document = ExtResource("5_other")

[node name="First" type="HTMLLabel" parent="."]
document = ExtResource("1_external")

[node name="Second" type="HTMLLabel" parent="."]
document = ExtResource("1_external")

[node name="Embedded" type="HTMLLabel" parent="."]
document = SubResource("HTMLDocument_embedded")
''')
        write(".godot/editor/editor_layout.cfg", '[EditorNode]\nopen_scenes=PackedStringArray("res://main.tscn")\n'
              'current_scene="res://main.tscn"\n[ScriptEditor]\n'
              'open_scripts=["res://source.html", "res://source.tex", "res://source.mmd", "res://other.mermaid"]\n'
              'selected_script="res://source.html"\n')
        result = subprocess.run([*command, "--editor", "--quit-after", "600"],
                                env={**env, "DOCRENDER_REFERENCE_PROBE": "1"}, cwd=project,
                                timeout=60, capture_output=True)
        passed = b"DocRender editor source resource references passed." in result.stdout
        if result.returncode or not passed:
            print(result.stdout.decode(errors="replace"), result.stderr.decode(errors="replace"))
            return result.returncode or 1
        print("DocRender editor source resource references passed.")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
