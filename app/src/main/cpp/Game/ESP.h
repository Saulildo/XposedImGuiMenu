#pragma once

#include <vector>
#include <string>
#include <algorithm>

using namespace BNM;
using namespace BNM::Structures::Unity;
using namespace BNM::Structures::Mono;

// ESP Configuration Structure
struct ESPConfig {
    bool Draw = false;
    bool Line = false;
    bool Hp = false;
    bool Distance = false;
    bool Bones = false;
    bool Name = false;
    bool Box3D = false;
} g_ESPConfig;

// Enemy/Player data structure
struct EnemyData {
    void *ptr = nullptr;
    void *trans = nullptr;
    void *playerinfo = nullptr;
    Vector3 pos;
    ImVec2 w2spos;
    ImVec2 w2stop;
    int Hp = 0;
    int MaxHp = 0;
    std::string Name;
};

// Player info cache
struct PlayerCache {
    Vector3 pos;
    int Hp;
    int MaxHp;
    std::string Name;
};

// ESP Drawing Configuration
float g_clappedFloat = 100.0f;
float g_topPosFloat = 1.82f;
float g_botPosFloat = 0.1f;
float g_calculatedPositionFloat = 2.2f;
float g_BonesColor[4] = {255, 255, 255, 255};

// Cached data
std::vector<PlayerCache> g_playerCache;
EnemyData g_enemy;
Vector3 g_selfPos;
void *g_Battle = nullptr;
void *g_MainCamera = nullptr;

// Template structures for IL2CPP mono types (compatible with hexipa's structures)
template<typename T>
struct monoArray {
    void *klass;
    void *monitor;
    void *bounds;
    int32_t capacity;
    T m_Items[0];
    
    int32_t getCapacity() { return capacity; }
    T *getPointer() { return m_Items; }
};

template<typename TKey, typename TValue>
struct monoDictionary2 {
    struct Entry {
        int hashCode, next;
        TKey key;
        TValue value;
    };
    void *klass;
    void *monitor;
    monoArray<int> *buckets;
    monoArray<Entry> *entries;
    int count;
    int version;
    int freeList;
    int freeCount;
    void *comparer;
    monoArray<TKey> *keys;
    monoArray<TValue> *values;
    void *syncRoot;
    
    std::vector<TValue> getValues() {
        std::vector<TValue> ret;
        if (!entries) return ret;
        for (int i = 0; i < count; i++) {
            ret.push_back(entries->m_Items[i].value);
        }
        return ret;
    }
    
    int getSize() { return count; }
};

typedef struct _monoString {
    void* klass;
    void* monitor;
    int length;
    char16_t chars[1];
    
    std::string toCPPString() {
        if (!this || length <= 0) return "";
        std::u16string u16str(chars, length);
        std::string str;
        for (char16_t c : u16str) {
            if (c < 128) str += (char)c;
            else str += '?';
        }
        return str;
    }
} monoString;

// Helper: Get field from instance pointer
template<typename T>
inline T& GetField(void* instance, uintptr_t offset) {
    return *(T*)((uintptr_t)instance + offset);
}

// World to Screen conversion
ImVec2 WorldToScreen(Vector3 worldPos) {
    if (!g_MainCamera) return {-1, -1};
    
    // Call Camera.WorldToViewportPoint
    static auto WorldToViewportPoint = OFFSET("0x0");
    if (!WorldToViewportPoint.IsValid()) {
        // Try to find it using BNM if offset not set
        auto cameraCls = Class("UnityEngine.Camera");
        if (cameraCls) {
            auto method = cameraCls.GetMethod("WorldToViewportPoint", 1);
            if (method) {
                Vector3 viewportPoint = method.cast<Vector3>()(g_MainCamera, worldPos);
                
                float screenWidth = (float)glWidth;
                float screenHeight = (float)glHeight;
                
                float x = screenWidth * viewportPoint.x;
                float y = screenHeight - (viewportPoint.y * screenHeight);
                
                return {x, y};
            }
        }
    }
    
    return {-1, -1};
}

// World to Screen with visibility check
ImVec2 WorldToScreenChecked(Vector3 worldPos, bool& isVisible) {
    if (!g_MainCamera) {
        isVisible = false;
        return {-1, -1};
    }
    
    auto cameraCls = Class("UnityEngine.Camera");
    if (cameraCls) {
        auto method = cameraCls.GetMethod("WorldToViewportPoint", 1);
        if (method) {
            Vector3 viewportPoint = method.cast<Vector3>()(g_MainCamera, worldPos);
            
            float screenWidth = (float)glWidth;
            float screenHeight = (float)glHeight;
            
            float x = screenWidth * viewportPoint.x;
            float y = screenHeight - (viewportPoint.y * screenHeight);
            
            isVisible = (viewportPoint.z > 1.0f);
            return {x, y};
        }
    }
    
    isVisible = false;
    return {-1, -1};
}

// Draw bones between two transforms
void DrawBoneToBone(void *bone1, void *bone2, ImColor color) {
    if (!bone1 || !bone2) return;
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // Get Transform.position for both bones
    auto transformCls = Class("UnityEngine.Transform");
    if (!transformCls) return;
    
    auto getPosMethod = transformCls.GetMethod("get_position");
    if (!getPosMethod) return;
    
    Vector3 pos1 = getPosMethod.cast<Vector3>()(bone1);
    Vector3 pos2 = getPosMethod.cast<Vector3>()(bone2);
    
    ImVec2 screen1 = WorldToScreen(pos1);
    ImVec2 screen2 = WorldToScreen(pos2);
    
    if (screen1.x >= 0 && screen2.x >= 0) {
        drawList->AddLine(screen1, screen2, color);
    }
}

// Draw skeleton bones
void DrawBones(monoArray<void**>* bodyParts, const std::vector<std::pair<int, int>>& connections, ImColor color) {
    if (!bodyParts) return;
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    auto transformCls = Class("UnityEngine.Transform");
    if (!transformCls) return;
    
    auto getPosMethod = transformCls.GetMethod("get_position");
    if (!getPosMethod) return;
    
    for (const auto& [index1, index2] : connections) {
        if (index1 >= bodyParts->getCapacity() || index2 >= bodyParts->getCapacity())
            continue;
            
        void* bone1 = bodyParts->getPointer()[index1];
        void* bone2 = bodyParts->getPointer()[index2];
        
        if (bone1 && bone2) {
            Vector3 pos1 = getPosMethod.cast<Vector3>()(bone1);
            Vector3 pos2 = getPosMethod.cast<Vector3>()(bone2);
            
            ImVec2 screen1 = WorldToScreen(pos1);
            ImVec2 screen2 = WorldToScreen(pos2);
            
            if (screen1.x >= 0 && screen2.x >= 0) {
                drawList->AddLine(screen1, screen2, color);
            }
        }
    }
}

// Update player cache
void UpdatePlayerCache(const PlayerCache& newPlayer) {
    auto it = std::find_if(g_playerCache.begin(), g_playerCache.end(),
                           [&](const PlayerCache& player) {
                               return player.Name == newPlayer.Name;
                           });
    
    if (it != g_playerCache.end()) {
        it->pos = newPlayer.pos;
        it->Hp = newPlayer.Hp;
        it->MaxHp = newPlayer.MaxHp;
    } else {
        g_playerCache.push_back(newPlayer);
    }
}

// Main ESP rendering function
void RenderESP() {
    if (!g_ESPConfig.Draw) return;
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // Find Battle instance if not cached
    if (!g_Battle) {
        auto battleCls = Class("Battle, Assembly-CSharp");
        if (battleCls) {
            auto insField = battleCls.GetField("Ins");
            if (insField) {
                g_Battle = insField.cast<void*>().GetStatic();
            }
        }
    }
    
    if (!g_Battle) return;
    
    // Get MainCamera from Battle
    g_MainCamera = GetField<void*>(g_Battle, 0x28);
    if (!g_MainCamera) return;
    
    // Get SelfPlayer position
    void* selfPlayer = GetField<void*>(g_Battle, 0x20);
    if (selfPlayer) {
        void* selfTransform = GetField<void*>(selfPlayer, 0x40); // PlayerTransform offset
        if (selfTransform) {
            auto transformCls = Class("UnityEngine.Transform");
            if (transformCls) {
                auto getPosMethod = transformCls.GetMethod("get_position");
                if (getPosMethod) {
                    g_selfPos = getPosMethod.cast<Vector3>()(selfTransform);
                }
            }
        }
    }
    
    // Get OtherPlayersDic from Battle at offset 0x60 (UPDATED from 0x58)
    auto* playersDic = GetField<monoDictionary2<long, void*>*>(g_Battle, 0x60);
    
    if (!playersDic || playersDic->getSize() == 0) return;
    
    // Iterate through all players
    auto players = playersDic->getValues();
    
    for (void* playerPtr : players) {
        if (!playerPtr) continue;
        
        // Check if player is alive (simple null check, can be enhanced)
        g_enemy.ptr = playerPtr;
        
        // Get PlayerTransform
        g_enemy.trans = GetField<void*>(playerPtr, 0x40);
        if (!g_enemy.trans) continue;
        
        // Get Transform.position
        auto transformCls = Class("UnityEngine.Transform");
        if (!transformCls) continue;
        auto getPosMethod = transformCls.GetMethod("get_position");
        if (!getPosMethod) continue;
        
        g_enemy.pos = getPosMethod.cast<Vector3>()(g_enemy.trans);
        
        // World to screen with visibility check
        bool isVisible = false;
        g_enemy.w2spos = WorldToScreenChecked(g_enemy.pos, isVisible);
        if (!isVisible) continue;
        
        // Get HeadTop position
        void* headTop = GetField<void*>(playerPtr, 0x80);
        if (headTop) {
            Vector3 headPos = getPosMethod.cast<Vector3>()(headTop);
            g_enemy.w2stop = WorldToScreen(headPos);
        }
        
        // Get HP
        g_enemy.Hp = GetField<int>(playerPtr, 0x110);
        if (g_enemy.Hp <= 0) continue;
        
        // Get PlayerInfo
        g_enemy.playerinfo = GetField<void*>(playerPtr, 0x20); // Assuming get_PlayerInfo() returns field at 0x20
        if (g_enemy.playerinfo) {
            g_enemy.MaxHp = GetField<int>(g_enemy.playerinfo, 0x68);
        }
        
        // Get Name
        monoString* nameStr = GetField<monoString*>(playerPtr, 0x128);
        if (nameStr) {
            g_enemy.Name = nameStr->toCPPString();
        }
        
        // Calculate distance
        float distance = Vector3::Distance(g_selfPos, g_enemy.pos);
        
        // Calculate box dimensions
        const float clapped_distance = distance / g_clappedFloat;
        const float clapped = clapped_distance / g_clappedFloat;
        
        ImVec2 top_pos = WorldToScreen(g_enemy.pos + Vector3(0, g_topPosFloat + clapped_distance, 0));
        ImVec2 bot_pos = WorldToScreen(g_enemy.pos - Vector3(0, g_botPosFloat + clapped_distance, 0));
        
        float posTop = std::min(top_pos.x, bot_pos.x);
        float posBottom = std::max(top_pos.x, bot_pos.x);
        
        float calculatedPosition = fabs((top_pos.y - bot_pos.y) * (0.0092f / 0.019f) / (g_calculatedPositionFloat + clapped));
        
        ImRect rect(
            ImVec2(posTop - calculatedPosition - clapped_distance, top_pos.y),
            ImVec2(posBottom + calculatedPosition + clapped_distance, bot_pos.y)
        );
        
        ImRect mirrored_rect(
            ImVec2(posTop - calculatedPosition - clapped_distance, bot_pos.y),
            ImVec2(posBottom + calculatedPosition + clapped_distance, top_pos.y)
        );
        
        // Draw ESP elements
        if (g_ESPConfig.Line) {
            drawList->AddLine(g_enemy.w2stop, 
                            ImVec2(ImGui::GetIO().DisplaySize.x / 2, 0), 
                            ImColor(255, 0, 0, 255));
            drawList->AddRect(rect.Min, rect.Max, ImColor(255, 255, 255), 0, 0, 1);
            drawList->AddRect(rect.Min - ImVec2(1, 1), rect.Max + ImVec2(1, 1), ImColor(0, 0, 0), 0, 0, 1);
            drawList->AddRect(rect.Min + ImVec2(1, 1), rect.Max - ImVec2(1, 1), ImColor(0, 0, 0), 0, 0, 1);
        }
        
        if (g_ESPConfig.Hp) {
            std::string hpStr = std::to_string(g_enemy.Hp) + "/" + std::to_string(g_enemy.MaxHp);
            ImVec2 health_pos = {rect.Min.x, rect.Max.y};
            drawList->AddText(ImVec2(health_pos.x, health_pos.y), ImColor(255, 0, 0, 255), hpStr.c_str());
        }
        
        if (g_ESPConfig.Bones) {
            ImColor bonesColor(g_BonesColor[0], g_BonesColor[1], g_BonesColor[2], g_BonesColor[3]);
            auto* bodyParts = GetField<monoArray<void**>*>(playerPtr, 0x60);
            if (bodyParts) {
                std::vector<std::pair<int, int>> connections = {
                    {16, 10}, {15, 2}, {2, 0}, {0, 16}, {14, 3}, {3, 1}, {1, 16},
                    {16, 8}, {8, 11}, {11, 4}, {4, 6}, {6, 12}, {11, 5}, {5, 7}, {7, 13}
                };
                DrawBones(bodyParts, connections, bonesColor);
            }
        }
        
        if (g_ESPConfig.Distance) {
            std::string distStr = std::to_string((int)distance) + "M";
            ImVec2 distance_pos = {mirrored_rect.Min.x, mirrored_rect.Max.y};
            drawList->AddText(distance_pos, ImColor(255, 0, 0, 255), distStr.c_str());
        }
        
        if (g_ESPConfig.Name && !g_enemy.Name.empty()) {
            ImVec2 name_pos = {rect.Min.x, rect.Min.y - 20};
            drawList->AddText(name_pos, ImColor(255, 0, 0, 255), g_enemy.Name.c_str());
        }
    }
}
