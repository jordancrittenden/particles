#!/bin/sh
clang++ \
    -std=c++14 \
    -Ofast \
    -I/opt/homebrew/Cellar/glfw/3.4/include/GLFW \
    -I/opt/homebrew/Cellar/glew/2.2.0_1/include \
    -I/opt/homebrew/Cellar/glm/1.0.1//include \
    -Iinclude \
    src/glad.c src/shader.cpp src/sim.cpp \
    /opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.4.dylib \
    /opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.2.2.0.dylib \
    /opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib \
    -framework OpenGL \
    -framework OpenCL