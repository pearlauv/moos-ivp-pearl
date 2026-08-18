#--------------------------------------------------------
# The CMakeLists.txt for:                       uSimAttachment
# Author(s):                              Charles Benjamin
#--------------------------------------------------------

SET(SRC
  SimAttachment.cpp
  SimAttachment_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(uSimAttachment ${SRC})

TARGET_LINK_LIBRARIES(uSimAttachment
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   pthread)
