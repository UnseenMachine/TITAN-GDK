#ifndef GDK_CORE_SYSTEM_H
#define GDK_CORE_SYSTEM_H

enum GDK_RenderMode { GDK_MODE_LEGACY = 0, GDK_MODE_STANDARD = 1, GDK_MODE_AZDO = 2 };

struct alignas(16) GDK_Light {
    glm::vec3 position;  float power;         
    glm::vec3 color;     float ambient;       
    glm::vec3 direction; int type;            
    uint32_t enabled;    float range;         
    float innerCutoff;   float outerCutoff;   
};

struct GDK_SystemState {
    // Matrices (Mapped directly to Matrix Stacks)
    glm::mat4 projection, view;

    // Camera & Environment
    glm::vec3 cameraPos; float time;
    glm::vec2 resolution; float deltaTime; int frameCount;
    float fov, nearPlane, farPlane;
    
    // Input State
    glm::vec2 mouse, mouseDelta;
    uint32_t mouseButtons;      
    glm::vec2 oldMousePos;      
    uint32_t oldMouseButtons;   
    float fps;

};

// --- 2. THE ENGINE NAMESPACE ---

namespace GDK {
    static HWND hwnd; static HDC hdc; static HGLRC hrc;
    static bool quit = false;
    static GDK_RenderMode mode = GDK_MODE_LEGACY;
    static GDK_SystemState* state = nullptr;
    
    static bool keys[256], lastKeys[256];
    static float mDX, mDY; static bool mLock = false;

    static std::chrono::high_resolution_clock::time_point tStart;
    static std::chrono::high_resolution_clock::time_point tFrame;

    // --- INTERNAL MATRIX STACKS (Zero-Copy Linkage) ---
    namespace Internal {
        static glm::vec3 g_UpVector = glm::vec3(0.0f, 1.0f, 0.0f);
        
        struct MatrixStack {
            std::vector<glm::mat4> storage;
            glm::mat4* current = nullptr; // Links to GDK::state->view or projection

            void Push() { if(current) storage.push_back(*current); }
            void Pop()  { if(current && !storage.empty()) { *current = storage.back(); storage.pop_back(); } }
        };

        static MatrixStack g_ModelStack;
        static MatrixStack g_ProjStack;
        static MatrixStack* g_ActiveStack = &g_ModelStack;
        static uint32_t g_SystemUBO = 0;
    }

    // Unified 2026 Sync Logic
    static inline void SyncMatrix() {
        if (!Internal::g_ActiveStack->current) return;
        if (mode == GDK_MODE_LEGACY) {
            // Mode 0: Force software-calculated GLM matrix into fixed-function pipe
            glLoadMatrixf(glm::value_ptr(*Internal::g_ActiveStack->current));
        }
        // Mode 1 & 2: No sync needed. Memory is already updated in GDK::state.
    }
}

// Window Procedure (Internal)
LRESULT CALLBACK GDK_WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_CLOSE: GDK::quit = true; return 0;
        case WM_SIZE: if(GDK::state) { 
            GDK::state->resolution = {(float)LOWORD(l), (float)HIWORD(l)}; 
            glViewport(0, 0, LOWORD(l), HIWORD(l)); 
        } return 0;
    }
    return DefWindowProc(h, m, w, l);
}


GDK_BEGIN_DECLS 

// --- MOUSE API ---
GDK_API float GDK_MouseX()           { return GDK::state->mouse.x; }
GDK_API float GDK_MouseY()           { return GDK::state->mouse.y; }
GDK_API float GDK_MouseDX()          { return GDK::mDX; }
GDK_API float GDK_MouseDY()          { return GDK::mDY; }

GDK_API int GDK_MouseDown(int b) {
    return (GDK::state->mouseButtons & (1 << (b - 1))) ? 1 : 0;
}

GDK_API int GDK_MouseClicked(int b) {
    uint32_t mask = (1 << (b - 1));
    return ((GDK::state->mouseButtons & mask) && !(GDK::state->oldMouseButtons & mask)) ? 1 : 0;
}

GDK_API int GDK_MouseReleased(int b) {
    uint32_t mask = (1 << (b - 1));
    return (!(GDK::state->mouseButtons & mask) && (GDK::state->oldMouseButtons & mask)) ? 1 : 0;
}

// --- KEYBOARD API ---
GDK_API int GDK_Key(int k)           { return GDK::keys[k & 0xFF] ? 1 : 0; }

GDK_API int GDK_KeyHit(int k) { 
    return (GDK::keys[k & 0xFF] && !GDK::lastKeys[k & 0xFF]) ? 1 : 0; 
}

GDK_API int GDK_KeyReleased(int k) {
    return (!GDK::keys[k & 0xFF] && GDK::lastKeys[k & 0xFF]) ? 1 : 0;
}

// --- SYSTEM API ---
GDK_API float GDK_GetTime()          { return GDK::state->time; } // Seconds since boot
GDK_API float GDK_GetDT()            { return GDK::state->deltaTime; } // Delta Time (Seconds)
GDK_API float GDK_GetFPS()           { return GDK::state->fps; } // Current FPS
GDK_API void GDK_MouseLock(int l) { // Now auto-centers on lock to avoid rapid jumps
    GDK::mLock = (l != 0);
    ShowCursor(!GDK::mLock);

    if (GDK::mLock) {
        RECT r;
        GetWindowRect(GDK::hwnd, &r);
        // Calculate initial center to prevent the first-frame jump
        int cx = r.left + (r.right - r.left) / 2;
        int cy = r.top + (r.bottom - r.top) / 2;
        SetCursorPos(cx, cy);
    }
}

GDK_API int   GDK_Quit()             { return GDK::quit ? 1 : 0; }
GDK_API void  GDK_Clear(float r, float g, float b) { glClearColor(r,g,b,1); glClear(0x4100); } // CLS
GDK_API void  GDK_Display()          { SwapBuffers(GDK::hdc); } // _DISPLAY

// -- Matrix Operations --
GDK_API void GDK_ModelMode() { 
    GDK::Internal::g_ActiveStack = &GDK::Internal::g_ModelStack; 
    if (GDK::mode == GDK_MODE_LEGACY) glMatrixMode(0x1700); // GL_MODELVIEW
}
GDK_API void GDK_ProjMode() { 
    GDK::Internal::g_ActiveStack = &GDK::Internal::g_ProjStack; 
    if (GDK::mode == GDK_MODE_LEGACY) glMatrixMode(0x1701); // GL_PROJECTION
}
GDK_API void GDK_LoadIdentity() { 
    if(GDK::Internal::g_ActiveStack->current) *GDK::Internal::g_ActiveStack->current = glm::mat4(1.0f); 
    GDK::SyncMatrix(); 
}
GDK_API void GDK_PushMatrix() { GDK::Internal::g_ActiveStack->Push(); }
GDK_API void GDK_PopMatrix()  { GDK::Internal::g_ActiveStack->Pop(); GDK::SyncMatrix(); }

GDK_API void GDK_Translate(float x, float y, float z) {
    auto& m = *GDK::Internal::g_ActiveStack->current;
    m = glm::translate(m, glm::vec3(x, y, z));
    GDK::SyncMatrix();
}
GDK_API void GDK_Rotate(float angle, float x, float y, float z) {
    auto& m = *GDK::Internal::g_ActiveStack->current;
    m = glm::rotate(m, glm::radians(angle), glm::vec3(x, y, z));
    GDK::SyncMatrix();
}
GDK_API void GDK_Scale(float x, float y, float z) {
    auto& m = *GDK::Internal::g_ActiveStack->current;
    m = glm::scale(m, glm::vec3(x, y, z));
    GDK::SyncMatrix();
}

// -- Camera Operations (S-Tier Simplified) --
GDK_API void GDK_SetCameraUp(float x, float y, float z) { 
    GDK::Internal::g_UpVector = glm::vec3(x, y, z); 
}

GDK_API void GDK_Perspective(float fov, float nearP, float farP) {
    if (!GDK::state) return;
    float aspect = GDK::state->resolution.x / GDK::state->resolution.y;
    GDK::state->fov = fov; GDK::state->nearPlane = nearP; GDK::state->farPlane = farP;
    
    GDK::state->projection = glm::perspective(glm::radians(fov), aspect, nearP, farP);
    if (GDK::mode == GDK_MODE_LEGACY) {
        glMatrixMode(0x1701); glLoadMatrixf(glm::value_ptr(GDK::state->projection));
        glMatrixMode(0x1700);
    }
}

GDK_API void GDK_LookAt(float ex, float ey, float ez, float tx, float ty, float tz) {
    if (!GDK::state) return;
    GDK::state->cameraPos = {ex, ey, ez};
    GDK::state->view = glm::lookAt(GDK::state->cameraPos, glm::vec3(tx, ty, tz), GDK::Internal::g_UpVector);
    if (GDK::mode == GDK_MODE_LEGACY) {
        glMatrixMode(0x1700); glLoadMatrixf(glm::value_ptr(GDK::state->view));
    }
}


// If this fails then nothing will work, so we return 0 on failure and exit 
// later we may add fallbacks but for now this is critical
GDK_API int GDK_Boot(int w, int h, const char* title, int modeIdx) {
    HINSTANCE inst = GetModuleHandle(NULL);
    WNDCLASS wc = {0}; wc.lpfnWndProc = GDK_WndProc; wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.lpszClassName = "GDK_Core";
    RegisterClass(&wc);

    GDK::hwnd = CreateWindow("GDK_Core", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
                             CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, inst, NULL);
    GDK::hdc = GetDC(GDK::hwnd);
    
    PIXELFORMATDESCRIPTOR pfd = {sizeof(pfd), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32, 0,0,0,0,0,0,0,0,0,0,0,0,0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0};
    SetPixelFormat(GDK::hdc, ChoosePixelFormat(GDK::hdc, &pfd), &pfd);
    
    // 1. Create temporary context to bootstrap GLEW
    HGLRC temp = wglCreateContext(GDK::hdc); 
    wglMakeCurrent(GDK::hdc, temp);
    glewExperimental = GL_TRUE; // Essential for core/modern functionality
    glewInit();

    GDK::mode = (GDK_RenderMode)modeIdx;

    if (GDK::mode != GDK_MODE_LEGACY) {
        // MODERN PATH: Create a Core Profile Context (3.3 for Standard, 4.6 for AZDO)
        int major = (GDK::mode == GDK_MODE_AZDO) ? 4 : 3;
        int minor = (GDK::mode == GDK_MODE_AZDO) ? 6 : 3;
        
        int attr[] = {
            0x2091, major, // WGL_CONTEXT_MAJOR_VERSION_ARB
            0x2092, minor, // WGL_CONTEXT_MINOR_VERSION_ARB
            0x9122, 0x0001, // WGL_CONTEXT_PROFILE_MASK_ARB (Core Profile)
            
        }; 

        GDK::hrc = wglCreateContextAttribsARB(GDK::hdc, 0, attr);
        wglMakeCurrent(GDK::hdc, GDK::hrc); 
        wglDeleteContext(temp); // Trash the old legacy context

        if (GDK::mode == GDK_MODE_AZDO) {
            // --- AZDO PERSISTENT UBO SETUP ---
            GLbitfield flags = 0x0040 | 0x0080 | 0x0100; // WRITE | PERSISTENT | COHERENT
            glCreateBuffers(1, &GDK::Internal::g_SystemUBO); // Save to the global internal
            
            glNamedBufferStorage(GDK::Internal::g_SystemUBO, sizeof(GDK_SystemState), NULL, flags);
            GDK::state = (GDK_SystemState*)glMapNamedBufferRange(GDK::Internal::g_SystemUBO, 0, sizeof(GDK_SystemState), flags);
            
            // Link the buffer to Binding Point 0 for the Shaders
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, GDK::Internal::g_SystemUBO);
        } else {
            // STANDARD: Traditional CPU-side memory state
            GDK::state = new GDK_SystemState();
        }
    } else {
        // LEGACY PATH: Keep the simple 1.x context
        GDK::hrc = temp; 
        GDK::state = new GDK_SystemState();
    }

    // --- SYSTEM INITIALIZATION ---
    GDK::tStart = std::chrono::high_resolution_clock::now();
    GDK::tFrame = GDK::tStart;
    GDK::state->resolution = {(float)w, (float)h};
    
    glEnable(GL_DEPTH_TEST); 
    glViewport(0, 0, w, h); // Explicitly set the pixel-to-NDC mapping


    // --- MATRIX STACK WIRING ---
    // Point the "Brain" to the memory we just allocated/mapped
    GDK::Internal::g_ModelStack.current = &GDK::state->view;
    GDK::Internal::g_ProjStack.current  = &GDK::state->projection;

    // Reset stacks to Identity
    GDK_ModelMode(); GDK_LoadIdentity();
    GDK_ProjMode();  GDK_LoadIdentity();
    
    GDK_SetCameraUp(0.0f, 1.0f, 0.0f); // Enforced Y-Up
    glEnable(GL_BLEND); // Enable blending for transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard Alpha Blend
    glEnable(GL_SMOOTH); // Enable smooth shading

    return 1;
}


GDK_API void GDK_UpdateSystem() {
    if (!GDK::state) return;

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> diff = now - GDK::tFrame;
    float dt = diff.count();
    GDK::tFrame = now;
    GDK::state->time = std::chrono::duration<float>(now - GDK::tStart).count();

    // 1. THE SYNC (Backup state BEFORE hardware polling)
    GDK::state->oldMouseButtons = GDK::state->mouseButtons;
    GDK::state->oldMousePos = GDK::state->mouse;
    memcpy(GDK::lastKeys, GDK::keys, 256);

    // 2. POLL MESSAGES
    MSG msg; while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }

    // 3. POLL RAW HARDWARE
    POINT p; GetCursorPos(&p); ScreenToClient(GDK::hwnd, &p);
    for(int i=0; i<256; i++) GDK::keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;

    GDK::state->mouseButtons = 0;
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) GDK::state->mouseButtons |= 1;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) GDK::state->mouseButtons |= 2;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) GDK::state->mouseButtons |= 4;


    if (GDK::mLock) {
    RECT r;
    POINT p;
    GetCursorPos(&p);
    GetWindowRect(GDK::hwnd, &r);

    int cx = r.left + (r.right - r.left) / 2;
    int cy = r.top + (r.bottom - r.top) / 2;

    // 1. Calculate Delta (Movement since last frame)
    GDK::mDX = (float)(p.x - cx);
    GDK::mDY = (float)(p.y - cy);

    // 2. Reset Cursor to Center
    // Only reset if the mouse actually moved to save overhead
    if (p.x != cx || p.y != cy) {
        SetCursorPos(cx, cy);
    }

    } else {
        GDK::mDX = (float)p.x - GDK::state->oldMousePos.x;
        GDK::mDY = (float)p.y - GDK::state->oldMousePos.y;
    }

    // 5. UPDATE STATE
    GDK::state->mouse = {(float)p.x, (float)p.y};
    GDK::state->mouseDelta = {GDK::mDX, GDK::mDY};
    GDK::state->deltaTime = (dt <= 0.0f || dt > 1.0f) ? 0.0166f : dt;
    GDK::state->frameCount++;
    GDK::state->fps = 1.0f / GDK::state->deltaTime;
}

GDK_END_DECLS // extern "C" {


#endif
