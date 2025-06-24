cmake_minimum_required(VERSION 3.13) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard


set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)



# Demo project
add_executable(demo
	src/dawn_demo.cpp
)

target_include_directories(demo PRIVATE
	/opt/homebrew/Cellar/glm/1.0.1/include
	include
	kernel
	src
)

if(EMSCRIPTEN)
	set_target_properties(demo PROPERTIES SUFFIX ".html")
	target_link_libraries(demo PRIVATE emdawnwebgpu_cpp)
	target_link_options(demo PRIVATE
		-s ASYNCIFY=1
		--embed-file ${CMAKE_BINARY_DIR}/kernel@kernel
		--preload-file ${CMAKE_BINARY_DIR}/shader@shader
	)
	
	# Make sim depend on copy_assets
	add_dependencies(demo copy_assets)
else()
	target_link_libraries(demo PRIVATE
		dawn::webgpu_dawn
		glfw
		webgpu_glfw
		/opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib
	)
endif()



# Simulation project
add_executable(sim
	src/util/wgpu_util.cpp
	src/compute/particles.cpp
	src/compute/fields.cpp
	src/render/axes.cpp
	src/render/particles.cpp
	src/render/fields.cpp
	src/render/ring.cpp
	src/render/solenoid.cpp
	src/render/torus.cpp
	src/shared/particles.cpp
	src/shared/fields.cpp
	src/scene.cpp
	src/args.cpp
	src/free_space.cpp
	src/tokamak.cpp
	src/plasma.cpp
	src/current_segment.cpp
	src/sim.cpp
)

target_include_directories(sim PRIVATE
	/opt/homebrew/Cellar/glm/1.0.1/include
	include
	kernel
	src
)

if(EMSCRIPTEN)
	# Copy kernel and shader directories to build-web before building
	add_custom_target(copy_assets ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/kernel ${CMAKE_BINARY_DIR}/kernel
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shader ${CMAKE_BINARY_DIR}/shader
		COMMENT "Copying kernel and shader directories to build directory"
	)

	set_target_properties(sim PROPERTIES SUFFIX ".html")
	target_link_libraries(sim PRIVATE emdawnwebgpu_cpp)
	target_link_options(sim PRIVATE -sASYNCIFY=1 -sUSE_GLFW=3 --embed-file ${CMAKE_BINARY_DIR}/kernel@kernel --preload-file ${CMAKE_BINARY_DIR}/shader@shader)
	
	# Make sim depend on copy_assets
	add_dependencies(sim copy_assets)
else()
	target_link_libraries(sim PRIVATE
		dawn::webgpu_dawn
		glfw
		webgpu_glfw
		/opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib
	)
endif()