"""Check that editor tab restoration preserves a scene's imported HTML reference."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory


PROBE = '''@tool
extends EditorPlugin


func _enter_tree() -> void:
	if OS.get_environment("DOCRENDER_REFERENCE_PROBE") == "1":
		_probe.call_deferred()


func _probe() -> void:
	var scene: Node
	var text_tab: ScriptEditorBase
	for frame: int in range(240):
		await get_tree().process_frame
		scene = EditorInterface.get_edited_scene_root()
		for editor: ScriptEditorBase in EditorInterface.get_script_editor().get_open_script_editors():
			var source: CodeEdit = editor.get_base_editor() as CodeEdit
			if editor.is_class("TextEditor") and source != null and source.text.contains("Shared document"):
				text_tab = editor
		if scene != null and scene.has_node("Second") and text_tab != null:
			break
	if scene == null or not scene.has_node("Second") or text_tab == null:
		push_error("Editor did not restore the fixture scene and source tab")
		get_tree().quit(1)
		return
	for frame: int in range(4):
		await get_tree().process_frame
	var first: HTMLLabel = scene.get_node("First") as HTMLLabel
	var second: HTMLLabel = scene.get_node("Second") as HTMLLabel
	var cached: Resource = ResourceLoader.get_cached_ref("res://source.html")
	var packed: PackedScene = PackedScene.new()
	if packed.pack(scene) != OK or ResourceSaver.save(packed, "res://roundtrip.tscn") != OK:
		push_error("Could not save the restored scene")
		get_tree().quit(1)
		return
	var saved: String = FileAccess.get_file_as_string("res://roundtrip.tscn")
	var external: bool = '[ext_resource type="HTMLDocument"' in saved and 'path="res://source.html"' in saved
	var embedded: bool = '[sub_resource type="HTMLDocument"' in saved and 'text = "Embedded document"' in saved
	if first.document.resource_path != "res://source.html" or cached != first.document or first.document != second.document \\
			or not external or not embedded:
		push_error("Editor changed the shared HTML reference or its saved resource classification")
		get_tree().quit(1)
		return
	print("DocRender editor HTML resource reference passed.")
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
        env = {**os.environ, "APPDATA": str(profile), "DOCRENDER_REFERENCE_PROBE": "0"}
        command = [str(godot), "--headless", "--path", str(project)]
        imported = subprocess.run([*command, "--import"], env=env, cwd=project, timeout=60, capture_output=True)
        if imported.returncode or not (project / "source.html.import").is_file():
            print(imported.stdout.decode(errors="replace"), imported.stderr.decode(errors="replace"))
            return imported.returncode or 1

        write("main.tscn", '''[gd_scene format=3]

[ext_resource type="HTMLDocument" path="res://source.html" id="1_external"]

[sub_resource type="HTMLDocument" id="HTMLDocument_embedded"]
text = "Embedded document"

[node name="Root" type="Control"]

[node name="First" type="HTMLLabel" parent="."]
document = ExtResource("1_external")

[node name="Second" type="HTMLLabel" parent="."]
document = ExtResource("1_external")

[node name="Embedded" type="HTMLLabel" parent="."]
document = SubResource("HTMLDocument_embedded")
''')
        write(".godot/editor/editor_layout.cfg", '[EditorNode]\nopen_scenes=PackedStringArray("res://main.tscn")\n'
              'current_scene="res://main.tscn"\n[ScriptEditor]\nopen_scripts=["res://source.html"]\n'
              'selected_script="res://source.html"\n')
        result = subprocess.run([*command, "--editor", "--quit-after", "600"],
                                env={**env, "DOCRENDER_REFERENCE_PROBE": "1"}, cwd=project,
                                timeout=60, capture_output=True)
        passed = b"DocRender editor HTML resource reference passed." in result.stdout
        if result.returncode or not passed:
            print(result.stdout.decode(errors="replace"), result.stderr.decode(errors="replace"))
            return result.returncode or 1
        print("DocRender editor HTML resource reference passed.")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
