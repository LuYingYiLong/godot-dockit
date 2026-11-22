"""Check RST imports, generated references, relative resources, and UID fallbacks."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory



def write(path, text):
    path.write_text(text, encoding="utf-8", newline="\n")


def run(command, workspace, failure_project=None):
    print(f"Running: {command}", flush=True)
    result = subprocess.run(command, env={**os.environ, "APPDATA": str(workspace)}, cwd=workspace,
                            capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=120)
    print(result.stdout, end="")
    print(result.stderr, end="")
    has_engine_errors = "ERROR:" in result.stdout or "ERROR:" in result.stderr
    if result.returncode or has_engine_errors:
        if failure_project is not None:
            if failure_project.exists():
                shutil.rmtree(failure_project)
            shutil.copytree(workspace / "project", failure_project, ignore=shutil.ignore_patterns(".godot"))
            print(f"Preserved failed project at: {failure_project}")
        raise SystemExit(f"Test command failed with exit {result.returncode} and engine_errors={has_engine_errors}: {command}")


def create_project(project, repo):
    addon = project / "addons/dockit"
    (addon / "bin").mkdir(parents=True)
    for asset in ("dockit.gdextension", "bin/dockit.windows.template_debug.x86_64.dll"):
        shutil.copyfile(repo / "addons/dockit" / asset, addon / asset)
    (project / ".godot").mkdir()
    write(project / ".godot/extension_list.cfg", "res://addons/dockit/dockit.gdextension\n")
    write(project / "project.godot", """config_version=5

[application]
config/name="Dockit RST references"
run/main_scene="res://main.tscn"

[dockit]
rst/document_roots=PackedStringArray("res://docs")

[rendering]
renderer/rendering_method="gl_compatibility"
shader_compiler/shader_cache/enabled=false
""")
    (project / "docs/assets").mkdir(parents=True)
    (project / "docs/downloads").mkdir(parents=True)
    write(project / "docs/index.rst", """Reference index example
=======================

.. _index-section:

Index section
-------------

See :ref:`guide-section` and :doc:`guide#guide-section`.

.. toctree::

   guide

.. image:: assets/picture.svg

:download:`notes <downloads/notes.txt>`
""")
    write(project / "docs/guide.rst", """Guide
=====

.. _guide-section:

Guide section
-------------

Return to :doc:`index`.
This section will be added later: :ref:`new-section`.
""")
    write(project / "docs/assets/picture.svg", """<svg xmlns="http://www.w3.org/2000/svg" width="8" height="8"><rect width="8" height="8" fill="#0f0"/></svg>""")
    write(project / "docs/downloads/notes.txt", "RST relative download fixture\n")
    shutil.copyfile(Path(__file__).with_name("references.gd"), project / "references.gd")
    write(project / "main.tscn", """[gd_scene load_steps=2 format=3]
[ext_resource type="Script" path="res://references.gd" id="1"]
[node name="RSTReferences" type="Node"]
script = ExtResource("1")
[node name="Viewer" type="RestructuredtextLabel" parent="."]
offset_right = 640.0
offset_bottom = 480.0
""")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("godot", type=Path)
    parser.add_argument("--work-dir", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    godot = str(args.godot.resolve())
    with TemporaryDirectory(prefix="dockit-rst-references-", dir=args.work_dir, ignore_cleanup_errors=True) as directory:
        workspace = Path(directory)
        project = workspace / "project"
        project.mkdir()
        settings = workspace / "Godot"
        settings.mkdir()
        write(settings / "editor_settings-4.7.tres", """[gd_resource type="EditorSettings" format=3]
[resource]
filesystem/on_save/safe_save_on_backup_then_rename = false
""")
        create_project(project, repo)
        debug_project = repo / "tests" / "rst" / "debug_project"
        run([godot, "--headless", "--editor", "--path", str(project), "--quit-after", "180"], workspace, debug_project)
        run([godot, "--headless", "--path", str(project), "--quit-after", "900"], workspace, debug_project)

        write(project / "docs/new.rst", """New guide
=========

.. _new-section:

New section
-----------
""")
        index_path = project / "docs/index.rst"
        index_text = index_path.read_text(encoding="utf-8")
        index_text = index_text.replace("See :ref:`guide-section` and :doc:`guide#guide-section`.", "See :ref:`guide-section`, :ref:`new-section`, and :doc:`guide#guide-section`.")
        write(index_path, index_text)
        write(project / "updated_references.gd", """extends SceneTree


func _initialize() -> void:
	var resource: Resource = load("res://docs/guide.rst")
	var document: RSTDocument = resource as RSTDocument
	if document == null:
		push_error("The updated guide did not load as an RSTDocument")
		quit(1)
		return
	var index: RSTReferenceIndex = document.get_reference_index()
	if index == null:
		push_error("The updated guide has no reference index")
		quit(1)
		return
	var uri: String = index.resolve_reference("res://docs/guide.rst", "new-section")
	if not uri.ends_with("#new-section"):
		push_error("The dependent document kept a stale reference index: " + uri)
		quit(1)
		return
	print("RST incremental reference checks passed")
	quit()
""")
        run([godot, "--headless", "--editor", "--path", str(project), "--quit-after", "180"], workspace, debug_project)
        run([godot, "--headless", "--path", str(project), "--script", "res://updated_references.gd"], workspace, debug_project)


if __name__ == "__main__":
    main()
