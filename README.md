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

2. Configure and build (from the project root):
   ```bash
   cmake -B build && cmake --build build
   ```

3. Run the simulation:
   ```bash
   ./build/sim
   ```
   (On Windows the executable may be `build\sim.exe`.)

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

Tests are built only for the **native** target (not the Emscripten build). The test executable `particles_tests` is built automatically when you run `cmake --build build`.

From the project root, run only this project’s tests (Dawn’s tests are also registered and may not be built, so plain `ctest` can fail):

```bash
ctest --test-dir build -L particles
```

Or run the test executable directly:

```bash
./build/tests/particles_tests
```

## Most important physics not modeled correctly

1. Poloidal field / rotational transform. This is the big one. With only toroidal field, grad-B and curvature drifts push ions and electrons vertically in opposite directions. The resulting charge separation creates an E field whose E×B drift throws the whole plasma outward. That’s why tokamaks drive a plasma current, which twists the field lines into helices. Your solenoid is meant to drive that current, but the current’s own magnetic field only exists via enableParticleFieldContributions (off by default). Even when on, 10⁵ real particles carry essentially no current. A good next step is to impose a plasma current as a ring of current segments (I_p ≈ 30 kA gives edge safety factor q ≈ 3 for your geometry). You’d then see passing vs. trapped (“banana”) orbits.

1. Vertical / poloidal-field coils. A current-carrying plasma ring expands outward (hoop force) and needs an applied vertical field to hold it in place. These coils also shape the cross-section and form the divertor.

1. Collective (self-consistent) behavior. 10⁵ particles in about 3 m³ is n ≈ 3×10⁴ m⁻³; a real tokamak has n ≈ 10²⁰. So there is effectively no plasma: no Debye shielding, no quasineutrality, no plasma oscillations. Getting there needs macroparticles of about 10¹⁵ each with finite-size shapes, charge and current deposition, and a field solve (Poisson/Maxwell) instead of direct Coulomb sums. With an explicit scheme the cell size must be near the Debye length (about 2 µm at your 8.6 eV temperature). That’s why real codes use implicit, gyrokinetic, or hybrid methods.

1. Coulomb collisions. These give plasma resistance (Spitzer resistivity), which is what makes ohmic heating work and limits how much current the solenoid can drive. They also bring the plasma to thermal equilibrium and cause cross-field transport. Without them, solenoid-accelerated electrons speed up forever (runaway). A Monte Carlo binary-collision model such as Takizuka–Abe is the standard way to add them.

1. Fusion, heating and losses. D-T reaction rates, alpha heating, auxiliary heating (neutral beams, RF), and radiation losses (bremsstrahlung, cyclotron, impurity lines). Note that 10⁵ K ≈ 8.6 eV, while fusion needs about 10 keV (≈10⁸ K).

1. Plasma-wall and neutral physics. Right now particles that hit the wall are simply removed. Missing are the sheath, recycling, ionization and charge exchange with neutral D/T, and the divertor. Runaway electrons also need relativistic Boris, which only requires pushing γv.

One thing you get for free: using 12 discrete coils gives you real toroidal field ripple, which actual machines have to deal with.
