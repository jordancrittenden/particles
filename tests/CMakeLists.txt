# Unit test executable
add_executable(particles_tests
	args_test.cpp
)

target_include_directories(particles_tests PRIVATE
	${GLM_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/kernel
	${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(particles_tests PRIVATE
	gtest_main
)

# Link in the source files under test (no main)
target_sources(particles_tests PRIVATE
	${CMAKE_SOURCE_DIR}/src/args.cpp
)

include(GoogleTest)
gtest_discover_tests(particles_tests PROPERTIES LABELS particles)
