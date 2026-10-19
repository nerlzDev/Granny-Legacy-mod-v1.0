using System;
using System.Collections.Generic;
using System.Reflection;
using HarmonyLib;
using UnityEngine;

namespace GrannyLegacyMod.Patches;

/// <summary>
/// Records which PlayerPrefs keys the game reads and whether it reads them as float or int.
/// Writing a key with the wrong type makes the game read 0, so the shop must know both.
/// Keys read while a shop script runs are marked as shop keys.
/// </summary>
internal static class PrefsSpy
{
    public sealed class Entry
    {
        public string Key;
        public bool IsFloat;
        public float Value;
        public int Reads;
        public bool InShop;   // read while a shop method that shows/spends coins was running
    }

    public static readonly Dictionary<string, Entry> Seen = new();
    public static int ShopDepth;

    // Set while the mod itself reads PlayerPrefs, so only the game's reads are recorded.
    public static int Suppress;

    public static void Record(string key, bool isFloat, float value)
    {
        if (Suppress > 0 || string.IsNullOrEmpty(key))
            return;
        if (!Seen.TryGetValue(key, out var e))
            Seen[key] = e = new Entry { Key = key };
        e.IsFloat = isFloat;
        e.Value = value;
        e.Reads++;
        if (ShopDepth > 0 && !e.InShop)
        {
            e.InShop = true;
            Plugin.Log.LogInfo($"Shop reads PlayerPrefs \"{key}\" as {(isFloat ? "float" : "int")} = {value}");
        }
    }

    /// <summary>Runs PlayerPrefs code of the mod without it counting as a game read.</summary>
    public static T Quiet<T>(Func<T> read)
    {
        Suppress++;
        try
        {
            return read();
        }
        finally
        {
            Suppress--;
        }
    }
}

[HarmonyPatch]
internal static class PrefsGetFloatPatch
{
    private static IEnumerable<MethodBase> TargetMethods()
    {
        yield return AccessTools.Method(typeof(PlayerPrefs), nameof(PlayerPrefs.GetFloat), new[] { typeof(string), typeof(float) });
        yield return AccessTools.Method(typeof(PlayerPrefs), nameof(PlayerPrefs.GetFloat), new[] { typeof(string) });
    }

    private static void Postfix(string key, float __result) => PrefsSpy.Record(key, true, __result);
}

[HarmonyPatch]
internal static class PrefsGetIntPatch
{
    private static IEnumerable<MethodBase> TargetMethods()
    {
        yield return AccessTools.Method(typeof(PlayerPrefs), nameof(PlayerPrefs.GetInt), new[] { typeof(string), typeof(int) });
        yield return AccessTools.Method(typeof(PlayerPrefs), nameof(PlayerPrefs.GetInt), new[] { typeof(string) });
    }

    private static void Postfix(string key, int __result) => PrefsSpy.Record(key, false, __result);
}

/// <summary>
/// Marks the PlayerPrefs reads made while the game shows or spends coins. Start methods are left
/// out on purpose: they also read settings such as the volume.
/// </summary>
[HarmonyPatch]
internal static class ShopContextPatch
{
    private static IEnumerable<MethodBase> TargetMethods()
    {
        yield return AccessTools.Method(typeof(MainMenu), nameof(MainMenu.Update));
        foreach (var m in typeof(MainMenu).GetMethods())
            if (m.Name.StartsWith("AttemptBuy_") && m.GetParameters().Length == 0)
                yield return m;
        yield return AccessTools.Method(typeof(ColorToyCapsule), nameof(ColorToyCapsule.AttemptInsertCoin));
        yield return AccessTools.Method(typeof(ColorToyCapsule), nameof(ColorToyCapsule.UpdateRarityUI));
    }

    private static void Prefix() => PrefsSpy.ShopDepth++;

    private static void Postfix() => PrefsSpy.ShopDepth = Math.Max(0, PrefsSpy.ShopDepth - 1);
}
