#--------------------------------------------------------
# The CMakeLists.txt for:              lib_behaviors-test
# Author(s):                                
#--------------------------------------------------------

# Set System Specific Libraries
if (${WIN32})
  # Windows Libraries
  SET(SYSTEM_LIBS
      )
else (${WIN32})
  # Linux and Apple Libraries
  SET(SYSTEM_LIBS
      m )
endif (${WIN32})

#--------------------------------------------------------
#                                      BHV_Pulse
#--------------------------------------------------------
ADD_LIBRARY(BHV_Garb SHARED 
  BHV_Garb.cpp)
  
TARGET_LINK_LIBRARIES(BHV_Garb
helmivp
behaviors 
ivpbuild 
logic 
ivpcore 
bhvutil 
mbutil 
geometry 
${SYSTEM_LIBS} )
