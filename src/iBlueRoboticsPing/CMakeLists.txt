#--------------------------------------------------------
# The CMakeLists.txt for:                       iBlueRoboticsPing
# Author(s):                              
#--------------------------------------------------------

SET(SRC
  BlueRoboticsPing.cpp
  BlueRoboticsPing_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(iBlueRoboticsPing ${SRC})


TARGET_LINK_LIBRARIES(iBlueRoboticsPing
   ${MOOS_LIBRARIES}
   boost_system
   DEVICE
   HAL
   apputil
   mbutil
   m
   pthread)

