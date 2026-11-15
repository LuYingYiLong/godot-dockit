"""Run the HTML cascade checks with the Windows x64 debug extension."""

import argparse
from pathlib import Path
import os
import shutil
import struct
import subprocess
from tempfile import TemporaryDirectory
import zlib


def write_rgba_png(path, width, height, rows, metadata=(), late_metadata=()):
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))

    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
        + b"".join(chunk(kind, payload) for kind, payload in metadata)
        + chunk(b"IDAT", zlib.compress(rows))
        + b"".join(chunk(kind, payload) for kind, payload in late_metadata)
        + chunk(b"IEND", b"")
    )


def write_density_png(path, horizontal, vertical):
    width, height = 48, 24
    rows = (b"\0" + b"\xff\0\0\xff" * width) * height
    write_rgba_png(path, width, height, rows, [(b"pHYs", struct.pack(">IIB", horizontal, vertical, 1))])


def write_orientation_png(path, orientation, late=False):
    width, height = 32, 20
    top = b"\0" + b"\xff\0\0\xff" * 16 + b"\0\xff\0\xff" * 16
    bottom = b"\0" + b"\0\0\xff\xff" * 16 + b"\xff\xff\0\xff" * 16
    rows = top * 10 + bottom * 10
    endian, marker = ("<", b"II") if orientation % 2 else (">", b"MM")
    # TIFF stores the SHORT inline after the entry's 32-bit count.
    exif = marker + struct.pack(endian + "HIHHHIH2xI", 42, 8, 1, 0x0112, 3, 1, orientation, 0)
    metadata = [(b"pHYs", struct.pack(">IIB", 7559, 7559, 1))]
    exif_chunk = (b"eXIf", exif)
    if not late:
        metadata.append(exif_chunk)
    write_rgba_png(path, width, height, rows, metadata, [exif_chunk] if late else [])


def write_border_image_png(path):
    palette = {
        "r": b"\xff\0\0\xff", "b": b"\0\0\xff\xff", "y": b"\xff\xff\0\xff",
        "c": b"\0\xff\xff\xff", "m": b"\xff\0\xff\xff", "w": b"\xff\xff\xff\xff",
        "g": b"\0\xff\0\xff", "o": b"\xee\x82\0\xff", "k": b"\0\0\0\xff",
    }
    rows = (
        ["rrrrmmwwbbbb"] * 4
        + ["kkkkoooocccc"] * 2
        + ["wwwwoooocccc"] * 2
        + ["yyyyggggcccc"] * 4
    )
    write_rgba_png(path, 12, 12, b"".join(b"\0" + b"".join(palette[pixel] for pixel in row) for row in rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("godot", type=Path, help="path to the Godot console executable")
    parser.add_argument("--renderer", choices=("gl_compatibility", "forward_plus"))
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    library = repo / "addons/docrender/bin/docrender.windows.template_debug.x86_64.dll"
    if not library.is_file():
        parser.error("Build the Windows x64 template_debug target first.")
    if not args.godot.is_file():
        parser.error("Godot executable does not exist.")

    with TemporaryDirectory(prefix="docrender-css-cascade-") as test_dir:
        project = Path(test_dir)
        extension = project / "addons/docrender"
        (extension / "bin").mkdir(parents=True)
        shutil.copyfile(library, extension / "bin" / library.name)
        shutil.copyfile(repo / "addons/docrender/docrender.gdextension", extension / "docrender.gdextension")
        (project / ".godot").mkdir()
        (project / ".godot/extension_list.cfg").write_text(
            "res://addons/docrender/docrender.gdextension\n", encoding="utf-8", newline="\n",
        )
        (project / "project.godot").write_text(
            'config_version=5\n\n[application]\nconfig/name="DocRender CSS cascade test"\n'
            '\n[debug]\nfile_logging/enable_file_logging=false\n'
            '\n[rendering]\nshader_compiler/shader_cache/enabled=false\n',
            encoding="utf-8", newline="\n",
        )
        shutil.copyfile(Path(__file__).with_name("css_cascade.gd"), project / "css_cascade.gd")
        styles = project / "styles"
        (styles / "nested").mkdir(parents=True)
        (styles / "sheet.css").write_text(
            '@import "nested/outer.css";\n@import "between.css";\n@import "nested/inner.css";\n'
            '.from_link { width: 30px; }\n'
            '.import_order { width: 23px; }\n.after_link { width: 13px; }\n'
            '.stylesheet_move { width: 31px; }\n',
            encoding="utf-8", newline="\n",
        )
        (styles / "nested/outer.css").write_text('@import "inner.css";\n', encoding="utf-8", newline="\n")
        (styles / "nested/inner.css").write_text(
            ".order { width: 22px; } .import_order { width: 22px; }\n"
            ".reimport_order { width: 24px; } .unused { width: bogus; }\n"
            "#invalid_nested { width: 58px; width: var(--nested-width); }\n",
            encoding="utf-8", newline="\n",
        )
        (styles / "between.css").write_text(".reimport_order { width: 25px; }\n", encoding="utf-8", newline="\n")
        (styles / "additional.css").write_text(".order { width: 31px; }\n", encoding="utf-8", newline="\n")
        (styles / "media.css").write_text(".link_media { width: 29px; }\n", encoding="utf-8", newline="\n")
        (styles / "typed.css").write_text(
            "#link_type { width: bogus; width: 68px !important; }\n", encoding="utf-8", newline="\n",
        )
        (styles / "conditional.css").write_text("#conditional_import { width: 51px; }\n", encoding="utf-8", newline="\n")
        (styles / "rejected.css").write_text(
            "#conditional_import { width: bogus; width: 99px !important; }\n", encoding="utf-8", newline="\n",
        )
        (styles / "layer.css").write_text(
            '@import "nested/layer-inner.css" layer(components);\n',
            encoding="utf-8", newline="\n",
        )
        (styles / "anonymous-layer.css").write_text(
            "#layer_import { width: 71px; }\n", encoding="utf-8", newline="\n",
        )
        (styles / "nested/layer-inner.css").write_text(
            "#nested_layer_import { width: 74px; } .unused { width: bogus; }\n",
            encoding="utf-8", newline="\n",
        )
        (styles / "layer-media.css").write_text(
            "#import_media { width: 98px; }\n", encoding="utf-8", newline="\n",
        )
        (styles / "counters.css").write_text(
            '@counter-style imported-counter { system: cyclic; symbols: IMPORT; }\n'
            '@counter-style layered { system: cyclic; symbols: imported; }\n',
            encoding="utf-8", newline="\n",
        )
        (styles / "font-alternates.css").write_text(
            '@font-feature-values Probe { @styleset { imported: 4; } }\n', encoding="utf-8", newline="\n",
        )
        (project / "object-fit.svg").write_text(
            '<svg xmlns="http://www.w3.org/2000/svg" width="40" height="20">'
            '<rect width="40" height="20" fill="#c5692b"/></svg>\n', encoding="utf-8", newline="\n",
        )
        (project / "sampling.svg").write_text(
            '<svg xmlns="http://www.w3.org/2000/svg" width="3" height="2">'
            '<path fill="red" d="M0 0h1v1H0z"/><path fill="lime" d="M1 0h1v1H1z"/>'
            '<path fill="blue" d="M2 0h1v1H2z"/><path fill="yellow" d="M0 1h1v1H0z"/>'
            '<path fill="cyan" d="M1 1h1v1H1z"/><path fill="magenta" d="M2 1h1v1H2z"/></svg>\n',
            encoding="utf-8", newline="\n",
        )
        (project / "image_set.html").write_text(
            '<!doctype html><link rel="stylesheet" href="styles/image-set.css"><div id="imported_set"></div>\n',
            encoding="utf-8", newline="\n",
        )
        (project / "image_function.html").write_text(
            '<!doctype html><link rel="stylesheet" href="styles/image-function.css"><div id="imported_image"></div>\n',
            encoding="utf-8", newline="\n",
        )
        write_density_png(project / "density-live.png", 7559, 7559)
        write_density_png(project / "density-import.png", 7559, 7559)
        write_density_png(project / "density-asymmetric.png", 7559, 3779)
        for orientation in range(1, 9):
            write_orientation_png(project / f"orientation-{orientation}.png", orientation)
        write_orientation_png(project / "orientation-late.png", 6, late=True)
        write_orientation_png(project / "orientation-import.png", 6)
        write_border_image_png(project / "border-image.png")
        shutil.copyfile(project / "border-image.png", project / "border-import.png")
        (styles / "border-image.css").write_text(
            'body { margin: 0; } #imported_border { width: 20px; height: 20px; border: 4px solid black;'
            'border-image: url(../border-import.png) 4 fill; }',
            encoding="utf-8", newline="\n",
        )
        (project / "border_image.html").write_text(
            '<!doctype html><link rel="stylesheet" href="styles/border-image.css"><div id="imported_border"></div>',
            encoding="utf-8", newline="\n",
        )
        (project / "image_orientation.html").write_text(
            '<!doctype html><style>body { margin: 0; }</style>'
            '<img id="imported_orientation" src="orientation-import.png" style="display: block">\n',
            encoding="utf-8", newline="\n",
        )
        (project / "image_resolution.html").write_text(
            '<!doctype html><img id="imported_density" src="density-import.png" style="image-resolution: from-image">\n',
            encoding="utf-8", newline="\n",
        )
        (styles / "image-function.css").write_text(
            'body { margin: 0; } #imported_image { width: 8px; height: 8px; '
            'background: image(ltr "../sampling.svg#xywh=1,0,1,1", red) no-repeat; }\n',
            encoding="utf-8", newline="\n",
        )
        (styles / "image-set.css").write_text(
            '@import "nested/candidates.css";\n', encoding="utf-8", newline="\n",
        )
        (styles / "nested/candidates.css").write_text(
            'body { margin: 0; } #imported_set { width: 28px; height: 20px; background: '
            'image-set("../set-low.svg", var(--missing, "set-high.svg") 2dppx) no-repeat; }\n',
            encoding="utf-8", newline="\n",
        )
        for path, width, height, color in (
            (styles / "set-low.svg", 12, 8, "#67bb39"),
            (styles / "nested/set-high.svg", 24, 16, "#d84333"),
        ):
            path.write_text(
                f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">'
                f'<rect width="{width}" height="{height}" fill="{color}"/></svg>\n',
                encoding="utf-8", newline="\n",
            )
        system_fonts = Path(os.environ["WINDIR"]) / "Fonts"
        (project / "fonts").mkdir()
        for filename in ("times.ttf", "cour.ttf", "arialbd.ttf", "georgiai.ttf", "bahnschrift.ttf", "cambria.ttc"):
            shutil.copyfile(system_fonts / filename, project / "fonts" / filename)
        optional_fonts = (
            ("OPTICAL_FONT_UID", "SegUIVar.ttf", "optical.ttf"),
            ("EAST_ASIAN_FONT_UID", "YuGothM.ttc", "east_asian.ttc"),
            ("VARIANT_FONT_UID", "calibri.ttf", "variant.ttf"),
            ("HISTORICAL_FONT_UID", "pala.ttf", "historical.ttf"),
        )
        font_fixtures = {}
        for key, filename, fixture_name in optional_fonts:
            source = system_fonts / filename
            if source.is_file():
                fixture = project / "fonts" / fixture_name
                shutil.copyfile(source, fixture)
                font_fixtures[key] = fixture.with_name(fixture.name + ".import")
        (styles / "font-faces.css").write_text(
            '@import "nested/font-regular.css";\n'
            '@font-face { font-family: "Probe Face"; src: url(../fonts/arialbd.ttf); font-weight: bold; }\n'
            '@font-face { font-family: "Probe Face"; src: url(../fonts/georgiai.ttf); font-style: italic; }\n'
            '@font-face { font-family: "Collection Probe";'
            'src: url("../fonts/cambria.ttc#Cambria%4Dath") format(collection); }\n',
            encoding="utf-8", newline="\n",
        )
        (styles / "nested/font-regular.css").write_text(
            '@font-face { font-family: "Probe Face"; src: url(../../fonts/times.ttf); }\n'
            '@media (max-width: 400px) { @supports font-format( Truetype ) and at-rule(@font-face) {\n'
            '@font-face { font-family: "Probe Face";\n'
            'src: local("Unavailable face"), url(missing.ttf), url(../../fonts/times.ttf) format(svg),\n'
            'url(../../fonts/times.ttf) tech(variations),\n'
            'url("../../fonts/cour.ttf") format("truetype"); } } }\n',
            encoding="utf-8", newline="\n",
        )
        (project / "font_face.html").write_text(
            '<!doctype html><link rel="stylesheet" href="styles/font-faces.css">\n'
            '<style>@supports font-tech(variations) { @font-face { font-family: "Variable Probe";'
            'src: url(fonts/bahnschrift.ttf) format("truetype-variations"); font-weight: 150 650; } }\n'
            '@supports font-format("truetype") or font-tech(incremental) or at-rule(@keyframes) {'
            '.variable { color: black; } }\n'
            'body { margin: 0; background: black; color: white; }'
            'div { font: 40px "probe FACE", serif; width: 7ch; height: 40px; }'
            '#bold { font-weight: bold; } #italic { font-style: italic; }'
            '.variable { font: 40px "Variable Probe", serif; width: 220px; height: 55px; line-height: 55px; }'
            '#light { font-weight: 150; } #heavy { font-weight: 650; } #above { font-weight: 900; }</style>\n'
            '<div id="normal">Normal</div><div id="bold">Bold</div><div id="italic">Italic</div>\n'
            '<div id="light" class="variable">MMMM</div><div id="heavy" class="variable">MMMM</div>'
            '<div id="above" class="variable">MMMM</div>\n',
            encoding="utf-8", newline="\n",
        )
        # Keep the test process's logs and shader cache in its temporary project.
        environment = {**os.environ, "APPDATA": str(project)}
        imported = subprocess.run(
            [str(args.godot.resolve()), "--headless", "--path", str(project), "--import"],
            env=environment, cwd=project, timeout=60, check=False, capture_output=True,
        )
        document_fixtures = {
            "FONT_FACE_UID": project / "font_face.html.import",
            "IMAGE_SET_UID": project / "image_set.html.import",
            "IMAGE_FUNCTION_UID": project / "image_function.html.import",
            "IMAGE_RESOLUTION_UID": project / "image_resolution.html.import",
            "IMAGE_ORIENTATION_UID": project / "image_orientation.html.import",
            "BORDER_IMAGE_UID": project / "border_image.html.import",
        }
        if imported.returncode != 0 or not all(path.is_file() for path in document_fixtures.values()):
            print(imported.stdout.decode(errors="replace"))
            print(imported.stderr.decode(errors="replace"))
            return imported.returncode or 1
        # A selected-resource export can omit the original stylesheets.
        for stylesheet in (
            styles / "font-faces.css", styles / "nested/font-regular.css",
            styles / "image-set.css", styles / "nested/candidates.css",
            styles / "image-function.css", styles / "border-image.css",
        ):
            stylesheet.unlink()
        (project / "density-import.png").unlink()
        (project / "orientation-import.png").unlink()
        (project / "border-import.png").unlink()
        test = project / "css_cascade.gd"
        script = test.read_text(encoding="utf-8")
        for key, path in document_fixtures.items():
            uid = path.read_text(encoding="utf-8").split('uid="', 1)[1].split('"', 1)[0]
            script = script.replace("{" + key + "}", uid)
        for key, _, _ in optional_fonts:
            fixture_uid = ""
            if key in font_fixtures:
                font_import = font_fixtures[key].read_text(encoding="utf-8")
                fixture_uid = font_import.split('uid="', 1)[1].split('"', 1)[0]
            script = script.replace("{" + key + "}", fixture_uid)
        test.write_text(script, encoding="utf-8", newline="\n")
        renderers = (args.renderer,) if args.renderer else ("gl_compatibility", "forward_plus")
        for renderer in renderers:
            completed = subprocess.run(
                [str(args.godot.resolve()), "--path", str(project), "--rendering-method", renderer,
                 "--script", "css_cascade.gd"],
                env=environment, cwd=project, timeout=60, check=False,
            )
            if completed.returncode != 0:
                return completed.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
