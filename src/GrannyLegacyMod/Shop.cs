using System;
using System.Linq;
using System.Text.RegularExpressions;
using GrannyLegacyMod.Patches;
using Microsoft.Win32;
using UnityEngine;
using Object = UnityEngine.Object;

namespace GrannyLegacyMod;

/// <summary>
/// Coins and unlocks. The coin key is learned from the game's own PlayerPrefs reads (see
/// <see cref="PrefsSpy"/>), which also tells us if coins are a float (the shop costs 0.5) or an int.
/// Until the game has been seen reading it, the registry copy of PlayerPrefs is used as a guess.
/// </summary>
[System.Runtime.Versioning.SupportedOSPlatform("windows")]
internal static class Shop
{
    private static readonly string[] CoinWords = { "coin", "money", "cash", "currency" };
    private static readonly string[] SettingWords = { "volume", "sens", "fov", "bright", "gamma", "quality", "music", "sound" };

    public static MainMenu Menu { get; private set; }
    public static ColorToyCapsule Capsule { get; private set; }

    public static string CoinKey { get; private set; }
    public static bool CoinIsFloat { get; private set; }
    public static bool Confirmed { get; private set; }
    public static bool Found => CoinKey != null;
    public static float Coins { get; private set; }

    private static float _nextRegistryLookup;
    private static bool _repaired;

    public static void Refresh()
    {
        // Backstop: Refresh runs outside game code, so no shop method can be "running" now.
        PrefsSpy.ShopDepth = 0;

        Cheats.Safe(() => Menu = Object.FindObjectOfType<MainMenu>());
        Cheats.Safe(() => Capsule = Object.FindObjectOfType<ColorToyCapsule>());

        Cheats.Safe(PickFromSpy);
        if (!Confirmed && Time.unscaledTime >= _nextRegistryLookup)
        {
            _nextRegistryLookup = Time.unscaledTime + 5f;
            Cheats.Safe(GuessFromRegistry);
        }

        if (Found)
        {
            Cheats.Safe(RepairType);
            Cheats.Safe(() => Coins = PrefsSpy.Quiet(() => CoinIsFloat ? PlayerPrefs.GetFloat(CoinKey, 0f) : PlayerPrefs.GetInt(CoinKey, 0)));
        }
    }

    private static bool LooksLikeCoins(string key) =>
        CoinWords.Any(w => key.IndexOf(w, StringComparison.OrdinalIgnoreCase) >= 0);

    private static bool LooksLikeSetting(string key) =>
        SettingWords.Any(w => key.IndexOf(w, StringComparison.OrdinalIgnoreCase) >= 0);

    // The game's own reads are the truth. Best: a float read while the shop shows or spends coins
    // (the name doesn't matter); coin-like names win ties. Without shop reads, a coin-named key.
    private static void PickFromSpy()
    {
        var best = PrefsSpy.Seen.Values
            .Where(e => e.InShop && e.IsFloat && !LooksLikeSetting(e.Key))
            .OrderByDescending(e => LooksLikeCoins(e.Key))
            .ThenByDescending(e => e.Reads)
            .FirstOrDefault()
            ?? PrefsSpy.Seen.Values
            .Where(e => LooksLikeCoins(e.Key))
            .OrderByDescending(e => e.InShop)
            .ThenByDescending(e => e.Reads)
            .FirstOrDefault();
        if (best == null)
            return;

        if (!Confirmed || CoinKey != best.Key || CoinIsFloat != best.IsFloat)
        {
            CoinKey = best.Key;
            CoinIsFloat = best.IsFloat;
            Confirmed = true;
            _repaired = false;
            Plugin.Log.LogInfo($"Coins: the game reads \"{CoinKey}\" as {(CoinIsFloat ? "float" : "int")}");
        }
    }

    private static void GuessFromRegistry()
    {
        var path = $@"Software\{Application.companyName}\{Application.productName}";
        using var key = Registry.CurrentUser.OpenSubKey(path);
        if (key == null)
            return;
        foreach (var raw in key.GetValueNames())
        {
            var name = Regex.Replace(raw, @"_h\d+$", "");
            if (!LooksLikeCoins(name))
                continue;
            // Strings are stored as binary; skip them. The registry type tag can't tell an int
            // from a float reliably, and the shop prices are fractions (0.5), so guess float.
            if (key.GetValue(raw) is byte[])
                continue;
            CoinKey = name;
            CoinIsFloat = true;
            Plugin.Log.LogInfo($"Coins (guess from registry): \"{name}\" as {(CoinIsFloat ? "float" : "int")}");
            return;
        }
    }

    // Older versions of this mod wrote coins as an int. If the game wants a float, convert it back.
    private static void RepairType()
    {
        if (_repaired || !Confirmed || !CoinIsFloat)
            return;
        _repaired = true;
        int asInt = PrefsSpy.Quiet(() => PlayerPrefs.GetInt(CoinKey, int.MinValue));
        if (asInt != int.MinValue && PrefsSpy.Quiet(() => PlayerPrefs.GetFloat(CoinKey, -1f)) == -1f)
        {
            PlayerPrefs.SetFloat(CoinKey, Math.Clamp(asInt, 0, 9_999_999));
            PlayerPrefs.Save();
            Plugin.Log.LogInfo($"Coins: repaired \"{CoinKey}\" (was saved as a whole number by an older mod version).");
        }
    }

    public static void AddCoins(float amount) => Cheats.Safe(() =>
    {
        if (!Found)
        {
            Plugin.Log.LogWarning("Coins not found yet - open the main menu or the toy capsule shop once.");
            return;
        }

        // Never lower an existing balance, even if it is above the cap.
        float next = Math.Max(Coins, Math.Min(Math.Max(Coins, 0f) + amount, 9_999_999f));
        if (CoinIsFloat)
            PlayerPrefs.SetFloat(CoinKey, next);
        else
            PlayerPrefs.SetInt(CoinKey, (int)next);
        PlayerPrefs.Save();
        Coins = next;
        Plugin.Log.LogInfo($"Coins: {next}");

        // The capsule shop shows coins on its own text; refresh it (without counting it as a shop read).
        if (Capsule != null)
            Cheats.Safe(() => PrefsSpy.Quiet(() => { Capsule.UpdateRarityUI(); return 0; }));
    });

    /// <summary>Buys every item in the main menu shop with the game's own purchase code.</summary>
    public static void UnlockEverything() => Cheats.Safe(() =>
    {
        var menu = Menu;
        if (menu == null)
        {
            Plugin.Log.LogWarning("Unlock everything only works in the main menu.");
            return;
        }

        var buys = new (string name, Action buy)[]
        {
            ("Nightmare", menu.AttemptBuy_NM), ("Grandpa", menu.AttemptBuy_GR), ("SH", menu.AttemptBuy_SH),
            ("IM", menu.AttemptBuy_IM), ("Extras", menu.AttemptBuy_EX), ("Custom", menu.AttemptBuy_CC),
            ("CH", menu.AttemptBuy_CH), ("Traps", menu.AttemptBuy_TRAP), ("Extra traps", menu.AttemptBuy_EXTRAPS),
            ("Robot", menu.AttemptBuy_ROBO), ("Flashlight", menu.AttemptBuy_FLASH), ("Version", menu.AttemptBuy_VERC),
            ("HO", menu.AttemptBuy_HO),
        };

        int done = 0;
        foreach (var (name, buy) in buys)
        {
            AddCoins(100_000);
            try
            {
                buy();
                done++;
            }
            catch (Exception e)
            {
                Plugin.Log.LogWarning($"Couldn't buy {name}: {e.Message}");
            }
        }
        Plugin.Log.LogInfo($"Unlock everything: ran {done}/{buys.Length} shop purchases. Restart the main menu to see them.");
    });

    /// <summary>Rolls the toy capsule until every colour tier is unlocked.</summary>
    public static void UnlockAllColours() => Cheats.Safe(() =>
    {
        var capsule = Capsule;
        if (capsule == null)
        {
            Plugin.Log.LogWarning("Stand at the toy capsule machine first.");
            return;
        }

        int rolls = 0;
        for (; rolls < 400; rolls++)
        {
            if (AllTiersUnlocked(capsule))
                break;
            capsule.UnlockCertainSkin();
        }
        Cheats.Safe(() => capsule.UpdateRarityUI());
        Plugin.Log.LogInfo($"Toy capsule: {rolls} rolls, all colours unlocked: {AllTiersUnlocked(capsule)}");
    });

    private static bool AllTiersUnlocked(ColorToyCapsule capsule)
    {
        var tiers = capsule.tiers;
        if (tiers == null)
            return false;
        for (int i = 0; i < tiers.Length; i++)
            if (!capsule.IsTierFullyUnlocked(tiers[i]))
                return false;
        return true;
    }
}
