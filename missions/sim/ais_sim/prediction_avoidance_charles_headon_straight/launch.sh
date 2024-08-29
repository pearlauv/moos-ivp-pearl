#!/bin/bash -e

#----------------------------------------------------------
#  Script: launch.sh
#  Author: Michael Benjamin
#  LastEd: May 17th 2019
#----------------------------------------------------------
#  Part 1: Set global var defaults
#----------------------------------------------------------
TIME_WARP=1
JUST_MAKE="no"

VNAME1="henry"           # The first vehicle Community
VNAME2="gilda"           # The second vehicle Community
VNAME3="brian"
VNAME4="julie"
VTYPE1="kayak"
VTYPE2="mokai"
VTYPE3="kayak"
VTYPE4="mokai"
START_POS1="-80,-60,70"     
START_POS2="150,-37,-90"  
START_POS3="-80,-60,70"     
START_POS4="150,-37,-90"      
LOITER_POS1="x=0,y=-75"
LOITER_POS2="x=125,y=-50"
SHORE_PSHARE="9200"

#----------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#----------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "launch.sh [SWITCHES] [time_warp]    " 
	echo "  --help, -h           Show this help message            " 
	echo "  --just_make, -j      Just create targ files, no launch " 
	echo "  --fast, -f           Init positions for fast encounter " 
	exit 0;
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ] ; then
	JUST_MAKE="yes"
    elif [ "${ARGI}" = "--fast" -o "${ARGI}" = "-f" ] ; then
	START_POS1="170,-80,270"         
	START_POS2="-30,-80,90"        
	LOITER_POS1="x=0,y=-95"
	LOITER_POS2="x=125,y=-65"
    else 
        echo "launch.sh Bad arg:" $ARGI " Exiting with code: 1"
        exit 1
    fi
done

#----------------------------------------------------------
#  Part 3: Create the .moos and .bhv files. 
#----------------------------------------------------------
nsplug meta_vehicle.moos targ_henry.moos -i -f WARP=$TIME_WARP \
       VNAME=$VNAME1            BOT_PSHARE="9201"              \
       BOT_MOOSDB="9001"        SHORE_PSHARE=$SHORE_PSHARE     \
       VTYPE=$VTYPE1            START_POS=$START_POS1  

nsplug meta_vehicle.moos targ_gilda.moos -i -f WARP=$TIME_WARP \
       VNAME=$VNAME2            BOT_PSHARE="9202"              \
       BOT_MOOSDB="9002"        SHORE_PSHARE=$SHORE_PSHARE     \
       VTYPE=$VTYPE2            START_POS=$START_POS2  

nsplug meta_vehicle.moos targ_brian.moos -i -f WARP=$TIME_WARP \
       VNAME=$VNAME3            BOT_PSHARE="9203"              \
       BOT_MOOSDB="9003"        SHORE_PSHARE=$SHORE_PSHARE     \
       VTYPE=$VTYPE3            START_POS=$START_POS3 
       
nsplug meta_vehicle.moos targ_julie.moos -i -f WARP=$TIME_WARP \
       VNAME=$VNAME4            BOT_PSHARE="9204"              \
       BOT_MOOSDB="9004"        SHORE_PSHARE=$SHORE_PSHARE     \
       VTYPE=$VTYPE4            START_POS=$START_POS4 

nsplug meta_shoreside.moos targ_shoreside.moos -i -f WARP=$TIME_WARP \
       SHORE_PSHARE=$SHORE_PSHARE  SHORE_MOOSDB="9000"        \
       VNAMES=$VNAME1:$VNAME2:$VNAME3:$VNAME4

nsplug meta_vehicle.bhv targ_henry.bhv -i -f VNAME=$VNAME1    \
       START_POS=$START_POS1 LOITER_POS=$LOITER_POS1       

nsplug meta_vehicle.bhv targ_gilda.bhv -i -f VNAME=$VNAME2    \
       START_POS=$START_POS2 LOITER_POS=$LOITER_POS2       

nsplug meta_vehicle.bhv targ_brian.bhv -i -f VNAME=$VNAME3    \
       START_POS=$START_POS3 LOITER_POS=$LOITER_POS3 

nsplug meta_vehicle.bhv targ_julie.bhv -i -f VNAME=$VNAME4    \
       START_POS=$START_POS4 LOITER_POS=$LOITER_POS4 

if [ ${JUST_MAKE} = "yes" ] ; then
    echo "Files assembled; nothing launched; exiting per request."
    exit 0
fi

#----------------------------------------------------------
#  Part 4: Launch the processes
#----------------------------------------------------------
echo "Launching Shoreside MOOS Community. WARP is" $TIME_WARP
pAntler targ_shoreside.moos >& /dev/null &

echo "Launching $VNAME1 MOOS Community. WARP is" $TIME_WARP
pAntler targ_henry.moos >& /dev/null &

echo "Launching $VNAME2 MOOS Community. WARP is" $TIME_WARP
pAntler targ_gilda.moos >& /dev/null &

echo "Launching $VNAME3 MOOS Community. WARP is" $TIME_WARP
pAntler targ_brian.moos >& /dev/null &

echo "Launching $VNAME4 MOOS Community. WARP is" $TIME_WARP
pAntler targ_julie.moos >& /dev/null &

uMAC targ_shoreside.moos

kill -- -$$
