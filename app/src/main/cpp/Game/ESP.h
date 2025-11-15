#pragma once

#include <cstdint>

#include <BNM/UserSettings/GlobalSettings.hpp>
#include <BNM/Assembly.hpp>
#include <BNM/Class.hpp>
#include <BNM/Field.hpp>
#include <BNM/Method.hpp>

#include <imgui.h>

#include "GameValues.h"

// Basic vector to match UnityEngine.Vector3
struct Vec3 {
    float x, y, z;
};

// Minimal IL2CPP structures for List<T> and T[] for our use
struct Il2CppObject {
    void* klass;
    void* monitor;
};

struct Il2CppArray : Il2CppObject {
    void*    bounds;
    uint32_t max_length;
    void*    vector[32]; // flexible array; we treat as pointer to data
};

struct Il2CppList : Il2CppObject {
    Il2CppArray* _items;
    int          _size;
    int          _version;
    void*        _syncRoot;
};

// Simple ESP settings
struct EspSettings {
    bool draw     = false;
    bool line     = true;
    bool name     = false;
    bool distance = false;
    bool hp       = false;
    bool bones    = false;
    bool zbox     = false;
};

inline EspSettings gEsp;

// Internal cached BNM handles
inline bool         gEspInit = false;

// Classes
inline BNM::Class   clsController;
inline BNM::Class   clsComponent;
inline BNM::Class   clsTransform;
inline BNM::Class   clsGameObject;
inline BNM::Class   clsCamera;

// Fields
inline BNM::Field<void*> fieldControllerList;

// Methods (we treat all returns as void* and cast ourselves)
inline BNM::Method<void*> mGetTransform;
inline BNM::Method<void*> mGetPosition;
inline BNM::Method<void*> mCamGetMain;
inline BNM::Method<void*> mCamWorldToScreen;

// Initialize all reflection handles once
inline void InitESP()
{
    if (gEspInit) return;

    auto asm_cs   = BNM::Assembly::Get(BNM_OBFUSCATE("Assembly-CSharp"));
    clsController = asm_cs.GetClass(BNM_OBFUSCATE(""), BNM_OBFUSCATE("KMQZQTPUQNQ"));
    fieldControllerList = clsController.GetField(BNM_OBFUSCATE("WKMVQWUNXMZ"));

    auto asm_unity = BNM::Assembly::Get(BNM_OBFUSCATE("UnityEngine.CoreModule"));
    clsComponent   = asm_unity.GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("Component"));
    clsTransform   = asm_unity.GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("Transform"));
    clsGameObject  = asm_unity.GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("GameObject"));
    clsCamera      = asm_unity.GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("Camera"));

    mGetTransform     = clsComponent.GetMethod(BNM_OBFUSCATE("get_transform"), 0);
    mGetPosition      = clsTransform.GetMethod(BNM_OBFUSCATE("get_position"), 0);
    mCamGetMain       = clsCamera.GetMethod(BNM_OBFUSCATE("get_main"), 0);
    // Camera.WorldToScreenPoint(Vector3)
    mCamWorldToScreen = clsCamera.GetMethod(BNM_OBFUSCATE("WorldToScreenPoint"), 1);

    gEspInit = true;
}

// WorldToScreen using Camera.main.WorldToScreenPoint
inline bool WorldToScreen(const Vec3& world, ImVec2& out)
{
    InitESP();

    // Camera.main (static, no args)
    void* camObj = mCamGetMain[nullptr]();
    if (!camObj)
        return false;

    // Camera.WorldToScreenPoint(Vector3)
    void* res = mCamWorldToScreen[camObj](world);
    if (!res)
        return false;

    Vec3 screen = *reinterpret_cast<Vec3*>(res);

    // If behind camera
    if (screen.z <= 0.0f)
        return false;

    float width  = static_cast<float>(glWidth);
    float height = static_cast<float>(glHeight);

    // Unity screen coordinates: (0,0) bottom-left
    // ImGui: (0,0) top-left
    out.x = screen.x;
    out.y = height - screen.y;

    return true;
}

// Iterate all controllers from KMQZQTPUQNQ.WKMVQWUNXMZ and draw simple ESP
inline void DrawESP()
{
    if (!esp_enabled || !gEsp.draw)
        return;

    InitESP();

    // Static field, instance is nullptr
    void** listPtr = fieldControllerList[nullptr].GetPointer();
    if (!listPtr || !*listPtr)
        return;

    auto* list = reinterpret_cast<Il2CppList*>(*listPtr);
    if (!list->_items || list->_size <= 0)
        return;

    auto* itemsArr = list->_items;
    auto** items   = reinterpret_cast<void**>(itemsArr->vector);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    for (int i = 0; i < list->_size; ++i)
    {
        void* ctrl = items[i];
        if (!ctrl) continue;

        // KMQZQTPUQNQ derives from MonoBehaviour -> Component
        void* trObj = mGetTransform[ctrl]();
        if (!trObj) continue;

        void* posObj = mGetPosition[trObj]();
        if (!posObj) continue;

        Vec3  worldPos  = *reinterpret_cast<Vec3*>(posObj);
        ImVec2 screenPos;

        if (!WorldToScreen(worldPos, screenPos))
            continue;

        // Optionally clamp distance using clappedFloat and worldPos.z or magnitude,
        // currently just draws everything in front of camera.

        // Draw line from bottom center
        if (gEsp.line) {
            drawList->AddLine(
                ImVec2(glWidth / 2.0f, static_cast<float>(glHeight)),
                screenPos,
                IM_COL32(
                    static_cast<int>(bonesColor[0] * 255.0f),
                    static_cast<int>(bonesColor[1] * 255.0f),
                    static_cast<int>(bonesColor[2] * 255.0f),
                    static_cast<int>(bonesColor[3] * 255.0f)),
                2.0f
            );
        }

        // Simple cross at player position
        float size = 5.0f;
        drawList->AddLine(
            ImVec2(screenPos.x - size, screenPos.y),
            ImVec2(screenPos.x + size, screenPos.y),
            IM_COL32(255, 0, 0, 255),
            1.5f
        );
        drawList->AddLine(
            ImVec2(screenPos.x, screenPos.y - size),
            ImVec2(screenPos.x, screenPos.y + size),
            IM_COL32(255, 0, 0, 255),
            1.5f
        );
    }
}
