using System;
using System.Reflection;

namespace GrannyLegacyMod;

/// <summary>Opens/closes Cinematic Unity Explorer (UnityExplorer.UI.UIManager.ShowMenu) if it is installed.</summary>
internal static class Explorer
{
    private static PropertyInfo _showMenu;
    private static float _nextSearch;
    private static bool _hotkeyRemoved;

    /// <summary>0 = not installed, 1 = closed, 2 = open.</summary>
    public static int Status
    {
        get
        {
            var p = Find();
            if (p == null)
                return 0;
            if (!_hotkeyRemoved)
                RemoveHotkey(p.DeclaringType.Assembly, p);
            try
            {
                return (bool)p.GetValue(null) ? 2 : 1;
            }
            catch
            {
                return 1;
            }
        }
    }

    public static void Toggle() => Cheats.Safe(() =>
    {
        var p = Find();
        if (p == null)
        {
            Plugin.Log.LogWarning("Cinematic Unity Explorer isn't installed (BepInEx/plugins).");
            return;
        }
        bool open = (bool)p.GetValue(null);
        p.SetValue(null, !open);
        if ((bool)p.GetValue(null) == open)
            Plugin.Log.LogWarning("Cinematic Unity Explorer is loaded but its UI isn't ready yet. " +
                                  "Wait a few seconds after the game starts, or check LogOutput.log for UniverseLib errors.");
    });

    /// <summary>
    /// The explorer opens from our menu only: set its toggle key (F7 by default) to None.
    /// ConfigManager.Master_Toggle is a ConfigElement&lt;KeyCode&gt; created when the explorer starts.
    /// </summary>
    private static void RemoveHotkey(Assembly asm, PropertyInfo showMenu)
    {
        try
        {
            var config = asm.GetType("UnityExplorer.Config.ConfigManager", false);
            var element = config?.GetField("Master_Toggle", BindingFlags.Public | BindingFlags.Static)?.GetValue(null);
            if (element == null)
                return;  // not started yet, try again next time
            var value = element.GetType().GetProperty("Value");
            if (value == null)
                return;
            if (!Equals(value.GetValue(element), UnityEngine.KeyCode.None))
                value.SetValue(element, UnityEngine.KeyCode.None);

            // Don't pop up when the game starts: turn on its "Hide On Startup" setting (saved for next
            // time) and close the window it already opened this time.
            var hide = config.GetField("Hide_On_Startup", BindingFlags.Public | BindingFlags.Static)?.GetValue(null);
            var hideValue = hide?.GetType().GetProperty("Value");
            if (hideValue != null && !Equals(hideValue.GetValue(hide), true))
                hideValue.SetValue(hide, true);
            if ((bool)showMenu.GetValue(null))
                showMenu.SetValue(null, false);

            _hotkeyRemoved = true;
            Plugin.Log.LogInfo("Cinematic Unity Explorer: hidden at startup, F7 removed - open it from the menu (Console > Tools).");
        }
        catch (Exception e)
        {
            _hotkeyRemoved = true;
            Plugin.Log.LogWarning($"Couldn't remove the explorer hotkey: {e.Message}");
        }
    }

    private static PropertyInfo Find()
    {
        if (_showMenu != null)
            return _showMenu;
        if (UnityEngine.Time.unscaledTime < _nextSearch)
            return null;
        _nextSearch = UnityEngine.Time.unscaledTime + 3f;

        foreach (var asm in AppDomain.CurrentDomain.GetAssemblies())
        {
            Type t;
            try
            {
                t = asm.GetType("UnityExplorer.UI.UIManager", false);
            }
            catch
            {
                continue;
            }
            var prop = t?.GetProperty("ShowMenu", BindingFlags.Public | BindingFlags.Static);
            if (prop != null)
            {
                _showMenu = prop;
                Plugin.Log.LogInfo($"Found Cinematic Unity Explorer in {asm.GetName().Name}");
                break;
            }
        }
        return _showMenu;
    }
}
