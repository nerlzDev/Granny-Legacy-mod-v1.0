#include "menu.h"
#include "shared.h"
#include "themes.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include "../gui/widgets/imgui_widgets.h"
#include "../gui/fonts/IconsFontAwesome6.h"
#include "../gui/fonts/Font.h"
#include "../gui/fonts/fasolid900.h"
#include "../gui/fonts/fabrands400.h"
#include "../gui/fonts/IconsFontAwesome6Brands.h"

#include <shellapi.h>

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace W = ImGui::Widgets;

enum Page : int
{
    Page_Player,
    Page_Movement,
    Page_Enemies,
    Page_Visuals,
    Page_World,
    Page_Items,
    Page_Shop,
    Page_Themes,
    Page_About,
    Page_Console,
    Page_COUNT
};

struct PageInfo { const char* name; const char* desc; const char* icon; };

static const PageInfo kPages[Page_COUNT] = {
    { "Player",   "Stay alive no matter what",          ICON_FA_USER },
    { "Movement", "Billions must fly!",                 ICON_FA_FEATHER_POINTED },
    { "Enemies",  "Granny can't catch what she can't move", ICON_FA_SKULL },
    { "Visuals",  "See them before they see you",       ICON_FA_EYE },
    { "World",    "Bend the house to your will",        ICON_FA_EARTH_AMERICAS },
    { "Items",    "Every item, one click away",         ICON_FA_BOX },
    { "Shop",     "Coins and unlocks",                  ICON_FA_COINS },
    { "Themes",   "Choose your client theme",           ICON_FA_DISPLAY },
    { "About",    "Granny Legacy Mod",                  ICON_FA_CIRCLE_INFO },
    { "Console",  "What the mod and the game are saying", ICON_FA_TERMINAL },
};

static struct
{
    ImFont* Main;
    ImFont* Big;
    ImFont* Icons;
    ImFont* Brands;
} g_Fonts;

static int g_Page = Page_Player;
static float g_WatermarkBottom;  // toasts start below the watermark

// ---------------------------------------------------------------- notifications

struct Toast { std::string msg; float born; };
static std::vector<Toast> g_Toasts;

static void Notify(const std::string& msg)
{
    if (!g_State.notifications)
        return;
    g_Toasts.push_back({ msg, (float)ImGui::GetTime() });
    if (g_Toasts.size() > 5)
        g_Toasts.erase(g_Toasts.begin());
}

static void RenderToasts()
{
    const float life = 2.5f, now = (float)ImGui::GetTime();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 display = ImGui::GetIO().DisplaySize;
    float y = g_State.watermark ? g_WatermarkBottom + 8.0f : 12.0f;
    const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);

    for (size_t i = 0; i < g_Toasts.size();)
    {
        float age = now - g_Toasts[i].born;
        if (age > life) { g_Toasts.erase(g_Toasts.begin() + i); continue; }

        float alpha = ImClamp(ImMin(age * 6.0f, (life - age) * 3.0f), 0.0f, 1.0f);
        ImVec2 ts = ImGui::CalcTextSize(g_Toasts[i].msg.c_str());
        ImVec2 size(ts.x + 30.0f, ts.y + 14.0f);
        ImVec2 pos(display.x / 2 - size.x / 2, y - (1.0f - alpha) * 10.0f);

        ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg); bg.w = 0.92f * alpha;
        ImVec4 bd = ImGui::GetStyleColorVec4(ImGuiCol_Border);  bd.w = alpha;
        ImVec4 tx = ImGui::GetStyleColorVec4(ImGuiCol_Text);    tx.w = alpha;
        ImVec4 ac = accent; ac.w = alpha;
        dl->AddRectFilled(pos, pos + size, ImGui::GetColorU32(bg), 6.0f);
        dl->AddRect(pos, pos + size, ImGui::GetColorU32(bd), 6.0f);
        dl->AddCircleFilled(pos + ImVec2(12.0f, size.y / 2), 3.5f, ImGui::GetColorU32(ac));
        dl->AddText(pos + ImVec2(22.0f, 7.0f), ImGui::GetColorU32(tx), g_Toasts[i].msg.c_str());

        y += size.y + 6.0f;
        ++i;
    }
}

// ---------------------------------------------------------------- theme

static void ApplyTheme()
{
    if (g_State.theme < 0 || g_State.theme >= kThemeCount)
        g_State.theme = 0;
    const Theme& t = kThemes[g_State.theme];
    ImGuiStyle& style = ImGui::GetStyle();
    auto C = [](unsigned hex, float a) { return W::HexToColorVec4(hex, a); };

    style.Colors[ImGuiCol_WindowBg]             = C(t.bg, 1.0f);
    style.Colors[ImGuiCol_PopupBg]              = C(t.popup, 1.0f);
    style.Colors[ImGuiCol_ChildBg]              = C(t.child, 1.0f);
    style.Colors[ImGuiCol_Text]                 = C(t.text, 1.0f);
    style.Colors[ImGuiCol_CheckMark]            = C(t.text, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]         = C(t.disabled, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg]       = C(t.accent, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]           = C(t.accent, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]     = C(t.accentDark, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ScrollbarGrab]        = C(t.child, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = C(t.child, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = C(t.child, 1.0f);
    style.Colors[ImGuiCol_Border]               = C(t.popup, 1.0f);
    style.Colors[ImGuiCol_Separator]            = C(t.popup, 1.0f);
    style.Colors[ImGuiCol_Button]               = C(t.bg, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]        = C(t.bg, 0.7f);
    style.Colors[ImGuiCol_ButtonActive]         = C(t.bg, 0.5f);
    style.Colors[ImGuiCol_FrameBg]              = C(t.bg, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]       = C(t.bg, 0.7f);
    style.Colors[ImGuiCol_FrameBgActive]        = C(t.bg, 0.5f);
    style.Colors[ImGuiCol_Header]               = C(t.child, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]        = C(t.child, 0.7f);
    style.Colors[ImGuiCol_HeaderActive]         = C(t.child, 0.5f);
}

// ---------------------------------------------------------------- watermark

static const char* kWatermark = "Cluado | v6.6.6";

// Centered box at the top with fixed text (so it never changes size), and a thin white bar
// that glides left to right, leaves on the right and comes back in from the left.
static void RenderWatermark()
{
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = g_Fonts.Big;
    const float size = font->LegacySize;
    static ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, kWatermark);
    const ImVec2 pad(16.0f, 8.0f);
    const ImVec2 box(ImFloor(ts.x + pad.x * 2), ImFloor(ts.y + pad.y * 2));
    const ImVec2 pos(ImFloor(ImGui::GetIO().DisplaySize.x / 2 - box.x / 2), 14.0f);

    dl->AddRectFilled(pos, pos + box, IM_COL32(38, 38, 42, 235), 4.0f);
    dl->AddRect(pos, pos + box, IM_COL32(70, 70, 76, 255), 4.0f);
    dl->AddText(font, size, pos + pad, IM_COL32(245, 245, 245, 255), kWatermark);

    // Sub-pixel position from real time, so the speed is the same at any frame rate.
    const float barW = box.x * 0.28f;
    const float period = 2.4f;  // seconds per pass
    const float p = fmodf((float)ImGui::GetTime(), period) / period;
    const float x = pos.x - barW + p * (box.x + barW);
    dl->PushClipRect(pos, pos + box, true);
    dl->AddRectFilled(ImVec2(x, pos.y + box.y - 2.0f), ImVec2(x + barW, pos.y + box.y), IM_COL32(255, 255, 255, 235));
    dl->PopClipRect();

    g_WatermarkBottom = pos.y + box.y;
}

// ---------------------------------------------------------------- ESP / crosshair

static ImU32 EspColor(int kind)
{
    switch (kind)
    {
    case Esp_Granny:    return IM_COL32(255, 70, 70, 255);
    case Esp_Grandpa:   return IM_COL32(255, 160, 50, 255);
    case Esp_Slendrina: return IM_COL32(120, 220, 120, 255);
    case Esp_Spider:    return IM_COL32(200, 120, 255, 255);
    case Esp_Robot:     return IM_COL32(80, 200, 255, 255);
    default:            return IM_COL32(255, 255, 255, 255);
    }
}

static const char* EspName(int kind)
{
    switch (kind)
    {
    case Esp_Granny:    return "Granny";
    case Esp_Grandpa:   return "Grandpa";
    case Esp_Slendrina: return "Slendrina";
    case Esp_Spider:    return "Spider";
    case Esp_Robot:     return "Robot";
    default:            return "Enemy";
    }
}

static void RenderEsp()
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 display = ImGui::GetIO().DisplaySize;
    int count = ImClamp((int)g_State.espCount, 0, MAX_ESP);

    // Unity renders at Screen.width x Screen.height, which can differ from the window size.
    float sx = g_State.screenW > 0 ? display.x / g_State.screenW : 1.0f;
    float sy = g_State.screenH > 0 ? display.y / g_State.screenH : 1.0f;

    for (int i = 0; i < count; ++i)
    {
        const EspEntry e = g_State.esp[i];
        if (!e.onScreen)
            continue;

        ImU32 col = EspColor(e.kind);
        ImVec2 tl(e.minX * sx, e.minY * sy), br(e.maxX * sx, e.maxY * sy);
        if (br.x - tl.x < 2.0f || br.y - tl.y < 2.0f)
            continue;

        if (g_State.espFilled)
            dl->AddRectFilled(tl, br, (col & 0x00FFFFFF) | (40u << 24), 2.0f);
        if (g_State.espBoxes)
        {
            dl->AddRect(tl - ImVec2(1, 1), br + ImVec2(1, 1), IM_COL32(0, 0, 0, 160), 2.0f, 0, 3.0f);
            dl->AddRect(tl, br, col, 2.0f, 0, 1.5f);
        }
        if (g_State.espCorner)
        {
            float lx = (br.x - tl.x) * 0.25f, ly = (br.y - tl.y) * 0.2f;
            const ImVec2 pts[4] = { tl, ImVec2(br.x, tl.y), br, ImVec2(tl.x, br.y) };
            const float dx[4] = { lx, -lx, -lx, lx }, dy[4] = { ly, ly, -ly, -ly };
            for (int k = 0; k < 4; ++k)
            {
                dl->AddLine(pts[k], pts[k] + ImVec2(dx[k], 0), col, 2.0f);
                dl->AddLine(pts[k], pts[k] + ImVec2(0, dy[k]), col, 2.0f);
            }
        }
        if (g_State.esp3D && g_State.espCornersValid[i])
        {
            ImVec2 c[8];
            for (int k = 0; k < 8; ++k)
                c[k] = ImVec2(g_State.espCorners[i][k * 2] * sx, g_State.espCorners[i][k * 2 + 1] * sy);
            // Edges join corners whose index differs in exactly one bit.
            for (int a = 0; a < 8; ++a)
                for (int bit = 1; bit < 8; bit <<= 1)
                    if (!(a & bit))
                        dl->AddLine(c[a], c[a | bit], col, 1.2f);
        }
        if (g_State.espHeadDot)
        {
            float r = ImClamp((br.x - tl.x) * 0.18f, 2.5f, 8.0f);
            dl->AddCircleFilled(ImVec2((tl.x + br.x) / 2, tl.y + r + 1), r, col);
        }
        if (g_State.espLines)
            dl->AddLine(ImVec2(display.x / 2, display.y), ImVec2((tl.x + br.x) / 2, br.y), col, 1.2f);

        char name[ESP_LABEL_LEN];
        memcpy(name, g_State.espLabel[i], ESP_LABEL_LEN);
        name[ESP_LABEL_LEN - 1] = 0;
        const char* shown = name[0] ? name : EspName(e.kind);

        char label[64] = "";
        if (g_State.espNames && g_State.espDistance)
            snprintf(label, sizeof(label), "%s [%.0fm]", shown, e.distance);
        else if (g_State.espNames)
            snprintf(label, sizeof(label), "%s", shown);
        else if (g_State.espDistance)
            snprintf(label, sizeof(label), "%.0fm", e.distance);

        if (label[0])
        {
            ImVec2 ts = ImGui::CalcTextSize(label);
            ImVec2 p((tl.x + br.x) / 2 - ts.x / 2, tl.y - ts.y - 2);
            dl->AddText(p + ImVec2(1, 1), IM_COL32(0, 0, 0, 200), label);
            dl->AddText(p, col, label);
        }
    }
}

static void RenderCrosshair()
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 c = ImGui::GetIO().DisplaySize / 2;
    ImU32 col = ImGui::GetColorU32(ImGuiCol_SliderGrab);
    const float len = 6.0f, gap = 3.0f;
    dl->AddLine(c - ImVec2(len + gap, 0), c - ImVec2(gap, 0), col, 2.0f);
    dl->AddLine(c + ImVec2(gap, 0), c + ImVec2(len + gap, 0), col, 2.0f);
    dl->AddLine(c - ImVec2(0, len + gap), c - ImVec2(0, gap), col, 2.0f);
    dl->AddLine(c + ImVec2(0, gap), c + ImVec2(0, len + gap), col, 2.0f);
}

// ---------------------------------------------------------------- widgets on shared ints

static bool Toggle(const char* label, int32_t* v, const char* notifyName = nullptr)
{
    bool b = *v != 0;
    bool pressed = W::ToggleButtonClassic(label, &b);
    if (pressed)
    {
        *v = b ? 1 : 0;
        if (notifyName)
            Notify(std::string(notifyName) + (b ? " enabled" : " disabled"));
    }
    return pressed;
}

static bool ActionButton(const char* label, int32_t* action, const char* toast = nullptr, float width = -0.1f)
{
    if (W::Button(label, ImVec2(width, 0)))
    {
        *action = 1;
        if (toast)
            Notify(toast);
        return true;
    }
    return false;
}

// Titled bordered section like the Nerlz tabs.
static void BeginSection(const char* title, float height = 0.0f)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemInnerSpacing);
    ImGui::PushFont(g_Fonts.Big, 0.0f);
    ImGui::TextDisabled("%s", title);
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4());
    ImGui::BeginChild(title, ImVec2(0, height), ImGuiChildFlags_Borders | (height == 0.0f ? ImGuiChildFlags_AutoResizeY : 0));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

static void EndSection()
{
    ImGui::EndChild();
    ImGui::Spacing();
}

static void NotInGame()
{
    if (!g_State.inGame)
        ImGui::TextDisabled(ICON_FA_CIRCLE_INFO "  Start a game to use everything here.");
}

// ---------------------------------------------------------------- pages

static void PlayerPage()
{
    BeginSection("Survival");
    Toggle("God mode", &g_State.godMode, "God mode");
    ImGui::TextDisabled("Traps, falls and enemies can't kill you.");
    Toggle("Invisible", &g_State.invisible, "Invisible");
    ImGui::TextDisabled("Granny and Grandpa never notice you.");
    EndSection();

    BeginSection("Position");
    ImGui::Text("X %.1f   Y %.1f   Z %.1f", g_State.posX, g_State.posY, g_State.posZ);
    float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
    ActionButton(ICON_FA_FLOPPY_DISK "  Save position", &g_State.actSavePos, "Position saved", half);
    ImGui::SameLine();
    ImGui::BeginDisabled(!g_State.hasSavedPos);
    ActionButton(ICON_FA_LOCATION_ARROW "  Teleport back", &g_State.actLoadPos, "Teleported", half);
    ImGui::EndDisabled();
    EndSection();
    NotInGame();
}

static void MovementPage()
{
    BeginSection("Speed");
    W::SliderFloat("Walk speed", &g_State.speedMult, 0.5f, 5.0f, -0.1f, "x%.1f");
    if (W::Button("Reset speed", ImVec2(-0.1f, 0)))
        g_State.speedMult = 1.0f;
    EndSection();

    BeginSection("Noclip");
    Toggle("Noclip / fly", &g_State.noclip, "Noclip");
    W::SliderFloat("Fly speed", &g_State.noclipSpeed, 2.0f, 30.0f, -0.1f, "%.0f");
    ImGui::TextDisabled("WASD move  |  Space up  |  Ctrl down  |  Shift fast");
    EndSection();

    BeginSection("Time");
    Toggle("Custom time scale", &g_State.timeScaleEnabled, "Time scale");
    W::SliderFloat("Time scale", &g_State.timeScale, 0.1f, 3.0f, -0.1f, "x%.2f");
    EndSection();
}

static void EnemiesPage()
{
    BeginSection("Control");
    Toggle("Freeze all enemies", &g_State.freezeEnemies, "Freeze enemies");
    ImGui::TextDisabled("Granny, Grandpa, Slendrina, the spider and the robot stop moving.");
    EndSection();

    BeginSection("Orbit");
    Toggle("Orbit around me", &g_State.orbitEnabled, "Orbit");
    ImGui::TextDisabled("Every enemy freezes and spins around you like a carousel, and follows you.");
    W::SliderFloat("Orbit radius", &g_State.orbitRadius, 1.0f, 15.0f, -0.1f, "%.1f");
    W::SliderFloat("Orbit speed", &g_State.orbitSpeed, -5.0f, 5.0f, -0.1f, "%.1f");
    EndSection();

    BeginSection("Knock out");
    float third = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3;
    ActionButton("Granny", &g_State.actKnockGranny, "Granny knocked out", third);
    ImGui::SameLine();
    ActionButton("Grandpa", &g_State.actKnockGrandpa, "Grandpa knocked out", third);
    ImGui::SameLine();
    ActionButton("Both", &g_State.actKnockAll, "Both knocked out", third);
    ImGui::TextDisabled("Same as hitting them - they wake up again after a while.");
    EndSection();
    NotInGame();
}

static void VisualsPage()
{
    BeginSection("Camera");
    Toggle("Custom FOV", &g_State.fovEnabled, "Custom FOV");
    W::SliderFloat("Field of view", &g_State.fov, 30.0f, 150.0f, -0.1f, "%.0f");
    EndSection();

    BeginSection("Enemy ESP");
    Toggle("Enabled", &g_State.espEnabled, "ESP");
    ImGui::BeginDisabled(!g_State.espEnabled);
    if (ImGui::TreeNodeEx("2D Box", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        Toggle("Box", &g_State.espBoxes);
        ImGui::SameLine(170);
        Toggle("Filled", &g_State.espFilled);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Corner box", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        Toggle("Corner box##c", &g_State.espCorner);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("3D Box", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        Toggle("3D box##3", &g_State.esp3D);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Tracers", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        Toggle("Snaplines", &g_State.espLines);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Nametag", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        Toggle("Names", &g_State.espNames);
        ImGui::SameLine(170);
        Toggle("Distance", &g_State.espDistance);
        Toggle("Head dot", &g_State.espHeadDot);
        ImGui::TreePop();
    }
    ImGui::EndDisabled();
    EndSection();

    BeginSection("Overlay");
    Toggle("Watermark", &g_State.watermark);
    ImGui::SameLine(170);
    Toggle("Crosshair", &g_State.crosshair);
    Toggle("Notifications", &g_State.notifications);
    EndSection();
}

static void WorldPage()
{
    BeginSection("Lighting");
    Toggle("Fullbright", &g_State.fullbright, "Fullbright");
    ImGui::TextDisabled("No darkness, no fog, plus a light that follows you.");
    EndSection();

    BeginSection("Days");
    ImGui::Text("Current day: %.0f", g_State.currentDay);
    float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
    ActionButton(ICON_FA_MINUS "  Previous day", &g_State.actDayMinus, nullptr, half);
    ImGui::SameLine();
    ActionButton(ICON_FA_PLUS "  Next day", &g_State.actDayPlus, nullptr, half);
    EndSection();
    NotInGame();
}

static void ItemsPage()
{
    static char filter[48] = "";
    BeginSection("Search");
    ImGui::SetNextItemWidth(-0.1f);
    ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS "  Search items...", filter, sizeof(filter));
    EndSection();

    BeginSection("Items", ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2);
    int count = ImClamp((int)g_State.itemCount, 0, MAX_ITEMS);
    if (count == 0)
        ImGui::TextDisabled("Start a game to see items.");

    float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
    int shown = 0;
    for (int i = 0; i < count; ++i)
    {
        char name[ITEM_NAME_LEN];
        memcpy(name, g_State.items[i], ITEM_NAME_LEN);
        name[ITEM_NAME_LEN - 1] = 0;

        if (filter[0])
        {
            std::string a = name, b = filter;
            for (auto& c : a) c = (char)tolower((unsigned char)c);
            for (auto& c : b) c = (char)tolower((unsigned char)c);
            if (a.find(b) == std::string::npos)
                continue;
        }

        if (shown++ % 2) ImGui::SameLine();
        ImGui::PushID(i);
        if (W::Button(name, ImVec2(half, 0)))
        {
            g_State.actGiveItem = i + 1;
            Notify(std::string("Took ") + name);
        }
        ImGui::PopID();
    }
    EndSection();
}

static void ShopPage()
{
    BeginSection("Coins");
    char key[48];
    memcpy(key, g_State.coinKey, sizeof(key));
    key[sizeof(key) - 1] = 0;

    if (g_State.coins >= 0)
    {
        ImGui::Text(ICON_FA_COINS "  %.1f coins", g_State.coinValue);
        ImGui::TextDisabled("Save key: %s (%s%s)", key, g_State.coinKeyType == 2 ? "decimal" : "whole number",
                            g_State.coinKeyConfirmed ? "" : ", guessed - open a shop once to confirm");
    }
    else
        ImGui::TextDisabled("Coins not found yet - open the main menu or the toy capsule shop once.");

    ImGui::BeginDisabled(g_State.coins < 0);
    float q = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 3) / 4;
    const int amounts[4] = { 10, 100, 1000, 100000 };
    const char* labels[4] = { "+10", "+100", "+1K", "+100K" };
    for (int i = 0; i < 4; ++i)
    {
        if (i) ImGui::SameLine();
        if (W::Button(labels[i], ImVec2(q, 0)))
        {
            g_State.actAddCoins = amounts[i];
            Notify(std::string("Added ") + labels[i] + " coins");
        }
    }
    ImGui::EndDisabled();
    EndSection();

    BeginSection("Unlocks");
    ImGui::BeginDisabled(!g_State.inMainMenu);
    ActionButton(ICON_FA_UNLOCK "  Unlock everything (main menu)", &g_State.actUnlockAll, "Unlocking everything...");
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!g_State.inCapsuleShop);
    ActionButton(ICON_FA_PALETTE "  Unlock all item colours (toy capsule)", &g_State.actUnlockSkins, "Unlocking colours...");
    ImGui::EndDisabled();
    if (!g_State.inMainMenu && !g_State.inCapsuleShop)
        ImGui::TextDisabled("Go to the main menu or stand at the toy capsule machine.");
    EndSection();
}

static void ThemesPage()
{
    BeginSection("Themes");
    Toggle("Menu Particles", &g_State.menuParticles);
    ImGui::Spacing();

    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const int perRow = 6;
    const float tileW = ImFloor((ImGui::GetContentRegionAvail().x - gap * (perRow - 1)) / perRow);
    const ImVec2 tile(tileW, ImFloor(tileW * 1.3f));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int i = 0; i < kThemeCount; ++i)
    {
        if (i % perRow) ImGui::SameLine(0, gap);
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##theme", tile))
        {
            g_State.theme = i;
            g_State.actSaveConfig = 1;
        }
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        float hover = ImGui::IsItemHovered() ? 1.0f : 0.0f;
        dl->AddRectFilled(mn, mx, ImGui::GetColorU32(W::HexToColorVec4(kThemes[i].accent, 1.0f - 0.15f * hover)), 4.0f);
        dl->AddRectFilled(ImVec2(mn.x, mx.y - tile.y * 0.22f), mx, ImGui::GetColorU32(W::HexToColorVec4(kThemes[i].bg, 0.9f)), 4.0f, ImDrawFlags_RoundCornersBottom);
        if (g_State.theme == i)
            dl->AddRect(mn - ImVec2(2, 2), mx + ImVec2(2, 2), IM_COL32(255, 255, 255, 255), 5.0f, 0, 2.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", kThemes[i].name);
        ImGui::PopID();
    }
    ImGui::TextDisabled("Current: %s", kThemes[ImClamp((int)g_State.theme, 0, kThemeCount - 1)].name);
    EndSection();
}

static void AboutPage()
{
    BeginSection("Granny Legacy Mod");
    ImGui::Text("Version 0.9.0  |  Menu: Delete  |  Enemy wheel: hold middle mouse");
    ImGui::TextDisabled("Dear ImGui %s  |  DirectX 11", IMGUI_VERSION);
    ImGui::TextDisabled("Settings save by themselves when you close the menu.");
    EndSection();

    BeginSection("Contact");
    {
        // Discord-blue button with the Discord logo.
        const ImVec2 size(ImGui::GetContentRegionAvail().x, 34.0f);
        if (ImGui::InvisibleButton("##discord", size))
            ShellExecuteW(nullptr, L"open", L"https://discordapp.com/users/317497421691355136", nullptr, nullptr, SW_SHOWNORMAL);
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        bool hot = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(mn, mx, hot ? IM_COL32(110, 124, 255, 255) : IM_COL32(88, 101, 242, 255), 6.0f);

        const char* text = "Discord  -  nerlz";
        const float iconSize = 18.0f;
        ImVec2 ts = ImGui::CalcTextSize(text);
        float total = iconSize + 8.0f + ts.x;
        ImVec2 p(mn.x + (size.x - total) / 2, mn.y + (size.y - iconSize) / 2);
        dl->AddText(g_Fonts.Brands, iconSize, p, IM_COL32_WHITE, ICON_FA_DISCORD);
        dl->AddText(ImVec2(p.x + iconSize + 8.0f, mn.y + (size.y - ts.y) / 2), IM_COL32_WHITE, text);
        if (hot)
            ImGui::SetTooltip("Open nerlz on Discord");
    }
    EndSection();

    BeginSection("Diagnostics");
    char debug[DEBUG_LEN];
    memcpy(debug, g_State.debug, DEBUG_LEN);
    debug[DEBUG_LEN - 1] = 0;
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("%s", debug[0] ? debug : "Waiting for the plugin...");
    ImGui::PopTextWrapPos();
    EndSection();
}

static void ConsolePage()
{
#ifndef STANDALONE
    BeginSection("Tools");
    const char* label = g_State.explorerStatus == 2 ? ICON_FA_CAMERA "  Close Cinematic Unity Explorer"
                                                    : ICON_FA_CAMERA "  Open Cinematic Unity Explorer";
    ImGui::BeginDisabled(g_State.explorerStatus == 0);
    if (W::Button(label, ImVec2(-0.1f, 0)))
        g_State.actToggleExplorer = 1;
    ImGui::EndDisabled();
    if (g_State.explorerStatus == 0)
        ImGui::TextDisabled("Not installed - put CinematicUnityExplorer in BepInEx/plugins.");
    else
        ImGui::TextDisabled("Opens only from here (F7 is turned off). Clicks outside this menu go to the explorer.");
    EndSection();
#endif

    BeginSection("Log", ImGui::GetContentRegionAvail().y - 4.0f);
    int count = ImClamp((int)g_State.consoleCount, 0, CONSOLE_LINES);
    if (count == 0)
        ImGui::TextDisabled("Nothing logged yet.");
    for (int i = 0; i < count; ++i)
    {
        char line[CONSOLE_LINE_LEN];
        memcpy(line, g_State.console[i], CONSOLE_LINE_LEN);
        line[CONSOLE_LINE_LEN - 1] = 0;
        ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        if (strstr(line, "[Error") || strstr(line, "[Fatal")) col = ImVec4(1.0f, 0.42f, 0.42f, 1);
        else if (strstr(line, "[Warning")) col = ImVec4(1.0f, 0.8f, 0.35f, 1);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextWrapped("%s", line);
        ImGui::PopStyleColor();
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f)
        ImGui::SetScrollHereY(1.0f);
    EndSection();
}

// ---------------------------------------------------------------- enemy wheel

enum RadialAction : int
{
    Radial_None, Radial_Freeze, Radial_Unfreeze, Radial_KnockOut, Radial_Pepper, Radial_Ice,
    Radial_TeleportTo, Radial_Bring, Radial_Fling, Radial_Despawn, Radial_RespawnAll, Radial_COUNT
};

static const char* kRadialLabels[Radial_COUNT] = {
    "", "Freeze", "Unfreeze", "Knock out", "Pepper", "Ice", "Teleport", "Bring", "Fling", "Despawn", "Respawn"
};

static const int kRadialKey = VK_MBUTTON;
static bool g_RadialHeld;
static int g_RadialHovered;
static int g_RadialClickPick;  // bubble pressed with the left button, applied on release
static ImVec2 g_RadialCenter;

static int FindEspById(int id)
{
    int count = ImClamp((int)g_State.espCount, 0, MAX_ESP);
    for (int i = 0; i < count; ++i)
        if (g_State.espId[i] == id)
            return i;
    return -1;
}

static ImVec2 EspCenter(int i, ImVec2 display)
{
    float sx = g_State.screenW > 0 ? display.x / g_State.screenW : 1.0f;
    float sy = g_State.screenH > 0 ? display.y / g_State.screenH : 1.0f;
    const EspEntry& e = g_State.esp[i];
    return ImVec2((e.minX + e.maxX) * 0.5f * sx, (e.minY + e.maxY) * 0.5f * sy);
}

// Hold the middle mouse button while looking at an enemy: a ring of actions appears around it. Release over one (or click it).
void Radial_Update(HWND window)
{
    bool down = !g_State.menuOpen && GetForegroundWindow() == window && (GetAsyncKeyState(kRadialKey) & 0x8000);
    ImVec2 display = ImGui::GetIO().DisplaySize;

    if (down && !g_RadialHeld)
    {
        // Pick the on-screen enemy closest to the crosshair.
        int best = -1;
        float bestD = display.y * 0.35f;
        int count = ImClamp((int)g_State.espCount, 0, MAX_ESP);
        for (int i = 0; i < count; ++i)
        {
            if (!g_State.esp[i].onScreen)
                continue;
            ImVec2 d = EspCenter(i, display) - display * 0.5f;
            float len = sqrtf(d.x * d.x + d.y * d.y);
            if (len < bestD) { bestD = len; best = i; }
        }
        // With no enemy nearby the wheel still opens in the middle, so "Respawn" stays reachable.
        g_State.radialTarget = best >= 0 ? g_State.espId[best] : 0;
        g_RadialCenter = best >= 0 ? EspCenter(best, display) : display * 0.5f;
        g_State.radialOpen = 1;
        g_RadialHovered = 0;
        g_RadialClickPick = 0;

        // Put the cursor in the middle and tell ImGui right away, so the first frame doesn't
        // hover whatever bubble the cursor was over last time.
        POINT pt = { (LONG)(display.x / 2), (LONG)(display.y / 2) };
        ClientToScreen(window, &pt);
        SetCursorPos(pt.x, pt.y);
        ImGui::GetIO().AddMousePosEvent(display.x * 0.5f, display.y * 0.5f);
    }
    else if (!down && g_RadialHeld && g_State.radialOpen)
    {
        if (g_RadialHovered > 0)
            g_State.radialAction = g_RadialHovered;
        g_State.radialOpen = 0;
    }
    g_RadialHeld = down;
}

static void RenderRadial()
{
    if (!g_State.radialOpen)
        return;

    ImVec2 display = ImGui::GetIO().DisplaySize;
    int idx = g_State.radialTarget != 0 ? FindEspById(g_State.radialTarget) : -1;
    if (idx >= 0 && g_State.esp[idx].onScreen)
        g_RadialCenter = EspCenter(idx, display);

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 c = g_RadialCenter;
    const int n = Radial_COUNT - 1;
    const float bubble = 36.0f;
    const float radius = ImMax(150.0f, bubble * n / 3.0f + 20.0f);
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    g_RadialHovered = 0;
    for (int k = 0; k < n; ++k)
    {
        float a = -IM_PI / 2 + k * (2 * IM_PI / n);
        ImVec2 p = c + ImVec2(cosf(a), sinf(a)) * radius;
        ImVec2 d = mouse - p;
        bool hovered = d.x * d.x + d.y * d.y <= bubble * bubble;
        if (hovered)
            g_RadialHovered = k + 1;

        float r = hovered ? bubble * 1.12f : bubble;
        dl->AddCircleFilled(p, r, hovered ? IM_COL32(255, 60, 60, 245) : IM_COL32(225, 25, 25, 230), 40);
        dl->AddCircle(p, r, IM_COL32(255, 255, 255, hovered ? 200 : 60), 40, 1.5f);
        const char* label = kRadialLabels[k + 1];
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(p - ts * 0.5f, IM_COL32(255, 255, 255, 255), label);
    }

    // Pick on release, so ImGui sees the button go up before the wheel closes (otherwise the
    // left button stays "held" inside ImGui and the next click is lost).
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        g_RadialClickPick = g_RadialHovered;
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && g_RadialClickPick > 0)
    {
        if (g_RadialClickPick == g_RadialHovered)
            g_State.radialAction = g_RadialClickPick;
        g_RadialClickPick = 0;
        g_State.radialOpen = 0;
    }

    if (idx < 0 && g_State.radialTarget == 0)
    {
        const char* hint = "No enemy here";
        ImVec2 ts = ImGui::CalcTextSize(hint);
        dl->AddText(c - ts * 0.5f, IM_COL32(255, 255, 255, 200), hint);
    }
    else if (idx >= 0)
    {
        char name[ESP_LABEL_LEN];
        memcpy(name, g_State.espLabel[idx], ESP_LABEL_LEN);
        name[ESP_LABEL_LEN - 1] = 0;
        ImVec2 ts = ImGui::CalcTextSize(name);
        dl->AddText(c - ts * 0.5f + ImVec2(1, 1), IM_COL32(0, 0, 0, 200), name);
        dl->AddText(c - ts * 0.5f, IM_COL32(255, 255, 255, 255), name);
    }
}

// ---------------------------------------------------------------- menu particles

struct Particle { ImVec2 pos, vel; };
static Particle g_Particles[48];
static bool g_ParticlesInit;

static void RenderParticles(ImDrawList* dl, ImVec2 origin, ImVec2 size)
{
    if (!g_ParticlesInit)
    {
        unsigned seed = 1234567u;
        auto rnd = [&]() { seed = seed * 1664525u + 1013904223u; return (seed >> 8) / 16777216.0f; };
        for (auto& p : g_Particles)
        {
            p.pos = ImVec2(rnd() * size.x, rnd() * size.y);
            p.vel = ImVec2((rnd() - 0.5f) * 30.0f, (rnd() - 0.5f) * 30.0f);
        }
        g_ParticlesInit = true;
    }

    const float dt = ImGui::GetIO().DeltaTime;
    ImVec4 acc = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
    for (auto& p : g_Particles)
    {
        p.pos += p.vel * dt;
        if (p.pos.x < 0 || p.pos.x > size.x) p.vel.x = -p.vel.x;
        if (p.pos.y < 0 || p.pos.y > size.y) p.vel.y = -p.vel.y;
        p.pos = ImClamp(p.pos, ImVec2(0, 0), size);
    }

    dl->PushClipRect(origin, origin + size, true);
    const int n = IM_ARRAYSIZE(g_Particles);
    for (int a = 0; a < n; ++a)
    {
        for (int b = a + 1; b < n; ++b)
        {
            ImVec2 d = g_Particles[a].pos - g_Particles[b].pos;
            float dist2 = d.x * d.x + d.y * d.y;
            if (dist2 < 70.0f * 70.0f)
            {
                ImVec4 c = acc; c.w = 0.25f * (1.0f - sqrtf(dist2) / 70.0f);
                dl->AddLine(origin + g_Particles[a].pos, origin + g_Particles[b].pos, ImGui::GetColorU32(c), 1.0f);
            }
        }
        ImVec4 c = acc; c.w = 0.6f;
        dl->AddCircleFilled(origin + g_Particles[a].pos, 1.8f, ImGui::GetColorU32(c));
    }
    dl->PopClipRect();
}

// ---------------------------------------------------------------- main window (layout from CGui::PreRender)

static void RenderMenu()
{
    ImGuiStyle& style = ImGui::GetStyle();
    const float sideBarWidth = 144;
    const ImVec2 menuSize(600, 520);

    ImGui::SetNextWindowSize(menuSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize / 2 - menuSize / 2, ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##granny_menu", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar(2);

    {
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 sz = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, pos + ImVec2(sideBarWidth, sz.y),
                          ImGui::GetColorU32(ImGuiCol_WindowBg), style.WindowRounding, ImDrawFlags_RoundCornersLeft);
        dl->AddRectFilled(pos + ImVec2(sideBarWidth, 0), pos + sz,
                          ImGui::GetColorU32(ImGuiCol_PopupBg), style.WindowRounding, ImDrawFlags_RoundCornersRight);
        if (g_State.menuParticles)
            RenderParticles(dl, pos, sz);
    }

    ImGui::BeginChild("SideBar", ImVec2(sideBarWidth, 0), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4());
        ImGui::BeginChild("LogoRect", ImVec2(0, ImGui::GetFontSize() + style.WindowPadding.y * 2), ImGuiChildFlags_Borders);
        ImGui::PopStyleColor();
        {
            // "Cluado" with a smaller "by nerlz" sitting a little lower, like a signature.
            const char* logo = "Cluado";
            const char* sub = "by nerlz";
            const float subSize = ImGui::GetFontSize() * 0.72f;
            ImGui::PushFont(nullptr, subSize);
            float subW = ImGui::CalcTextSize(sub).x;
            ImGui::PopFont();
            float w = ImGui::CalcTextSize(logo).x + 4.0f + subW;
            float x = ImGui::GetWindowWidth() / 2 - w / 2;
            ImGui::SetCursorPos({ x, style.WindowPadding.y });
            ImGui::TextColored(style.Colors[ImGuiCol_Text], "%s", logo);
            ImGui::SameLine(0, 4.0f);
            ImGui::SetCursorPosY(style.WindowPadding.y + ImGui::GetFontSize() - subSize + 3.0f);
            ImGui::PushFont(nullptr, subSize);
            ImGui::TextDisabled("%s", sub);
            ImGui::PopFont();
        }
        ImGui::EndChild();

        ImVec2 radioSize(ImGui::GetWindowWidth() - style.WindowPadding.x * 2, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.ChildRounding);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, style.Colors[ImGuiCol_PopupBg]);

        auto Tab = [&](int id) { W::RadioFrameIconText(kPages[id].name, kPages[id].icon, &g_Page, id, g_Fonts.Icons, radioSize); };
        W::SeparatorText("CHEATS");
        Tab(Page_Player);
        Tab(Page_Movement);
        Tab(Page_Enemies);
        Tab(Page_Visuals);
        Tab(Page_World);
        Tab(Page_Items);
        Tab(Page_Shop);
        W::SeparatorText("SETTINGS");
        Tab(Page_Themes);
        Tab(Page_About);
        W::SeparatorText("DEVELOPER");
        Tab(Page_Console);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::SameLine(sideBarWidth);
    ImGui::BeginChild("Content", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoBackground);
    {
        const float iconSz = ImGui::GetFontSize();
        const float bigSz = g_Fonts.Big->LegacySize;
        const float iconChildSz = iconSz + bigSz + style.ItemInnerSpacing.y * 3;
        ImGui::BeginChild("PageIcon", ImVec2(iconChildSz, iconChildSz));
        {
            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2 - ImVec2(iconSz, iconSz) / 2);
            ImGui::PushFont(g_Fonts.Icons, iconSz);
            ImGui::TextColored(style.Colors[ImGuiCol_SliderGrab], "%s", kPages[g_Page].icon);
            ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginGroup();
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemInnerSpacing);
            ImGui::Spacing();
            ImGui::PushFont(g_Fonts.Big, bigSz);
            ImGui::Text("%s", kPages[g_Page].name);
            ImGui::PopFont();
            ImGui::TextDisabled("%s", kPages[g_Page].desc);
            ImGui::PopStyleVar();
        }
        ImGui::EndGroup();
        ImGui::Spacing();

        switch (g_Page)
        {
        case Page_Player:   PlayerPage();   break;
        case Page_Movement: MovementPage(); break;
        case Page_Enemies:  EnemiesPage();  break;
        case Page_Visuals:  VisualsPage();  break;
        case Page_World:    WorldPage();    break;
        case Page_Items:    ItemsPage();    break;
        case Page_Shop:     ShopPage();     break;
        case Page_Themes:   ThemesPage();   break;
        case Page_About:    AboutPage();    break;
        case Page_Console:  ConsolePage();  break;
        default: break;
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

// ---------------------------------------------------------------- lifecycle

bool Menu_Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGuiStyle& style = ImGui::GetStyle();
    style.TabRounding = 4;
    style.ScrollbarRounding = 9;
    style.WindowRounding = 7;
    style.GrabRounding = 3;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ChildRounding = 4;
    ApplyTheme();

    static const ImWchar ranges_default[] = { 0x0020, 0x00FF, 0 };
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };

    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.GlyphRanges = ranges_default;
    g_Fonts.Main = io.Fonts->AddFontFromMemoryTTF((void*)poppinsmedium, (int)sizeof(poppinsmedium), 16.0f, &cfg);

    // Merge the icons into the main font so ICON_FA_* works inside normal labels too.
    ImFontConfig merge;
    merge.MergeMode = true;
    merge.PixelSnapH = true;
    merge.FontDataOwnedByAtlas = false;
    merge.GlyphRanges = icons_ranges;
    merge.GlyphMinAdvanceX = 16.0f;
    io.Fonts->AddFontFromMemoryTTF((void*)fasolid900, (int)sizeof(fasolid900), 13.0f, &merge);

    ImFontConfig ic;
    ic.PixelSnapH = true;
    ic.FontDataOwnedByAtlas = false;
    ic.GlyphRanges = icons_ranges;
    g_Fonts.Icons = io.Fonts->AddFontFromMemoryTTF((void*)fasolid900, (int)sizeof(fasolid900), 15.0f, &ic);

    static const ImWchar brand_ranges[] = { ICON_MIN_FAB, ICON_MAX_16_FAB, 0 };
    ImFontConfig br;
    br.PixelSnapH = true;
    br.FontDataOwnedByAtlas = false;
    br.GlyphRanges = brand_ranges;
    g_Fonts.Brands = io.Fonts->AddFontFromMemoryTTF((void*)fabrands400, (int)sizeof(fabrands400), 18.0f, &br);

    ImFontConfig big;
    big.FontDataOwnedByAtlas = false;
    big.GlyphRanges = ranges_default;
    g_Fonts.Big = io.Fonts->AddFontFromMemoryTTF((void*)poppinsmedium, (int)sizeof(poppinsmedium), 18.0f, &big);

    bool ok = ImGui_ImplWin32_Init(hwnd);
    ok &= ImGui_ImplDX11_Init(device, context);
    return ok;
}

void Menu_Frame()
{
    ApplyTheme();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::GetIO().MouseDrawCursor = false;

    if (g_State.espEnabled)
        RenderEsp();
    if (g_State.crosshair && !g_State.menuOpen)
        RenderCrosshair();
    if (g_State.watermark)
        RenderWatermark();
    if (g_State.menuOpen)
        RenderMenu();
    RenderRadial();
    RenderToasts();

    ImGui::Render();
}

void Menu_Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
