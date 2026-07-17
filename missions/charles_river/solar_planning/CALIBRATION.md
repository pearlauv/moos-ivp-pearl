# Solar Planning Calibration

Current mission settings:

| Value | Status | Setting | Evidence |
| --- | --- | --- | --- |
| SOC source | Set | `soc_var = BATT_SOC` | Should come from CMP BLE BMS SOC. |
| Battery capacity | Set | `battery_capacity_wh = 1536` | Battery label: 12.8 V * 120 Ah. |
| Reserve | Policy | `reserve_soc = 25` | Operator risk choice, not calibrated. |
| Idle/background draw | Roughly calibrated | `37.8 W` | Sherlock BMS data, 2026-06-11 to 2026-06-13. |
| Solar charge at no sun | Set | `charge_w_base = 0` | Reasonable baseline. |
| Full-sun charge gain | Placeholder | `charge_w_gain = 160` | Not calibrated; Sherlock showed near-zero charge. |
| Speed power curve | Placeholder | `40.5 * speed^2.97` | From solar-tracking example, not PEARL-calibrated. |
| Irradiance forecast | Set for demo | `demo_forecast_2026-06-12.csv` | Open-Meteo snapshot for deterministic simulation. |

The available Sherlock data can support SOC source, nominal capacity, and a
rough idle draw estimate. It cannot yet support irradiance-to-charge or
propulsion power calibration.
