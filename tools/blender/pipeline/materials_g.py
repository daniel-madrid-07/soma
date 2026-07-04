# SOMA G — PBR upgrade of all materials + 3-point lighting, headless.
# Classification WITHOUT names: by base-color HSV of each material.
#   saturated red/warm -> wet organic (low roughness, SSS)
#   bright desaturated -> matte ivory (mid roughness, light SSS)
#   dark/other         -> generic organic
import bpy, colorsys

def principled(mat):
    if not mat.use_nodes: mat.use_nodes = True
    nt = mat.node_tree
    for n in nt.nodes:
        if n.type == 'BSDF_PRINCIPLED': return n
    # sin Principled: rescata el color base del shader que haya y sustitúyelo
    base = None
    for n in nt.nodes:
        for sock in n.inputs:
            if sock.name in ("Base Color", "Color") and hasattr(sock, "default_value"):
                try:
                    base = tuple(sock.default_value)[:4]; break
                except TypeError:
                    pass
        if base: break
    out = next((n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL'), None)
    if out is None:
        out = nt.nodes.new('ShaderNodeOutputMaterial')
    p = nt.nodes.new('ShaderNodeBsdfPrincipled')
    if base:
        p.inputs["Base Color"].default_value = (*base[:3], 1.0)
    nt.links.new(p.outputs["BSDF"], out.inputs["Surface"])
    return p

n_wet = n_ivory = n_gen = n_skip = 0
for mat in bpy.data.materials:
    p = principled(mat)
    if p is None: n_skip += 1; continue
    base = p.inputs["Base Color"].default_value[:3]
    h, s, v = colorsys.rgb_to_hsv(*base)
    warm = (h < 0.10 or h > 0.90)
    p.inputs["Metallic"].default_value = 0.0
    if s > 0.25 and warm:
        p.inputs["Roughness"].default_value = 0.28          # húmedo especular
        p.inputs["Subsurface Weight"].default_value = 0.30
        p.inputs["Subsurface Radius"].default_value = (0.012, 0.004, 0.003)
        n_wet += 1
    elif v > 0.55 and s < 0.35:
        p.inputs["Roughness"].default_value = 0.50          # marfil mate
        p.inputs["Subsurface Weight"].default_value = 0.12
        p.inputs["Subsurface Radius"].default_value = (0.006, 0.005, 0.004)
        n_ivory += 1
    else:
        p.inputs["Roughness"].default_value = 0.40
        p.inputs["Subsurface Weight"].default_value = 0.18
        p.inputs["Subsurface Radius"].default_value = (0.010, 0.005, 0.004)
        n_gen += 1
    if "Coat Weight" in p.inputs and s > 0.25 and warm:
        p.inputs["Coat Weight"].default_value = 0.25        # brillo húmedo extra
print(f"MATS wet={n_wet} ivory={n_ivory} gen={n_gen} skip={n_skip}", flush=True)

# ---- lighting: key/fill existentes + rim nuevo + mundo suave
sc = bpy.context.scene
def ensure_light(name, kind, energy, loc, rot):
    ob = bpy.data.objects.get(name)
    if ob is None:
        data = bpy.data.lights.new(name, kind)
        ob = bpy.data.objects.new(name, data)
        sc.collection.objects.link(ob)
    elif ob.data.type != kind:
        ob.data = bpy.data.lights.new(name, kind)  # p.ej. SUN existente -> AREA
    ob.data.energy = energy
    ob.location = loc
    ob.rotation_euler = rot
    return ob
import math
ensure_light("SOMA_key", 'AREA', 400, (2.2, -2.4, 2.4), (math.radians(55), 0, math.radians(40)))
ensure_light("SOMA_fill", 'AREA', 120, (-2.6, -1.8, 1.4), (math.radians(75), 0, math.radians(-55)))
ensure_light("SOMA_rim", 'AREA', 300, (0.4, 2.8, 2.2), (math.radians(-60), 0, math.radians(180)))
for nm in ("SOMA_key", "SOMA_fill", "SOMA_rim"):
    bpy.data.objects[nm].data.size = 1.6
if sc.world is None:
    sc.world = bpy.data.worlds.new("SOMA_world")
sc.world.use_nodes = True
bg = sc.world.node_tree.nodes.get("Background")
if bg:
    bg.inputs[0].default_value = (0.015, 0.017, 0.022, 1.0)  # estudio oscuro
    bg.inputs[1].default_value = 1.0
print("LIGHTS_OK", flush=True)

# ---- cámara y ajustes de render (EEVEE, DOF ligero)
cam = bpy.data.objects.get("SOMA_cam")
if cam is None:
    cd = bpy.data.cameras.new("SOMA_cam")
    cam = bpy.data.objects.new("SOMA_cam", cd)
    sc.collection.objects.link(cam)
cam.location = (2.4, -3.0, 1.5)
cam.rotation_euler = (math.radians(80), 0, math.radians(38))
cam.data.lens = 65
cam.data.dof.use_dof = True
cam.data.dof.focus_distance = 3.4
cam.data.dof.aperture_fstop = 2.8
sc.camera = cam
try: sc.render.engine = 'BLENDER_EEVEE_NEXT'
except Exception: sc.render.engine = 'BLENDER_EEVEE'
sc.eevee.taa_render_samples = 24
print("CAM_OK engine=" + sc.render.engine, flush=True)

bpy.ops.wm.save_mainfile()
print("SAVED_G", flush=True)

# ---- render de control (bajo, headless)
sc.render.resolution_x = 640; sc.render.resolution_y = 640
sc.render.filepath = bpy.path.abspath("//..") + "/render/control_g.png"
bpy.ops.render.render(write_still=True)
print("RENDER_OK", sc.render.filepath, flush=True)
