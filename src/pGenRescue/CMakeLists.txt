#--------------------------------------------------------
# The CMakeLists.txt for:                       pGenPath
# Author(s):                              Molly Kierstead
#--------------------------------------------------------

SET(SRC
  GenRescue.cpp
  GenRescue_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pGenRescue ${SRC})

TARGET_LINK_LIBRARIES(pGenRescue
   ${MOOS_LIBRARIES}
   apputil
   geometry
   mbutil
   m
   pthread)

