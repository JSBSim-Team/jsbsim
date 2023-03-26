# Build the fpectl module

option(FPECTL_DISPLAY_STACK_TRACE "Set to ON to display the stack trace when a floating point exception is raised" OFF)

if (APPLE)
  file(COPY fpectl.py DESTINATION ${CMAKE_BINARY_DIR}/tests)
else()
  python3_add_library(fpectl MODULE ${PROJECT_SOURCE_DIR}/python/ExceptionManagement.h
                                    fpectlmodule.h
                                    fpectlmodule.cpp)
  set_target_properties(fpectl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

  # Output directories for MSVC
  set_target_properties(fpectl PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests)
  set_target_properties(fpectl PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests)
  set_target_properties(fpectl PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests)
  set_target_properties(fpectl PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests)

  if(FPECTL_DISPLAY_STACK_TRACE)
    find_package(Backward)
    if(BACKWARD_FOUND)
      target_include_directories(fpectl PRIVATE ${BACKWARD_INCLUDE_DIR})
      target_compile_definitions(fpectl PRIVATE BACKWARD_FOUND)
      if(LIBBFD_FOUND)
        target_link_libraries(fpectl PRIVATE bfd)
        target_compile_definitions(fpectl PRIVATE LIBBFD_FOUND)
      endif(LIBBFD_FOUND)
    endif(BACKWARD_FOUND)
  endif(FPECTL_DISPLAY_STACK_TRACE)
endif(APPLE)
