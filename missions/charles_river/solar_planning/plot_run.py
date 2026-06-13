#!/usr/bin/env python3
"""Plot the post-run SOC, irradiance, and active task windows from a MOOS alog."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt


SCRIPT_DIR = Path(__file__).resolve().parent
NUMERIC_VARS = {
    "BATT_SOC",
    "SOLAR_IRRADIANCE",
    "SUNPLAN_MISSION_HOURS",
    "SUNPLAN_REQUIRED_WH",
}
EVENT_VARS = {
    "SUNPLAN_DECISION",
    "SUNPLAN_REASON",
    "SUNPLAN_SURVEY_ACTIVE",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot post-run SOC, irradiance, and task windows from a MOOS alog."
    )
    parser.add_argument("alog", type=Path)
    parser.add_argument(
        "--output",
        type=Path,
        default=SCRIPT_DIR / "solar_planning_run.png",
    )
    parser.add_argument("--capacity-wh", type=float, default=1536.0)
    parser.add_argument("--reserve-soc", type=float, default=25.0)
    parser.add_argument(
        "--post-skip-hours",
        type=float,
        default=0.75,
        help="Hours to show after the first give_up decision. Use a negative value to plot the full alog.",
    )
    return parser.parse_args()


def parse_alog(path: Path):
    numeric = {var: [] for var in NUMERIC_VARS}
    events = {var: [] for var in EVENT_VARS}
    first_time = None

    with path.open(errors="replace") as file:
        for raw_line in file:
            if not raw_line or raw_line.startswith("%"):
                continue
            parts = raw_line.split(None, 3)
            if len(parts) < 4:
                continue
            try:
                timestamp = float(parts[0])
            except ValueError:
                continue

            var = parts[1]
            value = parts[3].strip()
            if first_time is None:
                first_time = timestamp
            rel_time_h = (timestamp - first_time) / 3600.0

            if var in numeric:
                try:
                    numeric[var].append((rel_time_h, float(value)))
                except ValueError:
                    pass
            elif var in events:
                events[var].append((rel_time_h, value))

    mission_time = numeric["SUNPLAN_MISSION_HOURS"]
    if mission_time:
        numeric = {
            var: remap_to_mission_time(series, mission_time)
            for var, series in numeric.items()
        }
        events = {
            var: remap_to_mission_time(series, mission_time)
            for var, series in events.items()
        }

    return numeric, events


def latest_before(series, time_h):
    latest = None
    for event_time, value in series:
        if event_time > time_h:
            break
        latest = value
    return latest


def remap_to_mission_time(series, mission_time):
    remapped = []
    mission_index = 0
    latest = None
    for rel_time_h, value in series:
        while mission_index < len(mission_time) and mission_time[mission_index][0] <= rel_time_h:
            latest = mission_time[mission_index][1]
            mission_index += 1
        if latest is not None:
            remapped.append((latest, value))
    return remapped


def unpack(series):
    return [point[0] for point in series], [point[1] for point in series]


def active_windows(events, end_time):
    windows = []
    start = None
    for time_h, value in events.get("SUNPLAN_SURVEY_ACTIVE", []):
        lowered = str(value).strip().lower()
        if lowered in ("true", "1", "on") and start is None:
            start = time_h
        elif lowered in ("false", "0", "off") and start is not None:
            if time_h > start:
                windows.append((start, time_h))
            start = None
    if start is not None and end_time > start:
        windows.append((start, end_time))
    return windows


def decision_events(events):
    decisions = []
    dispatch_count = 0
    for time_h, decision in events.get("SUNPLAN_DECISION", []):
        if decision == "dispatch":
            dispatch_count += 1
            decisions.append((time_h, f"T{dispatch_count}", decision))
        elif decision == "give_up":
            decisions.append((time_h, f"T{dispatch_count + 1} skip", decision))
    return decisions


def first_giveup_time(events):
    for time_h, decision in events.get("SUNPLAN_DECISION", []):
        if decision == "give_up":
            return time_h
    return None


def trim_series(series, end_time):
    return [point for point in series if point[0] <= end_time]


def value_at_or_before(series, time_h):
    return latest_before(series, time_h)


def main() -> int:
    args = parse_args()
    numeric, events = parse_alog(args.alog)

    soc_t, soc_pct = unpack(numeric["BATT_SOC"])
    irr_t, irradiance = unpack(numeric["SOLAR_IRRADIANCE"])
    if not soc_t:
        raise ValueError("BATT_SOC not found in alog")

    soc_wh = [value * args.capacity_wh / 100.0 for value in soc_pct]
    reserve_wh = args.reserve_soc * args.capacity_wh / 100.0
    end_time = max(soc_t[-1], irr_t[-1] if irr_t else soc_t[-1])
    giveup_time = first_giveup_time(events)
    if giveup_time is not None and args.post_skip_hours >= 0.0:
        end_time = min(end_time, giveup_time + args.post_skip_hours)
        numeric = {
            var: trim_series(series, end_time)
            for var, series in numeric.items()
        }
        events = {
            var: trim_series(series, end_time)
            for var, series in events.items()
        }
        soc_t, soc_pct = unpack(numeric["BATT_SOC"])
        irr_t, irradiance = unpack(numeric["SOLAR_IRRADIANCE"])
        soc_wh = [value * args.capacity_wh / 100.0 for value in soc_pct]

    fig, ax = plt.subplots(figsize=(12, 6))
    fig.suptitle("Solar Planning Run: Five Lawnmower Surveys, One Survey Skipped")

    if irr_t:
        ax.fill_between(
            irr_t,
            0,
            irradiance,
            step="post",
            color="#f4c430",
            alpha=0.22,
            label="Solar Irradiance",
        )

    for idx, (start, finish) in enumerate(active_windows(events, end_time), start=1):
        ax.axvspan(
            start,
            finish,
            color="#2a9d8f",
            alpha=0.12,
            label="Active task window" if idx == 1 else None,
        )

    ax.plot(soc_t, soc_wh, color="#1f77b4", linewidth=2.7, label="Battery SOC (Wh)")
    ax.axhline(reserve_wh, color="#1f77b4", linestyle=":", linewidth=1.3)
    ax.text(end_time, reserve_wh + 10, "reserve", color="#1f77b4", fontsize=8, ha="right")

    for time_h, label, decision in decision_events(events):
        if decision == "dispatch":
            ax.axvline(time_h, color="#777777", alpha=0.35, linewidth=1.0)
            ax.text(
                time_h,
                ax.get_ylim()[1] * 0.96,
                label,
                color="#555555",
                fontsize=8,
                ha="center",
                va="top",
            )
            continue

        soc_pct_at_skip = value_at_or_before(numeric["BATT_SOC"], time_h)
        marker_y = (
            soc_pct_at_skip * args.capacity_wh / 100.0
            if soc_pct_at_skip is not None
            else reserve_wh
        )
        ax.axvline(time_h, color="#d62728", alpha=0.35, linewidth=1.0)
        ax.scatter(
            [time_h],
            [marker_y],
            color="#d62728",
            s=50,
            zorder=5,
            label="Skipped survey",
        )
        ax.text(
            time_h,
            ax.get_ylim()[1] * 0.96,
            label,
            color="#d62728",
            fontsize=8,
            ha="center",
            va="top",
        )

    ax.set_xlabel("Planner mission hours")
    ax.set_ylabel("Energy (Wh) / Irradiance (W/m^2)", color="#1f77b4", weight="bold")
    ax.grid(True, alpha=0.25)
    ax.set_xlim(left=0)

    handles, labels = ax.get_legend_handles_labels()
    unique = dict(zip(labels, handles))
    legend_order = [
        "Battery SOC (Wh)",
        "Solar Irradiance",
        "Active task window",
        "Skipped survey",
    ]
    fig.legend(
        [unique[label] for label in legend_order if label in unique],
        [label for label in legend_order if label in unique],
        loc="lower center",
        bbox_to_anchor=(0.5, 0.01),
        ncol=4,
        framealpha=0.92,
    )

    fig.tight_layout(rect=(0, 0.08, 1, 0.96))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=180)
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
