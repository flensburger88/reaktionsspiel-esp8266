"""ButtonSpiel simulator: run the phone app in a normal browser without any hardware.

    python tools/simulator/simulate.py [--port 8765] [--units 4] [--bot] [--no-browser]

Serves the page from src/webpage.h (always the current version) together with
mock.js, a fake gateway that runs in the browser. Virtual buttons appear in the
bottom right corner; click the lit one. Only the Python standard library is needed.
The CSV export (POST/GET /csv) behaves like on the gateway.
"""
import argparse
import http.server
import re
import socketserver
import webbrowser
from pathlib import Path

HERE = Path(__file__).resolve().parent
WEBPAGE_H = HERE.parent.parent / "src" / "webpage.h"


def load_page() -> bytes:
    src = WEBPAGE_H.read_text(encoding="utf-8")
    m = re.search(r'R"HTML\((.*)\)HTML"', src, re.S)
    if not m:
        raise SystemExit(f"Seite nicht gefunden in {WEBPAGE_H}")
    html = m.group(1)
    # mock.js must replace window.WebSocket before the app script connects
    html = html.replace("<script>", '<script src="mock.js"></script>\n<script>', 1)
    return html.encode("utf-8")


class Handler(http.server.BaseHTTPRequestHandler):
    csv = (b"", "ButtonSpiel.csv")

    def send(self, code, body=b"", ctype="text/plain; charset=utf-8", extra=None):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        for k, v in (extra or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?")[0]
        if path in ("/", "/index.html"):
            self.send(200, load_page(), "text/html; charset=utf-8")
        elif path == "/mock.js":
            self.send(200, (HERE / "mock.js").read_bytes(), "application/javascript; charset=utf-8")
        elif path == "/csv":
            body, name = Handler.csv
            self.send(200, body, "text/csv; charset=utf-8",
                      {"Content-Disposition": f'attachment; filename="{name}"'})
        else:
            self.send(404, b"not found")

    def do_POST(self):
        if self.path.split("?")[0] != "/csv":
            return self.send(404, b"not found")
        n = int(self.headers.get("Content-Length", 0))
        if n > 256 * 1024:
            return self.send(413, b"too large")
        name = re.sub(r'[^\w.\-]', "_", self.headers.get("X-File", "ButtonSpiel.csv"), flags=re.ASCII)
        Handler.csv = (self.rfile.read(n), name)
        self.send(200, b"ok")

    def log_message(self, fmt, *args):
        pass


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", type=int, default=8765)
    ap.add_argument("--units", type=int, default=4, help="Anzahl simulierter Buttons (1-10)")
    ap.add_argument("--bot", action="store_true", help="Buttons drücken sich selbst")
    ap.add_argument("--no-browser", action="store_true")
    a = ap.parse_args()

    url = f"http://127.0.0.1:{a.port}/?units={a.units}" + ("&bot=1" if a.bot else "")
    with socketserver.ThreadingTCPServer(("127.0.0.1", a.port), Handler) as srv:
        print(f"ButtonSpiel-Simulator läuft: {url}  (Strg+C beendet)")
        if not a.no_browser:
            webbrowser.open(url)
        try:
            srv.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
