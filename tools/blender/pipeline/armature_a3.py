# SOMA A3 — jaw + eye pivot bones (geometric detection), leg/arm IK with pole
# targets, gated by rig["ik_on"] (0 = engine FK drives, 1 = IK).
import bpy, numpy as np, math
from mathutils import Vector

rig = bpy.data.objects["SOMA_rig"]
if rig.animation_data: rig.animation_data.action = None
AW = np.array(rig.matrix_world)

def verts_world(o):
    n = len(o.data.vertices)
    if n == 0: return np.empty((0,3))
    co = np.empty(n*3); o.data.vertices.foreach_get("co", co)
    M = np.array(o.matrix_world)
    return co.reshape(-1,3) @ M[:3,:3].T + M[:3,3]

def bone_P(name):
    from mathutils import Matrix
    b = rig.data.bones[name]
    return rig.matrix_world @ b.matrix_local @ Matrix.Translation((0, b.length, 0))

def rebind(o, bname):
    P_old = bone_P(o.parent_bone)
    world_rest = P_old @ o.matrix_parent_inverse @ o.matrix_basis
    basis = o.matrix_basis.copy()
    o.parent_bone = bname
    o.matrix_parent_inverse = bone_P(bname).inverted() @ world_rest @ basis.inverted()

# ---- collect head-parented rigid meshes with centroids
head_meshes = []
for o in bpy.data.objects:
    if o.type=='MESH' and o.parent_type=='BONE' and o.parent_bone=='head':
        w = verts_world(o)
        if len(w)==0: continue
        head_meshes.append((o, w.mean(0), w.min(0), w.max(0)))

# ---- eye clusters: small meshes, lateral, front-upper head
eyes = {"l": [], "r": []}
for o,c,mn,mx in head_meshes:
    size = (mx-mn).max()
    if size < 0.05 and 0.018 < abs(c[0]) < 0.055 and 1.55 < c[2] < 1.66 and c[1] < 0.015:
        eyes["l" if c[0]>0 else "r"].append((o,c))
piv = {}
for s in ("l","r"):
    if eyes[s]:
        piv[s] = np.mean([c for _,c in eyes[s]], axis=0)
    else:
        piv[s] = np.array([0.032 if s=="l" else -0.032, -0.045, 1.615])
print("EYE_L", len(eyes["l"]), [round(float(x),3) for x in piv["l"]],
      "EYE_R", len(eyes["r"]), [round(float(x),3) for x in piv["r"]], flush=True)

# ---- jaw candidates: rigid head meshes fully below occlusal plane, front
jaw_meshes = [o for o,c,mn,mx in head_meshes if mx[2] < 1.545 and c[1] < 0.02]
print("JAW_MESHES", len(jaw_meshes), flush=True)

# ---- create bones
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode='EDIT')
eb = rig.data.edit_bones
AWi = np.linalg.inv(AW)
def loc(p): return Vector(AWi[:3,:3] @ np.asarray(p,dtype=float) + AWi[:3,3])

def mk(nm, head, tail, parent, deform=True):
    if nm in eb: return
    b = eb.new(nm); b.head = loc(head); b.tail = loc(tail)
    b.parent = eb[parent]; b.use_deform = deform

mk("jaw", (0, 0.01, 1.555), (0, -0.085, 1.50), "head")
mk("eye_L", piv["l"], piv["l"] + np.array([0,-0.025,0]), "head")
mk("eye_R", piv["r"], piv["r"] + np.array([0,-0.025,0]), "head")

# IK control bones (non-deform)
for s, sign in (("L",1),("R",-1)):
    fb = rig.data.bones  # note: edit mode -> use eb positions of existing bones
    foot = eb[f"foot_{s}"]; hand = eb[f"hand_{s}"]
    knee_z = eb[f"calf_{s}"].head.z
    hipx = eb[f"thigh_{s}"].head.x
    mk(f"IK_foot_{s}", foot.head.copy(), foot.head + Vector((0,-0.12,0)), "pelvis", deform=False)
    mk(f"pole_knee_{s}", (hipx, -0.45, knee_z), (hipx, -0.55, knee_z), "pelvis", deform=False)
    elb = eb[f"lowerarm_{s}"].head
    mk(f"IK_hand_{s}", hand.head.copy(), hand.head + Vector((0,-0.10,0)), "spine_04", deform=False)
    mk(f"pole_elbow_{s}", (elb.x, 0.40, elb.z), (elb.x, 0.50, elb.z), "spine_04", deform=False)
bpy.ops.object.mode_set(mode='OBJECT')

# ---- re-parent eye + jaw meshes
for s in ("l","r"):
    for o,_ in eyes[s]:
        rebind(o, f"eye_{s.upper()}")
for o in jaw_meshes:
    rebind(o, "jaw")
print("REPARENT_OK", flush=True)

# ---- IK constraints gated by rig["ik_on"]
if "ik_on" not in rig: rig["ik_on"] = 0.0
rig.id_properties_ui("ik_on").update(min=0.0, max=1.0)

def add_ik(chain_end, target, pole, chain_count, pole_angle):
    pb = rig.pose.bones[chain_end]
    for c in list(pb.constraints):
        if c.type=='IK': pb.constraints.remove(c)
    c = pb.constraints.new('IK')
    c.target = rig; c.subtarget = target
    c.pole_target = rig; c.pole_subtarget = pole
    c.chain_count = chain_count
    c.pole_angle = pole_angle
    # influence driven by ik_on
    drv = c.driver_add("influence").driver
    drv.type = 'SCRIPTED'
    v = drv.variables.new(); v.name="k"; v.type='SINGLE_PROP'
    v.targets[0].id = rig; v.targets[0].data_path = '["ik_on"]'
    drv.expression = "k"
    return c

# choose pole_angle numerically: minimal foot drift at rest with ik_on=1
def drift(chain_end, target, pole, angle):
    c = add_ik(chain_end, target, pole, 2, angle)
    rig["ik_on"] = 1.0; rig.update_tag()
    dg = bpy.context.evaluated_depsgraph_get(); dg.update()
    ev = rig.evaluated_get(dg)
    tip = ev.pose.bones[chain_end].tail
    rest = rig.data.bones[chain_end].tail_local
    rig["ik_on"] = 0.0; rig.update_tag()
    return (np.array(tip) - np.array(rest))
best = {}
for s in ("L","R"):
    for chain_end, target, pole in ((f"calf_{s}", f"IK_foot_{s}", f"pole_knee_{s}"),
                                     (f"lowerarm_{s}", f"IK_hand_{s}", f"pole_elbow_{s}")):
        errs = []
        for ang in (0.0, math.pi/2, -math.pi/2, math.pi):
            d = np.linalg.norm(drift(chain_end, target, pole, ang))
            errs.append((d, ang))
        errs.sort()
        add_ik(chain_end, target, pole, 2, errs[0][1])
        best[chain_end] = (round(errs[0][0]*1000,1), round(math.degrees(errs[0][1])))
print("IK", best, flush=True)

bpy.ops.wm.save_mainfile()
print("SAVED_A3", flush=True)
