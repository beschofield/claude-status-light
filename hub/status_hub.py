#!/usr/bin/env python3
"""Status hub: Claude Code hooks report here, status lights pull from here.

Every Claude Code session (on any machine: charon, a home PC, a work laptop) sends its
state with `client/claude-light`. The hub combines the sessions on a channel into one
mode, and each light long-polls for its channel's mode. Lights only make outbound
requests, so they work on any Wi-Fi that reaches the internet.

Channels keep places apart: e.g. sessions on the work laptop post to "work" and the
work light listens to "work"; everything else uses "home". Only a session id and a
one-letter state are ever sent; no prompts, code or file names.

Modes (one character, same as the firmware):
    n  needs you   (any session waiting on a permission prompt or question)
    w  working     (any session busy)
    r  ready       (a session finished in the last READY_SECONDS)
    i  idle        (nothing going on)
Priority when sessions disagree: n > w > r > i.

API (all except /healthz need the token, as an `Authorization: Bearer TOKEN` header; a token
in the URL is not accepted, so it never lands in logs):
    POST /api/event   {"session": "...", "state": "w|n|r|i|end", "channel": "home"}
    GET  /api/light?channel=home&have=i&wait=25
         Plain-text mode character. Returns as soon as the mode differs from `have`,
         or after `wait` seconds (max 55) with the current mode.
    GET  /api/state?channel=home   JSON with the mode and each session (for debugging)
    GET  /healthz                  "ok"
Anything before /api/ in the path is ignored, so the hub can sit under a Funnel path.

Run:
    python3 hub/status_hub.py                 # 127.0.0.1:8787
    python3 hub/status_hub.py --port 8787 --bind 0.0.0.0
Token: $STATUS_HUB_TOKEN, else ~/.config/claude-status-light/token (created on first run).
"""

import argparse
import json
import os
import secrets
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

READY_SECONDS = 60           # how long "ready" shows after a session stops
WORKING_STALE = 20 * 60      # a "working" session silent this long is assumed gone
NEEDS_STALE = 3 * 60 * 60    # same for "needs you" (e.g. a killed session never answered)
MAX_WAIT = 55                # longest long-poll, below common proxy idle timeouts
MAX_CONNECTIONS = 32         # simultaneous connections; more are closed at once
IDLE_TIMEOUT = 70            # seconds a connection may sit silent (must exceed MAX_WAIT)
MAX_BODY = 4096              # bytes; event bodies are ~100
PRIORITY = {"n": 3, "w": 2, "r": 1, "i": 0}
STATES = set(PRIORITY) | {"end"}
DEFAULT_CHANNEL = "home"
CONFIG_DIR = Path(os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config")) / "claude-status-light"


class Hub:
    """Per-channel session states and the combined mode. Thread-safe."""

    def __init__(self, clock=time.monotonic):
        self.clock = clock
        self.cond = threading.Condition()
        self.channels: dict[str, dict[str, tuple[str, float]]] = {}

    def event(self, channel: str, session: str, state: str) -> str:
        with self.cond:
            sessions = self.channels.setdefault(channel, {})
            if state in ("i", "end"):
                sessions.pop(session, None)
            else:
                sessions[session] = (state, self.clock())
            self.cond.notify_all()
            return self._mode(channel)

    @staticmethod
    def _effective(state: str, age: float) -> str:
        limit = {"r": READY_SECONDS, "w": WORKING_STALE, "n": NEEDS_STALE}.get(state, 0)
        return state if age < limit else "i"

    def _mode(self, channel: str) -> str:
        now = self.clock()
        sessions = self.channels.get(channel, {})
        best = "i"
        for sid, (state, since) in list(sessions.items()):
            eff = self._effective(state, now - since)
            if eff == "i":
                del sessions[sid]  # expired: forget it
            elif PRIORITY[eff] > PRIORITY[best]:
                best = eff
        return best

    def mode(self, channel: str) -> str:
        with self.cond:
            return self._mode(channel)

    def wait(self, channel: str, have: str, timeout: float) -> str:
        """Block until the channel's mode differs from `have`, or until `timeout`."""
        deadline = self.clock() + timeout
        with self.cond:
            while True:
                current = self._mode(channel)
                remaining = deadline - self.clock()
                if current != have or remaining <= 0:
                    return current
                # Wake at least once a second so time-based changes (ready -> idle) are seen.
                self.cond.wait(min(1.0, remaining))

    def snapshot(self, channel: str) -> dict:
        with self.cond:
            mode = self._mode(channel)
            now = self.clock()
            sessions = [
                {"session": sid, "state": state, "age_s": round(now - since, 1)}
                for sid, (state, since) in self.channels.get(channel, {}).items()
            ]
            return {"channel": channel, "mode": mode, "sessions": sessions}


def load_token() -> str:
    token = os.environ.get("STATUS_HUB_TOKEN", "").strip()
    if token:
        return token
    path = CONFIG_DIR / "token"
    if path.exists():
        return path.read_text().strip()
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    token = secrets.token_urlsafe(24)
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w") as f:
        f.write(token + "\n")
    print(f"created a new token in {path}", file=sys.stderr)
    return token


class HubServer(ThreadingHTTPServer):
    """ThreadingHTTPServer with a cap on simultaneous connections.

    The hub is reachable from the internet, so a flood of connections must not pile up
    threads: past MAX_CONNECTIONS, new ones are closed straight away.
    """

    daemon_threads = True

    def __init__(self, *args, max_connections: int = MAX_CONNECTIONS, **kwargs):
        super().__init__(*args, **kwargs)
        self.slots = threading.BoundedSemaphore(max_connections)

    def process_request(self, request, client_address):
        if not self.slots.acquire(blocking=False):
            self.shutdown_request(request)
            return
        try:
            super().process_request(request, client_address)
        except Exception:
            self.slots.release()
            raise

    def process_request_thread(self, request, client_address):
        try:
            super().process_request_thread(request, client_address)
        finally:
            self.slots.release()


def make_handler(hub: Hub, token: str):
    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"  # keep-alive, so lights reuse one TLS connection
        timeout = IDLE_TIMEOUT  # drop connections that go quiet (slow or abandoned clients)

        def log_message(self, fmt, *args):  # quiet: lights poll constantly
            if os.environ.get("STATUS_HUB_VERBOSE"):
                super().log_message(fmt, *args)

        def _send(self, code: int, body: str, ctype: str = "text/plain") -> None:
            data = body.encode()
            self.send_response(code)
            self.send_header("Content-Type", ctype + "; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(data)

        def _route(self):
            url = urlparse(self.path)
            idx = url.path.find("/api/")
            path = url.path[idx:] if idx >= 0 else url.path
            query = {k: v[-1] for k, v in parse_qs(url.query).items()}
            return path, query

        def _authorized(self, query: dict) -> bool:
            # Header only: a token in the URL could end up in logs or browser history.
            header = self.headers.get("Authorization", "")
            given = header[7:].strip() if header.startswith("Bearer ") else ""
            return secrets.compare_digest(given.encode(), token.encode())

        def do_GET(self):
            path, query = self._route()
            if path.endswith("/healthz"):
                return self._send(200, "ok\n")
            if not self._authorized(query):
                return self._send(401, "bad token\n")
            channel = query.get("channel") or DEFAULT_CHANNEL
            if path == "/api/light":
                try:
                    wait = max(0.0, min(float(query.get("wait", 0)), MAX_WAIT))
                except ValueError:
                    wait = 0.0
                return self._send(200, hub.wait(channel, query.get("have", ""), wait))
            if path == "/api/state":
                return self._send(200, json.dumps(hub.snapshot(channel)) + "\n", "application/json")
            return self._send(404, "not found\n")

        def do_POST(self):
            path, query = self._route()
            try:
                length = int(self.headers.get("Content-Length", 0))
            except ValueError:
                length = -1
            if length < 0 or length > MAX_BODY:
                self.close_connection = True  # don't read the rest of it
                return self._send(413, "request too large\n")
            raw = self.rfile.read(length) if length else b""
            if not self._authorized(query):
                return self._send(401, "bad token\n")
            if path != "/api/event":
                return self._send(404, "not found\n")
            try:
                body = json.loads(raw or b"{}")
                state = str(body.get("state", ""))
                session = str(body.get("session") or "manual")[:128]
                channel = str(body.get("channel") or DEFAULT_CHANNEL)[:64]
            except (ValueError, AttributeError):
                return self._send(400, "bad json\n")
            if state not in STATES:
                return self._send(400, "state must be one of w n r i end\n")
            mode = hub.event(channel, session, state)
            return self._send(200, json.dumps({"mode": mode}) + "\n", "application/json")

    return Handler


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8787)
    parser.add_argument("--init-token", action="store_true",
                        help="create the token file if missing, print where it is, and exit")
    args = parser.parse_args()
    if args.init_token:
        load_token()
        print(f"token: {CONFIG_DIR / 'token'}")
        return
    hub = Hub()
    server = HubServer((args.bind, args.port), make_handler(hub, load_token()))
    print(f"status hub on http://{args.bind}:{args.port}", file=sys.stderr)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
