#pragma once
#include <windows.h>
#include <d3d11.h>

bool Menu_Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
void Menu_Frame();  // NewFrame + build UI + ImGui::Render()
void Menu_Shutdown();
void Radial_Update(HWND window);  // call before Menu_Frame each frame
