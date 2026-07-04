# SOMA A1 — segmented spine/neck chain + attach all stray meshes.
# - spine_01..04 + neck_01..02 deform bones, COPY_ROTATION (local, fractional)
#   from the engine-driven `spine`/`neck` bones -> smooth trunk bend, engine
#   retarget unchanged (still drives the original 18).
# - Re-parent spine/neck bone-parented meshes to the nearest chain segment.
# - Attach 1492 unparented meshes: z-extent > 0.20 m -> armature modifier with
#   per-vertex 2-nearest-bone inverse-square weights; else rigid bone-parent.
# - Pure data API (no bpy.ops parenting) to avoid the crash path.
import bpy, numpy as np, time
from mathutils import Matrix, Vector

t0 = time.time()
rig = bpy.data.objects["SOMA_rig"]
AW = np.array(rig.matrix_world)

# ---------- 1. chain bones ----------
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode='EDIT')
eb = rig.data.edit_bones

def add_chain(base, parent_name, n, prefix):
    b = eb[base]
    h, t = np.array(b.head), np.array(b.tail)
    names = []
    prev = parent_name
    for i in range(n):
        nm = f"{prefix}_{i+1:02d}"
        if nm in eb:
            names.append(nm); prev = nm; continue
        nb = eb.new(nm)
        nb.head = Vector(h + (t - h) * (i / n))
        nb.tail = Vector(h + (t - h) * ((i + 1) / n))
        nb.roll = b.roll
        nb.parent = eb[prev]
        nb.use_deform = True
        names.append(nm); prev = nm
    return names

spine_chain = add_chain("spine", "pelvis", 4, "spine")
neck_chain = add_chain("neck", spine_chain[-1], 2, "neck")
bpy.ops.object.mode_set(mode='OBJECT')

# constraints: each chain bone copies a fraction of the driver, accumulating
for names, driver in ((spine_chain, "spine"), (neck_chain, "neck")):
    f = 1.0 / len(names)
    for nm in names:
        pb = rig.pose.bones[nm]
        if any(c.type == 'COPY_ROTATION' for c in pb.constraints):
            continue
        c = pb.constraints.new('COPY_ROTATION')
        c.target = rig; c.subtarget = driver
        c.target_space = 'LOCAL'; c.owner_space = 'LOCAL'
        c.influence = f
print("CHAIN_OK", spine_chain, neck_chain, flush=True)

# ---------- helpers ----------
def bone_P(name):  # rest-pose parent matrix used by BONE parenting (at tail)
    b = rig.data.bones[name]
    return rig.matrix_world @ b.matrix_local @ Matrix.Translation((0, b.length, 0))

def true_centroid_and_bbox(o):
    n = len(o.data.vertices)
    if n == 0:
        c = np.array(o.matrix_world.translation)
        return c, c, c, np.empty((0, 3))
    co = np.empty(n * 3)
    o.data.vertices.foreach_get("co", co)
    M = np.array(o.matrix_world)
    w = co.reshape(-1, 3) @ M[:3, :3].T + M[:3, 3]
    return w.mean(0), w.min(0), w.max(0), w

DEFORM = [b.name for b in rig.data.bones
          if b.use_deform and b.name not in ("spine", "neck")]
SEG = {}
for nm in DEFORM:
    b = rig.data.bones[nm]
    h = AW[:3, :3] @ np.array(b.head_local) + AW[:3, 3]
    t = AW[:3, :3] @ np.array(b.tail_local) + AW[:3, 3]
    SEG[nm] = (h, t)

def seg_dist(pts, h, t):  # vectorized point-segment distance
    d = t - h
    L2 = float(d @ d)
    tt = np.clip(((pts - h) @ d) / L2, 0, 1)
    proj = h + tt[:, None] * d
    return np.linalg.norm(pts - proj, axis=1)

def rebind(o, bname, world_rest):
    basis = o.matrix_basis.copy()
    o.parent = rig; o.parent_type = 'BONE'; o.parent_bone = bname
    o.matrix_parent_inverse = bone_P(bname).inverted() @ world_rest @ basis.inverted()

# ---------- 2. re-parent spine/neck meshes to chain segments ----------
moved = 0
chain_all = spine_chain + neck_chain
zspan = {nm: (SEG[nm][0][2], SEG[nm][1][2]) for nm in chain_all}
for o in list(bpy.data.objects):
    if o.type != 'MESH' or o.parent_type != 'BONE':
        continue
    if o.parent_bone not in ("spine", "neck"):
        continue
    P_old = bone_P(o.parent_bone)
    world_rest = P_old @ o.matrix_parent_inverse @ o.matrix_basis
    try:
        c, mn, mx, _ = true_centroid_and_bbox(o)
    except Exception as e:
        print("TRUNK_FAIL", o.name, repr(e), flush=True)
        continue
    z = c[2]
    if z < zspan[spine_chain[0]][0]:
        target = "pelvis"
    else:
        target = chain_all[-1]
        for nm in chain_all:
            if zspan[nm][0] <= z <= zspan[nm][1]:
                target = nm; break
        else:
            if z > zspan[chain_all[-1]][1]:
                target = chain_all[-1]
    rebind(o, target, world_rest)
    moved += 1
print(f"TRUNK_REPARENTED {moved} in {time.time()-t0:.0f}s", flush=True)

# ---------- 3. attach unparented meshes ----------
rigid = smooth = failed = 0
names_seg = list(SEG.keys())
H = np.array([SEG[n][0] for n in names_seg])
T = np.array([SEG[n][1] for n in names_seg])

def assign_w(o, vgname, idx, w):
    vg = o.vertex_groups.get(vgname) or o.vertex_groups.new(name=vgname)
    q = np.round(w * 100).astype(np.int32)
    for val in np.unique(q):
        if val <= 0: continue
        sel = idx[q == val]
        vg.add(sel.tolist(), val / 100.0, 'ADD')

count = 0
for o in list(bpy.data.objects):
    if o.type != 'MESH': continue
    if any(m.type == 'ARMATURE' for m in o.modifiers): continue
    if o.parent_type == 'BONE' and o.parent_bone: continue
    if o.parent is not None and o.parent_type != 'BONE':  # parented to object?
        pass  # still attach by geometry
    count += 1
    try:
        c, mn, mx, w = true_centroid_and_bbox(o)
        extent = mx - mn
        if max(extent) > 0.20 and len(w) > 40:
            # elongated -> smooth per-vertex weights over 2 nearest bones
            if o.data.users > 1:
                o.data = o.data.copy()
            D = np.stack([seg_dist(w, H[i], T[i]) for i in range(len(names_seg))], axis=1)
            order = np.argsort(D, axis=1)
            d1 = D[np.arange(len(w)), order[:, 0]] + 1e-6
            d2 = D[np.arange(len(w)), order[:, 1]] + 1e-6
            w1 = (1 / d1**2); w2 = (1 / d2**2)
            s = w1 + w2
            w1, w2 = w1 / s, w2 / s
            idx = np.arange(len(w), dtype=np.int32)
            for col, wei in ((order[:, 0], w1), (order[:, 1], w2)):
                for bi in np.unique(col):
                    m = col == bi
                    assign_w(o, names_seg[bi], idx[m], wei[m])
            mod = o.modifiers.new("Armature", 'ARMATURE')
            mod.object = rig
            smooth += 1
        else:
            d = np.array([seg_dist(c[None, :], H[i], T[i])[0] for i in range(len(names_seg))])
            rebind(o, names_seg[int(d.argmin())], o.matrix_world.copy())
            rigid += 1
    except Exception as e:
        failed += 1
        print("FAIL", o.name, repr(e), flush=True)
    if count % 200 == 0:
        print(f"... attach {count} rigid={rigid} smooth={smooth} {time.time()-t0:.0f}s", flush=True)

print(f"ATTACH_DONE rigid={rigid} smooth={smooth} failed={failed} in {time.time()-t0:.0f}s", flush=True)
bpy.ops.wm.save_mainfile()
print("SAVED", flush=True)
