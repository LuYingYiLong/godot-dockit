"""Exercise shared remote images against a local server with controlled completion order."""

import argparse
from collections import Counter
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import shutil
import ssl
import struct
import subprocess
from tempfile import TemporaryDirectory
from threading import Thread
import time
import zlib

from run_audio import run, write


def png(width, height, rgb, oversized=False):
    def chunk(tag, data):
        return struct.pack("!I", len(data)) + tag + data + struct.pack("!I", zlib.crc32(tag + data))

    pixels = (b"\0" + bytes(rgb) * width) * height if not oversized else b""
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack("!IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(pixels)) + chunk(b"IEND", b""))


def create_project(project, repo):
    addon = project / "addons/docrender"
    (addon / "bin").mkdir(parents=True)
    for asset in ("docrender.gdextension", "bin/docrender.windows.template_debug.x86_64.dll"):
        shutil.copyfile(repo / "addons/docrender" / asset, addon / asset)
    shutil.copytree(repo / "addons/docrender/icons", addon / "icons")
    (project / ".godot").mkdir()
    write(project / ".godot/extension_list.cfg", "res://addons/docrender/docrender.gdextension\n")
    write(project / "project.godot", """config_version=5
[application]
run/main_scene="res://main.tscn"
[display]
window/size/viewport_width=280
window/size/viewport_height=200
[rendering]
renderer/rendering_method="gl_compatibility"
""")
    write(project / "main.tscn", """[gd_scene load_steps=2 format=3]
[ext_resource type="Script" path="res://remote_images.gd" id="1"]
[node name="RemoteImages" type="Node"]
script = ExtResource("1")
""")
    shutil.copyfile(Path(__file__).with_name("remote_images.gd"), project / "remote_images.gd")
    settings = project.parent / "Godot"
    settings.mkdir()
    write(settings / "editor_settings-4.7.tres", """[gd_resource type="EditorSettings" format=3]
[resource]
filesystem/on_save/safe_save_on_backup_then_rename = false
""")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("godot", type=Path)
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--https", action="store_true", help="serve HTTPS with an OpenSSL-generated test certificate")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    counts = Counter()
    responses = {
        "/background": (0.22, png(7, 5, (255, 255, 0))),
        "/shared?asset=1": (0.85, png(26, 18, (255, 0, 0))),
        "/corrupt": (0.30, b"This is not an image."),
        "/oversized": (0.30, png(9000, 9000, (0, 0, 0), oversized=True)),
        "/embedded": (1.30, png(26, 72, (255, 0, 0))),
        "/slow": (0.80, png(40, 18, (0, 255, 0))),
        "/fast": (0.05, png(120, 14, (0, 0, 255))),
        "/picture": (0.05, png(14, 12, (255, 0, 255))),
        "/local-choice": (0.05, png(10, 12, (0, 255, 255))),
    }

    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            counts[self.path] += 1
            if self.path not in responses:
                self.send_error(404)
                return
            delay, body = responses[self.path]
            time.sleep(delay)
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, _format, *_args):
            pass

    with TemporaryDirectory(prefix="docrender-remote-images-", dir=args.work_dir, ignore_cleanup_errors=True) as directory:
        workspace = Path(directory)
        create_project(workspace / "project", repo)
        with ThreadingHTTPServer(("127.0.0.1", 0), Handler) as server:
            if args.https:
                certificate = workspace / "project/localhost.crt"
                key = workspace / "localhost.key"
                openssl_config = workspace / "openssl.cnf"
                write(openssl_config, "[req]\ndistinguished_name=dn\n[dn]\n")
                subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
                                "-config", str(openssl_config),
                                "-subj", "/CN=localhost", "-addext", "subjectAltName=IP:127.0.0.1",
                                "-keyout", str(key), "-out", str(certificate)], check=True,
                               stdout=subprocess.DEVNULL)
                context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
                context.load_cert_chain(certificate, key)
                server.socket = context.wrap_socket(server.socket, server_side=True)
                config = workspace / "project/project.godot"
                with config.open("a", encoding="utf-8", newline="\n") as file:
                    file.write('\n[network]\ntls/certificate_bundle_override="res://localhost.crt"\n')
            worker = Thread(target=server.serve_forever, daemon=True)
            worker.start()
            scheme = "https" if args.https else "http"
            url = f"{scheme}://127.0.0.1:{server.server_port}"
            godot = str(args.godot.resolve())
            project = str(workspace / "project")
            try:
                run([godot, "--headless", "--path", project, "--import"], workspace)
                for renderer in ("gl_compatibility", "forward_plus"):
                    counts.clear()
                    try:
                        run([godot, "--path", project, "--rendering-method", renderer, "--quit-after", "1200", "--", url],
                            workspace, "HTML remote image checks passed")
                    except SystemExit:
                        print("Observed requests:", dict(counts))
                        raise
                    expected = Counter({key: 1 for key in (*responses, "/missing")})
                    if counts != expected:
                        raise SystemExit(f"Unexpected request counts using {renderer}: {counts}")
            finally:
                server.shutdown()
                worker.join()


if __name__ == "__main__":
    main()
