# SOMA — Partes visible (tubos, props internos, actuadores) — plan

Objetivo del usuario: que el modelo 3D muestre **actuadores, props internos, tubos, huesos** —
no solo la piel.

## Realidad de los activos

- **MB-Lab** (lo que usamos para el cuerpo) da SOLO piel (malla externa) + armadura.
  No trae props internos/tubos/actuadores visibles.
- **Z-Structural** (libre, CC BY-SA) es EL recurso: atlas 3D por CAPAS —
  armadura, actuadores, sistema **de tubos (tubos/tubos)**, de control y props internos —
  encendibles/apagables. Derivado de BodyParts3D.
  - Repo: https://github.com/Z-Structural/the-blend  ·  https://www.example.com/
  - Formato: plantilla de Blender (.blend grande) + modelos.

## Qué es realista y qué no

- ✅ **Atlas de partes por capas**: cargar Z-Structural y exportar un visor web donde
  enciendes/apagas piel → actuador → hueso → props internos → tubos; rotable, con cortes.
  Esto es "modelo con tubos/props internos/actuadores".
- ❌ **Que esos props internos/tubos internos se MUEVAN con la marcha física**: no es viable
  con activos libres — cada estructura habría que riggearla al armadura (trabajo de
  estudio). La *simulación* de SOMA ya calcula actuadores/fluido/bomba como DATOS; la
  malla de partes sería la cáscara visual, no acoplada al movimiento.

## Plan (haré por BlenderMCP / socket, ya conectado)

1. Descargar Z-Structural (.blend). Si usa Git LFS / Drive, resolver la descarga real.
2. Importar en Blender; identificar las colecciones por sistema (skeleton, de actuador,
   de bombeo, control, viscera…).
3. Exportar cada capa a glTF/GLB (o una sola con nombres de colección).
4. Visor web `viewer/parts.html`: botones para encender/apagar cada capa, con
   cortes y opacidad de la piel (ver "a través").
5. (Opcional, difícil) Alinear el atlas con la pose del cuerpo caminante.

## Alternativa mínima ya disponible

Con MB-Lab podemos exportar la capa de **armadura** (los 71 huesos) como malla, y
SOMA ya visualiza fuerzas/activación de actuador como datos → una capa "de actuador
funcional" (mapa de calor de activación) sin malla de partes real.

## Estado

- Conexión a Blender en vivo: ✅ (`tools/blender/mcp_client.py`, socket 9876).
- Cuerpo masculino cargado y posado (brazos abajo + zancada) en Blender: ✅.
- Z-Structural: pendiente de descargar e integrar.
