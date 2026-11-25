# Contributing to Godot Dockit

## Setup

Install Git, Python, SCons, and a C++ toolchain. On Windows, use MSVC and the Windows SDK.

```powershell
git clone https://github.com/LuYingYiLong/godot-dockit.git
cd godot-dockit
git submodule update --init --recursive
```

Optional Windows x64 SDKs for Mermaid and PDFium:

```powershell
python scripts/fetch_merman_sdk.py
python scripts/fetch_pdfium_sdk.py
```

## Build

Run from the repository root:

```powershell
scons platform=windows target=template_debug
scons platform=windows target=template_release
```

The addon libraries are written to `addons/dockit/bin/`. [Build workflow](.github/workflows/make_build.yml)

GitHub Actions builds the Windows x64 debug extension for pull requests, pushes to `main`, and merge queue runs. Use [Continuous Integration](.github/workflows/ci.yml) for details or **Build Godot Dockit** to run the platform matrix.

## Code changes

HTML CSS code lives in `src/formats/html/css/`; litehtml replacements live in `src/formats/html/litehtml/`. Keep replacements aligned with the pinned submodule and preserve license information in the README.

When changing editor text or class documentation, update the translation catalogs with the [localization tools](tools/README.md).

## Release

Push a version tag such as `v1.2.3`, or run **Build Godot Dockit** from the Actions tab and enter a tag. The workflow builds and packages the supported platforms, then publishes a GitHub Release. [Release workflow](.github/workflows/make_build.yml)
