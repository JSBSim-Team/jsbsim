find_program(Julia_EXECUTABLE julia DOC "Julia executable")

if (Julia_EXECUTABLE)
  execute_process(COMMAND ${Julia_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/get_cxxwrap.jl)
  execute_process(COMMAND ${Julia_EXECUTABLE} -e "using CxxWrap;print(CxxWrap.prefix_path())"
                          OUTPUT_VARIABLE CXXWRAP_PREFIX_PATH)
  set(JlCxx_DIR ${CXXWRAP_PREFIX_PATH}/lib/cmake/JlCxx)
  find_package(JlCxx)

  include_directories(${CMAKE_SOURCE_DIR}/src)
  add_library(JSBSimJL SHARED jsbsimjl.cpp)
  target_link_libraries(JSBSimJL libJSBSim JlCxx::cxxwrap_julia)

  add_test(NAME TestJulia COMMAND ${Julia_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.jl)
endif(Julia_EXECUTABLE)
