#pragma once

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData);

EM_BOOL keyup_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData);

bool is_key_pressed(int keyCode);
#endif