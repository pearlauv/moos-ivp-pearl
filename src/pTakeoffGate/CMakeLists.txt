#--------------------------------------------------------
# The CMakeLists.txt for:                       pTakeoffGate
# Author(s):                              Charles Benjamin
#--------------------------------------------------------

SET(SRC
  TakeoffGate.cpp
  TakeoffGate_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pTakeoffGate ${SRC})

TARGET_LINK_LIBRARIES(pTakeoffGate
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)
