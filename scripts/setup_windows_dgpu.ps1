$ErrorActionPreference = "Stop"
$Here = (Resolve-Path "$PSScriptRoot\..").Path
Set-Location $Here
$RepoRoot = (Resolve-Path "$Here\..\..").Path
$RootScript = Join-Path $RepoRoot "scripts\setup_windows_dgpu.ps1"
if (Test-Path $RootScript) {
    & $RootScript -ProjectPath $Here
} else {
    if (-not $env:VCPKG_ROOT) { throw "VCPKG_ROOT is not set." }
    & "$env:VCPKG_ROOT\vcpkg.exe" install glfw3:x64-windows glm:x64-windows
}
if (-not (Test-Path "external\glad\src\gl.c")) {
    python -m pip install --quiet --user glad2
    python -m glad --api gl:core=4.1 --out-path external\glad c --loader
}
cmake --preset win-dgpu
cmake --build --preset win-dgpu --parallel
Write-Host ""
Write-Host "[topic_09_checkerboard_scene] launching"
& "build\win-dgpu\Release\topic_09_checkerboard_scene.exe"
