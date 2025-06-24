#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

// Global key state tracking for web input
static bool keyStates[512] = {false};

EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    if (e->keyCode < 512) keyStates[e->keyCode] = true;
    return EM_TRUE;
}

EM_BOOL keyup_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    if (e->keyCode < 512) keyStates[e->keyCode] = false;
    return EM_TRUE;
}

bool is_key_pressed(int keyCode) {
    return keyCode < 512 && keyStates[keyCode];
}
#endif