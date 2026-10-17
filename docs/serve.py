#!/usr/bin/env python3
"""Локальный кабинет: отдаёт dashboard.html и data/latest.json через gh auth token."""
import base64
import json
import subprocess
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent
GH = Path.home() / "AppData/Local/Microsoft/WinGet/Packages/GitHub.cli_Microsoft.Winget.Source_8wekyb3d8bbwe/bin/gh.exe"
REPO = "muharlyamovi/AquaSense"


def gh_token() -> str:
    exe = str(GH) if GH.exists() else "gh"
    return subprocess.check_output([exe, "auth", "token"], text=True).strip()


def fetch_latest() -> bytes:
    req = Request(
        f"https://api.github.com/repos/{REPO}/contents/data/latest.json",
        headers={
            "Authorization": f"Bearer {gh_token()}",
            "Accept": "application/vnd.github+json",
            "User-Agent": "AquaSense-dashboard",
        },
    )
    with urlopen(req, timeout=20) as r:
        meta = json.loads(r.read().decode())
    raw = base64.b64decode(meta["content"])
    return raw


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=str(ROOT), **k)

    def do_GET(self):
        if self.path.split("?", 1)[0] in ("/latest.json", "/api/latest"):
            try:
                body = fetch_latest()
                self.send_response(200)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Cache-Control", "no-store")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
            except HTTPError as e:
                msg = f'{{"err":"github {e.code}"}}'.encode()
                self.send_response(e.code if e.code != 404 else 404)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(msg)))
                self.end_headers()
                self.wfile.write(msg)
            except (URLError, subprocess.CalledProcessError, KeyError) as e:
                msg = json.dumps({"err": str(e)}).encode()
                self.send_response(502)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(msg)))
                self.end_headers()
                self.wfile.write(msg)
            return
        if self.path in ("/", "/index.html"):
            self.path = "/dashboard.html"
        return super().do_GET()

    def log_message(self, fmt, *args):
        print(self.address_string(), "-", fmt % args)


if __name__ == "__main__":
    port = 8765
    print(f"http://127.0.0.1:{port}/dashboard.html")
    ThreadingHTTPServer(("0.0.0.0", port), Handler).serve_forever()
