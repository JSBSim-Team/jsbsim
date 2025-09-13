cmake_minimum_required (VERSION 3.15)

project(NRLMSIS)

# NRLMSIS-00 C package
add_executable(nrlmsise-test nrlmsise-00.c nrlmsise-00_data.c nrlmsise-00_test.c)
target_link_libraries(nrlmsise-test m)
