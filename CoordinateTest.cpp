/*
 * CoordinateTest.cpp
 * 
 * Test different coordinate retrieval methods in X-Plane
 * Find the most accurate way to get crosshair target coordinates
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
#include "XPLMMenus.h"
#include "XPLMGraphics.h"

// Test all available coordinate datarefs
static XPLMDataRef gClick3DX = NULL;
static XPLMDataRef gClick3DY = NULL;
static XPLMDataRef gClick3DZ = NULL;
static XPLMDataRef gMouseX = NULL;
static XPLMDataRef gMouseY = NULL;
static XPLMDataRef gScreenWidth = NULL;
static XPLMDataRef gScreenHeight = NULL;
static XPLMDataRef gCameraX = NULL;
static XPLMDataRef gCameraY = NULL;
static XPLMDataRef gCameraZ = NULL;
static XPLMDataRef gCameraHeading = NULL;
static XPLMDataRef gCameraPitch = NULL;
static XPLMDataRef gViewType = NULL;

// Aircraft position
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;

// Test functions
static void TestClick3DCallback(void* inRefcon);
static void TestMousePosCallback(void* inRefcon);
static void TestCameraInfoCallback(void* inRefcon);
static void TestScreenCenterCallback(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Coordinate Retrieval Test");
    strcpy(outSig, "coordinate.test");
    strcpy(outDesc, "Test different methods to get crosshair target coordinates");

    // Initialize all coordinate datarefs
    gClick3DX = XPLMFindDataRef("sim/graphics/view/click_3d_x");
    gClick3DY = XPLMFindDataRef("sim/graphics/view/click_3d_y");
    gClick3DZ = XPLMFindDataRef("sim/graphics/view/click_3d_z");
    gMouseX = XPLMFindDataRef("sim/graphics/view/mouse_x");
    gMouseY = XPLMFindDataRef("sim/graphics/view/mouse_y");
    gScreenWidth = XPLMFindDataRef("sim/graphics/view/window_width");
    gScreenHeight = XPLMFindDataRef("sim/graphics/view/window_height");
    gCameraX = XPLMFindDataRef("sim/graphics/view/view_x");
    gCameraY = XPLMFindDataRef("sim/graphics/view/view_y");
    gCameraZ = XPLMFindDataRef("sim/graphics/view/view_z");
    gCameraHeading = XPLMFindDataRef("sim/graphics/view/view_heading");
    gCameraPitch = XPLMFindDataRef("sim/graphics/view/view_pitch");
    gViewType = XPLMFindDataRef("sim/graphics/view/view_type");
    
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");

    // Register test hotkeys
    XPLMRegisterHotKey(XPLM_VK_F7, xplm_DownFlag, "Test: Click 3D Coords", TestClick3DCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, "Test: Mouse Position", TestMousePosCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F10, xplm_DownFlag, "Test: Camera Info", TestCameraInfoCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F11, xplm_DownFlag, "Test: Screen Center", TestScreenCenterCallback, NULL);

    XPLMDebugString("COORDINATE TEST: Plugin loaded\n");
    XPLMDebugString("COORDINATE TEST: F7=Click3D, F8=Mouse, F10=Camera, F11=ScreenCenter\n");
    XPLMDebugString("COORDINATE TEST: Activate FLIR (F9) first, then test these methods\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void TestClick3DCallback(void* inRefcon)
{
    char msg[512];
    
    if (gClick3DX && gClick3DY && gClick3DZ) {
        float clickX = XPLMGetDataf(gClick3DX);
        float clickY = XPLMGetDataf(gClick3DY);
        float clickZ = XPLMGetDataf(gClick3DZ);
        
        snprintf(msg, sizeof(msg), 
            "COORDINATE TEST: CLICK 3D METHOD\n"
            "COORDINATE TEST: Click3D coordinates: (%.0f, %.0f, %.0f)\n"
            "COORDINATE TEST: This should be where mouse cursor hits in 3D world\n",
            clickX, clickY, clickZ);
    } else {
        snprintf(msg, sizeof(msg), "COORDINATE TEST: Click 3D datarefs NOT AVAILABLE\n");
    }
    
    XPLMDebugString(msg);
}

static void TestMousePosCallback(void* inRefcon)
{
    char msg[512];
    
    // Get mouse screen position
    int mouseX, mouseY;
    XPLMGetMouseLocation(&mouseX, &mouseY);
    
    // Get screen dimensions
    int screenW = gScreenWidth ? (int)XPLMGetDataf(gScreenWidth) : 0;
    int screenH = gScreenHeight ? (int)XPLMGetDataf(gScreenHeight) : 0;
    
    // Calculate screen center offset
    int centerX = screenW / 2;
    int centerY = screenH / 2;
    int offsetX = mouseX - centerX;
    int offsetY = mouseY - centerY;
    
    snprintf(msg, sizeof(msg), 
        "COORDINATE TEST: MOUSE POSITION METHOD\n"
        "COORDINATE TEST: Screen: %dx%d, Center: (%d,%d)\n"
        "COORDINATE TEST: Mouse: (%d,%d), Offset from center: (%d,%d)\n"
        "COORDINATE TEST: Could convert this to world coordinates\n",
        screenW, screenH, centerX, centerY, mouseX, mouseY, offsetX, offsetY);
    
    XPLMDebugString(msg);
}

static void TestCameraInfoCallback(void* inRefcon)
{
    char msg[512];
    
    float camX = gCameraX ? XPLMGetDataf(gCameraX) : 0.0f;
    float camY = gCameraY ? XPLMGetDataf(gCameraY) : 0.0f;
    float camZ = gCameraZ ? XPLMGetDataf(gCameraZ) : 0.0f;
    float camHeading = gCameraHeading ? XPLMGetDataf(gCameraHeading) : 0.0f;
    float camPitch = gCameraPitch ? XPLMGetDataf(gCameraPitch) : 0.0f;
    int viewType = gViewType ? XPLMGetDatai(gViewType) : 0;
    
    snprintf(msg, sizeof(msg), 
        "COORDINATE TEST: CAMERA INFO METHOD\n"
        "COORDINATE TEST: Camera pos: (%.0f, %.0f, %.0f)\n"
        "COORDINATE TEST: Camera angles: Heading %.1f°, Pitch %.1f°\n"
        "COORDINATE TEST: View type: %d (should be custom view when FLIR active)\n",
        camX, camY, camZ, camHeading, camPitch, viewType);
    
    XPLMDebugString(msg);
}

static void TestScreenCenterCallback(void* inRefcon)
{
    char msg[512];
    
    // Get screen center coordinates
    int screenW = gScreenWidth ? (int)XPLMGetDataf(gScreenWidth) : 1024;
    int screenH = gScreenHeight ? (int)XPLMGetDataf(gScreenHeight) : 768;
    int centerX = screenW / 2;
    int centerY = screenH / 2;
    
    // Get aircraft position for reference
    float planeX = gAircraftX ? XPLMGetDataf(gAircraftX) : 0.0f;
    float planeY = gAircraftY ? XPLMGetDataf(gAircraftY) : 0.0f;
    float planeZ = gAircraftZ ? XPLMGetDataf(gAircraftZ) : 0.0f;
    
    snprintf(msg, sizeof(msg), 
        "COORDINATE TEST: SCREEN CENTER METHOD\n"
        "COORDINATE TEST: Screen center: (%d, %d) of %dx%d\n"
        "COORDINATE TEST: Aircraft reference: (%.0f, %.0f, %.0f)\n"
        "COORDINATE TEST: Need to cast ray from camera through screen center\n",
        centerX, centerY, screenW, screenH, planeX, planeY, planeZ);
    
    XPLMDebugString(msg);
}