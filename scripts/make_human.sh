#!/usr/bin/env bash
# SOMA — genera un humano REALISTA con Blender (headless) desde la librería base de
# MB-Lab y lo deja en viewer/assets/human.fbx (lo autocarga realistic_nav.html).
# No abre Blender ni requiere clics. Uso: scripts/make_human.sh [female|male]
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
SEX="${1:-female}"

# Blender 5.1 (su exportador funciona; MB-Lab es solo la fuente de la malla base).
BL="${BLENDER5:-}"
[ -z "$BL" ] && for p in "/c/Program Files/Blender Foundation/Blender 5"*/blender.exe \
                          "/c/Program Files/Blender Foundation/Blender 4"*/blender.exe; do
  [ -f "$p" ] && BL="$p" && break; done
[ -z "$BL" ] && BL="$(command -v blender 2>/dev/null || true)"
if [ -z "$BL" ]; then echo "No encuentro Blender 4/5. export BLENDER5=/ruta/blender.exe"; exit 1; fi
echo "· Blender: $BL"

"$BL" --background --python tools/blender/extract_human.py -- "$SEX" 2>&1 \
  | grep -E "MESH|SKEL|BIND_OK|FBX_OK|FBX_FAIL"
echo "· Listo → viewer/assets/human.fbx  (ábrelo en realistic_nav.html)"
