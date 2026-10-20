#!/usr/bin/env bash
# Builds Cinematic Unity Explorer (BepInEx 6 Unity IL2CPP CoreCLR) from source into release/CinematicUnityExplorer.
# Source: https://github.com/originalnicodr/CinematicUnityExplorer (GPL-3.0)
# mcs.dll and Tomlet.dll are merged into the plugin with ILRepack (run through mono), like the official release.
#   apt install mono-runtime
set -euo pipefail
OUT="$(cd "$(dirname "$0")/.." && pwd)/release/CinematicUnityExplorer"
WORK="${WORK:-/tmp/cue-build}"
rm -rf "$WORK" && git clone -q --depth 1 https://github.com/originalnicodr/CinematicUnityExplorer "$WORK"
cd "$WORK" && git submodule update --init --depth 1
dotnet build UniverseLib/src/UniverseLib.sln -c Release_IL2CPP_Interop_BIE
dotnet build src/CinematicUnityExplorer.sln -c Release_BIE_Unity_Cpp
SRC="$WORK/Release/CinematicUnityExplorer.BepInEx.Unity.IL2CPP.CoreCLR"
REF="$WORK/ref6"
curl -sSL -o "$WORK/ref6.nupkg" https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.ref/6.0.36/microsoft.netcore.app.ref.6.0.36.nupkg
mkdir -p "$REF" && unzip -qo "$WORK/ref6.nupkg" -d "$REF"
mkdir -p "$OUT"
# ILRepack prints an error after writing the output (System.Data lookup); the merged dll is complete.
mono lib/ILRepack.exe /ndebug /target:library /lib:lib/net472/BepInEx/build647+ /lib:lib/net6/ /lib:lib/interop/ \
  /lib:"$SRC" /lib:"$REF/ref/net6.0" /internalize /out:"$OUT/CinematicUnityExplorer.BIE.Unity.IL2CPP.CoreCLR.dll" \
  "$SRC/CinematicUnityExplorer.BIE.Unity.IL2CPP.CoreCLR.dll" "$SRC/mcs.dll" "$SRC/Tomlet.dll" || true
test -s "$OUT/CinematicUnityExplorer.BIE.Unity.IL2CPP.CoreCLR.dll"
cp "$SRC/UniverseLib.BIE.IL2CPP.Interop.dll" "$OUT/"
echo "Copy $OUT to BepInEx/plugins/CinematicUnityExplorer"
