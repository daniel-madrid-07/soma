#!/usr/bin/env bash
# SOMA — compila y corre un escenario grabador (exporta viewer/frames.js).
# Uso: scripts/record.sh scenarios/record_walk.cpp
set -u
GXX="${GXX:-/c/msys64/ucrt64/bin/g++}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${1:-scenarios/record_walk.cpp}"
OUT="${SCRATCH:-${TEMP:-/tmp}}/soma_rec_$$.exe"
INC=(-Iengine/l0_foundation -Iengine/l1_core -Iengine/l2_physics -Iengine/l3_anatomy
     -Iengine/l4_physiology -Iengine/l5_nervous -Iengine/l6_sensory -Iengine/l7_brain -Itests)
cd "$ROOT" || exit 2
for a in 1 2 3; do
  "$GXX" -std=c++20 -O2 "${INC[@]}" "$SRC" -o "$OUT" 2>"$OUT.log"
  [ -f "$OUT" ] && break
  grep -q "ld returned 116" "$OUT.log" && { sleep 0.3; continue; }
  cat "$OUT.log"; exit 1
done
"$OUT"
