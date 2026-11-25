"""Tests for the status hub. Run from the repo root: python3 -m unittest discover -s hub"""

import json
import threading
import time
import unittest
import urllib.error
import urllib.request
import http.client
import socket

from status_hub import READY_SECONDS, WORKING_STALE, Hub, HubServer, make_handler


class FakeClock:
    def __init__(self):
        self.t = 1000.0

    def __call__(self):
        return self.t


class HubLogic(unittest.TestCase):
    def setUp(self):
        self.clock = FakeClock()
        self.hub = Hub(clock=self.clock)

    def test_idle_by_default(self):
        self.assertEqual(self.hub.mode("home"), "i")

    def test_priority_needs_over_working_over_ready(self):
        self.hub.event("home", "a", "r")
        self.assertEqual(self.hub.mode("home"), "r")
        self.hub.event("home", "b", "w")
        self.assertEqual(self.hub.mode("home"), "w")
        self.hub.event("home", "c", "n")
        self.assertEqual(self.hub.mode("home"), "n")
        self.hub.event("home", "c", "end")
        self.assertEqual(self.hub.mode("home"), "w")

    def test_ready_fades_to_idle(self):
        self.hub.event("home", "a", "r")
        self.clock.t += READY_SECONDS + 1
        self.assertEqual(self.hub.mode("home"), "i")

    def test_stale_working_is_dropped(self):
        self.hub.event("home", "a", "w")
        self.clock.t += WORKING_STALE + 1
        self.assertEqual(self.hub.mode("home"), "i")
        self.assertEqual(self.hub.snapshot("home")["sessions"], [])

    def test_home_and_work_are_separate(self):
        self.hub.event("work", "a", "n")
        self.assertEqual(self.hub.mode("work"), "n")
        self.assertEqual(self.hub.mode("home"), "i")

    def test_session_moves_between_states(self):
        for state in ("w", "n", "w", "r"):
            self.hub.event("home", "a", state)
        self.assertEqual(self.hub.mode("home"), "r")


class HubHTTP(unittest.TestCase):
    TOKEN = "test-token"

    def setUp(self):
        self.hub = Hub()
        self.server = HubServer(("127.0.0.1", 0), make_handler(self.hub, self.TOKEN), max_connections=4)
        self.base = f"http://127.0.0.1:{self.server.server_address[1]}"
        threading.Thread(target=self.server.serve_forever, daemon=True).start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()

    def request(self, path, body=None, token=TOKEN):
        data = json.dumps(body).encode() if body is not None else None
        req = urllib.request.Request(self.base + path, data=data)
        if token:
            req.add_header("Authorization", f"Bearer {token}")
        with urllib.request.urlopen(req, timeout=5) as resp:
            return resp.status, resp.read().decode()

    def test_rejects_bad_token(self):
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self.request("/api/light", token="nope")
        self.assertEqual(ctx.exception.code, 401)

    def test_rejects_post_without_token(self):
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self.request("/api/event", {"session": "s", "state": "w"}, token=None)
        self.assertEqual(ctx.exception.code, 401)

    def test_token_in_url_is_not_accepted(self):
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self.request(f"/api/light?token={self.TOKEN}", token=None)
        self.assertEqual(ctx.exception.code, 401)

    def test_rejects_oversized_body(self):
        conn = http.client.HTTPConnection("127.0.0.1", self.server.server_address[1], timeout=5)
        conn.putrequest("POST", "/api/event")
        conn.putheader("Content-Length", str(10_000_000))
        conn.putheader("Authorization", f"Bearer {self.TOKEN}")
        conn.endheaders()
        self.assertEqual(conn.getresponse().status, 413)
        conn.close()

    def test_connection_cap(self):
        # Fill all 4 slots with idle connections; the next one is closed without an answer.
        port = self.server.server_address[1]
        held = [socket.create_connection(("127.0.0.1", port)) for _ in range(4)]
        time.sleep(0.3)
        try:
            extra = socket.create_connection(("127.0.0.1", port))
            extra.settimeout(3)
            extra.sendall(b"GET /healthz HTTP/1.1\r\nHost: x\r\n\r\n")
            try:
                self.assertEqual(extra.recv(100), b"")  # closed, no response
            except ConnectionResetError:
                pass
            extra.close()
        finally:
            for s in held:
                s.close()
        time.sleep(0.3)
        self.assertEqual(self.request("/healthz", token=None), (200, "ok\n"))  # slots freed

    def test_health_needs_no_token(self):
        self.assertEqual(self.request("/healthz", token=None), (200, "ok\n"))

    def test_event_then_light(self):
        status, body = self.request("/api/event", {"session": "s1", "state": "w"})
        self.assertEqual((status, json.loads(body)["mode"]), (200, "w"))
        self.assertEqual(self.request("/api/light?have=i&wait=0"), (200, "w"))

    def test_rejects_unknown_state(self):
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self.request("/api/event", {"session": "s1", "state": "x"})
        self.assertEqual(ctx.exception.code, 400)

    def test_path_prefix_is_ignored(self):
        self.request("/status-light/api/event", {"session": "s1", "state": "n"})
        self.assertEqual(self.request("/status-light/api/light?wait=0"), (200, "n"))

    def test_long_poll_returns_on_change(self):
        result = {}

        def poll():
            start = time.monotonic()
            result["body"] = self.request("/api/light?have=i&wait=10")[1]
            result["took"] = time.monotonic() - start

        t = threading.Thread(target=poll)
        t.start()
        time.sleep(0.3)
        self.request("/api/event", {"session": "s1", "state": "w"})
        t.join(5)
        self.assertEqual(result["body"], "w")
        self.assertLess(result["took"], 3)

    def test_long_poll_times_out_with_current_mode(self):
        start = time.monotonic()
        self.assertEqual(self.request("/api/light?have=i&wait=1")[1], "i")
        self.assertGreaterEqual(time.monotonic() - start, 0.9)

    def test_state_snapshot(self):
        self.request("/api/event", {"session": "s1", "state": "w", "channel": "work"})
        snap = json.loads(self.request("/api/state?channel=work")[1])
        self.assertEqual(snap["mode"], "w")
        self.assertEqual(snap["sessions"][0]["session"], "s1")


if __name__ == "__main__":
    unittest.main()
