using System.Collections.Generic;
using System.Reflection;
using HarmonyLib;
using UnityEngine;

namespace GrannyLegacyMod.Patches;

/// <summary>God mode: skip every way PlayerStatus can kill or hurt the player.</summary>
[HarmonyPatch]
internal static class PlayerDeathPatch
{
    private static IEnumerable<MethodBase> TargetMethods()
    {
        foreach (var name in new[]
                 {
                     nameof(PlayerStatus.GrannyCaughtYou), nameof(PlayerStatus.GrannyCaughtYouBed),
                     nameof(PlayerStatus.GrandpaCaughtYou), nameof(PlayerStatus.GrandpaCaughtYouBed),
                     nameof(PlayerStatus.KnockDeath), nameof(PlayerStatus.NormalDeath),
                     nameof(PlayerStatus.MomDeath), nameof(PlayerStatus.DamageSpider),
                     nameof(PlayerStatus.DamageFeet),
                 })
            yield return AccessTools.Method(typeof(PlayerStatus), name);
    }

    private static bool Prefix() => !Cheats.GodMode;
}

/// <summary>God mode: Granny and Grandpa can't grab the player (their catch trigger ignores you).</summary>
[HarmonyPatch]
internal static class EnemyCatchPatch
{
    private static IEnumerable<MethodBase> TargetMethods()
    {
        yield return AccessTools.Method(typeof(AI_Granny), nameof(AI_Granny.OnTriggerStay));
        yield return AccessTools.Method(typeof(AI_Grandpa), nameof(AI_Grandpa.OnTriggerStay));
    }

    // Orbit also blocks it: riders pass right next to the player.
    private static bool Prefix(Collider other) => !((Cheats.GodMode || Orbit.Enabled) && IsPlayer(other));

    private static bool IsPlayer(Collider other)
    {
        var fps = Cheats.Fps;
        if (other == null || fps == null)
            return false;
        var player = fps.characterController != null ? fps.characterController.transform : fps.transform;
        return other.transform.IsChildOf(player);
    }
}

/// <summary>Invisible: Granny and Grandpa never notice the player.</summary>
[HarmonyPatch]
internal static class EnemySensorPatch
{
    private static IEnumerable<MethodBase> TargetMethods()
    {
        yield return AccessTools.Method(typeof(AI_Granny), nameof(AI_Granny.SensorPlayer));
        yield return AccessTools.Method(typeof(AI_Grandpa), nameof(AI_Grandpa.SensorPlayer));
    }

    private static bool Prefix() => !Cheats.Invisible;
}
