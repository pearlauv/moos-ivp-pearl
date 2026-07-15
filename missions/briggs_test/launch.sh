#!/bin/bash
#------------------------------------------------------------
#   Script: launch.sh
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
LOG_CLEAN="no"
XLAUNCHED="no"
NOGUI="no"

MODE="SIM"
AP_URL=""
AP_PROTOCOL=""
TAKEOFF_ALTITUDE="8"

VNAME="briggs"
COLOR="yellow"
SHORE_IP="localhost"
VEH_IP="localhost"

SHORE_MPORT="9000"
VEH_MPORT="9001"
SHORE_PSHARE="9200"
VEH_PSHARE="9201"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
  CMD_ARGS+=" ${ARGI}"
  if [[ "${ARGI}" = "--help" || "${ARGI}" = "-h" ]]; then
    echo "$ME [OPTIONS] [time_warp]                              "
    echo "                                                       "
    echo "Modes:                                                 "
    echo "  --sim                   MOOS-IvP simulation (default) "
    echo "  --sitl                  ArduCopter SITL via pArduBridge"
    echo "  --real                  Physical ArduCopter hardware  "
    echo "                                                       "
    echo "Options:                                               "
    echo "  --help, -h              Show this help message        "
    echo "  --verbose, -v           Verbose launch summary        "
    echo "  --just_make, -j         Only create targ files        "
    echo "  --log_clean, -lc        Run clean.sh before launch    "
    echo "  --xlaunched, -x         Launched by automation        "
    echo "  --nogui, -ng            No pMarineViewer              "
    echo "                                                       "
    echo "  --ap_url=<endpoint>     Host:port or serial endpoint   "
    echo "  --ap_protocol=<proto>   udp, tcp, or serial            "
    echo "  --takeoff_altitude=<8>  Copter takeoff altitude, m AGL"
    echo "  --vname=<briggs>        Vehicle/community name         "
    echo "  --color=<yellow>        Viewer vehicle color           "
    echo "  --shore_ip=<localhost>  Shoreside host                 "
    echo "  --veh_ip=<localhost>    Vehicle host                   "
    echo "                                                       "
    echo "  --shore_mport=<9000>    Shoreside MOOSDB port          "
    echo "  --veh_mport=<9001>      Vehicle MOOSDB port            "
    echo "  --shore_pshare=<9200>   Shoreside pShare port          "
    echo "  --veh_pshare=<9201>     Vehicle pShare port            "
    exit 0
  elif [[ "${ARGI}" =~ ^[0-9]+$ && "$TIME_WARP" = 1 ]]; then
    if [ "$ARGI" -lt 1 ]; then
      echo "$ME: time_warp must be at least 1. Exit Code 1."
      exit 1
    fi
    TIME_WARP=$ARGI
  elif [ "${ARGI}" = "--sim" ]; then
    MODE="SIM"
  elif [ "${ARGI}" = "--sitl" ]; then
    MODE="SITL"
  elif [ "${ARGI}" = "--real" ]; then
    MODE="REAL"
  elif [[ "${ARGI}" = "--verbose" || "${ARGI}" = "-v" ]]; then
    VERBOSE="yes"
  elif [[ "${ARGI}" = "--just_make" || "${ARGI}" = "-j" ]]; then
    JUST_MAKE="yes"
  elif [[ "${ARGI}" = "--log_clean" || "${ARGI}" = "-lc" ]]; then
    LOG_CLEAN="yes"
  elif [[ "${ARGI}" = "--xlaunched" || "${ARGI}" = "-x" ]]; then
    XLAUNCHED="yes"
  elif [[ "${ARGI}" = "--nogui" || "${ARGI}" = "-ng" ]]; then
    NOGUI="yes"
  elif [[ "${ARGI}" == --ap_url=* ]]; then
    AP_URL="${ARGI#*=}"
  elif [[ "${ARGI}" == --ap_protocol=* ]]; then
    AP_PROTOCOL="${ARGI#*=}"
  elif [[ "${ARGI}" == --takeoff_altitude=* ]]; then
    TAKEOFF_ALTITUDE="${ARGI#*=}"
  elif [[ "${ARGI}" == --vname=* ]]; then
    VNAME="${ARGI#*=}"
  elif [[ "${ARGI}" == --color=* ]]; then
    COLOR="${ARGI#*=}"
  elif [[ "${ARGI}" == --shore_ip=* ]]; then
    SHORE_IP="${ARGI#*=}"
  elif [[ "${ARGI}" == --veh_ip=* ]]; then
    VEH_IP="${ARGI#*=}"
  elif [[ "${ARGI}" == --shore_mport=* ]]; then
    SHORE_MPORT="${ARGI#*=}"
  elif [[ "${ARGI}" == --veh_mport=* ]]; then
    VEH_MPORT="${ARGI#*=}"
  elif [[ "${ARGI}" == --shore_pshare=* ]]; then
    SHORE_PSHARE="${ARGI#*=}"
  elif [[ "${ARGI}" == --veh_pshare=* ]]; then
    VEH_PSHARE="${ARGI#*=}"
  else
    echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
    exit 1
  fi
done

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
#  Part 4: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then
  echo "============================================"
  echo "  $ME SUMMARY"
  echo "============================================"
  echo "CMD_ARGS =          [${CMD_ARGS}]"
  echo "MODE =              [${MODE}]"
  echo "TIME_WARP =         [${TIME_WARP}]"
  echo "JUST_MAKE =         [${JUST_MAKE}]"
  echo "XLAUNCHED =         [${XLAUNCHED}]"
  echo "NOGUI =             [${NOGUI}]"
  echo "VNAME =             [${VNAME}]"
  echo "AP_URL =            [${AP_URL}]"
  echo "AP_PROTOCOL =       [${AP_PROTOCOL}]"
  echo "TAKEOFF_ALTITUDE =  [${TAKEOFF_ALTITUDE}]"
  echo "SHORE_MPORT =       [${SHORE_MPORT}]"
  echo "VEH_MPORT =         [${VEH_MPORT}]"
  echo "SHORE_PSHARE =      [${SHORE_PSHARE}]"
  echo "VEH_PSHARE =        [${VEH_PSHARE}]"
  echo -n "Hit any key to continue launch "
  read -r _
fi

#------------------------------------------------------------
#  Part 5: Optionally clean old generated files
#------------------------------------------------------------
if [[ "$LOG_CLEAN" = "yes" && -f "clean.sh" ]]; then
  ./clean.sh
fi

#------------------------------------------------------------
#  Part 6: Launch the vehicle community
#------------------------------------------------------------
VARGS=(--auto "$TIME_WARP" --mode="$MODE")
VARGS+=(--ip="$VEH_IP" --mport="$VEH_MPORT" --pshare="$VEH_PSHARE")
VARGS+=(--shore="$SHORE_IP" --shore_pshare="$SHORE_PSHARE")
VARGS+=(--vname="$VNAME" --color="$COLOR")
VARGS+=(--ap_url="$AP_URL" --ap_protocol="$AP_PROTOCOL")
VARGS+=(--takeoff_altitude="$TAKEOFF_ALTITUDE")
[ "$JUST_MAKE" = "yes" ] && VARGS+=(--just_make)
[ "$VERBOSE" = "yes" ] && VARGS+=(--verbose)

vecho "Launching vehicle with args: ${VARGS[*]}"
./launch_vehicle.sh "${VARGS[@]}" || exit 1

#------------------------------------------------------------
#  Part 7: Launch the shoreside community
#------------------------------------------------------------
SARGS=(--auto "$TIME_WARP" --mode="$MODE" --mport="$SHORE_MPORT" --pshare="$SHORE_PSHARE")
SARGS+=(--ip="$SHORE_IP" --vnames="$VNAME")
[ "$JUST_MAKE" = "yes" ] && SARGS+=(--just_make)
[ "$VERBOSE" = "yes" ] && SARGS+=(--verbose)
[ "$NOGUI" = "yes" ] && SARGS+=(--nogui)

vecho "Launching shoreside with args: ${SARGS[*]}"
./launch_shoreside.sh "${SARGS[@]}" || exit 1

if [ "${JUST_MAKE}" = "yes" ]; then
  echo "$ME: Targ files made for $MODE mode; exiting without launch."
  exit 0
fi

#------------------------------------------------------------
#  Part 8: Unless automation launched, open one uMAC session
#------------------------------------------------------------
if [ "${XLAUNCHED}" != "yes" ]; then
  uMAC --paused targ_shoreside.moos
  trap "" SIGINT
  echo; echo "$ME: Halting all apps"
  kill -- -$$
fi

exit 0
