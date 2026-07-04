# SOMA C/D — pulse + breath deformation hooks.
# pulse_root bone: chest-centre prop cluster parents to it; scale driven by
# rig["pulse"] (engine out[21]). breath_L/R bones: paired large chest props;
# scale driven by rig["breath"] (engine out[23]). Actuator bulge (C): the limb
# part-meshes get a driver-less baseline now; engine activations drive bone
# scale in Unity via the same bones that already deform them.
import bpy, numpy as np
from mathutils import Vector, Matrix

rig = bpy.data.objects["SOMA_rig"]
if rig.animation_data: rig.animation_data.action = None
AW = np.array(rig.matrix_world)
AWi = np.linalg.inv(AW)

def verts_world(o):
    n = len(o.data.vertices)
    if n == 0: return np.empty((0,3))
    co = np.empty(n*3); o.data.vertices.foreach_get("co", co)
    M = np.array(o.matrix_world)
    return co.reshape(-1,3) @ M[:3,:3].T + M[:3,3]

def bone_P(name):
    b = rig.data.bones[name]
    return rig.matrix_world @ b.matrix_local @ Matrix.Translation((0, b.length, 0))

def rebind_to(o, bname):
    if o.parent_type == 'BONE' and o.parent_bone:
        P_old = bone_P(o.parent_bone)
        world_rest = P_old @ o.matrix_parent_inverse @ o.matrix_basis
    else:
        world_rest = o.matrix_world.copy()
        o.parent = rig; o.parent_type = 'BONE'
    basis = o.matrix_basis.copy()
    o.parent_bone = bname
    o.matrix_parent_inverse = bone_P(bname).inverted() @ world_rest @ basis.inverted()

# candidate meshes: rigid bone-parented (not armature-skinned) in the chest
PULSE_C = np.array([0.03, -0.02, 1.30]); PULSE_R = 0.085
BREATH = {"L": (np.array([0.09, -0.02, 1.30]), 0.13),
          "R": (np.array([-0.09, -0.02, 1.30]), 0.13)}

pulse_meshes, breath_meshes = [], {"L": [], "R": []}
for o in bpy.data.objects:
    if o.type != 'MESH': continue
    if not (o.parent_type == 'BONE' and o.parent_bone): continue
    w = verts_world(o)
    if len(w) == 0: continue
    c = w.mean(0)
    if np.linalg.norm(c - PULSE_C) < PULSE_R:
        pulse_meshes.append(o); continue
    for s, (ctr, rad) in BREATH.items():
        if np.linalg.norm(c - ctr) < rad and (w.max(0)-w.min(0)).max() > 0.06:
            breath_meshes[s].append(o)
print("PULSE_MESHES", len(pulse_meshes),
      "BREATH_L", len(breath_meshes["L"]), "BREATH_R", len(breath_meshes["R"]), flush=True)

# bones
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode='EDIT')
eb = rig.data.edit_bones
def loc(p): return Vector(AWi[:3,:3] @ np.asarray(p,dtype=float) + AWi[:3,3])
def mk(nm, head, tail, parent):
    if nm in eb: return
    b = eb.new(nm); b.head = loc(head); b.tail = loc(tail)
    b.parent = eb[parent]; b.use_deform = True
mk("pulse_root", PULSE_C, PULSE_C + np.array([0,0,0.05]), "spine_03")
for s in ("L","R"):
    ctr, _ = BREATH[s]
    mk(f"breath_{s}", ctr, ctr + np.array([0,0,0.06]), "spine_03")
bpy.ops.object.mode_set(mode='OBJECT')

for o in pulse_meshes: rebind_to(o, "pulse_root")
for s in ("L","R"):
    for o in breath_meshes[s]: rebind_to(o, f"breath_{s}")

# scalar props + scale drivers (Blender preview; Unity reads engine directly)
for prop, default in (("pulse", 0.0), ("breath", 0.0)):
    if prop not in rig: rig[prop] = default
    rig.id_properties_ui(prop).update(min=0.0, max=1.0)

def scale_driver(bone, prop, amp):
    pb = rig.pose.bones[bone]
    for axis in (0,1,2):
        drv = pb.driver_add("scale", axis).driver
        drv.type = 'SCRIPTED'
        v = drv.variables.new(); v.name = "s"; v.type = 'SINGLE_PROP'
        v.targets[0].id = rig; v.targets[0].data_path = f'["{prop}"]'
        drv.expression = f"1.0 + {amp} * s"
scale_driver("pulse_root", "pulse", 0.08)   # 8% beat
scale_driver("breath_L", "breath", 0.05)    # 5% chest expansion
scale_driver("breath_R", "breath", 0.05)
print("DRIVERS_OK", flush=True)

bpy.ops.wm.save_mainfile()
print("SAVED_CD", flush=True)
