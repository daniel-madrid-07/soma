# Pipeline del personaje (headless, reproducible)

Todos los pasos corren en Blender headless sobre `models/soma_character.blend`:

```
"C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" -b models/soma_character.blend --python tools/blender/pipeline/<paso>.py
```

Orden (cada paso guarda el .blend salvo los `verify_*` y `export_glb`):

| paso | qué hace | verificación |
|------|----------|--------------|
| `audit_blend.py` | inventario: rig, pesos, datablocks compartidos | lee el JSON |
| `smooth_skin.py` | skinning suave multi-hueso en mallas que cruzan articulación | `verify_skin.py`: gap ≤ ~6 mm a 60° |
| `audit_parenting.py` | distribución de parenting + mallas sueltas | lee el JSON |
| `armature_a1.py` | cadena spine_01..04/neck_01..02 + engancha mallas sueltas | `verify_a.py`: STRAY 0, RAMP_OK |
| `armature_a2.py` | 50 huesos de dedos desde geometría + drivers de curl | `verify_a.py`: CURL se mueve |
| `armature_a3.py` | mandíbula + ojos + IK con polos (gated `rig["ik_on"]`) | drift IK 0 mm impreso |
| `armature_a3b.py` | restringe ojos a piezas ≤18 mm del pivote | imprime kept/moved |
| `props_cd.py` | huesos pulse_root/breath_L/R + drivers de escala | — |
| `materials_g.py` | PBR por color HSV + luces 3 puntos + render de control | `render/control_g.png` |
| `export_glb.py` | rígidas→skinned, une por colección, exporta `models/soma_character.glb` | NO guarda el .blend |

Reglas duras aprendidas (no las rompas):
- Mallas espejo comparten datablock: `o.data = o.data.copy()` ANTES de escribir pesos.
- Cero `bpy.ops` de parenting: reparenta con `matrix_parent_inverse` (fórmula en A1).
- Tras `object.join()` las referencias Python caducan: guarda NOMBRES, no objetos.
- Hay mallas con 0 vértices: guarda con `len(o.data.vertices)==0`.
- Verifica NUMÉRICAMENTE (gap, ramp, drift), nunca por screenshot del viewport.
