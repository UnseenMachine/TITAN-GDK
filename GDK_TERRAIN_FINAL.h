#ifndef GDK_TERRAIN_H
#define GDK_TERRAIN_H

// --- Terrain System ---
#pragma pack(push, 1)
struct TerrainVertex {
    float x, y, z;    // Position First (S-Tier Standard)
    float nx, ny, nz; // Normals Second
    float u, v;       // UVs Last
};
#pragma pack(pop)

struct GDK_Internal_Terrain {
    uint32_t textureID;
    uint32_t displayList; // Legacy (Mode 0)
    uint32_t vao, vbo, ebo; // Standard/AZDO (Mode 1 & 2)
    void* azdoPtr;        // Persistent Mapping (Mode 2)
    
    int width, height;
    float scaleXZ, scaleY;
    uint32_t indexCount;
};

static std::vector<GDK_Internal_Terrain> g_Terrains;
static std::vector<std::vector<float>> g_HeightGrid;

// 2026 Optimized Normal Calculation (Baking scale in)
static glm::vec3 CalculateNormal(int x, int z, int w, int h, const std::vector<float>& heights, float scaleXZ) {
    float hL = (x > 0) ? heights[z * w + (x - 1)] : heights[z * w + x];
    float hR = (x < w - 1) ? heights[z * w + (x + 1)] : heights[z * w + x];
    float hD = (z > 0) ? heights[(z - 1) * w + x] : heights[z * w + x];
    float hU = (z < h - 1) ? heights[(z + 1) * w + x] : heights[z * w + x];
    // ScaleXZ factors into the cross product intensity
    return glm::normalize(glm::vec3(hL - hR, 2.0f * scaleXZ, hD - hU));
}


// Internal global for the terrain shader
static uint32_t g_TerrainShader = 0;

// Internal accessor that the terrain render call is looking for
static inline uint32_t GDK_GetTerrainShader() {
    return g_TerrainShader;
}

// Simple Box Blur for Heightmap Smoothing Makes a big difference!
void ApplyBoxBlur(unsigned char* data, int width, int height, int kernelSize) {
    // We create a temporary buffer to store the blurred results
    unsigned char* temp_data = new unsigned char[width * height];
    
    int halfKernel = kernelSize / 2;

    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;
            int count = 0;

            // Iterate over the kernel window
            for (int kz = -halfKernel; kz <= halfKernel; kz++) {
                for (int kx = -halfKernel; kx <= halfKernel; kx++) {
                    int sampleX = x + kx;
                    int sampleZ = z + kz;

                    // Check boundaries
                    if (sampleX >= 0 && sampleX < width && sampleZ >= 0 && sampleZ < height) {
                        sum += data[sampleZ * width + sampleX];
                        count++;
                    }
                }
            }
            temp_data[z * width + x] = (unsigned char)(sum / count);
        }
    }
    
    // Copy the blurred data back to the original array
    memcpy(data, temp_data, width * height * sizeof(unsigned char));
    delete[] temp_data;
}



GDK_BEGIN_DECLS

GDK_API int GDK_LoadTerrain(const char* heightmapPath, const char* texturePath, float scaleXZ, float scaleY) {
    int imgW, imgH, imgC;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char* data = stbi_load(heightmapPath, &imgW, &imgH, &imgC, 1);
    if (!data) return -1;
    ApplyBoxBlur(data, imgW, imgH, 3); // Use a kernel size of 3
    ApplyBoxBlur(data, imgW, imgH, 3); // Use a kernel size of 3
    ApplyBoxBlur(data, imgW, imgH, 3); // Use a kernel size of 3

    GDK_Internal_Terrain terrain = {};
    terrain.width = imgW; terrain.height = imgH;
    terrain.scaleXZ = scaleXZ; terrain.scaleY = scaleY;
    terrain.textureID = GDK_LoadTexture(texturePath);

    std::vector<TerrainVertex> verts(imgW * imgH);
    std::vector<float> rawHeights(imgW * imgH);
    g_HeightGrid.assign(imgW, std::vector<float>(imgH));

    // 1. Bake Vertex Positions and Store HeightGrid
    for (int z = 0; z < imgH; z++) {
        for (int x = 0; x < imgW; x++) {
            int i = z * imgW + x;
            float worldY = ((float)data[i] / 255.0f) * 15.0f * scaleY;
            verts[i].x = (float)x * scaleXZ;
            verts[i].y = worldY;
            verts[i].z = (float)z * scaleXZ;
            verts[i].u = (float)x / (imgW - 1);
            verts[i].v = (float)z / (imgH - 1);
            
            rawHeights[i] = worldY;
            g_HeightGrid[x][z] = worldY;
        }
    }
    stbi_image_free(data);

    // 2. Calculate Baked Normals
    for (int z = 0; z < imgH; z++) {
        for (int x = 0; x < imgW; x++) {
            glm::vec3 n = CalculateNormal(x, z, imgW, imgH, rawHeights, scaleXZ);
            verts[z * imgW + x].nx = n.x;
            verts[z * imgW + x].ny = n.y;
            verts[z * imgW + x].nz = n.z;
        }
    }

    // 3. Index Generation
    std::vector<uint32_t> indices;
    for (int z = 0; z < imgH - 1; z++) {
        for (int x = 0; x < imgW - 1; x++) {
            uint32_t v1 = z * imgW + x;
            uint32_t v2 = z * imgW + (x + 1);
            uint32_t v3 = (z + 1) * imgW + x;
            uint32_t v4 = (z + 1) * imgW + (x + 1);
            indices.insert(indices.end(), { v1, v3, v4, v1, v4, v2 });
        }
    }
    terrain.indexCount = (uint32_t)indices.size();

    // 4. Branching Render Mode Setup
    if (GDK::mode == GDK_MODE_LEGACY) {
        terrain.displayList = glGenLists(1);
        glNewList(terrain.displayList, GL_COMPILE);
        glBegin(GL_TRIANGLES);
        for (auto idx : indices) {
            glNormal3f(verts[idx].nx, verts[idx].ny, verts[idx].nz);
            glTexCoord2f(verts[idx].u, verts[idx].v);
            glVertex3f(verts[idx].x, verts[idx].y, verts[idx].z);
        }
        glEnd();
        glEndList();
    } 
    else {
        glGenVertexArrays(1, &terrain.vao);
        glGenBuffers(1, &terrain.vbo);
        glGenBuffers(1, &terrain.ebo);
        glBindVertexArray(terrain.vao);

        glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);
        if (GDK::mode == GDK_MODE_AZDO) {
            // Persistent Mapping for 2026 Hardware
            GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            glBufferStorage(GL_ARRAY_BUFFER, verts.size() * sizeof(TerrainVertex), verts.data(), flags);
            terrain.azdoPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(TerrainVertex), flags);
        } else {
            glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(TerrainVertex), verts.data(), GL_STATIC_DRAW);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0); // Pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, x));
        glEnableVertexAttribArray(1); // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, nx));
        glEnableVertexAttribArray(2); // Tex
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, u));
        glBindVertexArray(0);
    }

    g_Terrains.push_back(terrain);
    return (int)g_Terrains.size() - 1;
}

GDK_API float GDK_GetTerrainHeight(int terrainIdx, float worldX, float worldZ) {
    if (terrainIdx < 0 || (size_t)terrainIdx >= g_Terrains.size()) return 0.0f;
    auto& t = g_Terrains[terrainIdx];

    float gridX = worldX / t.scaleXZ;
    float gridZ = worldZ / t.scaleXZ;

    int x0 = (int)std::floor(gridX);
    int z0 = (int)std::floor(gridZ);
    
    if (x0 < 0 || x0 >= t.width - 1 || z0 < 0 || z0 >= t.height - 1) return 0.0f;

    float fracX = gridX - (float)x0;
    float fracZ = gridZ - (float)z0;

    // Data is already pre-scaled in g_HeightGrid
    float h00 = g_HeightGrid[x0][z0];
    float h10 = g_HeightGrid[x0 + 1][z0];
    float h01 = g_HeightGrid[x0][z0 + 1];
    float h11 = g_HeightGrid[x0 + 1][z0 + 1];

    float top = h00 + (h10 - h00) * fracX;
    float bottom = h01 + (h11 - h01) * fracX;
    return top + (bottom - top) * fracZ;
}

GDK_API void GDK_RenderTerrain(int terrainIdx) {
    if (terrainIdx < 0 || (size_t)terrainIdx >= g_Terrains.size()) return;
    auto& t = g_Terrains[terrainIdx];

    // 1. Get the Mode-Specific Key
    uint64_t texKey = g_Textures[t.textureID];

    // 2. Texture Application Branch
    if (GDK::mode == GDK_MODE_AZDO) {
        // AZDO: No binding! Just upload the 64-bit handle to the shader uniform.
        // This is extremely fast as it involves zero driver state changes.
        uint32_t shader = GDK_GetTerrainShader(); 
        glUseProgram(shader);
        glUniformHandleui64ARB(glGetUniformLocation(shader, "u_Texture"), texKey);
    } 
    else {
        // LEGACY/STANDARD: Standard binding.
        glEnable(GL_TEXTURE_2D); // Required for Mode 0
        glBindTexture(GL_TEXTURE_2D, texKey);
    }

    // 3. Drawing Branch
    if (GDK::mode == GDK_MODE_LEGACY) {
        glCallList(t.displayList);
    } else {
        glBindVertexArray(t.vao);
        glDrawElements(GL_TRIANGLES, t.indexCount, GL_UNSIGNED_INT, 0);
    }
}


GDK_END_DECLS

#endif 
