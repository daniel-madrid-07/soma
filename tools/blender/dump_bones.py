# SOMA — vuelca los nombres de hueso de un modelo, para ajustar el retargeting.
# Uso: blender --background --python tools/blender/dump_bones.py -- <modelo.glb|.fbx>
import bpy, sys, os

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
path = argv[0] if argv else 'viewer/assets/human.glb'
bpy.ops.wm.read_factory_settings(use_empty=True)
ext = os.path.splitext(path)[1].lower()
if ext == '.glb' or ext == '.gltf':
    bpy.ops.import_scene.gltf(filepath=path)
elif ext == '.fbx':
    bpy.ops.import_scene.fbx(filepath=path)
else:
    print("formato no soportado:", ext); sys.exit(1)

for ob in bpy.data.objects:
    if ob.type == 'ARMATURE':
        print("ARMATURE:", ob.name, "— %d huesos" % len(ob.pose.bones))
        for b in ob.pose.bones:
            print("  BONE:", b.name)
print("FIN")
