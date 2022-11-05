# Build the fpectl module
if (APPLE)
  file(COPY fpectl.py DESTINATION ${CMAKE_BINARY_DIR}/tests)
else()
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/python/findModuleFileName.py fpectl OUTPUT_VARIABLE FPECTL_MODULE_NAME)
  set(FPECTL_PYTHON_MODULE ${CMAKE_BINARY_DIR}/tests/${FPECTL_MODULE_NAME})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py ${CMAKE_CURRENT_BINARY_DIR})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fpectlmodule.h ${CMAKE_CURRENT_BINARY_DIR})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fpectlmodule.cpp ${CMAKE_CURRENT_BINARY_DIR})

  add_custom_command(OUTPUT ${FPECTL_PYTHON_MODULE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fpectlmodule.h ${CMAKE_CURRENT_SOURCE_DIR}/fpectlmodule.cpp ${PROJECT_SOURCE_DIR}/python/ExceptionManagement.h
    COMMAND ${PYTHON_EXECUTABLE} setup.py build_ext -b ${CMAKE_BINARY_DIR}/tests
    COMMENT "Building fpectl module...")

  add_custom_target(fpectl ALL DEPENDS ${FPECTL_PYTHON_MODULE})
endif(APPLE)
