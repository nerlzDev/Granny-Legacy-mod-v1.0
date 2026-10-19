#pragma once
#include <string>

// Standalone build only: the cheats in C++ (replaces the C# BepInEx plugin).
bool Game_Start(const std::string& dllDir);  // waits for IL2CPP, resolves the game, installs hooks
void Game_Log(const char* fmt, ...);
