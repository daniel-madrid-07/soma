# SOMA — smooth multi-bone skinning, headless.
# For every mesh with an Armature modifier + exactly 1 vertex group (rigid),
# blend weights across the adjacent same-side chain bones near each joint.
# Weights: projection along the primary bone axis, smoothstep in a blend zone
# centred at the bone head (toward parent) and tail (toward child).
# Shared mesh datablocks are made single-user BEFORE any weight write.
import bpy, numpy as np, time, sys

BLEND_MIN, BLEND_MAX, BLEND_K = 0.03, 0.08, 0.18
EPS = 0.005  # ignore weights below this

rig = bpy.data.objects["SOMA_rig"]
A = np.array(rig.matrix_world)

# neighbor map: bone -> (parent, child) within its same-side chain
NEI = {
    "pelvis": (None, "spine"), "spine": ("pelvis", "neck"),
    "neck": ("spine", "head"), "head": ("neck", None),
}
for s in ("L", "R"):
    NEI[f"clavicle_{s}"] = ("spine", f"upperarm_{s}")
    NEI[f"upperarm_{s}"] = (f"clavicle_{s}", f"lowerarm_{s}")
    NEI[f"lowerarm_{s}"] = (f"upperarm_{s}", f"hand_{s}")
    NEI[f"hand_{s}"] = (f"lowerarm_{s}", None)
    NEI[f"thigh_{s}"] = ("pelvis", f"calf_{s}")
    NEI[f"calf_{s}"] = (f"thigh_{s}", f"foot_{s}")
    NEI[f"foot_{s}"] = (f"calf_{s}", None)

def bworld(name):
    b = rig.data.bones[name]
    h = A[:3, :3] @ np.array(b.head_local) + A[:3, 3]
    t = A[:3, :3] @ np.array(b.tail_local) + A[:3, 3]
    return h, t, float(np.linalg.norm(t - h))

BONES = {n: bworld(n) for n in NEI}

def ss(x):  # smoothstep, clamped
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3.0 - 2.0 * x)

def blend_width(a, b):
    return float(np.clip(BLEND_K * min(BONES[a][2], BONES[b][2]), BLEND_MIN, BLEND_MAX))

def assign(o, vg, idx, w):
    # bucket by weight quantized to 0.01 -> few vg.add calls
    q = np.round(w * 100).astype(np.int32)
    for val in np.unique(q):
        if val <= 0:
            continue
        sel = idx[q == val]
        vg.add(sel.tolist(), val / 100.0, 'ADD')

t0 = time.time()
processed = modified = singled = skipped = 0
targets = []
for o in bpy.data.objects:
    if o.type != 'MESH':
        continue
    if not any(m.type == 'ARMATURE' for m in o.modifiers):
        continue
    if len(o.vertex_groups) != 1 or o.vertex_groups[0].name not in NEI:
        continue
    targets.append(o)
print(f"TARGETS {len(targets)}", flush=True)

for o in targets:
    processed += 1
    prim = o.vertex_groups[0].name
    par, chi = NEI[prim]
    h, t, L = BONES[prim]
    axis = (t - h) / L
    M = np.array(o.matrix_world)
    n = len(o.data.vertices)
    co = np.empty(n * 3, dtype=np.float64)
    o.data.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3) @ M[:3, :3].T + M[:3, 3]
    tt = (co - h) @ axis  # param along primary bone, metres from head

    wp = np.zeros(n)
    wc = np.zeros(n)
    if par is not None:
        Bh = blend_width(prim, par)
        if tt.min() < Bh:
            wp = 1.0 - ss((tt + Bh) / (2 * Bh))
    if chi is not None:
        Bt = blend_width(prim, chi)
        if tt.max() > L - Bt:
            u = tt - L
            wc = ss((u + Bt) / (2 * Bt))
    if wp.max() < EPS and wc.max() < EPS:
        skipped += 1
        if processed % 200 == 0:
            print(f"... {processed}/{len(targets)} mod={modified} {time.time()-t0:.0f}s", flush=True)
        continue

    if o.data.users > 1:  # shared-datablock trap: copy before writing weights
        o.data = o.data.copy()
        singled += 1

    wb = np.clip(1.0 - wp - wc, 0.0, 1.0)
    tot = wp + wb + wc
    wp, wb, wc = wp / tot, wb / tot, wc / tot

    idx = np.arange(n, dtype=np.int32)
    # wipe the old rigid group, rewrite everything
    for vg in list(o.vertex_groups):
        o.vertex_groups.remove(vg)
    for name, w in ((par, wp), (prim, wb), (chi, wc)):
        if name is None or w.max() < EPS:
            continue
        vg = o.vertex_groups.new(name=name)
        assign(o, vg, idx, w)
    modified += 1
    if processed % 200 == 0:
        print(f"... {processed}/{len(targets)} mod={modified} {time.time()-t0:.0f}s", flush=True)

print(f"DONE processed={processed} modified={modified} singled={singled} "
      f"skipped={skipped} in {time.time()-t0:.0f}s", flush=True)
bpy.ops.wm.save_mainfile()
print("SAVED", bpy.data.filepath, flush=True)
