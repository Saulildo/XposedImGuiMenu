#pragma once

#include "Includes.h"
#include "GameValues.h"

using namespace BNM;

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
    void* bounds;
    uint32_t max_length;
    void* vector[32]; // flexible array; we treat as pointer to data
};

struct Il2CppList : Il2CppObject {
    Il2CppArray* _items;
    int _size;
    int _version;
    void* _syncRoot;
};

// Simple ESP settings
struct EspSettings {
    bool draw   = false;
    bool line   = true;
    bool name   = false;
    bool distance = false;
    bool hp     = false;
    bool bones  = false;
    bool zbox   = false;
};

inline EspSettings gEsp;

// Internal cached BNM handles
static bool      gEspInit           = false;
static Class*    clsController      = nullptr;
static Field*    fieldControllerList = nullptr;

static Class*    clsComponent       = nullptr;
static Class*    clsTransform       = nullptr;
static Class*    clsGameObject      = nullptr;
static Class*    clsCamera          = nullptr;

static Method*   mGetTransform      = nullptr;
static Method*   mGetPosition       = nullptr;
static Method*   mCamGetMain        = nullptr;
static Method*   mCamWorldToScreen  = nullptr;

// Initialize all reflection handles once
static void InitESP()
{
    if (gEspInit) return;

    auto* asm_cs     = Assembly::Get(BNM_OBFUSCATE("Assembly-CSharp"));
    clsController    = asm_cs->GetClass(BNM_OBFUSCATE(""), BNM_OBFUSCATE("KMQZQTPUQNQ"));
    fieldControllerList = clsController->GetField(BNM_OBFUSCATE("WKMVQWUNXMZ"));

    auto* asm_unity  = Assembly::Get(BNM_OBFUSCATE("UnityEngine.CoreModule"));
    clsComponent     = asm_unity->GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("Component"));
    clsTransform     = asm_unity->GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("Transform"));
    clsGameObject    = asm_unity->GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("GameObject"));
    clsCamera        = asm_unity->GetClass(BNM_OBFUSCATE("UnityEngine"), BNM_OBFUSCATE("Camera"));

    mGetTransform    = clsComponent->GetMethod(BNM_OBFUSCATE("get_transform"), 0);
    mGetPosition     = clsTransform->GetMethod(BNM_OBFUSCATE("get_position"), 0);
    mCamGetMain      = clsCamera->GetMethod(BNM_OBFUSCATE("get_main"), 0);
    // Camera.WorldToScreenPoint(Vector3)
    mCamWorldToScreen = clsCamera->GetMethod(BNM_OBFUSCATE("WorldToScreenPoint"), 1);

    gEspInit = true;
}

// WorldToScreen using Camera.main.WorldToScreenPoint
static bool WorldToScreen(const Vec3& world, ImVec2& out)
{
    InitESP();
    if (!mCamGetMain || !mCamWorldToScreen) return false;

    Object* camObj = mCamGetMain->Invoke(nullptr, nullptr);
    if (!camObj) return false;

    Vec3 pos = world;
    void* args[] { &pos };

    Object* resObj = mCamWorldToScreen->Invoke(camObj, args);
    if (!resObj) return false;

    Vec3 screen = *(Vec3*)resObj;

    // If behind camera
    if (screen.z <= 0.0f)
        return false;

    float width  = (float)glWidth;
    float height = (float)glHeight;

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
    if (!fieldControllerList) return;

    Object* listObj = (Object*)fieldControllerList->GetValue(nullptr); // static field
    if (!listObj) return;

    auto* list = (Il2CppList*)listObj;
    if (!list->_items || list->_size <= 0) return;

    auto* itemsArr = list->_items;
    auto** items   = (Object**)itemsArr->vector;

    auto* drawList = ImGui::GetForegroundDrawList();

    for (int i = 0; i < list->_size; ++i)
    {
        Object* ctrl = items[i];
        if (!ctrl) continue;

        // KMQZQTPUQNQ derives from MonoBehaviour -> Component
        Object* trObj = mGetTransform->Invoke(ctrl, nullptr);
        if (!trObj) continue;

        Object* posObj = mGetPosition->Invoke(trObj, nullptr);
        if (!posObj) continue;

        Vec3 worldPos = *(Vec3*)posObj;
        ImVec2 screenPos;

        if (!WorldToScreen(worldPos, screenPos))
            continue;

        // Optionally clamp distance using clappedFloat and worldPos.z or magnitude,
        // currently just draws everything in front of camera.

        // Draw line from bottom center
        if (gEsp.line) {
            drawList->AddLine(
                ImVec2(glWidth / 2.0f, (float)glHeight),
                screenPos,
                IM_COL32((int)(bonesColor[0] * 255.0f),
                         (int)(bonesColor[1] * 255.0f),
                         (int)(bonesColor[2] * 255.0f),
                         (int)(bonesColor[3] * 255.0f)),
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
