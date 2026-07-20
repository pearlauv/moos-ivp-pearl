# briggs_test

`briggs_test` is a single-drone operator mission for the lower-left grass field
in the Briggs map. It has three launch modes: a self-contained MOOS-IvP
simulation, ArduPilot SITL through `pArduBridge`, and real ArduCopter hardware
through `pArduBridge`. The operator chooses every leg; each LEG/HOME button
automates only the ordered Copter handoff into Helm guidance.

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
`42.3557024671, -71.1010163059`. For real operation, place the landing target
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
motion on the map. Each LEG/HOME button activates its corresponding Helm
behavior directly, so the route remains operator-selected and no multi-leg
sequence is encoded.

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
3. Press `LEG 1`, `LEG 2`, and `LEG 3` as desired. Each click posts the selected
   target, enables Helm guidance internally, and starts that leg. In SITL/real
   mode, verify `UAV_COMMAND_RESULT` reports
   `detail=MOOS_GUIDANCE` followed by `detail=TOWAYPT_UPDATE_POSTED`, then verify
   `AUTOPILOT_MODE=HELM_TOWAYPT` and live `DESIRED_HEADING`, `DESIRED_SPEED`,
   and `DESIRED_ALTITUDE` values.
4. Press `HOME` to fly the fixed home leg through the same Helm path.
5. At each capture, a Helm station-keep behavior holds the Copter at the
   endpoint. No Helm re-enable or separate GO action is required for the next
   leg.
6. On real hardware, confirm `UAV_LANDING_TARGET_AVAILABLE=1` and a fresh
   `UAV_LANDING_TARGET_AGE`
7. Press `PREC LAND`.

`DISARM`, `FC LOITER`, `PREC OFF`, `VIZ HOME`, and `RTL (FC)` remain available
as explicit operator controls; they are not additional steps in the normal
leg workflow. `PREC LOITER` is an optional acquisition/hold control, not a
prerequisite for `PREC LAND`. After `FC LOITER`, the next LEG/HOME click resumes
Helm guidance internally.

The labels are intentionally similar across modes, but their underlying
postings differ:

- `--sim` LEG/HOME buttons select and activate the corresponding fixed waypoint
  behavior in one click.
- `--sitl` and `--real` LEG/HOME buttons post `NEXT_WAYPOINT` and trigger a
  short vehicle-side sequence that enables the Helm, requests
  `FLY_WAYPOINT`, and activates the waypoint behavior after its update arrives.
- With the helm enabled, `pArduBridge` converts the selected target to
  `TOWAYPT_UPDATE`; `pHelmIvP` then produces desired heading, speed, and
  altitude values that `pArduBridge` sends to ArduPilot.

The pMarineViewer `Variable` selector includes command result, armed state,
landed state, altitude, landing-target availability/age, autopilot and helm
state, `TOWAYPT_UPDATE`, the three `DESIRED_*` guidance values, and process-watch
health. In MOOS-only simulation, `NAV_ALTITUDE` is representative button state
because vertical dynamics are not modeled. In SITL and real mode, the vehicle
community bridges measured `NAV_ALTITUDE` from `pArduBridge`.

| Button | MOOS-only `--sim` | `--sitl` and `--real` |
| --- | --- | --- |
| `ARM` | Publishes representative armed/mode state. | Requests arming through `pArduBridge`. |
| `DISARM` | Parks the helm and publishes disarmed/on-ground state. | Requests disarming; the bridge requires fresh `ON_GROUND` telemetry. |
| `TAKEOFF` | Releases manual override, deploys the helm in station keep, and publishes 8 m simulated altitude state. Vertical flight is not modeled. | Requests an ArduCopter takeoff to the configured 8 m AGL altitude. |
| `LEG 1` | Activates the Helm leg to `(-197,-140)`. | Sends the corresponding target through `pArduBridge` and starts Helm guidance. |
| `LEG 2` | Activates the Helm leg to `(-196,-162)`. | Sends the corresponding target through `pArduBridge` and starts Helm guidance. |
| `LEG 3` | Activates the Helm leg to `(-160,-152)`. | Sends the corresponding target through `pArduBridge` and starts Helm guidance. |
| `HOME` | Activates the Helm leg to fixed mapped home at `(-167,-131)`. | Sends the fixed home target through `pArduBridge` and starts Helm guidance. |
| `FC LOITER` | Activates station keep centered on the vehicle's current position. | Requests native flight-controller Loiter. |
| `PREC LOITER` | Activates station keep centered on fixed mapped home. | Requests native FC Loiter plus the precision-loiter auxiliary function. |
| `PREC OFF` | Leaves precision station keep. | Disables the precision-loiter auxiliary function; select a LEG/HOME action to resume Helm travel. |
| `PREC LAND` | Parks the helm and publishes landed/disarmed simulated state. | Requests ArduCopter Land through the bridge's `AUTOLAND` command. |
| `VIZ HOME` | Draws the fixed mapped home point. | Requests visualization of the flight controller's recorded home. |
| `RETURN HOME` / `RTL (FC)` | Flies the helm to fixed mapped home. | Parks Helm guidance and requests flight-controller Return-to-Launch using its recorded home. |

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

Unlike the Plane-oriented base examples, this mission configures
`pArduBridge` with `vehicle_type=copter`. Waypoint capture therefore changes
from travel guidance to Helm-controlled station keeping, while FC Loiter,
Copter Land, and RTL remain explicit flight-controller actions.

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
