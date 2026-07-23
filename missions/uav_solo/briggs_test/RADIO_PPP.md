# Holybro SiK radio PPP link

This setup carries ordinary IP traffic between the shoreside Mac and the UAV
Raspberry Pi over a pair of Holybro SiK Telemetry Radio V3 radios. The radios
remain a transparent serial link; `pppd` creates a point-to-point `ppp0`
network interface on each computer.

```text
Mac ppp0: 10.0.0.1  <== SiK radio link ==>  Pi ppp0: 10.0.0.2
```

Ethernet and Wi-Fi continue operating normally. The PPP peer files do not add
a default route, DNS server, or NAT rule, so only traffic addressed to the PPP
peer uses the radio link.

## Current Holybro radio settings

Both radios must have matching serial and RF settings. The pair was initially
tested at its 57.6/64 defaults. Bench and 20-foot indoor testing on 2026-07-21
resulted in this current test profile:

```text
SERIAL_SPEED=115   # 115200 baud
AIR_SPEED=64       # 64 kbps; selected after the 20-foot packet-loss test
NETID=25
TXPOWER=20         # 20 dBm (100 mW radio maximum)
ECC=0
MAVLINK=0          # transparent serial mode
OPPRESEND=0
RTSCTS=1           # match crtscts in both PPP peer files
```

Holybro supports rates through 250 kbps, but higher rates reduce receiver
sensitivity and therefore range. The 128 kbps profile performed well at very
short range but lost 55-60% of 1000-byte pings at roughly 20 feet. A 96 kbps
test with RTS/CTS still lost 23%; the current 64 kbps profile reduced that to
3.3% and produced symmetric TCP throughput. This is the current preferred
profile, but it is not yet flight-qualified. If the intended-distance test is
still lossy, evaluate `ECC=1` or a lower air rate when reliable larger UDP
datagrams matter more than throughput.

Changing a radio parameter requires an explicit `AT&W` save and reboot. Do not
change radio parameters as part of normal PPP startup. PPP also requires
exclusive access to each serial device: close QGroundControl radio sessions,
`screen`, `picocom`, and any other process using that device first.

## Raspberry Pi configuration

The Pi runs PPP as a persistent systemd service. Its radio is selected by its
stable USB serial ID instead of the potentially changing `/dev/ttyUSB0` name.

Install the package:

```bash
sudo apt install ppp
```

Create `/etc/ppp/peers/sik`:

```text
/dev/serial/by-id/usb-FTDI_FT231X_USB_UART_DU0E4L0Y-if00-port0 115200
noauth
local
crtscts
nodetach
passive
persist
maxfail 0
holdoff 2
10.0.0.2:10.0.0.1
```

Create `/etc/systemd/system/ppp-sik.service`:

```ini
[Unit]
Description=PPP over Holybro SiK telemetry radio
StartLimitIntervalSec=0

[Service]
Type=simple
ExecStart=/usr/sbin/pppd call sik
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Enable it once:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now ppp-sik.service
```

The service starts at every Pi boot. If the radio is absent or unplugged,
systemd retries every five seconds and recovers after the same radio is
reconnected. It is normal for the service to wait without a `ppp0` address
until the Mac side starts.

Useful Pi checks:

```bash
systemctl status ppp-sik.service
journalctl -u ppp-sik.service -n 50 --no-pager
ip address show ppp0
ping 10.0.0.1
```

Do not also run `pppd` manually on the Pi while the service is enabled.

## macOS configuration and operation

macOS supplies `/usr/sbin/pppd`. Create `/etc/ppp/peers/sik` with the serial
name of the ground radio:

```text
/dev/cu.usbserial-DU0E55KZ 115200
noauth
local
crtscts
nodetach
passive
10.0.0.1:10.0.0.2
```

The Mac is the operator-controlled endpoint. Start it manually in a dedicated
terminal when the radio network is needed:

```bash
sudo pppd call sik
```

Because `nodetach` keeps it in the foreground, use `Ctrl-C` in that terminal
to stop it cleanly. In another terminal, verify the link:

```bash
ifconfig ppp0
ping 10.0.0.2
ssh -o ObscureKeystrokeTiming=no -i ~/.ssh/pearl_pi pearl@10.0.0.2
```

`ObscureKeystrokeTiming=no` matters on this low-bandwidth link. OpenSSH's
timing-obfuscation packets can create seconds of queueing while typing. The
option does not increase the radio's physical data rate or materially affect
bulk transfer speed; it removes that interactive chaff at the cost of exposing
keystroke timing on this dedicated point-to-point network.

### SSH versus Mosh

Mosh is installed on both endpoints and is useful when interactive SSH feels
laggy because it predicts local terminal state and survives link interruptions.
It uses SSH for authentication and startup, then switches to encrypted UDP.

```bash
mosh --ssh="ssh -i ~/.ssh/pearl_pi -o ObscureKeystrokeTiming=no" \
  pearl@10.0.0.2
```

The default adaptive predictor is recommended. To force prediction for the
most responsive visual echo, add `--predict=always`; predicted characters may
be temporarily underlined until the Pi confirms them.

The following controlled measurements used the current 20-foot, 64 kbps air
profile. The typing probe entered ten-character tokens at 20 ms per character;
"visible" is when the complete typed token appeared and "confirmed" is when a
Pi process acknowledged the submitted line.

| Interactive terminal | Startup | Token visible | Pi confirmation | Idle link use |
| --- | ---: | ---: | ---: | ---: |
| SSH with default timing obfuscation | 5.26 s | individual trials varied from 0.5-6.3 s | same delay | not retained |
| SSH with `ObscureKeystrokeTiming=no` | about 4.0 s | 0.53 s average | 0.74 s average | zero bytes in 15 s |
| Mosh, adaptive prediction | about 4.7 s | 0.39 s average | 0.75 s average | about 0.27 kbps each way |
| Mosh, `--predict=always` | about 4.8 s | 0.33 s average | 0.75 s average | similar to adaptive |

Mosh improves perceived typing latency and reduces visible jitter, but it
cannot make the Pi execute a command faster; confirmation still crosses the
same radio link. Its startup is slightly slower because it first bootstraps
through SSH. Active typing and especially full-screen repainting still compete
with MOOS traffic. SSH remains the right tool for `scp`, scripted commands,
port forwarding, and bulk transfers. Mosh is preferable for a long-lived
interactive shell when temporary packet loss or a PPP restart is expected.

Mosh normally opens a Pi-side UDP port in the 60000-61000 range. This works
directly across PPP, but any later firewall policy must allow the chosen UDP
port. Mosh synchronizes only visible terminal state, so use `tmux` on the Pi if
durable history and scrollback matter.

The `noauth` option disables PPP-level PAP/CHAP authentication for this direct
link. SSH authentication and encryption still protect the SSH session.

## Running `briggs_test` across PPP

The normal top-level `launch.sh` starts both communities on one computer. For
a split Pi/Mac mission, launch the two subcommunities separately.

On the Pi, start the simulated vehicle community:

```bash
cd ~/moos-ivp-pearl/missions/uav_solo/briggs_test
./launch_vehicle.sh --mode=SIM --auto \
  --ip=10.0.0.2 --shore=10.0.0.1 \
  --mport=9001 --pshare=9201 --shore_pshare=9200
```

On the Mac, start shoreside:

```bash
cd ~/moos-ivp-pearl/missions/uav_solo/briggs_test
./launch_shoreside.sh --mode=SIM --auto \
  --ip=10.0.0.1 --mport=9000 --pshare=9200
```

Use the PPP address explicitly on both hosts. Each launcher's `--ip` value is
forced into `pHostInfo`, so broker discovery advertises only `10.0.0.1` for
shoreside and `10.0.0.2` for the vehicle. The Pi's separate
`--shore=10.0.0.1` argument is the initial vehicle-to-shoreside route. This
prevents Wi-Fi, Ethernet, or Tailscale addresses from being selected as an
automatic return path.

The generated `pShare` blocks still contain `input = route = localhost:<port>`.
That is the local UDP listener declaration, not a remote-network fallback.
`pHostInfo` replaces `localhost` in the route it advertises to the broker with
the forced `--ip` value, yielding `10.0.0.1:9200` and `10.0.0.2:9201`.
`ServerHost` remains `localhost`, because each community's processes connect to
the MOOSDB on the same computer. This local connection is not a network
fallback: all broker-advertised and cross-community routes remain pinned to the
PPP addresses. If `ppp0` is down, the local communities may start, but they
cannot silently reconnect to each other through Wi-Fi or Tailscale.

The same network arguments apply to `--real`; only the vehicle-side
ArduPilot endpoint and operating mode change. Test with `--sim` first because
it validates PPP, MOOSDB, pShare, and the broker path without opening the
flight controller or commanding hardware.

Expected evidence of a working split mission includes:

- `ppp0` is `10.0.0.1` on the Mac and `10.0.0.2` on the Pi.
- Both PPP peers respond to `ping`.
- The Pi vehicle's `uFldNodeBroker` reports the shoreside route as connected.
- Shoreside receives the `briggs` node report and vehicle appcasts.

## Bench measurements and RFD900x2 comparison

These are end-to-end TCP-over-PPP measurements, not advertised RF rates. The
64 KiB payload used the `10.0.0.x` addresses. Direct Ethernet carried only the
benchmark control commands during the initial bench test; it was physically
inactive during the 20-foot test. Values are useful for comparing these two
setups, but are not a substitute for an outdoor range test.

| Radio/profile | Ping RTT | Mac to Pi | Pi to Mac | Warm SSH startup |
| --- | ---: | ---: | ---: | ---: |
| RFD900x2, Async 4.03, serial 115.2, air 64, directed unicast | about 232 ms | 1.47 KiB/s (12.0 kbps) | 2.13 KiB/s (17.5 kbps) | about 5.3 s |
| Holybro SiK V3 defaults, serial 57.6, air 64 | 211 ms | 1.08 KiB/s (8.9 kbps) | 1.48 KiB/s (12.1 kbps) | not measured |
| Holybro, serial 115.2, air 128, very-short-range bench | 128 ms | 6.08 KiB/s (49.8 kbps) | 6.54 KiB/s (53.6 kbps) | 2.36-3.21 s |
| Holybro, serial 115.2, air 96, RTS/CTS, about 20 feet | 143 ms | 7.91 KiB/s (64.8 kbps) | 3.37 KiB/s (27.6 kbps) | not remeasured |
| Holybro current profile, serial 115.2, air 64, RTS/CTS, about 20 feet | 208 ms | 5.69 KiB/s (46.6 kbps) | 5.65 KiB/s (46.3 kbps) | not remeasured |

All 56-byte ping samples had zero loss. At 20 feet, 1000-byte pings lost 55-60%
at air rate 128, 23% at air rate 96 with RTS/CTS, and 3.3% at air rate 64 with
RTS/CTS. TCP retransmission kept every sustained stream usable, but MOOS pShare
uses UDP and will not recover a lost datagram. Prefer small, low-rate shares
and verify the actual node-report payloads during mission testing.

The current Holybro profile measured about 3.9 times faster Mac-to-Pi and 2.7
times faster Pi-to-Mac than the tested RFD Async setup, with about 24 ms lower
small-packet RTT. RTS/CTS plus the 64 kbps air rate also removed the severe
direction asymmetry seen in the faster Holybro profiles. Large-frame loss and
intended-distance performance remain qualification concerns. Treat the serial
radio as a constrained, effectively half-duplex mission link: keep high-rate
MOOS shares off it and avoid large transfers while flying.

The RFD result is specific to its Async/mesh firmware despite using directed
destination IDs. It is not a comparison with RFD point-to-point firmware.
Likewise, these near-range bench results say nothing about which hardware will
retain a link farther away. Re-run sustained traffic and packet-loss tests at
the intended operational distance before relying on either pair.

For mission operation, reserve the radio for SSH commands, vehicle reports,
mission commands, and deliberately selected pShare variables. Move logs,
repositories, images, and build artifacts over Ethernet or Wi-Fi.

## Troubleshooting

Check serial ownership before restarting PPP:

```bash
# Mac
lsof /dev/cu.usbserial-DU0E55KZ

# Pi
sudo lsof /dev/serial/by-id/usb-FTDI_FT231X_USB_UART_DU0E4L0Y-if00-port0
```

Only `pppd` should own each radio during operation. If `ppp0` does not appear,
confirm both radios are powered, their green link indicators are established,
the settings above still match, and both PPP endpoints are running. Ethernet
SSH to `pearl@192.168.50.2` remains available for maintenance while the direct
cable is connected.

After swapping between Holybro and RFD radios, do not trust an existing
`ppp0`: a stale interface can remain present even though no packets cross it.
Update the serial device and baud rate in both peer files, stop the old Mac
`pppd`, restart `ppp-sik.service` on the Pi, and start the Mac peer again. A
successful `ping 10.0.0.2` is the required link check.

## References

- [Holybro SiK Telemetry Radio V3 specifications](https://holybro.com/products/sik-telemetry-radio-v3)
- [ArduPilot SiK advanced configuration and air-rate guidance](https://ardupilot.org/copter/docs/common-3dr-radio-advanced-configuration-and-technical-information.html)
- [SiK upstream discussion of sustained-transfer flow control](https://github.com/ArduPilot/SiK/pull/13)
- [Mosh design, usage, prediction, and UDP behavior](https://mosh.org/)
