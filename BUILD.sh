#!/bin/sh
clang++ \
    -std=c++17 \
    -O3 \
    -I/opt/homebrew/Cellar/glfw/3.4/include/GLFW \
    -I/opt/homebrew/Cellar/glew/2.2.0_1/include \
    -I/opt/homebrew/Cellar/glm/1.0.1//include \
    -Iinclude \
    -Ikernel \
    src/util/glad.c \
    src/util/cl_util.cpp \
    src/util/gl_util.cpp \
    src/render/axes.cpp \
    src/render/field_vector.cpp \
    src/render/ring.cpp \
    src/render/solenoid.cpp \
    src/render/torus.cpp \
    src/render/tracer.cpp \
    src/keyboard.cpp \
    src/particles.cpp \
    src/current_segment.cpp \
    src/state.cpp \
    src/scene.cpp \
    src/args.cpp \
    src/tokamak.cpp \
    src/sim.cpp \
    /opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.4.dylib \
    /opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.2.2.0.dylib \
    /opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib \
    -framework OpenGL \
    -framework OpenCL \
    -o sim.app