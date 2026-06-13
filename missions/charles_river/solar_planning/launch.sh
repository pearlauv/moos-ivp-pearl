#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: solar_planning
#   Author: Charles Benjamin
#   LastEd: Apr 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "${VERBOSE}" != "" ]; then echo "${ME}: $1"; fi }
on_exit() { echo; echo "${ME}: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=$(basename "$0")
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""

REGION="charles_river"
SIM_FLAG="--sim"
LAUNCH_MODE="SIM"
NOGUI=""
SHORE_MPORT="9000"
VEH_MPORT="9001"
SHORE_PSHARE="9300"
VEH_PSHARE="9301"

START_POS="10,-9,0"
VNAME="pearl"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "${ME} [OPTIONS] [time_warp]                    "
        echo "                                               "
        echo "Options:                                       "
        echo "  --help, -h         Show this help message    "
        echo "  --verbose, -v      Verbose, confirm launch   "
        echo "  --just_make, -j    Only create targ files    "
        echo "  --nogui            Accepted for headless targ generation"
        echo "  --shore_mport=N    Shoreside MOOSDB port     "
        echo "  --veh_mport=N      Vehicle MOOSDB port       "
        echo "  --shore_pshare=N   Shoreside pShare port     "
        echo "  --veh_pshare=N     Vehicle pShare port       "
        echo "                                               "
        echo "Options (custom):                              "
        echo "  --sim, -s          Launch the vehicle in sim "
        echo "  --pavlab, -p       Set region to MIT pavlab  "
        echo "  --startpos=X,Y[,H] Override sim start pos    "
        echo "  --start_pos=X,Y[,H] Alias for --startpos     "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--nogui" ]; then
        NOGUI=$ARGI
    elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
        SIM_FLAG="--sim"
        LAUNCH_MODE="SIM"
    elif [ "${ARGI}" = "--pavlab" -o "${ARGI}" = "-p" ]; then
        REGION="pavlab"
    elif [ "${ARGI:0:11}" = "--startpos=" ]; then
        START_POS="${ARGI#--startpos=*}"
    elif [ "${ARGI:0:12}" = "--start_pos=" ]; then
        START_POS="${ARGI#--start_pos=*}"
    elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
        SHORE_MPORT="${ARGI#--shore_mport=*}"
    elif [ "${ARGI:0:12}" = "--veh_mport=" ]; then
        VEH_MPORT="${ARGI#--veh_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:13}" = "--veh_pshare=" ]; then
        VEH_PSHARE="${ARGI#--veh_pshare=*}"
    else
        echo "${ME}: Bad arg: ${ARGI}. Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  ${ME} SUMMARY                              "
    echo "============================================"
    echo "CMD_ARGS =      [${CMD_ARGS}]                "
    echo "TIME_WARP =     [${TIME_WARP}]               "
    echo "JUST_MAKE =     [${JUST_MAKE}]               "
    echo "VERBOSE =       [${VERBOSE}]                 "
    echo "--------------------------------------------"
    echo "REGION =        [${REGION}]                  "
    echo "LAUNCH_MODE =   [${LAUNCH_MODE}]             "
    echo "VNAME =         [${VNAME}]                   "
    echo "START_POS =     [${START_POS}]               "
    echo "SHORE_MPORT =   [${SHORE_MPORT}]             "
    echo "VEH_MPORT =     [${VEH_MPORT}]               "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}]            "
    echo "VEH_PSHARE =    [${VEH_PSHARE}]              "
    echo "                                            "
    echo -n "Hit any key to continue launch           "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 5: Launch the vehicle mission file
#------------------------------------------------------------
VARGS=" --auto --mport=${VEH_MPORT} --pshare=${VEH_PSHARE} --shore_pshare=${SHORE_PSHARE} "
VARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE $NOGUI "
VARGS+=" --vname=$VNAME "
VARGS+=" --start_pos=$START_POS "
if [ "${SIM_FLAG}" != "" ]; then
    VARGS+=" ${SIM_FLAG} "
fi
if [ "${REGION}" = "pavlab" ]; then
    VARGS+=" --pavlab "
fi
vecho "Launching vehicle: $VARGS"
./launch_vehicle.sh $VARGS

#------------------------------------------------------------
#  Part 6: Launch the shoreside mission file
#------------------------------------------------------------
SARGS=" --auto --mport=${SHORE_MPORT} --pshare=${SHORE_PSHARE} "
SARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE $NOGUI "
if [ "${REGION}" = "pavlab" ]; then
    SARGS+=" --pavlab "
fi
vecho "Launching shoreside: $SARGS"
./launch_shoreside.sh $SARGS

if [ "${JUST_MAKE}" != "" ]; then
    echo "${ME}: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 7: Launch uMAC until mission quit
#------------------------------------------------------------
uMAC targ_shoreside.moos
trap "" SIGINT
echo
echo "${ME}: Halting all apps"
kill -- -$$

exit 0
