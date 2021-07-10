# Build the fpectl module

option(FPECTL_DISPLAY_STACK_TRACE "Set to ON to display the stack trace when a floating point exception is raised" OFF)

if (APPLE)
  file(COPY fpectl.py DESTINATION ${CMAKE_BINARY_DIR}/tests)
else()
  if(FPECTL_DISPLAY_STACK_TRACE)
    find_package(Backward)
    if(BACKWARD_FOUND)
      set(FPECTL_INCLUDE_DIRS '${BACKWARD_INCLUDE_DIR}')
      if(LIBBFD_FOUND)
        set(FPECTL_LIBRARIES 'bfd')
      endif(LIBBFD_FOUND)
    endif(BACKWARD_FOUND)
  endif(FPECTL_DISPLAY_STACK_TRACE)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fpectl_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/fpectl_config.h)
  set(SETUP_PY ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in ${SETUP_PY})
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/python/findModuleFileName.py fpectl OUTPUT_VARIABLE FPECTL_MODULE_NAME)
  set(FPECTL_PYTHON_MODULE ${CMAKE_BINARY_DIR}/tests/${FPECTL_MODULE_NAME})

  add_custom_command(OUTPUT ${FPECTL_PYTHON_MODULE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fpectlmodule.h ${CMAKE_CURRENT_SOURCE_DIR}/fpectlmodule.cpp
    COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build_ext -b ${CMAKE_BINARY_DIR}/tests
    COMMENT "Building fpectl module...")

  add_custom_target(fpectl ALL DEPENDS ${FPECTL_PYTHON_MODULE})
endif(APPLE)
