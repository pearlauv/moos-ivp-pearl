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

## Radio settings

Both radios must have matching serial and RF settings. The currently qualified
pair uses:

```text
SERIAL_SPEED=57    # 57600 baud
AIR_SPEED=64
NETID=25
MAVLINK=0          # transparent serial mode
```

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
/dev/serial/by-id/usb-FTDI_FT231X_USB_UART_DU0E4L0Y-if00-port0 57600
noauth
local
nocrtscts
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
After=multi-user.target
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
/dev/cu.usbserial-DU0E55KZ 57600
noauth
local
nocrtscts
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
ssh pearl@10.0.0.2
```

The `noauth` option disables PPP-level PAP/CHAP authentication for this direct
link. SSH authentication and encryption still protect the SSH session.

## Running `b33_test` across PPP

The normal top-level `launch.sh` starts both communities on one computer. For
a split Pi/Mac mission, launch the two subcommunities separately.

On the Pi, start the simulated vehicle community:

```bash
cd ~/moos-ivp-pearl/missions/uav_solo/b33_test
./launch_vehicle.sh --mode=SIM --auto \
  --ip=10.0.0.2 --shore=10.0.0.1 \
  --mport=9001 --pshare=9201 --shore_pshare=9200
```

On the Mac, start shoreside:

```bash
cd ~/moos-ivp-pearl/missions/uav_solo/b33_test
./launch_shoreside.sh --mode=SIM --auto \
  --ip=localhost --mport=9000 --pshare=9200
```

Keep the shoreside `--ip` value at `localhost` on macOS so its local MOOS apps
connect to the local MOOSDB through the loopback route. The shoreside `pShare`
listener still binds UDP port 9200 on all interfaces, and the Pi reaches that
listener through the Mac's PPP address `10.0.0.1`. Thus cross-community data
uses PPP even though shoreside's own intra-community connections stay local.

The same network arguments apply to `--real`; only the vehicle-side
ArduPilot endpoint and operating mode change. Test with `--sim` first because
it validates PPP, MOOSDB, pShare, and the broker path without opening the
flight controller or commanding hardware.

Expected evidence of a working split mission includes:

- `ppp0` is `10.0.0.1` on the Mac and `10.0.0.2` on the Pi.
- Both PPP peers respond to `ping`.
- The Pi vehicle's `uFldNodeBroker` reports the shoreside route as connected.
- Shoreside receives the `b33` node report and vehicle appcasts.

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
