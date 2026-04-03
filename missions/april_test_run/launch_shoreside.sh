#!/bin/bash -e
#--------------------------------------------------------------
#   Script: launch_shoreside.sh                                    
#   Author: Michael Benjamin  
#     Date: April 2020     
#--------------------------------------------------------------  
#  Part 1: Declare global var defaults
#--------------------------------------------------------------
ME=$(basename "$0")
CMD_ARGS=""
TIME_WARP=1
TIME_WARP_SET="no"
JUST_MAKE="no"
VERBOSE="no"
AUTO=""
REGION="charles_river"
SHORE_IP="localhost"  #IP address of the shoreside laptop
SHORESIDE_PORT="9000"
SHORE_LISTEN="9300"

#--------------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#--------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
	echo "launch_shoreside.sh [SWITCHES] [time_warp]        "
	echo "  --just_make, -j                                 "
	echo "  --verbose, -v                                   "
	echo "  --auto, -a        Auto-launched. uMAC not used. "
	echo "  --charles_river       Set region to be Charles River (Default is Charles River)"
	echo "  --ip=<addr>       (Default is 192.168.20.158)   "
	echo "  --sport=<port>    (Default is 9000)             "
	echo "  --slisten=<port>  (Default is 9300)             " 
        echo "  --help, -h                                      "
	exit 0;
    elif [[ "${ARGI}" =~ ^[0-9]+$ ]] && [ "${TIME_WARP_SET}" = "no" ]; then
        TIME_WARP=$ARGI
        TIME_WARP_SET="yes"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ] ; then
	JUST_MAKE="yes"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
        AUTO="yes"
    elif [ "${ARGI}" = "--charles_river" ]; then
        REGION="charles_river"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        SHORE_IP="${ARGI#--ip=*}"
    elif [ "${ARGI:0:8}" = "--sport=" ]; then
	SHORESIDE_PORT="${ARGI#--sport=*}"
    elif [ "${ARGI:0:10}" = "--slisten=" ]; then
        SHORE_LISTEN="${ARGI#--slisten=*}"
    else 
	echo "launch_shoreside.sh: Bad Arg: " $ARGI
	exit 1
    fi
done

#--------------------------------------------------------------
#  Part 3: If verbose, show vars and confirm before launching
#--------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then
    echo "=================================="
    echo "  launch_shoreside.sh SUMMARY     "
    echo "=================================="
    echo "${ME}"
    echo "CMD_ARGS =      [${CMD_ARGS}]"
    echo "TIME_WARP =     [${TIME_WARP}]"
    echo "JUST_MAKE =     [${JUST_MAKE}]"
    echo "AUTO =          [${AUTO}]"
    echo "----------------------------------"
    echo "REGION =        [${REGION}]"
    echo "SHORE_IP =      [${SHORE_IP}]"
    echo "SPORT =         [${SHORESIDE_PORT}]"
    echo "SHARE_LISTEN =  [${SHORE_LISTEN}]"
    echo "----------------------------------"
    echo -n "Hit any key to continue launch "
    read ANSWER
fi

#--------------------------------------------------------------
#  Part 4: Create the .moos and .bhv files using nsplug
#--------------------------------------------------------------
# What is nsplug? Type "nsplug --help" or "nsplug --manual"

NSFLAGS="-s -f"
if [ "${AUTO}" = "" ]; then
    NSFLAGS="-i -f"
fi
nsplug meta_shoreside.moos targ_shoreside.moos $NSFLAGS   \
       WARP=$TIME_WARP             \
       REGION=$REGION              \
       SHORE_IP=$SHORE_IP          \
       SHARE_LISTEN=$SHORE_LISTEN  \
       SPORT=$SHORESIDE_PORT       

if [ "${JUST_MAKE}" = "yes" ] ; then
    exit 0
fi

#--------------------------------------------------------------
#  Part 5: Launch the processes
#--------------------------------------------------------------
echo "Launching Shoreside MOOS Community WARP:" $TIME_WARP
pAntler targ_shoreside.moos >& /dev/null &
echo "Done launching shoreside"

#-------------------------------------------------------------- 
#  Part 6: Unless auto-launched, launch uMAC until mission quit          
#-------------------------------------------------------------- 
if [ "${AUTO}" = "" ]; then
    uMAC targ_shoreside.moos
    kill -- -$$
fi
