"""Check that Markdown code blocks retain source indentation while rendering."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory


def write(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8", newline="\n")


def run(command: list[str], workspace: Path) -> None:
    print(f"Running: {command}", flush=True)
    result = subprocess.run(
        command,
        env={**os.environ, "APPDATA": str(workspace)},
        cwd=workspace,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=120,
    )
    print(result.stdout, end="")
    print(result.stderr, end="")
    has_engine_errors = "ERROR:" in result.stdout or "ERROR:" in result.stderr
    if result.returncode or has_engine_errors:
        raise SystemExit(f"Test command failed with exit {result.returncode} and engine_errors={has_engine_errors}: {command}")


def create_project(project: Path, repo: Path) -> None:
    addon = project / "addons/dockit"
    (addon / "bin").mkdir(parents=True)
    for asset in ("dockit.gdextension", "bin/dockit.windows.template_debug.x86_64.dll"):
        shutil.copyfile(repo / "addons/dockit" / asset, addon / asset)
    shutil.copytree(repo / "addons/dockit/icons", addon / "icons")
    (project / ".godot").mkdir()
    write(project / ".godot/extension_list.cfg", "res://addons/dockit/dockit.gdextension\n")
    write(project / "project.godot", """config_version=5

[application]
config/name="Dockit Markdown code blocks"
run/main_scene="res://main.tscn"

[rendering]
renderer/rendering_method="gl_compatibility"
shader_compiler/shader_cache/enabled=false
""")
    shutil.copyfile(Path(__file__).with_name("code_blocks.gd"), project / "code_blocks.gd")
    write(project / "main.tscn", """[gd_scene load_steps=2 format=3]
[ext_resource type="Script" path="res://code_blocks.gd" id="1"]
[node name="MarkdownCodeBlocks" type="Node"]
script = ExtResource("1")
""")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("godot", type=Path)
    parser.add_argument("--work-dir", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    godot = str(args.godot.resolve())
    with TemporaryDirectory(prefix="dockit-markdown-code-blocks-", dir=args.work_dir, ignore_cleanup_errors=True) as directory:
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
        run([godot, "--headless", "--editor", "--path", str(project), "--quit-after", "180"], workspace)
        run([godot, "--headless", "--path", str(project), "--quit-after", "120"], workspace)


if __name__ == "__main__":
    main()
