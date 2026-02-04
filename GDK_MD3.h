#ifndef GDK_MD3_H
#define GDK_MD3_H


// Helper function to decompress MD3 normal shorts
static glm::vec3 DecompressMD3Normal(int16_t normal) {
    float lat = (float)((normal >> 8) & 0xFF) * (2.0f * glm::pi<float>()) / 255.0f;
    float lng = (float)((normal >> 0) & 0xFF) * (2.0f * glm::pi<float>()) / 255.0f;
    
    glm::vec3 out;
    out.x = glm::cos(lat) * glm::sin(lng);
    out.y = glm::sin(lat) * glm::sin(lng);
    out.z = glm::cos(lng);
    return out;
}
static bool GDK_Internal_LoadMD3(const char* mPath, GDK_Legacy_Model& model) {
    std::ifstream file(mPath, std::ios::binary);
    if (!file.is_open()) return false;

    // Ensure we start at the beginning since the router already peeked at the magic
    file.seekg(0, std::ios::beg);

    MD3_Header h;
    file.read((char*)&h, sizeof(MD3_Header));

    // Basic safety check for version/ident
    if (h.version != 15) { file.close(); return false; }

    // 1. Critical Meta-Data
    model.type = MD3;
    model.numFrames = h.num_frames;
    model.numTags = h.num_tags;
    model.numTris = 0;
    model.numVerts = 0;
    
    // Pre-size frames vector to prevent crashes during push_back
    model.frames.assign(h.num_frames, std::vector<GDK_Legacy_Vert>());

    // 2. Initialize Hierarchy
    if (!model.hierarchy) {
        model.hierarchy = new GDK_MD3_Hierarchy();
    }
    model.hierarchy->tagFrames.resize(h.num_frames);
    
    // Load Tags
    file.seekg(h.offset_tags);
    for (int i = 0; i < h.num_frames; ++i) {
        model.hierarchy->tagFrames[i].resize(h.num_tags);
        for (int j = 0; j < h.num_tags; ++j) {
            MD3_Tag rawTag;
            file.read((char*)&rawTag, sizeof(MD3_Tag));
            model.hierarchy->tagFrames[i][j].name = rawTag.name;
            model.hierarchy->tagFrames[i][j].pos  = rawTag.pos;
            model.hierarchy->tagFrames[i][j].axis = rawTag.axis;
        }
    }

    // 3. Surface Unrolling
    int surfaceOffset = h.offset_surfaces;
    for (int s = 0; s < h.num_surfaces; ++s) {
        file.seekg(surfaceOffset);
        MD3_Surface surf;
        file.read((char*)&surf, sizeof(MD3_Surface));

        // Load Buffers
        std::vector<MD3_Triangle> sTris(surf.num_tris);
        file.seekg(surfaceOffset + surf.offset_tris);
        file.read((char*)sTris.data(), surf.num_tris * sizeof(MD3_Triangle));

        std::vector<MD3_TexCoord> sUVs(surf.num_verts);
        file.seekg(surfaceOffset + surf.offset_st);
        file.read((char*)sUVs.data(), surf.num_verts * sizeof(MD3_TexCoord));

        std::vector<MD3_Vertex> sVerts(surf.num_verts * surf.num_frames);
        file.seekg(surfaceOffset + surf.offset_xyznormal);
        file.read((char*)sVerts.data(), sVerts.size() * sizeof(MD3_Vertex));

        // Unroll: 1 unique vertex per triangle corner
        for (int t = 0; t < surf.num_tris; ++t) {
            // Note: Swapped to i=2 to 0 if your model is inside-out (Winding Order)
            for (int i = 0; i < 3; ++i) { 
                int vIdx = sTris[t].indexes[i];

                model.indices.push_back(model.numVerts);

                for (int f = 0; f < h.num_frames; ++f) {
                    MD3_Vertex& vIn = sVerts[f * surf.num_verts + vIdx];
                    GDK_Legacy_Vert vOut;

                    vOut.x = vIn.x / 64.0f;
                    vOut.y = vIn.y / 64.0f;
                    vOut.z = vIn.z / 64.0f;
                    vOut.u = sUVs[vIdx].u;
                    vOut.v = sUVs[vIdx].v;

                    glm::vec3 n = DecompressMD3Normal(vIn.normal);
                    vOut.nx = n.x; vOut.ny = n.y; vOut.nz = n.z;

                    model.frames[f].push_back(vOut);
                }
                model.numVerts++;
            }
        }
        model.numTris += surf.num_tris;
        surfaceOffset += surf.offset_end;
    }

    file.close();
    return true;
}




#endif // GDK_MD3_H