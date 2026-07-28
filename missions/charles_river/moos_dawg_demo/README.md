# moos_dawg_demo

`moos_dawg_demo` is a two-vehicle Charles River mission for a generic UAV and
the PEARL surface vehicle. It combines the qualified UAV
route-buffer/pMediator operator workflow from `uav_solo/briggs_test` with the
PEARL hardware and behavior stack from `may_test_run`. In that source mission,
"Briggs" names the field background; the aircraft community here is simply
`uav`.

The vehicles share the shoreside, map, datum, node reports, and operator
display. The UAV can traverse operator-selected routes or fly to a snapshot of
PEARL's latest reported position.

## Layout

- Map: `MIT_SP.tif`
- Datum: `42.358436, -71.087448`
- Shoreside: MOOSDB `9000`, pShare `9200`
- UAV: MOOSDB `9001`, pShare `9201`
- PEARL: MOOSDB `9002`, pShare `9202`

The top-level launcher uses one mission mode:

- `SIM`: UAV SIM and PEARL SIM
- `SITL`: UAV SITL and PEARL SIM
- `REAL`: UAV REAL and PEARL REAL

The default simulates both vehicles:

```bash
./launch.sh --mode=SIM
```

Generate targets without launching:

```bash
./launch.sh --mode=SIM --just_make --nogui 5
```

Exercise the UAV flight-controller integration while retaining simulated
PEARL:

```bash
./launch.sh --mode=SITL
```

Launch both hardware vehicles:

```bash
./launch.sh --mode=REAL
```

The top-level launcher is intended for same-host testing. Field operation uses
the sublaunchers independently so that each community advertises its own
network address:

```bash
# Shoreside
./launch_shoreside.sh --ip=<shore-ip> --mode=REAL

# UAV vehicle computer
./launch_uav.sh --auto --mode=REAL \
  --ip=<uav-ip> --shore=<shore-ip>

# PEARL vehicle computer
./launch_pearl.sh --auto --mode=REAL \
  --ip=<pearl-ip> --shore=<shore-ip>
```

All applications connect to a same-host MOOSDB through
`ServerHost=localhost`. A sublauncher's `--ip` controls only the address
advertised by `pHostInfo`; `--shore` independently controls the vehicle
broker's shoreside route.

## Operator interface

The UAV controls are:

- `UAV ARM`
- `UAV DISARM`
- `UAV TAKEOFF`
- `UAV DEPLOY`
- `UAV CLEAR`
- `UAV PREC LAND`
- `UAV TO PEARL`

Choose the `route` mouse context and click an ordered UAV route. As in
`briggs_test`, the clicks remain in the shoreside `pRouteBuffer`; DEPLOY sends
one full route snapshot through pMediator.

`UAV TO PEARL` replaces the pending UAV route with PEARL's most recently
reported X/Y position and sends that one-point route through the same mediated
path. Reports older than three seconds are rejected. This is a position
snapshot at button press, not continuous pursuit; the UAV holds at that point
after arrival.

Buttons 7–10 operate PEARL's May test-run behavior:

- `PEARL DEPLOY`
- `PEARL RETURN`
- `PEARL STATION`
- `PEARL ALLSTOP`

PEARL control variables are namespaced as `PEARL_*` across the field broker
and translated locally into the existing `DEPLOY`, `RETURN`,
`STATION_KEEP`, and `MOOS_MANUAL_OVERRIDE` behavior variables. They therefore
cannot accidentally activate the UAV.

`PEARL RETURN` means PEARL's original fixed return waypoint; it is distinct
from `UAV TO PEARL`.

## PEARL stack

REAL mode retains the `may_test_run` setup:

- `iDualGPS`
- `iPEARL`
- `pPearlPID`
- `iBlueRoboticsPing`
- `pEchoVar` sensor-to-NAV translations

SIM mode uses `pHelmIvP`, `pMarinePIDV22`, and `uSimMarineV22`.

## Cleanup

```bash
./clean.sh
```
