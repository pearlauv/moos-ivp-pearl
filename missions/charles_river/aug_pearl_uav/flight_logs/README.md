# July 30 demo flight logs

This directory contains the two ArduCopter DataFlash recordings and all seven
original shoreside MOOS logs still present from the July 30 demo tests. Three
of the shoreside logs match the substantial flights summarized below. The
DataFlash files were downloaded directly from the Pixhawk without changing
parameters or deleting the onboard copies. The `.alog` files are byte-for-byte
copies of the original mission logs; no filtered or derived log was substituted.

| Repository file | Pixhawk log | Size | SHA-256 |
| --- | ---: | ---: | --- |
| `demo_flight_1.BIN` | 43 | 9,688,533 bytes | `a578903c0b4085c7fedc1e36ebff9a8ddfb7535caa8e20a36fb729ecbfe31e0b` |
| `demo_flight_2.BIN` | 44 | 3,206,828 bytes | `140f0cc3cdf4356d3a8a46676d78474790e6590b1d8be4ead4989d432cba0840` |

| Repository file | Original shoreside session | Size | SHA-256 |
| --- | --- | ---: | --- |
| `demo_flight_1_sortie_1a_shoreside.alog` | `09_46_23` | 3,267,201 bytes | `c26947909c6eb056101da83256458f23a44cf3ebaa8d4bf4c1b847038772641b` |
| `demo_flight_1_sortie_1b_shoreside.alog` | `09_52_07` | 13,780,175 bytes | `94b872e018808c1b6c1a03e9886801fbe5b980ce701287911b83d6b940708e7a` |
| `demo_flight_2_sortie_2a_shoreside.alog` | `11_19_37` | 8,374,203 bytes | `77cffafa1956e2d15292cd4ce93653a8144805889523f472d700939412c0f1d4` |

| Additional preserved session | Size | SHA-256 |
| --- | ---: | --- |
| `shoreside_session_09_17_29.alog` | 1,420,366 bytes | `d0244627ac2a65e2ec65c7668375ea9492a841a981660a086953cd699d1a5de5` |
| `shoreside_session_09_19_27.alog` | 513,352 bytes | `4ef5a1af6918e3588c0d665e5daad1992fbd0b3a894c36cd6a852f13cd3d0718` |
| `shoreside_session_09_27_52.alog` | 5,524,544 bytes | `df0d9fd05b59276a465ef5f636222be2d54d57d5c829647fcdb49cea6b64d298` |
| `shoreside_session_11_01_34.alog` | 925,925 bytes | `bf08df297268ea0f217ea7a2d5fd6962d4a8f89fa7dc58bffbc5bea9ef44c150` |

## Important recording detail

ArduPilot did not create one file per arming. `demo_flight_1.BIN` contains two
substantial armed flights and one earlier four-second arm/disarm cycle with no
takeoff. `demo_flight_2.BIN` contains one substantial armed flight. This
accounts for the three remembered tests: two flights without a precision-
landing target acquisition, followed by one successful acquisition and
landing.

The MOOS-to-DataFlash correlation is based on event timing, not merely file
order. In sortie 1B, MOOS recorded ARM acceptance at 97.12250 s and LAND
acceptance at 206.03004 s, an interval of 108.90754 s; DataFlash recorded LAND
108.910 s after arming. In sortie 2A, the corresponding interval was 29.38137 s
in MOOS and 29.377 s in DataFlash. MOOS first reported the landing target about
5.01 s after LAND in sortie 2A, while DataFlash recorded acquisition 5.033 s
after LAND.

Times below are seconds from the start of each armed interval. Altitudes are
the flight controller's `CTUN.Alt` estimate. `PL.TAcq=1` is the controller's
logged indication that the precision-landing target was acquired.

## Demo flight 1 (`demo_flight_1.BIN`)

### Sortie 1A: no target acquisition; radio-failsafe RTL

| Flight time | Event |
| ---: | --- |
| 0.000 s | Armed. The controller entered Loiter following a GCS-requested mode change. |
| 2.002 s | Motor interlock enabled. |
| 4.305 s | GCS requested Guided mode. |
| 4.315 s | ArduCopter became auto-armed. |
| 5.365 s | The landed state cleared. |
| 7.185 s | Estimated altitude first exceeded 0.5 m. |
| 7.485 s | Estimated altitude first exceeded 1.0 m. |
| 7.885 s | Estimated altitude first exceeded 2.0 m. |
| 50.307 s | A radio late-frame error and RC failsafe were logged. ArduCopter changed to RTL for `RADIO_FAILSAFE`. |
| 56.507 s | The RC failsafe was logged as resolved. |
| 60.985 s | Maximum estimated altitude: 14.588 m. |
| 82.110 s | An RC command changed the controller to Stabilize. |
| 86.685 s | Disarmed. |

Precision-landing result: all 2,080 `PL` samples reported `TAcq=0`. The target
was never acquired. This sortie never entered Land mode; RTL preempted the
flight. Consequently, this was not merely a failed tag-guided descent—the
controller records a radio-failsafe return instead of an accepted Land-mode
sequence.

Battery voltage recorded during the armed interval ranged from 23.61 V to
22.47 V.

The matching shoreside log adds the operator-side route sequence. Four points
were selected at `(32.8,-2.5)`, `(2.7,-15.9)`, `(25.9,-17.5)`, and
`(9.2,-5.3)`. DEPLOY was pressed at 184.45480 s; the vehicle reported the route
staged at 185.21036 s and accepted at 185.46061 s; Helm control became active
at 185.58982 s; and Guided mode was confirmed at 186.99401 s. Every one of the
1,582 logged `UAV_LANDING_TARGET_AVAILABLE` samples was zero. There is no LAND
request in this shoreside session. The radio-failsafe RTL cause is established
by the Pixhawk log; this shoreside schema did not report unsolicited flight-mode
changes and therefore does not independently expose that cause.

### Sortie 1B: Land mode, but no target acquisition

| Flight time | Event |
| ---: | --- |
| 0.000 s | Armed in Guided mode. |
| 2.001 s | Motor interlock enabled. |
| 4.520 s | ArduCopter became auto-armed. |
| 5.587 s | The landed state cleared. |
| 7.577 s | Estimated altitude first exceeded 0.5 m. |
| 7.877 s | Estimated altitude first exceeded 1.0 m. |
| 8.277 s | Estimated altitude first exceeded 2.0 m. |
| 48.777 s | Maximum estimated altitude: 6.880 m. |
| 108.910 s | GCS requested Land mode. |
| 161.342 s | An RC command changed the controller to Stabilize. |
| 178.877 s | Disarmed. |

Precision-landing result: all 4,289 `PL` samples reported `TAcq=0`. The target
was never acquired. No `LAND_COMPLETE` event occurred before the switch to
Stabilize and subsequent disarm.

Battery voltage recorded during the armed interval ranged from 23.31 V to
21.66 V.

The matching shoreside log shows three selected route points:
`(-3.3,-10.6)`, `(19.8,0.4)`, and `(7.4,-4.3)`. DEPLOY occurred at 113.90254 s,
followed by vehicle-side STAGED at 114.69744 s, DEPLOY_ACCEPTED at 114.94761 s,
and Helm activation at 115.06941 s. A `HOLD_POSITION` result at 142.15388 s
marks the route endpoint. LAND was submitted at 205.92985 s and accepted at
206.03004 s. All 9,312 target-availability samples in the original shoreside
log were zero, independently agreeing with DataFlash that no landing target was
accepted during this flight.

### Non-flight arm cycle

The same recording contains an earlier arm at recording-relative time
1,075.467 s followed by disarm 4.212 s later. It did not reach takeoff altitude
and is excluded from the flight summaries above.

## Demo flight 2 (`demo_flight_2.BIN`)

### Sortie 2A: target acquired and precision landing completed

| Flight time | Event |
| ---: | --- |
| 0.000 s | Armed in Guided mode. |
| 2.001 s | Motor interlock enabled. |
| 5.022 s | ArduCopter became auto-armed. |
| 6.139 s | The landed state cleared. |
| 7.342 s | Estimated altitude first exceeded 0.5 m. |
| 7.642 s | Estimated altitude first exceeded 1.0 m. |
| 8.242 s | Estimated altitude first exceeded 2.0 m. |
| 19.742 s | Maximum estimated altitude: 7.903 m. |
| 29.377 s | GCS requested Land mode. |
| 34.410 s | Precision-landing target acquired (`PL.TAcq` changed from 0 to 1), 5.033 seconds after entering Land. |
| 63.910 s | `PL.TAcq` returned to 0 near touchdown. The final acquired sample was at 63.870 s. |
| 65.412 s | `LAND_COMPLETE_MAYBE` logged. |
| 66.214 s | `LAND_COMPLETE` logged. |
| 66.722 s | Disarmed. |

Precision-landing result: 703 of 1,587 `PL` samples reported `TAcq=1`. The
continuous acquired interval lasted approximately 29.46 seconds and ended only
about 2.34 seconds before confirmed landing.

Battery voltage recorded during the armed interval ranged from 22.81 V to
22.01 V.

The matching shoreside log shows `UAV to PEARL` selected at 248.84231 s. The
route buffer serialized PEARL's then-current location as the single point
`(11.5,-5.6)`, reported it staged at 251.75116 s, and received the Guided-mode
acceptance at 253.03007 s. `HOLD_POSITION` was accepted at 260.25209 s. PREC
LAND was requested at 264.45153 s and LAND was accepted at 264.96694 s. MOOS
first reported `UAV_LANDING_TARGET_AVAILABLE=1` at 269.97930 s, briefly reported
zero at 270.68196 s, reacquired at 270.88265 s, and retained availability until
the last true sample at 296.95748 s. It changed to zero at 298.36176 s near
touchdown; the aircraft disarmed at 303.37914 s.

### Additional shoreside sessions

The four unmatched sessions are preserved because they still provide useful
bench and operator-history evidence, but they are not treated as flight logs:

- `09_17_29` contains one selected route point, `(30.9,-42.8)`, and no DEPLOY
  or UAV command result.
- `09_19_27` contains no route or UAV command result.
- `09_27_52` contains an on-ground ARM/DISARM exercise. Armed telemetry first
  became true at 83.62084 s; DISARM was accepted at 86.22965 s; an RTL result
  followed at 86.32993 s because that version of the DISARM action also invoked
  the manual-override/RTL path; and armed telemetry returned false at
  89.63931 s. There was no takeoff.
- `11_01_34` contains no route or UAV command result.

### pMediator observations

The preserved shoreside logs show that pMediator's retries supplied useful
redundancy and that return acknowledgements were not perfectly reliable:

- In sortie 1A, the TAKEOFF request was transmitted 12 times because its
  acknowledgement did not return, yet pArduBridge completed takeoff. The route
  snapshot was acknowledged after two transmissions.
- In sortie 1B, ARM, TAKEOFF, the route snapshot, and PREC LAND each produced a
  returned `ACK_MESSAGE`; the route was nevertheless transmitted twice before
  the first acknowledgement arrived.
- In sortie 2A, the one-point PEARL route was transmitted 12 times and exhausted
  the configured retry sequence without a returned acknowledgement, but the
  vehicle reported STAGED and then accepted Guided flight. This is direct
  evidence of a lost acknowledgement rather than a lost command. ARM, TAKEOFF,
  and PREC LAND also produced returned acknowledgements.

These observations do not provide a packet-loss percentage: pMediator retries
at a fixed interval, and a duplicate may already be queued when an
acknowledgement arrives. They also do not prove that any first forward command
was dropped or that a retry rescued it. They establish that the vehicle could
execute a command despite a missing return acknowledgement and that pMediator
continued retrying rather than treating silence as success.

## Precision-landing configuration

The relevant parameters embedded in both recordings are identical:

| Parameter | Logged value | Interpretation |
| --- | ---: | --- |
| `PLND_ENABLED` | 1 | Precision landing enabled |
| `PLND_TYPE` | 1 | MAVLink landing-target backend |
| `PLND_STRICT` | 2 | Strict retry/hold behavior |
| `PLND_TIMEOUT` | 4 s | Target-loss timeout |
| `PLND_ALT_MAX` | 4.999 m | Maximum acquisition/retry altitude |
| `PLND_ALT_MIN` | 0.75 m | Minimum retry altitude |
| `PLND_CAM_POS_X` | -0.12 m | Camera X offset |
| `PLND_CAM_POS_Y` | 0.00 m | Camera Y offset |
| `PLND_CAM_POS_Z` | 0.09144 m | Camera Z offset |
| `PLND_LAND_OFS_X` | 17 cm | Landing X offset |
| `PLND_LAND_OFS_Y` | 0 cm | Landing Y offset |
| `LAND_SPEED` | 50 cm/s | Final descent speed |
| `LAND_SPEED_HIGH` | 100 cm/s | Higher-altitude descent speed |

## Concise lessons and suggested changes

Observed lessons:

- Both unsuccessful sorties had `PL.TAcq=0`, and the entire recording retained
  `PL.LastMeasMS=0`, so ArduPilot never accepted a landing-target measurement.
  The log does not establish why. The target may simply have been outside the
  downward camera's field of view or not underneath the UAV; other possibilities
  include occlusion, unsuitable apparent tag size, lighting/exposure, detector
  failure, or failure to deliver a valid MAVLink measurement. `PL.Heal=1` only
  shows that the ArduPilot precision-landing backend initialized as healthy; it
  does not prove that the camera saw the target or that the complete detection
  pipeline was working.
- The successful sortie acquired the target near 4.8 m range and maintained
  tracking through most of the descent. After settling, its controller-
  estimated horizontal error was typically 0.10--0.25 m.
- The long low hover was mostly intentional final-descent slowing, not target
  loss. With `PLND_OPTIONS=1`, ArduCopter slows below about 2 m while correcting
  lateral error.
- `PLND_ALT_MAX=5` means missing-target handling begins below 5 m;
  `PLND_ALT_MIN=0.75` means the vehicle commits to vertical landing if the
  target is lost below 0.75 m; and `PLND_TIMEOUT=4` permits up to four seconds
  from the last valid target before retry/strict handling applies.
- The final fresh target measurement occurred at approximately 0.51 m range
  and about 4.4 seconds before `LAND_COMPLETE`. Because the landed aircraft was
  hanging partly over the platform edge, the final rangefinder readings do not
  provide a valid ground-clearance calibration or a reliable measurement of
  deck clearance.

Recommended controlled tests, not yet applied to the aircraft:

1. Change `PLND_OPTIONS` from `1` to `5` to retain moving-target support and
   enable the faster final descent. Initially retain `LAND_SPEED=50` cm/s.
2. Test `PLND_EST_TYPE=1` (Kalman filter) instead of raw mode (`0`) for better
   moving-platform velocity estimation. Initially retain
   `PLND_ACC_P_NSE=2.5`.
3. Measure exposure-to-Pixhawk latency before changing `PLND_LAG`. The current
   0.02 s value may be low; the existing FC log shows a 35 ms median and 66 ms
   95th-percentile corrected-measurement age, but that includes camera
   processing only if `LANDING_TARGET.time_usec` contains the frame-exposure
   timestamp.
4. Correlate future misses with vehicle/tag geometry and recorded camera output.
   First determine whether the tag was actually inside the camera's field of
   view. If it was visible but not detected, then investigate tag size,
   occlusion, glare, contrast, camera exposure/shutter, detector confidence, and
   MAVLink delivery rather than assigning the miss to sunlight alone.
5. Keep `PLND_STRICT=2` and `PLND_ALT_MIN=0.75` initially. Consider shortening
   `PLND_TIMEOUT` toward 2 s only after reliable acquisition is demonstrated.

For `PLND_OPTIONS`, the bit values are `1` for moving target, `2` for allowing
precision landing to resume after pilot repositioning, and `4` for maintaining
the faster final descent. Therefore `5` selects moving target plus fast descent;
it does not enable automatic re-engagement after pilot repositioning.

### Landing-accuracy expectations

The successful sortie's controller-estimated horizontal target error, after
initial convergence, had a median of approximately 0.16 m, a 95th percentile
of approximately 0.23 m, and an observed maximum of approximately 0.29 m. These
figures describe ArduPilot's target-relative estimate, not independently
surveyed touchdown error. The edge-hanging touchdown geometry prevents a useful
physical accuracy measurement from this test.

One successful landing is insufficient to establish an accuracy distribution.
For planning, treat roughly 0.10--0.30 m as the demonstrated tracking-error
scale when the target is acquired, while allowing substantially larger errors,
a hover/abort, or no precision landing when acquisition fails. Target
visibility, camera calibration and latency, wind, platform motion, and the
blind interval after the final valid measurement will all increase variability.
Establish operational accuracy with repeated landings on surveyed touchdown
marks under representative stationary, moving-platform, and lighting
conditions.

## Evidence and limitations

The DataFlash timelines were reconstructed from the original `ARM`, `MODE`,
`EV`, `ERR`, `CTUN`, `BAT`, `PARM`, and `PL` records using `pymavlink`. Mode
reasons were decoded from the matching ArduPilot source (`RC_COMMAND=1`,
`GCS_COMMAND=2`, and `RADIO_FAILSAFE=3`).

The shoreside analysis used only the three preserved original `.alog` files.
The principal commands were:

```sh
alogscan LOG.alog --nocolor
aloggrep LOG.alog ROUTE_POINT ROUTE_BUFFER_DEPLOY ROUTE_BUFFER_GOTO \
  ROUTE_BUFFER_STATE ROUTE_BUFFER_VEHICLE_STATE UAV_COMMAND_RESULT \
  AUTOPILOT_MODE UAV_LANDING_TARGET_AVAILABLE --tvv --sd -nc -nr
aloggrep LOG.alog MEDIATED_MESSAGE_LOCAL ACK_MESSAGE --tvv --sd -nc -nr
aloghelm LOG.alog -l -b -m --nocolor
```

`aloghelm` found no Helm life-event records in these shoreside logs, so the
analysis does not infer unlogged behavior-spawn transitions. It uses the
explicit route-buffer, pArduBridge, target-availability, mediation, and
acknowledgement records instead. The MOOS logs establish the operator commands,
route snapshots, and target-availability state received at shoreside; the
Pixhawk logs remain authoritative for FC mode reasons and the controller's
precision-landing estimator state.
