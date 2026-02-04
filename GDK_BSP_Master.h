
#ifndef GDK_MAP_MASTER_H
#define GDK_MAP_MASTER_H
// --- THE MASTER MAP REGISTRY ---
enum GDK_MapType { MAP_NONE = 0, Q1_BSP = 1, Q2_BSP = 2, Q3_BSP = 3 };

struct GDK_Map_Master {
    GDK_MapType type;
    int internalIdx;
};

static std::vector<GDK_Map_Master> g_MapMaster; // The Universal Map Registry

GDK_API int GDK_Map_Load(const char* mPath) {
    std::ifstream file(mPath, std::ios::binary);
    if (!file) return -1;

    int32_t version;
    file.read((char*)&version, 4);
    file.close();

    GDK_Map_Master masterRecord = { MAP_NONE, -1 };
    bool success = false;

    if (version == 29) {
        int slot = -1;
        for (int i = 0; i < (int)g_Q1MapStore.size(); ++i) {
            if (!g_Q1MapStore[i].InUse) { slot = i; break; }
        }
        if (slot == -1) {
            g_Q1MapStore.push_back(GDK_Q1_Map());
            slot = (int)g_Q1MapStore.size() - 1;
        }

        GDK_Q1_Map& m = g_Q1MapStore[slot];
        m.Free(); 

        if (GDK_Internal_BSP1_Load(mPath, m)) {
            masterRecord.type = Q1_BSP;
            masterRecord.internalIdx = slot;
            m.InUse = true;
            success = true;
        }
    }

    if (!success) return -1;

    // Register in Master Registry
    int masterIdx = -1;
    for (int i = 0; i < (int)g_MapMaster.size(); ++i) {
        if (g_MapMaster[i].type == MAP_NONE) { masterIdx = i; break; }
    }
    
    if (masterIdx == -1) {
        g_MapMaster.push_back(masterRecord);
        masterIdx = (int)g_MapMaster.size() - 1;
    } else {
        g_MapMaster[masterIdx] = masterRecord;
    }

    return masterIdx;
}


GDK_API void GDK_Map_Draw(int mIdx) {
    if (mIdx < 0 || (size_t)mIdx >= g_MapMaster.size()) return;
    
    GDK_Map_Master& master = g_MapMaster[mIdx];
    if (master.type == MAP_NONE || master.internalIdx == -1) return;

    switch (master.type) {
        case Q1_BSP:
            // Final safety check before jumping into the vertex loop
            if ((size_t)master.internalIdx < g_Q1MapStore.size()) {
                GDK_Internal_BSP1_Draw(master.internalIdx);
            }
            break;
    }
}

// Don't forget a way to nuke the map from BASIC!
GDK_API void GDK_Map_Free(int mIdx) {
    if (mIdx < 0 || (size_t)mIdx >= g_MapMaster.size()) return;
    GDK_Map_Master& master = g_MapMaster[mIdx];

    if (master.type == Q1_BSP) {
        g_Q1MapStore[master.internalIdx].Free();
    }
    
    master.type = MAP_NONE;
    master.internalIdx = -1;
}

#endif

