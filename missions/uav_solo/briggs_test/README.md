# briggs_test

`briggs_test` is a single-drone operator mission for the Briggs field. It
supports a self-contained MOOS-IvP simulation, ArduPilot SITL through
`pArduBridge`, and real ArduCopter hardware through `pArduBridge`.

The operator builds an ordered route directly in pMarineViewer. There are no
hardcoded mission waypoints, home waypoint, or FC RTL/Loiter controls in the
normal route interface.

## Layout

- Map: `../../images/briggs_test/briggs_test.tif`
- Datum: `42.3569186052, -71.0990291116`
- Vehicle: `briggs`
- Route altitude in SITL/real mode: 8 m AGL by default
- Route endpoint: passive Helm StationKeep

The top-level `launch.sh` runs both communities locally. For a split
vehicle/shoreside deployment over the Holybro SiK PPP link, run the
sublaunchers as documented in [`RADIO_PPP.md`](RADIO_PPP.md).
`ServerHost=localhost` keeps each community local; a sublauncher's `--ip`
selects only its externally advertised pShare identity.

## MOOS-IvP simulation

```bash
./launch.sh --sim
```

This mode runs the conventional `pHelmIvP` -> `pMarinePIDV22` ->
`uSimMarineV22` horizontal-motion stack. Drone arming, takeoff altitude, and
precision-landing state are represented for the operator display; vertical
dynamics are not simulated.

## ArduPilot SITL

Start ArduCopter SITL:

```bash
./launch_sitl.sh
```

Then launch MOOS in another terminal:

```bash
./launch.sh --sitl
```

`launch_sitl.sh` uses `~/ardupilot` by default. Use
`--ardupilot_root=/path/to/ardupilot` when needed, and `--no_rebuild` when a
valid SITL binary already exists.

In SITL, `pHelmIvP` supplies route course, speed, and altitude setpoints.
`pArduBridge` streams them to ArduCopter in Guided mode. Route completion and
CLEAR both transfer horizontal guidance to passive Helm StationKeep while
ArduCopter remains in Guided mode.

## Real hardware

The real-mode default is `ttyACM0:115200`:

```bash
./launch.sh --real
```

Override the endpoint when required:

```bash
./launch.sh --real --ap_url=ttyUSB0:57600 --ap_protocol=serial
```

Complete the `pArduBridge` hardware qualification procedure before powered
flight. Precision landing also requires a qualified ArduPilot backend,
verified target traffic, and correct sensor orientation and offsets.

## Operator workflow

1. In pMarineViewer, choose the `route` left-click context.
2. Left-click the desired waypoints in traversal order. The yellow markers are
   a pending route; the aircraft does not move while the route is being built.
   Marker labels are intentionally hidden because pMarineViewer's mouse-click
   counter is global to the viewer and does not reset when a route is cleared.
3. Press `ARM`, then `TAKEOFF`.
4. Press `DEPLOY` once to traverse the complete selected route.
5. At the final waypoint, the route is discarded and passive StationKeep is
   centered at the captured position.
6. Press `CLEAR` at any time to discard the selected/active route, clear its
   map markers, and enter passive StationKeep at the current position.
7. Use `PREC LAND` only for the qualified precision-landing workflow.

Every mouse click updates one named, dynamically spawned `BHV_Waypoint`.
The first click starts the route; later clicks append to the same route. Route
completion or CLEAR removes that behavior instance, so the next click starts a
new list. In SITL/real mode, pHelmIvP starts in DRIVE with these behaviors idle
so it can retain every staged point before DEPLOY; ArduCopter does not transfer
to Helm guidance until DEPLOY performs the explicit handoff.

The StationKeep behavior has a 1 m inner radius, 3 m outer radius, and 5 m
hibernation radius. It permits passive drift within the hibernation zone and
restores the vehicle with Helm guidance outside it. StationKeep is locally
inhibited whenever `ROUTE_DEPLOY=true`, so it cannot compete with an active
route if a repeated shoreside state update is suppressed. In SITL/real mode,
`BHV_ConstantAltitude` continues commanding the configured altitude.

## Buttons

| Button | `--sim` | `--sitl` and `--real` |
| --- | --- | --- |
| `ARM` | Publishes simulated armed state. | Requests arming through `pArduBridge`. |
| `DISARM` | Parks horizontal guidance and publishes on-ground state. | Requests disarming through `pArduBridge`. |
| `TAKEOFF` | Publishes the configured simulated altitude. | Requests Copter takeoff to the configured altitude. |
| `DEPLOY` | Activates the complete staged route. | Performs the Guided/Helm handoff, then activates the complete route. |
| `CLEAR` | Deletes the route and enters StationKeep. | Deletes the route and enters Guided Helm StationKeep. |
| `PREC LAND` | Publishes simulated landed/disarmed state. | Requests Copter Land through `pArduBridge`. |

In SITL/real mode, `PREC LAND` cancels route traversal, parks pHelmIvP, and asks
ArduCopter to enter Land mode through `pArduBridge`. In SIM it only models the
corresponding landed state and does not simulate a landing target sensor.

## Generation and cleanup

```bash
./launch.sh --sim --just_make --nogui 5
./launch.sh --sitl --just_make --nogui
./launch.sh --real --just_make --nogui
./clean.sh
```
