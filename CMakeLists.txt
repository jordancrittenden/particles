cmake_minimum_required(VERSION 3.13) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard


set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)



# Simulation project
add_executable(sim_opengl
	src/opengl/util/glad.c
	src/opengl/util/cl_util.cpp
	src/opengl/util/gl_util.cpp
	src/opengl/render/axes.cpp
	src/opengl/render/field_vector.cpp
	src/opengl/render/particles.cpp
	src/opengl/render/ring.cpp
	src/opengl/render/solenoid.cpp
	src/opengl/render/torus.cpp
	src/opengl/render/tracer.cpp
	src/opengl/keyboard.cpp
	src/opengl/plasma.cpp
	src/opengl/current_segment.cpp
	src/opengl/state.cpp
	src/opengl/scene.cpp
	src/opengl/args.cpp
	src/opengl/tokamak.cpp
	src/opengl/sim.cpp
)

target_include_directories(sim_opengl PRIVATE
	/opt/homebrew/Cellar/glfw/3.4/include/GLFW
	/opt/homebrew/Cellar/glew/2.2.0_1/include
	/opt/homebrew/Cellar/glm/1.0.1/include
	include
	kernel
	src/opengl
)

target_link_libraries(sim_opengl PRIVATE
	/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.4.dylib
	/opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.2.2.0.dylib
	/opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib
	"-framework OpenGL"
	"-framework OpenCL"
)



# Simulation project (Dawn)
add_executable(sim_dawn
	src/dawn/util/wgpu_util.cpp
	src/dawn/compute/particles.cpp
	src/dawn/compute/fields.cpp
	src/dawn/render/axes.cpp
	src/dawn/render/particles.cpp
	src/dawn/render/fields.cpp
	src/dawn/render/ring.cpp
	src/dawn/render/solenoid.cpp
	src/dawn/render/torus.cpp
	src/dawn/shared/particles.cpp
	src/dawn/shared/fields.cpp
	src/dawn/scene.cpp
	src/dawn/args.cpp
	src/dawn/free_space.cpp
	src/dawn/tokamak.cpp
	src/dawn/plasma.cpp
	src/dawn/current_segment.cpp
	src/dawn/sim.cpp
)

target_include_directories(sim_dawn PRIVATE
	/opt/homebrew/Cellar/glm/1.0.1/include
	include
	kernel
	src/dawn
)

if(EMSCRIPTEN)
	set_target_properties(sim_dawn PROPERTIES SUFFIX ".html")
	target_link_libraries(sim_dawn PRIVATE emdawnwebgpu_cpp)
	target_link_options(sim_dawn PRIVATE
		-s ASYNCIFY=1
		--embed-file kernel/wgpu
		--preload-file shader/wgpu@shader/wgpu
	)
else()
	target_link_libraries(sim_dawn PRIVATE
		dawn::webgpu_dawn
		glfw
		webgpu_glfw
		/opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib
	)
endif()