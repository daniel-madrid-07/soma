# SOMA — Anatomía visible (venas, órganos, músculos) — plan

Objetivo del usuario: que el modelo 3D muestre **músculos, órganos, venas, huesos** —
no solo la piel.

## Realidad de los activos

- **MB-Lab** (lo que usamos para el cuerpo) da SOLO piel (malla externa) + esqueleto.
  No trae órganos/venas/músculos visibles.
- **Z-Anatomy** (libre, CC BY-SA) es EL recurso: atlas 3D por CAPAS —
  esqueleto, músculos, sistema **vascular (venas/arterias)**, nervioso y órganos —
  encendibles/apagables. Derivado de BodyParts3D.
  - Repo: https://github.com/Z-Anatomy/The-blend  ·  https://www.z-anatomy.com/
  - Formato: plantilla de Blender (.blend grande) + modelos.

## Qué es realista y qué no

- ✅ **Atlas anatómico por capas**: cargar Z-Anatomy y exportar un visor web donde
  enciendes/apagas piel → músculo → hueso → órganos → venas; rotable, con cortes.
  Esto es "modelo con venas/órganos/músculos".
- ❌ **Que esos órganos/venas internos se MUEVAN con la marcha física**: no es viable
  con activos libres — cada estructura habría que riggearla al esqueleto (trabajo de
  estudio). La *simulación* de SOMA ya calcula músculos/sangre/corazón como DATOS; la
  malla anatómica sería la cáscara visual, no acoplada al movimiento.

## Plan (haré por BlenderMCP / socket, ya conectado)

1. Descargar Z-Anatomy (.blend). Si usa Git LFS / Drive, resolver la descarga real.
2. Importar en Blender; identificar las colecciones por sistema (skeleton, muscular,
   cardiovascular, nervous, viscera…).
3. Exportar cada capa a glTF/GLB (o una sola con nombres de colección).
4. Visor web `viewer/anatomy.html`: botones para encender/apagar cada capa, con
   cortes y opacidad de la piel (ver "a través").
5. (Opcional, difícil) Alinear el atlas con la pose del cuerpo caminante.

## Alternativa mínima ya disponible

Con MB-Lab podemos exportar la capa de **esqueleto** (los 71 huesos) como malla, y
SOMA ya visualiza fuerzas/activación muscular como datos → una capa "muscular
funcional" (mapa de calor de activación) sin malla anatómica real.

## Estado

- Conexión a Blender en vivo: ✅ (`tools/blender/mcp_client.py`, socket 9876).
- Cuerpo masculino cargado y posado (brazos abajo + zancada) en Blender: ✅.
- Z-Anatomy: pendiente de descargar e integrar.
