# Unit test executable
add_executable(particles_tests
	args_test.cpp
	particles_collision_test.cpp
	particles_webgpu_collision_test.cpp
)

target_include_directories(particles_tests PRIVATE
	${GLM_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/kernel
	${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(particles_tests PRIVATE
	gtest_main
	dawn::webgpu_dawn
)

# Source files under test and sources needed for WebGPU collision tests
target_sources(particles_tests PRIVATE
	${CMAKE_SOURCE_DIR}/src/args.cpp
	${CMAKE_SOURCE_DIR}/src/util/wgpu_util.cpp
	${CMAKE_SOURCE_DIR}/src/shared/particles.cpp
	${CMAKE_SOURCE_DIR}/src/shared/fields.cpp
	${CMAKE_SOURCE_DIR}/src/compute/particles_exact.cpp
	${CMAKE_SOURCE_DIR}/src/compute/particles_pic.cpp
	${CMAKE_SOURCE_DIR}/src/compute/fields.cpp
	${CMAKE_SOURCE_DIR}/src/current_segment.cpp
)

# Run tests from project root so kernel/ and shader paths resolve
include(GoogleTest)
gtest_discover_tests(particles_tests
	PROPERTIES
		LABELS particles
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
