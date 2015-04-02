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
            FGUDPInputSocket.cpp
            FGUDPOutputSocket.cpp)

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
            FGUDPInputSocket.h
            FGUDPOutputSocket.h)

add_full_path_name(INPUT_OUTPUT_SRC "${SOURCES}")
add_full_path_name(INPUT_OUTPUT_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/input_output)

