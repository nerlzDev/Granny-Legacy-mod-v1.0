using BepInEx.Configuration;

namespace GrannyLegacyMod;

/// <summary>Saved settings (BepInEx/config/GrannyLegacyMod.cfg). Cheat toggles themselves are not saved.</summary>
internal static class Settings
{
    public static ConfigEntry<bool> Enabled;
    public static ConfigEntry<bool> UseImGui;

    public static ConfigEntry<bool> FovEnabled;
    public static ConfigEntry<float> Fov;
    public static ConfigEntry<float> SpeedMult;
    public static ConfigEntry<float> NoclipSpeed;
    public static ConfigEntry<float> TimeScale;

    public static ConfigEntry<bool> EspBoxes, EspLines, EspNames, EspDistance;
    public static ConfigEntry<bool> EspCorner, EspFilled, Esp3D, EspHeadDot, MenuParticles;
    public static ConfigEntry<bool> Watermark, Crosshair, Notifications;
    public static ConfigEntry<int> Theme;

    private static ConfigFile _config;

    public static void Bind(ConfigFile config)
    {
        _config = config;
        Enabled = config.Bind("General", "Enabled", true, "Turn the mod on or off.");
        UseImGui = config.Bind("General", "UseImGui", true, "Use the Dear ImGui menu (GrannyOverlay.dll). Off = basic Unity menu.");

        FovEnabled = config.Bind("Camera", "CustomFov", true, "Override the game's field of view.");
        Fov = config.Bind("Camera", "Fov", 90f, new ConfigDescription("Field of view in degrees.", new AcceptableValueRange<float>(30f, 150f)));

        SpeedMult = config.Bind("Movement", "SpeedMultiplier", 1f, new ConfigDescription("Walk speed multiplier.", new AcceptableValueRange<float>(0.5f, 5f)));
        NoclipSpeed = config.Bind("Movement", "NoclipSpeed", 8f, new ConfigDescription("Fly speed.", new AcceptableValueRange<float>(2f, 30f)));
        TimeScale = config.Bind("Movement", "TimeScale", 1f, new ConfigDescription("Game speed when custom time scale is on.", new AcceptableValueRange<float>(0.1f, 3f)));

        EspBoxes = config.Bind("Visuals", "EspBoxes", true, "");
        EspLines = config.Bind("Visuals", "EspSnaplines", false, "");
        EspNames = config.Bind("Visuals", "EspNames", true, "");
        EspDistance = config.Bind("Visuals", "EspDistance", true, "");
        EspCorner = config.Bind("Visuals", "EspCornerBox", false, "");
        EspFilled = config.Bind("Visuals", "EspFilledBox", false, "");
        Esp3D = config.Bind("Visuals", "Esp3DBox", false, "");
        EspHeadDot = config.Bind("Visuals", "EspHeadDot", false, "");
        MenuParticles = config.Bind("Visuals", "MenuParticles", true, "");
        Watermark = config.Bind("Visuals", "Watermark", true, "");
        Crosshair = config.Bind("Visuals", "Crosshair", false, "");
        Notifications = config.Bind("Visuals", "Notifications", true, "");
        // New key so everyone starts on the black & white "Mono" theme (index 52).
        Theme = config.Bind("Visuals", "ThemeIndex", 52, "ImGui theme index (see the Themes tab).");
    }

    public static unsafe void WriteTo(SharedState* s)
    {
        s->FovEnabled = FovEnabled.Value ? 1 : 0;
        s->Fov = Fov.Value;
        s->SpeedMult = SpeedMult.Value;
        s->NoclipSpeed = NoclipSpeed.Value;
        s->TimeScale = TimeScale.Value;
        s->EspBoxes = EspBoxes.Value ? 1 : 0;
        s->EspLines = EspLines.Value ? 1 : 0;
        s->EspNames = EspNames.Value ? 1 : 0;
        s->EspDistance = EspDistance.Value ? 1 : 0;
        s->EspCorner = EspCorner.Value ? 1 : 0;
        s->EspFilled = EspFilled.Value ? 1 : 0;
        s->Esp3D = Esp3D.Value ? 1 : 0;
        s->EspHeadDot = EspHeadDot.Value ? 1 : 0;
        s->MenuParticles = MenuParticles.Value ? 1 : 0;
        s->Watermark = Watermark.Value ? 1 : 0;
        s->Crosshair = Crosshair.Value ? 1 : 0;
        s->Notifications = Notifications.Value ? 1 : 0;
        s->Theme = Theme.Value;
        s->OrbitRadius = 3f;
        s->OrbitSpeed = 1f;
    }

    public static unsafe void ReadFrom(SharedState* s)
    {
        FovEnabled.Value = s->FovEnabled != 0;
        Fov.Value = s->Fov;
        SpeedMult.Value = s->SpeedMult;
        NoclipSpeed.Value = s->NoclipSpeed;
        TimeScale.Value = s->TimeScale;
        EspBoxes.Value = s->EspBoxes != 0;
        EspLines.Value = s->EspLines != 0;
        EspNames.Value = s->EspNames != 0;
        EspDistance.Value = s->EspDistance != 0;
        EspCorner.Value = s->EspCorner != 0;
        EspFilled.Value = s->EspFilled != 0;
        Esp3D.Value = s->Esp3D != 0;
        EspHeadDot.Value = s->EspHeadDot != 0;
        MenuParticles.Value = s->MenuParticles != 0;
        Watermark.Value = s->Watermark != 0;
        Crosshair.Value = s->Crosshair != 0;
        Notifications.Value = s->Notifications != 0;
        Theme.Value = s->Theme;
    }

    public static void Save() => _config.Save();
}
