#!/usr/bin/env bash
# SOMA — compila una app nativa (usa el motor C++). Reintenta ante el ld 116.
# Uso: scripts/build_app.sh apps/interactive.cpp [salida.exe]
set -u
GXX="${GXX:-/c/msys64/ucrt64/bin/g++}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${1:-apps/interactive.cpp}"
OUT="${2:-$ROOT/soma-interactive.exe}"
INC=(-Iengine/l0_foundation -Iengine/l1_core -Iengine/l2_physics -Iengine/l3_actuators
     -Iengine/l4_systems -Iengine/l5_control -Iengine/l6_sensors -Iengine/l7_agent
     -Iengine/l8_render -Iengine/l9_tools -Itests)
cd "$ROOT" || exit 2
for a in 1 2 3; do
  "$GXX" -std=c++20 -O2 "${INC[@]}" "$SRC" -o "$OUT" -luser32 2>"$OUT.log"
  [ -f "$OUT" ] && break
  grep -q "ld returned 116" "$OUT.log" && { sleep 0.3; continue; }
  cat "$OUT.log"; exit 1
done
[ -f "$OUT" ] || { cat "$OUT.log"; exit 1; }
grep -v "ld returned 116\|collect2" "$OUT.log" 1>&2 || true
echo "OK -> $OUT"
