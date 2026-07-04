# SOMA E2a — export game-ready GLB: all bone-parented meshes -> skinned,
# join per system collection, export armature + joined meshes.
# IN-MEMORY ONLY: never saves the .blend.
import bpy, numpy as np, time
from mathutils import Matrix

t0 = time.time()
rig = bpy.data.objects["SOMA_rig"]
if rig.animation_data: rig.animation_data.action = None

def bone_P(name):
    b = rig.data.bones[name]
    return rig.matrix_world @ b.matrix_local @ Matrix.Translation((0, b.length, 0))

# ---- 1. bone-parented -> skinned
conv = 0
for o in list(bpy.data.objects):
    if o.type != 'MESH': continue
    if not (o.parent_type == 'BONE' and o.parent_bone): continue
    bname = o.parent_bone
    world_rest = bone_P(bname) @ o.matrix_parent_inverse @ o.matrix_basis
    o.parent = None
    o.matrix_world = world_rest
    if o.data.users > 1:
        o.data = o.data.copy()
    for vg in list(o.vertex_groups): o.vertex_groups.remove(vg)
    vg = o.vertex_groups.new(name=bname)
    vg.add(list(range(len(o.data.vertices))), 1.0, 'ADD')
    mod = o.modifiers.new("Armature", 'ARMATURE'); mod.object = rig
    conv += 1
print(f"CONVERTED {conv} in {time.time()-t0:.0f}s", flush=True)

# ---- 2. join per system collection (dedupe: first collection wins)
taken = set()
joined = []
cols = list(bpy.context.scene.collection.children)
for col in cols:
    objs = [o for o in col.all_objects
            if o.type == 'MESH' and o.name not in taken
            and any(m.type == 'ARMATURE' for m in o.modifiers)
            and len(o.data.vertices) > 0]
    for o in objs: taken.add(o.name)
    if not objs: continue
    target = objs[0]
    CH = 150
    rest = objs[1:]
    for i in range(0, len(rest), CH):
        chunk = rest[i:i+CH]
        with bpy.context.temp_override(active_object=target,
                                       selected_editable_objects=[target]+chunk,
                                       selected_objects=[target]+chunk):
            bpy.ops.object.join()
    target.name = f"SOMA_{col.name}"
    # keep exactly one armature modifier
    seen = False
    for m in list(target.modifiers):
        if m.type == 'ARMATURE':
            if seen: target.modifiers.remove(m)
            else: m.object = rig; seen = True
    joined.append(target.name)   # nombre, no referencia: los joins posteriores
    print(f"JOINED {target.name} n={len(objs)} verts={len(target.data.vertices)} "
          f"{time.time()-t0:.0f}s", flush=True)  # invalidan los objetos Python

# ---- 3. export GLB (armature + joined meshes only)
joined_names = joined
rig = bpy.data.objects["SOMA_rig"]  # re-fetch: joins invalidate old references
for o in bpy.data.objects:
    try: o.select_set(False)
    except Exception: pass
rig.select_set(True)
for nm in joined_names:
    ob = bpy.data.objects.get(nm)
    if ob: ob.select_set(True)
bpy.context.view_layer.objects.active = rig
out = bpy.path.abspath("//soma_character.glb")
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB',
                          use_selection=True, export_animations=False,
                          export_apply=False, export_yup=True)
import os
print(f"GLB_OK {out} {os.path.getsize(out)//(1024*1024)}MB in {time.time()-t0:.0f}s", flush=True)
