#--------------------------------------------------------
# The CMakeLists.txt for:                       pAUVdock
# Author(s):                              jmhamel
#--------------------------------------------------------

SET(SRC
  AUVdock.cpp
  AUVdock_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pAUVdock ${SRC})

TARGET_LINK_LIBRARIES(pAUVdock
   ${MOOS_LIBRARIES}
   geometry
   apputil
   mbutil
   m
   pthread)

