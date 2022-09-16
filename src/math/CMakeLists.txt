set(SOURCES FGColumnVector3.cpp
            FGFunction.cpp
            FGLocation.cpp
            FGMatrix33.cpp
            FGPropertyValue.cpp
            FGQuaternion.cpp
            FGRealValue.cpp
            FGTable.cpp
            FGCondition.cpp
            FGRungeKutta.cpp
            FGModelFunctions.cpp
            FGTemplateFunc.cpp
            FGStateSpace.cpp)

set(HEADERS FGColumnVector3.h
            FGFunction.h
            FGLocation.h
            FGMatrix33.h
            FGParameter.h
            FGPropertyValue.h
            FGQuaternion.h
            FGRealValue.h
            FGTable.h
            FGCondition.h
            FGRungeKutta.h
            FGModelFunctions.h
            LagrangeMultiplier.h
            FGTemplateFunc.h
            FGFunctionValue.h
            FGParameterValue.h
            FGStateSpace.h)

add_library(Math OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Math PROPERTIES TARGET_DIRECTORY
                           ${CMAKE_CURRENT_SOURCE_DIR})
add_coverage(Math)

install(FILES ${HEADERS} DESTINATION include/JSBSim/math COMPONENT devel)
