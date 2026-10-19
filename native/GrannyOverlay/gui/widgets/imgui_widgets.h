#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_internal.h"

#define IMWIDGETS_ANIMATION_SPEED 0.05f

namespace ImGui
{
    namespace Widgets
    {
        // Helpers
        ImVec4  HexToColorVec4(unsigned int hex_color, float alpha);

        // Text / Separator
        void    DoubleText(ImVec4 color1, ImVec4 color2, const char* label1, const char* label2);
        void    TextCustomFont(ImFont* font, const char* fmt, ...);
        void    SeparatorText(const char* label, float thickness = 1.0f);
        void    VSeparator(float thickness = 1.0f);

        // Inputs / Buttons
        bool    RadioFrameIcon(const char* label, ImTextureID user_texture_id, int* v, int current_id, const ImVec2& size = ImVec2(0, 0), const ImVec2& adjust_icon_pos = ImVec2(0, 0));
        bool    RadioFrameIconColor(const char* label, ImTextureID user_texture_id, int* v, int current_id, const ImVec4& custom_icon_color, const ImVec2& size = ImVec2(0, 0), const ImVec2& adjust_icon_pos = ImVec2(0, 0));
        bool    RadioFrameIconText(const char* label, const char* icon_text, int* v, int current_id, ImFont* icon_font = nullptr, const ImVec2& size = ImVec2(0, 0));
        bool    RadioFrame(const char* label, int* v, int current_id, const ImVec2& size = ImVec2(0, 0));
        bool    RadioFrameColor(const char* str_id, int* v, int current_id, ImVec4 accent_color, const ImVec2& size_arg = ImVec2(0, 0));
        bool    InputText(const char* label, const char* text, char* buf, size_t buf_size, float width = 0, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
        bool    Button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags = 0);
        bool    ButtonAutoWidth(const char* label, ImGuiButtonFlags flags = 0);

        // Toggle / Checkbox
        bool    ToggleButtonClassic(const char* label, bool* v);
        bool    ToggleButton(const char* label, bool* v, float width = -0.1f);
        bool    CheckBox(const char* label, bool* v);

        // Slider
        bool    SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, float width, const char* format = NULL);
        bool    SliderFloat(const char* label, float* v, float v_min, float v_max, float width = -0.1f, const char* format = "%.1f");
        bool    SliderInt(const char* label, int* v, int v_min, int v_max, float width = -0.1f, const char* format = "%d");

        // Combo
        bool    Selectable(const char* label, bool selected = false, const ImVec2& size_arg = ImVec2(0, 0));
        bool    BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
        bool    BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags);
        bool    Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
        bool    Combo(const char* label, int* current_item, const char* items_separated_by_zeros, float width = -0.1f, int popup_max_height_in_items = -1);
        bool    Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items = -1);

        // Highlight group
        bool    BeginHighlightGroup();
        bool    EndHighlightGroup(float lineThickness = 5.0f, float offset = 5, ImVec4 color = HexToColorVec4(0xFFFFFF, 255));
    }
}

// Backward-compat alias so older ImAdd:: callsites compile unchanged
namespace ImAdd = ImGui::Widgets;
