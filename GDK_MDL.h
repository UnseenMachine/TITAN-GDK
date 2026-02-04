#ifndef GDK_MDL_H
#define GDK_MDL_H

// THIS IS THE ULTIMATE QUIETLY PERFECT VERSION OF THE QUAKE 1 MDL DEFINITIONS FOR LEGACY MODELS and Legacy MODE
// DO NOT CHANGE ANY OF THESE STRUCTURES OR THE PALETTE DATA OR THEY WILL BREAK PERFECT LOADING/RENDERING OF MDL FILES in gl 1.x mode

// --- 1. MDL STRUCTURE DEFINITIONS (RESTORED TO YOUR ORIGINAL PERFECT ONES) ---
#define IDPOLYHEADER 0x4F504944 
#define MDL_VERSION  6
// The above is ignored in this context but kept for reference


// --- THE COMPLETE QUAKE 1 PALETTE (768 Bytes) ---
static const uint8_t g_Q1_Palette[768] = {
    0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,139,107,107,151,115,115,163,123,123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,55,0,75,59,7,87,67,7,95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,95,183,135,107,195,147,123,211,163,139,227,179,151,171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,107,87,71,95,75,59,83,63,51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,43,175,47,47,159,47,47,143,47,47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,159,91,83
};

// --- INTERNAL TEXTURE GENERATOR FOR QUAKE 1 MODELS ---
// Now populates a reference target directly to keep models self-sufficient
static void GDK_Internal_CreateTexture(uint8_t* rgb, int w, int h, int& target) { 
    if (!rgb) { target = 0; return; }

    uint32_t tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    
    // Quake 1 textures are often not power-of-two or have specific alignments
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Generate Mipmaps for GL 1.1 - 3.3
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb);

    // Specific Quake 1 parameters to prevent UV bleeding on atlas-style skins
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

    // Store the raw GL handle in the model's defaultTex
    target = (int)tid;
}


static bool GDK_Internal_LoadMDL(const char* path, GDK_Legacy_Model& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    mdl_header_t h;
    file.read((char*)&h, sizeof(mdl_header_t));

    // 1. Process Internal Skin
    int skinSize = h.skinwidth * h.skinheight;
    std::vector<uint8_t> palData(skinSize);
    int32_t skinType; 
    file.read((char*)&skinType, 4); 
    file.read((char*)palData.data(), skinSize);

    std::vector<uint8_t> rgbBuffer(skinSize * 3);
    for (int i = 0; i < skinSize; i++) {
        uint8_t idx = palData[i];
        rgbBuffer[i * 3 + 0] = g_Q1_Palette[idx * 3 + 0];
        rgbBuffer[i * 3 + 1] = g_Q1_Palette[idx * 3 + 1];
        rgbBuffer[i * 3 + 2] = g_Q1_Palette[idx * 3 + 2];
    }
    // Explicitly cast the uint32_t reference to an int reference
    GDK_Internal_CreateTexture(rgbBuffer.data(), h.skinwidth, h.skinheight, (int&)out.defaultTex);

    // 2. Load Metadata
    std::vector<mdl_stvert_t> rawST(h.numverts);
    file.read((char*)rawST.data(), h.numverts * sizeof(mdl_stvert_t));
    std::vector<mdl_triangle_t> rawTris(h.numtris);
    file.read((char*)rawTris.data(), h.numtris * sizeof(mdl_triangle_t));

    // 3. Load Raw Frames
    struct RawFrame { std::vector<mdl_trivert_t> verts; };
    std::vector<RawFrame> rawFrames(h.numframes);
    for (int f = 0; f < h.numframes; f++) {
        int32_t fType; file.read((char*)&fType, 4);
        mdl_trivert_t q_min, q_max;
        file.read((char*)&q_min, sizeof(mdl_trivert_t));
        file.read((char*)&q_max, sizeof(mdl_trivert_t));
        char name[16]; file.read(name, 16);
        rawFrames[f].verts.resize(h.numverts);
        file.read((char*)rawFrames[f].verts.data(), h.numverts * sizeof(mdl_trivert_t));
    }

    // --- 4. ADVANCED GEOMETRY PROCESSING ---
    float lambda = 0.5f;   
    float mu = -0.54f;      
    int iterations = 4;    

    // Build Adjacency for Smoothing AND Normals
    std::vector<std::vector<int>> neighbors(h.numverts);
    std::vector<std::vector<int>> vertToTris(h.numverts); // Which triangles use this vertex?
    for (int t = 0; t < h.numtris; t++) {
        for (int i = 0; i < 3; i++) {
            int curr = rawTris[t].vertindex[i];
            int next = rawTris[t].vertindex[(i + 1) % 3];
            vertToTris[curr].push_back(t);
            bool exists = false;
            for (int e : neighbors[curr]) { if (e == next) { exists = true; break; } }
            if (!exists) neighbors[curr].push_back(next);
        }
    }

    out.numTris = h.numtris;
    out.numVerts = h.numtris * 3;
    out.numFrames = h.numframes;
    out.frames.resize(h.numframes);

    for (int f = 0; f < h.numframes; f++) {
        // A. Precision Smoothing
        std::vector<float> fv(h.numverts * 3);
        for(int i=0; i < h.numverts; i++) {
            fv[i*3+0] = (float)rawFrames[f].verts[i].v[0];
            fv[i*3+1] = (float)rawFrames[f].verts[i].v[1];
            fv[i*3+2] = (float)rawFrames[f].verts[i].v[2];
        }

        for (int iter = 0; iter < iterations; iter++) {
            float factors[2] = { lambda, mu };
            for (int step = 0; step < 2; step++) {
                std::vector<float> nextPass = fv;
                for (int i = 0; i < h.numverts; i++) {
                    if (neighbors[i].empty()) continue;
                    float ax=0, ay=0, az=0;
                    for (int n : neighbors[i]) { ax += fv[n*3+0]; ay += fv[n*3+1]; az += fv[n*3+2]; }
                    ax /= neighbors[i].size(); ay /= neighbors[i].size(); az /= neighbors[i].size();
                    nextPass[i*3+0] += (ax - fv[i*3+0]) * factors[step];
                    nextPass[i*3+1] += (ay - fv[i*3+1]) * factors[step];
                    nextPass[i*3+2] += (az - fv[i*3+2]) * factors[step];
                }
                fv = nextPass;
            }
        }

        // B. Calculate Smooth Normals for this frame
        // First, calculate all face normals
        std::vector<float> triNorms(h.numtris * 3);
        for(int t=0; t < h.numtris; t++) {
            int i0 = rawTris[t].vertindex[0] * 3;
            int i1 = rawTris[t].vertindex[1] * 3;
            int i2 = rawTris[t].vertindex[2] * 3;
            float ux = fv[i1+0] - fv[i0+0], uy = fv[i1+1] - fv[i0+1], uz = fv[i1+2] - fv[i0+2];
            float vx = fv[i2+0] - fv[i0+0], vy = fv[i2+1] - fv[i0+1], vz = fv[i2+2] - fv[i0+2];
            triNorms[t*3+0] = uy*vz - uz*vy; triNorms[t*3+1] = uz*vx - ux*vz; triNorms[t*3+2] = ux*vy - uy*vx;
        }

        // C. Unroll and Apply
        out.frames[f].reserve(out.numVerts);
        for (int t = 0; t < h.numtris; t++) {
            bool isBackFace = (rawTris[t].facesfront == 0);
            for (int i = 0; i < 3; i++) {
                int vIdx = rawTris[t].vertindex[i];
                GDK_Legacy_Vert v;
                
                // Position (Scaled)
                v.x = (h.scale[0] * fv[vIdx*3+0]) + h.translate[0];
                v.z = -((h.scale[1] * fv[vIdx*3+1]) + h.translate[1]);
                v.y = (h.scale[2] * fv[vIdx*3+2]) + h.translate[2];

                // UVs
                mdl_stvert_t& st = rawST[vIdx];
                float s = (float)st.s;
                if (isBackFace && st.onseam != 0) s += (h.skinwidth / 2.0f);
                v.u = (s + 0.5f) / (float)h.skinwidth;
                v.v = ((float)st.t + 0.5f) / (float)h.skinheight;

                // Smooth Normal Calculation
                float nx=0, ny=0, nz=0;
                for(int triIdx : vertToTris[vIdx]) {
                    nx += triNorms[triIdx*3+0]; ny += triNorms[triIdx*3+1]; nz += triNorms[triIdx*3+2];
                }
                float mag = sqrt(nx*nx + ny*ny + nz*nz);
                v.nx = (mag > 0) ? nx/mag : 0; 
                v.ny = (mag > 0) ? ny/mag : 1; 
                v.nz = (mag > 0) ? nz/mag : 0;

                out.frames[f].push_back(v);
                if (f == 0) out.indices.push_back((int)out.frames[f].size() - 1);
            }
        }
    }
    file.close();
    return true;
}


#endif
