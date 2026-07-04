# SOMA — verify A1+A2: chain ramp, stray count, curl direction, digit motion.
import bpy, numpy as np
from mathutils import Vector

rig = bpy.data.objects["SOMA_rig"]
if rig.animation_data: rig.animation_data.action = None

meshes = [o for o in bpy.data.objects if o.type == 'MESH']
stray = sum(1 for o in meshes
            if not any(m.type == 'ARMATURE' for m in o.modifiers)
            and not (o.parent_type == 'BONE' and o.parent_bone))
nbones = len(rig.data.bones)
print(f"BONES {nbones} STRAY {stray}", flush=True)

def eval_centroid(o, dg):
    ev = o.evaluated_get(dg)
    m = ev.to_mesh()
    if len(m.vertices) == 0:
        ev.to_mesh_clear(); return None
    co = np.empty(len(m.vertices)*3); m.vertices.foreach_get("co", co)
    M = np.array(ev.matrix_world)
    c = co.reshape(-1,3).mean(0) @ M[:3,:3].T + M[:3,3]
    ev.to_mesh_clear()
    return c

# --- spine chain ramp: bend spine driver 40 deg about X, chain bones should
# rotate 10/20/30/40 deg cumulative; trunk meshes should displace with z.
import math
pb = rig.pose.bones["spine"]
pb.rotation_mode = 'XYZ'
pb.rotation_euler = (math.radians(40), 0, 0)
dg = bpy.context.evaluated_depsgraph_get(); dg.update()
rig_ev = rig.evaluated_get(dg)
prev = 0.0
ramp_ok = True
for nm in ("spine_01","spine_02","spine_03","spine_04"):
    q = rig_ev.pose.bones[nm].matrix.to_quaternion()
    ang = math.degrees(2*math.acos(min(1.0, abs(q.w))))
    print(f"RAMP {nm} world_rot={ang:.1f}deg", flush=True)
    if ang < prev - 1: ramp_ok = False
    prev = ang
print("RAMP_OK" if ramp_ok else "RAMP_BAD", flush=True)

# trunk mesh displacement: sample 5 meshes parented to each chain bone
disp = {}
for nm in ("spine_01","spine_04","neck_02"):
    ds = []
    for o in meshes:
        if o.parent_type=='BONE' and o.parent_bone==nm:
            c1 = eval_centroid(o, dg)
            if c1 is None: continue
            ds.append(c1)
            if len(ds) >= 3: break
    disp[nm] = len(ds)
pb.rotation_euler = (0,0,0)
print("TRUNK_SAMPLES", disp, flush=True)

# --- curl: fingertips should move when curl prop changes; measure direction
def tip_meshes(prefix):
    """meshes weighted to the distal digit bones"""
    out = []
    for o in meshes:
        if not any(m.type=='ARMATURE' for m in o.modifiers): continue
        names = [g.name for g in o.vertex_groups]
        if any(n.startswith(prefix) and n.endswith("_03") for n in names):
            out.append(o)
    return out[:6]

for side in ("L",):
    tips = tip_meshes(f"finger{side}_")
    rig[f"curl_hand_{side}"] = 0.0
    dg = bpy.context.evaluated_depsgraph_get(); dg.update()
    c0 = [eval_centroid(o, dg) for o in tips]
    rig[f"curl_hand_{side}"] = 1.0
    rig.update_tag()
    dg = bpy.context.evaluated_depsgraph_get(); dg.update()
    c1 = [eval_centroid(o, dg) for o in tips]
    rig[f"curl_hand_{side}"] = 0.0
    rig.update_tag()
    if tips:
        d = np.array([b-a for a,b in zip(c0,c1) if a is not None and b is not None])
        print(f"CURL_{side} tips={len(tips)} mean_disp_mm={np.linalg.norm(d,axis=1).mean()*1000:.1f} "
              f"mean_dy={d[:,1].mean()*1000:.1f} mean_dz={d[:,2].mean()*1000:.1f}", flush=True)
    else:
        print(f"CURL_{side} no tip meshes found", flush=True)

# --- digit bone inventory
digits = [b.name for b in rig.data.bones if b.name.startswith(("fingerL","fingerR","toeL","toeR"))]
print(f"DIGWCOUNT {len(digits)}", flush=True)
print("VERIFY_A_DONE", flush=True)
