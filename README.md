# Project 9 — Advanced Shaders 1: Scene Composition

CST-310 · Project 9 (100 pts). Built from the `topic_09_checkerboard_scene`
starter kit, extended per Topic 9 Activity 2: the four required primitives
(checkerboard, sphere, cylinder, cube) composed deliberately, plus a fractal
tree as the chosen optional background element, a fly-through camera, and
this documentation.

## What's here

- `src/main.cpp` — the full scene: mesh generation for all four primitives,
  the fractal-tree background prop, the fly-through camera, and the render
  loop.
- `shaders/scene.vert` / `scene.frag` — placeholder Lambert + ambient shading
  (Project 10 replaces these with environment/parallax/bump mapping).
- `docs/math.md` — derivations for every primitive's parameterization and the
  camera, with parameters and normals explained.
- `docs/Project9_navigation.mp4` — a 33-second navigation video (see below).

## Scene composition

The three required objects are staged with intentional depth and spacing
rather than lined up in a row:

- **Sphere** — near the camera's starting position, off to the left.
- **Cylinder** — centered directly on the camera's initial straight-ahead
  path (x = 0), so it's the first thing the fly-through approaches head-on.
- **Cube** — farther back and to the right, rotated 25° around its vertical
  axis so it reads as deliberately placed rather than dropped in
  axis-aligned.
- **Fractal tree** (the chosen optional element) — anchored well behind the
  three primitives and slightly off-axis, giving the scene a background
  layer and a genuine sense of depth as the camera moves through it.

Positions were chosen so no two objects' bounding volumes overlap from any
angle in the flythrough, while keeping the cylinder squarely in view from the
camera's starting orientation.

## Optional element: fractal tree background prop

Per Activity 2 step 4, the fractal tree from `topic_09_fractal_tree` was
ported in as a static background prop (`grow_tree()` in `main.cpp`) — the
same recursive branching rule (fork into 3, tilt 28°, shrink by 0.72 per
generation, depth 5), generated once at startup as a `GL_LINES` mesh rather
than rebuilt every frame, since it's set dressing here rather than the
interactive subject.

## Controls

| Input | Action |
|---|---|
| W A S D / arrows | move forward / strafe / back / strafe |
| Left-drag | look around |
| Esc | close |

## Navigation video

`docs/Project9_navigation.mp4` (33 seconds, exceeds the 30-second minimum)
shows the camera moving through the scene and past every object — the
sphere, the centered cylinder, the rotated cube, and the fractal tree — from
multiple distinct angles and distances, ending on a wide shot with all four
visible together.

Video link: https://github.com/ChrisLuciano2/CST310-Project9-AdvancedShaders1/blob/9f0aa0938f4ce6705814dcf81d582fef4c9adb80/docs/Project9_navigation.mp4

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
