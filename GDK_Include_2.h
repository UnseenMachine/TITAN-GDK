// Unseen GL Master Include File - v0.4.2025
// NO BOILERPLATE - ONLY THE MASTER INCLUDE
// NO Extras EVER - If it does not NEED to be here, it is NOT here


#ifndef MASTER_GDK_H
#define MASTER_GDK_H

// --- 1. Platform Specific Includes ---
#include <windows.h>

// Fix the Warning: Only define if not already defined by freeglut/system
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
// --- GDK Modern Linkage Macros ---
#ifdef __cplusplus
    // Opens a C-linkage block with a cleaner name
    #define GDK_BEGIN_DECLS extern "C" {
    // Closes the C-linkage block
    #define GDK_END_DECLS   }
#else
    // C compilers don't see these, so they become empty
    #define GDK_BEGIN_DECLS
    #define GDK_END_DECLS
#endif

// Your existing S-Tier Export macro
#ifdef _WIN32
    #define GDK_API __declspec(dllexport)
#else
    #define GDK_API __attribute__((visibility("default")))
#endif


// --- 3. THIRD PARTY LIBRARIES ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// dr_pcx is a single-header lib for Quake-era PCX support
#define DR_PCX_IMPLEMENTATION
#include "dr_pcx.h"

// C/C++ Standard Suite
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdbool.h>
#include <cmath>
#include <type_traits>
#include <cassert>
#include <map>
#include <chrono>
#include <random>


// --- 2. Platform & Graphics Core ---
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/wglext.h>
#include <GL/gl.h>
#include <GL/glu.h>


// --- 4. Automated Linker Truths ---
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glu32.lib")

// Add GLM for Matrix Math
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp" 
#include "glm/gtc/quaternion.hpp" // For MD3 axis manipulation


// Window & Context Management
#include "GDK_CORE_SYSTEM.h"//new System Core
#include "GDK_Lighting.h"   //new Lighting Core
#include "GDK_TEXTURE_2.h"
//#include "GDK_SHAPES_FINAL.h" //
#include "GDK_TERRAIN_FINAL.h"


enum GDK_ModelType { TYPE_NONE = 0, MDL = 1, MD2 = 2, MD3 = 3, OBJ = 4, STL = 5, REVOLT = 6 };

#include "GDK_id_Tech.h"
#include "GDK_MDL.h"
#include "GDK_MD2.h"
#include "GDK_MD3.h"
#include "GDK_STL.h"
#include "GDK_Prm_Dev.h"
#include "GDK_MODEL_ENGINE.h"
#include "GDK_Bsp1.h"
#include "GDK_BSP_Master.h"
//#include "GDK_MD3_ACTOR.h"




// Testing
//#include "Triangle_33.h"
//#include "Cube_Test_33.h"

#endif // MASTER_GDK_H
