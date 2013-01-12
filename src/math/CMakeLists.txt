set(JSBSIM_MATH_SRC FGColumnVector3.cpp
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

set(JSBSIM_MATH_HDR FGColumnVector3.h
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

add_library(Math ${JSBSIM_STATIC_LIBS_TYPE} ${JSBSIM_MATH_SRC})

install(TARGETS Math LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)

