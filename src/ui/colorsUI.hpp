#ifndef UI_COLORS_HPP
#define UI_COLORS_HPP

#include <imgui/imgui.h>

class ColorsUI {
public:
    static constexpr ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    static constexpr ImVec4 mgColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    static constexpr ImVec4 fgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    static constexpr ImU32 lightBlue32 = IM_COL32(109, 155, 212, 255);
    static constexpr ImVec4 lightBlue = ImVec4(0.42f, 0.6f, 0.83f, 1.0f);
    static constexpr ImVec4 blue = ImVec4(0.2f, 0.41f, 0.68f, 1.0f);
    static constexpr ImVec4 lightBlueBorder = ImVec4(0.42f, 0.6f, 0.83f, 0.1f);

    static constexpr ImVec4 selectionColor = ImVec4(1.0f, 0.68f, 0.0f, 1.0f);
};

#endif