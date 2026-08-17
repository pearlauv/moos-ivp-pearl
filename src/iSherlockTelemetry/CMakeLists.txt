#--------------------------------------------------------
# The CMakeLists.txt for:                       iSherlockTelemetry
# Author(s):                              Charles Benjamin
#--------------------------------------------------------

SET(SRC
  SherlockTelemetry.cpp
  SherlockTelemetry_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(iSherlockTelemetry ${SRC})

TARGET_LINK_LIBRARIES(iSherlockTelemetry
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)
