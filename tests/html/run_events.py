"""Test DOM event dispatch, cancelable default actions, and document-level delegation."""

import argparse
from pathlib import Path
import shutil
from tempfile import TemporaryDirectory

from run_audio import run, write


def create_project(project, repo):
    addon = project / "addons/docrender"
    (addon / "bin").mkdir(parents=True)
    for asset in ("docrender.gdextension", "bin/docrender.windows.template_debug.x86_64.dll"):
        shutil.copyfile(repo / "addons/docrender" / asset, addon / asset)
    (project / ".godot").mkdir()
    write(project / ".godot/extension_list.cfg", "res://addons/docrender/docrender.gdextension\n")
    write(project / "project.godot", """config_version=5
[application]
run/main_scene="res://main.tscn"
[display]
window/size/viewport_width=520
window/size/viewport_height=300
[rendering]
renderer/rendering_method="gl_compatibility"
""")
    shutil.copyfile(Path(__file__).with_name("events.gd"), project / "events.gd")
    write(project / "main.tscn", """[gd_scene load_steps=2 format=3]
[ext_resource type="Script" path="res://events.gd" id="1"]
[node name="Events" type="Node"]
script = ExtResource("1")
[node name="Label" type="HTMLLabel" parent="."]
offset_right = 420.0
offset_bottom = 240.0
""")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("godot", type=Path)
    parser.add_argument("--work-dir", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    godot = str(args.godot.resolve())
    with TemporaryDirectory(prefix="docrender-html-events-", dir=args.work_dir, ignore_cleanup_errors=True) as directory:
        workspace = Path(directory)
        project = workspace / "project"
        create_project(project, repo)
        run([godot, "--headless", "--path", str(project), "--import"], workspace)
        run([godot, "--headless", "--path", str(project), "--quit-after", "360"],
            workspace, "HTML event checks passed")


if __name__ == "__main__":
    main()
