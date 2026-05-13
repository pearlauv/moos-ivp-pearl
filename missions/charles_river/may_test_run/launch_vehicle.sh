#!/bin/bash
#------------------------------------------------------------
#   Script: launch_vehicle.sh
#  Mission: may_test_run
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
#  Part 2: Declare global var defaults
#------------------------------------------------------------
ME=$(basename "$0")
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE="no"
AUTO_LAUNCHED="no"

IP_ADDR="localhost"
MOOS_PORT="9001"
PSHARE_PORT="9301"
SHORE_IP="localhost"
SHORE_PSHARE="9300"

VNAME="pearl"
REGION="charles_river"
XMODE="PEARL"
START_POS="-90,-65,0"
CRUISESPEED="0.5"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "${ME} [OPTIONS] [time_warp]                     "
        echo "                                               "
        echo "Options:                                       "
        echo "  --help, -h         Show this help message    "
        echo "  --just_make, -j    Only create targ files    "
        echo "  --verbose, -v      Verbose, confirm launch   "
        echo "  --auto, -a         Script launch, no uMAC    "
        echo "                                               "
        echo "  --ip=<localhost>   Vehicle IP address        "
        echo "  --mport=<9001>     Vehicle MOOSDB port       "
        echo "  --pshare=<9301>    Vehicle pShare port       "
        echo "  --shore=<localhost> Shoreside IP address     "
        echo "  --shore_pshare=<9300> Shoreside pShare port  "
        echo "                                               "
        echo "  --vname=<pearl>    Vehicle name              "
        echo "  --sim, -s          Sim launch                "
        echo "  --charles_river    Set region to Charles Riv "
        echo "  --pavlab, -p       Set region to MIT pavlab  "
        echo "  --startpos=X,Y[,H] Override sim start pos    "
        echo "  --start_pos=X,Y[,H] Alias for --startpos     "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
        AUTO_LAUNCHED="yes"
    elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
        XMODE="SIM"
    elif [ "${ARGI}" = "--charles_river" ]; then
        REGION="charles_river"
    elif [ "${ARGI}" = "--pavlab" -o "${ARGI}" = "-p" ]; then
        REGION="pavlab"
    elif [ "${ARGI:0:8}" = "--vname=" ]; then
        VNAME="${ARGI#--vname=*}"
    elif [ "${ARGI:0:11}" = "--startpos=" ]; then
        START_POS="${ARGI#--startpos=*}"
    elif [ "${ARGI:0:12}" = "--start_pos=" ]; then
        START_POS="${ARGI#--start_pos=*}"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR="${ARGI#--ip=*}"
    elif [ "${ARGI:0:8}" = "--mport=" ]; then
        MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:8}" = "--shore=" ]; then
        SHORE_IP="${ARGI#--shore=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:8}" = "--vport=" ]; then
        MOOS_PORT="${ARGI#--vport=*}"
    elif [ "${ARGI:0:10}" = "--vlisten=" ]; then
        PSHARE_PORT="${ARGI#--vlisten=*}"
    elif [ "${ARGI:0:10}" = "--slisten=" ]; then
        SHORE_PSHARE="${ARGI#--slisten=*}"
    else
        echo "${ME}: Bad Arg:[${ARGI}]. Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Normalize sim start position to x,y,heading form
#------------------------------------------------------------
if [ "${XMODE}" = "SIM" ]; then
    if [[ "${START_POS}" != *,* ]]; then
        echo "${ME}: Bad --startpos format: ${START_POS}"
        exit 1
    elif [[ "${START_POS}" != *,*,* ]]; then
        START_POS="${START_POS},0"
    fi
fi

#------------------------------------------------------------
#  Part 5: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then
    echo "============================================"
    echo "     launch_vehicle.sh SUMMARY        ${VNAME}"
    echo "============================================"
    echo "${ME}                               "
    echo "CMD_ARGS =      [${CMD_ARGS}]     "
    echo "TIME_WARP =     [${TIME_WARP}]    "
    echo "JUST_MAKE =     [${JUST_MAKE}]    "
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}]"
    echo "----------------------------------"
    echo "IP_ADDR =       [${IP_ADDR}]      "
    echo "MOOS_PORT =     [${MOOS_PORT}]    "
    echo "PSHARE_PORT =   [${PSHARE_PORT}]  "
    echo "SHORE_IP =      [${SHORE_IP}]     "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}] "
    echo "----------------------------------"
    echo "VNAME =         [${VNAME}]        "
    echo "REGION =        [${REGION}]       "
    echo "XMODE =         [${XMODE}]        "
    echo "START_POS =     [${START_POS}]    "
    echo "CRUISESPEED =   [${CRUISESPEED}]  "
    echo "                                  "
    echo -n "Hit any key to continue launching ${VNAME} "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 6: Create the .moos and .bhv files
#------------------------------------------------------------
NSFLAGS="--strict --force"
if [ "${AUTO_LAUNCHED}" = "no" ]; then
    NSFLAGS="--interactive --force"
fi

nsplug meta_vehicle.moos targ_${VNAME}.moos $NSFLAGS WARP=$TIME_WARP \
       VNAME=$VNAME                  REGION=$REGION                \
       START_POS=$START_POS          PEARL_IP=$IP_ADDR            \
       SHORE_IP=$SHORE_IP            VPORT=$MOOS_PORT             \
       SHARE_LISTEN=$PSHARE_PORT     SHORE_LISTEN=$SHORE_PSHARE   \
       XMODE=$XMODE

nsplug meta_vehicle.bhv targ_${VNAME}.bhv $NSFLAGS \
       VNAME=$VNAME              START_POS=$START_POS \
       REGION=$REGION            SPEED=$CRUISESPEED   \
       ORDER="normal"

if [ "${JUST_MAKE}" = "yes" ]; then
    echo "${ME}: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 7: Launch the vehicle mission
#------------------------------------------------------------
echo "Launching ${VNAME} MOOS Community. WARP=$TIME_WARP"
pAntler targ_${VNAME}.moos >& /dev/null &
echo "Done Launching ${VNAME} MOOS Community"

#------------------------------------------------------------
#  Part 8: If launched from script, we're done, exit now
#------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

#------------------------------------------------------------
#  Part 9: Launch uMAC until the mission is quit
#------------------------------------------------------------
uMAC targ_${VNAME}.moos
trap "" SIGINT
kill -- -$$
