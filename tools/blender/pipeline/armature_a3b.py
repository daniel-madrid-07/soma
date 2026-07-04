# SOMA A3b — tighten eye parenting: only true eyeball parts stay on eye bones.
import bpy, numpy as np
from mathutils import Matrix

rig = bpy.data.objects["SOMA_rig"]
AW = np.array(rig.matrix_world)

def verts_world(o):
    n = len(o.data.vertices)
    if n == 0: return np.empty((0,3))
    co = np.empty(n*3); o.data.vertices.foreach_get("co", co)
    M = np.array(o.matrix_world)
    return co.reshape(-1,3) @ M[:3,:3].T + M[:3,3]

def bone_P(name):
    b = rig.data.bones[name]
    return rig.matrix_world @ b.matrix_local @ Matrix.Translation((0, b.length, 0))

def rebind(o, bname):
    P_old = bone_P(o.parent_bone)
    world_rest = P_old @ o.matrix_parent_inverse @ o.matrix_basis
    basis = o.matrix_basis.copy()
    o.parent_bone = bname
    o.matrix_parent_inverse = bone_P(bname).inverted() @ world_rest @ basis.inverted()

for s in ("L","R"):
    bn = f"eye_{s}"
    b = rig.data.bones[bn]
    piv = AW[:3,:3] @ np.array(b.head_local) + AW[:3,3]
    kept = moved = 0
    for o in bpy.data.objects:
        if o.type=='MESH' and o.parent_type=='BONE' and o.parent_bone==bn:
            w = verts_world(o)
            c = w.mean(0); size = (w.max(0)-w.min(0)).max()
            if np.linalg.norm(c-piv) > 0.018 or size > 0.032:
                rebind(o, "head"); moved += 1
            else:
                kept += 1
    print(f"EYE_{s} kept={kept} back_to_head={moved}", flush=True)

bpy.ops.wm.save_mainfile()
print("SAVED_A3B", flush=True)
