#!/usr/bin/env bash
# SOMA — compila el motor como soma.dll (API C) para Unity. Enlaza estático el
# runtime de C++ para que la DLL no dependa de mingw (solo del sistema Windows).
set -u
GXX="${GXX:-/c/msys64/ucrt64/bin/g++}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
OUT="$ROOT/soma.dll"
INC=(-Iengine/l0_foundation -Iengine/l1_core -Iengine/l2_physics -Iengine/l3_actuators
     -Iengine/l4_systems -Iengine/l5_control -Iengine/l6_sensors -Iengine/l7_agent
     -Iengine/l8_render -Iengine/l9_tools -Itests)
for a in 1 2 3; do
  "$GXX" -std=c++20 -O2 -shared -static-libgcc -static-libstdc++ \
    "${INC[@]}" bindings/soma_unity.cpp -o "$OUT" 2>"$OUT.log"
  [ -f "$OUT" ] && break
  grep -q "ld returned 116" "$OUT.log" && { sleep 0.3; continue; }
  cat "$OUT.log"; exit 1
done
[ -f "$OUT" ] || { cat "$OUT.log"; exit 1; }
echo "OK -> $OUT"
# Si el proyecto Unity está en SOMA/, copia la DLL directamente a sus Plugins.
if [ -d "$ROOT/SOMA/Assets" ]; then
  mkdir -p "$ROOT/SOMA/Assets/Plugins"
  cp -f "$OUT" "$ROOT/SOMA/Assets/Plugins/soma.dll" && echo "   copiada a SOMA/Assets/Plugins/soma.dll"
fi
