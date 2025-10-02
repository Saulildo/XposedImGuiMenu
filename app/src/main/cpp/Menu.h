#pragma once

using namespace ImGui;

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
    SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    if (Begin("Xposed Menu - com.herogame.gplay.survival.lite", &showMenu)) {
        
        // ESP Section
        if (CollapsingHeader("ESP Settings")) {
            Checkbox("Enable ESP", &g_ESPConfig.Draw);
            
            if (g_ESPConfig.Draw) {
                Indent(20.0f);
                Checkbox("Show Lines", &g_ESPConfig.Line);
                Checkbox("Show HP", &g_ESPConfig.Hp);
                Checkbox("Show Distance", &g_ESPConfig.Distance);
                Checkbox("Show Bones", &g_ESPConfig.Bones);
                Checkbox("Show Name", &g_ESPConfig.Name);
                Checkbox("Show 3D Box", &g_ESPConfig.Box3D);
                
                Separator();
                Text("Box Adjustments:");
                SliderFloat("Top Position", &g_topPosFloat, 1.0f, 3.0f);
                SliderFloat("Bottom Position", &g_botPosFloat, 0.0f, 1.0f);
                SliderFloat("Distance Scale", &g_clappedFloat, 50.0f, 200.0f);
                SliderFloat("Box Scale", &g_calculatedPositionFloat, 1.0f, 5.0f);
                
                Separator();
                Text("Bone Color:");
                ColorEdit4("Bones", g_BonesColor);
                
                Unindent(20.0f);
            }
        }
        
        Separator();
        
        // Other features section (placeholder)
        if (CollapsingHeader("Other Features")) {
            Checkbox("Example feature", &some_feature);
        }
        
        Separator();
        
        if (Button("Close Menu")) {
            showMenu = false;
        }
    }
    End();
}
