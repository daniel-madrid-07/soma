# SOMA — genera un humano con MB-Lab y lo exporta a viewer/assets/human.glb.
# MB-Lab necesita contexto GUI real; por eso este script se ejecuta con un TIMER
# (Blender abierto, no --background), genera, exporta y cierra Blender solo.
# Uso:  blender --python tools/blender/make_human.py -- [caracter]
import bpy, addon_utils, os, sys

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
WANT = argv[0] if argv else None
OUT = os.path.abspath('viewer/assets/human.glb')


def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()


def pick_and_init():
    sc = bpy.context.scene
    cands = [WANT] if WANT else ['f_ca01', 'm_ca01', 'f_af01', 'm_af01', 'f_as01', 'f_la01']
    for c in cands:
        if not c:
            continue
        try:
            sc.mblab_character_name = c
            print("SET_CHAR", c); break
        except Exception as e:
            print("BAD_CHAR", c, repr(e))
    for prop, val in [('mblab_use_cycles', False), ('mblab_use_eevee', True),
                      ('mblab_use_lamps', False), ('mblab_use_ik', False),
                      ('mblab_use_muscle', False)]:
        if hasattr(sc, prop):
            try: setattr(sc, prop, val)
            except Exception: pass
    print("INIT…")
    bpy.ops.mbast.init_character()
    print("OBJETOS", [o.name + ':' + o.type for o in bpy.data.objects])


def finalize_and_export():
    try:
        bpy.ops.mbast.finalize_character()
        print("FINALIZE_OK")
    except Exception as e:
        print("FINALIZE_FAIL", repr(e))
    bpy.ops.object.select_all(action='SELECT')
    try:
        bpy.ops.export_scene.gltf(filepath=OUT, export_format='GLB',
                                  export_apply=True, export_skins=True, use_selection=True)
        print("EXPORT_OK", OUT, os.path.getsize(OUT), "bytes")
    except Exception as e:
        print("EXPORT_FAIL", repr(e))
    for ob in bpy.data.objects:
        if ob.type == 'ARMATURE':
            print("BONES", [b.name for b in ob.pose.bones]); break


def run():
    try:
        for name in ['MB-Lab', 'mb_lab', 'MB_Lab', 'mblab']:
            try: addon_utils.enable(name, default_set=True)
            except Exception: pass
        clear_scene()
        pick_and_init()
        finalize_and_export()
    except Exception as e:
        print("RUN_EXC", repr(e))
    print("FIN")
    if not bpy.app.background:
        bpy.ops.wm.quit_blender()
    return None


if bpy.app.background:
    run()
else:
    # Espera a que la GUI esté lista y ejecuta una vez.
    bpy.app.timers.register(run, first_interval=2.0)
