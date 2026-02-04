

namespace PRM {

    // --- 1. DATA STRUCTURES ---
#pragma pack(push, 1)
    struct PRM_Vector { float x, y, z; };
    struct PRM_UV     { float u, v; };

    struct PRM_Vertex {
        PRM_Vector pos;
        PRM_Vector normal;
    };

    struct PRM_Polygon {
        short type;             // Bit 0: Quad(1) or Tri(0)
        short texture;          // Texture Page Index
        unsigned short indices[4];
        unsigned int colors[4]; // ARGB/BGRA
        PRM_UV uv[4];
    };
#pragma pack(pop)

    struct PRM_Mesh {
        std::vector<PRM_Vertex> vertices;
        std::vector<PRM_Polygon> polygons;
        int prmIndex; // The MODEL ID from Parameters.txt
    };

    // --- 2. INDIVIDUAL COMPONENT STRUCTS ---
    struct PRM_Car_Wheel {
        int meshIdx;           // Index into meshLibrary
        PRM_Vector offset;
        float spinAngle;       
        float steerAngle;
        float radius;
        bool isPowered;
        bool isTurnable;
    };

    struct PRM_Car_Spring {
        int meshIdx;
        PRM_Vector offset;
        float length;
        float stiffness;
        float compression;     // 0.0 to 1.0 for manual control
    };

    struct PRM_Car_Axle {
        int meshIdx;
        PRM_Vector offset;
        float width = 1.0f;
    };

    struct PRM_Car_Body {
        int meshIdx;
        PRM_Vector offset;
        float mass;
    };

    // --- 3. THE MASTER CAR STRUCT ---
    struct PRM_Car {
        bool InUse = false;
        
        // The Mesh Library (Stored once)
        std::vector<PRM_Mesh> meshLibrary;
        
        // Individualized Component Vectors
        PRM_Car_Body body;
        std::vector<PRM_Car_Wheel> wheels;
        std::vector<PRM_Car_Spring> springs;
        std::vector<PRM_Car_Axle> axles;

        std::vector<uint32_t> textures;
        float globalScale = 1.0f;
        PRM_Vector pos, rot, CoM;

        
        // Clean up for reuse
        void Free() {
            meshLibrary.clear();
            wheels.clear();
            springs.clear();
            axles.clear();
            textures.clear(); // Ideally glDeleteTextures here too
            InUse = false;
        }
    };

    static std::vector<PRM_Car> g_PRMStore;


    // ---  INTERNAL UTILITIES ---
    int GetFreeSlot() {
        for (int i = 0; i < (int)g_PRMStore.size(); ++i) {
            if (!g_PRMStore[i].InUse) return i;
        }
        g_PRMStore.push_back({});
        return (int)g_PRMStore.size() - 1;
    }

    void FreeSlot(int index) {
        if (index >= 0 && index < (int)g_PRMStore.size()) {
            g_PRMStore[index].Free();
        }
    }

    static std::string Internal_GetDirectory(const std::string& path) {
        size_t last = path.find_last_of("/\\");
        return (last == std::string::npos) ? "" : path.substr(0, last + 1);
    }

    static bool Internal_LoadSinglePRM(const std::string& path, PRM_Mesh& out, int id) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            printf("  [PRM ERR] Could not find mesh: %s\n", path.c_str());
            return false;
        }

        short nPolys, nVerts;
        file.read((char*)&nPolys, 2);
        file.read((char*)&nVerts, 2);

        out.prmIndex = id;
        out.polygons.resize(nPolys);
        file.read((char*)out.polygons.data(), nPolys * sizeof(PRM_Polygon));

        out.vertices.resize(nVerts);
        for (int i = 0; i < nVerts; ++i) {
            PRM_Vertex v;
            file.read((char*)&v, sizeof(PRM_Vertex));
            // Apply your working Y-Up flip logic
            //v.pos.y = -v.pos.y;       
            v.normal.y = -v.normal.y;
            out.vertices[i] = v;
        }
        printf("  [PRM] Mesh Library: Registered ID %d (%d polys, %d verts)\n", id, nPolys, nVerts);
        return true;
    }


    
    static int Internal_FindMeshIdx(PRM_Car& car, int prmID) {
        for (int i = 0; i < (int)car.meshLibrary.size(); ++i) {
            if (car.meshLibrary[i].prmIndex == prmID) return i;
        }
        return -1;
    }

    static void Internal_ParseBody(std::ifstream& file, PRM_Car& car) {
        int prmID = -1;
        std::string token;
        while (file >> token && token != "}") {
            if (token == "ModelNum") file >> prmID;
            if (token == "Offset") {
                float fx, fz, fy; file >> fx >> fz >> fy;
                car.body.offset = { fx, fy, fz };
            }
            if (token == "Mass") file >> car.body.mass;
        }
        car.body.meshIdx = Internal_FindMeshIdx(car, prmID);
        if (car.body.meshIdx != -1) printf("  [PRM] Body Linked: Mesh %d (Mass %.2f)\n", prmID, car.body.mass);
    }

    static void Internal_ParseWheel(std::ifstream& file, PRM_Car& car) {
        PRM_Car_Wheel w = {};
        int prmID = -1, subIdx; 
        file >> subIdx; // Skip wheel index (e.g. WHEEL 0)
        std::string token;
        while (file >> token && token != "}") {
            if (token == "ModelNum") file >> prmID;
            if (token == "Offset1") {
                float fx, fz, fy; file >> fx >> fz >> fy;
                w.offset = { fx, fy, fz };
            }
            if (token == "IsPowered") { std::string b; file >> b; w.isPowered = (b == "true"); }
            if (token == "IsTurnable") { std::string b; file >> b; w.isTurnable = (b == "true"); }
            if (token == "Radius") file >> w.radius;
        }
        w.meshIdx = Internal_FindMeshIdx(car, prmID);
        if (w.meshIdx != -1) {
            car.wheels.push_back(w);
            printf("  [PRM] Wheel %d Linked: Mesh %d\n", subIdx, prmID);
        }
    }

    static void Internal_ParseSpring(std::ifstream& file, PRM_Car& car) {
        PRM_Car_Spring s = {};
        int prmID = -1, subIdx; 
        file >> subIdx;
        std::string token;
        while (file >> token && token != "}") {
            if (token == "ModelNum") file >> prmID;
            if (token == "Offset") {
                float fx, fz, fy; file >> fx >> fz >> fy;
                s.offset = { fx, fy, fz };
            }
            if (token == "Length")    file >> s.length;
            if (token == "Stiffness") file >> s.stiffness;
        }
        s.meshIdx = Internal_FindMeshIdx(car, prmID);
        if (s.meshIdx != -1) {
            car.springs.push_back(s);
            printf("  [PRM] Spring %d Linked: Mesh %d\n", subIdx, prmID);
        }
    }


    static void Internal_ParseAxle(std::ifstream& file, PRM_Car& car) {
        PRM_Car_Axle a = {}; int prmID = -1, sub; file >> sub; std::string t;
        while (file >> t && t != "}") {
            if (t == "ModelNum") file >> prmID;
            if (t == "Offset")   { 
                float fileX, fileZ, fileY; 
                file >> fileX >> fileZ >> fileY; 
                // Map to OpenGL (X=X, Y=-Y, Z=-Z) but file order is XZY
                a.offset = { fileX, fileY, fileZ}; 
            }
            if (t == "Length")   file >> a.width; // Catch the stretch factor!
        }
        a.meshIdx = Internal_FindMeshIdx(car, prmID);
        if (a.meshIdx != -1) car.axles.push_back(a);
    }

    // This is the function your Master System calls
    int PRM_LoadRevoltCar(const char* pPath) {
        // 1. Find an empty slot automatically
        int slot = GetFreeSlot(); 
        PRM_Car& car = g_PRMStore[slot];
        
        // 2. Clear it out and set as active
        car.Free(); 
        car.InUse = true;

        std::ifstream file(pPath);
        if (!file.is_open()) {
            printf("[PRM ERR] Parameters not found: %s\n", pPath);
            car.InUse = false; // Release the slot if file is missing
            return -1; 
        }

        std::string dir = Internal_GetDirectory(pPath);
        std::string token;

        printf("[PRM] Loading: %s into Slot %d\n", pPath, slot);

        while (file >> token) {
            // --- PHASE 1: ASSET LIBRARY (Models & Textures) ---
            if (token == "MODEL") {
                int id; file >> id;
                std::string line; std::getline(file, line);
                size_t s = line.find('\"'), e = line.find('\"', s + 1);
                if (s != std::string::npos) {
                    std::string fileName = line.substr(s + 1, e - s - 1);
                    
                    // --- STRIP PATHS ---
                    // Find the last slash (either / or \) and keep only what's after it
                    size_t lastSlash = fileName.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        fileName = fileName.substr(lastSlash + 1);
                    }

                    PRM_Mesh m;
                    if (Internal_LoadSinglePRM(dir + fileName, m, id)) {
                        car.meshLibrary.push_back(m);
                    }
                }
            }
            if (token == "TPAGE") {
                std::string line; std::getline(file, line);
                size_t s = line.find('\"'), e = line.find('\"', s + 1);
                if (s != std::string::npos) {
                    std::string texName = line.substr(s + 1, e - s - 1);
                    
                    // Strip old hardcoded paths (e.g., "cars/ant_jester/car.bmp" -> "car.bmp")
                    size_t lastSlash = texName.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        texName = texName.substr(lastSlash + 1);
                    }

                    // Now look for the texture in the SAME folder as Parameters.txt
                    uint32_t tid = GDK_Internal_CreateGLTexture((dir + texName).c_str());
                    if (tid > 0) {
                        car.textures.push_back(tid);
                        printf("  [PRM] Texture Registered: %s\n", texName.c_str());
                    } else {
                        printf("  [PRM ERR] Texture Failed to Load: %s\n", (dir + texName).c_str());
                    }
                }
            }


            // --- PHASE 2: COMPONENT INSTANCING ---
            if (token == "BODY")   Internal_ParseBody(file, car);
            if (token == "WHEEL")  Internal_ParseWheel(file, car);
            if (token == "SPRING") Internal_ParseSpring(file, car);
            if (token == "AXLE")   Internal_ParseAxle(file, car);
            // Global Constants
            if (token == "CoM") { 
                file >> car.CoM.x >> car.CoM.z >> car.CoM.y; 
                car.CoM.y = -car.CoM.y; 
            }
            if (token == "SCALE")  file >> car.globalScale;
        }

        if (car.meshLibrary.empty()) {
            printf("[PRM ERR] No geometry loaded for %s\n", pPath);
            car.InUse = false;
            return -1;
        }

        printf("[PRM] Slot %d Ready. (Body + %d Wheels + %d Springs)\n", 
                slot, (int)car.wheels.size(), (int)car.springs.size());

        // Return the handle back to QB64
        return slot; 
    }


    static void Internal_RenderLibraryMesh(const PRM_Mesh& mesh, const std::vector<uint32_t>& textures) {
        uint32_t activeTex = 0xFFFFFFFF;
        for (const auto& poly : mesh.polygons) {
            // Texture Atlas Binding
            if (poly.texture >= 0 && poly.texture < (int)textures.size()) {
                uint32_t tid = textures[poly.texture];
                if (tid != activeTex) {
                    glBindTexture(GL_TEXTURE_2D, tid);
                    activeTex = tid;
                }
            }

            bool isQuad = poly.type & 1;
            glBegin(isQuad ? GL_QUADS : GL_TRIANGLES);
            for (int i = 0; i < (isQuad ? 4 : 3); ++i) {
                unsigned char* c = (unsigned char*)&poly.colors[i];
                glColor4ub(c[2], c[1], c[0], c[3]); // Maps BGRA bytes to RGBA call
                glTexCoord2f(poly.uv[i].u, 1.0f - poly.uv[i].v);
                const auto& v = mesh.vertices[poly.indices[i]];
                glNormal3f(v.normal.x, v.normal.y, v.normal.z);
                glVertex3f(v.pos.x, v.pos.y, v.pos.z);
            }
            glEnd();
        }
    }
    void ApplyLookAt(PRM::PRM_Vector origin, PRM::PRM_Vector target) {
        float dx = target.x - origin.x;
        float dy = target.y - origin.y;
        float dz = target.z - origin.z;

        // Yaw: Rotation around Y (horizontal alignment)
        // We use atan2(dz, dx) because Re-Volt models point along X
        float yaw = atan2(dz, dx) * 180.0f / 3.14159265f;

        // Pitch: Rotation around Z (vertical tilt for suspension)
        float distXZ = sqrt(dx * dx + dz * dz);
        float pitch = -atan2(dy, distXZ) * 180.0f / 3.14159265f;

        glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
        glRotatef(pitch, 0.0f, 0.0f, 1.0f);
    }


        // Helper to calculate distance
    float GetDist(PRM::PRM_Vector a, PRM::PRM_Vector b) {
            return sqrtf(powf(b.x - a.x, 2) + powf(b.y - a.y, 2) + powf(b.z - a.z, 2));

    }

    void PointAt(PRM::PRM_Vector origin, PRM::PRM_Vector target, bool isAxle) {
        float dx = target.x - origin.x;
        float dy = target.y - origin.y;
        float dz = target.z - origin.z;

        float yaw = atan2f(dz, dx) * 180.0f / 3.14159f;
        float distXZ = sqrtf(dx * dx + dz * dz);
        float pitch = -atan2f(dy, distXZ) * 180.0f / 3.14159f;

        if (isAxle) {
            // AXLE FIX: Shift yaw by 90 to correct the "90 off" issue
            glRotatef(-yaw + 90.0f, 0.0f, 1.0f, 0.0f);
            // Tilt the Y-axis (Up/Down) toward the target
            glRotatef(pitch, 1.0f, 0.0f, 0.0f); 
        } else {
            // SPRING FIX: Use the calculated yaw to swing the Z-axis (Forward/Back)
            glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
            // Apply pitch to tilt the spring up/down toward the wheel hub
            glRotatef(pitch, 0.0f, 1.0f, 0.0f); 
        }
    }



    void PRM_Draw(int slot) {
        if (slot < 0 || slot >= (int)g_PRMStore.size() || !g_PRMStore[slot].InUse) return;
        PRM_Car& car = g_PRMStore[slot];

        glEnable(GL_TEXTURE_2D);
        glPushMatrix();

        // 1. Apply World Transform & Scaling
        //glTranslatef(car.pos.x, car.pos.y, car.pos.z);
        //glScalef(car.globalScale, car.globalScale, car.globalScale);

        // 2. Pivot around Center of Mass (CoM)
        //glTranslatef(car.CoM.x, car.CoM.y, car.CoM.z);
        //glRotatef(car.rot.y, 0, 1, 0); 
        //glRotatef(car.rot.x, 1, 0, 0); 
        //glRotatef(car.rot.z, 0, 0, 1);
        //glTranslatef(-car.CoM.x, -car.CoM.y, -car.CoM.z);
        

        // 3. Draw Body
        if (car.body.meshIdx != -1) {
            glPushMatrix();
            glRotatef(90, 1, 0, 0);
            glRotatef(180, 0, 1, 0);
            
            //glTranslatef(car.body.offset.x, car.body.offset.y, car.body.offset.z);
            Internal_RenderLibraryMesh(car.meshLibrary[car.body.meshIdx], car.textures);
            glPopMatrix();
        }

        // 4. Draw Wheels
        for (const auto& w : car.wheels) {
            if (w.meshIdx == -1) continue;
            glPushMatrix();
            glTranslatef(w.offset.x, w.offset.y, w.offset.z);
            if (w.isTurnable) glRotatef(w.steerAngle, 0, 1, 0);
            glRotatef(w.spinAngle, 1, 0, 0);
            Internal_RenderLibraryMesh(car.meshLibrary[w.meshIdx], car.textures);
            glPopMatrix();
        }
        
        // --- AXLES ---
        for (int i = 0; i < (int)car.axles.size(); ++i) {
            auto& a = car.axles[i];
            if (a.meshIdx == -1 || i >= (int)car.wheels.size()) continue;

            PRM_Vector target = car.wheels[i].offset;
            float dx = target.x - a.offset.x;
            float dy = target.y - a.offset.y;
            float dz = target.z - a.offset.z;
            
            // Calculate the yaw right here
            float currentYaw = atan2f(dz, dx) * 180.0f / 3.14159f;
            float d = sqrtf(dx*dx + dy*dy + dz*dz);

            glPushMatrix();
            glTranslatef(a.offset.x, a.offset.y, a.offset.z);
            
            // Rotate to face wheel + the 90 degree fix for your model
            glRotatef(-currentYaw + 90.0f, 0.0f, 1.0f, 0.0f);
            
            // Stretch on Y (since you said they are up/down models)
            glScalef(1.0f, d / (a.width > 0 ? a.width : 1.0f), 1.0f);

            Internal_RenderLibraryMesh(car.meshLibrary[a.meshIdx], car.textures);
            glPopMatrix();
        }

        // --- SPRINGS ---
        for (int i = 0; i < (int)car.springs.size(); ++i) {
            auto& s = car.springs[i];
            if (s.meshIdx == -1 || i >= (int)car.wheels.size()) continue;

            PRM_Vector target = car.wheels[i].offset;
            float dx = target.x - s.offset.x;
            float dz = target.z - s.offset.z;
            
            // Calculate yaw for the spring
            float currentYaw = atan2f(dz, dx) * 180.0f / 3.14159f;
            float d = GetDist(s.offset, target);

            glPushMatrix();
            glTranslatef(s.offset.x, s.offset.y, s.offset.z);
            
            // Point the spring's Z-axis at the wheel
            glRotatef(-currentYaw, 0.0f, 0.0f, 1.0f);
            
            // Stretch on Z (since you said they are backwards models)
            glScalef(1.0f, 1.0f, d / (s.length > 0 ? s.length : 1.0f));

            Internal_RenderLibraryMesh(car.meshLibrary[s.meshIdx], car.textures);
            glPopMatrix();
        }




        glPopMatrix();
        glDisable(GL_TEXTURE_2D);
    }

}
