#--------------------------------------------------------
# The CMakeLists.txt for:                       pTargetCPA
# Author(s):                              Theo Schafer
#--------------------------------------------------------

SET(SRC
  TargetCPA.cpp
  TargetCPA_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pTargetCPA ${SRC})

TARGET_LINK_LIBRARIES(pTargetCPA
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)

