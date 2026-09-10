#!/usr/bin/env python3

import json
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST = "0.0.0.0"
PORT = 8765


def get_cpu_temp():
    result = subprocess.run(
        ["sensors", "-j"],
        capture_output=True,
        text=True,
        check=True,
    )

    data = json.loads(result.stdout)
    return data["coretemp-isa-0000"]["Package id 0"]["temp1_input"]


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/temp":
            self.send_response(404)
            self.end_headers()
            return

        try:
            body = json.dumps({"cpu_temp": get_cpu_temp()})
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body.encode())
        except Exception as exc:
            body = json.dumps({"error": str(exc)})
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body.encode())

    def log_message(self, format, *args):
        pass


server = HTTPServer((HOST, PORT), Handler)
print(f"Homelab temperature API listening on port {PORT}")
server.serve_forever()
