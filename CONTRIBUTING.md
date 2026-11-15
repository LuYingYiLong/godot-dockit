# Contributing to Godot DocRender

## Set up the repository

Install Git, Python, and a C++ toolchain for your target. For the Windows builds below, install MSVC and the Windows SDK through Visual Studio or Visual Studio Build Tools.

```powershell
git clone https://github.com/LuYingYiLong/godot-docrender.git
cd godot-docrender
git submodule update --init --recursive
python -m pip install scons
```

The submodules provide godot-cpp, MicroTeX and its fonts, TinyXML-2, and litehtml v0.10 with bundled Gumbo. SCons compiles litehtml and Gumbo into the extension on each target. Run a build before opening the solution in Visual Studio so IntelliSense can find the generated godot-cpp bindings and staged litehtml headers. [Visual Studio solution](Godot-DocRender.sln)

DocRender maintains its litehtml replacements as C++ sources and headers in `src/formats/html/litehtml/`. Each group mirrors paths relative to the pinned litehtml submodule. Edit these files directly. SCons compiles the replacement sources in place, compiles the remaining sources from the submodule, and stages the combined headers in `src/gen/html/include/`. When updating the litehtml submodule, reconcile each replacement with its matching upstream file. The replacements retain the [litehtml BSD 3-Clause license](addons/docrender/LiteHTML-LICENSES/litehtml.txt).

Edit CSS tokenization, grammar, stylesheet rules, matching, the cascade, value types, default stylesheet, and computed styles in `src/formats/html/css/`. The litehtml adapter supplies a read-only DOM view, cascaded declarations, parent styles, and document-specific font and image queries. Layout reads the resulting `ComputedStyle`, resolves box-dependent values, and positions Grid items. The renderer turns resolved background layers into draw commands. CSS compatibility headers under `src/formats/html/litehtml/` forward to the canonical definitions. Material migrated from litehtml retains its BSD 3-Clause license.

## Install optional SDKs

On Windows x64, install Merman for Mermaid rendering with MSVC and PDFium for PDF import, reading, and writing. The installers fetch the pinned archives and verify their checksums.

```powershell
python scripts/fetch_merman_sdk.py
python scripts/fetch_pdfium_sdk.py
```

For offline PDFium setup, pass a local copy of the pinned `pdfium-win-x64.tgz` archive. [PDFium SDK release](https://github.com/bblanchon/pdfium-binaries/releases/tag/chromium/8152)

```powershell
python scripts/fetch_pdfium_sdk.py --archive "C:/Downloads/pdfium-win-x64.tgz"
```

## Build and test

Run SCons from the repository root. Build `template_debug` for the editor and debug exports, and `template_release` for release exports.

```powershell
scons godot-docrender platform=windows target=template_debug arch=x86_64
scons godot-docrender platform=windows target=template_release arch=x86_64
```

SCons writes the libraries to `addons/docrender/bin/` and copies `pdfium.dll` there when the Windows x64 SDK is installed. Copy the full `addons/docrender` directory into a Godot project and reopen it to load the new binary. Use the CI build matrix to find `platform` and `arch` values for other targets. [Build workflow](.github/workflows/make_build.yml)

When changing editor text or class documentation, update and check the translation catalogs. [Localization tools](tools/README.md)

After building Windows x64 `template_debug`, run the CSS cascade regression checks with Godot 4.7.2. The runner tests both Compatibility and Forward+ in a temporary project. Use `--renderer gl_compatibility` or `--renderer forward_plus` to select one.

```powershell
python tests/html/run_css_cascade.py "C:/path/to/Godot_v4.7.2-stable_win64_console.exe"
```
