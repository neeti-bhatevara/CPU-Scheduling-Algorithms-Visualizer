#!/usr/bin/env python3
"""
CPU Scheduling & Visualization - Lightweight Python HTTP Backend Server
Connects the web frontend to the native C11 scheduling engine (build/cpu-scheduler.exe).
Standard library only: zero external dependencies.
"""

import http.server
import json
import mimetypes
import os
import pathlib
import socketserver
import subprocess
import sys
import tempfile

PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent
FRONTEND_DIR = PROJECT_ROOT / "frontend"

def find_c_executable():
    candidates = [
        PROJECT_ROOT / "build" / "cpu-scheduler.exe",
        PROJECT_ROOT / "cpu-scheduler.exe",
        PROJECT_ROOT / "build" / "main.exe",
        PROJECT_ROOT / "main.exe",
    ]
    for c in candidates:
        if c.exists() and os.access(str(c), os.X_OK):
            return c
    # Fallback to the primary build location even if yet to be compiled
    return PROJECT_ROOT / "build" / "cpu-scheduler.exe"

C_EXECUTABLE = find_c_executable()

VALID_ALGORITHMS = {
    "fcfs": "fcfs",
    "sjf": "sjf",
    "srtf": "srtf",
    "rr": "rr",
    "prioritynp": "priority_np",
    "priority_np": "priority_np",
    "priority-np": "priority_np",
    "priorityp": "priority_p",
    "priority_p": "priority_p",
    "priority-p": "priority_p",
    "hrrn": "hrrn",
    "mlfq": "mlfq"
}

class SchedulerRequestHandler(http.server.BaseHTTPRequestHandler):
    def send_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_cors_headers()
        self.end_headers()

    def send_json(self, status_code, payload):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_cors_headers()
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        url_path = self.path.split("?")[0]

        if url_path == "/api/health":
            self.send_json(200, {
                "status": "online",
                "engine": "C11 Native Scheduler",
                "executable": str(C_EXECUTABLE),
                "executable_exists": C_EXECUTABLE.exists(),
                "algorithms_supported": 8
            })
            return

        # Serve static frontend files
        if url_path in ["", "/", "/index.html"]:
            file_to_serve = FRONTEND_DIR / "index.html"
        elif url_path.startswith("/frontend/"):
            rel_path = url_path[len("/frontend/"):]
            file_to_serve = FRONTEND_DIR / rel_path
        else:
            rel_path = url_path.lstrip("/")
            file_to_serve = FRONTEND_DIR / rel_path

        if file_to_serve.is_file():
            mime_type, _ = mimetypes.guess_type(str(file_to_serve))
            if mime_type is None:
                mime_type = "application/octet-stream"
            try:
                with open(file_to_serve, "rb") as f:
                    content = f.read()
                self.send_response(200)
                self.send_header("Content-Type", mime_type)
                self.send_header("Content-Length", str(len(content)))
                self.send_cors_headers()
                self.end_headers()
                self.wfile.write(content)
            except Exception as e:
                self.send_json(500, {"error": f"Failed reading file: {str(e)}"})
        else:
            self.send_json(404, {"error": f"File not found: {url_path}"})

    def do_POST(self):
        url_path = self.path.split("?")[0]

        if url_path not in ["/api/simulate", "/api/compare"]:
            self.send_json(404, {"error": f"Unknown endpoint: {url_path}"})
            return

        if not C_EXECUTABLE.exists():
            self.send_json(500, {
                "error": f"C executable not found at '{C_EXECUTABLE}'. Please compile the project with GCC first."
            })
            return

        try:
            content_length = int(self.headers.get("Content-Length", 0))
            if content_length <= 0:
                self.send_json(400, {"error": "Missing or empty request body"})
                return

            raw_body = self.rfile.read(content_length)
            data = json.loads(raw_body.decode("utf-8"))
        except Exception as e:
            self.send_json(400, {"error": f"Invalid JSON in request: {str(e)}"})
            return

        processes = data.get("processes", [])
        if not isinstance(processes, list) or len(processes) == 0:
            self.send_json(400, {"error": "Workload must contain at least 1 process."})
            return

        if len(processes) > 100:
            self.send_json(400, {"error": "Workload cannot exceed 100 processes."})
            return

        # Validate process data
        pids_seen = set()
        total_burst = 0
        validated_rows = []

        for p in processes:
            try:
                pid = int(p.get("pid", 0))
                arrival = int(p.get("arrivalTime", p.get("arrival", 0)))
                burst = int(p.get("burstTime", p.get("burst", 0)))
                priority = int(p.get("priority", 1))
            except (ValueError, TypeError):
                self.send_json(400, {"error": f"Invalid non-integer fields in process: {p}"})
                return

            if pid <= 0:
                self.send_json(400, {"error": f"PID must be positive (got {pid})."})
                return
            if pid in pids_seen:
                self.send_json(400, {"error": f"Duplicate PID {pid} detected."})
                return
            pids_seen.add(pid)

            if arrival < 0 or arrival > 1000000:
                self.send_json(400, {"error": f"Arrival time for P{pid} must be 0 to 1,000,000."})
                return
            if burst <= 0 or burst > 100000:
                self.send_json(400, {"error": f"Burst time for P{pid} must be 1 to 100,000."})
                return
            if priority <= 0 or priority > 1000000:
                self.send_json(400, {"error": f"Priority for P{pid} must be 1 to 1,000,000."})
                return

            total_burst += burst
            validated_rows.append((pid, arrival, burst, priority))

        if total_burst > 100000:
            self.send_json(400, {"error": f"Cumulative burst ({total_burst}) exceeds maximum limit of 100,000."})
            return

        quantum = int(data.get("quantum", 2))
        if quantum <= 0:
            quantum = 2

        # Create temporary CSV file for C program import
        temp_csv = None
        try:
            with tempfile.NamedTemporaryFile(mode="w", suffix=".csv", delete=False, newline="") as tf:
                temp_csv = tf.name
                tf.write("pid,arrival,burst,priority\n")
                for r in validated_rows:
                    tf.write(f"{r[0]},{r[1]},{r[2]},{r[3]}\n")

            if url_path == "/api/simulate":
                algo_raw = str(data.get("algorithm", "fcfs")).lower().strip()
                algo_key = VALID_ALGORITHMS.get(algo_raw)
                if not algo_key:
                    self.send_json(400, {"error": f"Unknown algorithm '{algo_raw}'. Supported: {list(VALID_ALGORITHMS.keys())}"})
                    return

                # Execute C binary with --json
                cmd = [str(C_EXECUTABLE), "--json", algo_key, str(quantum), temp_csv]
                proc = subprocess.run(cmd, capture_output=True, text=True, shell=False)

                if proc.returncode != 0:
                    err_msg = proc.stderr.strip() or proc.stdout.strip() or f"Process exited with code {proc.returncode}"
                    self.send_json(500, {"error": f"C Engine execution error: {err_msg}"})
                    return

                # Parse C engine JSON output
                try:
                    result = json.loads(proc.stdout)
                    self.send_json(200, result)
                except json.JSONDecodeError as jde:
                    self.send_json(500, {
                        "error": f"Failed parsing JSON from C engine output: {str(jde)}",
                        "raw_output": proc.stdout
                    })

            elif url_path == "/api/compare":
                # Execute C binary with --json-all
                cmd = [str(C_EXECUTABLE), "--json-all", str(quantum), temp_csv]
                proc = subprocess.run(cmd, capture_output=True, text=True, shell=False)

                if proc.returncode != 0:
                    err_msg = proc.stderr.strip() or proc.stdout.strip() or f"Process exited with code {proc.returncode}"
                    self.send_json(500, {"error": f"C Engine execution error: {err_msg}"})
                    return

                try:
                    results = json.loads(proc.stdout)
                    self.send_json(200, results)
                except json.JSONDecodeError as jde:
                    self.send_json(500, {
                        "error": f"Failed parsing JSON comparison array from C engine: {str(jde)}",
                        "raw_output": proc.stdout
                    })

        except Exception as ex:
            self.send_json(500, {"error": f"Internal server exception: {str(ex)}"})
        finally:
            if temp_csv and os.path.exists(temp_csv):
                try:
                    os.remove(temp_csv)
                except Exception:
                    pass

def run_server(port=8080):
    handler = SchedulerRequestHandler
    # Allow port reuse immediately
    socketserver.TCPServer.allow_reuse_address = True
    
    server_address = ("", port)
    try:
        with socketserver.TCPServer(server_address, handler) as httpd:
            print("=" * 60)
            print("  CPU Scheduling & Visualization Engine - Web Backend")
            print("=" * 60)
            print(f"  Server URL:    http://localhost:{port}")
            print(f"  Frontend:      http://localhost:{port}/frontend/index.html")
            print(f"  C Engine Exec: {C_EXECUTABLE}")
            print(f"  C Executable:  {'FOUND & READY' if C_EXECUTABLE.exists() else 'NOT FOUND (compile with GCC)'}")
            print("=" * 60)
            print("  Press Ctrl+C to terminate.")
            httpd.serve_forever()
    except OSError as err:
        if port < 8090:
            print(f"Port {port} in use, trying port {port + 1}...")
            run_server(port + 1)
        else:
            print(f"Failed to bind server: {err}")
            sys.exit(1)

if __name__ == "__main__":
    port_arg = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    run_server(port_arg)
