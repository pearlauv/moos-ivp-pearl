#!/bin/bash 
#-------------------------------------------------------------- 
#   Script: pickpos.sh
#   Author: Michael Benjamin   
#     Date: Feb 4th 2020       
#    About: This call is common to both launch.sh and rlaunch.sh
#           One stop location for changing the polygons
#-------------------------------------------------------------- 

AMT=1
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
	echo "pickpos.sh [AMOUNT]           " 
	echo "  --help, -h                       " 
	exit 0;
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$AMT" = 1 ]; then 
        AMT=$ARGI
    else 
	echo "Bad Argument: $ARGI"
	exit 0
    fi
done

if [ ! $AMT -ge 0 ] ; then
    echo "Vehicle amount must be >= 0. Exiting now."
    exit 1
fi


#pickpos --poly="21,-6:28,-25:77,-4:66,14" --amt=$AMT --hdg=0:359  > vpositions.txt  

#pickpos --poly="10,-15 : 30,-40 : 100,0 : 75,15" --buffer=8 --amt=$AMT --hdg=0:359  > vpositions.txt  

pickpos --poly="0,-15:0,-40:50,-14:50,10" --buffer=8 --amt=$AMT --hdg=0:359  > vpositions.txt  

# pickpos --poly="95,-35:95,-75:125,-75:125,-35" --buffer=8 --amt=$AMT --hdg=0:359  > vpositions.txt  
