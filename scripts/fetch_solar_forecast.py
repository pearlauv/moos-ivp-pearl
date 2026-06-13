#!/usr/bin/env python3
"""Fetch a dated 24-hour Open-Meteo shortwave-radiation forecast for pPearlSunPlan."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import sys
import urllib.parse
import urllib.request
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--lat", type=float, required=True)
    parser.add_argument("--lon", type=float, required=True)
    parser.add_argument("--hours", type=int, default=24)
    parser.add_argument("--timezone", default="America/New_York")
    parser.add_argument(
        "--day-offset",
        type=int,
        default=1,
        help="0=today, 1=tomorrow, 2=day after tomorrow.",
    )
    parser.add_argument(
        "--date",
        help="Forecast local date to extract, YYYY-MM-DD. Overrides --day-offset.",
    )
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def target_date(day_offset: int, date_text: str | None) -> dt.date:
    if date_text:
        return dt.date.fromisoformat(date_text)
    if day_offset < 0 or day_offset > 15:
        raise SystemExit("--day-offset must be in the range 0..15")
    return dt.date.today() + dt.timedelta(days=day_offset)


def fetch_shortwave(lat: float, lon: float, hours: int, timezone: str,
                    forecast_date: dt.date) -> tuple[list[float], str]:
    today = dt.date.today()
    days_needed = max(1, (forecast_date - today).days + 1)
    query = urllib.parse.urlencode(
        {
            "latitude": lat,
            "longitude": lon,
            "hourly": "shortwave_radiation",
            "forecast_days": days_needed,
            "timezone": timezone,
        },
    )
    url = f"https://api.open-meteo.com/v1/forecast?{query}"
    with urllib.request.urlopen(url, timeout=15) as response:
        payload = json.load(response)

    hourly = payload.get("hourly", {})
    times = hourly.get("time", [])
    values = hourly.get("shortwave_radiation", [])
    if not times or not values:
        raise RuntimeError("Open-Meteo response did not include shortwave_radiation")

    selected = []
    date_prefix = forecast_date.isoformat() + "T"
    for timestamp, value in zip(times, values):
        if timestamp.startswith(date_prefix):
            selected.append(max(0.0, float(value)))
        if len(selected) >= hours:
            break

    if len(selected) < hours:
        raise RuntimeError(
            f"Open-Meteo response had {len(selected)} hours for {forecast_date}, "
            f"expected {hours}"
        )
    return selected, url


def main() -> int:
    args = parse_args()
    if args.hours <= 0 or args.hours > 24:
        raise SystemExit("--hours must be in the range 1..24")

    forecast_date = target_date(args.day_offset, args.date)
    values, url = fetch_shortwave(args.lat, args.lon, args.hours, args.timezone, forecast_date)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow([f"# source=Open-Meteo forecast API"])
        writer.writerow([f"# url={url}"])
        writer.writerow([f"# date={forecast_date.isoformat()}"])
        writer.writerow([f"# timezone={args.timezone}"])
        writer.writerow(["# hour", "shortwave_radiation_w_m2"])
        for hour, value in enumerate(values):
            writer.writerow([hour, f"{value:.3f}"])
    print(args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
