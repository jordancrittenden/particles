#!/bin/sh
clang++ \
    -std=c++17 \
    -O3 \
    -I/opt/homebrew/Cellar/glfw/3.4/include/GLFW \
    -I/opt/homebrew/Cellar/glew/2.2.0_1/include \
    -I/opt/homebrew/Cellar/glm/1.0.1//include \
    -Iinclude \
    -Ikernel \
    src/glad.c \
    src/cl_util.cpp \
    src/gl_util.cpp \
    src/keyboard.cpp \
    src/torus.cpp \
    src/axes.cpp \
    src/particles.cpp \
    src/field_vector.cpp \
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