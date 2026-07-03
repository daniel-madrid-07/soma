# SOMA — sonda: comprueba Blender + MB-Lab en modo headless.
import bpy, addon_utils
print("VERSION", bpy.app.version_string)
mods = [m.__name__ for m in addon_utils.modules()]
print("ADDONS_LAB", [m for m in mods if 'lab' in m.lower() or m.lower().startswith('mb')])
for name in ['MB-Lab', 'mb_lab', 'MB_Lab', 'mblab']:
    try:
        addon_utils.enable(name, default_set=True)
        print("ENABLED_OK", name)
    except Exception as e:
        print("ENABLE_FAIL", name, repr(e))
print("HAS_MBAST", hasattr(bpy.ops, 'mbast'))
sc = bpy.context.scene
print("MBLAB_PROPS", [p for p in dir(sc) if 'mblab' in p.lower()][:20])
