# SOMA — visual en Unity (el motor sigue siendo C++)

Unity solo **dibuja** y **lee el teclado**. El agente/física es tu C++, compilado
como `soma.dll`. Unity manda tu intención → el motor responde → Unity pinta.

```
Unity (C#, teclado + render)  ⇄  soma.dll  (motor SOMA en C++)
```

## Pasos (5 min)

1. **Compila la DLL** (ya está hecha; para rehacerla):
   ```
   bash scripts/build_dll.sh
   ```
   Produce `C:\ALL\Coding\Human\soma.dll`.

2. **Crea un proyecto Unity** 3D (cualquier plantilla; Built-in o URP sirven).

3. **Coloca la DLL**: crea la carpeta `Assets/Plugins/` y copia ahí `soma.dll`.
   (Unity Editor es 64-bit → la DLL es 64-bit, compatible.)

4. **Coloca el script**: copia `unity/SomaEngine.cs` a `Assets/Scripts/SomaEngine.cs`.

5. **Escena**: crea un **GameObject vacío**, arrástrale el script `SomaEngine`.
   Asegúrate de que hay una **Main Camera** (la escena nueva ya la trae).

6. **Play**. Con la ventana de Game enfocada:
   - **W** (mantener) = caminar
   - **A / D** = girar izquierda / derecha
   - **SHIFT** = más rápido
   - **ESPACIO** = parar
   Arriba verás el HUD: estado, velocidad, posición, rumbo.

## Qué verás

Una figura de primitivas (torso, cabeza, dos piernas con rodilla, pies) que **camina
por física** cuando pulsas W. No es una animación: cada paso sale de
intención → CPG → actuadores no lineal → huesos → suelo, calculado por `soma.dll` en tiempo real.

## Si algo se ve al revés

En el Inspector del objeto con `SomaEngine`, invierte:
- `Heading Sign` — si gira al lado contrario.
- `Hip Sign` / `Knee Sign` — si las piernas flexionan al revés.

## Personaje completo (soma_character.glb + SomaCharacter.cs)

El personaje 3D rigged completo (84 huesos, 12 mallas skinned unidas) se exporta con:
```
"C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" -b models/soma_character.blend \
  --python <script export_glb.py>
```
Produce `models/soma_character.glb` (~mallas unidas por sistema, pesos incluidos).

1. **Package Manager** → añade `com.unity.cloud.gltfast` (glTFast, importador glTF oficial).
2. Copia `models/soma_character.glb` a `Assets/` — se importa como prefab.
3. Arrastra el prefab a la escena; crea un GameObject vacío con
   `Assets/Scripts/SomaCharacter.cs` y asigna `characterRoot` = la instancia del prefab.
4. Play. El script lee `soma_get_full_state` (32 floats) y conduce:
   - piernas FK (ángulos del motor), raíz con bob físico real (`pin_z=false`),
   - cadena espinal `spine_01..04` + `neck_01..02` (contrarrotación, cabeza estable),
   - brazos en antifase (péndulos físicos ArmRig del motor, `st[31..32]`),
   - `pulse_root` (late con la bomba, `st[21]`), `breath_L/R` (fuelle, `st[23]`),
   - contactos por pie (`st[25..26]`), CoM (`st[27..29]`) en el HUD.

### Estado completo (v1, 36 floats — `soma_full_state_size()`)
| idx | contenido |
|-----|-----------|
| 0..3 | hipL kneeL hipR kneeR (rad) |
| 4..8 | X Y heading bob speed |
| 9..12 | grfL grfR hipHeight walking |
| 13..20 | activaciones de actuadores (4 por pierna) |
| 21..24 | pulso (0..1) · presión norm. · respiración (0..1) · tasa fuelle |
| 25..26 | contacto pie L/R |
| 27..29 | centro de masa x y z |
| 30 | demanda |
| 31..32 | hombro L/R (rad, péndulos ArmRig físicos) |
| 33..35 | reservado |

## Nota

Cada vez que cambies el motor C++, recompila la DLL (`scripts/build_dll.sh`) y
reemplaza `Assets/Plugins/soma.dll` (cierra Unity antes; bloquea el archivo abierto).
