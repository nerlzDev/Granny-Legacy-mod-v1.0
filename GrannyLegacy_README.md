# Granny Legacy Mod

A BepInEx 6 (IL2CPP) plugin for Granny Legacy.

## Features

Press **Delete** in game to open/close the menu. It is a **Dear ImGui** DirectX 11 overlay
(`GrannyOverlay.dll`) with the Nerlz sidebar layout, 52 themes, a watermark and notifications.
If the overlay can't load, a basic Unity menu with the same cheats is used instead.

| Tab | Features |
|---|---|
| Player | God mode, invisible to Granny & Grandpa, save position / teleport back |
| Movement | Walk speed x0.5-x5, noclip / fly, custom time scale |
| Enemies | Freeze all enemies, knock out Granny / Grandpa / both |
| Visuals | Custom FOV, enemy ESP (2D / filled / corner / 3D box, tracers, names, distance, head dot), watermark, crosshair, notifications |
| Console | Live log from the mod and the game |
| World | Fullbright (no darkness or fog), change the current day |
| Items | Searchable list of every item, click to take it |
| Shop | Add coins, unlock everything (main menu) |
| Enemy wheel | Hold **X** on an enemy: freeze, knock out, pepper, ice, teleport to, bring, fling, despawn |
| Themes | 53 themes (Mono black & white by default, Mocha, Dracula, Tokyo Night, Nord, Monokai, Matrix, Galaxy, ...) |

Settings are saved to `BepInEx/config/GrannyLegacyMod.cfg`.

## Standalone (no BepInEx)

`Cluado.dll` is one DLL with the ImGui menu and all cheats rewritten in C++ (IL2CPP API + MinHook).
Start the game, then inject `Cluado.dll` with any DLL injector. Settings go to `Cluado.ini` and a log to
`Cluado.log` next to the DLL. Cinematic Unity Explorer needs BepInEx, so it is only in the BepInEx build.

## 1. Install BepInEx in the game

1. Download [BepInEx 6.0.0-be.789 IL2CPP win-x64](https://builds.bepinex.dev/projects/bepinex_be/789/BepInEx-Unity.IL2CPP-win-x64-6.0.0-be.789%2B6cea286.zip).
   Older builds (e.g. be.697) fail with `Unsupported metadata version ... got 31` on this game.
2. Extract it next to `Granny_Legacy.exe`.
3. Run the game once and wait. BepInEx generates `BepInEx/interop/` (game + Unity assemblies).

## 2. Build the mod

### ImGui overlay (C++)

`native/GrannyOverlay` - Dear ImGui + DirectX 11 Present hook. Build it **before** the C# plugin:

```
# Linux / WSL (MinGW-w64)
native/GrannyOverlay/build.sh

# Windows (Visual Studio)
cmake -S native/GrannyOverlay -B native/GrannyOverlay/cmake-build -A x64
cmake --build native/GrannyOverlay/cmake-build --config Release
```

Output: `native/GrannyOverlay/build/GrannyOverlay.dll` (picked up automatically by the C# build).

### Plugin (C#)

Needs the [.NET SDK](https://dotnet.microsoft.com/download) (6.0 or newer).

```
dotnet build src/GrannyLegacyMod -c Release -p:GameDir="C:\Path\To\Granny Legacy"
```

If `GameDir` points at the game, both DLLs are copied to `BepInEx/plugins/GrannyLegacyMod/` automatically.
Otherwise copy `GrannyLegacyMod.dll` and `GrannyOverlay.dll` from `src/GrannyLegacyMod/bin/Release/net6.0/` there yourself.

Start the game and look for `GrannyLegacyMod 0.9.0 loaded!` and `ImGui overlay loaded.` in `BepInEx/LogOutput.log`.

## 3. Write mod features

- Open `BepInEx/interop/Assembly-CSharp.dll` in [dnSpy](https://github.com/dnSpyEx/dnSpy) or ILSpy to find the game's classes.
- Add Harmony patches in `src/GrannyLegacyMod/Patches/` (see the README there).
- Config file is created at `BepInEx/config/GrannyLegacyMod.cfg` after the first run.

## Cinematic Unity Explorer (optional)

Release builds merge it into GrannyLegacyMod.dll (`tools/build-cinematic-unity-explorer.sh`, then `tools/make-release.sh`). Don't also keep a separate CinematicUnityExplorer folder. Or install [CinematicUnityExplorer for BepInEx 6 IL2CPP](https://github.com/originalnicodr/CinematicUnityExplorer/releases/latest/download/CinematicUnityExplorer.BepInEx.Unity.IL2CPP.CoreCLR.zip)
into `BepInEx/plugins`, then open it from Console > Tools in the menu (or press F7).
