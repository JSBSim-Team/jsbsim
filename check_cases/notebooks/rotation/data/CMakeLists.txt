cmake_minimum_required (VERSION 3.15)

project(ExactRotations)

add_executable(exact_free_body_rot free_rigid_body_rotations.c)
target_link_libraries(exact_free_body_rot m gsl gslcblas)
