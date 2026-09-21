#!/usr/bin/env bash
# Stop every VNC/noVNC GUI session owned by the current user and remove stale
# display state. This intentionally does not kill pAntler or other mission apps.
set -Eeuo pipefail

if (( $# > 0 )); then
  if [[ $1 == "--help" || $1 == "-h" ]]; then
    cat <<'EOF'
Usage: kill_vnc.sh

Stops all current-user MOOS VNC sessions and their GUI children, including
TigerVNC, pMarineViewer, Openbox, and websockify/noVNC. It also disables the
MOOS Tailscale Serve endpoint and removes stale VNC display locks.
EOF
    exit 0
  fi
  echo "Usage: $(basename "$0")" >&2
  exit 2
fi

uid=$(id -u)
runtime_root=${XDG_RUNTIME_DIR:-/tmp/${USER}-runtime}/moos-vnc
tailscale_https_port=${MOOS_VNC_TAILSCALE_HTTPS_PORT:-443}
shutdown_grace=${MOOS_VNC_KILL_GRACE:-3}

if ! [[ $shutdown_grace =~ ^[0-9]+$ ]]; then
  echo "MOOS_VNC_KILL_GRACE must be a non-negative integer" >&2
  exit 2
fi

declare -A targets=()

add_tree() {
  local pid=$1
  local child owner

  [[ $pid =~ ^[0-9]+$ ]] || return 0
  [[ $pid != "$$" ]] || return 0
  [[ -z ${targets[$pid]+set} ]] || return 0

  owner=$(ps -o uid= -p "$pid" 2>/dev/null | tr -d '[:space:]')
  [[ $owner == "$uid" ]] || return 0

  targets[$pid]=1
  while read -r child; do
    [[ -n $child ]] && add_tree "$child"
  done < <(pgrep -P "$pid" 2>/dev/null || true)
}

pids_with_arg_basename() {
  local wanted=$1
  local proc pid owner arg

  for proc in /proc/[0-9]*; do
    pid=${proc##*/}
    [[ $pid != "$$" && -r $proc/cmdline ]] || continue
    owner=$(stat -c '%u' "$proc" 2>/dev/null || true)
    [[ $owner == "$uid" ]] || continue

    while IFS= read -r -d '' arg; do
      if [[ ${arg##*/} == "$wanted" ]]; then
        printf '%s\n' "$pid"
        break
      fi
    done <"$proc/cmdline"
  done
}

collect_processes() {
  local name pid
  local -a names=(
    Xtigervnc Xvnc tigervncserver
    websockify novnc_proxy
    openbox openbox-session
    pMarineViewer
  )

  # A shebang script commonly appears as "bash /path/to/script" in /proc.
  # Match complete argv tokens rather than pgrep -f, which can accidentally
  # match an unrelated shell whose command text merely mentions the script.
  while read -r pid; do
    [[ -n $pid ]] && add_tree "$pid"
  done < <(pids_with_arg_basename moos-vnc-session)

  for name in "${names[@]}"; do
    while read -r pid; do
      [[ -n $pid ]] && add_tree "$pid"
    done < <(pgrep -u "$uid" -x "$name" 2>/dev/null || true)
  done

  # Some distributions run noVNC tools through a Python interpreter, leaving
  # "python" as the process name. Match exact argv path basenames as fallback.
  for name in websockify novnc_proxy; do
    while read -r pid; do
      [[ -n $pid ]] && add_tree "$pid"
    done < <(pids_with_arg_basename "$name")
  done
}

collect_processes

if (( ${#targets[@]} > 0 )); then
  echo "Stopping ${#targets[@]} VNC/noVNC process(es)..."
  kill -TERM "${!targets[@]}" 2>/dev/null || true

  for ((attempt = 0; attempt < shutdown_grace * 10; attempt++)); do
    alive=false
    for pid in "${!targets[@]}"; do
      if kill -0 "$pid" 2>/dev/null; then
        alive=true
        break
      fi
    done
    [[ $alive == false ]] && break
    sleep 0.1
  done

  for pid in "${!targets[@]}"; do
    kill -KILL "$pid" 2>/dev/null || true
  done
else
  echo "No running VNC/noVNC processes found."
fi

# Stop sessions known to the TigerVNC wrapper as well as directly launched
# Xtigervnc processes handled above.
if command -v tigervncserver >/dev/null 2>&1; then
  while read -r display; do
    [[ $display =~ ^:[0-9]+$ ]] || continue
    tigervncserver -kill "$display" >/dev/null 2>&1 || true
  done < <(tigervncserver -list 2>/dev/null | awk '$1 ~ /^:[0-9]+$/ {print $1}')
elif command -v vncserver >/dev/null 2>&1; then
  while read -r display; do
    [[ $display =~ ^:[0-9]+$ ]] || continue
    vncserver -kill "$display" >/dev/null 2>&1 || true
  done < <(vncserver -list 2>/dev/null | awk '$1 ~ /^:[0-9]+$/ {print $1}')
fi

# The launcher owns only this Serve endpoint. Do not reset unrelated Tailscale
# Serve configuration.
if command -v tailscale >/dev/null 2>&1; then
  tailscale serve --https="$tailscale_https_port" off >/dev/null 2>&1 || true
fi

rm -rf -- "$runtime_root"

# Remove only stale X lock/socket files owned by this user. Active unrelated X
# servers are left untouched.
for ((display = 1; display <= 99; display++)); do
  if DISPLAY=":$display" xdpyinfo >/dev/null 2>&1; then
    continue
  fi

  for stale_path in "/tmp/.X${display}-lock" "/tmp/.X11-unix/X${display}"; do
    [[ -e $stale_path || -S $stale_path ]] || continue
    owner=$(stat -c '%u' "$stale_path" 2>/dev/null || true)
    if [[ $owner == "$uid" ]]; then
      rm -f -- "$stale_path"
    fi
  done
done

if [[ -d $HOME/.vnc ]]; then
  find "$HOME/.vnc" -maxdepth 1 -type f -name '*.pid' -user "$USER" -delete
fi

# Verify that no matching process survived or was immediately restarted.
targets=()
collect_processes
if (( ${#targets[@]} > 0 )); then
  echo "Unable to stop all VNC/noVNC processes: ${!targets[*]}" >&2
  echo "A running supervisor (for example pAntler) may be restarting them." >&2
  exit 1
fi

echo "All current-user VNC/noVNC sessions are stopped."
