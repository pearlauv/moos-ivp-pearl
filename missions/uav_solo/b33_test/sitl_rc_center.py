#!/usr/bin/env python3
"""Supply neutral, center-sprung input to ArduPilot's SITL RC UDP port."""

import argparse
import socket
import struct
import time

from pymavlink import mavutil


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=5501)
    parser.add_argument("--mavlink", default="udpin:127.0.0.1:14551")
    args = parser.parse_args()

    mavlink = mavutil.mavlink_connection(args.mavlink, source_system=255)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    fc_loiter = False

    while True:
        message = mavlink.recv_match(blocking=True, timeout=0.02)
        if (
            message is not None
            and message.get_type() == "HEARTBEAT"
            and message.autopilot != mavutil.mavlink.MAV_AUTOPILOT_INVALID
        ):
            fc_loiter = message.custom_mode == 5

        if fc_loiter:
            rc_packet = struct.pack(
                "<8H", 1500, 1500, 1500, 1500, 1800, 1000, 1000, 1800
            )
            sock.sendto(rc_packet, (args.host, args.port))
        time.sleep(0.005)


if __name__ == "__main__":
    main()
