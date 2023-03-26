# Build the fpectl module
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
endif(APPLE)
