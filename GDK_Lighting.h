#ifndef GDK_Lighting_H
#define GDK_Lighting_H



// --- 1. THE SHARED DATA (Standalone or System Linked) ---
namespace GDK_Internal {
    static GDK_Light lights[64];
    static int activeLightCount = 1;
    static glm::vec4 worldAmbient;
}

// Updated Helper: Mode-aware lockdown
inline GDK_Light* GetLightInternal(int index) {
    if (index <= 0 || index >= 64) return nullptr; 
    
    if (index >= GDK_Internal::activeLightCount) {
        GDK_Internal::activeLightCount = index + 1;
    }
    return &GDK_Internal::lights[index];
}

extern "C" {

    GDK_API void GDK_Scene_Clear(float r, float g, float b, float power) {
        GDK_Internal::activeLightCount = 1; 
        
        GDK_Light& amb = GDK_Internal::lights[0];
        amb.type = 3; // Global Ambient
        amb.color = {r, g, b};
        amb.power = power;
        amb.enabled = 1;

        GDK_Internal::worldAmbient = {r, g, b, power};

        // BRANCH: Legacy fixed-function
        if (GDK::mode == GDK_MODE_LEGACY) {
            float col[] = { r * power, g * power, b * power, 1.0f };
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, col);
        }
    }

    GDK_API void GDK_Light_SetType(int index, int type) {
        if (index == 0) return;
        if (auto* l = GetLightInternal(index)) l->type = type;
    }

    GDK_API void GDK_Light_SetPos(int index, float x, float y, float z) {
        if (index == 0) return;
        if (auto* l = GetLightInternal(index)) l->position = {x, y, z};
    }

    GDK_API void GDK_Light_SetDir(int index, float x, float y, float z) {
        if (auto* l = GetLightInternal(index)) {
            l->direction = glm::normalize(glm::vec3(x, y, z));
        }
    }

    GDK_API void GDK_Light_SetColor(int index, float r, float g, float b, float power) {
        if (auto* l = GetLightInternal(index)) {
            l->color = {r, g, b};
            l->power = power;
            l->enabled = 1;
        }
    }

    GDK_API void GDK_Light_SetSpot(int index, float inner, float outer) {
        if (index == 0) return;
        if (auto* l = GetLightInternal(index)) {
            l->innerCutoff = inner;
            l->outerCutoff = outer;
        }
    }

    GDK_API void GDK_Light_SetRange(int index, float range) {
        if (auto* l = GetLightInternal(index)) l->range = range;
    }

    GDK_API void GDK_Scene_Apply() {
        // --- MODE 0: LEGACY PATH ---
        if (GDK::mode == GDK_MODE_LEGACY) {
            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
            
            for (int i = 1; i < 8; i++) {
                GDK_Light& l = GDK_Internal::lights[i];
                uint32_t lID = GL_LIGHT0 + i;
                
                if (l.enabled) {
                    glEnable(lID);
                    float pos[] = { l.position.x, l.position.y, l.position.z, (l.type == 2 ? 0.0f : 1.0f) };
                    float col[] = { l.color.r * l.power, l.color.g * l.power, l.color.b * l.power, 1.0f };
                    
                    glLightfv(lID, GL_POSITION, pos);
                    glLightfv(lID, GL_DIFFUSE, col);
                    
                    if (l.type == 1) { // Spotlight
                        float dir[] = { l.direction.x, l.direction.y, l.direction.z };
                        glLightfv(lID, GL_SPOT_DIRECTION, dir);
                        glLightf(lID, GL_SPOT_CUTOFF, l.outerCutoff); 
                        glLightf(lID, GL_SPOT_EXPONENT, 15.0f); 
                        glLightf(lID, GL_CONSTANT_ATTENUATION, 1.0f);
                        glLightf(lID, GL_LINEAR_ATTENUATION, 0.0f); 
                        glLightf(lID, GL_QUADRATIC_ATTENUATION, 0.0f);
                    }
                } else {
                    glDisable(lID);
                }
            }
        }
        
        // --- MODE 1 & 2: MODERN PATH ---
        // Shaders or AZDO UBOs will pull from GDK_Internal::lights directly.
        // No hardware sync needed here for Modern modes.
    }
}

#endif
