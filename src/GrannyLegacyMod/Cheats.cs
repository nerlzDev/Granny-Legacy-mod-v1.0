using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;
using UnityEngine.Rendering;
using Object = UnityEngine.Object;

namespace GrannyLegacyMod;

/// <summary>
/// Runtime cheat state and the per-frame logic that applies it. Driven by <see cref="ModMenu"/>.
/// </summary>
internal static class Cheats
{
    public static bool GodMode;
    public static bool Invisible;
    public static bool FreezeEnemies;
    public static bool Noclip;
    public static bool Fullbright;

    public static bool TimeScaleEnabled;
    public static bool FovEnabled = true;

    public static float SpeedMultiplier = 1f;
    public static float NoclipSpeed = 8f;
    public static float TimeScale = 1f;
    public static float Fov = 90f;

    /// <summary>An enemy for ESP / freezing. The head bone (if any) gives the ESP box height.</summary>
    public sealed class Enemy
    {
        public Behaviour Ai;
        public NavMeshAgent Agent;
        public string Label;
        public int Kind;
        public Transform Head;
    }

    // Head bone per enemy instance, so the bone search runs once per enemy, not every refresh.
    private static readonly Dictionary<IntPtr, Transform> HeadCache = new();

    // ESP colour groups, must match EspKind in native/GrannyOverlay/src/shared.h.
    public const int KindGranny = 0, KindGrandpa = 1, KindSlendrina = 2, KindSpider = 3, KindRobot = 4, KindOther = 5;

    public static readonly List<Enemy> EnemyList = new();

    public static bool HasSavedPosition;
    private static Vector3 _savedPosition;

    // Cached scene objects, refreshed once a second.
    public static MobileFPS Fps;
    public static Days Days;
    public static Inventory Inventory;
    public static EnemyController Enemies;
    public static readonly List<string> Items = new();
    private static float _nextRefresh;

    /// <summary>The camera the player actually sees through (not always tagged MainCamera).</summary>
    public static Camera PlayerCamera { get; private set; }

    /// <summary>One-line status for the About tab and the log.</summary>
    public static string Diagnostics { get; private set; } = "";
    private static string _loggedDiagnostics;

    private static readonly List<(Behaviour ai, NavMeshAgent agent)> Frozen = new();

    private static bool _wasNoclip;
    private static bool _wasTimeScale;

    private static Vector3 _lastPlayerPos;
    private static IntPtr _lastPlayerCc;

    private static bool _brightSaved;
    private static AmbientMode _ambientMode;
    private static Color _ambientLight;
    private static bool _fog;
    private static GameObject _lamp;

    public static void Tick()
    {
        if (Time.unscaledTime >= _nextRefresh)
        {
            _nextRefresh = Time.unscaledTime + 1f;
            Refresh();
        }

        Safe(ResolveCamera);
        Safe(ApplyNoclip);
        Safe(ApplyFullbright);
        Safe(ApplyTimeScale);
    }

    private static void Refresh()
    {
        Safe(FindPlayer);
        Safe(() => Days = Object.FindObjectOfType<Days>());
        Safe(() => Inventory = Object.FindObjectOfType<Inventory>());
        Safe(() => Enemies = Object.FindObjectOfType<EnemyController>());
        Safe(RefreshEnemies);
        Safe(RefreshEnemyEyes);
        Safe(ApplyFreeze);
        Safe(RefreshItems);
        Safe(Shop.Refresh);
        Safe(UpdateDiagnostics);
    }

    // Prefer the MobileFPS whose controller is live; cutscenes and waking up can leave others around.
    private static void FindPlayer()
    {
        MobileFPS first = null, live = null;
        foreach (var fps in Object.FindObjectsOfType<MobileFPS>())
        {
            first ??= fps;
            var cc = fps.characterController;
            if (cc != null && cc.enabled && cc.gameObject.activeInHierarchy)
            {
                live = fps;
                break;
            }
        }
        Fps = live ?? first;
    }

    // ------------------------------------------------------------------ camera

    /// <summary>
    /// The game swaps cameras (hiding in a bed or closet, waking up, cutscenes), so every frame pick
    /// the camera that is drawing the world last: enabled, on screen, renders the Default layer,
    /// highest depth. On a tie the player's own camera wins.
    /// </summary>
    private static void ResolveCamera()
    {
        Camera own = null;
        if (Fps != null)
        {
            own = Usable(Fps.playerCamera2);
            if (own == null && Fps.PS != null)
                own = Usable(Fps.PS.PlayerCam);
        }

        Camera best = null;
        var cams = Camera.allCameras;
        for (int i = 0; i < cams.Length; i++)
        {
            var c = cams[i];
            if (c == null || c.targetTexture != null || (c.cullingMask & 1) == 0 || EnemyEyes.Contains(c.Pointer))
                continue;
            var r = c.rect;
            if (r.width * r.height < 0.5f)  // minimaps, picture-in-picture
                continue;
            bool better = best == null || c.depth > best.depth ||
                          (c.depth == best.depth && own != null && c.Pointer == own.Pointer);
            if (better)
                best = c;
        }

        PlayerCamera = best ?? own ?? Usable(Camera.main);
    }

    private static Camera Usable(Camera cam) => cam != null && cam.isActiveAndEnabled ? cam : null;

    // Granny/Grandpa/spider vision cameras must never be mistaken for the player's view.
    private static readonly HashSet<IntPtr> EnemyEyes = new();

    private static void RefreshEnemyEyes()
    {
        EnemyEyes.Clear();
        foreach (var eyes in Object.FindObjectsOfType<Eyes_Granny>()) AddEyes(eyes.CameraView);
        foreach (var eyes in Object.FindObjectsOfType<Eyes_Grandpa>()) AddEyes(eyes.CameraView);
        foreach (var eyes in Object.FindObjectsOfType<Eyes_MomSpider>()) AddEyes(eyes.CameraView);
    }

    private static void AddEyes(Camera cam)
    {
        if (cam != null)
            EnemyEyes.Add(cam.Pointer);
    }

    /// <summary>Called right before each camera renders, so the FOV can't be overwritten afterwards.</summary>
    public static void ApplyFov(Camera cam)
    {
        if (FovEnabled && cam != null && PlayerCamera != null && cam.Pointer == PlayerCamera.Pointer)
            cam.fieldOfView = Fov;
    }

    // ------------------------------------------------------------------ enemies

    private static void RefreshEnemies()
    {
        EnemyList.Clear();
        if (HeadCache.Count > 256)
            HeadCache.Clear();
        foreach (var ai in Object.FindObjectsOfType<AI_Granny>()) AddEnemy(ai, ai.Agent, "Granny", KindGranny);
        foreach (var ai in Object.FindObjectsOfType<AI_Grandpa>()) AddEnemy(ai, ai.Agent, "Grandpa", KindGrandpa);
        foreach (var ai in Object.FindObjectsOfType<AI_Slendrina>()) AddEnemy(ai, ai.Agent, "Slendrina", KindSlendrina);
        foreach (var ai in Object.FindObjectsOfType<AI_MomSpider>()) AddEnemy(ai, ai.Agent, "Spider", KindSpider);
        foreach (var ai in Object.FindObjectsOfType<AI_RobotEnemy>()) AddEnemy(ai, ai.Agent, "Robot", KindRobot);
        foreach (var ai in Object.FindObjectsOfType<SpectralGranny>()) AddEnemy(ai, ai.Agent, "Spectral Granny", KindGranny);
        foreach (var ai in Object.FindObjectsOfType<SunnyGrannyEnemy>()) AddEnemy(ai, null, "Sunny Granny", KindGranny);
        foreach (var ai in Object.FindObjectsOfType<LittleFucker>()) AddEnemy(ai, ai.Agent, "Creature", KindOther);
        foreach (var ai in Object.FindObjectsOfType<LeprechaunAI>()) AddEnemy(ai, ai.agent, "Leprechaun", KindOther);
        foreach (var ai in Object.FindObjectsOfType<SnowManAI>()) AddEnemy(ai, ai.Agent, "Snowman", KindOther);
        foreach (var ai in Object.FindObjectsOfType<RatEnemy>()) AddEnemy(ai, ai.Agent, "Rat", KindOther);
        foreach (var ai in Object.FindObjectsOfType<UndeadBunny>()) AddEnemy(ai, ai.agent, "Undead Bunny", KindOther);
    }

    private static void AddEnemy(Behaviour ai, NavMeshAgent agent, string label, int kind)
    {
        if (ai == null)
            return;

        // Search again if the cached bone was destroyed (the pointer can be reused by a new enemy).
        if (!HeadCache.TryGetValue(ai.Pointer, out var head) || (!ReferenceEquals(head, null) && head == null))
        {
            head = null;
            Safe(() => head = FindHead(ai.transform));
            HeadCache[ai.Pointer] = head;
        }

        EnemyList.Add(new Enemy { Ai = ai, Agent = agent, Label = label, Kind = kind, Head = head });
    }

    // Rigs name it "Head", "mixamorig:Head", "Bip01 Head", ... - skip helper bones like "HeadTop_End".
    private static Transform FindHead(Transform root)
    {
        foreach (var t in root.GetComponentsInChildren<Transform>())
        {
            var name = t.name.ToLowerInvariant();
            if (name.Contains("head") && !name.Contains("end") && !name.Contains("top") && !name.Contains("nub"))
                return t;
        }
        return null;
    }

    /// <summary>
    /// World-space box around an enemy: a column from its feet to just above its head bone.
    /// Mesh bounds aren't used because the game's skinned meshes report oversized bounds.
    /// </summary>
    public static void GetEnemyBounds(Enemy e, out Vector3 min, out Vector3 max)
    {
        var origin = e.Ai.transform.position;
        float height = 1.8f;

        if (e.Head != null)
            height = e.Head.position.y - origin.y + 0.25f;
        else if (e.Agent != null && e.Agent.height > 0.3f)
            height = e.Agent.height;

        height = Math.Clamp(height, 0.4f, 3.5f);
        float radius = height * 0.22f;
        min = new Vector3(origin.x - radius, origin.y, origin.z - radius);
        max = new Vector3(origin.x + radius, origin.y + height, origin.z + radius);
    }

    private static void ApplyFreeze()
    {
        if (!FreezeEnemies)
        {
            foreach (var (ai, agent) in Frozen)
            {
                Safe(() =>
                {
                    if (ai != null) ai.enabled = true;
                    if (agent != null && agent.enabled && agent.isOnNavMesh) agent.isStopped = false;
                });
            }
            Frozen.Clear();
            return;
        }

        foreach (var e in EnemyList)
            Safe(() => Freeze(e.Ai, e.Agent));
    }

    // Only touch enemies that are currently running, so unfreezing never wakes one the game turned off.
    private static void Freeze(Behaviour ai, NavMeshAgent agent)
    {
        if (ai == null || !ai.enabled)
            return;

        ai.enabled = false;
        if (agent != null && agent.enabled && agent.isOnNavMesh)
            agent.isStopped = true;
        Frozen.Add((ai, agent));
    }

    public static void KnockOutGranny() => Safe(() => Enemies?.GrannyDeath(false));

    public static void KnockOutGrandpa() => Safe(() => Enemies?.GrandpaDeath(false));

    public static void KnockOutAll()
    {
        KnockOutGranny();
        KnockOutGrandpa();
    }

    // ------------------------------------------------------------------ player

    public static Vector3? PlayerPosition()
    {
        var cc = Fps != null ? Fps.characterController : null;
        return cc != null ? cc.transform.position : null;
    }

    /// <summary>
    /// Speed multiplier. The game resets its own speed fields (crouching, item weight), so instead
    /// of touching them we look at how far the game moved the player this frame and add the rest.
    /// Must run after the game's Update, i.e. from LateUpdate.
    /// </summary>
    public static void ApplySpeed()
    {
        var cc = Fps != null ? Fps.characterController : null;
        if (cc == null)
        {
            _lastPlayerCc = IntPtr.Zero;
            return;
        }

        var pos = cc.transform.position;
        bool sameController = _lastPlayerCc == cc.Pointer;
        _lastPlayerCc = cc.Pointer;

        if (sameController && !Noclip && Math.Abs(SpeedMultiplier - 1f) > 0.01f && PlayerWalking(cc))
        {
            float dx = pos.x - _lastPlayerPos.x;
            float dz = pos.z - _lastPlayerPos.z;
            float moved = dx * dx + dz * dz;

            // Ignore standing still and big jumps (teleports, respawns, cutscenes).
            if (moved > 1e-10f && moved < 1f)
            {
                float extra = SpeedMultiplier - 1f;

                // isGrounded still describes the game's own move this frame. When on the ground, push
                // down as well so the extra distance follows stairs and slopes instead of floating off.
                float down = cc.isGrounded ? -((float)Math.Sqrt(moved) * Math.Abs(extra) + 0.05f) : 0f;
                cc.Move(new Vector3(dx * extra, down, dz * extra));
                pos = cc.transform.position;
            }
        }

        _lastPlayerPos = pos;
    }

    // Only boost normal walking, not while the game moves you into or out of a hiding spot.
    // (Killed / IsJumpscared aren't checked: the game may not clear them after you wake up.)
    private static bool PlayerWalking(CharacterController cc)
    {
        if (!cc.enabled || !cc.gameObject.activeInHierarchy || !Fps.enabled)
            return false;

        var ps = Fps.PS;
        if (ps == null)
            return true;
        return !(ps.HidingUnderThings || ps.HidingInClosets || ps.HidingFromMom ||
                 ps.InBed1 || ps.InBed2 || ps.InBed3 || ps.InCar || ps.InCoffinBackyard || ps.InCoffinAttic);
    }

    private static void ApplyNoclip()
    {
        if (Fps == null || Fps.characterController == null)
            return;

        var cc = Fps.characterController;
        if (!Noclip)
        {
            if (_wasNoclip)
                cc.enabled = true;
            _wasNoclip = false;
            return;
        }

        _wasNoclip = true;
        cc.enabled = false;

        var view = Fps.playerCamera != null ? Fps.playerCamera : PlayerCamera?.transform;
        if (view == null)
            return;

        var dir = Vector3.zero;
        if (Input.GetKey(KeyCode.W)) dir += view.forward;
        if (Input.GetKey(KeyCode.S)) dir -= view.forward;
        if (Input.GetKey(KeyCode.D)) dir += view.right;
        if (Input.GetKey(KeyCode.A)) dir -= view.right;
        if (Input.GetKey(KeyCode.Space)) dir += Vector3.up;
        if (Input.GetKey(KeyCode.LeftControl)) dir -= Vector3.up;

        float speed = NoclipSpeed * (Input.GetKey(KeyCode.LeftShift) ? 3f : 1f);
        cc.transform.position += dir.normalized * (speed * Time.deltaTime);
    }

    public static void SavePosition() => Safe(() =>
    {
        var pos = PlayerPosition();
        if (pos == null)
            return;
        _savedPosition = pos.Value;
        HasSavedPosition = true;
    });

    public static void LoadPosition() => Safe(() =>
    {
        var cc = Fps != null ? Fps.characterController : null;
        if (cc == null || !HasSavedPosition)
            return;
        cc.enabled = false;
        cc.transform.position = _savedPosition;
        cc.enabled = !Noclip;
        _lastPlayerPos = _savedPosition;
    });

    // ------------------------------------------------------------------ world

    private static void ApplyTimeScale()
    {
        if (TimeScaleEnabled)
        {
            Time.timeScale = TimeScale;
            _wasTimeScale = true;
        }
        else if (_wasTimeScale)
        {
            Time.timeScale = 1f;
            _wasTimeScale = false;
        }
    }

    public static void ChangeDay(int delta) => Safe(() =>
    {
        if (Days != null)
            Days.CurrentDay = Math.Max(1f, Days.CurrentDay + delta);
    });

    private static void ApplyFullbright()
    {
        if (Fullbright)
        {
            if (!_brightSaved)
            {
                _ambientMode = RenderSettings.ambientMode;
                _ambientLight = RenderSettings.ambientLight;
                _fog = RenderSettings.fog;
                _brightSaved = true;
            }

            RenderSettings.ambientMode = AmbientMode.Flat;
            RenderSettings.ambientLight = Color.white;
            RenderSettings.fog = false;

            var cam = PlayerCamera;
            if (_lamp == null && cam != null)
            {
                _lamp = new GameObject("GrannyLegacyMod_Lamp");
                var light = _lamp.AddComponent<Light>();
                light.type = LightType.Point;
                light.range = 40f;
                light.intensity = 1.5f;
                light.shadows = LightShadows.None;
            }
            if (_lamp != null)
            {
                // Follow the camera across camera swaps (beds, closets, waking up).
                if (cam != null && (_lamp.transform.parent == null || _lamp.transform.parent.Pointer != cam.transform.Pointer))
                {
                    _lamp.transform.SetParent(cam.transform, false);
                    _lamp.transform.localPosition = Vector3.zero;
                }
                _lamp.SetActive(true);
            }
        }
        else if (_brightSaved)
        {
            RenderSettings.ambientMode = _ambientMode;
            RenderSettings.ambientLight = _ambientLight;
            RenderSettings.fog = _fog;
            _brightSaved = false;
            if (_lamp != null)
                _lamp.SetActive(false);
        }
    }

    public static void GiveItem(string name) => Safe(() => Inventory?.PickupItem(name));

    private static void RefreshItems()
    {
        Items.Clear();
        var defs = Inventory?.ItemDefs;
        if (defs == null)
            return;
        for (int i = 0; i < defs.Count; i++)
        {
            var name = defs[i]?.itemName;
            if (!string.IsNullOrEmpty(name) && !Items.Contains(name))
                Items.Add(name);
        }
    }

    // ------------------------------------------------------------------ diagnostics

    private static void UpdateDiagnostics()
    {
        var cam = PlayerCamera;
        string camText = cam == null
            ? "none"
            : $"{cam.gameObject.name}{(cam.CompareTag("MainCamera") ? "" : " (not MainCamera)")}";

        string enemies = EnemyList.Count == 0 ? "none" : string.Join(", ", EnemyList.ConvertAll(e => e.Label));
        string status = $"Player: {(Fps != null ? "found" : "not found")} | Camera: {camText} | " +
                        $"Enemies: {enemies} | Items: {Items.Count}";

        if (status != _loggedDiagnostics)
        {
            _loggedDiagnostics = status;
            Plugin.Log.LogInfo(status);
        }

        Diagnostics = cam != null ? $"{status} | FOV now: {cam.fieldOfView:0}" : status;
    }

    internal static void Safe(Action action)
    {
        try
        {
            action();
        }
        catch (Exception e)
        {
            Plugin.Log.LogWarning(e);
        }
    }
}
