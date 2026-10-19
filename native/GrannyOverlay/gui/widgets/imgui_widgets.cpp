#include "imgui_widgets.h"
#include <map>
#include <string>
#include <vector>

namespace ImGui
{
namespace Widgets
{

ImVec4 HexToColorVec4(unsigned int hex_color, float alpha)
{
    ImVec4 color;
    color.x = ((hex_color >> 16) & 0xFF) / 255.0f;
    color.y = ((hex_color >> 8) & 0xFF) / 255.0f;
    color.z = (hex_color & 0xFF) / 255.0f;
    color.w = alpha;
    return color;
}

void DoubleText(ImVec4 color1, ImVec4 color2, const char* label1, const char* label2)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(std::string(label1 + std::string(label2)).c_str());
    const ImVec2 label1_size = CalcTextSize(label1, NULL, true);
    const ImVec2 label2_size = CalcTextSize(label2, NULL, true);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize), label1_size.x + label2_size.x, g.FontSize);

    const ImRect total_bb(pos, pos + size);
    ItemSize(total_bb);
    if (!ItemAdd(total_bb, id)) return;

    window->DrawList->AddText(pos, GetColorU32(color1), label1);
    window->DrawList->AddText(pos + ImVec2(size.x - CalcTextSize(label2).x, 0), GetColorU32(color2), label2);
}

void TextCustomFont(ImFont* font, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    PushFont(font);
    TextV(fmt, args);
    PopFont();
    va_end(args);
}

void SeparatorText(const char* label, float thickness)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize), label_size.x, g.FontSize);

    const ImRect total_bb(pos, pos + size);
    ItemSize(total_bb);
    if (!ItemAdd(total_bb, id)) return;

    window->DrawList->AddText(pos, GetColorU32(ImGuiCol_TextDisabled), label);
    if (thickness > 0)
        window->DrawList->AddLine(pos + ImVec2(label_size.x + style.ItemInnerSpacing.x, size.y / 2),
                                  pos + ImVec2(size.x, size.y / 2),
                                  GetColorU32(ImGuiCol_Border), thickness);
}

void VSeparator(float thickness)
{
    if (thickness <= 0) return;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(thickness, -0.1f), thickness, thickness);

    const ImRect bb(pos, pos + size);
    const ImRect bb_rect(pos + ImVec2(0, style.FramePadding.y), pos + size - ImVec2(0, style.FramePadding.y));

    ItemSize(ImVec2(thickness, 0.0f));
    if (!ItemAdd(bb, 0)) return;

    window->DrawList->AddRectFilled(bb_rect.Min, bb_rect.Max, GetColorU32(ImGuiCol_Border));
}

bool RadioFrameIcon(const char* label, ImTextureID user_texture_id, int* v, int current_id,
                    const ImVec2& size_arg, const ImVec2& adjust_icon_pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 icon_size  = ImVec2(g.FontSize, g.FontSize);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + icon_size.x + style.FramePadding.x * 3.0f,
                               g.FontSize + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool active   = *v == current_id;
    bool pressed  = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = current_id;

    ImVec4 colFrameMain  = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 colFrameNull  = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame      = (active ? colFrameMain : colFrameNull);
    ImVec4 colBorderMain = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 colBorderNull = colBorderMain; colBorderNull.w = 0.0f;
    ImVec4 colBorder     = (active ? colBorderMain : colBorderNull);
    ImVec4 colIcon       = GetStyleColorVec4(active ? ImGuiCol_SliderGrab :
                                             (hovered || held) ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    ImVec4 colLabel      = GetStyleColorVec4((hovered || held || active) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct State { ImVec4 Frame, Border, Label, Icon; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0 && s.Frame.y == 0 && s.Frame.z == 0)
        s = { colFrame, colBorder, colLabel, colIcon };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame  = ImLerp(s.Frame,  colFrame,  t);
    s.Border = ImLerp(s.Border, colBorder, t);
    s.Label  = ImLerp(s.Label,  colLabel,  t);
    s.Icon   = ImLerp(s.Icon,   colIcon,   t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(pos, pos + size, GetColorU32(s.Frame), style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + size, GetColorU32(s.Border), style.FrameRounding, 0, style.FrameBorderSize);

    if (user_texture_id)
        window->DrawList->AddImage(user_texture_id,
                                   pos + style.FramePadding + adjust_icon_pos,
                                   pos + style.FramePadding + icon_size + adjust_icon_pos,
                                   ImVec2(0,0), ImVec2(1,1), GetColorU32(s.Icon));
    if (label_size.x > 0)
        window->DrawList->AddText(pos + ImVec2(size.y, style.FramePadding.y), GetColorU32(s.Label), label);

    return pressed;
}

bool RadioFrameIconColor(const char* label, ImTextureID user_texture_id, int* v, int current_id,
                         const ImVec4& custom_icon_color, const ImVec2& size_arg,
                         const ImVec2& adjust_icon_pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 icon_size  = ImVec2(g.FontSize, g.FontSize);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + icon_size.x + style.FramePadding.x * 3.0f,
                               g.FontSize + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool active  = *v == current_id;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = current_id;

    ImVec4 colFrameMain  = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 colFrameNull  = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame      = (active ? colFrameMain : colFrameNull);
    ImVec4 colBorderMain = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 colBorderNull = colBorderMain; colBorderNull.w = 0.0f;
    ImVec4 colBorder     = (active ? colBorderMain : colBorderNull);
    ImVec4 colIcon       = custom_icon_color;
    if (hovered || held || active)
        colIcon = ImVec4(custom_icon_color.x * 1.5f, custom_icon_color.y * 1.5f,
                         custom_icon_color.z * 1.5f, custom_icon_color.w);
    ImVec4 colLabel = GetStyleColorVec4((hovered || held || active) ? ImGuiCol_SliderGrab : ImGuiCol_TextDisabled);

    struct State { ImVec4 Frame, Border, Label, Icon; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s = { colFrame, colBorder, colLabel, colIcon };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame  = ImLerp(s.Frame,  colFrame,  t);
    s.Border = ImLerp(s.Border, colBorder, t);
    s.Label  = ImLerp(s.Label,  colLabel,  t);
    s.Icon   = ImLerp(s.Icon,   colIcon,   t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(pos, pos + size, GetColorU32(s.Frame), style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + size, GetColorU32(s.Border), style.FrameRounding, 0, style.FrameBorderSize);

    if (user_texture_id)
        window->DrawList->AddImage(user_texture_id,
                                   pos + style.FramePadding + adjust_icon_pos,
                                   pos + style.FramePadding + icon_size + adjust_icon_pos,
                                   ImVec2(0,0), ImVec2(1,1), GetColorU32(s.Icon));
    if (label_size.x > 0)
        window->DrawList->AddText(pos + ImVec2(size.y, style.FramePadding.y), GetColorU32(s.Label), label);

    return pressed;
}

bool RadioFrameIconText(const char* label, const char* icon_text, int* v, int current_id,
                        ImFont* icon_font, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const float  icon_px    = (icon_font && icon_font->LegacySize > 0.0f) ? icon_font->LegacySize : g.FontSize;
    const ImVec2 icon_size  = ImVec2(icon_px, icon_px);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + icon_size.x + style.FramePadding.x * 3.0f,
                               g.FontSize + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool active  = *v == current_id;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = current_id;

    ImVec4 colFrameMain  = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 colFrameNull  = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame      = (active ? colFrameMain : colFrameNull);
    ImVec4 colBorderMain = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 colBorderNull = colBorderMain; colBorderNull.w = 0.0f;
    ImVec4 colBorder     = (active ? colBorderMain : colBorderNull);
    ImVec4 colIcon       = GetStyleColorVec4(active ? ImGuiCol_SliderGrab :
                                             (hovered || held) ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    ImVec4 colLabel      = GetStyleColorVec4((hovered || held || active) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct State { ImVec4 Frame, Border, Label, Icon; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s = { colFrame, colBorder, colLabel, colIcon };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame  = ImLerp(s.Frame,  colFrame,  t);
    s.Border = ImLerp(s.Border, colBorder, t);
    s.Label  = ImLerp(s.Label,  colLabel,  t);
    s.Icon   = ImLerp(s.Icon,   colIcon,   t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(pos, pos + size, GetColorU32(s.Frame), style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + size, GetColorU32(s.Border), style.FrameRounding, 0, style.FrameBorderSize);

    if (icon_text && *icon_text)
    {
        float icon_offset = (size.y - icon_px) * 0.5f;
        if (icon_font)
            window->DrawList->AddText(icon_font, icon_font->LegacySize, pos + ImVec2(icon_offset, icon_offset), GetColorU32(s.Icon), icon_text);
        else
            window->DrawList->AddText(pos + ImVec2(icon_offset, icon_offset), GetColorU32(s.Icon), icon_text);
    }
    if (label_size.x > 0)
        window->DrawList->AddText(pos + ImVec2(size.y + style.FramePadding.x, style.FramePadding.y), GetColorU32(s.Label), label);

    return pressed;
}

bool RadioFrame(const char* label, int* v, int current_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f,
                               g.FontSize + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool active  = *v == current_id;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = current_id;

    ImVec4 colFrameMain  = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 colFrameNull  = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame      = (active ? colFrameMain : colFrameNull);
    ImVec4 colBorderMain = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 colBorderNull = colBorderMain; colBorderNull.w = 0.0f;
    ImVec4 colBorder     = (active ? colBorderMain : colBorderNull);
    ImVec4 colLabel      = GetStyleColorVec4((hovered || held || active) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct State { ImVec4 Frame, Border, Label; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s = { colFrame, colBorder, colLabel };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame  = ImLerp(s.Frame,  colFrame,  t);
    s.Border = ImLerp(s.Border, colBorder, t);
    s.Label  = ImLerp(s.Label,  colLabel,  t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(pos, pos + size, GetColorU32(s.Frame), style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + size, GetColorU32(s.Border), style.FrameRounding, 0, style.FrameBorderSize);
    if (label_size.x > 0)
        window->DrawList->AddText(pos + style.FramePadding, GetColorU32(s.Label), label);

    return pressed;
}

bool RadioFrameColor(const char* str_id, int* v, int current_id, ImVec4 accent_color,
                     const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(str_id);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, GetFrameHeight(), GetFrameHeight());

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    bool active  = *v == current_id;
    if (pressed) *v = current_id;

    RenderNavHighlight(bb, id);

    ImVec2 bpad = active ? ImVec2(2, 2) : ImVec2(0, 0);
    window->DrawList->AddRectFilled(pos + bpad, pos + size - bpad,
                                    GetColorU32(accent_color), style.FrameRounding);
    if (active)
        window->DrawList->AddRect(pos, pos + size, GetColorU32(ImGuiCol_SliderGrab),
                                  style.FrameRounding, (ImDrawFlags)0, 1.0f);
    return pressed;
}

bool InputText(const char* label, const char* text, char* buf, size_t buf_size,
               float width, ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback, void* user_data)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 text_size  = CalcTextSize(text, NULL, true);

    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline));

    bool result   = false;
    bool has_label = label_size.x > 0;
    const float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;

    BeginGroup();
    {
        if (has_label)
        {
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, style.ItemInnerSpacing.y));
            Text("%s", label);
        }
        {
            ImVec2 pos = GetCursorScreenPos();
            PushItemWidth(w);
            result |= InputTextEx(std::string("##" + std::string(label)).c_str(), NULL, buf,
                                  (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
            PopItemWidth();

            if (text_size.x > 0 && !IsItemActive() && !strlen(buf))
            {
                SetCursorScreenPos(pos + style.FramePadding);
                TextDisabled("%s", text);
            }
        }
        if (has_label) PopStyleVar();
    }
    EndGroup();
    return result;
}

bool Button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) &&
        style.FramePadding.y < window->DC.CurrLineTextBaseOffset)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;

    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f,
                               label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    ImVec4 colFrame = GetStyleColorVec4((held && hovered) ? ImGuiCol_ButtonActive :
                                        hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

    struct State { ImVec4 Frame; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s.Frame = colFrame;
    s.Frame = ImLerp(s.Frame, colFrame, 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime);

    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, GetColorU32(s.Frame), true, style.FrameRounding);

    if (g.LogEnabled) LogSetNextTextDecoration("[", "]");
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding,
                      label, NULL, &label_size, style.ButtonTextAlign, &bb);
    return pressed;
}

bool ButtonAutoWidth(const char* label, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos  = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) &&
        style.FramePadding.y < window->DC.CurrLineTextBaseOffset)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;

    ImVec2 size(label_size.x + style.FramePadding.x * 2.0f,
                label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    ImVec4 colFrame = GetStyleColorVec4((held && hovered) ? ImGuiCol_ButtonActive :
                                        hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

    struct State { ImVec4 Frame; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s.Frame = colFrame;
    s.Frame = ImLerp(s.Frame, colFrame, 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime);

    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, GetColorU32(s.Frame), true, style.FrameRounding);

    if (g.LogEnabled) LogSetNextTextDecoration("[", "]");
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding,
                      label, NULL, &label_size, style.ButtonTextAlign, &bb);
    return pressed;
}

bool ToggleButtonClassic(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    float height   = g.FontSize;
    float width    = height * 2;
    float rounding = height / 2;
    float grab_radius = (height / 2) - 3;

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = ImVec2(label_size.x > 0 ? label_size.x + style.ItemInnerSpacing.x + width : width, height);

    const ImRect bb(pos, pos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !*v;

    ImVec4 colFrame = GetStyleColorVec4(*v ? ImGuiCol_SliderGrab : ImGuiCol_FrameBg);
    ImVec4 colBorder = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive : ImGuiCol_Border);
    ImVec4 colGrab  = GetStyleColorVec4(*v ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct State { ImVec4 Frame, Border, Grab; float GrabPos; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0)
        s = { colFrame, colBorder, colGrab, *v ? 1.0f : 0.0f };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame   = ImLerp(s.Frame,   colFrame,  t);
    s.Border  = ImLerp(s.Border,  colBorder, t);
    s.Grab    = ImLerp(s.Grab,    colGrab,   t);
    s.GrabPos = ImLerp(s.GrabPos, *v ? (width - rounding) : rounding, t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(pos, pos + ImVec2(width, size.y), GetColorU32(s.Frame), rounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + ImVec2(width, size.y), GetColorU32(s.Border), rounding, 0, style.FrameBorderSize);
    window->DrawList->AddCircleFilled(pos + ImVec2(s.GrabPos, rounding), grab_radius, GetColorU32(s.Grab));
    if (label_size.x > 0)
        RenderText(pos + ImVec2(width + style.ItemInnerSpacing.x, 0), label);

    return pressed;
}

bool ToggleButton(const char* label, bool* v, float spacing_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    float height   = g.FontSize;
    float width    = height * 2;
    float rounding = height / 2;
    float grab_radius = (height / 2) - 3;
    float spacing = CalcItemSize(ImVec2(spacing_arg > 0 ? spacing_arg : -0.1f, 0),
                                 label_size.x > 0 ? (label_size.x + style.ItemInnerSpacing.x) : 0, 0).x;

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = ImVec2(label_size.x > 0 ? label_size.x + style.ItemInnerSpacing.x + width : width, height);

    const ImRect bb(pos, pos + ImVec2(spacing, size.y));
    ItemSize(size);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !*v;

    ImVec4 colFrame  = GetStyleColorVec4(*v ? ImGuiCol_SliderGrab : ImGuiCol_FrameBg);
    ImVec4 colBorder = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive : ImGuiCol_Border);
    ImVec4 colGrab   = GetStyleColorVec4(*v ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct State { ImVec4 Frame, Border, Grab; float GrabPos; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0)
        s = { colFrame, colBorder, colGrab, *v ? 1.0f : 0.0f };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame   = ImLerp(s.Frame,   colFrame,  t);
    s.Border  = ImLerp(s.Border,  colBorder, t);
    s.Grab    = ImLerp(s.Grab,    colGrab,   t);
    s.GrabPos = ImLerp(s.GrabPos, *v ? (width - rounding) : rounding, t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(pos + ImVec2(spacing - width, 0),
                                    pos + ImVec2(spacing - width, 0) + ImVec2(width, size.y),
                                    GetColorU32(s.Frame), rounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos + ImVec2(spacing - width, 0),
                                  pos + ImVec2(spacing - width, 0) + ImVec2(width, size.y),
                                  GetColorU32(s.Border), rounding, 0, style.FrameBorderSize);
    window->DrawList->AddCircleFilled(pos + ImVec2(spacing - width, 0) + ImVec2(s.GrabPos, rounding),
                                      grab_radius, GetColorU32(s.Grab));
    if (label_size.x > 0)
        RenderText(pos, label);

    return pressed;
}

bool CheckBox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    float height = g.FontSize;
    ImVec2 pos   = window->DC.CursorPos;
    ImVec2 size  = ImVec2(label_size.x > 0 ? label_size.x + style.ItemInnerSpacing.x + height : height, height);

    const ImRect bb(pos, pos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !*v;

    ImVec4 colFrame = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive :
                                        (hovered && !held) ? ImGuiCol_FrameBgHovered :
                                        held ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg);
    ImVec4 colBorder = GetStyleColorVec4(*v ? ImGuiCol_SliderGrab : ImGuiCol_Border);
    ImVec4 colCMMain = GetStyleColorVec4(ImGuiCol_CheckMark);
    ImVec4 colCMNull = colCMMain; colCMNull.w = 0.0f;
    ImVec4 colCM     = (*v ? colCMMain : colCMNull);

    struct State { ImVec4 Frame, Border, CheckMark; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s = { colFrame, colBorder, colCM };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame     = ImLerp(s.Frame,     colFrame,  t);
    s.Border    = ImLerp(s.Border,    colBorder, t);
    s.CheckMark = ImLerp(s.CheckMark, colCM,     t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(pos, pos + ImVec2(height, size.y), GetColorU32(s.Frame), style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + ImVec2(height, size.y), GetColorU32(s.Border), style.FrameRounding, 0, style.FrameBorderSize);

    float pad = ImMax(1.0f, IM_TRUNC(height / 3.6f));
    RenderCheckMark(window->DrawList, pos + ImVec2(pad, pad), GetColorU32(s.CheckMark), height - pad * 2.0f);

    if (label_size.x > 0)
        RenderText(pos + ImVec2(height + style.ItemInnerSpacing.x, 0), label);
    return pressed;
}

bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data,
                  const void* p_min, const void* p_max, float width, const char* format)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    char value_buf[64];
    DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format ? format : DataTypeGetInfo(data_type)->PrintFmt);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;
    w -= label_size.x > 0 ? 0 : CalcTextSize(value_buf).x + style.ItemInnerSpacing.x;

    ImVec2 pos = window->DC.CursorPos;
    const float label_height = label_size.x > 0 ? g.FontSize + style.ItemInnerSpacing.y : 0.0f;
    const ImRect frame_bb(pos + ImVec2(0, label_height), pos + ImVec2(w, g.FontSize + label_height));
    const ImRect total_bb(pos, pos + ImVec2(w, g.FontSize + label_height));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb)) return false;

    if (format == NULL) format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered    = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    const bool clicked    = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
    const bool make_active = (clicked || g.NavActivateId == id);

    if (make_active)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    ImVec4 colFrame = GetStyleColorVec4(g.ActiveId == id ? ImGuiCol_FrameBgActive :
                                        hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImVec4 colGrab  = GetStyleColorVec4(g.ActiveId == id ? ImGuiCol_SliderGrab : ImGuiCol_SliderGrabActive);

    struct State { ImVec4 Frame, Grab; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s = { colFrame, colGrab };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame = ImLerp(s.Frame, colFrame, t);
    s.Grab  = ImLerp(s.Grab,  colGrab,  t);

    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(s.Frame), true, style.FrameRounding);

    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, 0, &grab_bb);
    if (value_changed) MarkItemEdited(id);

    ImVec2 grab_padding = ImVec2(grab_bb.Min.y - frame_bb.Min.y, 0);
    grab_padding.y = grab_padding.x;

    if (grab_bb.Max.x > grab_bb.Min.x)
    {
        window->DrawList->AddRectFilled(frame_bb.Min, grab_bb.Max + grab_padding,
                                        GetColorU32(s.Grab), style.FrameRounding);
        if (style.FrameBorderSize > 0)
            window->DrawList->AddRect(frame_bb.Min, grab_bb.Max + grab_padding,
                                      GetColorU32(ImGuiCol_SliderGrab), style.FrameRounding, 0, style.FrameBorderSize);
    }

    float extra_width = label_size.x > 0 ? -CalcTextSize(value_buf).x : style.ItemInnerSpacing.x;
    window->DrawList->AddText(pos + ImVec2(w + extra_width, 0), GetColorU32(ImGuiCol_TextDisabled), value_buf);
    if (label_size.x > 0) RenderText(pos, label);

    return value_changed;
}

bool SliderFloat(const char* label, float* v, float v_min, float v_max, float width, const char* format)
{
    return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, width, format);
}

bool SliderInt(const char* label, int* v, int v_min, int v_max, float width, const char* format)
{
    return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, width, format);
}

bool Selectable(const char* label, bool selected, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f,
                               label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    ImVec4 colFrameMain = GetStyleColorVec4((hovered && !selected) ?
                          held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered : ImGuiCol_Header);
    ImVec4 colFrameNull = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame     = ((!hovered && !selected) ? colFrameNull : colFrameMain);
    ImVec4 colBorderMain = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 colBorderNull = colBorderMain; colBorderNull.w = 0.0f;
    ImVec4 colBorder     = (selected ? colBorderMain : colBorderNull);

    struct State { ImVec4 Frame, Border; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s = { colFrame, colBorder };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame  = ImLerp(s.Frame,  colFrame,  t);
    s.Border = ImLerp(s.Border, colBorder, t);

    RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(s.Frame), style.FrameRounding);
    if (style.FrameBorderSize > 0)
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(s.Border), style.FrameRounding, 0, style.FrameBorderSize);
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding,
                      label, NULL, &label_size, style.ButtonTextAlign, &bb);
    return pressed;
}

static float CalcMaxPopupHeightFromItemCount_W(int items_count)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0) return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.HasFlags;
    g.NextWindowData.ClearFlags();
    if (window->SkipItems) return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) !=
              (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview));
    if (flags & ImGuiComboFlags_WidthFitPreview)
        IM_ASSERT((flags & (ImGuiComboFlags_NoPreview | ImGuiComboFlags_CustomPreview)) == 0);

    const float arrow_size    = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const ImVec2 label_size   = CalcTextSize(label, NULL, true);
    const float preview_width = ((flags & ImGuiComboFlags_WidthFitPreview) && preview_value != NULL) ?
                                 CalcTextSize(preview_value, NULL, true).x : 0.0f;
    const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size :
                    ((flags & ImGuiComboFlags_WidthFitPreview) ?
                     (arrow_size + preview_width + style.FramePadding.x * 2.0f) : CalcItemWidth());

    const ImVec2 offset(0.0f, label_size.x > 0 ? label_size.y + style.ItemInnerSpacing.y : 0.0f);
    const ImRect bb(window->DC.CursorPos + offset,
                    window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f) + offset);
    const ImRect total_bb(window->DC.CursorPos,
                          window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f) + offset);

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &bb)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    ImVec4 colFrame = GetStyleColorVec4(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImVec4 colText  = GetStyleColorVec4((popup_open || hovered) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct State { ImVec4 Frame, Text; };
    static std::map<ImGuiID, State> anim;
    auto& s = anim[id];
    if (s.Frame.w == 0 && s.Frame.x == 0) s = { colFrame, colText };

    float t = 1.0f / IMWIDGETS_ANIMATION_SPEED * GetIO().DeltaTime;
    s.Frame = ImLerp(s.Frame, colFrame, t);
    s.Text  = ImLerp(s.Text,  colText,  t);

    const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    RenderNavHighlight(bb, id);
    if (!(flags & ImGuiComboFlags_NoPreview))
        window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(s.Frame), style.FrameRounding);
    if (!(flags & ImGuiComboFlags_NoArrowButton))
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
            RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y),
                        GetColorU32(s.Text), ImGuiDir_Down, 1.0f);
    RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    if (flags & ImGuiComboFlags_CustomPreview)
    {
        g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
        preview_value = NULL;
    }

    if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
    {
        if (g.LogEnabled) LogSetNextTextDecoration("{", "}");
        PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
        RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
        PopStyleColor();
    }
    if (label_size.x > 0) RenderText(total_bb.Min, label);

    if (!popup_open) return false;
    g.NextWindowData.HasFlags = backup_next_window_data_flags;
    return BeginComboPopup(popup_id, bb, flags);
}

bool BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(popup_id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();
        return false;
    }

    float w = bb.GetWidth();
    if (g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint)
    {
        g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
    }
    else
    {
        if ((flags & ImGuiComboFlags_HeightMask_) == 0) flags |= ImGuiComboFlags_HeightRegular;
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));
        int popup_max_height_in_items = -1;
        if (flags & ImGuiComboFlags_HeightRegular) popup_max_height_in_items = 8;
        else if (flags & ImGuiComboFlags_HeightSmall) popup_max_height_in_items = 4;
        else if (flags & ImGuiComboFlags_HeightLarge) popup_max_height_in_items = 20;
        ImVec2 cmin(0.0f, 0.0f), cmax(FLT_MAX, FLT_MAX);
        if ((g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.x <= 0.0f) cmin.x = w;
        if ((g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.y <= 0.0f)
            cmax.y = CalcMaxPopupHeightFromItemCount_W(popup_max_height_in_items);
        SetNextWindowSizeConstraints(cmin, cmax);
    }

    char name[16];
    ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginComboDepth);

    if (ImGuiWindow* popup_window = FindWindowByName(name))
        if (popup_window->WasActive)
        {
            ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
            popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ?
                                                 ImGuiDir_Left : ImGuiDir_Down;
            ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
            ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected,
                                                     &popup_window->AutoPosLastDirection, r_outer, bb,
                                                     ImGuiPopupPositionPolicy_ComboBox);
            SetNextWindowPos(pos + ImVec2(0, g.Style.ItemSpacing.y));
        }

    ImGuiWindowFlags wf = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup |
                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.FramePadding);
    PushStyleVar(ImGuiStyleVar_PopupRounding, g.Style.FrameRounding);
    bool ret = Begin(name, NULL, wf);
    PopStyleVar(2);
    if (!ret) { EndPopup(); IM_ASSERT(0); return false; }
    g.BeginComboDepth++;
    return true;
}

static const char* Items_ArrayGetter_W(void* data, int idx) { return ((const char* const*)data)[idx]; }
static const char* Items_SingleStringGetter_W(void* data, int idx)
{
    const char* p = (const char*)data;
    int n = 0;
    while (*p) { if (n == idx) break; p += strlen(p) + 1; n++; }
    return *p ? p : NULL;
}

bool Combo(const char* label, int* current_item, const char* (*getter)(void*, int), void* user_data,
           int items_count, int popup_max_height_in_items)
{
    ImGuiContext& g = *GImGui;
    const char* preview_value = (*current_item >= 0 && *current_item < items_count) ?
                                getter(user_data, *current_item) : NULL;
    if (popup_max_height_in_items != -1 && !(g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint))
        SetNextWindowSizeConstraints(ImVec2(0,0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount_W(popup_max_height_in_items)));
    if (!BeginCombo(label, preview_value, ImGuiComboFlags_None)) return false;

    bool value_changed = false;
    for (int i = 0; i < items_count; i++)
    {
        const char* item_text = getter(user_data, i);
        if (!item_text) item_text = "*Unknown item*";
        PushID(i);
        const bool item_selected = (i == *current_item);
        if (Selectable(item_text, item_selected, ImVec2(-0.1f, 0)) && *current_item != i)
        {
            value_changed = true;
            *current_item = i;
        }
        if (item_selected) SetItemDefaultFocus();
        PopID();
    }
    EndCombo();
    if (value_changed) MarkItemEdited(g.LastItemData.ID);
    return value_changed;
}

bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
    return Combo(label, current_item, Items_ArrayGetter_W, (void*)items, items_count, height_in_items);
}

bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, float width, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p) { p += strlen(p) + 1; items_count++; }
    const float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;
    PushItemWidth(w);
    bool changed = Combo(label, current_item, Items_SingleStringGetter_W,
                         (void*)items_separated_by_zeros, items_count, height_in_items);
    PopItemWidth();
    return changed;
}

static std::vector<ImVec2> s_highlightStack;

bool BeginHighlightGroup()
{
    BeginGroup();
    s_highlightStack.push_back(GetCursorScreenPos());
    return true;
}

bool EndHighlightGroup(float lineThickness, float offset, ImVec4 color)
{
    EndGroup();
    if (!s_highlightStack.empty())
    {
        ImVec2 start = s_highlightStack.back();
        s_highlightStack.pop_back();
        ImVec2 end = GetCursorScreenPos();
        ImDrawList* dl = GetWindowDrawList();
        dl->AddLine(ImVec2(start.x - offset, start.y),
                    ImVec2(end.x - offset, end.y),
                    ColorConvertFloat4ToU32(color), lineThickness);
    }
    return true;
}

} // namespace Widgets
} // namespace ImGui
