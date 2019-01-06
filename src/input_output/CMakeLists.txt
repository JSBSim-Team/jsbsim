set(SOURCES FGGroundCallback.cpp
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
            FGPropertyReader.cpp
            FGModelLoader.cpp
            FGInputType.cpp
            FGInputSocket.cpp
            FGUDPInputSocket.cpp)

set(HEADERS FGGroundCallback.h
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
            FGPropertyReader.h
            FGModelLoader.h
            FGInputType.h
            FGInputSocket.h
            FGUDPInputSocket.h)

add_library(InputOutput OBJECT ${HEADERS} ${SOURCES})
set_target_properties(InputOutput PROPERTIES TARGET_DIRECTORY
                                  ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION include/JSBSim/input_output
                         COMPONENT devel)
