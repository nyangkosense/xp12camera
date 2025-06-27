/*
 * CrosshairTest.cpp
 * 
 * Simple test to verify crosshair direction calculation
 * Tests if pan angle convention matches actual camera movement
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

// External access to FLIR camera variables
extern int gCameraActive;
extern float gCameraPan;
extern float gCameraTilt;

// Aircraft position datarefs
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;

// Test function declarations
static void TestCenterCallback(void* inRefcon);
static void TestLeftCallback(void* inRefcon);
static void TestRightCallback(void* inRefcon);
static void TestForwardCallback(void* inRefcon);
static void TestBackwardCallback(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Crosshair Direction Test");
    strcpy(outSig, "crosshair.test");
    strcpy(outDesc, "Test crosshair direction calculation accuracy");

    // Initialize aircraft datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");

    // Register test hotkeys
    XPLMRegisterHotKey(XPLM_VK_NUMPAD5, xplm_DownFlag, "Test: Center", TestCenterCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_NUMPAD4, xplm_DownFlag, "Test: Left", TestLeftCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_NUMPAD6, xplm_DownFlag, "Test: Right", TestRightCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_NUMPAD8, xplm_DownFlag, "Test: Forward", TestForwardCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_NUMPAD2, xplm_DownFlag, "Test: Backward", TestBackwardCallback, NULL);

    XPLMDebugString("CROSSHAIR TEST: Plugin loaded\n");
    XPLMDebugString("CROSSHAIR TEST: Numpad 5=Center, 4=Left, 6=Right, 8=Forward, 2=Backward\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void TestDirection(const char* directionName, float testPan, float testTilt)
{
    if (!gCameraActive) {
        XPLMDebugString("CROSSHAIR TEST: FLIR camera not active! Press F9 first\n");
        return;
    }
    
    if (!gAircraftX || !gAircraftY || !gAircraftZ || !gAircraftHeading) {
        XPLMDebugString("CROSSHAIR TEST: Aircraft position unavailable\n");
        return;
    }
    
    float planeX = XPLMGetDataf(gAircraftX);
    float planeY = XPLMGetDataf(gAircraftY);
    float planeZ = XPLMGetDataf(gAircraftZ);
    float planeHeading = XPLMGetDataf(gAircraftHeading);
    
    // Use test angles
    float panAngle = testPan;
    float tiltAngle = testTilt;
    
    // Calculate target coordinates using same logic as DesignateTarget
    float headingRad = (planeHeading + panAngle) * M_PI / 180.0f;
    float tiltRad = tiltAngle * M_PI / 180.0f;
    
    double targetRange = 2000.0; // Fixed range for consistent testing
    
    double deltaX = targetRange * sin(headingRad) * cos(tiltRad);
    double deltaZ = targetRange * cos(headingRad) * cos(tiltRad);
    double deltaY = targetRange * sin(tiltRad);
    
    float targetX = planeX + (float)deltaX;
    float targetY = planeY + (float)deltaY;
    float targetZ = planeZ + (float)deltaZ;
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "CROSSHAIR TEST: %s DIRECTION\n"
        "CROSSHAIR TEST: Aircraft:(%.0f,%.0f,%.0f) Heading:%.1f°\n"
        "CROSSHAIR TEST: Camera Pan:%.1f° Tilt:%.1f° → Look:%.1f°\n"
        "CROSSHAIR TEST: Range:%.0fm → Target:(%.0f,%.0f,%.0f)\n"
        "CROSSHAIR TEST: Delta from aircraft: X:%.0f Y:%.0f Z:%.0f\n", 
        directionName,
        planeX, planeY, planeZ, planeHeading,
        panAngle, tiltAngle, (planeHeading + panAngle),
        targetRange, targetX, targetY, targetZ,
        (float)deltaX, (float)deltaY, (float)deltaZ);
    XPLMDebugString(msg);
}

static void TestCenterCallback(void* inRefcon)
{
    TestDirection("CENTER", 0.0f, -10.0f);
}

static void TestLeftCallback(void* inRefcon)
{
    TestDirection("LEFT", -30.0f, -10.0f);
}

static void TestRightCallback(void* inRefcon)
{
    TestDirection("RIGHT", +30.0f, -10.0f);
}

static void TestForwardCallback(void* inRefcon)
{
    TestDirection("FORWARD", 0.0f, -10.0f);
}

static void TestBackwardCallback(void* inRefcon)
{
    TestDirection("BACKWARD", 180.0f, -10.0f);
}