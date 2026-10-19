using BepInEx;
using BepInEx.Logging;
using BepInEx.Unity.IL2CPP;
using HarmonyLib;
using Il2CppInterop.Runtime.Injection;

namespace GrannyLegacyMod;

[BepInPlugin(MyPluginInfo.PLUGIN_GUID, MyPluginInfo.PLUGIN_NAME, MyPluginInfo.PLUGIN_VERSION)]
public class Plugin : BasePlugin
{
    internal static Plugin Instance;
    internal static new ManualLogSource Log;

    public override void Load()
    {
        Instance = this;
        Log = base.Log;

        Settings.Bind(Config);
        ConsoleLog.Install();

        if (!Settings.Enabled.Value)
        {
            Log.LogInfo("Disabled in config, not patching.");
            return;
        }

        // Apply every [HarmonyPatch] class in this assembly (Patches folder) one by one,
        // so one failing patch doesn't take the others down with it.
        var harmony = new Harmony(MyPluginInfo.PLUGIN_GUID);
        foreach (var type in typeof(Plugin).Assembly.GetTypes())
        {
            if (type.GetCustomAttributes(typeof(HarmonyPatch), false).Length == 0)
                continue;
            try
            {
                harmony.CreateClassProcessor(type).Patch();
            }
            catch (System.Exception e)
            {
                Log.LogError($"Patch {type.Name} failed: {e.Message}");
            }
        }

        Cheats.SpeedMultiplier = Settings.SpeedMult.Value;
        Cheats.NoclipSpeed = Settings.NoclipSpeed.Value;

        if (Settings.UseImGui.Value)
            Overlay.Load();

        ClassInjector.RegisterTypeInIl2Cpp<ModMenu>();
        AddComponent<ModMenu>();

        Log.LogInfo($"{MyPluginInfo.PLUGIN_NAME} {MyPluginInfo.PLUGIN_VERSION} loaded! Press Delete for the menu.");
    }
}
