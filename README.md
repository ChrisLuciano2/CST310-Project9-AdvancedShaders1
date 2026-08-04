# Project 9 — Advanced Shaders 1: Scene Composition

CST-310 · Project 9 (100 pts). Built from the `topic_09_checkerboard_scene`
starter kit: the four required primitives (checkerboard, sphere, cylinder,
cube) composed deliberately, each with a basic per-object shader, a fractal
tree as an optional background element, a keyboard-driven camera matching the
Project 9 Resource Guide exactly, and this documentation.

## What's here

- `src/main.cpp` — mesh generation for all four primitives, the fractal-tree
  background prop, the keyboard camera, and the render loop.
- `shaders/scene.vert` / `scene.frag` — one shared program with a per-object
  shading mode: procedural checkerboard (ground), Fresnel rim (sphere),
  procedural bump mapping (cylinder), procedural panel pattern (cube).
- `docs/math.md` — derivations for every primitive's parameterization, every
  shader's math, and the camera.
- `docs/Project9_navigation.mp4` — a navigation video (see below).
- `Readme.txt` — build/run instructions and requirements.

## Scene composition

The three required objects are staged with intentional depth and spacing
rather than lined up in a row:

- **Sphere** — near the camera's starting position, off to the left.
- **Cylinder** — centered directly on the camera's initial straight-ahead
  path (x = 0), so it's the first thing the camera approaches head-on.
- **Cube** — farther back and to the right, rotated 25° around its vertical
  axis so it reads as deliberately placed rather than dropped in
  axis-aligned.
- **Fractal tree** (optional element) — anchored well behind the three
  primitives and slightly off-axis, giving the scene a background layer and
  a genuine sense of depth.

Positions were chosen so no two objects' bounding volumes overlap from any
angle, while keeping the cylinder squarely in view from the camera's
starting orientation.

## Basic shaders (one per object)

Each object gets a distinct, lightweight technique that Project 10 (Advanced
Shaders 2) will replace with the full version of the same idea:

| Object | Technique | Intended effect | Project 10 upgrades to |
|---|---|---|---|
| Sphere | Fresnel rim (Lambert + `pow(1-N·V, 3)` brightening at grazing angles) | Hints at reflectivity without a real reflection | Full environment/cube-map mapping |
| Cylinder | Procedural bump mapping (analytic-derivative normal perturbation) | Visible ridge relief from a formula, no texture | Refined/authored bump or normal map |
| Cube | Procedural panel pattern (UV-space seam grid) | Visible surface structure/paneling | True parallax (height-map UV displacement) mapping |
| Ground | Procedural checkerboard | Reference grid for scale/navigation | Unchanged |

## Optional element: fractal tree background prop

The fractal tree from `topic_09_fractal_tree` was ported in as a static
background prop (`grow_tree()` in `main.cpp`) — the same recursive branching
rule (fork into 3, tilt 28°, shrink by 0.72 per generation, depth 5),
generated once at startup as a `GL_LINES` mesh rather than rebuilt every
frame, since it's set dressing here rather than the interactive subject.

## Controls

Matches the Project 9 Resource Guide's keyboard scheme exactly — every
control is a discrete keyboard event, no mouse input:

| Keystroke | Action |
|---|---|
| Right Arrow | Slide camera 1 unit in the positive X direction |
| Left Arrow | Slide camera 1 unit in the negative X direction |
| Up Arrow | Slide camera 1 unit in the positive Y direction |
| Down Arrow | Slide camera 1 unit in the negative Y direction |
| Shift+Up Arrow | Slide camera 1 unit in the positive Z ("in") direction |
| Shift+Down Arrow | Slide camera 1 unit in the negative Z ("out") direction |
| Ctrl+Down Arrow | Change camera pitch by 2 degrees |
| Ctrl+Up Arrow | Change camera pitch by -2 degrees |
| Ctrl+Right Arrow | Change camera yaw by 2 degrees |
| Ctrl+Left Arrow | Change camera yaw by -2 degrees |
| `,` (`<`) | Change camera roll by 2 degrees |
| `.` (`>`) | Change camera roll by -2 degrees |
| `r` | Reset to the default position and orientation |
| Esc | Close |

## Navigation video

`docs/Project9_navigation.mp4` shows the camera navigating across the
checkerboard past all three objects using these real keyboard controls, from
multiple distinct angles and distances.

Video link: https://github.com/ChrisLuciano2/CST310-Project9-AdvancedShaders1/blob/master/docs/Project9_navigation.mp4

## Build & run

Requirements: Visual Studio 2022+ (or Build Tools) with the C++ workload,
CMake 3.20+, vcpkg with `VCPKG_ROOT` set, Python 3 (only if regenerating
`external/glad`).

```powershell
cmake --preset win-igpu
cmake --build --preset win-igpu --parallel
.\build\win-igpu\Release\topic_09_checkerboard_scene.exe
```

(This build was verified against a CMake generator override for locally
installed Visual Studio 2026 Build Tools; the presets target VS 2022 —
adjust `-G` if needed for your toolchain.)
