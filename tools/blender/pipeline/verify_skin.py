# SOMA — numeric skinning verification.
# 1) VG stats. 2) Bend knee + elbow 60 deg; measure worst gap between vertex
# pairs that touch at rest across the joint (thigh-mesh vert vs calf-mesh vert).
# Rigid skinning -> gap grows with distance to joint; smooth -> stays small.
import bpy, numpy as np, json
from mathutils import kdtree

rig = bpy.data.objects["SOMA_rig"]
A = np.array(rig.matrix_world)

def joint_world(bone):  # bone head in world
    b = rig.data.bones[bone]
    return A[:3, :3] @ np.array(b.head_local) + A[:3, 3]

meshes = [o for o in bpy.data.objects if o.type == 'MESH'
          and any(m.type == 'ARMATURE' for m in o.modifiers)]
multi = sum(1 for o in meshes if len(o.vertex_groups) > 1)
one = sum(1 for o in meshes if len(o.vertex_groups) == 1)
print(f"STATS skinned={len(meshes)} multiVG={multi} oneVG={one}", flush=True)

def eval_verts(o, dg):
    ev = o.evaluated_get(dg)
    m = ev.to_mesh()
    n = len(m.vertices)
    co = np.empty(n * 3)
    m.vertices.foreach_get("co", co)
    M = np.array(ev.matrix_world)
    out = co.reshape(-1, 3) @ M[:3, :3].T + M[:3, 3]
    ev.to_mesh_clear()
    return out

def gap_test(joint_bone, prox_vg, dist_vg, bend_bone):
    """pairs across the joint: verts of prox-primary vs dist-primary meshes
    within 6 mm at rest and within 10 cm of the joint."""
    J = joint_world(joint_bone)
    def side_objs(vgname):
        objs = []
        for o in meshes:
            if not o.vertex_groups or o.vertex_groups[0].name != vgname:
                continue
            c = np.array(o.matrix_world.translation)
            if np.linalg.norm(c - J) < 0.6:
                objs.append(o)
        return objs
    prox_o, dist_o = side_objs(prox_vg), side_objs(dist_vg)
    if not prox_o or not dist_o:
        print(f"GAP {joint_bone}: no mesh pair found", flush=True)
        return

    rig.data.pose_position = 'REST'
    dg = bpy.context.evaluated_depsgraph_get()
    dg.update()
    def collect(objs):
        pts, tags = [], []
        for o in objs:
            v = eval_verts(o, dg)
            near = np.linalg.norm(v - J, axis=1) < 0.10
            idx = np.nonzero(near)[0]
            for i in idx:
                pts.append(v[i]); tags.append((o.name, int(i)))
        return pts, tags
    ppts, ptags = collect(prox_o)
    dpts, dtags = collect(dist_o)
    if not ppts or not dpts:
        print(f"GAP {joint_bone}: no verts near joint", flush=True)
        return
    kd = kdtree.KDTree(len(dpts))
    for i, p in enumerate(dpts):
        kd.insert(p, i)
    kd.balance()
    pairs = []
    for i, p in enumerate(ppts):
        hit = kd.find(p)
        if hit[2] is not None and hit[2] < 0.006:
            pairs.append((ptags[i], dtags[hit[1]]))
    if not pairs:
        print(f"GAP {joint_bone}: no touching rest pairs", flush=True)
        return

    # bend
    pb = rig.pose.bones[bend_bone]
    pb.rotation_mode = 'XYZ'
    old = tuple(pb.rotation_euler)
    pb.rotation_euler = (np.radians(60), 0, 0)
    rig.data.pose_position = 'POSE'
    dg = bpy.context.evaluated_depsgraph_get()
    dg.update()
    cache = {}
    def wv(name):
        if name not in cache:
            cache[name] = eval_verts(bpy.data.objects[name], dg)
        return cache[name]
    gaps = [float(np.linalg.norm(wv(a[0])[a[1]] - wv(b[0])[b[1]])) for a, b in pairs]
    gaps = np.array(gaps)
    print(f"GAP {joint_bone}: pairs={len(gaps)} mean={gaps.mean()*1000:.1f}mm "
          f"p95={np.percentile(gaps,95)*1000:.1f}mm max={gaps.max()*1000:.1f}mm", flush=True)
    pb.rotation_euler = old
    rig.data.pose_position = 'REST'

# detach action so the pose is ours
if rig.animation_data:
    rig.animation_data.action = None

gap_test("calf_L", "thigh_L", "calf_L", "calf_L")
gap_test("lowerarm_L", "upperarm_L", "lowerarm_L", "lowerarm_L")
gap_test("calf_R", "thigh_R", "calf_R", "calf_R")
print("VERIFY_DONE", flush=True)
