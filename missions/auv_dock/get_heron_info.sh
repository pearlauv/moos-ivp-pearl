#!/bin/bash
#--------------------------------------------------------
# Script: get_heron_info.sh
#   Date: Apr 17th, 2023
#     By: Mike Benjamin
#  About: A script for getting the pablo heron info based
#         on the IP address. When pablos are connected to 
#         Herons, they have one of a known set of IP addrs
#--------------------------------------------------------
#  Part 1: A convenience function for producing terminal
#          debugging/status output depending on verbosity.
#--------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

#--------------------------------------------------------
#  Part 2: Initialize global variables
#--------------------------------------------------------
ME=`basename "$0"`
VNAME=""
FSEAT=""
COLOR="yellow"
RPOINT="26,-2"
ACTION="name"
HINT_COLOR="coral"

#--------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#--------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME  [OPTIONS]                                       "
        echo "                                                     "
        echo "Synopsis:                                            "
        echo "  Get a Pablo info based on Heron IP Address. When a "
        echo "  pablo is connected to the frontseat of a Heron it  "
        echo "  will have a set IP address specific to the vname.  "
        echo "  This script will get the IP address from the system"
        echo "  answer either (a) the IP address itself, (b) the   "
        echo "  vehicle name, or (c) the front seat IP address from"
        echo "  the perspective of the pablo.                      "
        echo "  These three pieces of info are all need within the "
        echo "  the launch_vehicle.sh scripts for in-water missions"
        echo "                                                     "
        echo "  --help, -h            Display this help message    "
	echo "  --name, -n            Get vehicle name             "
	echo "  --fseat, -f           Get Frontseat IP address     "
	echo "  --color, -c           Get Vehicle color            "
	echo "  --return, -r          Get Vehicle return point     "
	echo "  --ip, -ip             Get IP address               "
	echo "  --hint=<value>        Non-empty hint is the answer "
        echo "                                                     "
        echo "Returns:                                             "
        echo "  0: Success                                         "
        echo "  1: Bad cmd line argument                           "
        echo "  2: Linux hostname did not succeed                  "
        echo "  3: Detected IP address not related to a heron      "
        echo "                                                     "
        echo "Returns:                                             "
        echo "  0: Success                                         " 
        exit 0;
    elif [ "${ARGI}" = "--name" -o "${ARGI}" = "-n" ]; then
	ACTION="name"
    elif [ "${ARGI}" = "--fseat" -o "${ARGI}" = "-f" ]; then
	ACTION="fseat"
    elif [ "${ARGI}" = "--color" -o "${ARGI}" = "-c" ]; then
	ACTION="color"
    elif [ "${ARGI}" = "--return" -o "${ARGI}" = "-r" ]; then
	ACTION="return"
    elif [ "${ARGI}" = "--ip" -o "${ARGI}" = "-ip" ]; then
	ACTION="ip"
    elif [ "${ARGI:0:7}" = "--hint=" ]; then
        HINT_COLOR="${ARGI#--hint=*}"
    else
	echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
	exit 1
    fi
done

#---------------------------------------------------------------
# Silently check that hostname succeeds and return "" if so.
# Should work on all linux systems
#---------------------------------------------------------------
hostname -I >& /dev/null
if [ $? != 0 ]; then
    echo "hostname -I failed. Exit Code 2"
    exit 2
fi

IP_ADDR=`hostname -I | cut -d ' ' -f 1`

#vecho "ip_addr:[$IP_ADDR]"

#---------------------------------------------------------------
# Part 3: Match IP address to Heron name
#---------------------------------------------------------------
if [ "${IP_ADDR}" = "192.168.14.100" ]; then
    VNAME="abe";
    FSEAT="192.168.14.1"
    COLOR="light_blue"
    RPOINT="52,9"
elif [ "${IP_ADDR}" = "192.168.15.100" ]; then
    VNAME="ben";
    FSEAT="192.168.15.1"
    COLOR="dark_green"
    RPOINT="39,4"
elif [ "${IP_ADDR}" = "192.168.16.100" ]; then
    VNAME="cal";
    FSEAT="192.168.16.1"
    COLOR="white"
    RPOINT="29,0"
elif [ "${IP_ADDR}" = "192.168.17.100" ]; then
    VNAME="deb";
    FSEAT="192.168.17.1"
    COLOR="purple"
    RPOINT="16,-6"
elif [ "${IP_ADDR}" = "192.168.18.100" ]; then
    VNAME="eve";
    FSEAT="192.168.18.1"
    COLOR="orange"
    RPOINT="4,-11"
elif [ "${IP_ADDR}" = "192.168.19.100" ]; then
    VNAME="fin";
    FSEAT="192.168.19.1"
    COLOR="dodger_blue"
    RPOINT="2,-15"
elif [ "${IP_ADDR}" = "192.168.20.100" ]; then
    VNAME="max";
    FSEAT="192.168.20.1"
    COLOR="yellow"
    RPOINT="26,-2"
elif [ "${IP_ADDR}" = "192.168.21.100" ]; then
    VNAME="ned";
    FSEAT="192.168.21.1"
    COLOR="red"
    RPOINT="12,-8"
elif [ "${IP_ADDR}" = "192.168.22.100" ]; then
    VNAME="oak";
    FSEAT="192.168.22.1"
    COLOR="magenta"
    RPOINT="14,-10"
fi

#---------------------------------------------------------------
# Part 4: Depending on the action, output results
#---------------------------------------------------------------
if [ "${ACTION}" = "name" ]; then
    echo -n $VNAME
elif [ "${ACTION}" = "fseat" ]; then
    echo -n $FSEAT
elif [ "${ACTION}" = "ip" ]; then
    echo -n $IP_ADDR
elif [ "${ACTION}" = "color" ]; then
    if [ "${HINT_COLOR}" != "coral" ]; then
	echo -n $HINT_COLOR
    else
	echo -n $COLOR
    fi
elif [ "${ACTION}" = "return" ]; then
    echo -n $RPOINT
else
    echo -n ""
    exit 3
fi

exit 0
