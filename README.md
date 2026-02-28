# Particles

A WebGPU-based particle simulation (native and web).

## Prerequisites

- **CMake** 3.13 or newer  
- **C++20** compiler  
- **Dawn** (WebGPU) — included as a git submodule  
- **Native only:** GLFW, GLM  
  - On macOS: `brew install glfw glm`  
- **Web (Dawn webapp) only:** [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) installed and on your `PATH` (`emsdk activate` and `source emsdk_env.sh`)

## Building the native app

1. Clone and init the Dawn submodule:
   ```bash
   git submodule update --init --recursive
   ```

2. Configure and build:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

3. Run the simulation:
   ```bash
   ./sim
   ```
   (On Windows the executable may be `sim.exe`.)

## Building the Dawn webapp (Emscripten)

1. Ensure the Dawn submodule is initialized (see above) and Emscripten is active in your shell.

2. Configure with the Emscripten toolchain:
   ```bash
   mkdir build-web && cd build-web
   emcmake cmake ..
   ```

3. Build:
   ```bash
   emmake cmake --build .
   ```
   This produces `sim.html` (and related `.js`/`.wasm` assets) in `build-web`.

4. Serve the build directory with a local web server (required for WebGPU and file loading). For example:
   ```bash
   emrun --no_browser --port 8080 .
   ```
   Then open `http://localhost:8080/sim.html` in a browser. Or use any static server (e.g. `python -m http.server 8080`) from the `build-web` directory.

## Running tests

Tests are built only for the **native** target (not the Emscripten build).

From the native build directory, run only this project’s tests (Dawn’s tests are also registered and may not be built, so plain `ctest` can fail):

```bash
cd build
ctest -L particles
```

Or run the test executable directly:

```bash
./particles_tests
```
