#ifndef BSP_ID_TECH_H
#define BSP_ID_TECH_H

// --- QUAKE 1 BSP STRUCTURES ---
#pragma pack(push, 1)
struct BSP1_Lump {
    int32_t offset;
    int32_t length;
};

struct BSP1_Header {
    int32_t version;           // Must be 29
    BSP1_Lump lumps[15];       // Exactly 15 for Q1/v29
};
struct BSP1_TexLump {
    int32_t num_textures;
};

struct BSP1_MipTex {
    char name[16];
    uint32_t width, height;
    uint32_t offsets[4];     
};

struct BSP1_Vertex { float x, y, z; };

struct BSP1_Edge { uint16_t v[2]; };
struct BSP1_Face {
    int16_t plane_id;
    int16_t side;
    int32_t first_edge; // This is 4 bytes in both
    int16_t num_edges;  // THIS IS 2 BYTES IN BSP1
    int16_t tex_info_id;// THIS IS 2 BYTES IN BSP1
    uint8_t styles[4];
    int32_t light_offset;
};

struct BSP1_TexInfo {
    float s[4], t[4]; 
    int32_t miptex_id;
    int32_t flags;
};
#pragma pack(pop)

// --- INTERNAL STORAGE ---
struct GDK_Q1_Map {
    bool InUse = false;
    std::vector<uint32_t> textureIDs;
    std::vector<uint32_t> texWidths;
    std::vector<uint32_t> texHeights;
    
    struct FaceBatch {
        uint32_t texID;
        std::vector<GDK_Legacy_Vert> verts;
    };
    std::vector<FaceBatch> renderBatches;

    void Free() {
        for(auto t : textureIDs) {
            if (t > 0) glDeleteTextures(1, (GLuint*)&t);
        }
        textureIDs.clear();
        texWidths.clear();
        texHeights.clear();
        renderBatches.clear();
        InUse = false;
    }
};

static std::vector<GDK_Q1_Map> g_Q1MapStore;

// --- INTERNAL HELPERS ---
static void GDK_Internal_CreateBSPTexture(uint8_t* rgb, int w, int h, uint32_t& target) { 
    if (!rgb || w <= 0 || h <= 0) { target = 0; return; }

    GLuint tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Wrap (Quake uses atlases/UVs that rely on this)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 

    target = tid;

    // DEBUG PRINT: Check if texture generation succeeded
    printf("GDK_CreateBSPTexture: Generated GL_ID %d for size %dx%d\n", target, w, h);
}


// --- INTERNAL LOADERS ---
static bool GDK_Internal_BSP1_LoadTextures(std::ifstream& file, int texLumpOffset, GDK_Q1_Map& map) {
    file.seekg(texLumpOffset, std::ios::beg);
    BSP1_TexLump texHeader;
    file.read((char*)&texHeader, sizeof(BSP1_TexLump));

    std::vector<int32_t> texOffsets(texHeader.num_textures);
    file.read((char*)texOffsets.data(), texHeader.num_textures * sizeof(int32_t));

    map.textureIDs.resize(texHeader.num_textures, 0);
    map.texWidths.resize(texHeader.num_textures, 0);
    map.texHeights.resize(texHeader.num_textures, 0);

    for (int i = 0; i < texHeader.num_textures; ++i) {
        if (texOffsets[i] == -1) continue;

        // Use the lump offset + the texture specific offset
        file.seekg(texLumpOffset + texOffsets[i], std::ios::beg);
        BSP1_MipTex mip;
        file.read((char*)&mip, sizeof(BSP1_MipTex));

        map.texWidths[i] = mip.width;
        map.texHeights[i] = mip.height;

        int dataSize = mip.width * mip.height;
        if (dataSize <= 0) continue;

        std::vector<uint8_t> pixels(dataSize);
        // Correctly seek to the first mipmap level's data
        file.seekg(texLumpOffset + texOffsets[i] + mip.offsets[0], std::ios::beg);
        file.read((char*)pixels.data(), dataSize);
        
        // Safety check for file read
        if (file.fail()) break; 

        std::vector<uint8_t> rgbBuffer(dataSize * 3);
        for (int p = 0; p < dataSize; ++p) {
            int palIdx = pixels[p] * 3;
            rgbBuffer[p * 3 + 0] = g_Q1_Palette[palIdx + 0];
            rgbBuffer[p * 3 + 1] = g_Q1_Palette[palIdx + 1];
            rgbBuffer[p * 3 + 2] = g_Q1_Palette[palIdx + 2];
        }

        uint32_t glHandle = 0;
        GDK_Internal_CreateBSPTexture(rgbBuffer.data(), mip.width, mip.height, glHandle);
        map.textureIDs[i] = glHandle;
    }
    return true;
}

static bool GDK_Internal_BSP1_LoadGeometry(std::ifstream& file, const BSP1_Header& h, GDK_Q1_Map& map) {
    auto LoadLump = [&](int lumpID, auto& vec, size_t sz) {
        size_t len = static_cast<size_t>(h.lumps[lumpID].length);
        size_t off = static_cast<size_t>(h.lumps[lumpID].offset);
        if (len == 0) return;
        file.seekg(off, std::ios::beg);
        vec.resize(len / sz);
        file.read((char*)vec.data(), len);
    };

    std::vector<BSP1_Vertex> rawVerts; LoadLump(3, rawVerts, sizeof(BSP1_Vertex));
    std::vector<BSP1_Edge>   rawEdges; LoadLump(12, rawEdges, sizeof(BSP1_Edge));
    std::vector<int32_t>     rawSurf;  LoadLump(13, rawSurf, sizeof(int32_t));
    std::vector<BSP1_Face>   rawFaces; LoadLump(7, rawFaces, sizeof(BSP1_Face));
    std::vector<BSP1_TexInfo> rawTexI; LoadLump(6, rawTexI, sizeof(BSP1_TexInfo));

    if (rawFaces.empty() || rawVerts.empty()) return false;

    // Clear and prepare batches for each texture slot
    map.renderBatches.clear();
    for (size_t i = 0; i < map.textureIDs.size(); ++i) {
        GDK_Q1_Map::FaceBatch batch;
        batch.texID = map.textureIDs[i];
        map.renderBatches.push_back(batch);
    }

    for (const auto& f : rawFaces) {
        if (f.tex_info_id < 0 || (size_t)f.tex_info_id >= rawTexI.size()) continue;
        const auto& ti = rawTexI[f.tex_info_id];
        
        // Map miptex_id to our batch list
        if (ti.miptex_id < 0 || (size_t)ti.miptex_id >= map.renderBatches.size()) continue;
        auto& batch = map.renderBatches[ti.miptex_id];
        
        float tw = (float)map.texWidths[ti.miptex_id];
        float th = (float)map.texHeights[ti.miptex_id];
        if (tw < 1.0f) tw = 1.0f; if (th < 1.0f) th = 1.0f;

        std::vector<GDK_Legacy_Vert> faceVerts;
        for (int i = 0; i < f.num_edges; ++i) {
            int32_t sIdx = f.first_edge + i;
            int32_t sEdge = rawSurf[sIdx];

            // Quake Edge Logic: Positive = Edge Start->End, Negative = Edge End->Start
            uint32_t edgeIdx = (uint32_t)abs(sEdge);
            if (edgeIdx >= rawEdges.size()) continue;

            int vIdx = (sEdge >= 0) ? rawEdges[edgeIdx].v[0] : rawEdges[edgeIdx].v[1];
            const auto& rv = rawVerts[vIdx];

            GDK_Legacy_Vert gv;
            gv.x = rv.x; gv.y = rv.z; gv.z = -rv.y; // Standard Quake -> GL Coordinate Swap

            gv.u = (rv.x * ti.s[0] + rv.y * ti.s[1] + rv.z * ti.s[2] + ti.s[3]) / tw;
            gv.v = (rv.x * ti.t[0] + rv.y * ti.t[1] + rv.z * ti.t[2] + ti.t[3]) / th;
            gv.nx = 0; gv.ny = 1; gv.nz = 0; // Temp normal
            faceVerts.push_back(gv);
        }

        // Fan Triangulation for the N-Gon
        if (faceVerts.size() >= 3) {
            for (size_t i = 1; i < faceVerts.size() - 1; ++i) {
                batch.verts.push_back(faceVerts[0]);
                batch.verts.push_back(faceVerts[i]);
                batch.verts.push_back(faceVerts[i + 1]);
            }
        }
    }
    return true;
}

static bool GDK_Internal_BSP1_Load(const char* path, GDK_Q1_Map& m) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    BSP1_Header h;
    file.read((char*)&h, sizeof(h));
    if (h.version != 29) { file.close(); return false; }

    // Use Lump Index 2 for Quake 1 Textures
    if (!GDK_Internal_BSP1_LoadTextures(file, h.lumps[2].offset, m)) {
        file.close();
        return false;
    }
    
    // Lump Index 7 for Faces, 3 for Vertices etc.
    if (!GDK_Internal_BSP1_LoadGeometry(file, h, m)) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

static void GDK_Internal_BSP1_Draw(int internalIdx) {
    if (internalIdx < 0 || (size_t)internalIdx >= g_Q1MapStore.size()) return;
    GDK_Q1_Map& m = g_Q1MapStore[internalIdx];
    if (!m.InUse) return;

    // --- CRITICAL OPENGL FLAGS ---
    glEnable(GL_TEXTURE_2D);

    for (auto& batch : m.renderBatches) {
        if (batch.verts.empty()) continue;
        
        // This is where we bind the specific texture ID from your struct
        glBindTexture(GL_TEXTURE_2D, batch.texID);
        
        glBegin(GL_TRIANGLES);
        for (const auto& v : batch.verts) {
            glTexCoord2f(v.u, v.v);
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();
    }
    glBindTexture(GL_TEXTURE_2D, 0); 
    glDisable(GL_TEXTURE_2D); // Good practice to disable when done
}

#endif
