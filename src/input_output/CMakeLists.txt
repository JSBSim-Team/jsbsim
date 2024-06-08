set(SOURCES FGGroundCallback.cpp
            FGPropertyManager.cpp
            FGScript.cpp
            FGXMLElement.cpp
            FGXMLParse.cpp
            FGfdmSocket.cpp
            FGXMLFileRead.cpp
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
            string_utilities.cpp
            FGLog.cpp)

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
            FGLog.h)

add_library(InputOutput OBJECT ${HEADERS} ${SOURCES})
set_target_properties(InputOutput PROPERTIES TARGET_DIRECTORY
                                  ${CMAKE_CURRENT_SOURCE_DIR})
# For MinGW, we need to force _WIN32_WINNT to a quite recent value for FGfdmSocket
# to compile due to the usage of the flags AI_ADDRCONFIG and AI_V4MAPPED
# See https://docs.microsoft.com/fr-fr/windows/win32/winprog/using-the-windows-headers#setting-winver-or-_win32_winnt
target_compile_definitions(InputOutput PRIVATE $<$<BOOL:${MINGW}>:_WIN32_WINNT=0x600>)  # Windows Vista/Windows Server 2008

add_coverage(InputOutput)

install(FILES ${HEADERS} DESTINATION include/JSBSim/input_output
                         COMPONENT devel)
