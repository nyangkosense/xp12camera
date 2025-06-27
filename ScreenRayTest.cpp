/*
 * ScreenRayTest.cpp
 * 
 * Test screen center ray casting to ground intersection
 * Calculate where camera center ray hits the ground plane
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// Camera datarefs
static XPLMDataRef gCameraX = NULL;
static XPLMDataRef gCameraY = NULL;
static XPLMDataRef gCameraZ = NULL;
static XPLMDataRef gCameraHeading = NULL;
static XPLMDataRef gCameraPitch = NULL;
static XPLMDataRef gCameraRoll = NULL;

// Screen datarefs
static XPLMDataRef gScreenWidth = NULL;
static XPLMDataRef gScreenHeight = NULL;
static XPLMDataRef gMouseX = NULL;
static XPLMDataRef gMouseY = NULL;

// Test functions
static void TestScreenCenterRayCallback(void* inRefcon);
static void TestMouseRayCallback(void* inRefcon);
static void CalculateGroundIntersection(float camX, float camY, float camZ, float heading, float pitch, float* outX, float* outY, float* outZ);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Screen Ray Test");
    strcpy(outSig, "screen.ray.test");
    strcpy(outDesc, "Test screen center ray casting to ground intersection");

    // Initialize camera datarefs
    gCameraX = XPLMFindDataRef("sim/graphics/view/view_x");
    gCameraY = XPLMFindDataRef("sim/graphics/view/view_y");
    gCameraZ = XPLMFindDataRef("sim/graphics/view/view_z");
    gCameraHeading = XPLMFindDataRef("sim/graphics/view/view_heading");
    gCameraPitch = XPLMFindDataRef("sim/graphics/view/view_pitch");
    gCameraRoll = XPLMFindDataRef("sim/graphics/view/view_roll");
    
    // Screen datarefs
    gScreenWidth = XPLMFindDataRef("sim/graphics/view/window_width");
    gScreenHeight = XPLMFindDataRef("sim/graphics/view/window_height");
    gMouseX = XPLMFindDataRef("sim/graphics/view/mouse_x");
    gMouseY = XPLMFindDataRef("sim/graphics/view/mouse_y");

    // Register test hotkeys
    XPLMRegisterHotKey(XPLM_VK_R, xplm_DownFlag, "Test: Screen Center Ray", TestScreenCenterRayCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_M, xplm_DownFlag, "Test: Mouse Position Ray", TestMouseRayCallback, NULL);

    XPLMDebugString("SCREEN RAY TEST: Plugin loaded\n");
    XPLMDebugString("SCREEN RAY TEST: R=Screen Center Ray, M=Mouse Ray\n");
    XPLMDebugString("SCREEN RAY TEST: Activate FLIR first, then test rays\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void TestScreenCenterRayCallback(void* inRefcon)
{
    if (!gCameraX || !gCameraY || !gCameraZ || !gCameraHeading || !gCameraPitch) {
        XPLMDebugString("SCREEN RAY TEST: Camera datarefs not available\n");
        return;
    }
    
    // Get camera data
    float camX = XPLMGetDataf(gCameraX);
    float camY = XPLMGetDataf(gCameraY);
    float camZ = XPLMGetDataf(gCameraZ);
    float heading = XPLMGetDataf(gCameraHeading);
    float pitch = XPLMGetDataf(gCameraPitch);
    
    // Calculate ground intersection
    float targetX, targetY, targetZ;
    CalculateGroundIntersection(camX, camY, camZ, heading, pitch, &targetX, &targetY, &targetZ);
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "SCREEN RAY TEST: SCREEN CENTER RAY CASTING\n"
        "SCREEN RAY TEST: Camera: (%.0f, %.0f, %.0f)\n"
        "SCREEN RAY TEST: Angles: Heading %.1f°, Pitch %.1f°\n"
        "SCREEN RAY TEST: Ground Hit: (%.0f, %.0f, %.0f)\n"
        "SCREEN RAY TEST: Distance: %.0f meters\n",
        camX, camY, camZ, heading, pitch, targetX, targetY, targetZ,
        sqrt((targetX-camX)*(targetX-camX) + (targetY-camY)*(targetY-camY) + (targetZ-camZ)*(targetZ-camZ)));
    
    XPLMDebugString(msg);
}

static void TestMouseRayCallback(void* inRefcon)
{
    // Get mouse position
    int mouseX, mouseY;
    XPLMGetMouseLocation(&mouseX, &mouseY);
    
    // Get screen size (fallback if datarefs don't work)
    int screenW = gScreenWidth ? (int)XPLMGetDataf(gScreenWidth) : 1920;
    int screenH = gScreenHeight ? (int)XPLMGetDataf(gScreenHeight) : 1080;
    
    if (screenW == 0 || screenH == 0) {
        screenW = 1920;
        screenH = 1080;
    }
    
    // Calculate offset from screen center
    int centerX = screenW / 2;
    int centerY = screenH / 2;
    int offsetX = mouseX - centerX;
    int offsetY = mouseY - centerY;
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "SCREEN RAY TEST: MOUSE RAY CASTING\n"
        "SCREEN RAY TEST: Screen: %dx%d, Center: (%d, %d)\n"
        "SCREEN RAY TEST: Mouse: (%d, %d), Offset: (%d, %d)\n"
        "SCREEN RAY TEST: TODO: Convert mouse offset to angle offset\n",
        screenW, screenH, centerX, centerY, mouseX, mouseY, offsetX, offsetY);
    
    XPLMDebugString(msg);
}

static void CalculateGroundIntersection(float camX, float camY, float camZ, float heading, float pitch, float* outX, float* outY, float* outZ)
{
    // Convert angles to radians
    float headingRad = heading * M_PI / 180.0f;
    float pitchRad = pitch * M_PI / 180.0f;
    
    // Calculate ray direction vector
    float rayDirX = sin(headingRad) * cos(pitchRad);
    float rayDirY = sin(pitchRad);
    float rayDirZ = cos(headingRad) * cos(pitchRad);
    
    // Find intersection with ground plane (Y = 0)
    // Ray equation: P = camPos + t * rayDir
    // Ground plane: Y = 0
    // Solve: camY + t * rayDirY = 0
    
    if (fabs(rayDirY) < 0.001f) {
        // Ray is nearly horizontal - use maximum range
        float maxRange = 10000.0f;
        *outX = camX + rayDirX * maxRange;
        *outY = 0.0f;
        *outZ = camZ + rayDirZ * maxRange;
    } else {
        // Calculate intersection parameter
        float t = -camY / rayDirY;
        
        // Calculate intersection point
        *outX = camX + rayDirX * t;
        *outY = 0.0f;  // Ground level
        *outZ = camZ + rayDirZ * t;
    }
    
    // Clamp to reasonable range
    float distance = sqrt((*outX - camX) * (*outX - camX) + (*outZ - camZ) * (*outZ - camZ));
    if (distance > 20000.0f) {
        float scale = 20000.0f / distance;
        *outX = camX + ((*outX - camX) * scale);
        *outZ = camZ + ((*outZ - camZ) * scale);
    }
}