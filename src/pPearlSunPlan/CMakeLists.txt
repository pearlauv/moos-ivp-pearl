#--------------------------------------------------------
# The CMakeLists.txt for:                       pPearlSunPlan
# Author(s):                              Charles Benjamin
#--------------------------------------------------------

SET(SRC
  PearlSunPlan.cpp
  PearlSunPlan_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pPearlSunPlan ${SRC})

TARGET_LINK_LIBRARIES(pPearlSunPlan
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)
