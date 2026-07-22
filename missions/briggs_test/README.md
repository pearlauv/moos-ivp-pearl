# briggs_test

`briggs_test` is a single-drone operator mission for the lower-left grass field
in the Briggs map. It has three launch modes: a self-contained MOOS-IvP
simulation, ArduPilot SITL through `pArduBridge`, and real ArduCopter hardware
through `pArduBridge`. The operator chooses every leg; each LEG/HOME button
updates one waypoint behavior, with no automatic multi-leg sequence.

The top-level `launch.sh` is the local, two-community launcher. For a split
vehicle/shoreside deployment, run the two sublaunchers with their explicit host
arguments as documented in `RADIO_PPP.md`. The `--ip` value is also forced as
the address advertised by `pHostInfo`, so split operation cannot silently pick
a Wi-Fi, Ethernet, or Tailscale address. Intra-community processes always use
their local MOOSDB through `ServerHost=localhost`; `--ip` controls only the
externally advertised pShare identity.
If `--ip` is omitted, its default is the literal `localhost`. An empty,
malformed, or out-of-range IPv4 value is rejected instead of falling back to
interface autodiscovery.

## Layout

- Map: `../images/briggs_test/briggs_test.tif`
- Datum: `42.3569186052, -71.0990291116`
- Vehicle: `briggs`, rendered with the native `quadcopter` shape added by the
  companion MOOS-IvP feature branch
- Takeoff altitude: 8 m AGL
- Home / precision target: `x=-167, y=-131`
- Pattern: the four marked points `(-167,-131)`, `(-197,-140)`, `(-196,-162)`,
  and `(-160,-152)`

The fixed home point corresponds to approximately
`42.3557400134, -71.1010623410`. For real operation, place the landing target
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
motion on the map. Each LEG/HOME button updates one `BHV_Waypoint`, so the route
remains operator-selected and no multi-leg sequence is encoded.

## ArduPilot SITL

Start ArduCopter SITL in one terminal:

```bash
cd missions/briggs_test
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

In SITL, `pHelmIvP` supplies the waypoint course, speed, and altitude setpoints;
`pArduBridge` streams them to ArduCopter in Guided mode. Capturing the waypoint
hands control back to native flight-controller Loiter.

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

The normal visible workflow is:

1. `ARM`
2. `TAKEOFF`
3. Press `LEG 1`, `LEG 2`, and `LEG 3` as desired. In simulation, each click
   updates the single waypoint behavior. In SITL/real mode, each click posts
   `HELM_STATUS=true`, one local/geodetic `NEXT_WAYPOINT`, and
   `ARDU_COMMAND=FLY_WAYPOINT`; the bridge synchronizes the behavior with
   `TOWAYPT_UPDATE`. Verify that `UAV_COMMAND_RESULT` reports `MOOS_GUIDANCE`
   before issuing the next command.
4. Press `HOME` to fly the fixed home waypoint.
5. On real hardware, confirm `UAV_LANDING_TARGET_AVAILABLE=1` and a fresh
   `UAV_LANDING_TARGET_AGE`
6. Press `PREC LAND`.

`DISARM`, `FC LOITER`, `PREC OFF`, `VIZ HOME`, and `RTL (FC)` remain available
as explicit operator controls; they are not additional steps in the normal
leg workflow. `PREC LOITER` is an optional acquisition/hold control, not a
prerequisite for `PREC LAND`.

The labels are intentionally similar across modes, but their underlying
postings differ:

- `--sim` LEG/HOME buttons post `WPT_UPDATE` to one `BHV_Waypoint` behavior.
- `--sitl` and `--real` LEG/HOME buttons enable the Helm and post one
  `NEXT_WAYPOINT` containing matching local/geodetic coordinates plus
  `ARDU_COMMAND=FLY_WAYPOINT`. `pArduBridge` posts the synchronized
  `TOWAYPT_UPDATE` that drives the same waypoint behavior.

The pMarineViewer `Variable` selector includes command result, armed state,
landed state, altitude, landing-target availability/age, autopilot mode,
control authority, Helm state, waypoint update, the three desired guidance
values, and process-watch health. In MOOS-only simulation, `NAV_ALTITUDE` is
representative button state because vertical dynamics are not modeled. In SITL
and real mode, the vehicle community bridges measured `NAV_ALTITUDE` from
`pArduBridge`.

| Button | MOOS-only `--sim` | `--sitl` and `--real` |
| --- | --- | --- |
| `ARM` | Publishes representative armed/mode state. | Requests arming through `pArduBridge`. |
| `DISARM` | Parks the helm and publishes disarmed/on-ground state. | Requests disarming; the bridge requires fresh `ON_GROUND` telemetry. |
| `TAKEOFF` | Keeps horizontal motion stopped and publishes 8 m simulated altitude state. Vertical flight is not modeled. | Requests an ArduCopter takeoff to the configured 8 m AGL altitude. |
| `LEG 1` | Updates the Helm waypoint to `(-197,-140)`. | Enables Helm guidance to the corresponding target; capture switches to FC Loiter. |
| `LEG 2` | Updates the Helm waypoint to `(-196,-162)`. | Enables Helm guidance to the corresponding target; capture switches to FC Loiter. |
| `LEG 3` | Updates the Helm waypoint to `(-160,-152)`. | Enables Helm guidance to the corresponding target; capture switches to FC Loiter. |
| `HOME` | Updates the Helm waypoint to fixed mapped home at `(-167,-131)`. | Uses the same Helm waypoint path to fixed home; capture switches to FC Loiter. |
| `FC LOITER` | Stops horizontal motion with Helm all-stop. | Requests native flight-controller Loiter. |
| `PREC LOITER` | Sends the single waypoint behavior to fixed mapped home, then all-stops at capture. | Requests native FC Loiter plus the precision-loiter auxiliary function. |
| `PREC OFF` | Stops the simulated precision approach. | Disables the precision-loiter auxiliary function. |
| `PREC LAND` | Parks the helm and publishes landed/disarmed simulated state. | Requests ArduCopter Land through the bridge's `AUTOLAND` command. |
| `VIZ HOME` | Draws the fixed mapped home point. | Requests visualization of the flight controller's recorded home. |
| `RETURN HOME` / `RTL (FC)` | Flies the helm to fixed mapped home. | Requests flight-controller Return-to-Launch using its recorded home. |

All modes use one updateable `BHV_Waypoint` and no station-keeping behavior.
Every leg and home selection uses that behavior. SIM capture undeploys the Helm
and enters all-stop; SITL/real capture posts `LOITER_FC`, transferring control
from the Helm to native ArduCopter Loiter. `RTL (FC)` remains an independent
flight-controller return using its recorded home.

SITL's `PLND_TYPE=4` target is internal to ArduPilot and does not emit a
MAVLink `LANDING_TARGET` packet back to `pArduBridge`, so
`UAV_LANDING_TARGET_AVAILABLE` remains false in SITL mode. The simulated
precision backend is instead checked by its loaded parameters and observed
loiter/landing behavior.

`FC LOITER` is the immediate flight-controller hold button. `RTL (FC)` is an
independent flight-controller return action and is not part of the normal
precision-landing sequence. `DISARM` is accepted by `pArduBridge` only when
fresh landed-state telemetry reports `ON_GROUND`.

Unlike the Plane-oriented base examples, this mission configures
`pArduBridge` with `vehicle_type=copter`. FC Loiter, Copter Land, and RTL remain
explicit flight-controller actions.

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
