cmake_minimum_required (VERSION 2.8.0)

project(JSBSim)

add_subdirectory(src)

set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_CURRENT_BINARY_DIR}/"
                               "/admin/"
                               "/autom4te.cache"
                               "/Makefile$"
                               "/Makefile.in$"
                               "depcomp$"
                               "install-sh$"
                               "libtool$"
                               "ltmain.sh"
                               "missing"
                               "/.*\\\\.csv$"
                               "/.*\\\\.o$"
                               "/.*\\\\.a$"
                               "/.*\\\\.m4$"
                               "/.cvsignore$"
                               "config.guess"
                               "config.log"
                               "config.status"
                               "config.sub"
                               "configure$"
                               "/JSBSim$"
                               "/.deps/"
                               "/.libs/"
                               "/CVS/"
                               "${CPACK_SOURCE_IGNORE_FILES}")
include (CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

