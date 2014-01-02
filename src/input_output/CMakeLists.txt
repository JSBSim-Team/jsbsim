set(JSBSIM_INPUT_OUTPUT_SRC FGGroundCallback.cpp
                            FGPropertyManager.cpp
                            FGScript.cpp
                            FGXMLElement.cpp
                            FGXMLParse.cpp
                            FGfdmSocket.cpp
                            FGOutputType.cpp
                            FGOutputFG.cpp
                            FGOutputSocket.cpp
                            FGOutputFile.cpp
                            FGOutputTextFile.cpp
                            FGPropertyReader.cpp)

set(JSBSIM_INPUT_OUTPUT_HDR FGGroundCallback.h
                            FGPropertyManager.h
                            FGScript.h
                            FGXMLElement.h
                            FGXMLParse.h
                            FGfdmSocket.h
                            FGXMLFileRead.h
                            net_fdm.hxx
                            string_utilities.h
                            FGOutputType.h
                            FGOutputFG.h
                            FGOutputSocket.h
                            FGOutputFile.h
                            FGOutputTextFile.h
                            FGPropertyReader.h)

add_library(InputOutput ${JSBSIM_INPUT_OUTPUT_HDR} ${JSBSIM_INPUT_OUTPUT_SRC})
set_target_properties (InputOutput PROPERTIES
                                   VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (InputOutput PROPERTIES
                                     SOVERSION ${LIBRARY_SOVERSION}
                                     FRAMEWORK ON)
endif()

install(TARGETS InputOutput LIBRARY DESTINATION lib
                            ARCHIVE DESTINATION lib
                            RUNTIME DESTINATION lib
                            # For Mac
                            FRAMEWORK DESTINATION "/Library/Frameworks")
install(FILES ${JSBSIM_INPUT_OUTPUT_HDR} DESTINATION include/JSBSim/input_output)

