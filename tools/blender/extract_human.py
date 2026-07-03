# SOMA — extrae una malla humana rigged de la librería base de MB-Lab (sin usar sus
# operadores de generación) y la exporta a viewer/assets/human.glb.
# Uso: blender --background --python tools/blender/extract_human.py -- [female|male]
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
sex = argv[0] if argv else 'female'
MESH = 'MBLab_human_' + ('male' if sex == 'male' else 'female')
SKEL = 'MBLab_skeleton_base_fk'
LIB = r"C:/Users/Daniel/AppData/Roaming/Blender Foundation/Blender/3.6/scripts/addons/MB-Lab/data/humanoid_library.blend"
OUT = os.path.abspath('viewer/assets/human.glb')

bpy.ops.wm.read_factory_settings(use_empty=True)

with bpy.data.libraries.load(LIB, link=False) as (src, dst):
    dst.objects = [MESH, SKEL]

mesh = skel = None
for o in dst.objects:
    if o is None:
        continue
    bpy.context.collection.objects.link(o)
    if o.type == 'MESH': mesh = o
    if o.type == 'ARMATURE': skel = o

print("MESH", mesh.name if mesh else None, "verts", len(mesh.data.vertices) if mesh else 0,
      "vgroups", len(mesh.vertex_groups) if mesh else 0)
print("SKEL", skel.name if skel else None, "bones", len(skel.pose.bones) if skel else 0)
print("BONES", [b.name for b in skel.pose.bones] if skel else [])

# Enlaza malla ↔ esqueleto con el setup canónico (parent + modificador + pesos
# automáticos en una sola operación → el exportador glTF escribe bien el skin).
if mesh and skel:
    for o in bpy.context.selected_objects: o.select_set(False)
    mesh.select_set(True); skel.select_set(True)
    bpy.context.view_layer.objects.active = skel
    try:
        bpy.ops.object.parent_set(type='ARMATURE_AUTO')
        mesh.parent = None   # deja SOLO el modificador Armature (destraba el skin en glTF)
        for m in mesh.modifiers:
            if m.type == 'ARMATURE':
                m.object = skel; m.use_vertex_groups = True
        print("BIND_OK vgroups=", len(mesh.vertex_groups),
              "mods=", [(m.type, m.object.name if getattr(m, 'object', None) else None) for m in mesh.modifiers])
    except Exception as e:
        print("BIND_FAIL", repr(e))

# Exporta malla + esqueleto.
for o in bpy.context.selected_objects: o.select_set(False)
if mesh: mesh.select_set(True)
if skel: skel.select_set(True)
FBX = os.path.abspath('viewer/assets/human.fbx')
try:
    bpy.ops.export_scene.fbx(filepath=FBX, use_selection=True, add_leaf_bones=False,
                             bake_anim=False, object_types={'ARMATURE', 'MESH'})
    print("FBX_OK", FBX, os.path.getsize(FBX), "bytes")
except Exception as e:
    print("FBX_FAIL", repr(e))
