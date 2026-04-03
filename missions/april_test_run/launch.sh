#!/bin/bash -e
#--------------------------------------------------------------
#   Script: launch_shoreside.sh                                    
#   Author: Michael Benjamin  
#     Date: April 2020     
#--------------------------------------------------------------  
#  Part 1: Set Exit actions and declare global var defaults
#--------------------------------------------------------------
trap "kill -- -$$" EXIT SIGTERM SIGHUP SIGINT SIGKILL

TIME_WARP=1
TIME_WARP_SET="no"
JUST_MAKE="no"
SIM="no"
VERBOSE="no"
PASS_ARGS=()

#--------------------------------------------------------------  
#  Part 2: Check for and handle command-line arguments
#--------------------------------------------------------------  
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
	echo "launch.sh [SWITCHES] [time_warp]    "
	echo "  --help, -h                       " 
	echo "  --just_make, -j                  " 
	echo "  --verbose, -v                    "
	echo "  --pavlab, -p                     "
	echo "    Set region to be MIT pavlab    "
	echo "  --sim, -s                        "
	echo "    Launch the vehicle in sim mode "
	exit 0;
    elif [[ "${ARGI}" =~ ^[0-9]+$ ]] && [ "${TIME_WARP_SET}" = "no" ]; then
        TIME_WARP=$ARGI
        TIME_WARP_SET="yes"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ] ; then
	JUST_MAKE="yes"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--pavlab" -o "${ARGI}" = "-p" ]; then
        PASS_ARGS+=("${ARGI}")
    elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
        SIM="yes"
    else 
        echo "launch.sh Bad arg:" $ARGI " Exiting with code: 1"
        exit 1
    fi
done


#--------------------------------------------------------------  
#  Part 3: Pre-launch. Better to exit now if err building targs
#--------------------------------------------------------------  
PRE_VERBOSE_ARGS=()
RUN_VERBOSE_ARGS=()
if [ "${VERBOSE}" = "yes" ]; then
    if [ "${JUST_MAKE}" = "yes" ]; then
        PRE_VERBOSE_ARGS=(--verbose)
    else
        RUN_VERBOSE_ARGS=(--verbose)
    fi
fi

SHORE_PRE_ARGS=(-j --auto "${PRE_VERBOSE_ARGS[@]}" "${PASS_ARGS[@]}" "${TIME_WARP}")
VEHICLE_PRE_ARGS=(-j --auto "${PRE_VERBOSE_ARGS[@]}" "${PASS_ARGS[@]}" "${TIME_WARP}" --vname=pearl)
if [ "${SIM}" = "yes" ]; then
    VEHICLE_PRE_ARGS=(--sim "${VEHICLE_PRE_ARGS[@]}")
fi

./launch_shoreside.sh "${SHORE_PRE_ARGS[@]}"
./launch_vehicle.sh   "${VEHICLE_PRE_ARGS[@]}"

if [ "${JUST_MAKE}" = "yes" ] ; then
    exit 0
fi

#--------------------------------------------------------------  
#  Part 4: Actual launch
#--------------------------------------------------------------  
SHORE_RUN_ARGS=(--auto "${RUN_VERBOSE_ARGS[@]}" "${PASS_ARGS[@]}" "${TIME_WARP}")
VEHICLE_RUN_ARGS=(--auto "${RUN_VERBOSE_ARGS[@]}" "${PASS_ARGS[@]}" "${TIME_WARP}" --vname=pearl)
if [ "${SIM}" = "yes" ]; then
    VEHICLE_RUN_ARGS=(--sim "${VEHICLE_RUN_ARGS[@]}")
fi

./launch_shoreside.sh "${SHORE_RUN_ARGS[@]}"
sleep 1
./launch_vehicle.sh   "${VEHICLE_RUN_ARGS[@]}"

#--------------------------------------------------------------  
#  Part 5: Launch uMAC until mission quit
#--------------------------------------------------------------  
uMAC targ_shoreside.moos
