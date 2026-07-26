# briggs_test

`briggs_test` is a single-drone operator mission for the Briggs field. It
supports a self-contained MOOS-IvP simulation, ArduPilot SITL through
`pArduBridge`, and real ArduCopter hardware through `pArduBridge`.

The operator builds an ordered route directly in pMarineViewer. `pRouteBuffer`
keeps those clicks shoreside and submits the complete route through
`pMediator` when DEPLOY is pressed. There are no hardcoded mission waypoints,
home waypoint, or FC RTL/Loiter controls in the normal route interface.

## Layout

- Map: `../../images/briggs_test/briggs_test.tif`
- Datum: `42.3569186052, -71.0990291116`
- Vehicle: `briggs`
- Route altitude in SITL/real mode: 8 m AGL by default
- Route endpoint in SITL/real: native ArduCopter Guided 3D hold

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
CLEAR transfers the captured current latitude, longitude, and altitude to
ArduCopter as a native Guided position hold. The bridge also captures and
holds the current yaw, so the vehicle does not keep chasing a Helm course.

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
   The pending route exists only in the shoreside `pRouteBuffer`, so individual
   clicks do not cross the vehicle link.
3. Press `ARM`, then `TAKEOFF`.
4. Press `DEPLOY` once to traverse the complete selected route.
5. At the final waypoint, the route is discarded. SIM enters passive Helm
   StationKeep; SITL/real transfers the captured position to native Guided
   hold.
6. Press `CLEAR` at any time to discard the selected/active route, clear its
   map markers, and hold the current position using the same mode-specific
   endpoint behavior.
7. Use `PREC LAND` only for the qualified precision-landing workflow.

Every mouse click updates the shoreside `pRouteBuffer`. DEPLOY serializes the
ordered point list into one full-route snapshot. `pMediator` gives that snapshot
an ordered message ID and resends it until the vehicle acknowledges delivery or
the configured retry limit is reached. The vehicle `pRouteBuffer` validates the
snapshot, updates one named dynamically spawned `BHV_Waypoint`, waits for
`ROUTE_READY`, and only then posts the local deploy trigger. Repeated DEPLOY
presses for an unchanged shoreside route are ignored.

In all modes, pHelmIvP starts in DRIVE with the route behavior idle. In
SITL/real, the local deploy sequence sends the established `FLY_WAYPOINT`
command while the Helm is on. This selects its Helm-guidance branch:
pArduBridge requests Guided mode and enters `HELM_TOWAYPT` without requiring or
duplicating a `NEXT_WAYPOINT`. The staged `BHV_Waypoint` remains the sole route
owner.

`CLEAR` clears the shoreside buffer and sends one mediated, repeatable
`action=clear` command. The vehicle expands that command into the existing
local `ROUTE_CLEAR=true` sequence, which cancels execution, empties the dynamic
waypoint list, and resets the clear request. Repeated CLEAR presses therefore
converge on the same empty-route state. Because both DEPLOY and CLEAR are
single ordered commands, their effects cannot be partially delivered as
independent route variables.

`ARM`, `DISARM`, `TAKEOFF`, and `PREC LAND` also cross the vehicle link as
single pMediator messages. Each button uses its own request variable so
pMediator ordering cannot allow a later command type to suppress an earlier
one. The vehicle expands the received request into the existing SIM or
ArduCopter actions and resets it locally for later reuse. pMediator confirms
remote message delivery; `UAV_COMMAND_RESULT` remains the authoritative
ArduPilot command-status path in SITL/real.

In SIM, the StationKeep behavior has a 1 m inner radius, 3 m outer radius, and
5 m hibernation radius. It permits passive drift within the hibernation zone
and restores the vehicle with Helm guidance outside it. In SITL/real, the route
endflag explicitly posts `ARDU_COMMAND=HOLD_POSITION`. `pArduBridge` captures
the current latitude, longitude, altitude, and yaw and refreshes
that native ArduCopter Guided target. `STATION_KEEP` remains a mission/Helm
concept used only by SIM; `pArduBridge` does not subscribe to it. A later
DEPLOY returns control to the Helm route.
Briggs uses `GUID_OPTIONS=4` so pilot yaw input cannot compete with the
autonomous yaw target while Guided owns the vehicle.

The live pMarineViewer emits a `HEARTBEAT` to the vehicle. After the first
heartbeat, twenty continuous seconds without another heartbeat causes
`pDeadManPost` to request native ArduPilot RTL through `pArduBridge`, bypassing
the Helm and any mission return waypoint. Headless launches do not arm the
dead-man until a heartbeat is explicitly posted. Vehicle AppCast terminal
reports are limited to one every two seconds to reduce radio load.
RealmCast auto-bridging is disabled in both field brokers to keep RealmCast
request/response traffic off the radio link. Each community still runs its
local `pRealm`, and the Pi's complete `pLogger` logs remain available directly
over Ethernet or from local storage.

## Buttons

| Button | `--sim` | `--sitl` and `--real` |
| --- | --- | --- |
| `ARM` | Publishes simulated armed state. | Requests arming through `pArduBridge`. |
| `DISARM` | Cancels route execution and publishes on-ground state without parking the Helm. | Requests manual override/RTL and disarming through `pArduBridge`; airborne disarm is rejected while RTL remains active. |
| `TAKEOFF` | Publishes the configured simulated altitude. | Requests Copter takeoff to the configured altitude. |
| `DEPLOY` | Activates the complete staged route. | Performs the Guided/Helm handoff, then activates the complete route. |
| `CLEAR` | Deletes the route and enters StationKeep. | Deletes the route and enters native Guided XYZ hold. |
| `PREC LAND` | Cancels route execution and publishes simulated landed/disarmed state without parking the Helm. | Requests Copter Land through `pArduBridge`. |

In SITL/real mode, `PREC LAND` cancels route traversal, parks pHelmIvP, and asks
ArduCopter to enter Land mode through `pArduBridge`. In SIM it only models the
corresponding landed state and does not simulate a landing target sensor.

For a split-host flight, launch the Pi vehicle subcommunity with `--auto`.
That suppresses the Pi-side uMAC; use the shoreside pMarineViewer as the only
interactive AppCast requester and close any extra uMAC/uMAC-like viewers.
The mission requires `pMediator` from `moos-ivp-swarm` and `pRouteBuffer` from
this repository to be built and available in `PATH` on both hosts.

## Generation and cleanup

```bash
./launch.sh --sim --just_make --nogui 5
./launch.sh --sitl --just_make --nogui
./launch.sh --real --just_make --nogui
./clean.sh
```
