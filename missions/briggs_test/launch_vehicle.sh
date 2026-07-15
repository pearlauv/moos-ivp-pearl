#!/bin/bash
#------------------------------------------------------------
#   Script: launch_vehicle.sh
#  Mission: briggs_test
#   Author: Charles Benjamin
#------------------------------------------------------------
#  Part 1: Set convenience functions for terminal output and
#          catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" = "yes" ]; then echo "$ME: $1"; fi; }
# shellcheck disable=SC2329  # Invoked by the SIGINT trap.
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=$(basename "$0")
MISSION_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$MISSION_DIR" || exit 1
CMD_ARGS=""
TIME_WARP=1
VERBOSE="no"
JUST_MAKE="no"
AUTO_LAUNCHED="no"

MODE="SIM"
IS_SIM="true"
IP_ADDR="localhost"
MOOS_PORT="9001"
PSHARE_PORT="9201"
SHORE_IP="localhost"
SHORE_PSHARE="9200"

VNAME="briggs"
COLOR="yellow"
AP_URL=""
AP_PROTOCOL=""
TAKEOFF_ALTITUDE="8"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
  CMD_ARGS+=" ${ARGI}"
  if [[ "${ARGI}" = "--help" || "${ARGI}" = "-h" ]]; then
    echo "$ME [OPTIONS] [time_warp]                              "
    echo "  --help, -h              Show this help message        "
    echo "  --just_make, -j         Only create targ files        "
    echo "  --verbose, -v           Verbose launch summary        "
    echo "  --auto, -a              Script launch, no uMAC        "
    echo "  --mode=<SIM|SITL|REAL>  Vehicle operating mode         "
    echo "  --ip=<localhost>        Vehicle IP address             "
    echo "  --mport=<9001>          Vehicle MOOSDB port            "
    echo "  --pshare=<9201>         Vehicle pShare port            "
    echo "  --shore=<localhost>     Shoreside host                 "
    echo "  --shore_pshare=<9200>   Shoreside pShare port          "
    echo "  --vname=<briggs>        Vehicle name                   "
    echo "  --color=<yellow>        Vehicle color                  "
    echo "  --ap_url=<endpoint>     ArduPilot endpoint             "
    echo "  --ap_protocol=<proto>   udp, tcp, or serial            "
    echo "  --takeoff_altitude=<8>  Copter takeoff altitude, m AGL"
    exit 0
  elif [[ "${ARGI}" =~ ^[0-9]+$ && "$TIME_WARP" = 1 ]]; then
    if [ "$ARGI" -lt 1 ]; then
      echo "$ME: time_warp must be at least 1. Exit Code 1."
      exit 1
    fi
    TIME_WARP=$ARGI
  elif [[ "${ARGI}" = "--just_make" || "${ARGI}" = "-j" ]]; then
    JUST_MAKE="yes"
  elif [[ "${ARGI}" = "--verbose" || "${ARGI}" = "-v" ]]; then
    VERBOSE="yes"
  elif [[ "${ARGI}" = "--auto" || "${ARGI}" = "-a" ]]; then
    AUTO_LAUNCHED="yes"
  elif [[ "${ARGI}" == --mode=* ]]; then
    MODE="${ARGI#*=}"
  elif [[ "${ARGI}" == --ip=* ]]; then
    IP_ADDR="${ARGI#*=}"
  elif [[ "${ARGI}" == --mport=* ]]; then
    MOOS_PORT="${ARGI#*=}"
  elif [[ "${ARGI}" == --pshare=* ]]; then
    PSHARE_PORT="${ARGI#*=}"
  elif [[ "${ARGI}" == --shore=* ]]; then
    SHORE_IP="${ARGI#*=}"
  elif [[ "${ARGI}" == --shore_pshare=* ]]; then
    SHORE_PSHARE="${ARGI#*=}"
  elif [[ "${ARGI}" == --vname=* ]]; then
    VNAME="${ARGI#*=}"
  elif [[ "${ARGI}" == --color=* ]]; then
    COLOR="${ARGI#*=}"
  elif [[ "${ARGI}" == --ap_url=* ]]; then
    AP_URL="${ARGI#*=}"
  elif [[ "${ARGI}" == --ap_protocol=* ]]; then
    AP_PROTOCOL="${ARGI#*=}"
  elif [[ "${ARGI}" == --takeoff_altitude=* ]]; then
    TAKEOFF_ALTITUDE="${ARGI#*=}"
  else
    echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
    exit 1
  fi
done

MODE=$(echo "$MODE" | tr '[:lower:]' '[:upper:]')
if [ "$MODE" = "SIM" ]; then
  IS_SIM="true"
elif [ "$MODE" = "SITL" ]; then
  IS_SIM="true"
elif [ "$MODE" = "REAL" ]; then
  IS_SIM="false"
else
  echo "$ME: --mode must be SIM, SITL, or REAL. Exit Code 1."
  exit 1
fi

if [ -z "$AP_URL" ]; then
  if [ "$MODE" = "SITL" ]; then
    AP_URL="127.0.0.1:5760"
  elif [ "$MODE" = "REAL" ]; then
    AP_URL="ttyACM0:115200"
  else
    AP_URL="unused"
  fi
fi
if [ -z "$AP_PROTOCOL" ]; then
  if [ "$MODE" = "SITL" ]; then
    AP_PROTOCOL="tcp"
  elif [ "$MODE" = "REAL" ]; then
    AP_PROTOCOL="serial"
  else
    AP_PROTOCOL="unused"
  fi
fi

#------------------------------------------------------------
#  Part 4: Show verbose summary
#------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then
  echo "============================================"
  echo "  $ME SUMMARY for $VNAME"
  echo "============================================"
  echo "CMD_ARGS =          [${CMD_ARGS}]"
  echo "MODE =              [${MODE}]"
  echo "IS_SIM =            [${IS_SIM}]"
  echo "TIME_WARP =         [${TIME_WARP}]"
  echo "JUST_MAKE =         [${JUST_MAKE}]"
  echo "AUTO_LAUNCHED =     [${AUTO_LAUNCHED}]"
  echo "IP_ADDR =           [${IP_ADDR}]"
  echo "MOOS_PORT =         [${MOOS_PORT}]"
  echo "PSHARE_PORT =       [${PSHARE_PORT}]"
  echo "SHORE_IP =          [${SHORE_IP}]"
  echo "SHORE_PSHARE =      [${SHORE_PSHARE}]"
  echo "AP_URL =            [${AP_URL}]"
  echo "AP_PROTOCOL =       [${AP_PROTOCOL}]"
  echo "TAKEOFF_ALTITUDE =  [${TAKEOFF_ALTITUDE}]"
fi

#------------------------------------------------------------
#  Part 5: Create the vehicle .moos and .bhv files
#------------------------------------------------------------
NSFLAGS=(--strict --force -x)
if [ "${AUTO_LAUNCHED}" = "no" ]; then
  NSFLAGS=(--interactive --force -x)
fi

nsplug meta_vehicle.moos "targ_${VNAME}.moos" "${NSFLAGS[@]}" WARP="$TIME_WARP" \
  IP_ADDR="$IP_ADDR" MOOS_PORT="$MOOS_PORT" PSHARE_PORT="$PSHARE_PORT" \
  SHORE_IP="$SHORE_IP" SHORE_PSHARE="$SHORE_PSHARE" VNAME="$VNAME" \
  COLOR="$COLOR" AP_URL="$AP_URL" AP_PROTOCOL="$AP_PROTOCOL" \
  TAKEOFF_ALTITUDE="$TAKEOFF_ALTITUDE" IS_SIM="$IS_SIM" XMODE="$MODE"

nsplug meta_vehicle.bhv "targ_${VNAME}.bhv" "${NSFLAGS[@]}" VNAME="$VNAME"

if [ "${JUST_MAKE}" = "yes" ]; then
  echo "$ME: Targ files made; exiting without launch."
  exit 0
fi

#------------------------------------------------------------
#  Part 6: Launch the vehicle community
#------------------------------------------------------------
echo "Launching $VNAME MOOS Community. MODE=$MODE WARP=$TIME_WARP"
pAntler "targ_${VNAME}.moos" >& /dev/null &
echo "Done Launching $VNAME MOOS Community"

#------------------------------------------------------------
#  Part 7: If launched from another script, exit now
#------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
  exit 0
fi

#------------------------------------------------------------
#  Part 8: Launch uMAC until the mission is quit
#------------------------------------------------------------
uMAC "targ_${VNAME}.moos"
trap "" SIGINT
kill -- -$$
