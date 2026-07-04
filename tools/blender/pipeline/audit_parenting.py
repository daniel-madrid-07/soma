# SOMA — audit 2: parenting distribution, unparented meshes, hand/foot mesh
# geometry stats (for PCA digit bones), head-region small symmetric meshes.
import bpy, numpy as np, json, collections

rig = bpy.data.objects["SOMA_rig"]
A = np.array(rig.matrix_world)
def bw(name):
    b = rig.data.bones[name]
    return (A[:3,:3] @ np.array(b.head_local) + A[:3,3],
            A[:3,:3] @ np.array(b.tail_local) + A[:3,3])

out = {}
meshes = [o for o in bpy.data.objects if o.type == 'MESH']

# parenting distribution
bp = collections.Counter()
np_count = 0
np_bbox_z = []
for o in meshes:
    has_arm = any(m.type == 'ARMATURE' for m in o.modifiers)
    if has_arm: continue
    if o.parent_type == 'BONE' and o.parent_bone:
        bp[o.parent_bone] += 1
    else:
        np_count += 1
        c = np.array(o.matrix_world.translation)
        np_bbox_z.append(float(c[2]))
out["bone_parented"] = dict(bp)
out["unparented"] = {"count": np_count,
                     "z_min": min(np_bbox_z) if np_bbox_z else None,
                     "z_max": max(np_bbox_z) if np_bbox_z else None}

# collection membership of unparented meshes
colmap = collections.Counter()
for c in bpy.context.scene.collection.children:
    names = {o.name for o in c.all_objects}
    for o in meshes:
        if o.name in names and not any(m.type=='ARMATURE' for m in o.modifiers) \
           and not (o.parent_type=='BONE' and o.parent_bone):
            colmap[c.name] += 1
out["unparented_by_collection"] = dict(colmap)

# hand meshes: those weighted 100% to hand_L -> vert counts + bbox sizes
def wcentroid(o):
    n = len(o.data.vertices)
    co = np.empty(n*3); o.data.vertices.foreach_get("co", co)
    M = np.array(o.matrix_world)
    w = co.reshape(-1,3) @ M[:3,:3].T + M[:3,3]
    return w
hand_stats = []
for o in meshes:
    if o.vertex_groups and len(o.vertex_groups)>=1:
        names = [g.name for g in o.vertex_groups]
        if names == ["hand_L"] or (len(names)>1 and "hand_L" in names):
            w = wcentroid(o)
            hand_stats.append({"n_verts": len(w),
                               "size": [float(x) for x in (w.max(0)-w.min(0))],
                               "c": [round(float(x),3) for x in w.mean(0)]})
out["handL_meshes"] = {"count": len(hand_stats),
                       "sample": hand_stats[:8]}

# head region small symmetric meshes (eye candidates): bone-parented to head,
# near-cubic bbox, size 15-40mm
hh, ht = bw("head")
eyes = []
for o in meshes:
    if o.parent_type=='BONE' and o.parent_bone=='head':
        w = wcentroid(o)
        s = w.max(0)-w.min(0)
        if 0.012 < s.max() < 0.045 and s.min()/s.max() > 0.7:
            eyes.append({"name": o.name, "size": [round(float(x),4) for x in s],
                         "c": [round(float(x),3) for x in w.mean(0)],
                         "n": len(w)})
out["head_cubic_small"] = eyes[:20]
out["head_parented_total"] = bp.get("head", 0)

# spine/pelvis/neck bone-parented: z-range of centroids (for chain split)
zs = {}
for bone in ("pelvis","spine","neck","head"):
    arr = []
    for o in meshes:
        if o.parent_type=='BONE' and o.parent_bone==bone:
            arr.append(float(np.array(o.matrix_world.translation)[2]))
    if arr: zs[bone] = {"n": len(arr), "z_min": round(min(arr),3), "z_max": round(max(arr),3)}
out["trunk_z"] = zs

# bone landmark positions
out["bones_world"] = {n: {"head":[round(float(x),3) for x in bw(n)[0]],
                          "tail":[round(float(x),3) for x in bw(n)[1]]}
                      for n in ("pelvis","spine","neck","head","hand_L","foot_L")}

print("AUDIT2_BEGIN")
print(json.dumps(out, indent=1))
print("AUDIT2_END")
