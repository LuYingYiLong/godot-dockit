"""Stage and verify the addon files used by the full build workflow."""

from argparse import ArgumentParser
from configparser import ConfigParser
from json import loads
from pathlib import Path
from shutil import copy2, copytree, ignore_patterns


ROOT = Path(__file__).resolve().parents[1]
ADDON = ROOT / "addons/dockit"
PACKAGE_EXCLUDES = (
    "bin",
    "*.import",
    ".godot",
    "README*",
    "*LICENSE*",
    "*LICENCE*",
    "*COPYING*",
    "*NOTICE*",
    "*SOURCE_AVAILABILITY*",
)


def read_libraries():
    descriptor = (ADDON / "dockit.gdextension").read_text(encoding="utf-8")
    section = descriptor.split("[libraries]", 1)[1].split("[", 1)[0]
    config = ConfigParser(interpolation=None)
    config.read_string("[libraries]" + section)
    return {key: Path(loads(value)) for key, value in config["libraries"].items()}


def library_file(path):
    if path.suffix == ".framework":
        return path / path.stem
    return path


def require_file(path):
    if not path.is_file() or path.stat().st_size == 0:
        raise SystemExit(f"Missing or empty package file: {path}")


def copy_file(source, destination):
    require_file(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    copy2(source, destination)


def stage_build(destination, platform, arch, target):
    platform_key = f"{platform}.{target.removeprefix('template_')}"
    key = platform_key if platform in ("macos", "ios") else f"{platform_key}.{arch}"
    library = read_libraries().get(key)
    if library is None:
        raise SystemExit(f"No GDExtension library declared for {platform} {arch} {target}")
    binary = library_file(library)
    copy_file(ADDON / binary, destination / binary)

    # The PDFium runtime is shared by the Windows x64 debug and release builds.
    if (platform, arch, target) == ("windows", "x86_64", "template_release"):
        copy_file(ADDON / "bin/pdfium.dll", destination / "bin/pdfium.dll")


def assemble(destination):
    libraries = read_libraries()
    for library in libraries.values():
        require_file(destination / library_file(library))

    copytree(
        ADDON,
        destination,
        dirs_exist_ok=True,
        ignore=ignore_patterns(*PACKAGE_EXCLUDES),
    )

    require_file(destination / "bin/pdfium.dll")
    package_notices = [
        path
        for path in destination.rglob("*")
        if any(
            marker in path.name.upper()
            for marker in ("LICENSE", "LICENCE", "COPYING", "NOTICE", "README", "SOURCE_AVAILABILITY")
        )
    ]
    if package_notices:
        raise SystemExit(f"Unexpected documentation or license files in package: {package_notices}")
    print(f"Verified all {len(libraries)} GDExtension libraries in {destination}")


def main():
    parser = ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    stage = commands.add_parser("stage")
    stage.add_argument("destination", type=Path)
    stage.add_argument("platform")
    stage.add_argument("arch")
    stage.add_argument("target", choices=("template_debug", "template_release"))
    merge = commands.add_parser("assemble")
    merge.add_argument("destination", type=Path)
    args = parser.parse_args()

    if args.command == "stage":
        stage_build(args.destination, args.platform, args.arch, args.target)
    else:
        assemble(args.destination)


if __name__ == "__main__":
    main()
