#!/bin/bash
#------------------------------------------------------------
#   Script: launch_shoreside.sh
#  Mission: april_test_run
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
JUST_MAKE="no"
VERBOSE=""
AUTO_LAUNCHED="no"

REGION="charles_river"
IP_ADDR="localhost"
MOOS_PORT="9000"
PSHARE_PORT="9300"

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
        echo "                                               "
        echo "  --auto, -a                                   "
        echo "    Auto-launched by a script.                 "
        echo "    Will not launch uMAC as the final step.    "
        echo "                                               "
        echo "  --ip=<localhost>                             "
        echo "    Force pHostInfo to use this IP address     "
        echo "  --mport=<9000>                               "
        echo "    Port number of this MOOSDB port            "
        echo "  --pshare=<9300>                              "
        echo "    Port number of this pShare port            "
        echo "  --charles_river   Set region to Charles River"
        echo "  --pavlab, -p      Set region to MIT pavlab   "
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
        AUTO_LAUNCHED="yes"
    elif [ "${ARGI}" = "--charles_river" ]; then
        REGION="charles_river"
    elif [ "${ARGI}" = "--pavlab" -o "${ARGI}" = "-p" ]; then
        REGION="pavlab"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR="${ARGI#--ip=*}"
    elif [ "${ARGI:0:8}" = "--mport=" ]; then
        MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:8}" = "--sport=" ]; then
        MOOS_PORT="${ARGI#--sport=*}"
    elif [ "${ARGI:0:10}" = "--slisten=" ]; then
        PSHARE_PORT="${ARGI#--slisten=*}"
    else
        echo "${ME}: Bad Arg:[${ARGI}]. Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then
    echo "=================================="
    echo "  launch_shoreside.sh SUMMARY     "
    echo "=================================="
    echo "${ME}                               "
    echo "CMD_ARGS =      [${CMD_ARGS}]     "
    echo "TIME_WARP =     [${TIME_WARP}]    "
    echo "JUST_MAKE =     [${JUST_MAKE}]    "
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}]"
    echo "----------------------------------"
    echo "REGION =        [${REGION}]       "
    echo "IP_ADDR =       [${IP_ADDR}]      "
    echo "MOOS_PORT =     [${MOOS_PORT}]    "
    echo "PSHARE_PORT =   [${PSHARE_PORT}]  "
    echo "----------------------------------"
    echo -n "Hit any key to continue launch "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 5: Create the shoreside mission file
#------------------------------------------------------------
NSFLAGS="--strict --force"
if [ "${AUTO_LAUNCHED}" = "no" ]; then
    NSFLAGS="--interactive --force"
fi

nsplug meta_shoreside.moos targ_shoreside.moos $NSFLAGS WARP=$TIME_WARP \
       REGION=$REGION                SHORE_IP=$IP_ADDR      \
       SPORT=$MOOS_PORT              SHARE_LISTEN=$PSHARE_PORT

if [ "${JUST_MAKE}" = "yes" ]; then
    echo "${ME}: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 6: Launch the shoreside MOOS community
#------------------------------------------------------------
echo "Launching Shoreside MOOS Community. WARP=$TIME_WARP"
pAntler targ_shoreside.moos >& /dev/null &
echo "Done Launching Shoreside Community"

#------------------------------------------------------------
#  Part 7: If launched from script, we're done, exit now
#------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

#------------------------------------------------------------
#  Part 8: Launch uMAC until the mission is quit
#------------------------------------------------------------
uMAC targ_shoreside.moos
trap "" SIGINT
kill -- -$$
