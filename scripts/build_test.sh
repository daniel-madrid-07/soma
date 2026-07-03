#!/usr/bin/env bash
# SOMA — compila y corre un test con g++ (C++20).
# Reintenta ante el error espurio "ld returned 116" (interferencia de antivirus
# al escribir el .exe en Windows). Uso: scripts/build_test.sh tests/test_x.cpp
set -u
GXX="${GXX:-/c/msys64/ucrt64/bin/g++}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$1"
OUT="${SCRATCH:-${TEMP:-/tmp}}/soma_$(basename "$SRC" .cpp)_$$.exe"
INC=(-Iengine/l0_foundation -Iengine/l1_core -Iengine/l2_physics -Iengine/l3_actuators \
     -Iengine/l4_systems -Iengine/l5_control -Iengine/l6_sensors -Iengine/l7_agent \
     -Iengine/l8_render -Iengine/l9_tools -Itests)

cd "$ROOT" || exit 2
for attempt in 1 2 3; do
  "$GXX" -std=c++20 -Wall -Wextra "${INC[@]}" "$SRC" -o "$OUT" 2>"$OUT.log"
  rc=$?
  if [ -f "$OUT" ]; then break; fi           # binario producido pese al rc raro
  if [ $rc -eq 0 ]; then break; fi
  grep -q "ld returned 116" "$OUT.log" && { sleep 0.3; continue; }
  cat "$OUT.log"; exit $rc                    # error real de compilación
done

if [ ! -f "$OUT" ]; then cat "$OUT.log"; exit 1; fi
# Muestra warnings reales si los hubo (ignora la línea espuria del linker).
grep -v "ld returned 116\|collect2" "$OUT.log" 1>&2 || true
"$OUT"; run_rc=$?
if [ $run_rc -eq 0 ]; then echo "PASS: $(basename "$SRC")"; else echo "FAIL: $(basename "$SRC") (rc=$run_rc)"; fi
exit $run_rc
