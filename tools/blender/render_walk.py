# SOMA — Blender headless: hornea el MOVIMIENTO DE LA FÍSICA de SOMA sobre el
# esqueleto de un modelo humano y renderiza un vídeo. El movimiento NO es una
# animación del modelo: son los ángulos articulares (cadera/rodilla/hombro) y la
# pose en el mundo que produce la simulación (render/motion.json).
#
# Uso:
#   blender --background --python tools/blender/render_walk.py -- \
#       <modelo.glb> render/motion.json render/soma_walk.mp4
#
# Ajustes (signos/ejes) al principio; si algo sale torcido, se afinan aquí.
import bpy, sys, json, math, os
from mathutils import Euler, Vector

# ---------- ajustes de retargeting ----------
HIP_SIGN, KNEE_SIGN, SHO_SIGN = 1.0, 1.0, 1.0
ARM_DOWN = 1.15          # bajar los brazos si el modelo está en T-pose (rad, 0 si A-pose)
FLEX_AXIS = 0            # eje local de flexión del hueso (0=X,1=Y,2=Z)
MODEL_FWD = 0.0          # ajuste del "frente" del modelo (0 o pi)

argv = sys.argv[sys.argv.index('--') + 1:]
model_path = argv[0] if len(argv) > 0 else 'viewer/assets/human.glb'
motion_path = argv[1] if len(argv) > 1 else 'render/motion.json'
out_path = argv[2] if len(argv) > 2 else 'render/soma_walk.mp4'

with open(motion_path) as fp:
    M = json.load(fp)
frames, fps, target = M['frames'], M['fps'], M['target']

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=model_path)

arm = next((o for o in bpy.data.objects if o.type == 'ARMATURE'), None)
if not arm:
    print("SIN ARMATURE en el modelo"); sys.exit(1)

def fb(inc, exc, side):
    S = ['left', '.l', '_l', ':l'] if side == 'l' else ['right', '.r', '_r', ':r']
    for b in arm.pose.bones:
        n = b.name.lower()
        if any(t in n for t in inc) and not any(t in n for t in exc) and any(s in n for s in S):
            return b.name
    return None

B = {
    'upLegL': fb(['upleg', 'upperleg', 'thigh'], [], 'l'),
    'legL':   fb(['lowerleg', 'leg', 'shin', 'calf'], ['up'], 'l'),
    'upLegR': fb(['upleg', 'upperleg', 'thigh'], [], 'r'),
    'legR':   fb(['lowerleg', 'leg', 'shin', 'calf'], ['up'], 'r'),
    'armL':   fb(['upperarm', 'arm'], ['fore', 'lower'], 'l'),
    'armR':   fb(['upperarm', 'arm'], ['fore', 'lower'], 'r'),
}
print("HUESOS:", B)

for pb in arm.pose.bones:
    pb.rotation_mode = 'XYZ'

def set_flex(bone_name, angle, frame):
    if not bone_name: return
    pb = arm.pose.bones[bone_name]
    e = [0.0, 0.0, 0.0]; e[FLEX_AXIS] = angle
    pb.rotation_euler = Euler(e, 'XYZ')
    pb.keyframe_insert('rotation_euler', frame=frame)

def set_arm(bone_name, swing, down_sign, frame):
    if not bone_name: return
    pb = arm.pose.bones[bone_name]
    # bajar el brazo (eje Z local aprox.) + balanceo de hombro (eje de flexión)
    e = [0.0, 0.0, down_sign * ARM_DOWN]; e[FLEX_AXIS] += SHO_SIGN * swing
    pb.rotation_euler = Euler(e, 'XYZ')
    pb.keyframe_insert('rotation_euler', frame=frame)

# pies al suelo: base z tal que el punto más bajo de la malla quede en z=0
zmin = min((arm.matrix_world @ v.co).z
           for o in arm.children if o.type == 'MESH' for v in o.data.vertices) \
       if any(o.type == 'MESH' for o in arm.children) else 0.0
base_z = -zmin

arm.rotation_mode = 'XYZ'
for i, fr in enumerate(frames):
    f = i + 1
    hL, kL, hR, kR, X, Y, th, bob, shL, shR = fr
    set_flex(B['upLegL'], HIP_SIGN * hL, f)
    set_flex(B['legL'],  KNEE_SIGN * max(0.0, -kL), f)
    set_flex(B['upLegR'], HIP_SIGN * hR, f)
    set_flex(B['legR'],  KNEE_SIGN * max(0.0, -kR), f)
    set_arm(B['armL'], shL, -1.0, f)
    set_arm(B['armR'], shR, +1.0, f)
    arm.location = Vector((X, Y, base_z + bob))
    arm.rotation_euler = Euler((0.0, 0.0, th + MODEL_FWD), 'XYZ')
    arm.keyframe_insert('location', frame=f)
    arm.keyframe_insert('rotation_euler', frame=f)

# ---------- escena: suelo, luces, baliza, cámara ----------
bpy.ops.mesh.primitive_plane_add(size=60, location=(3, 2, 0))
floor = bpy.context.active_object
mat = bpy.data.materials.new('floor'); mat.use_nodes = True
mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.08, 0.09, 0.11, 1)
floor.data.materials.append(mat)

sun = bpy.data.lights.new('sun', 'SUN'); sun.energy = 4.0
so = bpy.data.objects.new('sun', sun); bpy.context.collection.objects.link(so)
so.rotation_euler = Euler((math.radians(55), math.radians(20), math.radians(30)), 'XYZ')

world = bpy.data.worlds.new('W'); bpy.context.scene.world = world
world.use_nodes = True
world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.05, 0.055, 0.07, 1)

bpy.ops.mesh.primitive_uv_sphere_add(radius=0.14, location=(target[0], target[1], 0.9))
beacon = bpy.context.active_object
bm = bpy.data.materials.new('beacon'); bm.use_nodes = True
n = bm.node_tree.nodes['Principled BSDF']
n.inputs['Emission Color'].default_value = (1.0, 0.7, 0.28, 1) if 'Emission Color' in n.inputs else (0, 0, 0, 1)
if 'Emission Strength' in n.inputs: n.inputs['Emission Strength'].default_value = 4.0
beacon.data.materials.append(bm)

cam = bpy.data.cameras.new('cam'); co = bpy.data.objects.new('cam', cam)
bpy.context.collection.objects.link(co)
co.location = Vector((3.5, -6.5, 4.5))
# apuntar al centro del recorrido
direction = Vector((3.5, 2.0, 1.0)) - co.location
co.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
bpy.context.scene.camera = co

# ---------- render ----------
sc = bpy.context.scene
sc.render.engine = 'BLENDER_EEVEE' if 'BLENDER_EEVEE' in \
    [e.identifier for e in sc.render.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE_NEXT'
sc.render.fps = int(round(fps))
sc.frame_start = 1
sc.frame_end = len(frames)
sc.render.resolution_x = 1280
sc.render.resolution_y = 720
sc.render.image_settings.file_format = 'FFMPEG'
sc.render.ffmpeg.format = 'MPEG4'
sc.render.ffmpeg.codec = 'H264'
sc.render.filepath = os.path.abspath(out_path)
print("Renderizando %d frames a %s" % (len(frames), out_path))
bpy.ops.render.render(animation=True)
print("RENDER OK:", out_path)
