// SOMA — puente Unity ↔ motor C++ (soma.dll).
// Unity NO simula: manda tu intención (teclado), avanza el motor y dibuja la figura
// con los ángulos que devuelve el C++. El cerebro/física es soma.dll.
//
// USO:
//   1) Copia soma.dll a  Assets/Plugins/soma.dll
//   2) Copia este archivo a  Assets/Scripts/SomaEngine.cs
//   3) Crea un GameObject vacío, añádele este script. Pulsa Play.
//   4) Mantén W = caminar · A/D = girar · SHIFT = rápido · ESPACIO = parar
//
// Si algo se ve al revés (gira o flexiona hacia el lado contrario), invierte los
// signos públicos en el Inspector (hipSign, kneeSign, headingSign).
using System;
using System.Runtime.InteropServices;
using UnityEngine;

public class SomaEngine : MonoBehaviour
{
    // --- API de soma.dll ---
    [DllImport("soma")] static extern IntPtr soma_create();
    [DllImport("soma")] static extern void soma_destroy(IntPtr p);
    [DllImport("soma")] static extern void soma_set_intention(IntPtr p, int walk, float effort, float steer);
    [DllImport("soma")] static extern void soma_step(IntPtr p, float frame);
    [DllImport("soma")] static extern void soma_get_state(IntPtr p, [Out] float[] outState);

    // --- Ajustes (invierte si algo va al revés) ---
    public float hipSign = 1f, kneeSign = 1f, headingSign = 1f;
    public float thighLen = 0.4f, shankLen = 0.4f;

    IntPtr eng;
    float[] st = new float[12];
    Transform root, hip, torso, head;
    Transform[] thigh = new Transform[2], shank = new Transform[2], foot = new Transform[2];

    Transform MakeCapsule(string name, Transform parent, float len, float rad, Color c)
    {
        var pivot = new GameObject(name).transform;      // pivote en la articulación
        pivot.SetParent(parent, false);
        var go = GameObject.CreatePrimitive(PrimitiveType.Capsule);
        go.name = name + "_mesh";
        DestroyImmediate(go.GetComponent<Collider>());
        go.transform.SetParent(pivot, false);
        go.transform.localScale = new Vector3(rad * 2, len * 0.5f, rad * 2); // capsule alto = 2
        go.transform.localPosition = new Vector3(0, -len * 0.5f, 0);          // cuelga desde el pivote
        var mr = go.GetComponent<MeshRenderer>();
        mr.material = new Material(Shader.Find("Standard")) { color = c };
        return pivot;
    }
    Transform MakeBox(string name, Transform parent, Vector3 size, Color c)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
        go.name = name; DestroyImmediate(go.GetComponent<Collider>());
        go.transform.SetParent(parent, false); go.transform.localScale = size;
        go.GetComponent<MeshRenderer>().material = new Material(Shader.Find("Standard")) { color = c };
        return go.transform;
    }

    void Start()
    {
        eng = soma_create();

        // Suelo.
        var floor = GameObject.CreatePrimitive(PrimitiveType.Plane);
        floor.transform.localScale = new Vector3(20, 1, 20);
        floor.GetComponent<MeshRenderer>().material = new Material(Shader.Find("Standard")) { color = new Color(0.12f, 0.13f, 0.16f) };

        // Jerarquía de la figura.
        root = new GameObject("SomaBody").transform;
        hip = new GameObject("hip").transform; hip.SetParent(root, false);
        torso = MakeBox("torso", hip, new Vector3(0.26f, 0.36f, 0.20f), new Color(0.93f, 0.90f, 0.84f));
        torso.localPosition = new Vector3(0, 0.20f, 0);
        head = MakeBox("head", hip, new Vector3(0.20f, 0.22f, 0.22f), new Color(0.95f, 0.92f, 0.85f));
        head.localPosition = new Vector3(0, 0.52f, 0);
        var legCol = new Color(0.85f, 0.55f, 0.48f);
        for (int s = 0; s < 2; s++)
        {
            thigh[s] = MakeCapsule("thigh" + s, hip, thighLen, 0.06f, legCol);
            thigh[s].localPosition = new Vector3((s == 0 ? -0.09f : 0.09f), 0, 0);
            shank[s] = MakeCapsule("shank" + s, thigh[s], shankLen, 0.05f, legCol);
            shank[s].localPosition = new Vector3(0, -thighLen, 0);
            foot[s] = MakeBox("foot" + s, shank[s], new Vector3(0.10f, 0.05f, 0.22f), new Color(0.6f, 0.55f, 0.47f));
            foot[s].localPosition = new Vector3(0, -shankLen, 0.05f);
        }

        var key = new GameObject("keylight");
        var l = key.AddComponent<Light>(); l.type = LightType.Directional; l.intensity = 1.1f;
        key.transform.rotation = Quaternion.Euler(50, -30, 0);
    }

    void Update()
    {
        if (eng == IntPtr.Zero) return;

        // --- TU intención desde el teclado ---
        bool walk = Input.GetKey(KeyCode.W) && !Input.GetKey(KeyCode.Space);
        float effort = Input.GetKey(KeyCode.LeftShift) ? 1.5f : 1.0f;
        float steer = (Input.GetKey(KeyCode.A) ? 1f : 0f) - (Input.GetKey(KeyCode.D) ? 1f : 0f);
        soma_set_intention(eng, walk ? 1 : 0, effort, steer);

        // --- El motor C++ ejecuta ---
        soma_step(eng, Mathf.Clamp(Time.deltaTime, 0.001f, 0.05f));
        soma_get_state(eng, st);
        float hipL = st[0], kneeL = st[1], hipR = st[2], kneeR = st[3];
        float X = st[4], Y = st[5], heading = st[6], bob = st[7], hipH = st[10];

        // --- Coloca la figura (Unity Y-up; suelo horizontal, avanza en el plano) ---
        root.position = new Vector3(X, hipH + bob, Y);
        root.rotation = Quaternion.Euler(0, -headingSign * heading * Mathf.Rad2Deg, 0);
        // Piernas: flexión sobre el eje lateral local (X). Rodilla: solo flexión.
        thigh[0].localRotation = Quaternion.Euler(hipSign * hipL * Mathf.Rad2Deg, 0, 0);
        thigh[1].localRotation = Quaternion.Euler(hipSign * hipR * Mathf.Rad2Deg, 0, 0);
        shank[0].localRotation = Quaternion.Euler(kneeSign * Mathf.Max(0, -kneeL) * Mathf.Rad2Deg, 0, 0);
        shank[1].localRotation = Quaternion.Euler(kneeSign * Mathf.Max(0, -kneeR) * Mathf.Rad2Deg, 0, 0);

        // --- Cámara sigue ---
        if (Camera.main)
        {
            Vector3 back = root.rotation * new Vector3(0, 0, -3.2f) + Vector3.up * 1.7f;
            Camera.main.transform.position = Vector3.Lerp(Camera.main.transform.position, root.position + back, 0.1f);
            Camera.main.transform.LookAt(root.position + Vector3.up * 0.7f);
        }
    }

    void OnGUI()
    {
        bool walk = st[11] > 0.5f;
        GUI.Label(new Rect(14, 10, 700, 22),
            $"[{(walk ? "CAMINA" : "quieto")}]  vel={st[8]:0.00} m/s  pos=({st[4]:0.0},{st[5]:0.0})  rumbo={st[6] * Mathf.Rad2Deg:0}°   —   W caminar · A/D girar · SHIFT rápido · ESPACIO parar");
    }

    void OnDestroy() { if (eng != IntPtr.Zero) { soma_destroy(eng); eng = IntPtr.Zero; } }
}
