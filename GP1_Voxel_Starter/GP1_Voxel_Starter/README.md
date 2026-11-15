# GP1 Voxel

Minimal CPU voxel ray tracer, built on the framework from Graphics Programming 1 (2026-2027).
Dependencies: **SDL3** and **spdlog** (via Conan).

## Prerequisites

- [Python 3.10+](https://www.python.org/downloads/)
- [CMake 3.25+](https://cmake.org/download/)
- MSVC 19.5 (Visual Studio 2026), MinGW-w64 GCC 16, GCC 13 or Clang 18

Conan and Ninja are installed automatically by `bootstrap.py`.

## Quick Start

```powershell
python bootstrap.py                      # Windows, MSVC
python bootstrap.py --compilers gcc      # GCC (MSYS2 / Linux)
python3 bootstrap.py --compilers clang   # Linux, Clang
```

Add `--no-conan-remote` to skip the private course Conan server and use conancenter only.

Then open the folder in your IDE and pick a CMake preset, or build from the command line:

```powershell
cmake --preset conan-windows-msvc
cmake --build --preset conan-windows-msvc-release
./build/msvc/bin/Release/GP1_Voxel.exe
```

## Controls

| Input | Action |
|---|---|
| Right Mouse + WASD / QE | Fly around / move up-down |
| Middle Mouse | Pan |
| Left Mouse | Move forward/backward and turn |
| Shift | Speed boost |
| F1 / F2 | Previous / next scene |
| F3 / F4 | Previous / next visualization mode (Shaded, Normals, Depth, Step Count) |
| F6 | Cycle tone mapper |
| F10 | Toggle scene updating (e.g. the rotating light in the bunny scene) |
| F11 | Toggle timer info |
| F12 | Save screenshot |
| P | Print camera settings |

## Notes

- Pixels are rendered in parallel with `std::execution::par_unseq`. On MSVC this works out of the box.
  With GCC/Clang (libstdc++) it only runs in parallel when TBB is available; without it the loop
  silently runs single-threaded.

## Layout

Mirrors the GP1 course project:

```
source/
├── main.cpp      window, input, main loop
├── core/         math, color, logger, timer, leak detector, debug helpers, OBJ asset loader
├── scene/        camera, context, primitives (ray, AABB, light, voxel grid), scene manager, scenes
├── shading/      debug views (visualization modes, key bindings)
└── renderers/    renderer base, intersection tests (AABB, voxel grid DDA), voxel ray tracer
```

`resources/` (the bunny OBJ) is copied next to the executable on build.
