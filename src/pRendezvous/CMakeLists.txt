#--------------------------------------------------------
# The CMakeLists.txt for:                       pRendezvous
# Author(s):                              Charles Benjamin
#--------------------------------------------------------

SET(SRC
  Rendezvous.cpp
  Rendezvous_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pRendezvous ${SRC})

TARGET_LINK_LIBRARIES(pRendezvous
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)

