#!/usr/bin/env bash
# SOMA — visor 3D de un comando: graba la simulación y sirve el visor en el navegador.
# Uso: scripts/view.sh [scenarios/record_walk.cpp]
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
echo "· Grabando la simulación (física real → viewer/frames.js)…"
scripts/record.sh "${1:-scenarios/record_walk.cpp}"
echo
echo "· Abre en el navegador:  http://127.0.0.1:8971/viewer/home.html  (portada con todos los visores)"
echo "· (Ctrl+C para parar el servidor)"
echo
python -m http.server 8971 --bind 127.0.0.1
