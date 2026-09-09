#!/bin/bash
# Builds and runs tools/aether_headless_test.cpp against the emscripten object cache
# produced by project/emscripten/build_web.py (run that first).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
export EM_CONFIG=${EM_CONFIG:-/home/user/emscripten/.emscripten}
EMPP=${EMPP:-/home/user/emscripten/em++}
O=$ROOT/project/emscripten/obj
OUT=${OUT:-/tmp/ht}
mkdir -p "$OUT"
FLAGS="-O1 -std=c++17 -DOPENGL_ES -DUSE_VBO -DEMSCRIPTEN -Wno-c++11-narrowing -Wno-register -Wno-unused-value -Wno-comment -sUSE_SDL=2 -sUSE_LIBPNG=1"
# Recompile any Aether sources passed as arguments (so the test picks up edits without a full build)
for f in "$@"; do
  name=$(echo "${f#src/}" | tr '/' '_').o
  $EMPP $FLAGS -O2 -I"$ROOT/src" -c "$ROOT/$f" -o "$O/$name"
done
$EMPP $FLAGS -I"$ROOT/src" -c "$ROOT/tools/aether_headless_test.cpp" -o "$OUT/test.o"
OBJS=$(ls $O/*.o | grep -v -E "/(client_|main_emscripten|AppPlatform_emscripten|NinecraftApp)")
$EMPP -O1 "$OUT/test.o" $OBJS -o "$OUT/test.js" -sUSE_SDL=2 -sUSE_LIBPNG=1 -sWASM_BIGINT -sALLOW_MEMORY_GROWTH=1 -sASSERTIONS=1 -sEXIT_RUNTIME=1 -lopenal
node "$OUT/test.js"
