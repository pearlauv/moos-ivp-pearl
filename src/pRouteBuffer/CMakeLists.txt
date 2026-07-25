#--------------------------------------------------------
# The CMakeLists.txt for:                       pRouteBuffer
# Author(s):                              Charles Benjamin
#--------------------------------------------------------

SET(SRC
  RouteBuffer.cpp
  RouteBuffer_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pRouteBuffer ${SRC})

TARGET_LINK_LIBRARIES(pRouteBuffer
   ${MOOS_LIBRARIES}
   geometry
   apputil
   mbutil
   m
   pthread)
