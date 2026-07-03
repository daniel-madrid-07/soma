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

## Después (lo visual "bonito")

Sustituir las primitivas por el **modelo humano rigged** (el MB-Lab que generamos):
importas el `.glb/.fbx` en Unity y, en vez de rotar cápsulas, rotas los huesos
`thigh/calf` del Animator con los mismos ángulos (`st[0..3]`). El puente C++ no cambia.

## Nota

Cada vez que cambies el motor C++, recompila la DLL (`scripts/build_dll.sh`) y
reemplaza `Assets/Plugins/soma.dll` (cierra Unity antes; bloquea el archivo abierto).
