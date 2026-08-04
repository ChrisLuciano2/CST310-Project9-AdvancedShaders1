CST-310 - Project 9: Advanced Shaders 1
Scene Composition, Basic Shaders, and Keyboard Camera
Author: Christopher Luciano

===============================================================================
WHAT THIS IS
===============================================================================
An OpenGL 4.1 scene with a checkerboard ground plane, a sphere, a cylinder,
and a cube, each shaded with a distinct basic technique (Fresnel rim on the
sphere, procedural bump mapping on the cylinder, procedural panel pattern on
the cube), plus a recursive fractal tree as a background element. The camera
is controlled entirely by discrete keyboard events matching the Project 9
Resource Guide's keyboard scheme.

Full write-up (theoretical background, math, code explanation, flowchart,
screenshots, video link): see the accompanying .docx in this folder.

===============================================================================
REQUIREMENTS
===============================================================================
Windows:
  - Visual Studio 2022 (or newer Build Tools) with "Desktop development
    with C++"
  - CMake 3.20+
  - vcpkg, with the VCPKG_ROOT environment variable set
  - Python 3 on PATH (only needed if external/glad is regenerated)

===============================================================================
HOW TO BUILD AND RUN (Windows)
===============================================================================
From a "Developer PowerShell for VS 2022" (or newer) prompt, from the
project root:

    cmake --preset win-igpu
    cmake --build --preset win-igpu --parallel
    .\build\win-igpu\Release\topic_09_checkerboard_scene.exe

(If your installed Visual Studio version does not match the preset's
generator, e.g. VS 2026 Build Tools instead of VS 2022, configure manually:

    cmake -S . -B build\win-igpu -G "Visual Studio 18 2026" -A x64 ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
      -DVCPKG_TARGET_TRIPLET=x64-windows
    cmake --build build\win-igpu --config Release --parallel
)

Run the executable from the project root (so it can find shaders/scene.vert
and shaders/scene.frag by relative path):

    .\build\win-igpu\Release\topic_09_checkerboard_scene.exe

===============================================================================
CONTROLS
===============================================================================
Right Arrow          slide camera 1 unit in the positive X direction
Left Arrow           slide camera 1 unit in the negative X direction
Up Arrow             slide camera 1 unit in the positive Y direction
Down Arrow           slide camera 1 unit in the negative Y direction
Shift+Up Arrow       slide camera 1 unit in the positive Z ("in") direction
Shift+Down Arrow     slide camera 1 unit in the negative Z ("out") direction
Ctrl+Down Arrow      change camera pitch by 2 degrees
Ctrl+Up Arrow        change camera pitch by -2 degrees
Ctrl+Right Arrow     change camera yaw by 2 degrees
Ctrl+Left Arrow      change camera yaw by -2 degrees
,  (<)               change camera roll by 2 degrees
.  (>)               change camera roll by -2 degrees
r                    reset to the default position and orientation
Esc                  close

===============================================================================
PROJECT LAYOUT
===============================================================================
src/main.cpp             all scene logic (mesh generation, camera, render loop)
shaders/scene.vert        vertex shader (passthrough position/normal/uv)
shaders/scene.frag        fragment shader (checkerboard + 3 basic shader modes)
CMakeLists.txt            build configuration
CMakePresets.json         mac-arm64 / linux / win-dgpu / win-igpu presets
docs/                     documentation assets
  math.md                   mathematical derivations for all primitives,
                             shaders, and the camera
  Project9_navigation.mp4   navigation video
