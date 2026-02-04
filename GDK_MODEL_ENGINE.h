#ifndef GDK_MODEL_ENGINE_H
#define GDK_MODEL_ENGINE_H


// --- 1. THE UNIFIED REGISTRY ---

struct GDK_Model_Master {
    int TypeID;        // GDK_ModelType ID
    int InternalIndex; // Index within the type-specific storage array
};

static std::vector<GDK_Model_Master> gdk_models; // The Universal Master Registry

// --- 2. THE UNIVERSAL LOADER ---
GDK_BEGIN_DECLS

static int GDK_Internal_GetFreeLegacySlot() {
    // 1. Look for an empty hole to reuse memory
    for (int i = 0; i < (int)g_ModelStore.size(); ++i) {
        if (!g_ModelStore[i].InUse) return i;
    }
    // 2. If no holes, expand the vector
    g_ModelStore.push_back(GDK_Legacy_Model());
    return (int)g_ModelStore.size() - 1;
}

// Returns the number of frames (MDL/MD2/MD3) or meshes (OBJ) in the model
GDK_API int GDK_Model_Get_Frames(int mIdx) {
    if (mIdx < 0 || (size_t)mIdx >= gdk_models.size()) return 0;
    auto& master = gdk_models[mIdx];
    if (master.TypeID >= MDL && master.TypeID <= MD3) {
        return g_ModelStore[master.InternalIndex].numFrames;
    }
    return 1; // OBJ/STL usually 1 frame
}

GDK_API void GDK_Model_AddAnim(int mIdx, int tag, int start, int end, float fps) {
    // 1. Check if the Master Index is valid
    if (mIdx < 0 || (size_t)mIdx >= gdk_models.size()) return;

    // 2. Get the pointer to our internal storage index
    GDK_Model_Master &master = gdk_models[mIdx];

    // 3. Routing: Only legacy models (MDL, MD2, MD3) use g_ModelStore
    if (master.TypeID == MDL || master.TypeID == MD2 || master.TypeID == MD3) {
        
        int internalIdx = master.InternalIndex;
        
        // Safety check on the internal legacy array
        if (internalIdx < 0 || (size_t)internalIdx >= g_ModelStore.size()) return;

        GDK_Legacy_Model &m = g_ModelStore[internalIdx];

        // 4. Populate the animation data
        GDK_Animation a;
        a.Start    = start; 
        a.End      = end; 
        a.Current  = start;
        a.Speed    = fps; 
        a.Timer    = 0.0f; 
        a.Lerp     = 0.0f; 
        a.Looping  = true; 
        a.Priority = 0;

        // 5. Save it into the routed model's library
        m.animLibrary[tag] = a;
        m.NumAnims = (int)m.animLibrary.size();
    }
    // Else: If you later add FBX or GLTF to a different store, you'd add another 'else if' here
}

GDK_API int GDK_Model_Load(const char* mPath, const char* tPath) {
    std::string p = mPath;
    bool success = false;
    GDK_Model_Master masterRecord = { TYPE_NONE, -1 };

    // --- 1. EXTENSION SNIFF (Prioritise Loose/Static Formats) ---
    if (p.find(".stl") != std::string::npos || p.find(".STL") != std::string::npos) {
    int slot = GDK_Internal_GetFreeSTLSlot(); // <--- CLEAN
    GDK_STL_Model& m = g_STLStore[slot];
    m.Free();

    if (GDK_Internal_LoadSTL(mPath, m)) {
        m.InUse = true;
        masterRecord.TypeID = STL;
        masterRecord.InternalIndex = slot;
        success = true;
    }
    }
    else if (p.find(".txt") != std::string::npos) {
        // Let the loader find the slot and do the heavy lifting
        int slot = PRM::PRM_LoadRevoltCar(mPath); 
        
        if (slot != -1) {
            // Success! slot is the index of the newly loaded car
            masterRecord.TypeID = REVOLT; 
            masterRecord.InternalIndex = slot;
            success = true;
        } else {
            // Fail - likely file not found or mesh load error
            success = false;
        }
    }

    // --- 2. MAGIC SNIFF (For Legacy Binary Formats) ---
    else {
        std::ifstream file(mPath, std::ios::binary);
        if (!file) return -1;
        char magic[4] = {0};
        file.read(magic, 4);
        file.close();

        if (strncmp(magic, "IDPO", 4) == 0 || strncmp(magic, "IDST", 4) == 0 || 
            strncmp(magic, "IDP2", 4) == 0 || strncmp(magic, "IDP3", 4) == 0) {
            
            int internalIdx = GDK_Internal_GetFreeLegacySlot();
            GDK_Legacy_Model& m = g_ModelStore[internalIdx];
            m.Free();

            if (tPath && strlen(tPath) > 0) {
                GDK_LoadTexture(tPath, (int&)m.defaultTex);
            }
            
            if (strncmp(magic, "IDP2", 4) == 0) {
                m.type = MD2;
                success = GDK_Internal_LoadMD2(mPath, m);
            } else if (strncmp(magic, "IDP3", 4) == 0) {
                m.type = MD3;
                success = GDK_Internal_LoadMD3(mPath, m);
            } else {
                m.type = MDL;
                success = GDK_Internal_LoadMDL(mPath, m); 
            }

            if (success) {
                m.InUse = true;
                masterRecord.TypeID = m.type;
                masterRecord.InternalIndex = internalIdx;
            }
        }
    }

    if (!success) return -1;

    // --- 3. MASTER RECORD REUSE ---
    int masterIdx = -1;
    for (int i = 0; i < (int)gdk_models.size(); ++i) {
        if (gdk_models[i].TypeID == TYPE_NONE) { masterIdx = i; break; }
    }
    if (masterIdx == -1) {
        gdk_models.push_back(masterRecord);
        masterIdx = (int)gdk_models.size() - 1;
    } else {
        gdk_models[masterIdx] = masterRecord;
    }

    return masterIdx; 
}


// --- 3. THE UNIVERSAL DRAW ---
GDK_API void GDK_Model_Draw(int mIdx, int texOverride, int frameOrMesh) {
    // 1. Master Index Safety
    if (mIdx < 0 || (size_t)mIdx >= gdk_models.size()) return;
    
    GDK_Model_Master& master = gdk_models[mIdx];

    switch (master.TypeID) {
        case MDL: 
        case MD2: 
        case MD3: {
            // Check internal legacy storage
            if (master.InternalIndex < 0 || (size_t)master.InternalIndex >= g_ModelStore.size()) return;
            GDK_Legacy_Model& m = g_ModelStore[master.InternalIndex];

            // Safety: Don't draw if not in use or if frames weren't loaded
            if (!m.InUse || m.frames.empty()) return;

            // Frame selection logic
            int f = (frameOrMesh < 0) ? 0 : frameOrMesh % m.numFrames;
            if (f >= (int)m.frames.size()) f = 0;
            
            // Texture Resolution
            uint32_t activeID = 0;
            if (texOverride >= 0 && (size_t)texOverride < g_Textures.size()) {
                activeID = g_Textures[texOverride];
            } else {
                activeID = (uint32_t)m.defaultTex;
            }

            if (activeID > 0) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, activeID);
            }

            // Draw unrolled frames
            glBegin(GL_TRIANGLES);
            for (const auto& v : m.frames[f]) {
                glNormal3f(v.nx, v.ny, v.nz);   
                glTexCoord2f(v.u, v.v);        
                glVertex3f(v.x, v.y, v.z);     
            }
            glEnd();
            
            if (activeID > 0) glDisable(GL_TEXTURE_2D);
        } break;

        case STL: {
            // Direct call to the simplified STL renderer
            GDK_Internal_STL_Draw(master.InternalIndex);
        } break;
        case REVOLT: {
            PRM::PRM_Draw(master.InternalIndex);
        } break;

        default:
            // Placeholder for OBJ/other future types
            break;
    }
}


GDK_API void GDK_Model_Free(int mIdx) {
    if (mIdx < 0 || (size_t)mIdx >= gdk_models.size()) return;
    
    GDK_Model_Master& master = gdk_models[mIdx];
    
    // Only legacy models (MDL, MD2, MD3) currently use g_ModelStore
    if (master.TypeID == MDL || master.TypeID == MD2 || master.TypeID == MD3) {
        if (master.InternalIndex >= 0 && (size_t)master.InternalIndex < g_ModelStore.size()) {
            g_ModelStore[master.InternalIndex].Free();
        }
    }
    
    // Mark the Master Record as empty so it can be reused too
    master.TypeID = TYPE_NONE;
    master.InternalIndex = -1;
}


GDK_END_DECLS
#endif
