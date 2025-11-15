#pragma once

using namespace ImGui;

#include "ESP.h"
#include "GameValues.h"

void SetupImGui() {
    IMGUI_CHECKVERSION();
    CreateContext();
    ImGuiIO &io = GetIO();
    io.DisplaySize = ImVec2((float) glWidth, (float) glHeight);
    ImGui_ImplOpenGL3_Init("#version 100");
    StyleColorsDark();

    GetStyle().ScaleAllSizes(4.0f); // Change this to scale everything
}

void DrawMenu() {
    static bool showMenu = true;
    SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
    if (Begin("Menu", &showMenu)) {
        if (CollapsingHeader("Player ESP")) {
            Checkbox("Enable ESP", &esp_enabled);
            if (esp_enabled) {
                Checkbox("Draw ESP", &gEsp.draw);
                if (gEsp.draw) {
                    Indent();
                    Checkbox("Lines", &gEsp.line);

                    Separator();
                    Text("ESP Configuration:");
                    SliderFloat("Clamp Distance", &clappedFloat, 50.0f, 200.0f);
                    SliderFloat("Top Position", &topPosFloat, 1.0f, 3.0f);
                    SliderFloat("Bot Position", &botPosFloat, 0.0f, 1.0f);
                    SliderFloat("Calc Position", &calculatedPositionFloat, 1.0f, 5.0f);

                    ColorEdit4("Bones Color", bonesColor);
                    Unindent();
                }
            }
        }

        End();
    }
}
