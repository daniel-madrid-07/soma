# SOMA A2 — finger + toe bone chains, geometry-driven (no asset names).
# Fingertips = farthest vert clusters from the wrist over all hand-weighted
# meshes; 3 bones per digit (0.45/0.30/0.25); per-vertex weights re-blended
# between hand bone and digit bones. Toes: same along -Y from foot. Curl is
# exposed as rig["curl_hand_L"] etc. driving all digit-bone X rotations.
import bpy, numpy as np, time
from mathutils import Vector

t0 = time.time()
rig = bpy.data.objects["SOMA_rig"]
AW = np.array(rig.matrix_world)
AWi = np.linalg.inv(AW)

def bones_world(name):
    b = rig.data.bones[name]
    return (AW[:3,:3] @ np.array(b.head_local) + AW[:3,3],
            AW[:3,:3] @ np.array(b.tail_local) + AW[:3,3])

def dominant_vg(o):
    best, bw = None, -1
    tot = {g.index: 0.0 for g in o.vertex_groups}
    for v in o.data.vertices:
        for g in v.groups:
            if g.group in tot: tot[g.group] += g.weight
    for g in o.vertex_groups:
        if tot[g.index] > bw: best, bw = g.name, tot[g.index]
    return best

def collect(base):  # meshes rigid-parented to or dominantly weighted to base
    objs = []
    for o in bpy.data.objects:
        if o.type != 'MESH': continue
        if o.parent_type == 'BONE' and o.parent_bone == base:
            objs.append(o); continue
        if any(m.type=='ARMATURE' for m in o.modifiers) and o.vertex_groups:
            if dominant_vg(o) == base: objs.append(o)
    return objs

def verts_world(o):
    n = len(o.data.vertices)
    co = np.empty(n*3); o.data.vertices.foreach_get("co", co)
    M = np.array(o.matrix_world)
    return co.reshape(-1,3) @ M[:3,:3].T + M[:3,3]

def find_tips(pts, origin, k=5, min_sep=0.02):
    d = np.linalg.norm(pts - origin, axis=1)
    order = np.argsort(-d)
    tips = []
    for i in order:
        p = pts[i]
        if all(np.linalg.norm(p - q) > min_sep for q in tips):
            tips.append(p)
            if len(tips) == k: break
    return tips

made = []
def build_digits(base, prefix, nseg, fracs, tip_axis=None):
    """base bone -> digit chains toward the k farthest tip clusters."""
    objs = collect(base)
    if not objs:
        print("NO_MESHES", base, flush=True); return []
    pts = np.vstack([verts_world(o) for o in objs])
    W, Wt = bones_world(base)
    origin = W
    tips = find_tips(pts, origin, k=5)
    tips.sort(key=lambda p: p[0])  # order by x
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode='EDIT')
    eb = rig.data.edit_bones
    chains = []
    for di, T in enumerate(tips):
        Tl = AWi[:3,:3] @ T + AWi[:3,3]
        Wl = AWi[:3,:3] @ (origin + 0.45*(T-origin)) + AWi[:3,3]  # knuckle
        prev = base
        chain = []
        p0 = np.array(Wl)
        seg = np.array(Tl) - p0
        acc = 0.0
        for si in range(nseg):
            nm = f"{prefix}{di+1}_{si+1:02d}"
            if nm in eb:
                chain.append(nm); prev = nm; acc += fracs[si]; continue
            nb = eb.new(nm)
            nb.head = Vector(p0 + seg*acc)
            acc += fracs[si]
            nb.tail = Vector(p0 + seg*acc)
            nb.use_deform = True
            nb.parent = eb[prev]
            nb.use_connect = si > 0
            if tip_axis is not None:
                nb.align_roll(Vector(tip_axis))
            chain.append(nm); prev = nm
        chains.append(chain)
    bpy.ops.object.mode_set(mode='OBJECT')
    made.extend([b for c in chains for b in c])

    # ---- re-weight the meshes over base + digit bones (2-nearest segments)
    segs = {}
    for c in chains:
        for nm in c:
            segs[nm] = bones_world(nm)
    segs[base] = (W, Wt)
    names = list(segs.keys())
    H = np.array([segs[n][0] for n in names]); Tt = np.array([segs[n][1] for n in names])
    def segd(p, h, t):
        d = t-h; L2 = float(d@d)
        u = np.clip(((p-h)@d)/L2, 0, 1)
        return np.linalg.norm(p - (h + u[:,None]*d), axis=1)
    for o in objs:
        if o.data.users > 1: o.data = o.data.copy()
        w = verts_world(o)
        D = np.stack([segd(w, H[i], Tt[i]) for i in range(len(names))], axis=1)
        order = np.argsort(D, axis=1)
        i1, i2 = order[:,0], order[:,1]
        d1 = D[np.arange(len(w)), i1] + 1e-6
        d2 = D[np.arange(len(w)), i2] + 1e-6
        w1 = 1/d1**2; w2 = 1/d2**2; s = w1+w2
        w1 /= s; w2 /= s
        # sharpen: mostly-rigid pieces
        hard = w1 > 0.75
        w1[hard] = 1.0; w2[hard] = 0.0
        was_parented = o.parent_type == 'BONE' and o.parent_bone == base
        if was_parented:
            # convert to armature modifier so digit weights can act
            mw = o.matrix_world.copy()
            o.parent = None
            o.matrix_world = mw
            mod = o.modifiers.new("Armature", 'ARMATURE'); mod.object = rig
        for vg in list(o.vertex_groups): o.vertex_groups.remove(vg)
        idx = np.arange(len(w), dtype=np.int32)
        for col, wei in ((i1, w1), (i2, w2)):
            for bi in np.unique(col):
                m = (col == bi) & (wei > 0.01)
                if not m.any(): continue
                vg = o.vertex_groups.get(names[bi]) or o.vertex_groups.new(name=names[bi])
                q = np.round(wei[m]*100).astype(int)
                for val in np.unique(q):
                    if val <= 0: continue
                    vg.add(idx[m][q==val].tolist(), val/100.0, 'ADD')
    return chains

for side in ("L", "R"):
    build_digits(f"hand_{side}", f"finger{side}_", 3, (0.45, 0.30, 0.25), tip_axis=(0,-1,0))
    build_digits(f"foot_{side}", f"toe{side}_", 2, (0.55, 0.45), tip_axis=(0,0,1))
print("DIGITS", len(made), flush=True)

# ---- curl drivers: one scalar per extremity drives all its digit X-rotations
for side in ("L", "R"):
    for prefix, prop in ((f"finger{side}_", f"curl_hand_{side}"), (f"toe{side}_", f"curl_foot_{side}")):
        if prop not in rig: rig[prop] = 0.0
        id_props = rig.id_properties_ui(prop)
        id_props.update(min=-1.0, max=1.0)
        for pb in rig.pose.bones:
            if not pb.name.startswith(prefix): continue
            pb.rotation_mode = 'XYZ'
            for fc in [d for d in (rig.animation_data.drivers if rig.animation_data else [])]:
                pass
            drv = pb.driver_add("rotation_euler", 0).driver
            drv.type = 'SCRIPTED'
            v = drv.variables.new(); v.name = "c"; v.type = 'SINGLE_PROP'
            v.targets[0].id = rig
            v.targets[0].data_path = f'["{prop}"]'
            drv.expression = "c * 1.2"
print("DRIVERS_OK", flush=True)
bpy.ops.wm.save_mainfile()
print(f"SAVED in {time.time()-t0:.0f}s", flush=True)
