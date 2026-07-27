# PEARL Wi-Fi backhaul

Configuration record for the dedicated wireless link between the ground Pi and
the UAV Pi. This is separate from the Starlink router SSID, `PEARL Uplink`.

Last checked: 2026-07-24

## Design

The intended path is:

```text
laptop / pMarineViewer
        |
        | PEARL Uplink / Starlink LAN
        |
ground Pi
        |
        | PEARL-STARLINK-BACKHAUL
        |
UAV Pi
```

The ground Pi will route traffic between its upstream `PEARL Uplink`
connection and the dedicated UAV backhaul. The design is routed IP, not a
transparent Layer-2 repeater. NAT should remain disabled so the laptop can
reach the UAV's advertised backhaul address directly.

## Common radio settings

These settings should match on both radios:

| Setting | Value |
|---|---|
| Country/regulatory domain | US |
| Band | 5 GHz |
| Channel | 44, non-DFS |
| Channel width | 20 MHz, set by the ground AP |
| Backhaul SSID | `PEARL-STARLINK-BACKHAUL` |
| Security | WPA-PSK / RSN / CCMP |
| Power saving | Disabled |

The Wi-Fi password is intentionally not stored in this repository. It is
stored in the protected NetworkManager profile on the Pi.

## UAV Pi status

The external MediaTek MT7921U adapter is `wlan1`:

- MAC: `00:c0:ca:b8:00:88`
- Mode: managed/client
- Static address: `172.22.90.2/30`
- Default route: none for now
- IPv6: disabled on the backhaul profile
- NetworkManager profile: `uav-starlink-backhaul`
- Autoconnect: disabled until the ground AP exists
- Power saving: disabled in the profile and at runtime

The profile is intentionally inactive. Existing `MIT` Wi-Fi on `wlan0`, direct
Ethernet on `eth0`, and SiK PPP on `ppp0` are not changed.

The client profile leaves channel width at `auto` because NetworkManager does
not allow a fixed channel width on an infrastructure/client profile. The AP
will advertise the fixed 20 MHz width.

## Ground/PEARL radio staged on the UAV Pi

The second adapter is currently plugged into the UAV Pi for preparation. It
was identified as the MediaTek MT7921U with MAC `00:c0:ca:b8:00:84`.

An inactive ground-side AP profile is now stored on the Pi:

- NetworkManager profile: `pearl-ground-backhaul-ap`
- Mode: access point
- Static address: `172.22.90.1/30`
- Channel width: fixed 20 MHz
- Autoconnect: disabled until the adapter is moved to the ground Pi
- Power saving: disabled

The profile is not active on the UAV Pi. The adapter can be moved to the
ground Pi, where this profile will be used after confirming the interface name
and enabling forwarding toward `PEARL Uplink`.

## Ground Pi target

When the second adapter is installed on the ground Pi:

- Configure it as an AP on `wlan1`.
- Use the common settings above.
- Assign `172.22.90.1/30`.
- Enable IPv4 forwarding between the backhaul and `PEARL Uplink` interfaces.
- Allow forwarding without NAT.
- Add a laptop route to `172.22.90.0/30` through the ground Pi's
  `PEARL Uplink` address.

The upstream `PEARL Uplink` address/subnet and the laptop route are still
unknown and will be recorded after the ground Pi is connected.

## Mission addressing

For a shoreside community running on the laptop:

- Vehicle `--ip`: `172.22.90.2`
- Vehicle `--shore`: the laptop's reachable `PEARL Uplink` address
- Shoreside `--ip`: that same laptop address
- Vehicle pShare: `9201`
- Shoreside pShare: `9200`

The mission scripts force these addresses through `pHostInfo` and use the
explicit vehicle-to-shoreside route in `uFldNodeBroker`.

## Verification checklist

1. Confirm the UAV client associates with the ground AP.
2. Ping `172.22.90.1` from the UAV Pi.
3. Ping the laptop's `PEARL Uplink` address from the UAV Pi.
4. Ping `172.22.90.2` from the laptop using the added route.
5. Verify bidirectional UDP reachability on pShare ports 9200 and 9201.
6. Launch the mission in SIM mode before using REAL mode.
7. Test the Wi-Fi link while SiK PPP traffic is active.
