#!/usr/bin/env bash
# Merges GrannyLegacyMod.dll + Cinematic Unity Explorer + UniverseLib into one GrannyLegacyMod.dll
# (both BepInEx plugins stay in the same assembly). Run tools/build-cinematic-unity-explorer.sh first.
#   apt install mono-runtime
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CUE="${CUE:-/tmp/cue-build}"
INTEROP="${INTEROP:?set INTEROP to the game's BepInEx/interop folder}"
OUT="$ROOT/release/onefile"
mkdir -p "$OUT"

LIBS=()
for d in $(find ~/.nuget/packages/{bepinex.unity.il2cpp,bepinex.core,bepinex.unity.common,il2cppinterop.runtime,il2cppinterop.common,harmonyx,microsoft.extensions.logging.abstractions} \
           -path "*lib/net*" -maxdepth 4 -type d 2>/dev/null); do LIBS+=("/lib:$d"); done

cd "$CUE"
mono lib/ILRepack.exe /ndebug /target:library "${LIBS[@]}" /lib:"$INTEROP" /lib:lib/net6/ /lib:lib/interop/ \
  /lib:"$CUE/ref6/ref/net6.0" /lib:Release/CinematicUnityExplorer.BepInEx.Unity.IL2CPP.CoreCLR \
  /out:"$OUT/GrannyLegacyMod.dll" \
  "$ROOT/src/GrannyLegacyMod/bin/Release/net6.0/GrannyLegacyMod.dll" \
  "$ROOT/release/CinematicUnityExplorer/CinematicUnityExplorer.BIE.Unity.IL2CPP.CoreCLR.dll" \
  "$ROOT/release/CinematicUnityExplorer/UniverseLib.BIE.IL2CPP.Interop.dll" || true
test -s "$OUT/GrannyLegacyMod.dll"
cp "$ROOT/native/GrannyOverlay/build/GrannyOverlay.dll" "$OUT/"
echo "Release in $OUT"
