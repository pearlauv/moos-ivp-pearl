# July 30 demo flight logs

This directory contains the two ArduCopter DataFlash recordings from the July
30 demo tests. The files were downloaded directly from the Pixhawk without
changing parameters or deleting the onboard copies.

| Repository file | Pixhawk log | Size | SHA-256 |
| --- | ---: | ---: | --- |
| `demo_flight_1.BIN` | 43 | 9,688,533 bytes | `a578903c0b4085c7fedc1e36ebff9a8ddfb7535caa8e20a36fb729ecbfe31e0b` |
| `demo_flight_2.BIN` | 44 | 3,206,828 bytes | `140f0cc3cdf4356d3a8a46676d78474790e6590b1d8be4ead4989d432cba0840` |

## Important recording detail

ArduPilot did not create one file per arming. `demo_flight_1.BIN` contains two
substantial armed flights and one earlier four-second arm/disarm cycle with no
takeoff. `demo_flight_2.BIN` contains one substantial armed flight. This
accounts for the three remembered tests: two flights without a precision-
landing target acquisition, followed by one successful acquisition and
landing.

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

The timelines were reconstructed directly from the original DataFlash `ARM`,
`MODE`, `EV`, `ERR`, `CTUN`, `BAT`, `PARM`, and `PL` records using `pymavlink`.
Mode reasons were decoded from the matching ArduPilot source (`RC_COMMAND=1`,
`GCS_COMMAND=2`, and `RADIO_FAILSAFE=3`).

These Pixhawk logs establish the flight-controller sequence and whether the
landing target was acquired. They do not contain the complete shoreside route
buffer, mouse-click, pMediator, or MOOS Helm history. Exact waypoint selection,
delivery, and behavior transitions require the matching original MOOS `.alog`
files if those are available.
