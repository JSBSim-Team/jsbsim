# Build the fpectl module

option(FPECTL_DISPLAY_STACK_TRACE "Set to ON to display the stack trace when a floating point exception is raised" OFF)

if (APPLE)
  file(COPY fpectl.py DESTINATION ${CMAKE_BINARY_DIR}/tests)
else()
  set(FPECTL_CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/fpectl_config.h)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fpectl_config.h.in ${FPECTL_CONFIG_HEADER})
  python3_add_library(fpectl MODULE ${PROJECT_SOURCE_DIR}/python/ExceptionManagement.h
                                    fpectlmodule.h
                                    ${FPECTL_CONFIG_HEADER}
                                    fpectlmodule.cpp)
  target_include_directories(fpectl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  set_target_properties(fpectl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

  if(FPECTL_DISPLAY_STACK_TRACE)
    find_package(Backward)
    if(BACKWARD_FOUND)
      target_include_directories(fpectl PRIVATE ${BACKWARD_INCLUDE_DIR})
      if(LIBBFD_FOUND)
        target_link_libraries(fpectl PRIVATE bfd)
      endif(LIBBFD_FOUND)
    endif(BACKWARD_FOUND)
  endif(FPECTL_DISPLAY_STACK_TRACE)
endif(APPLE)
