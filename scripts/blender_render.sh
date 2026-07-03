#!/usr/bin/env bash
# SOMA — render de vídeo con Blender headless: hornea la física de SOMA sobre un
# modelo humano y produce render/soma_walk.mp4. No necesita abrir Blender.
# Uso: scripts/blender_render.sh [viewer/assets/human.glb]
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

MODEL="${1:-viewer/assets/human.glb}"
[ -f "$MODEL" ] || { echo "· '$MODEL' no existe; uso Xbot de respaldo."; MODEL="viewer/assets/Xbot.glb"; }

# Localiza Blender: $BLENDER, PATH, o Program Files.
BL="${BLENDER:-}"
[ -z "$BL" ] && BL="$(command -v blender 2>/dev/null || true)"
if [ -z "$BL" ]; then
  for p in "/c/Program Files/Blender Foundation"/*/blender.exe; do
    [ -f "$p" ] && BL="$p" && break
  done
fi
if [ -z "$BL" ]; then
  echo "No encuentro Blender. Instálalo (blender.org) o:  export BLENDER=/c/Program\\ Files/Blender\\ Foundation/Blender\\ X.Y/blender.exe"
  exit 1
fi
echo "· Blender: $BL"

echo "· Generando el movimiento de la física (render/motion.json)…"
scripts/record.sh scenarios/record_motion_json.cpp

echo "· Renderizando con Blender (headless)…"
"$BL" --background --python tools/blender/render_walk.py -- "$MODEL" render/motion.json render/soma_walk.mp4

echo "· Listo → render/soma_walk.mp4"
