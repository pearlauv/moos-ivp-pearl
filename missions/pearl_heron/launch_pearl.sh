#!/bin/bash
#-------------------------------------------------------------- 
#   Script: launch_vehicle.sh                                    
#  Mission: legrun_pavlab
#   Author: Mike Benjamin
#   LastEd: Oct 2022
#-------------------------------------------------------------- 
#  Part 1: Define a convenience function for producing terminal
#          debugging/status output depending on the verbosity.
#-------------------------------------------------------------- 
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

#--------------------------------------------------------------
#  Part 2: Declare global var defaults
#--------------------------------------------------------------
ME=`basename "$0"`
GRN=$(tput setaf 2) # Green 
NC=$(tput setaf 0)  # Reset

TIME_WARP=1
JUST_MAKE="no"
VERBOSE="no"
AUTO_LAUNCHED="no"
CMD_ARGS=""

SHORE_IP="192.168.1.112"
SHORE_PSHARE="9200"
REGION="pavlab"

COLOR="coral"
MEDIATED="yes"

XMODE0="SIM"

IP_ADDR1="192.168.1.94"
MOOS_PORT1="9002"
PSHARE_PORT1="9202"
VNAME1="pearl"
INDEX1="2"
XMODE1="SIM"
START_POS1="-85,-49,64"
SPEED1="0.5"
RETURN_POS1="5,0"
MAXSPD1="2"
VLANE1=arlo



#--------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#--------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME [OPTIONS] [time_warp]                        "
	echo "                                                 " 
	echo "Options:                                         "
	echo "  --help, -h                                     " 
	echo "    Print this help message and exit             "
	echo "  --just_make, -j                                " 
	echo "    Just make targ files, but do not launch      "
	echo "  --verbose, -v                                  " 
	echo "    Verbose output, confirm before launching     "
        echo "  --auto, -a                                     "
        echo "     Auto-launched by a script.                  "
        echo "     Will not launch uMAC as the final step.     "
	echo "                                                 "
	echo "  --ip=<localhost>                               " 
	echo "    Force pHostInfo to use this IP Address       "
	echo "  --mport=<9001>                                 "
	echo "    Port number of this vehicle's MOOSDB port    "
	echo "  --pshare=<9201>                                " 
	echo "    Port number of this vehicle's pShare port    "
	echo "                                                 "
	echo "  --shore=<localhost>                            " 
	echo "    IP address location of shoreside             "
	echo "  --shore_pshare=<9200>                          " 
	echo "    Port on which shoreside pShare is listening  "
	echo "  --vname=<abe>                                  " 
	echo "    Name of the vehicle being launched           " 
	echo "                                                 "
	echo "  --nomediate, -nm                               " 
	echo "    No use of pMediator for inter-vessel comms   "
	echo "  --region=<region>                              " 
	echo "    Name of the region of operation              "
	echo "                                                 "
	echo "  --start=<X,Y>     (default is 0,0)             " 
	echo "    Start position chosen by script launching    "
	echo "    this script (to ensure separation)           "
	echo "  --speed=meters/sec                             " 
	echo "    The speed use for transiting/loitering       "
	echo "  --maxspd=meters/sec                            " 
	echo "    Max speed of vehicle (for sim and in-field)  "
	echo "                                                 "
	echo "  --vlane=arlo                                   " 
	echo "    Specify a lane explicitly                    "
	echo "                                                 "
	echo "  --sim, -s: This is simultion not robot         "
	exit 0;
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
	JUST_MAKE="yes"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
        AUTO_LAUNCHED="yes" 
    elif [ "${ARGI}" = "--logclean" -o "${ARGI}" = "-l" ]; then
	LOG_CLEAN="yes"

    elif [ "${ARGI:0:9}" = "--region=" ]; then
        REGION="${ARGI#--region=*}"
    elif [ "${ARGI}" = "--nomediate" -o "${ARGI}" = "-nm" ]; then
	MEDIATED="no"

    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR0="${ARGI#--ip=*}"
    elif [ "${ARGI:0:7}" = "--mport" ]; then
	MOOS_PORT0="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT0="${ARGI#--pshare=*}"

    elif [ "${ARGI:0:8}" = "--shore=" ]; then
        SHORE_IP="${ARGI#--shore=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:8}" = "--vname=" ]; then
        VNAME0="${ARGI#--vname=*}"
    elif [ "${ARGI:0:8}" = "--color=" ]; then
        COLOR="${ARGI#--color=*}"
    elif [ "${ARGI:0:8}" = "--index=" ]; then
        INDEX0="${ARGI#--index=*}"

    elif [ "${ARGI:0:8}" = "--start=" ]; then
        START_POS0="${ARGI#--start=*}"
    elif [ "${ARGI:0:8}" = "--speed=" ]; then
        SPEED0="${ARGI#--speed=*}"
    elif [ "${ARGI:0:9}" = "--maxspd=" ]; then
        MAXSPD0="${ARGI#--maxspd=*}"

    elif [ "${ARGI:0:8}" = "--vlane=" ]; then
        VLANE0="${ARGI#--vlane=*}"

    elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
        XMODE0="SIM"
        echo "Simulation mode ON."
    else
	echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
	exit 1
    fi
done

#--------------------------------------------------------------
#  Part 3: If Heron hardware, set key info based on IP address
#--------------------------------------------------------------
if [ "${XMODE0}" = "M300" ]; then
    COLOR=`./get_heron_info.sh --color --hint=$COLOR`
    IP_ADDR0=`./get_heron_info.sh --ip`
    FSEAT_IP=`./get_heron_info.sh --fseat`
    VNAME0=`./get_heron_info.sh --name`
    if [ $? != 0 ]; then
	echo "$ME: Problem getting Heron Info. Exit Code 2"
	exit 2
    fi
fi
     
#---------------------------------------------------------------
#  Part 4: If verbose, show vars and confirm before launching
#---------------------------------------------------------------
if [ "${VERBOSE}" = "yes" ]; then 
    echo "=================================="
    echo "   launch_vehicle.sh SUMMARY      "
    echo "=================================="
    echo "$ME"
    echo "CMD_ARGS =      [${CMD_ARGS}]     "
    echo "TIME_WARP =     [${TIME_WARP}]    "
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}]"
    echo "JUST_MAKE =     [${JUST_MAKE}]    "
    echo "----------------------------------"
    echo "IP_ADDR =       [${IP_ADDR0}]      "
    echo "MOOS_PORT =     [${MOOS_PORT0}]    "
    echo "PSHARE_PORT =   [${PSHARE_PORT0}]  "
    echo "----------------------------------"
    echo "SHORE_IP =      [${SHORE_IP}]     "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}] "
    echo "VNAME =         [${VNAME0}]        "
    echo "COLOR =         [${COLOR}]        "
    echo "----------------------------------"
    echo "XMODE =         [${XMODE0}]        "
    echo "FSEAT_IP =      [${FSEAT_IP}]     "
    echo "----------------------------------"
    echo "START_POS =     [${START_POS0}]    "
    echo "SPEED =         [${SPEED0}]        "
    echo "MAXSPD =        [${MAXSPD0}]       "
    echo "----------------------------------"
    echo "REGION =        [${REGION}]       "
    echo "VLANE =         [${VLANE0}]         "
    echo "MEDIATED =      [${MEDIATED}]     "
    echo -n "Hit any key to continue with launching ${VNAME0}"
    read ANSWER
fi


#-------------------------------------------------------
#  Part 5: If Log clean before launch, do it now. 
#          In MTASC missions, remote cleaning is essential.
#-------------------------------------------------------
if [ "$LOG_CLEAN" = "yes" ]; then
    vecho "Cleaning local Log Files"
    rm -rf LOG* XLOG* MOOSLog*
fi


#--------------------------------------------------------------
#  Part 6: Create the .moos and .bhv files. 
#--------------------------------------------------------------
NSFLAGS="-s -f"
if [ "${AUTO}" = "" ]; then
    NSFLAGS="-i -f"
fi

nsplug meta_vehicle.moos targ_$VNAME1.moos $NSFLAGS WARP=$TIME_WARP \
       PSHARE_PORT=$PSHARE_PORT1     VNAME=$VNAME1               \
       IP_ADDR=$IP_ADDR1            SHORE_IP=$SHORE_IP         \
       SHORE_PSHARE=$SHORE_PSHARE   MOOS_PORT=$MOOS_PORT1       \
       FSEAT_IP=$FSEAT_IP           XMODE=$XMODE1               \
       MAXSPD=$MAXSPD1               START_POS=$START_POS1       \
       COLOR=$COLOR                 REGION=$REGION

nsplug meta_vehicle.bhv targ_$VNAME1.bhv $NSFLAGS VNAME=$VNAME1  \
       START_POS=$START_POS1         SPEED=$SPEED1               \
       VLANE=$VLANE1                 COLOR=$COLOR

if [ ${JUST_MAKE} = "yes" ] ; then
    echo "Files assembled; nothing launched; exiting per request."
    exit 0
fi


#--------------------------------------------------------------
#  Part 6: Launch the processes
#--------------------------------------------------------------


echo "$GRN Launching $VNAME1 MOOS Community. WARP=$TIME_WARP $NC"
pAntler targ_${VNAME1}.moos >& /dev/null &
echo "$GRN Done Launching $VNAME1 MOOS Community $NC"

#---------------------------------------------------------------
#  Part 7: If launched from script, we're done, exit now
#---------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

#---------------------------------------------------------------
# Part 8: Launch uMAC until the mission is quit
#---------------------------------------------------------------
uMAC targ_$VNAME1.moos
kill -- -$$
