using System;
using Il2CppInterop.Runtime;
using UnityEngine;

namespace GrannyLegacyMod;

/// <summary>
/// Drives the cheats every frame and hosts the menu: the Dear ImGui overlay when it is loaded
/// (see <see cref="Overlay"/>), otherwise a basic Unity IMGUI menu. Press Delete to open/close.
/// </summary>
public class ModMenu : MonoBehaviour
{
    private const float MinFov = 30f;
    private const float MaxFov = 150f;
    private const float Width = 340f;

    private bool _open;
    private Vector2 _scroll;
    private int _sentItemCount = -1;
    private string _sentDiagnostics;
    private string _sentCoinKey;

    public ModMenu(IntPtr ptr) : base(ptr) { }

    private Camera.CameraCallback _preCull;

    public void Start()
    {
        Cheats.FovEnabled = Settings.FovEnabled.Value;
        Cheats.Fov = Settings.Fov.Value;
        Cheats.TimeScale = Settings.TimeScale.Value;

        // Set the FOV right before the player camera renders, after every game script has run.
        Cheats.Safe(() =>
        {
            _preCull = DelegateSupport.ConvertDelegate<Camera.CameraCallback>(new Action<Camera>(OnCameraPreCull));
            Camera.onPreCull = Il2CppSystem.Delegate.Combine(Camera.onPreCull, _preCull).Cast<Camera.CameraCallback>();
        });
    }

    private static void OnCameraPreCull(Camera cam)
    {
        try
        {
            Cheats.ApplyFov(cam);
        }
        catch (Exception e)
        {
            Plugin.Log.LogWarning(e);
        }
    }

    public void Update()
    {
        bool menuOpen;
        if (Overlay.Active)
        {
            ReadOverlay();
            menuOpen = OverlayMenuOpen();
        }
        else
        {
            if (Input.GetKeyDown(KeyCode.Delete))
            {
                _open = !_open;
                if (!_open)
                    SaveFallbackSettings();
            }
            menuOpen = _open;
        }

        if (menuOpen)
        {
            // The game locks the cursor every frame, so keep freeing it while the menu is up.
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }

        Cheats.Tick();

        if (Overlay.Active)
            WriteOverlayInfo();
    }

    // LateUpdate runs after the game's own Update, so movement and camera are final for this frame.
    public void LateUpdate()
    {
        Cheats.Safe(Cheats.ApplySpeed);
        Cheats.Safe(Orbit.Apply);

        var cam = Cheats.PlayerCamera;
        if (cam == null)
        {
            if (Overlay.Active)
                ClearEsp();
            return;
        }

        Cheats.ApplyFov(cam);

        if (Overlay.Active)
            Cheats.Safe(() => WriteEsp(cam));
    }

    // ------------------------------------------------------------------ ImGui overlay

    private static unsafe bool OverlayMenuOpen() => Overlay.State->MenuOpen != 0 || Overlay.State->RadialOpen != 0;

    private static unsafe void ReadOverlay()
    {
        var s = Overlay.State;
        Cheats.GodMode = s->GodMode != 0;
        Cheats.Invisible = s->Invisible != 0;
        Cheats.FreezeEnemies = s->FreezeEnemies != 0;
        Cheats.Noclip = s->Noclip != 0;
        Cheats.Fullbright = s->Fullbright != 0;
        Cheats.TimeScaleEnabled = s->TimeScaleEnabled != 0;
        Cheats.FovEnabled = s->FovEnabled != 0;
        Cheats.Fov = s->Fov;
        Cheats.SpeedMultiplier = s->SpeedMult;
        Cheats.NoclipSpeed = s->NoclipSpeed;
        Cheats.TimeScale = s->TimeScale;
        Orbit.Enabled = s->OrbitEnabled != 0;
        Orbit.Radius = s->OrbitRadius;
        Orbit.Speed = s->OrbitSpeed;

        if (Take(ref s->ActKnockGranny)) Cheats.KnockOutGranny();
        if (Take(ref s->ActKnockGrandpa)) Cheats.KnockOutGrandpa();
        if (Take(ref s->ActKnockAll)) Cheats.KnockOutAll();
        if (Take(ref s->ActSavePos)) Cheats.SavePosition();
        if (Take(ref s->ActLoadPos)) Cheats.LoadPosition();
        if (Take(ref s->ActDayMinus)) Cheats.ChangeDay(-1);
        if (Take(ref s->ActDayPlus)) Cheats.ChangeDay(1);

        int item = s->ActGiveItem;
        if (item != 0)
        {
            s->ActGiveItem = 0;
            if (item - 1 < Cheats.Items.Count)
                Cheats.GiveItem(Cheats.Items[item - 1]);
        }

        if (Take(ref s->ActUnlockSkins)) Shop.UnlockAllColours();
        if (Take(ref s->ActToggleExplorer)) Explorer.Toggle();

        int coins = s->ActAddCoins;
        if (coins != 0)
        {
            s->ActAddCoins = 0;
            Shop.AddCoins(coins);
        }
        if (Take(ref s->ActUnlockAll)) Shop.UnlockEverything();

        int radial = s->RadialAction;
        if (radial != 0)
        {
            s->RadialAction = 0;
            EnemyWheel.Run(radial, s->RadialTarget);
        }

        if (Take(ref s->ActSaveConfig))
        {
            Settings.ReadFrom(s);
            Settings.Save();
        }
    }

    private static bool Take(ref int action)
    {
        if (action == 0)
            return false;
        action = 0;
        return true;
    }

    private unsafe void WriteOverlayInfo()
    {
        var s = Overlay.State;
        s->InGame = Cheats.Fps != null ? 1 : 0;
        s->HasSavedPos = Cheats.HasSavedPosition ? 1 : 0;
        s->Coins = Shop.Found ? 1 : -1;
        s->CoinValue = Shop.Coins;
        s->CoinKeyType = !Shop.Found ? 0 : Shop.CoinIsFloat ? 2 : 1;
        s->CoinKeyConfirmed = Shop.Confirmed ? 1 : 0;
        if (_sentCoinKey != Shop.CoinKey)
        {
            _sentCoinKey = Shop.CoinKey;
            Overlay.SetCoinKey(_sentCoinKey ?? "");
        }
        s->InMainMenu = Shop.Menu != null ? 1 : 0;
        s->InCapsuleShop = Shop.Capsule != null ? 1 : 0;
        s->ExplorerStatus = Explorer.Status;
        s->CurrentDay = Cheats.Days != null ? Cheats.Days.CurrentDay : 0f;

        var pos = Cheats.PlayerPosition();
        if (pos != null)
        {
            s->PosX = pos.Value.x;
            s->PosY = pos.Value.y;
            s->PosZ = pos.Value.z;
        }

        if (_sentItemCount != Cheats.Items.Count)
        {
            _sentItemCount = Cheats.Items.Count;
            Overlay.SetItems(Cheats.Items);
        }

        if (ConsoleLog.TakeChanged(out var lines))
            Overlay.SetConsole(lines);

        if (_sentDiagnostics != Cheats.Diagnostics)
        {
            _sentDiagnostics = Cheats.Diagnostics;
            Overlay.SetDebug(_sentDiagnostics);
        }
    }

    private static unsafe void ClearEsp() => Overlay.State->EspCount = 0;

    private static unsafe void WriteEsp(Camera cam)
    {
        // Always fill the list: the enemy wheel needs it even with ESP off (the overlay only
        // draws boxes when ESP is on).
        var s = Overlay.State;
        float screenH = Screen.height;
        s->ScreenW = Screen.width;
        s->ScreenH = screenH;
        var camPos = cam.transform.position;
        int count = 0;

        foreach (var e in Cheats.EnemyList)
        {
            if (count >= SharedState.MaxEsp)
                break;
            if (e.Ai == null || !e.Ai.gameObject.activeInHierarchy)
                continue;

            Cheats.GetEnemyBounds(e, out var min, out var max);

            // Project the 8 corners of the box.
            float x0 = float.MaxValue, y0 = float.MaxValue, x1 = float.MinValue, y1 = float.MinValue;
            bool allInFront = true;
            var corners = new float[16];
            for (int c = 0; c < 8; c++)
            {
                var corner = new Vector3((c & 1) != 0 ? max.x : min.x, (c & 2) != 0 ? max.y : min.y, (c & 4) != 0 ? max.z : min.z);
                var p = cam.WorldToScreenPoint(corner);
                if (p.z <= 0.05f)
                {
                    allInFront = false;
                    break;
                }
                float y = screenH - p.y;
                corners[c * 2] = p.x;
                corners[c * 2 + 1] = y;
                x0 = Math.Min(x0, p.x);
                x1 = Math.Max(x1, p.x);
                y0 = Math.Min(y0, y);
                y1 = Math.Max(y1, y);
            }

            // Very close enemies can have a corner behind the camera: fall back to the
            // centre line from feet to head so the box doesn't vanish next to you.
            bool onScreen = allInFront;
            if (!allInFront)
            {
                float midX = (min.x + max.x) / 2f, midZ = (min.z + max.z) / 2f;
                var feet = cam.WorldToScreenPoint(new Vector3(midX, min.y, midZ));
                var head = cam.WorldToScreenPoint(new Vector3(midX, max.y, midZ));
                if (feet.z > 0.05f && head.z > 0.05f)
                {
                    float top = screenH - head.y, bottom = screenH - feet.y;
                    float halfW = Math.Abs(bottom - top) * 0.22f;
                    x0 = Math.Min(head.x, feet.x) - halfW;
                    x1 = Math.Max(head.x, feet.x) + halfW;
                    y0 = Math.Min(top, bottom);
                    y1 = Math.Max(top, bottom);
                    onScreen = true;
                }
            }

            Overlay.SetEspCorners(count, allInFront ? corners : null);
            var center = new Vector3((min.x + max.x) / 2f, (min.y + max.y) / 2f, (min.z + max.z) / 2f);
            s->EspId[count] = EnemyWheel.IdOf(e.Ai);
            Overlay.SetEsp(count++, x0, y0, x1, y1, Vector3.Distance(camPos, center), e.Kind, onScreen, e.Label);
        }

        s->EspCount = count;
    }

    // ------------------------------------------------------------------ fallback Unity menu

    private static void SaveFallbackSettings()
    {
        Settings.FovEnabled.Value = Cheats.FovEnabled;
        Settings.Fov.Value = Cheats.Fov;
        Settings.SpeedMult.Value = Cheats.SpeedMultiplier;
        Settings.NoclipSpeed.Value = Cheats.NoclipSpeed;
        Settings.Save();
    }

    public void OnGUI()
    {
        if (!_open || Overlay.Active)
            return;

        var area = new Rect(20f, 20f, Width, Math.Min(Screen.height - 40f, 720f));
        GUI.Box(area, "Granny Legacy Mod  (Delete to close)");
        GUILayout.BeginArea(new Rect(area.x + 10f, area.y + 25f, area.width - 20f, area.height - 35f));
        _scroll = GUILayout.BeginScrollView(_scroll);

        if (Overlay.FailReason != null)
            GUILayout.Label($"<color=#ff6060><b>Basic menu: {Overlay.FailReason}</b></color>");

        Header("Camera");
        Cheats.FovEnabled = GUILayout.Toggle(Cheats.FovEnabled, " Custom FOV");
        GUILayout.Label($"FOV: {Cheats.Fov:0}");
        Cheats.Fov = (float)Math.Round(GUILayout.HorizontalSlider(Cheats.Fov, MinFov, MaxFov));
        if (GUILayout.Button("Reset FOV (60)"))
            Cheats.Fov = 60f;

        Header("Player");
        Cheats.GodMode = GUILayout.Toggle(Cheats.GodMode, " God mode (can't die or be caught)");
        Cheats.Invisible = GUILayout.Toggle(Cheats.Invisible, " Invisible to Granny & Grandpa");
        GUILayout.Label($"Speed: x{Cheats.SpeedMultiplier:0.0}");
        Cheats.SpeedMultiplier = (float)Math.Round(GUILayout.HorizontalSlider(Cheats.SpeedMultiplier, 0.5f, 5f), 1);
        Cheats.Noclip = GUILayout.Toggle(Cheats.Noclip, " Noclip / fly (WASD, Space up, Ctrl down, Shift fast)");
        GUILayout.Label($"Fly speed: {Cheats.NoclipSpeed:0}");
        Cheats.NoclipSpeed = (float)Math.Round(GUILayout.HorizontalSlider(Cheats.NoclipSpeed, 2f, 30f));
        GUILayout.BeginHorizontal();
        if (GUILayout.Button("Save position"))
            Cheats.SavePosition();
        if (GUILayout.Button("Teleport back"))
            Cheats.LoadPosition();
        GUILayout.EndHorizontal();

        Header("Enemies");
        Cheats.FreezeEnemies = GUILayout.Toggle(Cheats.FreezeEnemies, " Freeze all enemies");
        GUILayout.BeginHorizontal();
        if (GUILayout.Button("Knock out Granny"))
            Cheats.KnockOutGranny();
        if (GUILayout.Button("Knock out Grandpa"))
            Cheats.KnockOutGrandpa();
        GUILayout.EndHorizontal();

        Header("World");
        Cheats.Fullbright = GUILayout.Toggle(Cheats.Fullbright, " Fullbright (no darkness / fog)");
        if (Cheats.Days != null)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label($"Day: {Cheats.Days.CurrentDay:0}");
            if (GUILayout.Button("-", GUILayout.Width(40f)))
                Cheats.ChangeDay(-1);
            if (GUILayout.Button("+", GUILayout.Width(40f)))
                Cheats.ChangeDay(1);
            GUILayout.EndHorizontal();
        }

        Header("Items (click to take)");
        if (Cheats.Items.Count == 0)
            GUILayout.Label("Start a game to see items.");
        foreach (var item in Cheats.Items.ToArray())
        {
            if (GUILayout.Button(item))
                Cheats.GiveItem(item);
        }

        GUILayout.EndScrollView();
        GUILayout.EndArea();
    }

    private static void Header(string text)
    {
        GUILayout.Space(8f);
        GUILayout.Label($"<b>{text}</b>");
    }
}
