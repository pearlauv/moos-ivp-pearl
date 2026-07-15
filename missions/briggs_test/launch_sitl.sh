#!/bin/bash
#------------------------------------------------------------
#   Script: launch_sitl.sh
#  Mission: briggs_test
#   Author: Charles Benjamin
#------------------------------------------------------------
#  Part 1: Set global defaults
#------------------------------------------------------------
ME=$(basename "$0")
MISSION_DIR=$(cd "$(dirname "$0")" && pwd)
ARDUPILOT_ROOT="${ARDUPILOT_ROOT:-$HOME/ardupilot}"
SPEEDUP="1"
WIPE="yes"
NO_REBUILD="no"

HOME_LAT="42.3555241248"
HOME_LON="-71.1014110416"

#------------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
  if [[ "${ARGI}" = "--help" || "${ARGI}" = "-h" ]]; then
    echo "$ME [OPTIONS]"
    echo "  --help, -h                    Show this help message"
    echo "  --ardupilot_root=<path>       ArduPilot checkout"
    echo "  --speedup=<1>                 SITL speedup"
    echo "  --keep_eeprom                 Reuse prior SITL state"
    echo "  --no_rebuild                  Reuse the existing SITL binary"
    exit 0
  elif [[ "${ARGI}" == --ardupilot_root=* ]]; then
    ARDUPILOT_ROOT="${ARGI#*=}"
  elif [[ "${ARGI}" == --speedup=* ]]; then
    SPEEDUP="${ARGI#*=}"
  elif [ "${ARGI}" = "--keep_eeprom" ]; then
    WIPE="no"
  elif [ "${ARGI}" = "--no_rebuild" ]; then
    NO_REBUILD="yes"
  else
    echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
    exit 1
  fi
done

SIM_VEHICLE="$ARDUPILOT_ROOT/Tools/autotest/sim_vehicle.py"
if [ ! -x "$SIM_VEHICLE" ]; then
  echo "$ME: ArduPilot sim_vehicle.py not found at: $SIM_VEHICLE"
  exit 1
fi

#------------------------------------------------------------
#  Part 3: Launch ArduCopter SITL at the marked home point
#------------------------------------------------------------
ARGS=(-v ArduCopter -f quad)
ARGS+=(--custom-location="$HOME_LAT,$HOME_LON,5,0")
ARGS+=(--speedup="$SPEEDUP")
ARGS+=(--use-dir="$MISSION_DIR/SITL_briggs_test")
ARGS+=(--add-param-file="$MISSION_DIR/sitl.parm")
ARGS+=(--no-mavproxy)
ARGS+=(--sitl-instance-args="--serial1=udpclient:127.0.0.1:14551")
[ "$WIPE" = "yes" ] && ARGS+=(--wipe-eeprom)
[ "$NO_REBUILD" = "yes" ] && ARGS+=(--no-rebuild)

echo "$ME: Starting ArduCopter SITL at $HOME_LAT,$HOME_LON"
echo "$ME: Direct MAVLink endpoint is TCP 127.0.0.1:5760"
PYTHONPATH="$ARDUPILOT_ROOT/modules/mavlink${PYTHONPATH:+:$PYTHONPATH}" \
  python3 "$MISSION_DIR/sitl_rc_center.py" &
RC_CENTER_PID=$!
trap 'kill "$RC_CENTER_PID" 2>/dev/null || true' EXIT INT TERM
cd "$ARDUPILOT_ROOT" || exit 1
"$SIM_VEHICLE" "${ARGS[@]}"
