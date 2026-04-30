#!/usr/bin/env python3
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from datetime import datetime


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length).decode("utf-8", errors="replace")
        now = datetime.utcnow().isoformat() + "Z"

        print(f"[{now}] path={self.path} len={length}")
        try:
            parsed = json.loads(body)
            if isinstance(parsed, list) and parsed:
                first = parsed[0]
                labels = first.get("labels", {}) if isinstance(first, dict) else {}
                print(
                    "  alert="
                    f"{labels.get('alertname', 'unknown')} "
                    f"severity={labels.get('severity', 'unknown')}"
                )
            else:
                print("  payload=list-empty-or-non-list")
        except Exception:
            print("  payload=non-json")

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"ok":true}')

    def log_message(self, format, *args):
        return


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18080
    server = HTTPServer(("0.0.0.0", port), Handler)
    print(f"mock alert webhook listening on :{port}")
    server.serve_forever()
