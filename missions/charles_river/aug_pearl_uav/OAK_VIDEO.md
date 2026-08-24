# OAK video over Tailscale

The UAV Pi runs one boot-managed `oak-camera.service` that owns the connected
Luxonis OAK device. Rigging's `oak_camera` role installs the DepthAI runtime,
USB permissions, service configuration, and local telemetry integration. The
MOOS mission does not start, stop, or supervise this service.

The UAV Pi serves a small selectable JPEG monitor directly on its Tailnet
address. From an authorized Tailnet client, open
`http://uav-pi-1:8082/`. The page uses ordinary same-origin HTTP requests for
the selected RGB, left, right, or disparity JPEG and needs no separate browser
transport.

| Function | Direct Tailnet endpoint | Optional local relay |
| --- | --- | --- |
| Selectable JPEG monitor | `http://uav-pi-1:8082/` | `http://127.0.0.1:8082/` |
| Health, snapshots, and metrics | `http://uav-pi-1:9102/` | `http://127.0.0.1:9102/` |

The monitor presents one large view, with `rgb` selected by default. Its
dropdown can switch to `disparity`, `left`, or `right` without restarting the
camera service. The page updates the view from independently decodable JPEG
frames, so an ordinary browser can connect or reconnect without codec history.

## Deployed remote profile

The current UAV profile limits the encoded streams for remote operation:

| Stream | Resolution | Rate | Encoding |
| --- | --- | --- | --- |
| RGB | 640 x 360 | 4 fps | MJPEG, quality 45 |
| Left mono | 320 x 200 | 2 fps | MJPEG, quality 40 |
| Right mono | 320 x 200 | 2 fps | MJPEG, quality 40 |
| Disparity | 160 x 100 | 2 fps | JPEG |

The full RGB snapshot path is 1280 x 720 at 1 fps with JPEG quality 85; the
disparity snapshot uses the latest cached disparity output. Each viewer frame
is independently decodable, so a late viewer or a client reconnecting after a
link interruption can display the next frame without waiting for video-stream
history. This profile favors reliable late and reconnect viewing over motion
smoothness.

The monitor retrieves one latest frame at a time for the selected view. Slow
links therefore lower the visible refresh rate instead of accumulating
concurrent requests. A timed-out request is skipped while its last good frame
remains visible, and a stale-view guard reloads the page if the selected view
stops advancing. The three unselected views are not sent to that browser.

MJPEG has no configured bitrate target. Its measured bitrate is
scene-dependent and is reported by the camera metrics and Grafana dashboard
together with frame rate, sequence gaps, and last-frame age.

## Starting and recovery

Normally there is nothing to launch. Rigging enables `oak-camera.service`,
systemd starts it when the UAV Pi boots, and the restart policy retries if the
camera or Tailscale is not ready yet. Once the Pi is online, connect to the
Tailnet and open `http://uav-pi-1:8082/`.

If recovery is needed, run these commands on the UAV Pi:

```sh
sudo systemctl start oak-camera.service
sudo systemctl restart oak-camera.service
curl -fsS http://127.0.0.1:9102/readyz | jq
```

After a reimage, reapply Rigging's targeted `oak_camera,telegraf` deployment;
that recreates the runtime, service, USB permissions, configuration, and local
metrics scrape. The MOOS mission launchers never own this service.

## Tailnet name changes

The current operator-facing MagicDNS name is `uav-pi-1`; the Linux hostname
and Prometheus `host` label remain `uav-pi`. The camera service binds to the
Pi's current Tailscale IP rather than either hostname, so changing the Tailnet
name does not require changing or restarting the camera pipeline.

On the UAV Pi, discover the actual control-plane DNS name or request a new one:

```sh
tailscale status --json | jq -r '.Self.DNSName | rtrimstr(".")'
sudo tailscale set --hostname=<new-tailnet-name>
tailscale status --json | jq -r '.Self.DNSName | rtrimstr(".")'
```

Tailscale may append a numeric suffix if another node still owns the requested
name. For a replacement Pi, retire the old node in the Tailscale admin console
or use the actual unique name returned above.

After a rename, update Rigging's `uav_pi_tailscale_hostname`, UAV static
inventory `ansible_host`/`tailscale_hostname`, operator bookmarks, and the
Grafana dashboard's `OAK Tailnet host` variable. The optional helper needs no
code change: pass `--host <actual-tailnet-name>` or set `OAK_CAMERA_HOST`.
Rigging's `docs/uav-pi.md` is the authoritative rename and rebuild procedure.

## Operator checks

From an authorized Tailnet client, confirm the path, API, snapshots, and
monitor directly:

```sh
tailscale status
tailscale ping uav-pi-1
curl -fsS http://uav-pi-1:9102/readyz
curl -fsS -o oak-rgb.jpg \
  http://uav-pi-1:9102/snapshot/rgb.jpg
curl -fsS -o oak-disparity.jpg \
  http://uav-pi-1:9102/snapshot/disparity.jpg
```

Then open `http://uav-pi-1:8082/` in a browser. No local helper is required.

If a localhost alias is useful for troubleshooting, Rigging provides an
optional relay. Run it from the shoreside Rigging checkout:

```sh
./.bin/oak-camera-viewer
```

The defaults bind local `127.0.0.1` ports `8082` and `9102`, relay them to the
same ports on `uav-pi-1`, and advertise a 1200-byte upstream TCP maximum
segment size. Open `http://127.0.0.1:8082/` while it is running and stop it with
Ctrl-C.

Before opening its listeners, the helper resolves the target and retains only
Tailscale CGNAT (`100.64.0.0/10`) or Tailscale IPv6
(`fd7a:115c:a1e0::/48`) addresses. It refuses a target name that does not
resolve into either Tailnet range, preventing an accidental public or LAN
upstream.

`/healthz` reports service liveness and always returns JSON while the HTTP
service is running. `/readyz` returns HTTP 200 only while the DepthAI pipeline
is running and all four configured streams have produced a frame within five
seconds. It returns HTTP 503 when the camera or a stream is unavailable.

Useful API paths on port `9102` are:

- `/healthz`
- `/readyz`
- `/snapshot/rgb.jpg`
- `/snapshot/disparity.jpg`
- `/metrics`

On the UAV Pi, inspect the boot service without launching a mission:

```sh
systemctl --no-pager --full status oak-camera.service
journalctl -u oak-camera.service --since "10 minutes ago" --no-pager
```

The service is the exclusive OAK owner. Do not run another DepthAI application
against the device at the same time.

## Network path and bandwidth qualification

The live browser and API connect directly to the UAV Tailnet node. The monitor
uses MTU-safe TCP segmentation and small HTTP writes for the 1280-byte Tailnet
MTU. Neither path uses SSH forwarding, pShare, PEARL as a video proxy, port
`9300`, Tailscale Serve, Funnel, or a fixed UDP relay. Tailscale may carry the
TCP connection locally, over Sherlock's upstream connection, or through DERP.
With Starlink as Sherlock's upstream, the same direct URL can cross Starlink.

The optional helper is an operator-run, transient shoreside process. It is not
a fixed or deployed intermediate relay and disappears when the operator
presses Ctrl-C.

The present bench setup exercises the application through Tailscale, but it
does not by itself qualify the complete Alfa-to-Sherlock-to-Starlink underlay.
For field qualification, retain direct Ethernet as recovery, disconnect the
UAV's normal `wlan0` path, and verify all of the following while viewing the
remote profile:

1. `tailscale status` records whether the path is direct or DERP.
2. `tailscale ping uav-pi-1`, the direct `/readyz`, and both snapshots remain
   usable.
3. `http://uav-pi-1:8082/` shows RGB by default and can switch among all four
   views without the optional helper.
4. Observed stream FPS, scene-dependent bitrate, last-frame age, and dropped
   frames remain acceptable for the available link.
5. Alfa signal, inactive time, retries, packet loss, and mission broker
   freshness remain acceptable under the added load.
6. The Tailnet connection and monitor recover after a controlled link
   interruption.

Close the live viewer and use snapshots when the available link cannot sustain
the selected live view.

## Metrics and Grafana boundary

The camera exports Prometheus text metrics on the loopback and direct Tailnet
API endpoints: `http://127.0.0.1:9102/metrics` on the UAV and
`http://uav-pi-1:9102/metrics` from an authorized Tailnet client. On the UAV
Pi, Telegraf scrapes over loopback and the local Prometheus service scrapes
Telegraf. This local collection is enabled by the Rigging UAV configuration.

The metrics include camera readiness, device and USB information, per-stream
FPS, bitrate, frames, sequence gaps, last-frame age, snapshot age, OAK
temperature, processor utilization, and memory use. Grafana is for these
health and performance measurements; it does not transport or render the live
video. The Rigging role contains an OAK dashboard definition, but dashboard
publication is disabled by default. Its `OAK Tailnet host` variable controls
the direct viewer link and defaults to `uav-pi-1`; it does not require the
optional localhost relay.
Prometheus `remote_write` is also disabled by default. Enabling either sends
data or configuration to an external service and requires explicit approval
plus the intended Grafana endpoint and credentials.

## Flight-safety boundary

This viewer is currently for situational awareness and diagnostics. It does
not publish MOOS variables, MAVLink `LANDING_TARGET`, or the
`UAV_LANDING_TARGET_*` inputs used by the rendezvous landing gate. Camera
readiness therefore receives no precision-landing safety credit. Any future
perception or landing-target producer must share this single camera-owner
process and be qualified separately before the mission may depend on it.
