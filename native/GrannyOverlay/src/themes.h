#pragma once
#include "../imgui/imgui.h"

// Theme table: the first 28 come from the Nerlz GUI (CGui::ApplyTheme), the rest were added for this mod.
// New themes go at the end so saved theme indices keep pointing at the same theme.

struct Theme { const char* name; unsigned bg, popup, child, text, disabled, accent, accentDark; ImVec4 wm1, wm2; };

static const Theme kThemes[] = {
    { "Mocha", 0x1F1F1F, 0x272727, 0x2F2F2F, 0xFFFFFF, 0xA2A2A2, 0xD8A1FC, 0xB67FDF, ImVec4(0.85f,0.63f,0.99f,1), ImVec4(0.71f,0.50f,0.87f,1) },
    { "Dark Blue", 0x1D2125, 0x24292E, 0x2B3137, 0xFFFFFF, 0x959DA5, 0x99C8FF, 0x8AB2E1, ImVec4(0.60f,0.75f,1.00f,1), ImVec4(0.30f,0.50f,0.80f,1) },
    { "Dark Purple", 0x231D2B, 0x2C2336, 0x342B40, 0xFFFFFF, 0xA491AF, 0xB199FF, 0x9985DA, ImVec4(0.70f,0.60f,1.00f,1), ImVec4(0.40f,0.30f,0.80f,1) },
    { "Dark", 0x1F1F1F, 0x272727, 0x2F2F2F, 0xFFFFFF, 0xA2A2A2, 0x99C8FF, 0x8AB2E1, ImVec4(0.70f,0.70f,0.70f,1), ImVec4(0.40f,0.40f,0.40f,1) },
    { "Red", 0x2B1D1D, 0x362323, 0x402B2B, 0xFFFFFF, 0xAF9191, 0xFF9999, 0xDA8585, ImVec4(1.00f,0.60f,0.60f,1), ImVec4(0.80f,0.30f,0.30f,1) },
    { "Frappe", 0x303446, 0x292C3C, 0x414559, 0xC6D0F5, 0x838BA7, 0xCA9EE6, 0xA47EBA, ImVec4(0.79f,0.62f,0.90f,1), ImVec4(0.50f,0.35f,0.65f,1) },
    { "Macchiato", 0x1E2030, 0x181926, 0x363A4F, 0xCAD3F5, 0x6E738D, 0xC6A0F6, 0xA076CF, ImVec4(0.78f,0.63f,0.96f,1), ImVec4(0.48f,0.35f,0.70f,1) },
    { "Nord", 0x2E3440, 0x3B4252, 0x434C5E, 0xECEFF4, 0x7B88A1, 0x88C0D0, 0x6AAAB8, ImVec4(0.53f,0.75f,0.82f,1), ImVec4(0.28f,0.50f,0.60f,1) },
    { "Tokyo Night", 0x1A1B26, 0x16161E, 0x24283B, 0xC0CAF5, 0x565F89, 0x7AA2F7, 0x4E7FD5, ImVec4(0.48f,0.64f,0.97f,1), ImVec4(0.24f,0.40f,0.75f,1) },
    { "Dracula", 0x282A36, 0x21222C, 0x44475A, 0xF8F8F2, 0x6272A4, 0xBD93F9, 0x9D73D9, ImVec4(0.74f,0.58f,0.98f,1), ImVec4(0.44f,0.28f,0.75f,1) },
    { "One Dark", 0x282C34, 0x21252B, 0x2C313C, 0xABB2BF, 0x5C6370, 0x61AFEF, 0x3D8ED0, ImVec4(0.38f,0.69f,0.94f,1), ImVec4(0.18f,0.45f,0.72f,1) },
    { "Gruvbox", 0x282828, 0x1D2021, 0x3C3836, 0xEBDBB2, 0x928374, 0xD79921, 0xB57614, ImVec4(0.84f,0.60f,0.13f,1), ImVec4(0.58f,0.38f,0.05f,1) },
    { "Everforest", 0x2D353B, 0x272E33, 0x3D484D, 0xD3C6AA, 0x7A8478, 0xA7C080, 0x83A15C, ImVec4(0.65f,0.75f,0.50f,1), ImVec4(0.38f,0.52f,0.24f,1) },
    { "Rose Pine", 0x191724, 0x1F1D2E, 0x26233A, 0xE0DEF4, 0x6E6A86, 0xEB6F92, 0xC44F6E, ImVec4(0.92f,0.44f,0.57f,1), ImVec4(0.62f,0.18f,0.32f,1) },
    { "Kanagawa", 0x1F1F28, 0x16161D, 0x2A2A37, 0xDCD7BA, 0x7A7A90, 0x7E9CD8, 0x5A7DB5, ImVec4(0.49f,0.61f,0.85f,1), ImVec4(0.26f,0.40f,0.63f,1) },
    { "Synthwave", 0x2B213A, 0x1A1025, 0x3D2B56, 0xF8F8F2, 0x8B6AB5, 0xF92AAD, 0xCC0888, ImVec4(0.98f,0.17f,0.68f,1), ImVec4(0.68f,0.03f,0.43f,1) },
    { "Cyberpunk", 0x0D0221, 0x0A0118, 0x1A0533, 0xF8F8F8, 0x8A6EB5, 0xFCEE09, 0xD4C507, ImVec4(0.99f,0.93f,0.04f,1), ImVec4(0.68f,0.62f,0.02f,1) },
    { "Ocean", 0x0F2536, 0x0A1929, 0x1E3A5F, 0xE0F4FF, 0x5A8FA8, 0x00B4D8, 0x0090B0, ImVec4(0.00f,0.71f,0.85f,1), ImVec4(0.00f,0.44f,0.58f,1) },
    { "Forest", 0x1A2E1A, 0x152315, 0x2D4A2D, 0xD4EDDA, 0x5A8A5A, 0x52B788, 0x3A9267, ImVec4(0.32f,0.72f,0.53f,1), ImVec4(0.14f,0.46f,0.28f,1) },
    { "Sakura", 0x2D1B2E, 0x21152F, 0x3D2440, 0xFCE4EC, 0x9A7A90, 0xF48FB1, 0xD16B8E, ImVec4(0.96f,0.56f,0.69f,1), ImVec4(0.68f,0.22f,0.40f,1) },
    { "Coffee", 0x2C1F14, 0x1E1510, 0x3D2C1E, 0xF0E0C0, 0x9A7A5A, 0xC8956C, 0xA67550, ImVec4(0.78f,0.58f,0.42f,1), ImVec4(0.52f,0.32f,0.18f,1) },
    { "Arctic", 0x1C2333, 0x151C2B, 0x2D3A4A, 0xE8F4FD, 0x6A8A9A, 0x74B9E6, 0x4A9AC8, ImVec4(0.45f,0.73f,0.90f,1), ImVec4(0.18f,0.47f,0.66f,1) },
    { "Midnight", 0x0D0D1A, 0x080810, 0x1A1A2E, 0xE0E0FF, 0x5A5A90, 0x6060EE, 0x4040CC, ImVec4(0.38f,0.38f,0.93f,1), ImVec4(0.15f,0.15f,0.65f,1) },
    { "Ember", 0x1E1208, 0x160D05, 0x2E1C0C, 0xFFE8CC, 0x9A6A3A, 0xFF6B35, 0xD84A18, ImVec4(1.00f,0.42f,0.21f,1), ImVec4(0.72f,0.20f,0.04f,1) },
    { "Mint", 0x0D1E1A, 0x091512, 0x1A2E28, 0xE0FFF5, 0x3A8A6A, 0x00E5A0, 0x00B880, ImVec4(0.00f,0.90f,0.63f,1), ImVec4(0.00f,0.54f,0.36f,1) },
    { "Lavender", 0x1E1A2E, 0x161222, 0x2E2844, 0xF0E8FF, 0x8A7A9A, 0xB896F0, 0x9470D0, ImVec4(0.72f,0.59f,0.94f,1), ImVec4(0.44f,0.28f,0.70f,1) },
    { "Sunset", 0x1E1020, 0x160C18, 0x2E1830, 0xFFE0F0, 0x9A5A70, 0xFF7EB3, 0xD8508A, ImVec4(1.00f,0.49f,0.70f,1), ImVec4(0.72f,0.17f,0.42f,1) },
    { "Neon", 0x0A0A12, 0x050508, 0x14141F, 0xFFFFFF, 0x6A9A7A, 0x00FF88, 0x00CC6E, ImVec4(0.00f,1.00f,0.53f,1), ImVec4(0.00f,0.62f,0.28f,1) },
    { "Catppuccin Latte", 0xEFF1F5, 0xE6E9EF, 0xDCE0E8, 0x4C4F69, 0x8C8FA1, 0x8839EF, 0x7287FD, ImVec4(0.53f,0.22f,0.94f,1), ImVec4(0.45f,0.53f,0.99f,1) },
    { "Solarized", 0x002B36, 0x073642, 0x0E4A57, 0xEEE8D5, 0x839496, 0x268BD2, 0x1E6FA8, ImVec4(0.15f,0.55f,0.82f,1), ImVec4(0.12f,0.44f,0.66f,1) },
    { "Monokai", 0x272822, 0x1E1F1C, 0x3E3D32, 0xF8F8F2, 0x75715E, 0xA6E22E, 0x86B51F, ImVec4(0.65f,0.89f,0.18f,1), ImVec4(0.53f,0.71f,0.12f,1) },
    { "Ayu Dark", 0x0A0E14, 0x0D1017, 0x1F2430, 0xB3B1AD, 0x5C6773, 0xFFB454, 0xE6A045, ImVec4(1.00f,0.71f,0.33f,1), ImVec4(0.90f,0.63f,0.27f,1) },
    { "GitHub Dark", 0x0D1117, 0x161B22, 0x21262D, 0xE6EDF3, 0x7D8590, 0x2F81F7, 0x1F6FEB, ImVec4(0.18f,0.51f,0.97f,1), ImVec4(0.12f,0.44f,0.92f,1) },
    { "Material Ocean", 0x0F111A, 0x090B10, 0x1F2233, 0xA6ACCD, 0x464B5D, 0x84FFFF, 0x5CCFCF, ImVec4(0.52f,1.00f,1.00f,1), ImVec4(0.36f,0.81f,0.81f,1) },
    { "Blood Moon", 0x140808, 0x0D0505, 0x2A0F0F, 0xFFE5E5, 0x8A5A5A, 0xE53935, 0xB71C1C, ImVec4(0.90f,0.22f,0.21f,1), ImVec4(0.72f,0.11f,0.11f,1) },
    { "Crimson", 0x1A0E12, 0x12090C, 0x2B1720, 0xFFE6EC, 0x9A6A78, 0xDC143C, 0xA80F2E, ImVec4(0.86f,0.08f,0.24f,1), ImVec4(0.66f,0.06f,0.18f,1) },
    { "Emerald", 0x0B1A14, 0x07120E, 0x163427, 0xE3FFF2, 0x5C8F78, 0x2ECC71, 0x229A55, ImVec4(0.18f,0.80f,0.44f,1), ImVec4(0.13f,0.60f,0.33f,1) },
    { "Gold", 0x17140B, 0x100E07, 0x2A2412, 0xFFF6DC, 0x9A8A5A, 0xFFC93C, 0xD9A520, ImVec4(1.00f,0.79f,0.24f,1), ImVec4(0.85f,0.65f,0.13f,1) },
    { "Royal", 0x120F24, 0x0C0A19, 0x221C40, 0xEEE8FF, 0x7D74A8, 0x7C4DFF, 0x5E35D1, ImVec4(0.49f,0.30f,1.00f,1), ImVec4(0.37f,0.21f,0.82f,1) },
    { "Ice", 0x0E1A22, 0x091218, 0x1A2E3A, 0xEAF8FF, 0x6D8FA3, 0xA0E9FF, 0x6CC8E8, ImVec4(0.63f,0.91f,1.00f,1), ImVec4(0.42f,0.78f,0.91f,1) },
    { "Toxic", 0x0D140A, 0x080D06, 0x1A2814, 0xEFFFE0, 0x6E8A5A, 0x9DFF00, 0x7ACC00, ImVec4(0.62f,1.00f,0.00f,1), ImVec4(0.48f,0.80f,0.00f,1) },
    { "Peach", 0x22161A, 0x180F12, 0x352328, 0xFFEFE8, 0xA38079, 0xFFAB91, 0xE88B6F, ImVec4(1.00f,0.67f,0.57f,1), ImVec4(0.91f,0.55f,0.44f,1) },
    { "Matrix", 0x000000, 0x050A05, 0x0A1A0A, 0x00FF41, 0x008F11, 0x00FF41, 0x00B32D, ImVec4(0.00f,1.00f,0.25f,1), ImVec4(0.00f,0.70f,0.18f,1) },
    { "Steel", 0x1B1E23, 0x14161A, 0x2A2F36, 0xE8ECF1, 0x7A8494, 0x9AA8BA, 0x77869A, ImVec4(0.60f,0.66f,0.73f,1), ImVec4(0.47f,0.53f,0.60f,1) },
    { "Bubblegum", 0x231526, 0x190E1B, 0x382240, 0xFFEAF8, 0xA77E9E, 0xFF77D9, 0xE055B8, ImVec4(1.00f,0.47f,0.85f,1), ImVec4(0.88f,0.33f,0.72f,1) },
    { "Deep Sea", 0x061423, 0x040E19, 0x0E2640, 0xDDF1FF, 0x4F7699, 0x1E90FF, 0x1570CC, ImVec4(0.12f,0.56f,1.00f,1), ImVec4(0.08f,0.44f,0.80f,1) },
    { "Amethyst", 0x1A1226, 0x120C1B, 0x2C1F40, 0xF3E9FF, 0x8E78A8, 0x9966CC, 0x7A4FB0, ImVec4(0.60f,0.40f,0.80f,1), ImVec4(0.48f,0.31f,0.69f,1) },
    { "Cotton Candy", 0x1D1B2E, 0x151322, 0x2E2A48, 0xFFF0FA, 0x9C93B5, 0x89CFF0, 0xF4A6D7, ImVec4(0.54f,0.81f,0.94f,1), ImVec4(0.96f,0.65f,0.84f,1) },
    { "Galaxy", 0x0B0820, 0x070514, 0x1A1440, 0xEDE7FF, 0x6F64A8, 0xC471ED, 0x8E54E9, ImVec4(0.77f,0.44f,0.93f,1), ImVec4(0.56f,0.33f,0.91f,1) },
    { "Lime", 0x151A0D, 0x0E1208, 0x253015, 0xF4FFE0, 0x86956A, 0xC6FF00, 0x9ECC00, ImVec4(0.78f,1.00f,0.00f,1), ImVec4(0.62f,0.80f,0.00f,1) },
    { "Rose Gold", 0x211819, 0x171112, 0x342728, 0xFFF0EE, 0xA88A86, 0xE8B4B8, 0xC9918F, ImVec4(0.91f,0.71f,0.72f,1), ImVec4(0.79f,0.57f,0.56f,1) },
    { "Obsidian", 0x0B0B0D, 0x111114, 0x1A1A1F, 0xF2F2F2, 0x7A7A80, 0xBDBDBD, 0x9E9E9E, ImVec4(1.00f,1.00f,1.00f,1), ImVec4(0.74f,0.74f,0.74f,1) },
    { "Mono", 0x0A0A0A, 0x111111, 0x1A1A1A, 0xFFFFFF, 0x8A8A8A, 0xC8C8C8, 0xA0A0A0, ImVec4(1,1,1,1), ImVec4(0.78f,0.78f,0.78f,1) },
};

// Default theme for new installs: black and white.
static const int kThemeMono = 52;

static const int kThemeCount = sizeof(kThemes) / sizeof(kThemes[0]);
