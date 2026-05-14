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
   # Old direct Ping1D serial dependencies, kept for easy manual rollback:
   # ${PINGCPP_DEVICE_LIBRARY}
   # ${PINGCPP_HAL_LIBRARY}
   apputil
   mbutil
   m
   pthread)
