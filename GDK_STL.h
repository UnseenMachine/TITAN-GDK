#ifndef GDK_STL_H
#define GDK_STL_H

struct GDK_STL_Model {
    bool InUse = false;
    uint32_t numTris = 0;
    std::vector<GDK_Legacy_Vert> vertices; 

    void Free() {
        vertices.clear();
        InUse = false;
        numTris = 0;
    }
};

static std::vector<GDK_STL_Model> g_STLStore; 

// Quake 3/Binary STL structures must be byte-aligned for direct file reading
#pragma pack(push, 1)
struct STL_Triangle {
    float normal[3];   // Facet Normal
    float vertex1[3];  // Vertex 1
    float vertex2[3];  // Vertex 2
    float vertex3[3];  // Vertex 3
    uint16_t attribute; // Attribute byte count (usually 0)
};
#pragma pack(pop)




// 1. Slot Finder: Manages the internal g_STLStore array
static int GDK_Internal_GetFreeSTLSlot() {
    for (int i = 0; i < (int)g_STLStore.size(); ++i) {
        if (!g_STLStore[i].InUse) return i;
    }
    g_STLStore.push_back(GDK_STL_Model());
    return (int)g_STLStore.size() - 1;
}

// 2. Internal Draw: Pure Legacy 1.1 Geometry
static void GDK_Internal_STL_Draw(int internalIdx) {
    if (internalIdx < 0 || (size_t)internalIdx >= g_STLStore.size()) return;
    GDK_STL_Model& m = g_STLStore[internalIdx];
    if (!m.InUse || m.vertices.empty()) return;

    glDisable(GL_TEXTURE_2D); 
    
    glBegin(GL_TRIANGLES);
    for (const auto& v : m.vertices) {
        glNormal3f(v.nx, v.ny, v.nz);
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();
}

// 3. Binary Loader
static bool GDK_Internal_LoadSTL(const char* mPath, GDK_STL_Model& model) {
    std::ifstream file(mPath, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(80, std::ios::beg); // Skip header

    uint32_t numTris = 0;
    file.read((char*)&numTris, 4);
    if (numTris == 0) { file.close(); return false; }

    model.numTris = numTris;
    model.vertices.reserve(numTris * 3);

    for (uint32_t i = 0; i < numTris; ++i) {
        STL_Triangle tri;
        file.read((char*)&tri, sizeof(STL_Triangle));

        for (int v = 0; v < 3; ++v) {
            GDK_Legacy_Vert gv;
            float* p;
            if (v == 0) p = (float*)&tri.vertex1;
            else if (v == 1) p = (float*)&tri.vertex2;
            else p = (float*)&tri.vertex3;

            gv.x = p[0]; gv.y = p[1]; gv.z = p[2];
            gv.nx = tri.normal[0]; gv.ny = tri.normal[1]; gv.nz = tri.normal[2];
            gv.u = 0; gv.v = 0;
            model.vertices.push_back(gv);
        }
    }

    file.close();
    return true;
}

#endif
