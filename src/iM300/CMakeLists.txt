#--------------------------------------------------------
# The CMakeLists.txt for:                           iM300
# Author(s):                                   Blake Cole                             
#--------------------------------------------------------

SET(SRC
  M300.cpp
  M300_Info.cpp
  Thruster.cpp
  main.cpp
  VehRotController.cpp
)

ADD_EXECUTABLE(iM300 ${SRC})

TARGET_LINK_LIBRARIES(iM300
  ${MOOS_LIBRARIES}
  ${MOOSGeodesy_LIBRARIES}
  geometry
  apputil
  sock_util
  mbutil
  m
  pthread)

