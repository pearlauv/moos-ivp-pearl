# aug_pearl_uav

`aug_pearl_uav` is a two-vehicle Charles River mission for a generic UAV and
the PEARL surface vehicle. It combines the qualified UAV operator workflow from
`uav_solo/briggs_test` with PEARL's hardware and Helm stack from
`may_test_run`.

The UAV may still fly operator-selected routes. The additional `RENDEZVOUS`
workflow is deliberately small: after the UAV is airborne, PEARL chooses one
meeting point, the UAV validates it, both vehicles travel there, PEARL grants
landing clearance, and the UAV requests precision landing.

## Implemented rendezvous state machine

PEARL is treated as having negligible turn radius. The coordinator therefore
chooses a point on the straight line between the current vehicle positions.
Its fractional distance from PEARL is:

```text
pearl_speed / (pearl_speed + uav_speed)
```

This equalizes nominal arrival times using the configured speeds. It is a
demonstration policy, not a current-aware or obstacle-aware planner.

| State | Action and transition |
| --- | --- |
| `IDLE` | Wait for an airborne operator request. |
| `REQUESTING` | UAV sends position, speed, health, and available battery data. PEARL proposes one session-tagged point. UAV accepts only after its complete one-point route is staged locally. |
| `RENDEZVOUS` | PEARL activates its dedicated rendezvous waypoint and the UAV follows its existing Helm route. Both must remain inside the 5 m capture zone for the configured dwell. |
| `LANDING` | PEARL enters StationKeep and sends session-tagged clearance. The UAV requests precision landing only after both clearance and its own arrival are true. |
| `COMPLETE` | UAV reports landed; both coordinators publish completion. |
| `ABORT` | Cancel the UAV route, put the UAV in its configured hold, and put PEARL in StationKeep. |

The UAV must have fresh navigation and, outside basic SIM, must be armed,
airborne, and healthy before it may start. PEARL rejects stale navigation,
unhealthy requests, malformed proposals, low known battery, and timeouts.
Rendezvous-level battery reserve enforcement remains disabled; takeoff has the
separate gate described below.

## Reliable versus periodic transport

`pMediator` carries the actions that must survive an occasional dropped UDP
datagram:

- operator ARM, DISARM, TAKEOFF, PREC LAND, route DEPLOY, route CLEAR,
  RENDEZVOUS, and RNDV ABORT;
- UAV request, PEARL proposal, UAV acceptance/completion, and PEARL landing
  clearance;
- acknowledgements and retries at one-second mission-time intervals, up to ten
  retries.

Mouse clicks remain shoreside-local until DEPLOY sends one complete route
snapshot. Periodic state such as node reports, health, rendezvous status, and
navigation uses ordinary broker/pShare routing. RealmCast bridging is disabled.

UAV and PEARL pMediator envelopes also use a direct unicast pShare route on
their shared vehicle network. The existing shoreside relay remains active as a
redundant path. Both paths carry the same message ID, so pMediator acknowledges
the first arrival and suppresses duplicate execution.

PEARL and the UAV intentionally use different platform groups, so
`uFldNodeComms` group filtering is disabled for this mission; otherwise PEARL
would not receive the UAV reports required to grant clearance.

## Operator interface

UAV controls:

- `UAV ARM`, `UAV DISARM`, `UAV TAKEOFF`
- `UAV DEPLOY`, `UAV CLEAR`, `UAV PREC LAND`
- `UAV TO PEARL`

Each `UAV TAKEOFF` request is consumed on the UAV and approved only when all
three fresh, valid inputs pass:

- UAV battery SOC is at least 30 percent;
- PEARL battery SOC is at least 15 percent; and
- apparent wind over PEARL is no more than 4 m/s.

The gate publishes `UAV_TAKEOFF_GATE_READY`, `UAV_TAKEOFF_GATE_REASON`, and a
per-request `UAV_TAKEOFF_RESULT`. A rejected request cannot remain pending and
cause a later takeoff. SIM supplies 80-percent batteries and 2 m/s wind so the
normal operator sequence remains immediately usable; focused tests may poke
the same variables to exercise each rejection.

Choose the `route` mouse context and click an ordered route. `UAV DEPLOY` sends
the stored snapshot. `UAV TO PEARL` instead sends a one-point snapshot of
PEARL's most recent reported position; it is not continuous pursuit.

PEARL controls:

- `PEARL DEPLOY`, `PEARL RETURN`, `PEARL STATION`, `PEARL ALLSTOP`

Coordinated controls:

- `RENDEZVOUS`: request a meeting after the UAV is armed and airborne.
- `RNDV ABORT`: independently sends acknowledged aborts to both vehicles.

The normal sequence is:

1. ARM and TAKEOFF the UAV.
2. Press `RENDEZVOUS`.
3. Monitor `UAV_RENDEZVOUS_STATE` and `PEARL_RENDEZVOUS_STATE`.
4. Let the coordinated workflow request precision landing, or press
   `RNDV ABORT` to hold both vehicles.

## SIM landing attachment

SIM starts with the UAV visually attached to PEARL. Once fresh PEARL pose data
arrives, `uSimAttachment` disables the UAV's `uSimMarineV22` navigation output
and publishes the PEARL position, heading, and speed as the UAV navigation
solution. The two icons therefore move together without running two competing
UAV navigation publishers.

An approved SIM takeoff detaches the UAV automatically. The app captures one
final platform position and heading, resets `uSimMarineV22` to that exact pose,
and then returns navigation ownership to the simulator. The UAV therefore
resumes independent flight from the takeoff point without jumping. SIM
precision landing reattaches it, and the cycle may repeat.

The app interface is vehicle-name agnostic: it consumes generic attachment
pose and request variables. This mission maps PEARL navigation into that
interface and aliases the generic status outputs to `UAV_SIM_ATTACHED` and
`UAV_SIM_ATTACHMENT_STATE` for operator display.

If the attachment pose becomes stale, the UAV freezes at the last received
pose and reports `ATTACHED_POSE_STALE`. While detached, the app reasserts local
simulator ownership so restarting the attachment helper cannot leave the UAV
simulator disabled.

This visual attachment is SIM-only. SITL and REAL continue to use flight-
controller navigation, do not launch `uSimAttachment`, and do not forward the
PEARL attachment-pose stream.

## Layout and launch modes

- Map: `MIT_SP.tif`
- Datum: `42.358436, -71.087448`
- Shoreside: MOOSDB `9000`, pShare `9200`
- UAV: MOOSDB `9001`, pShare `9201`
- PEARL: MOOSDB `9002`, pShare `9202`

The top-level launcher is for same-host testing:

```bash
./launch.sh --mode=SIM
```

Modes are:

- `SIM`: UAV SIM and PEARL SIM
- `SITL`: UAV ArduCopter SITL and PEARL SIM
- `REAL`: UAV REAL and PEARL REAL

Generate targets without launching:

```bash
./launch.sh --mode=SIM --just_make --nogui 5
```

For SITL, start ArduCopter first and then start the mission:

```bash
./launch_sitl.sh --no_rebuild
./launch.sh --mode=SITL
```

The included precision target matches the default SITL UAV home and default
PEARL start. If those positions change, update `SIM_PLD_LAT` and
`SIM_PLD_LON` in `sitl.parm` before treating precision-landing results as
meaningful.

Field operation uses the sublaunchers independently:

The deployed Alfa topology is routed through Sherlock rather than attaching
both radios directly to the two mission computers. See
[`RADIO_BACKHAUL.md`](RADIO_BACKHAUL.md) for provisioning, routes, activation,
and verification.

```bash
# Shoreside
./launch_shoreside.sh --auto --mode=REAL \
  --ip=100.127.231.65 \
  --uav_relay_ip=100.70.189.91 --uav_relay_pshare=9201

# UAV vehicle computer
./launch_uav.sh --auto --mode=REAL \
  --ip=100.70.189.91 \
  --shore=100.127.231.65 --shore_pshare=9200 \
  --pearl_ip=192.168.88.253 --pearl_pshare=9202

# PEARL vehicle computer
./launch_pearl.sh --auto --mode=REAL \
  --ip=100.69.111.61 \
  --shore=100.127.231.65 --shore_pshare=9200 \
  --uav_ip=172.22.90.2 --uav_pshare=9201
```

PEARL REAL mode also runs `iSherlockTelemetry`. By default it reads Sherlock's
Telegraf endpoint over the direct PEARL Ethernet network at
`192.168.88.252:9273`. Override that independently when needed:

```bash
./launch_pearl.sh --auto --mode=REAL \
  --sherlock_metrics_host=100.64.194.59 --sherlock_metrics_port=9273
```

The interface publishes PEARL battery state, apparent wind, and Alfa backhaul
health. Wind speed means apparent wind over the moving PEARL deck, which is the
airflow relevant to UAV landing. The mission does not currently publish a
second apparent-wind alias or true wind because neither adds a new input to the
present landing decision.

Consumers must also check `PEARL_BATTERY_DATA_VALID` or
`PEARL_WIND_DATA_VALID`. `PEARL_BATTERY_DATA_AGE` is the CMP sample age;
`PEARL_AIRMAR_DATA_AGE` is the age of the latest valid sentence from the Airmar
feed. The current Airmar exporter does not provide a wind-sentence-specific
age, so wind validity combines the latest MWV validity status with overall
Airmar feed freshness.

Sherlock polls the configured UAV station every two seconds. `ALFA_LINK_UP`
reports association independently from `ALFA_DATA_VALID`, which reports that
the collector itself is healthy and fresh. When linked, the interface also
publishes `ALFA_SIGNAL_DBM`, `ALFA_SIGNAL_AVG_DBM`,
`ALFA_TX_BITRATE_MBPS`, `ALFA_RX_BITRATE_MBPS`,
`ALFA_TX_RETRIES_TOTAL`, `ALFA_TX_FAILED_TOTAL`, and `ALFA_INACTIVE_MS`.
TX is Sherlock-to-UAV and RX is UAV-to-Sherlock; signal is the UAV signal as
received by Sherlock. Retry and failure values are cumulative since the
current association. `ALFA_DATA_AGE` is the age of the latest collector
observation as measured on Sherlock, and `ALFA_STATION_COUNT` helps detect an
unexpected client. Since MOOSDB retains the last numeric values after a link
drops, consumers must require `ALFA_SIGNAL_DATA_VALID=1` before interpreting
signal or bitrate. Sherlock's collector and Telegraf endpoint both update on a
two-second interval.

Selected battery, wind, and Alfa values are bridged to shoreside as ordinary
latest-state telemetry, not mediated commands. PEARL's wildcard `pLogger`
records every published Alfa variable in the mission `.alog`.

Battery, wind, and Alfa telemetry remain in one interface app because Sherlock
exposes them through the same HTTP endpoint. Splitting the app would download
the same roughly 180 KiB metrics document repeatedly and would not isolate
endpoint failure. The app instead validates and retains all three source
snapshots independently: one missing source does not suppress the others.

In SITL and REAL, `pArduBridge` publishes the flight controller's primary
battery estimate as `UAV_BATTERY_SOC` and `UAV_BATTERY_VOLTAGE`, together with
`UAV_BATTERY_DATA_VALID` and `UAV_BATTERY_DATA_AGE`. The SOC estimate is used
only when it is finite and no more than three seconds old.

All applications connect to their same-host MOOSDB through
`ServerHost=localhost`. Each sublauncher's `--ip` is its advertised pShare
address; `--shore` is independently the shoreside address reachable from that
vehicle. `--pearl_ip` and `--uav_ip` are the other vehicle's directly reachable
addresses through the routed backhaul. Supplying them enables the direct
pShare outputs;
omitting them leaves mediated traffic on the ordinary vehicle-to-shoreside
broker path. The direct addresses and ports are launch-time inputs, not
hardcoded Wi-Fi configuration.

The preferred traffic paths are:

| Traffic | Addressing | Physical path |
| --- | --- | --- |
| UAV to/from PEARL mission traffic | `172.22.90.2` and `192.168.88.253` | Alfa to Sherlock to PEARL Ethernet; no Tailscale or internet required |
| Shoreside mission traffic | Tailnet `100.x` addresses | Tailscale over the available underlay |
| SSH and administration | Tailnet `100.x` addresses | Tailscale over the available underlay |

A Tailscale address is a logical endpoint, not a guarantee that packets visit
the public internet. Tailscale normally uses a direct UDP path when possible;
that path may remain on a mutually reachable local network or use Sherlock's
upstream internet connection. If direct traversal fails, Tailscale may use a
DERP relay. At deployments where Sherlock's upstream is Starlink, remote
shoreside and management traffic may therefore cross Starlink. Explicit
PEARL/UAV peer traffic does not.

PEARL's managed UDP relays on ports `9201` and `9300` remain enabled as a
fallback for operation without UAV Tailscale. They are not used by the
preferred commands above. Normally allow UDP ports `9200`-`9202`; the fallback
also requires `9300`. Disable wireless client isolation.

## Validation performed

- Generated SIM, SITL, and REAL targets and standard static/port checks.
- Normal SIM rendezvous from the default starts through landing and completion.
- Extended SIM rendezvous from 283 m initial separation, proving UAV report
  freshness beyond the timeout interval.
- Mid-transit operator abort, including UAV route clear/hold and PEARL
  StationKeep.
- Start while disarmed rejected without movement.
- SIM with 50% `uFldNodeComms` message loss; the workflow completed after
  observed pMediator retransmissions.
- Bidirectional direct pMediator delivery in SIM with shoreside relay stopped,
  plus bidirectional shoreside-relay delivery with direct routes omitted.
- Live ArduCopter SITL telemetry and direct UAV-to-PEARL mediation with the
  shoreside relay stopped. REAL targets were generated and checked with custom
  vehicle, peer, and shoreside addresses.
- ArduCopter SITL arm, takeoff to 8 m, Guided Helm transit, endpoint hold,
  coordinated clearance, Land, touchdown, and disarm.
- Three-computer dock SIM across shoreside, PEARL, and the UAV Pi over the
  Tailscale-to-PEARL relay and Alfa link. A shore route command reached the
  UAV, both simulated vehicles moved, rendezvous session
  `uav_1786659149728036` completed at `(3.32,-5.63)`, and the simulated UAV
  finished disarmed, on ground, at zero altitude. Isolated ports were used and
  no hardware interface application was launched.
- Three-computer headless SIM using the preferred Tailscale address advertisements
  and direct Alfa peer addresses. Shoreside saw both nodes, process watch was
  healthy, and a shoreside route was accepted by and moved the simulated UAV.
  No arm request or hardware interface application was launched.
- On 2026-08-19, the deployed Sherlock collector reported a healthy AP with
  no UAV associated. An isolated PEARL MOOS test on port 19402 published
  `ALFA_LINK_UP=0`, `ALFA_DATA_VALID=1`, `ALFA_STATION_COUNT=0`, and fresh
  `ALFA_DATA_AGE`; wildcard pLogger logging recorded all four in an `.alog`.
  No helm, actuator, or vehicle hardware process was launched.
- The automated Alfa telemetry integration test covers connected,
  disconnected, collector-failed, and stale samples using a temporary MOOSDB:
  `python3 src/iSherlockTelemetry/tests/test_alfa_telemetry.py`.
- On 2026-08-20, all three communities ran concurrently in REAL mode over the
  close-range physical Alfa link. Bidirectional packet tests had zero loss;
  the PEARL log recorded -52 to -40 dBm, zero TX retries/failures, and actual
  battery/wind delivery to the UAV. Telemetry loss rejected takeoff as stale,
  a shoreside takeoff message was acknowledged then rejected on the actual
  low PEARL battery, and a shoreside rendezvous request safely aborted on
  missing Pixhawk navigation. After PEARL charged to its real 15% threshold,
  the gate reached READY with only the disconnected UAV battery synthesized.
  No flight-controller command was posted. See
  [`FIELD_TEST_2026-08-20.md`](FIELD_TEST_2026-08-20.md).

## PEARL stack

REAL mode retains the `may_test_run` setup:

- `iDualGPS`
- `iPEARL`
- `pPearlPID`
- `iBlueRoboticsPing`
- `iSherlockTelemetry`
- `pEchoVar` sensor-to-NAV translations

SIM mode uses `pHelmIvP`, `pMarinePIDV22`, and `uSimMarineV22`.

## Cleanup

```bash
./clean.sh
```
