# Build GrannyOverlay.dll with Visual Studio:
#   cmake -B build -A x64 && cmake --build build --config Release
cmake_minimum_required(VERSION 3.16)
project(GrannyOverlay C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

add_library(GrannyOverlay SHARED
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_tables.cpp
    imgui/imgui_impl_dx11.cpp imgui/imgui_impl_win32.cpp
    gui/widgets/imgui_widgets.cpp
    src/menu.cpp src/hook.cpp)

target_compile_definitions(GrannyOverlay PRIVATE WIN32_LEAN_AND_MEAN UNICODE _UNICODE NOMINMAX)
target_link_libraries(GrannyOverlay PRIVATE d3d11 d3dcompiler dxgi dwmapi imm32 shell32)
set_target_properties(GrannyOverlay PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/build)

# Standalone injectable build: overlay + all cheats in C++ (no BepInEx).
add_library(Cluado SHARED
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_tables.cpp
    imgui/imgui_impl_dx11.cpp imgui/imgui_impl_win32.cpp
    gui/widgets/imgui_widgets.cpp
    src/menu.cpp src/hook.cpp src/game.cpp src/il2cpp.cpp
    third_party/minhook/src/hook.c third_party/minhook/src/buffer.c
    third_party/minhook/src/trampoline.c third_party/minhook/src/hde/hde64.c)
target_compile_definitions(Cluado PRIVATE STANDALONE WIN32_LEAN_AND_MEAN UNICODE _UNICODE NOMINMAX)
target_link_libraries(Cluado PRIVATE d3d11 d3dcompiler dxgi dwmapi imm32 shell32 advapi32)
set_target_properties(Cluado PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/build)
