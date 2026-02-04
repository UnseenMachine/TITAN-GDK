#ifndef GDK_LEGACY_TYPES_H
#define GDK_LEGACY_TYPES_H

struct GDK_Legacy_Vert {
    float x, y, z;    
    float nx, ny, nz; 
    float u, v;       
};

struct GDK_Animation {
    int Start, End, Current; 
    float Speed, Timer, Lerp;
    bool Looping; 
    int Priority; 
};

// 1. Define the internal Tag structure first
struct GDK_MD3_Tag_Data { 
    std::string name; 
    glm::vec3 pos; 
    glm::mat3 axis; 
};

// 2. Define the Hierarchy container
struct GDK_MD3_Hierarchy {
     std::vector<std::vector<GDK_MD3_Tag_Data>> tagFrames;
};

// --- QUAKE 3 MD3 FILE STRUCTURES ---
#pragma pack(push, 1)
struct MD3_Header {
    int32_t ident, version;
    char name[64];
    int32_t flags, num_frames, num_tags, num_surfaces, num_skins;
    int32_t offset_frames, offset_tags, offset_surfaces, offset_eof;
};

struct MD3_Tag { // The raw file structure
    char name[64];
    glm::vec3 pos;
    glm::mat3 axis;
};

struct MD3_Surface {
    int32_t ident;
    char name[64];
    int32_t flags, num_frames, num_shaders, num_verts, num_tris;
    int32_t offset_tris, offset_shaders, offset_st, offset_xyznormal, offset_end;
};

struct MD3_Triangle { int32_t indexes[3]; };
struct MD3_TexCoord { float u, v; };
struct MD3_Vertex   { int16_t x, y, z, normal; };
#pragma pack(pop)

// --- QUAKE 2 MD2 FILE STRUCTURES ---
#pragma pack(push, 1)
struct MD2_Header {
    int32_t magic, version, skinwidth, skinheight, framesize;
    int32_t num_skins, num_vertices, num_st, num_tris, num_glcmds, num_frames;
    int32_t offset_skins, offset_st, offset_tris, offset_frames, offset_glcmds, offset_end;
};
struct MD2_Alias_ST { int16_t s, t; };
struct MD2_Alias_Triangle { uint16_t vertex[3], st[3]; };
struct MD2_Alias_Vert { uint8_t v[3], lightNormalIndex; };
#pragma pack(pop)

// --- QUAKE 1 MDL FILE STRUCTURES ---
#pragma pack(push, 1)
typedef struct {
    int32_t id, version;
    float scale[3], translate[3], boundingradius, eyePosition[3];
    int32_t numskins, skinwidth, skinheight, numverts, numtris, numframes, synctype, flags;
    float size;
} mdl_header_t;
typedef struct { int32_t onseam, s, t; } mdl_stvert_t;
typedef struct { int32_t facesfront, vertindex[3]; } mdl_triangle_t;
typedef struct { uint8_t v[3], lightnormalindex; } mdl_trivert_t;
#pragma pack(pop)




// --- THE MASTER MODEL STRUCT ---
struct GDK_Legacy_Model {
    GDK_ModelType type;
    uint32_t defaultTex;
    int numTris, numVerts, numFrames, numTags;
    bool InUse;

    std::vector<uint32_t> indices;
    std::vector<std::vector<GDK_Legacy_Vert>> frames; 

    GDK_MD3_Hierarchy* hierarchy = nullptr; 

    std::map<int, GDK_Animation> animLibrary;
    int NumAnims;

    bool HasHierarchy() const { return hierarchy != nullptr; }

    void Free() {

        if (defaultTex > 0) {
            glDeleteTextures(1, (GLuint*)&defaultTex);
            defaultTex = 0;
        }
        if (hierarchy) {
            delete hierarchy; // Kill the MD3 tag data
            hierarchy = nullptr;
        }
        indices.clear();
        frames.clear();
        animLibrary.clear();
        InUse = false;
    }
};

static std::vector<GDK_Legacy_Model> g_ModelStore;

#endif
