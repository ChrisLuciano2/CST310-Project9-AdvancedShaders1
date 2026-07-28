#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$HERE"
REPO_ROOT="$(cd "$HERE/../.." && pwd)"
if [ -x "$REPO_ROOT/scripts/setup_mac.sh" ]; then
    "$REPO_ROOT/scripts/setup_mac.sh" --project "$HERE"
else
    command -v brew >/dev/null 2>&1 || { echo "Install Homebrew first"; exit 1; }
    brew list cmake glfw glm >/dev/null 2>&1 || brew install cmake glfw glm
fi
if [ ! -f "external/glad/src/gl.c" ]; then
    python3 -m pip install --quiet --user glad2 2>/dev/null || \
        python3 -m pip install --quiet --break-system-packages glad2
    python3 -m glad --api gl:core=4.1 --out-path external/glad c --loader
fi
cmake --preset mac-arm64
cmake --build --preset mac-arm64 --parallel
echo ""
echo "[topic_09_checkerboard_scene] launching"
./build/mac-arm64/topic_09_checkerboard_scene
