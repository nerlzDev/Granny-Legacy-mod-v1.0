#!/usr/bin/env bash
# Cross-compiles with MinGW-w64 (Linux / WSL / MSYS2):
#   GrannyOverlay.dll - ImGui overlay for the BepInEx plugin
#   Cluado.dll        - standalone: overlay + all cheats in C++, inject it into the game (no BepInEx)
#   apt install g++-mingw-w64-x86-64-posix
set -euo pipefail
cd "$(dirname "$0")"
CXX=${CXX:-x86_64-w64-mingw32-g++}
CC=${CC:-x86_64-w64-mingw32-gcc}
OUT=${1:-build}
mkdir -p "$OUT/obj"

COMMON=(
  imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_tables.cpp
  imgui/imgui_impl_dx11.cpp imgui/imgui_impl_win32.cpp
  gui/widgets/imgui_widgets.cpp
)
STANDALONE_ONLY=( src/game.cpp src/il2cpp.cpp )
MINHOOK=( third_party/minhook/src/hook.c third_party/minhook/src/buffer.c
          third_party/minhook/src/trampoline.c third_party/minhook/src/hde/hde64.c )
FLAGS=(-O2 -DNDEBUG -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE)
LIBS=(-static -static-libgcc -static-libstdc++ -s
      -ld3d11 -ld3dcompiler_47 -ldxgi -ldwmapi -limm32 -lshell32 -lgdi32 -luser32 -ladvapi32)

PIDS=()
compile() {  # compile <src> <obj> <extra flags...>
  local src=$1 obj=$2; shift 2
  if [[ $src == *.c ]]; then "$CC" "${FLAGS[@]}" "$@" -c "$src" -o "$obj" &
  else "$CXX" -std=c++17 "${FLAGS[@]}" "$@" -c "$src" -o "$obj" & fi
  PIDS+=($!)
}
obj_of() { echo "$OUT/obj/${2:-}$(echo "$1" | tr '/' '_').o"; }

OBJS=(); SOBJS=()
for src in "${COMMON[@]}"; do o=$(obj_of "$src"); compile "$src" "$o"; OBJS+=("$o"); SOBJS+=("$o"); done
for src in src/menu.cpp src/hook.cpp; do
  o=$(obj_of "$src"); compile "$src" "$o"; OBJS+=("$o")
  s=$(obj_of "$src" sa_); compile "$src" "$s" -DSTANDALONE; SOBJS+=("$s")
done
for src in "${STANDALONE_ONLY[@]}" "${MINHOOK[@]}"; do
  s=$(obj_of "$src" sa_); compile "$src" "$s" -DSTANDALONE; SOBJS+=("$s")
done
for pid in "${PIDS[@]}"; do wait "$pid"; done

"$CXX" -shared -o "$OUT/GrannyOverlay.dll" "${OBJS[@]}" "${LIBS[@]}"
echo "Built $OUT/GrannyOverlay.dll"
"$CXX" -shared -o "$OUT/Cluado.dll" "${SOBJS[@]}" "${LIBS[@]}"
echo "Built $OUT/Cluado.dll"
