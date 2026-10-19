using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace GrannyLegacyMod;

/// <summary>
/// Mirror of SharedState in native/GrannyOverlay/src/shared.h. Keep both in sync.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct SharedState
{
    public const int Version = 6;
    public const int MaxItems = 96;
    public const int ItemNameLen = 48;
    public const int MaxEsp = 16;
    public const int EspFields = 7;
    public const int EspLabelLen = 24;
    public const int DebugLen = 512;
    public const int ConsoleLines = 32;
    public const int ConsoleLineLen = 128;

    public int StateVersion, Hooked, MenuOpen;

    public int GodMode, Invisible, FreezeEnemies, Noclip, Fullbright;
    public int FovEnabled, TimeScaleEnabled;
    public int EspEnabled, EspBoxes, EspLines, EspNames, EspDistance;
    public int Watermark, Crosshair, Notifications;
    public int Theme;
    public float Fov, SpeedMult, NoclipSpeed, TimeScale;

    public int ActKnockGranny, ActKnockGrandpa, ActKnockAll;
    public int ActSavePos, ActLoadPos;
    public int ActDayMinus, ActDayPlus;
    public int ActGiveItem;
    public int ActSaveConfig;

    public int InGame, HasSavedPos;
    public float CurrentDay;
    public float PosX, PosY, PosZ;
    public int ItemCount;
    public fixed byte Items[MaxItems * ItemNameLen];
    public int EspCount;
    // Per entry: minX, minY, maxX, maxY, distance (float bits), kind, onScreen.
    public fixed int Esp[MaxEsp * EspFields];
    public fixed byte EspLabel[MaxEsp * EspLabelLen];
    public float ScreenW, ScreenH;
    public fixed byte Debug[DebugLen];

    // v3
    public fixed float EspCorners[MaxEsp * 16];
    public fixed int EspCornersValid[MaxEsp];
    public int EspCorner, EspFilled, Esp3D, EspHeadDot;
    public int MenuParticles;
    public int ConsoleCount;
    public fixed byte Console[ConsoleLines * ConsoleLineLen];

    // v4
    public fixed int EspId[MaxEsp];
    public int RadialOpen, RadialTarget, RadialAction;
    public int Coins;
    public int InMainMenu;
    public int ActAddCoins;
    public int ActUnlockAll;

    // v5
    public float CoinValue;
    public int CoinKeyType;
    public int CoinKeyConfirmed;
    public fixed byte CoinKey[48];
    public int InCapsuleShop;
    public int ActUnlockSkins;
    public int ExplorerStatus;
    public int ActToggleExplorer;

    // v6
    public int OrbitEnabled;
    public float OrbitRadius, OrbitSpeed;
}

/// <summary>
/// Loads GrannyOverlay.dll (the Dear ImGui DirectX 11 menu) from the plugin folder.
/// If it is missing or can't hook the game, the plugin falls back to the built-in Unity menu.
/// </summary>
internal static unsafe class Overlay
{
    public static SharedState* State { get; private set; }

    /// <summary>Why the ImGui menu isn't used (shown at the top of the basic menu), or null.</summary>
    public static string FailReason { get; private set; }

    /// <summary>True once the ImGui overlay is drawing in the game.</summary>
    public static bool Active => State != null && State->Hooked != 0;

    public static void Load()
    {
        try
        {
            var dir = Path.GetDirectoryName(typeof(Plugin).Assembly.Location)!;
            var path = Path.Combine(dir, "GrannyOverlay.dll");
            if (!File.Exists(path))
            {
                FailReason = "GrannyOverlay.dll is missing - put it next to GrannyLegacyMod.dll";
                Plugin.Log.LogWarning("GrannyOverlay.dll not found next to the plugin, using the basic menu.");
                return;
            }

            var lib = NativeLibrary.Load(path);
            var getVersion = (delegate* unmanaged<int>)NativeLibrary.GetExport(lib, "Overlay_GetVersion");
            var getSize = (delegate* unmanaged<int>)NativeLibrary.GetExport(lib, "Overlay_GetStateSize");
            var getState = (delegate* unmanaged<SharedState*>)NativeLibrary.GetExport(lib, "Overlay_GetState");
            var init = (delegate* unmanaged<int>)NativeLibrary.GetExport(lib, "Overlay_Init");

            if (getVersion() != SharedState.Version || getSize() != sizeof(SharedState))
            {
                FailReason = $"GrannyOverlay.dll is an old version (v{getVersion()}, needs v{SharedState.Version}) - replace it with the one from the same release";
                Plugin.Log.LogError("GrannyOverlay.dll doesn't match this plugin version, using the basic menu.");
                return;
            }

            State = getState();
            Settings.WriteTo(State);

            if (init() != 1)
            {
                FailReason = "ImGui couldn't hook DirectX 11";
                Plugin.Log.LogWarning("ImGui overlay couldn't hook DirectX 11, using the basic menu.");
                State = null;
                return;
            }

            Plugin.Log.LogInfo("ImGui overlay loaded.");
        }
        catch (Exception e)
        {
            Plugin.Log.LogError($"Failed to load the ImGui overlay, using the basic menu: {e}");
            State = null;
        }
    }

    public static void SetItems(System.Collections.Generic.List<string> items)
    {
        var s = State;
        int count = Math.Min(items.Count, SharedState.MaxItems);
        for (int i = 0; i < count; i++)
            WriteString(s->Items + i * SharedState.ItemNameLen, SharedState.ItemNameLen, items[i]);
        s->ItemCount = count;
    }

    public static void SetEsp(int index, float minX, float minY, float maxX, float maxY, float distance, int kind, bool onScreen, string label)
    {
        int* e = State->Esp + index * SharedState.EspFields;
        e[0] = BitConverter.SingleToInt32Bits(minX);
        e[1] = BitConverter.SingleToInt32Bits(minY);
        e[2] = BitConverter.SingleToInt32Bits(maxX);
        e[3] = BitConverter.SingleToInt32Bits(maxY);
        e[4] = BitConverter.SingleToInt32Bits(distance);
        e[5] = kind;
        e[6] = onScreen ? 1 : 0;
        WriteString(State->EspLabel + index * SharedState.EspLabelLen, SharedState.EspLabelLen, label);
    }

    public static void SetDebug(string text) => WriteString(State->Debug, SharedState.DebugLen, text);

    public static void SetCoinKey(string key) => WriteString(State->CoinKey, 48, key);

    /// <summary>Screen positions of the 8 box corners (x,y pairs), or null if any is behind the camera.</summary>
    public static void SetEspCorners(int index, float[] corners)
    {
        State->EspCornersValid[index] = corners != null ? 1 : 0;
        if (corners == null)
            return;
        float* dst = State->EspCorners + index * 16;
        for (int k = 0; k < 16; k++)
            dst[k] = corners[k];
    }

    public static void SetConsole(System.Collections.Generic.IReadOnlyList<string> lines)
    {
        int count = Math.Min(lines.Count, SharedState.ConsoleLines);
        int skip = lines.Count - count;
        for (int i = 0; i < count; i++)
            WriteString(State->Console + i * SharedState.ConsoleLineLen, SharedState.ConsoleLineLen, lines[skip + i]);
        State->ConsoleCount = count;
    }

    // Writes a NUL-terminated UTF-8 string, cut to fit the buffer.
    private static void WriteString(byte* dst, int capacity, string text)
    {
        var bytes = Encoding.UTF8.GetBytes(text ?? "");
        int len = Math.Min(bytes.Length, capacity - 1);
        for (int b = 0; b < len; b++)
            dst[b] = bytes[b];
        dst[len] = 0;
    }
}
