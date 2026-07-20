# b33_test

`b33_test` is a single-drone operator mission for the B33 map. It has three
launch modes: a self-contained MOOS-IvP
simulation, ArduPilot SITL through `pArduBridge`, and real ArduCopter hardware
through `pArduBridge`. The mission does not automate the flight sequence:
every leg and flight-mode change has its own pMarineViewer button.

## Layout

- Map: `../images/B33_Test/B33_Test.tif`
- Datum: `42.3598322041, -71.0931945226`
- Vehicle: `b33`, rendered with the native `quadcopter` shape added by the
  companion MOOS-IvP feature branch
- Takeoff altitude: 8 m AGL
- Home / precision target: `x=0, y=0`
- Pattern: three first-quadrant operator points `(35,20)`, `(65,55)`, and
  `(20,65)`

The fixed home point corresponds to approximately
`42.3598322041, -71.0931945226`. For real operation, place the landing target
there and verify the aircraft's physical launch location, fence, and flight
permissions before arming.

For split operation with the vehicle community on the Raspberry Pi and
shoreside on a Mac over the Holybro SiK radio pair, see
[`RADIO_PPP.md`](RADIO_PPP.md). It covers the persistent Pi service, manual Mac
startup, radio settings, PPP addresses, and the two-host mission commands.

## MOOS-IvP simulation

This is the default mode and does not require ArduPilot, MAVSDK, or an
autopilot connection:

```bash
./launch.sh --sim
```

The operator buttons drive a conventional `pHelmIvP` -> `pMarinePIDV22` ->
`uSimMarineV22` stack. `ARM`, `TAKEOFF`, and `PREC LAND` publish representative
drone state for the operator display; the simulator itself models the horizontal
motion on the map. Each leg button updates one helm waypoint, so the route is
still fully manual and no multi-leg sequence is encoded.

## ArduPilot SITL

Start ArduCopter SITL in one terminal:

```bash
cd missions/b33_test
./launch_sitl.sh
```

After SITL reports that the flight controller is ready, launch MOOS in another
terminal:

```bash
./launch.sh --sitl
```

`launch_sitl.sh` uses `~/ardupilot` by default. Override that with
`--ardupilot_root=/path/to/ardupilot` when needed. The supplied `sitl.parm`
enables ArduPilot's simulated precision-landing target at the marked home
point. It uses ArduPilot's direct TCP endpoint at `127.0.0.1:5760`, so MAVProxy
is not required. If the checkout already has a valid SITL build, pass
`--no_rebuild` to skip rebuilding it. A SITL-only helper supplies neutral
throttle only while native Copter Loiter is active so it holds altitude without
a joystick; it leaves all other modes untouched. It does not issue any mission
or flight-mode commands.
These settings are not used in MOOS-IvP simulation or real mode.

## Real hardware

The real-mode default is a Linux serial device at `ttyACM0:115200`:

```bash
./launch.sh --real
```

Override the endpoint when necessary:

```bash
./launch.sh --real --ap_url=ttyUSB0:57600 --ap_protocol=serial
```

The flight controller must already have a qualified Precision Landing backend:
`PLND_ENABLED=1`, the correct nonzero `PLND_TYPE`, sensor orientation/offsets,
and verified landing-target traffic. Complete the `pArduBridge` hardware
qualification procedure before powered flight.

## Operator buttons

Use the buttons deliberately. In SITL/real mode, wait for
`UAV_COMMAND_RESULT`; in MOOS simulation, wait for the vehicle to settle before
pressing the next leg:

1. `ARM`
2. `TAKEOFF`
3. `LEG 1`, `LEG 2`, `LEG 3`
4. `HOME`
5. `PREC LOITER`
6. On real hardware, confirm `UAV_LANDING_TARGET_AVAILABLE=1` and a fresh
   `UAV_LANDING_TARGET_AGE`
7. `PREC LAND`

The labels are intentionally similar across modes, but their underlying
postings differ:

- `--sim` buttons update helm variables such as `WPT_UPDATE`, `DEPLOY`, and
  `STATION_KEEP`; `PREC LAND` stops the helm and publishes the simulated landed
  state.
- `--sitl` and `--real` buttons post `ARDU_COMMAND`, `ARM_UAV`, and
  `NEXT_WAYPOINT` to `pArduBridge`.

The pMarineViewer `Variable` selector includes command result, armed state,
landed state, altitude, landing-target availability/age, autopilot mode, and
process-watch health. In MOOS-only simulation, `NAV_ALTITUDE` is representative
button state because vertical dynamics are not modeled. In SITL and real mode,
the vehicle community bridges measured `NAV_ALTITUDE` from `pArduBridge`.

| Button | MOOS-only `--sim` | `--sitl` and `--real` |
| --- | --- | --- |
| `ARM` | Publishes representative armed/mode state. | Requests arming through `pArduBridge`. |
| `DISARM` | Parks the helm and publishes disarmed/on-ground state. | Requests disarming; the bridge requires fresh `ON_GROUND` telemetry. |
| `TAKEOFF` | Releases manual override, deploys the helm in station keep, and publishes 8 m simulated altitude state. Vertical flight is not modeled. | Requests an ArduCopter takeoff to the configured 8 m AGL altitude. |
| `LEG 1` | Sends the helm to `(35,20)`. | Sends the corresponding GPS waypoint and requests `FLY_WAYPOINT`. |
| `LEG 2` | Sends the helm to `(65,55)`. | Sends the corresponding GPS waypoint and requests `FLY_WAYPOINT`. |
| `LEG 3` | Sends the helm to `(20,65)`. | Sends the corresponding GPS waypoint and requests `FLY_WAYPOINT`. |
| `HOME` | Sends the helm to fixed mapped home at `(0,0)`. | Sends the corresponding fixed GPS waypoint and requests `FLY_WAYPOINT`. |
| `FC LOITER` | Activates station keep centered on the vehicle's current position. | Requests native flight-controller Loiter. |
| `PREC LOITER` | Activates station keep centered on fixed mapped home. | Requests native FC Loiter plus the precision-loiter auxiliary function. |
| `PREC OFF` | Leaves station keep and resumes the currently loaded helm waypoint. | Disables the precision-loiter auxiliary function. |
| `PREC LAND` | Parks the helm and publishes landed/disarmed simulated state. | Requests ArduCopter Land through the bridge's `AUTOLAND` command. |
| `VIZ HOME` | Draws the fixed mapped home point. | Requests visualization of the flight controller's recorded home. |
| `RETURN HOME` / `RTL (FC)` | Flies the helm to fixed mapped home. | Requests flight-controller Return-to-Launch using its recorded home. |

In MOOS-only simulation, `FC LOITER` uses `BHV_StationKeep` centered on the
current position, while `PREC LOITER` uses a separate station-keeping behavior
centered on the fixed home/landing target. In SITL and real modes, they remain
distinct ArduPilot commands. Likewise, `RETURN HOME` in MOOS simulation targets
the fixed mapped home point, while `RTL (FC)` in SITL/real mode uses the flight
controller's recorded home.

SITL's `PLND_TYPE=4` target is internal to ArduPilot and does not emit a
MAVLink `LANDING_TARGET` packet back to `pArduBridge`, so
`UAV_LANDING_TARGET_AVAILABLE` remains false in SITL mode. The simulated
precision backend is instead checked by its loaded parameters and observed
loiter/landing behavior.

`FC LOITER` is the immediate flight-controller hold button. `RTL (FC)` is an
independent flight-controller return action and is not part of the normal
precision-landing sequence. `DISARM` is accepted by `pArduBridge` only when
fresh landed-state telemetry reports `ON_GROUND`.

## Generation and cleanup

Generate MOOS-IvP simulation targets without launching:

```bash
./launch.sh --sim --just_make --nogui 5
```

Generate ArduPilot SITL targets without launching:

```bash
./launch.sh --sitl --just_make --nogui
```

Generate real-mode targets without opening a serial connection:

```bash
./launch.sh --real --just_make --nogui
```

Remove generated targets, logs, and local SITL state:

```bash
./clean.sh
```
