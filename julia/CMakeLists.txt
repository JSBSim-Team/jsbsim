set(CMAKE_CXX_STANDARD 14)
find_package(JlCxx) # May modify CMAKE_CXX_STANDARD thus called after set(CMAKE_CXX_STANDARD ...)

if(Julia_FOUND)
  include_directories(${CMAKE_SOURCE_DIR}/src)

  add_library(JSBSimJL SHARED jsbsimjl.cpp)
  target_link_libraries(JSBSimJL libJSBSim JlCxx::cxxwrap_julia)

  add_test(NAME TestJulia COMMAND ${Julia_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.jl)
else(Julia_FOUND)
  message(WARNING "Julia NOT FOUND. Skipping Julia package build...")
endif(Julia_FOUND)
