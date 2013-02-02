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
                            FGOutputTextFile.cpp)

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
                            FGOutputTextFile.h)

add_library(InputOutput ${JSBSIM_STATIC_LIBS_TYPE} ${JSBSIM_INPUT_OUTPUT_SRC})

install(TARGETS InputOutput LIBRARY DESTINATION lib
                            ARCHIVE DESTINATION lib)
install(FILES ${JSBSIM_INPUT_OUTPUT_HDR} DESTINATION include/JSBSim/input_output)

