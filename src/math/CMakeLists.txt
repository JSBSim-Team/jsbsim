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
            FGNelderMead.cpp
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
            FGNelderMead.h
            FGStateSpace.h)

add_full_path_name(MATH_SRC "${SOURCES}")
add_full_path_name(MATH_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/math)

