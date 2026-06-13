#--------------------------------------------------------
# The CMakeLists.txt for:                       pSimSOC
# Author(s):                              Charles Benjamin
#--------------------------------------------------------

SET(SRC
  SimSOC.cpp
  SimSOC_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pSimSOC ${SRC})

TARGET_LINK_LIBRARIES(pSimSOC
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)
