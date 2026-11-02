"""Loopback HTTP client for the CASTLE HTTP server sample (127.0.0.1:8080)."""

import http.client
import json
import os

HOST = "127.0.0.1"
PORT = int(os.environ.get("CASTLE_HTTP_PORT", "8080"))


def check(connection, method, path, expected, body=None, headers=None):
    connection.request(method, path, body=body, headers=headers or {})
    response = connection.getresponse()
    payload = response.read()
    if response.status != expected:
        raise RuntimeError(
            f"{method} {path}: expected {expected}, received {response.status}: {payload!r}"
        )
    return response.status, payload, dict(response.getheaders())


def main():
    connection = http.client.HTTPConnection(HOST, PORT, timeout=5)
    try:
        check(connection, "GET", "/health", 200)
        status, payload, _ = check(connection, "GET", "/api/status", 200)
        assert json.loads(payload)["status"] == "ok"

        status, payload, _ = check(connection, "GET", "/devices/17/state?refresh=1", 200)
        assert payload == b"device=17;refresh=1"

        status, payload, _ = check(
            connection,
            "POST",
            "/api/devices",
            201,
            body=b'{"name":"sensor-1"}',
            headers={"Content-Type": "application/json"},
        )
        assert json.loads(payload)["name"] == "sensor-1"

        status, payload, _ = check(
            connection,
            "PUT",
            "/devices/17/state",
            200,
            body=b"active",
        )
        assert payload == b"active"

        status, payload, _ = check(connection, "HEAD", "/health", 200)
        assert payload == b""

        check(connection, "OPTIONS", "/api/status", 204)
        check(connection, "DELETE", "/devices/17", 204)
        status, payload, _ = check(connection, "GET", "/static", 200)
        assert b"CASTLE" in payload

        # Demonstrate sequential HTTP/1.1 keep-alive over the same connection.
        check(connection, "GET", "/api/v1/sensors", 200)
    finally:
        connection.close()


if __name__ == "__main__":
    main()
