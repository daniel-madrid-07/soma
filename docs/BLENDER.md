# SOMA — Humano realista con Blender (guía para ti)

Objetivo: conseguir una malla humana realista rigged en `.glb`, que yo muevo con la
física de SOMA (retargeting) y renderizo en vídeo fotorrealista (headless).

Tú haces **1 y 2** (generar y exportar el modelo). El resto lo hago yo.

---

## 1. Instalar

- **Blender** (gratis): https://www.blender.org/download/  → instala normal.
- Un generador de humano realista (elige uno):
  - **MB-Lab** (addon gratis de Blender, recomendado, todo en una app):
    descarga el .zip de https://github.com/animate1978/MB-Lab → en Blender:
    `Edit ▸ Preferences ▸ Add-ons ▸ Install…` → selecciona el .zip → actívalo.
  - **MakeHuman** (app aparte, gratis): https://static.makehumancommunity.org/makehuman/downloads.html

Tras instalar, comprueba que Blender está en el PATH (para que yo lo llame):
en una terminal, `blender --version` debe responder. Si no, dime la ruta del
`blender.exe` (normalmente `C:\Program Files\Blender Foundation\Blender X.Y\blender.exe`).

---

## 2. Generar y exportar el humano

### Opción A — MB-Lab (dentro de Blender)
1. `N` para abrir el panel lateral → pestaña **MB-Lab**.
2. **Create** un humano (elige tipo, p. ej. "Human Female/Male Caucasic"). Ajusta a gusto.
3. **Finalize ▸ Finalize** (esto congela la malla con su armadura).
4. Selecciona el personaje completo (malla + armature).
5. `File ▸ Export ▸ glTF 2.0 (.glb)`. En las opciones de exportación:
   - Format: **glTF Binary (.glb)**
   - Include: **Selected Objects** ✔, **Armature** ✔
   - Transform: **+Y Up** ✔ (por defecto)
   - Data ▸ Skinning ✔
6. Guárdalo como **`viewer/assets/human.glb`** dentro del proyecto.

### Opción B — MakeHuman → Blender
1. En MakeHuman: crea el humano. En **Pose/Animate ▸ Skeleton** elige **"Game engine"** (nombres de hueso simples y predecibles).
2. `Files ▸ Export` → **Filmbox (fbx)** → guarda como `viewer/assets/human.fbx`.
3. Yo lo convierto a `.glb` con Blender headless (te doy el comando abajo).

---

## 3. (Lo hago yo) Enchufarlo al visor 3D

Con `viewer/assets/human.glb` en su sitio, el visor lo **autocarga** (si no está, usa Xbot).
El detector de huesos ya soporta nombres de Mixamo, MakeHuman y MB-Lab (thigh/calf/
upperarm/forearm, upperleg/lowerleg, LeftUpLeg/LeftLeg…). Solo abre:

```
http://127.0.0.1:8971/viewer/realistic_nav.html
```

Si algún hueso no se detecta (el modelo se mueve raro), pásame los nombres —
los saco con: `blender --background viewer/assets/human.glb --python tools/blender/dump_bones.py`.

---

## 4. (Lo hago yo) Render de vídeo fotorrealista (headless)

Yo horneo el movimiento de la física de SOMA sobre el armadura del modelo y renderizo
un vídeo con luces y materiales, todo por línea de comandos (sin abrir Blender):

```
bash scripts/blender_render.sh viewer/assets/human.glb
```

Genera `render/soma_walk.mp4` (o PNGs). Necesita que Blender esté en el PATH.

Convertir FBX→GLB (Opción B):
```
blender --background --python tools/blender/fbx_to_glb.py -- viewer/assets/human.fbx viewer/assets/human.glb
```

---

## Resumen de lo que necesito de ti

1. Instala Blender (+ MB-Lab o MakeHuman).
2. Genera un humano y expórtalo a **`viewer/assets/human.glb`**.
3. Dime "listo" (y, si `blender` no está en el PATH, la ruta del ejecutable).

Yo me encargo del retargeting, el visor y el render.
