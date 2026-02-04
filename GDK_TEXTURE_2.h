#ifndef GDK_TEXTURE_H
#define GDK_TEXTURE_H


static uint32_t GDK_Internal_CreateGLTexture(const char* path) {
    if (!path) return 0;

    std::string filename = path;
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    int w, h, c;
    unsigned char* data = nullptr;
    bool is_pcx = (ext == "pcx");

    if (is_pcx) {
        // Load PCX (Force 4 components / RGBA)
        data = drpcx_load_file(path, DRPCX_FALSE, &w, &h, &c, 4);
        
        // Manual vertical flip for PCX to match STB behavior
        if (data) {
            int rowSize = w * 4;
            std::vector<uint8_t> tempRow(rowSize);
            for (int y = 0; y < h / 2; ++y) {
                uint8_t* top = data + (y * rowSize);
                uint8_t* bottom = data + ((h - 1 - y) * rowSize);
                memcpy(tempRow.data(), top, rowSize);
                memcpy(top, bottom, rowSize);
                memcpy(bottom, tempRow.data(), rowSize);
            }
        }
    } else {
        // Load Standard (PNG, TGA, BMP, JPG) - Always flipped for OpenGL
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load(path, &w, &h, &c, 4);
    }

    if (!data) return 0;

    uint32_t tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);

    // Standard 1.1 - 3.3 Loading
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Default Filtering for MD2 compatibility
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (is_pcx) drpcx_free(data);
    else stbi_image_free(data);

    return tid;
}
// Global Registry for sprites/UI
static std::vector<uint32_t> g_Textures;

GDK_BEGIN_DECLS

// Version A: Returns index for global use
GDK_API int GDK_LoadTexture(const char* path) {
    uint32_t tid = GDK_Internal_CreateGLTexture(path);
    if (tid == 0) return -1;
    g_Textures.push_back(tid);
    return (int)g_Textures.size() - 1;
}

GDK_END_DECLS

// Version B: Direct to model (Internal use)
GDK_API void GDK_LoadTexture(const char* path, int& target) {
    target = (int)GDK_Internal_CreateGLTexture(path);
}

#endif // GDK_TEXTURE_H