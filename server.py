"""
Tiny web server (no pip install needed).
 - Opens the HTML front-end
 - On "Run", it runs YOUR C program, types the menu choices into it,
   reads what it prints (Gantt chart + table) and sends that to the page.
   The C file is used exactly as it is, nothing is changed in it.

Steps:
    gcc cpu_scheduler_basic.c -o scheduler        (Windows: -o scheduler.exe)
    python server.py
    open http://localhost:8000 in the browser
"""
import json
import os
import re
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer

FOLDER = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(FOLDER, "scheduler.exe" if os.name == "nt" else "scheduler")


def run_c_program(algo, quantum, procs):
    # 1) the keys we would type in the C menu
    keys = ""
    for p in procs:
        # menu 1 = Add process, then PID, arrival, burst, priority
        keys += "1\n%d\n%d\n%d\n%d\n" % (p["pid"], p["arrival"], p["burst"], p["priority"])
    if algo == "rr":
        keys += "4\n%d\n" % quantum     # menu 4 = Round Robin, then quantum
    elif algo == "pnp":
        keys += "5\n"                   # menu 5 = Priority (Non-Preemptive)
    else:
        keys += "6\n"                   # menu 6 = Priority (Preemptive)
    keys += "0\n"                       # menu 0 = Exit

    text = subprocess.run([EXE], input=keys, capture_output=True,
                          text=True, timeout=5).stdout

    # 2) read the Gantt chart line:  | P1 (0-3) | IDLE (3-5) | ...
    segments = []
    for m in re.finditer(r"\|\s*(?:P(\d+)|IDLE)\s*\((\d+)-(\d+)\)", text):
        pid = int(m.group(1)) if m.group(1) else -1
        segments.append({"pid": pid, "start": int(m.group(2)), "end": int(m.group(3))})

    # 3) read the result table rows (7 numbers per row)
    names = ["pid", "arrival", "burst", "priority", "completion", "turnaround", "waiting"]
    rows = re.findall(r"^(\d+)\t(\d+)\t(\d+)\t(\d+)\t+(\d+)\t+(\d+)\t+(\d+)\s*$", text, re.M)
    processes = [dict(zip(names, map(int, r))) for r in rows]

    # 4) read the averages
    w = re.search(r"Average Waiting Time\s*=\s*([\d.]+)", text)
    t = re.search(r"Average Turnaround Time\s*=\s*([\d.]+)", text)
    if not segments or not processes or not w or not t:
        raise RuntimeError("could not read the C program output")

    return {"segments": segments, "processes": processes,
            "avgWaiting": float(w.group(1)), "avgTurnaround": float(t.group(1))}


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path in ("/", "/index.html"):
            with open(os.path.join(FOLDER, "cpu_scheduler.html"), "rb") as f:
                body = f.read()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_error(404)

    def do_POST(self):
        if self.path != "/run":
            self.send_error(404)
            return
        try:
            size = int(self.headers.get("Content-Length", 0))
            data = json.loads(self.rfile.read(size))
            if data["algo"] not in ("rr", "pnp", "pp"):
                raise ValueError("bad algorithm")
            result = run_c_program(data["algo"], int(data.get("quantum") or 1),
                                   data["processes"])
            body = json.dumps(result).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except Exception as e:
            self.send_error(500, str(e))


if __name__ == "__main__":
    if not os.path.exists(EXE):
        print("Compile the C program first:  gcc cpu_scheduler_basic.c -o scheduler")
    print("Open http://localhost:8000")
    HTTPServer(("localhost", 8000), Handler).serve_forever()
