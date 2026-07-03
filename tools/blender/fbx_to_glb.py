# SOMA — convierte FBX (p. ej. exportado de MakeHuman) a GLB.
# Uso: blender --background --python tools/blender/fbx_to_glb.py -- <in.fbx> <out.glb>
import bpy, sys

argv = sys.argv[sys.argv.index('--') + 1:]
inp, out = argv[0], argv[1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=inp)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB',
                          export_apply=False, export_skins=True, export_yup=True)
print("GLB escrito en", out)
