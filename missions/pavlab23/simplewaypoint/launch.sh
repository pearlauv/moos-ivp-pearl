#!/bin/bash
#---------------------------------------------------------------
#   Script: launch.sh
#  Mission: legrun_heron
#   Author: Mike Benjamin
#   LastEd: Oct 2022
#-------------------------------------------------------------- 
#  Part 1: Define a convenience function for producing terminal
#          debugging/status output depending on the verbosity.
#-------------------------------------------------------------- 
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

#---------------------------------------------------------------
#  Part 2: Set global var defaults
#---------------------------------------------------------------
ME=`basename "$0"`
TIME_WARP=1
JUST_MAKE=""
VERBOSE=""
VNAME1="abe"
REGION="pavlab"
CLEAN="no"
CMD_ARGS=""

RANDSTART="true"
AMT=1

VNAMES=""
VLAUNCH_ARGS=" --auto --region=$REGION --sim --vname=$VNAME1 --index=1 "
SLAUNCH_ARGS=" --auto --region=$REGION --sim"

#---------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#---------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME [OPTIONS] [time_warp]                      "
	echo "                                               "
	echo "Options:                                       "
	echo "  --help, -h                                   "
	echo "    Show this help message                     "
	echo "  --just_make, -j                              "
	echo "    Just make targ files, no launch            "
	echo "  --verbose, -v                                "
	echo "    Increase verbosity,  confirm before launch "
	echo "  --clean, -c                                  "
	echo "    Run clean.sh and ktm prior to launch       "
	echo "  --norand                                     " 
	echo "    Do not randomly generate vpositions.txt.   "
	echo "  --amt=<N>                                    " 
	echo "    Number of vehicles to launch.              "
	exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
	JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
	VERBOSE=$ARGI
    elif [ "${ARGI}" = "--clean" -o "${ARGI}" = "-cc" ]; then
	CLEAN="yes"
    elif [ "${ARGI:0:6}" = "--amt=" ]; then
        AMT="${ARGI#--amt=*}"
	if [ ! $AMT -ge 1 ]; then
	    echo "$ME: Vehicle amount must be >= 1."
	    exit 1
	fi
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

VLAUNCH_ARGS+=" $VERBOSE $JUST_MAKE $TIME_WARP "
SLAUNCH_ARGS+=" $VERBOSE $JUST_MAKE $TIME_WARP "

#---------------------------------------------------------------
#  Part 4: If verbose, show vars and confirm before launching
#---------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then 
    echo "======================================="
    echo "  launch.sh SUMMARY                    "
    echo "======================================="
    echo "$ME"
    echo "CMD_ARGS =       [${CMD_ARGS}]         "
    echo "TIME_WARP =      [${TIME_WARP}]        "
    echo "AUTO_LAUNCHED =  [${AUTO_LAUNCHED}]    "
    echo "AMT =            [${AMT}]              "
    echo -n "Hit the RETURN key to continue with launching"
    read ANSWER
fi

#-------------------------------------------------------------
# Part 5A: If Cleaning enabled, clean first
#-------------------------------------------------------------
vecho "Running ./clean.sh and ktm prior to launch"
if [ "${CLEAN}" = "yes" ]; then
    ./clean.sh; ktm
fi

#-------------------------------------------------------------
# Part 5B: Generate random starting positions, speeds and vnames
#-------------------------------------------------------------
#vecho "Picking vehicle starting positions."
#if [ "${RANDSTART}" = "true" -o  ! -f "vpositions.txt" ]; then
#    ./pickpos.sh $AMT
#fi

# vehicle names and colors are always deterministic
pickpos --amt=$AMT --vnames  > vnames.txt
pickpos --amt=$AMT --colors  > vcolors.txt

VEHPOS=(`cat vpositions.txt`)
VNAMES=(`cat vnames.txt`)
VLANES=(`cat vlanes.txt`)
COLORS=(`cat vcolors.txt`)

#-------------------------------------------------------------
# Part 5C: Launch the vehicles
#-------------------------------------------------------------
for INDEX in `seq 1 $AMT`;
do
    sleep 0.2
    ARRAY_INDEX=`expr $INDEX - 1`
    START=${VEHPOS[$ARRAY_INDEX]}
    VNAME=${VNAMES[$ARRAY_INDEX]}
    VLANE=${VLANES[$ARRAY_INDEX]}
    COLOR=${COLORS[$ARRAY_INDEX]}

    VNAMES+=":$VNAME"
    
    MOOS_PORT=`expr $INDEX + 9000`
    PSHARE_PORT=`expr $INDEX + 9200`
    
    VLAUNCH_ARGS+=" $NOCONFIRM "
    IX_VLAUNCH_ARGS=$VLAUNCH_ARGS
    IX_VLAUNCH_ARGS+=" --index=$INDEX --start=$START    "
    IX_VLAUNCH_ARGS+=" --mport=$MOOS_PORT "
    IX_VLAUNCH_ARGS+=" --pshare=$PSHARE_PORT "
    IX_VLAUNCH_ARGS+=" --maxspd=$MAXIMUM_SPD "
    IX_VLAUNCH_ARGS+=" --speed=$TRANSIT_SPD " 
    IX_VLAUNCH_ARGS+=" --color=$COLOR --vlane=$VLANE" 
    IX_VLAUNCH_ARGS+=" --vname=$VNAME --shore=localhost "

    ./launch_vehicle.sh $IX_VLAUNCH_ARGS	
done

#---------------------------------------------------------------
#  Part 6: Launch the shoreside
#---------------------------------------------------------------
echo "$ME: Launching Shoreside ..."
./launch_shoreside.sh $SLAUNCH_ARGS --vnames=$VNAMES

#---------------------------------------------------------------
#  Part 7: If launched from script, we're done, exit now
#---------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" -o "${JUST_MAKE}" != "" ]; then
    exit 0
fi

#---------------------------------------------------------------
# Part 8: Launch uMAC until the mission is quit
#---------------------------------------------------------------
uMAC targ_shoreside.moos
kill -- -$$

