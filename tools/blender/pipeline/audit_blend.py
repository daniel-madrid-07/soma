# SOMA — headless audit: rig, skinning state, shared datablocks, actions.
import bpy, json, collections

out = {}
out["version"] = bpy.app.version_string
scene = bpy.context.scene

objs = list(bpy.data.objects)
meshes = [o for o in objs if o.type == 'MESH']
arms = [o for o in objs if o.type == 'ARMATURE']
out["counts"] = {"objects": len(objs), "meshes": len(meshes), "armatures": len(arms)}

rig = bpy.data.objects.get("SOMA_rig")
if rig:
    bones = rig.data.bones
    out["rig"] = {
        "bones": len(bones),
        "bone_names": sorted(b.name for b in bones),
        "action": rig.animation_data.action.name if rig.animation_data and rig.animation_data.action else None,
    }

# skinning state
armmod = 0          # meshes with armature modifier
boneparent = 0      # meshes bone-parented
noparent = 0
multi_vg = 0        # meshes with >1 vertex group (smooth candidates already done)
one_vg = 0
zero_vg = 0
shared_data = 0     # remaining multi-user mesh datablocks
vg_bone_mismatch = 0
side_l = side_r = mid = 0
for o in meshes:
    if o.name.endswith(".l"): side_l += 1
    elif o.name.endswith(".r"): side_r += 1
    else: mid += 1
    if o.data.users > 1: shared_data += 1
    has_arm = any(m.type == 'ARMATURE' for m in o.modifiers)
    if has_arm: armmod += 1
    elif o.parent_type == 'BONE': boneparent += 1
    else: noparent += 1
    n = len(o.vertex_groups)
    if n == 0: zero_vg += 1
    elif n == 1: one_vg += 1
    else: multi_vg += 1
out["skin"] = {"armature_mod": armmod, "bone_parented": boneparent, "no_parent": noparent,
               "vg0": zero_vg, "vg1": one_vg, "vg_multi": multi_vg,
               "shared_data_meshes": shared_data, "L": side_l, "R": side_r, "mid": mid}

# vertex-group name distribution (which bones are used)
vg_names = collections.Counter()
for o in meshes:
    for vg in o.vertex_groups:
        vg_names[vg.name] += 1
out["vg_usage"] = dict(vg_names.most_common(40))

# actions
out["actions"] = [a.name for a in bpy.data.actions]

# collections (top level)
out["collections"] = [(c.name, len(c.all_objects)) for c in scene.collection.children]

# total verts
out["total_verts"] = sum(len(m.vertices) for m in bpy.data.meshes)

print("AUDIT_JSON_BEGIN")
print(json.dumps(out, indent=1))
print("AUDIT_JSON_END")
