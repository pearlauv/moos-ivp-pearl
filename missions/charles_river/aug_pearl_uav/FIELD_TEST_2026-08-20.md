# Close-range REAL field test: 2026-08-20

This report records the close-range bench/dock qualification of the
three-computer `aug_pearl_uav` mission. The UAV Pi and Sherlock/PEARL Alfa
radios were approximately 2-3 m apart. The UAV Pixhawk was physically
disconnected, the UAV was disarmed, and no arm, takeoff, RTL, or other flight
controller command was sent.

## Configuration

The ordinary REAL launchers and default mission ports were used:

| Community | Advertised address | MOOSDB | pShare |
| --- | --- | --- | --- |
| shoreside | `100.127.231.65` | `9000` | `9200` |
| UAV | `100.70.189.91` | `9001` | `9201` |
| PEARL | `100.69.111.61` | `9002` | `9202` |

The UAV-to-PEARL peer route used `172.22.90.2 -> 192.168.88.253` through
Alfa, Sherlock, and PEARL Ethernet. Shoreside traffic used the existing
Tailscale endpoints. On the UAV, `wlan0` was down and the only default route
was through Alfa `wlan1` via `172.22.90.1`, so the Tailscale and internet
checks were not accidentally using the UAV's normal Wi-Fi.

## Results

### Physical radio and routing

- Sherlock saw exactly the configured UAV station, associated and authorized.
- UAV-to-Sherlock, UAV-to-PEARL, and PEARL-to-UAV 50-packet ICMP tests all had
  zero loss. Average latency was approximately 1.33, 1.35, and 1.18 ms,
  respectively.
- Twenty 1200-byte ICMP packets in each PEARL/UAV direction also had zero
  loss, with average latency approximately 1.39 and 1.79 ms.
- The UAV resolved public DNS, reached the internet through Alfa/Sherlock,
  and kept its existing Tailscale node online. Tailscale reported direct paths
  to PEARL and Sherlock over their `192.168.88.x` addresses.
- Across 757 logged Alfa samples while the collector was running, signal
  ranged from -52 to -40 dBm and averaged -47.7 dBm. TX and RX negotiated
  rates averaged 125.2 and 124.4 Mbit/s; both ranged from 52 to 130 Mbit/s.
  Maximum cumulative TX retries and TX failures were both zero.
- Deliberately stopping `iSherlockTelemetry` produced the expected gaps in
  PEARL's Alfa postings. Those gaps are test artifacts, not radio outages.

This is a successful close-range qualification. It is not an operational
range qualification; the failed obstructed 50 m placement from 2026-08-13
still requires a clearer launch position and a fresh under-load range test.

### Reboot recovery

Sherlock and the UAV Pi were rebooted independently after the mission test;
no network or service setting was changed by hand during recovery.

- Sherlock returned over Tailscale in 63 seconds. `hostapd` restored the AP
  address and the still-running UAV reassociated automatically after about 36
  seconds. `systemd-networkd-wait-online` held `network-online.target` until
  approximately two minutes after boot, so Telegraf and the NAT service were
  initially queued rather than failed. Both then started automatically. PEARL
  received a newer collector timestamp, and the UAV again had DNS, public
  internet, and a direct Tailscale path to PEARL.
- The UAV Pi returned over direct Ethernet in 34 seconds. NetworkManager
  reported `pearl-uav-backhaul-client` connected with `AUTOCONNECT=yes`,
  `wlan1=172.22.90.2/30`, `wlan0` down, and the only default route through
  `172.22.90.1`. Five packets in each PEARL/UAV direction had zero loss.
  PEARL's Tailscale probe reached the UAV directly via
  `172.22.90.2:41641`, and Sherlock again exposed fresh linked telemetry with
  one station, -48 dBm, and zero retries or failures.

Operationally, allow roughly two minutes after a Sherlock reboot before
launching the mission. Association alone is not the readiness signal: confirm
the port `9273` sample timestamp is advancing and that the UAV has upstream
connectivity.

### REAL mission and safety gates

All three communities ran concurrently in REAL mode. Shoreside broker state
showed both vehicle nodes. PEARL and UAV Helms were forced to `PARK` before
testing and remained parked.

The screenshot checklist was covered as far as the disconnected Pixhawk
allowed:

1. **Three REAL communities:** passed.
2. **Actual sensor delivery:** actual PEARL SOC and apparent wind reached the
   UAV through the direct Alfa route. PEARL charged from 14% to 15% during the
   run. Actual UAV SOC could not be tested without the Pixhawk.
3. **Telemetry-loss rejection:** passed. After stopping PEARL telemetry long
   enough to exceed the gate's three-second freshness limit, a labeled test
   request returned
   `status=REJECTED,reason=PEARL_BATTERY_STALE` and
   `UAV_TAKEOFF_APPROVED=0`.
4. **Restore and READY calculation:** passed without commanding takeoff.
   Restored live PEARL telemetry initially blocked on its actual 14% battery.
   After PEARL reached its real 15% minimum, the gate reported `READY` using
   actual PEARL battery, actual wind, and a clearly labeled synthetic 80% UAV
   battery sample. No `UAV_TAKEOFF_REQUEST` accompanied this READY test.
5. **Logs:** all three original `.alog` files were preserved and reviewed.

A button-equivalent `UAV TAKEOFF` message was also posted from the shoreside
community while PEARL was at 14%. `pMediator` added message ID
`shoreside_1787242531982439`; the UAV acknowledged it, then returned
`status=REJECTED,reason=PEARL_BATTERY_LOW`. This proves the ordinary operator
message path, acknowledgement path, and gate consumption without bypassing
the mission routing.

A button-equivalent `RENDEZVOUS_START` used message ID
`shoreside_1787242531982440` and was acknowledged by the UAV. With no Pixhawk
navigation, the UAV coordinator transitioned from `IDLE` to
`ABORT#reason=navigation_stale`; `IVPHELM_STATE` stayed `PARK` and
`ROUTE_DEPLOY` stayed false.

The UAV log contains no `ARDU_COMMAND`, `ARM_UAV`, or `RETURN_TO_LAUNCH`
posting. Flight-controller health, GPS, and landed-state variables were never
published, as expected with the Pixhawk disconnected.

## Remaining flight qualification

The following items were intentionally not claimed by this test:

- actual Pixhawk UAV battery percentage and its validity/age;
- actual Pixhawk health, GPS, armed, and landed-state telemetry;
- propellers-off arm-policy and real flight-controller command checks;
- outdoor arm/takeoff to 8 m and coordinated landing; and
- packet loss, signal, retry, and throughput at the intended launch position.

Connect the Pixhawk with propellers removed before repeating the sensor and
policy checks. Do not proceed to outdoor arm/takeoff until the actual UAV
battery and all flight-state inputs are valid and the gate is READY without
synthetic values.

## Preserved logs

Protected local copies are under the ignored directory
`field_logs/2026-08-20_close_range_real/`. This directory is not removed by
`clean.sh`. The original UAV and PEARL copies also remain on their respective
Pis.

| Community | `.alog` SHA-256 |
| --- | --- |
| shoreside | `d6c81f03ee0b9d35403c04cdb381a26ecf560f93aad41aec5ffaeb805ddc2f78` |
| UAV | `a8289235ae178fe7fd0d259dda9f07f74f03d2cc707553b7844b0a1d4cadeb92` |
| PEARL | `708312749d0c66973b1fd5c65ac151b0a833006c154998847e0dd91c5a79d05f` |

The key log checks were:

```sh
aloggrep field_logs/2026-08-20_close_range_real/LOG_SHORESIDE_20_8_2026_____12_15_32/LOG_SHORESIDE_20_8_2026_____12_15_32.alog \
  FIELD_TEST_EVENT NODE_MESSAGE_LOCAL MEDIATED_MESSAGE_LOCAL ACK_MESSAGE \
  --format=time:var:val -nc -nr --sd

aloggrep field_logs/2026-08-20_close_range_real/LOG_uav_20_8_2026_____17_16_19/LOG_uav_20_8_2026_____17_16_19.alog \
  FIELD_TEST_EVENT UAV_TAKEOFF_RESULT UAV_TAKEOFF_APPROVED \
  --format=time:var:val -nc -nr --sd

# Expected to produce no lines for this run.
aloggrep field_logs/2026-08-20_close_range_real/LOG_uav_20_8_2026_____17_16_19/LOG_uav_20_8_2026_____17_16_19.alog \
  ARDU_COMMAND ARM_UAV RETURN_TO_LAUNCH \
  --format=time:var:val -nc -nr --sd
```
