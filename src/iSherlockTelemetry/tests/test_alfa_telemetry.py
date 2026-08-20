#!/usr/bin/env python3

"""End-to-end Alfa telemetry checks using a temporary MOOS community."""

from __future__ import annotations

import http.server
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import threading
import time
import unittest


REPO_ROOT = Path(__file__).resolve().parents[3]
MOOS_BIN_DIR = Path(
    os.environ.get("MOOS_BIN_DIR", REPO_ROOT.parent / "moos-ivp" / "bin")
)
APP_BINARY = Path(
    os.environ.get(
        "ISHERLOCK_TELEMETRY_BINARY", REPO_ROOT / "bin" / "iSherlockTelemetry"
    )
)


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def metrics_body(
    *,
    sample_time: float,
    collector_ok: int,
    link_up: int,
    station_count: int,
) -> str:
    lines = [
        "pearl_cmp_ble_soc_percent_gauge 80",
        "pearl_cmp_ble_charging_gauge 0",
        "pearl_cmp_ble_connected_gauge 1",
        "pearl_cmp_ble_last_sample_age_seconds_gauge 0",
        "pearl_airmar_wind_speed_meters_per_second_gauge 2",
        "pearl_airmar_wind_valid_gauge 1",
        "pearl_airmar_up_gauge 1",
        "pearl_airmar_last_sentence_age_seconds_gauge 0",
        f"pearl_wifi_backhaul_collector_ok {collector_ok}",
        f"pearl_wifi_backhaul_link_up {link_up}",
        f"pearl_wifi_backhaul_station_count {station_count}",
        f"pearl_wifi_backhaul_sample_time_seconds {sample_time:.6f}",
    ]
    if collector_ok and link_up:
        lines.extend(
            [
                "pearl_wifi_backhaul_signal_dbm -44",
                "pearl_wifi_backhaul_signal_avg_dbm -46",
                "pearl_wifi_backhaul_tx_bitrate_mbps 72.2",
                "pearl_wifi_backhaul_rx_bitrate_mbps 65.0",
                "pearl_wifi_backhaul_tx_retries_total 7",
                "pearl_wifi_backhaul_tx_failed_total 1",
                "pearl_wifi_backhaul_inactive_ms 25",
            ]
        )
    return "\n".join(lines) + "\n"


class MutableMetricsServer(http.server.ThreadingHTTPServer):
    def __init__(self, address: tuple[str, int]):
        super().__init__(address, MetricsHandler)
        self.body = b""
        self.body_lock = threading.Lock()

    def set_body(self, body: str) -> None:
        with self.body_lock:
            self.body = body.encode("utf-8")


class MetricsHandler(http.server.BaseHTTPRequestHandler):
    server: MutableMetricsServer

    def do_GET(self) -> None:  # noqa: N802 - BaseHTTPRequestHandler API
        if self.path != "/metrics":
            self.send_error(404)
            return
        with self.server.body_lock:
            body = self.server.body
        self.send_response(200)
        self.send_header("Content-Type", "text/plain; version=0.0.4")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, _format: str, *args: object) -> None:
        return


class AlfaTelemetryIntegrationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        required = [
            MOOS_BIN_DIR / "MOOSDB",
            MOOS_BIN_DIR / "pAntler",
            MOOS_BIN_DIR / "uQueryDB",
            APP_BINARY,
        ]
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            raise RuntimeError("Build dependencies are missing: " + ", ".join(missing))

        cls.metrics_port = free_port()
        cls.moos_port = free_port()
        cls.metrics_server = MutableMetricsServer(
            ("127.0.0.1", cls.metrics_port)
        )
        cls.metrics_server.set_body(
            metrics_body(
                sample_time=time.time(),
                collector_ok=1,
                link_up=1,
                station_count=1,
            )
        )
        cls.server_thread = threading.Thread(
            target=cls.metrics_server.serve_forever, daemon=True
        )
        cls.server_thread.start()

        cls.temp_dir = tempfile.TemporaryDirectory(prefix="alfa-telemetry-")
        cls.mission_path = Path(cls.temp_dir.name) / "alfa_test.moos"
        mission = f"""ServerHost = 127.0.0.1
ServerPort = {cls.moos_port}
Community  = alfa_test

ProcessConfig = ANTLER
{{
  MSBetweenLaunches = 100
  Run = {MOOS_BIN_DIR / 'MOOSDB'} @ NewConsole = false
  Run = {APP_BINARY} @ NewConsole = false
}}

ProcessConfig = iSherlockTelemetry
{{
  AppTick         = 10
  CommsTick       = 10
  metrics_host    = 127.0.0.1
  metrics_port    = {cls.metrics_port}
  metrics_path    = /metrics
  poll_interval   = 0.2
  http_timeout    = 0.5
  battery_max_age = 30
  airmar_max_age  = 5
  alfa_max_age    = 1
}}
"""
        cls.mission_path.write_text(mission, encoding="utf-8")
        cls.antler = subprocess.Popen(
            [str(MOOS_BIN_DIR / "pAntler"), str(cls.mission_path)],
            cwd=cls.temp_dir.name,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True,
        )
        cls.wait_for_moosdb()

    @classmethod
    def tearDownClass(cls) -> None:
        antler = getattr(cls, "antler", None)
        if antler is not None and antler.poll() is None:
            os.killpg(antler.pid, signal.SIGTERM)
            try:
                antler.wait(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(antler.pid, signal.SIGKILL)
                antler.wait(timeout=3)
        server = getattr(cls, "metrics_server", None)
        if server is not None:
            server.shutdown()
            server.server_close()
        temp_dir = getattr(cls, "temp_dir", None)
        if temp_dir is not None:
            temp_dir.cleanup()

    @classmethod
    def wait_for_moosdb(cls) -> None:
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if cls.antler.poll() is not None:
                raise RuntimeError("pAntler exited before MOOSDB became available")
            try:
                with socket.create_connection(
                    ("127.0.0.1", cls.moos_port), timeout=0.2
                ):
                    return
            except OSError:
                time.sleep(0.1)
        raise RuntimeError("MOOSDB did not become available within five seconds")

    def assert_query(self, condition: str) -> None:
        conditions = condition.split(" and ")
        command = [
            str(MOOS_BIN_DIR / "uQueryDB"),
            *(f"--condition={item}" for item in conditions),
            "--host=127.0.0.1",
            f"--port={self.moos_port}",
            "--wait=5",
        ]
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=8,
            check=False,
        )
        if result.returncode != 0:
            self.fail(
                f"MOOS condition did not become true: {condition}\n"
                f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
            )

    def test_connected_disconnected_failed_and_stale_samples(self) -> None:
        with self.subTest("connected"):
            self.metrics_server.set_body(
                metrics_body(
                    sample_time=time.time(),
                    collector_ok=1,
                    link_up=1,
                    station_count=1,
                )
            )
            self.assert_query(
                "ALFA_LINK_UP=1 and ALFA_DATA_VALID=1 and "
                "ALFA_SIGNAL_DATA_VALID=1 and ALFA_SIGNAL_DBM=-44"
            )

        with self.subTest("disconnected"):
            self.metrics_server.set_body(
                metrics_body(
                    sample_time=time.time(),
                    collector_ok=1,
                    link_up=0,
                    station_count=0,
                )
            )
            self.assert_query(
                "ALFA_LINK_UP=0 and ALFA_DATA_VALID=1 and "
                "ALFA_SIGNAL_DATA_VALID=0"
            )

        with self.subTest("collector failed"):
            self.metrics_server.set_body(
                metrics_body(
                    sample_time=time.time(),
                    collector_ok=0,
                    link_up=0,
                    station_count=0,
                )
            )
            self.assert_query(
                "ALFA_DATA_VALID=0 and ALFA_SIGNAL_DATA_VALID=0"
            )

        with self.subTest("stale"):
            self.metrics_server.set_body(
                metrics_body(
                    sample_time=time.time() - 5,
                    collector_ok=1,
                    link_up=1,
                    station_count=1,
                )
            )
            self.assert_query(
                "ALFA_LINK_UP=1 and ALFA_DATA_AGE>3 and "
                "ALFA_DATA_VALID=0 and ALFA_SIGNAL_DATA_VALID=0"
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
