# Charles River Solar Planning

This mission is a Charles River simulation for the PEARL solar planner. It keeps
the May test run app stack, but schedules five lawnmower survey blocks across a
forecast day followed by one deliberately infeasible evening survey to
demonstrate strategic give-up. It adds:

- `pPearlSunPlan` to dispatch waypoint work from reported SOC and a 24-hour
  irradiance forecast.
- `pSimSOC` to publish simulated `BATT_SOC` from `SOLAR_INPUT_FACTOR` and
  `NAV_SPEED`.
- Planner diagnostics including `SUNPLAN_DECISION`, `SUNPLAN_REASON`,
  `SUNPLAN_AVAILABLE_WH`, and `SUNPLAN_EXPECTED_CHARGE_WH`.

The example battery is configured from the PEARL battery label:

- LiFePO4, 12.8 V nominal
- 120 Ah
- 1536 Wh nominal capacity
- Recommended charge current up to 60 A, so the battery-side charge ceiling is
  roughly 14.4 V * 60 A = 864 W

The example solar model uses 160 W at full sun. That number is intentionally a
panel/controller placeholder below the battery charging limit; replace it after
calibrating real irradiance-to-charge gain.

The mission defaults to simulation. Launch target generation:

```sh
./launch.sh --just_make --nogui 5
```

Run the mission:

```sh
./launch.sh --nogui 10
```

The mission uses `forecast_mode = file` and reads `forecast_24h.csv`. To refresh
that file from Open-Meteo for tomorrow:

```sh
../../../scripts/fetch_solar_forecast.py \
  --lat 42.358436 \
  --lon -71.087448 \
  --day-offset 1 \
  --timezone America/New_York \
  --output forecast_24h.csv
```

`pPearlSunPlan` uses `forecast_start_hour = 0`, so mission hour 0 maps to
midnight in the Open-Meteo forecast file. The planner and SOC simulator use
MOOS time directly; use MOOS launch warp `60` for validation runs so each
planned survey window is long enough for the vehicle dynamics to settle while
still compressing a full-day scenario into a practical run.

After running the mission, graph the actual MOOS log:

```sh
./plot_run.py LOG_PEARL_*/LOG_PEARL_*.alog
```

This writes `solar_planning_run.png` from logged `BATT_SOC`,
`SOLAR_IRRADIANCE`, `SUNPLAN_SURVEY_ACTIVE`, and `SUNPLAN_DECISION`. The chart
shows battery energy in Wh, the irradiance forecast used during the run, the
active lawnmower windows, and the skipped survey marker. For the current demo
settings it should show five dispatch events followed by one `give_up`.
By default the plot stops 0.75 hours after the skipped survey so the planning
decision stays readable; use `--post-skip-hours -1` to include the full alog,
including any return-to-home energy draw after the planner gives up.

The simulated vehicle starts close to the first lawnmower block by default. All
tasks omit both `duration_h` and `cost_wh`, so `pPearlSunPlan` estimates
duration from path length and speed, then estimates energy with the original
solar-tracking model: `power_w = 40.5 * speed^2.97 + 6.513`. `pSimSOC` uses that
same speed-power form against logged `NAV_SPEED`, so SOC changes come from
motion and solar input rather than a separate task-active load. The first five
paths are 3 km each. The final configured task is a deliberate reserve
stress-test with a roughly 35 km path; under the same model at `speed=2.2`, this
is about a 1900 Wh task. The planner should publish
`SUNPLAN_DECISION=give_up` and command `RETURN=true` when that task becomes due.
