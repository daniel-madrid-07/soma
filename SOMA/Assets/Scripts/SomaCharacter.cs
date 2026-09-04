// SOMA — conduce el personaje glTF importado (soma_character.glb) con el estado
// COMPLETO del motor C++ (soma_get_full_state, 32 floats). Unity solo dibuja.
//
// USO:
//   1) Importa models/soma_character.glb al proyecto (necesita un importador
//      glTF, p.ej. "glTFast" del Package Manager).
//   2) Arrastra la instancia a la escena y este script a un GameObject vacío.
//   3) Asigna 'characterRoot' = raíz de la instancia (contiene SOMA_rig).
//   4) Play: W caminar · A/D girar · SHIFT rápido · ESPACIO parar.
//
// Los huesos extra (spine_01.., dedos, pulse_root, breath_L/R) se conducen aquí
// con las MISMAS reglas que los drivers de Blender: fracciones del hueso guía y
// escalares del motor. No hay animación grabada en ninguna parte.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class SomaCharacter : MonoBehaviour
{
    [DllImport("soma")] static extern IntPtr soma_create();
    [DllImport("soma")] static extern void soma_destroy(IntPtr p);
    [DllImport("soma")] static extern void soma_set_intention(IntPtr p, int walk, float effort, float steer);
    [DllImport("soma")] static extern void soma_step(IntPtr p, float frame);
    [DllImport("soma")] static extern int soma_full_state_size();
    [DllImport("soma")] static extern void soma_get_full_state(IntPtr p, [Out] float[] outState);

    public Transform characterRoot;          // instancia del GLB (contiene SOMA_rig)
    public float hipSign = 1f, kneeSign = -1f, headingSign = 1f;
    public bool autoWalk = false;            // caminar sin teclado (para pruebas)
    public float faceYaw = 90f;              // giro para que MIRE hacia donde anda
    public float legGain = 1.5f;             // amplifica la zancada
    public float kneeGain = 1.25f;           // amplifica la flexión de rodilla
    public float armGain = 1.7f;             // balanceo de brazos
    public float elbowGain = 0.5f;           // flexión de codo
    public bool walkInPlace = false;         // andar sin desplazarse (para revisar)

    IntPtr eng;
    float[] st;
    readonly Dictionary<string, Transform> bones = new();
    readonly Dictionary<string, Quaternion> rest = new();
    readonly Dictionary<string, Vector3> restScale = new();
    readonly Dictionary<string, Vector3> lateralAxis = new();  // eje lateral del mundo en espacio local del hueso

    static readonly string[] kDriven = {
        "pelvis","spine","neck","head","thigh_L","thigh_R","calf_L","calf_R",
        "foot_L","foot_R","upperarm_L","upperarm_R","lowerarm_L","lowerarm_R",
        "spine_01","spine_02","spine_03","spine_04","neck_01","neck_02",
        "pulse_root","breath_L","breath_R","jaw","eye_L","eye_R"
    };

    void Start()
    {
        eng = soma_create();
        st = new float[Mathf.Max(36, soma_full_state_size())];
        if (!characterRoot) { Debug.LogError("SOMA: asigna characterRoot"); enabled = false; return; }

        foreach (var t in characterRoot.GetComponentsInChildren<Transform>(true))
            if (!bones.ContainsKey(t.name)) bones[t.name] = t;
        foreach (var n in kDriven)
        {
            if (!bones.TryGetValue(n, out var b)) continue;
            rest[n] = b.localRotation;
            restScale[n] = b.localScale;
            lateralAxis[n] = Quaternion.Inverse(b.rotation) * characterRoot.right;
        }
    }

    Quaternion Bend(string bone, float rad)
    {
        return rest[bone] * Quaternion.AngleAxis(rad * Mathf.Rad2Deg, lateralAxis[bone]);
    }
    void SetBend(string bone, float rad)
    {
        if (bones.TryGetValue(bone, out var b) && rest.ContainsKey(bone))
            b.localRotation = Bend(bone, rad);
    }
    void SetScale(string bone, float k)
    {
        if (bones.TryGetValue(bone, out var b) && restScale.ContainsKey(bone))
            b.localScale = restScale[bone] * k;
    }

    void Update()
    {
        if (eng == IntPtr.Zero) return;
        bool walk = (autoWalk || Input.GetKey(KeyCode.W)) && !Input.GetKey(KeyCode.Space);
        float effort = Input.GetKey(KeyCode.LeftShift) ? 1.5f : 1.0f;
        float steer = (Input.GetKey(KeyCode.A) ? 1f : 0f) - (Input.GetKey(KeyCode.D) ? 1f : 0f);
        soma_set_intention(eng, walk ? 1 : 0, effort, steer);
        soma_step(eng, Mathf.Clamp(Time.deltaTime, 0.001f, 0.05f));
        soma_get_full_state(eng, st);

        float hipL = st[0], kneeL = st[1], hipR = st[2], kneeR = st[3];
        float X = st[4], Y = st[5], heading = st[6], bob = st[7];

        // raíz: el motor camina en su plano; Unity es Y-up
        characterRoot.position = walkInPlace ? new Vector3(0, bob, 0) : new Vector3(X, bob, Y);
        characterRoot.rotation = Quaternion.Euler(0, -headingSign * heading * Mathf.Rad2Deg + faceYaw, 0);

        // piernas (FK del motor) — amplificado para una zancada natural
        SetBend("thigh_L", hipSign * hipL * legGain);
        SetBend("thigh_R", hipSign * hipR * legGain);
        SetBend("calf_L", kneeSign * Mathf.Max(0, -kneeL) * kneeGain);
        SetBend("calf_R", kneeSign * Mathf.Max(0, -kneeR) * kneeGain);

        // brazos: balanceo CONTRALATERAL (brazo opuesto a la pierna) desde la cadera
        SetBend("upperarm_L", hipSign * hipR * armGain);
        SetBend("upperarm_R", hipSign * hipL * armGain);
        SetBend("lowerarm_L", -elbowGain * (0.4f + Mathf.Max(0, hipSign * hipR)));
        SetBend("lowerarm_R", -elbowGain * (0.4f + Mathf.Max(0, hipSign * hipL)));

        // tronco: contrarrotación ligera repartida por la cadena (regla de Blender:
        // cada spine_xx toma 1/4 del guía). Aquí el guía es la contrarrotación.
        float counter = -0.15f * (hipL - hipR) * 0.5f;
        for (int i = 1; i <= 4; i++) SetBend($"spine_0{i}", counter * 0.25f * i);
        SetBend("neck_01", -counter * 0.5f);   // cabeza estabilizada
        SetBend("neck_02", -counter * 0.5f);

        // escalares del motor -> deformación de props
        SetScale("pulse_root", 1f + 0.08f * st[21]);
        float br = 1f + 0.05f * st[23];
        SetScale("breath_L", br);
        SetScale("breath_R", br);

        if (Camera.main)
        {
            Vector3 back = characterRoot.rotation * new Vector3(0, 0, -3.2f) + Vector3.up * 1.7f;
            Camera.main.transform.position = Vector3.Lerp(Camera.main.transform.position,
                characterRoot.position + back, 0.1f);
            Camera.main.transform.LookAt(characterRoot.position + Vector3.up * 1.1f);
        }
    }

    void OnGUI()
    {
        if (st == null) return;
        bool walk = st[12] > 0.5f;
        GUI.Label(new Rect(14, 10, 900, 22),
            $"[{(walk ? "CAMINA" : "quieto")}] vel={st[8]:0.00} m/s  pulso={st[21]:0.00}  " +
            $"fuelle={st[23]:0.00}@{st[24]:0}/min  contacto L{(st[25] > .5f ? "■" : "□")} R{(st[26] > .5f ? "■" : "□")}  " +
            $"demanda={st[30]:0.00}  —  W caminar · A/D girar · SHIFT rápido");
    }

    void OnDestroy() { if (eng != IntPtr.Zero) { soma_destroy(eng); eng = IntPtr.Zero; } }
}
