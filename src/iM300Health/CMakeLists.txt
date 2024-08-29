#--------------------------------------------------------
# The CMakeLists.txt for:                       iM300Health
# Author(s):                              Tyler
#--------------------------------------------------------

SET(SRC
  M300Health.cpp
  M300Health_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(iM300Health ${SRC})

TARGET_LINK_LIBRARIES(iM300Health
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)

