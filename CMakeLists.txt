cmake_minimum_required(VERSION 3.13) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard



# Dawn demo project
add_executable(dawn_demo "src/dawn_demo.cpp")

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

if(EMSCRIPTEN)
	set_target_properties(dawn_demo PROPERTIES SUFFIX ".html")
	target_link_libraries(dawn_demo PRIVATE emdawnwebgpu_cpp)
	target_link_options(dawn_demo PRIVATE "-sASYNCIFY=1")
else()
	target_link_libraries(dawn_demo PRIVATE
		dawn::webgpu_dawn
		glfw
		webgpu_glfw
	)
endif()



# Simulation project
add_executable(sim
	src/util/glad.c
	src/util/cl_util.cpp
	src/util/gl_util.cpp
	src/render/axes.cpp
	src/render/field_vector.cpp
	src/render/particles.cpp
	src/render/ring.cpp
	src/render/solenoid.cpp
	src/render/torus.cpp
	src/render/tracer.cpp
	src/keyboard.cpp
	src/plasma.cpp
	src/current_segment.cpp
	src/state.cpp
	src/scene.cpp
	src/args.cpp
	src/tokamak.cpp
	src/sim.cpp
)

target_include_directories(sim PRIVATE
	/opt/homebrew/Cellar/glfw/3.4/include/GLFW
	/opt/homebrew/Cellar/glew/2.2.0_1/include
	/opt/homebrew/Cellar/glm/1.0.1/include
	include
	kernel
)

target_link_libraries(sim PRIVATE
	/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.4.dylib
	/opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.2.2.0.dylib
	/opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib
	"-framework OpenGL"
	"-framework OpenCL"
)



# Simulation project (Dawn)
add_executable(sim_dawn
	src/util/wgpu_util.cpp
	src/render/axes_dawn.cpp
	src/render/particles_dawn.cpp
	src/render/ring_dawn.cpp
	src/render/solenoid_dawn.cpp
	src/render/torus_dawn.cpp
	src/keyboard_dawn.cpp
	src/state_dawn.cpp
	src/scene_dawn.cpp
	src/args_dawn.cpp
	src/tokamak_dawn.cpp
	src/plasma_dawn.cpp
	src/sim_dawn.cpp
)

target_include_directories(sim_dawn PRIVATE
	/opt/homebrew/Cellar/glm/1.0.1/include
	include
	kernel
	src
)

if(EMSCRIPTEN)
	set_target_properties(sim_dawn PROPERTIES SUFFIX ".html")
	target_link_libraries(sim_dawn PRIVATE emdawnwebgpu_cpp)
	target_link_options(sim_dawn PRIVATE "-sASYNCIFY=1")
else()
	target_link_libraries(sim_dawn PRIVATE
		dawn::webgpu_dawn
		glfw
		webgpu_glfw
		/opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib
	)
endif()