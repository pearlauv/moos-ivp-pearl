#!/usr/bin/env python3

"""End-to-end fail-closed landing-gate checks for pRendezvous."""

from __future__ import annotations

import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import time
import unittest


REPO_ROOT = Path(__file__).resolve().parents[3]
MOOS_BIN_DIR = Path(
    os.environ.get("MOOS_BIN_DIR", REPO_ROOT.parent / "moos-ivp" / "bin")
)
APP_BINARY = Path(
    os.environ.get("PRENDEZVOUS_BINARY", REPO_ROOT / "bin" / "pRendezvous")
)


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


class LandingGateIntegrationTest(unittest.TestCase):
    def setUp(self) -> None:
        required = [
            MOOS_BIN_DIR / "MOOSDB",
            MOOS_BIN_DIR / "pAntler",
            MOOS_BIN_DIR / "uPokeDB",
            MOOS_BIN_DIR / "uQueryDB",
            APP_BINARY,
        ]
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            raise RuntimeError("Build dependencies are missing: " + ", ".join(missing))

        self.moos_port = free_port()
        self.temp_dir = tempfile.TemporaryDirectory(prefix="rendezvous-gate-")
        self.temp_path = Path(self.temp_dir.name)
        self.mission_path = self.temp_path / "landing_gate.moos"
        self.antler_log_path = self.temp_path / "pAntler.log"
        pearl_role = self._testMethodName == "test_pearl_separation_is_continuous"
        role = "pearl" if pearl_role else "uav"
        ownship = "pearl" if pearl_role else "uav"
        peer_node = "uav" if pearl_role else "pearl"
        arrival_dwell = 4 if pearl_role else 0
        acquire_timeout = 5 if self._testMethodName == "test_target_timeout" else 60
        self.mission_path.write_text(
            f"""ServerHost = 127.0.0.1
ServerPort = {self.moos_port}
Community  = landing_gate_test

ProcessConfig = ANTLER
{{
  MSBetweenLaunches = 100
  Run = {MOOS_BIN_DIR / 'MOOSDB'} @ NewConsole = false
  Run = {APP_BINARY} @ NewConsole = false
}}

ProcessConfig = pRendezvous
{{
  AppTick   = 10
  CommsTick = 10
  role      = {role}
  ownship   = {ownship}
  peer_node = {peer_node}
  require_health = false
  require_battery = false
  require_flight_state = false
  require_landing_target = true
  nav_stale_thresh = 30
  request_timeout = 30
  route_timeout = 10
  transit_timeout = 30
  arrival_radius = 5
  arrival_dwell = {arrival_dwell}
  max_peer_separation = 3
  landing_target_max_age = 5
  landing_target_lock_dwell = 4
  landing_target_acquire_timeout = {acquire_timeout}
  landing_target_max_offset = 1.5
  landing_target_max_angle = 0.2
  expected_target_num = 0
  reacquire_update_interval = 0.2
  reacquire_update_distance = 0.1
  state_post_interval = 0.1
}}
""",
            encoding="utf-8",
        )
        self.antler_log = self.antler_log_path.open("w", encoding="utf-8")
        self.antler = subprocess.Popen(
            [str(MOOS_BIN_DIR / "pAntler"), str(self.mission_path)],
            cwd=self.temp_path,
            stdout=self.antler_log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        self.wait_for_moosdb()

    def tearDown(self) -> None:
        if self.antler.poll() is None:
            os.killpg(self.antler.pid, signal.SIGTERM)
            try:
                self.antler.wait(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(self.antler.pid, signal.SIGKILL)
                self.antler.wait(timeout=3)
        self.antler_log.close()
        self.temp_dir.cleanup()

    def wait_for_moosdb(self) -> None:
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if self.antler.poll() is not None:
                self.fail("pAntler exited before MOOSDB became available")
            try:
                with socket.create_connection(
                    ("127.0.0.1", self.moos_port), timeout=0.2
                ):
                    return
            except OSError:
                time.sleep(0.1)
        self.fail("MOOSDB did not become available within five seconds")

    def poke(self, *pairs: str) -> None:
        result = subprocess.run(
            [str(MOOS_BIN_DIR / "uPokeDB"), str(self.mission_path), "--quiet", *pairs],
            cwd=self.temp_path,
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        if result.returncode != 0:
            self.fail(f"uPokeDB failed: {pairs}\n{result.stdout}\n{result.stderr}")

    def assert_query(self, *conditions: str, wait: float = 3) -> None:
        result = subprocess.run(
            [
                str(MOOS_BIN_DIR / "uQueryDB"),
                str(self.mission_path),
                *(f"--condition={condition}" for condition in conditions),
                f"--wait={wait}",
            ],
            cwd=self.temp_path,
            capture_output=True,
            text=True,
            timeout=wait + 3,
            check=False,
        )
        if result.returncode != 0:
            output = self.antler_log_path.read_text(encoding="utf-8", errors="replace")
            self.fail(
                f"MOOS conditions did not become true: {conditions}\n"
                f"{result.stdout}\n{result.stderr}\npAntler:\n{output}"
            )

    def query_value(self, variable: str, condition: str) -> str:
        checkvars = self.temp_path / ".checkvars"
        checkvars.unlink(missing_ok=True)
        result = subprocess.run(
            [
                str(MOOS_BIN_DIR / "uQueryDB"),
                str(self.mission_path),
                f"--condition={condition}",
                f"--check_var={variable}",
                "--csv",
                "--wait=3",
            ],
            cwd=self.temp_path,
            capture_output=True,
            text=True,
            timeout=6,
            check=False,
        )
        if result.returncode != 0 or not checkvars.is_file():
            output = self.antler_log_path.read_text(encoding="utf-8", errors="replace")
            self.fail(
                f"Could not query {variable} under {condition}\n"
                f"{result.stdout}\n{result.stderr}\npAntler:\n{output}"
            )
        line = checkvars.read_text(encoding="utf-8").strip()
        return line.split(",", 1)[1]

    def enter_acquisition(self) -> str:
        self.poke("NAV_X=0", "NAV_Y=0")
        time.sleep(0.2)
        self.poke("RENDEZVOUS_START=true")
        session = self.query_value("UAV_RENDEZVOUS_SESSION", "DB_UPTIME>1")
        self.assertTrue(session, "rendezvous request did not create a session")
        self.poke(f"RENDEZVOUS_PROPOSAL:=id={session}#x=0#y=0")
        time.sleep(0.2)
        self.poke("ROUTE_BUFFER_VEHICLE_STATE:=DEPLOY_ACCEPTED")
        phase = self.query_value(
            "UAV_RENDEZVOUS_PHASE", "UAV_RENDEZVOUS_PHASE!=__missing__"
        )
        self.assertEqual(phase, "RENDEZVOUS")
        self.poke(
            "NODE_REPORT:=NAME=pearl,X=0.5,Y=0",
            f"LANDING_CLEARANCE:=id={session}#status=cleared#x=0.5#y=0",
        )
        self.assert_query("UAV_RENDEZVOUS_PHASE=ACQUIRING_TARGET")
        return session

    def post_target(self, *, available: int = 1, target_num: int = 0) -> None:
        self.poke(
            f"UAV_LANDING_TARGET_AVAILABLE={available}",
            "UAV_LANDING_TARGET_AGE=0.1",
            f"UAV_LANDING_TARGET_TARGET_NUM={target_num}",
            "UAV_LANDING_TARGET_POSITION_VALID=1",
            "UAV_LANDING_TARGET_X=0.2",
            "UAV_LANDING_TARGET_Y=0.1",
            "UAV_LANDING_TARGET_ANGLE_X=0.01",
            "UAV_LANDING_TARGET_ANGLE_Y=0.01",
        )

    def test_wrong_target_and_lock_loss_cannot_commit(self) -> None:
        self.enter_acquisition()

        self.poke("UAV_PREC_LAND_REQUEST=true")
        result = self.query_value(
            "UAV_PREC_LAND_RESULT", "UAV_PREC_LAND_RESULT!=__missing__"
        )
        self.assertEqual(
            result,
            "status=rejected#reason=coordinated_clearance_required",
        )
        self.assert_query("UAV_RENDEZVOUS_PHASE=ACQUIRING_TARGET")

        self.post_target(target_num=0)
        self.poke("UAV_LANDING_TARGET_AGE=6")
        self.assert_query(
            "UAV_LANDING_GATE_REASON=landing_target_too_old",
            "UAV_RENDEZVOUS_PHASE=ACQUIRING_TARGET",
        )

        self.post_target(target_num=1)
        self.assert_query(
            "UAV_LANDING_GATE_REASON=unexpected_landing_target",
            "UAV_RENDEZVOUS_PHASE=ACQUIRING_TARGET",
        )

        self.post_target(target_num=0)
        self.post_target(available=0)
        self.assert_query(
            "UAV_LANDING_GATE_REASON=landing_target_unavailable",
            "UAV_RENDEZVOUS_PHASE=ACQUIRING_TARGET",
        )

        self.post_target(target_num=0)
        self.post_target(target_num=0)
        self.post_target(target_num=0)
        self.post_target(target_num=0)
        self.assert_query(
            "UAV_LANDING_GATE_READY=1",
            "UAV_RENDEZVOUS_PHASE=LANDING",
            "UAV_PREC_LAND_COMMIT=true",
        )

    def test_target_timeout(self) -> None:
        self.enter_acquisition()
        self.assert_query(
            "UAV_RENDEZVOUS_PHASE=ABORT",
            "UAV_LANDING_GATE_REASON=landing_target_timeout",
            wait=7,
        )

    def test_pearl_separation_is_continuous(self) -> None:
        self.poke("NAV_X=0", "NAV_Y=0")
        time.sleep(0.2)
        self.poke(
            "RENDEZVOUS_REQUEST:=id=test#x=20#y=0#speed=3#battery=80#health=ok"
        )
        self.assert_query("PEARL_RENDEZVOUS_PHASE=REQUESTING")
        self.poke("RENDEZVOUS_RESPONSE:=id=test#status=accepted")
        self.assert_query("PEARL_RENDEZVOUS_PHASE=RENDEZVOUS")

        # Both vehicles are inside the five-metre rendezvous circle, but they
        # are more than three metres apart, so clearance remains blocked.
        self.poke("NAV_X=2.86", "NAV_Y=0", "NODE_REPORT:=NAME=uav,X=7,Y=0")
        self.assert_query(
            "PEARL_LANDING_GATE_READY=0",
            "PEARL_LANDING_GATE_REASON=peer_separation_too_large",
            "PEARL_RENDEZVOUS_PHASE=RENDEZVOUS",
        )

        # A brief close report starts the dwell, then moving apart must reset
        # it instead of leaving an old 'arrived' decision latched.
        self.poke("NODE_REPORT:=NAME=uav,X=2.9,Y=0")
        self.poke("NODE_REPORT:=NAME=uav,X=7,Y=0")
        self.assert_query(
            "PEARL_LANDING_GATE_REASON=peer_separation_too_large",
            "PEARL_RENDEZVOUS_PHASE=RENDEZVOUS",
        )

        self.poke("NODE_REPORT:=NAME=uav,X=2.9,Y=0")
        self.assert_query(
            "PEARL_RENDEZVOUS_PHASE=ACQUIRING_TARGET",
            "PEARL_LANDING_GATE_READY=1",
            wait=7,
        )
        self.poke("RENDEZVOUS_RESPONSE:=id=test#status=landing")
        self.assert_query("PEARL_RENDEZVOUS_PHASE=LANDING")


if __name__ == "__main__":
    unittest.main(verbosity=2)
