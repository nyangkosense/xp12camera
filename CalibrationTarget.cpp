/*
 * CalibrationTarget.cpp
 * 
 * Manual calibration system for FLIR targeting
 * Learn the offset between crosshair position and actual missile impact
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

// Calibration data
static float gCalibrationOffsetX = 0.0f;
static float gCalibrationOffsetY = 0.0f;
static float gCalibrationOffsetZ = 0.0f;
static int gCalibrationActive = 0;

// Camera datarefs
static XPLMDataRef gCameraX = NULL;
static XPLMDataRef gCameraY = NULL;
static XPLMDataRef gCameraZ = NULL;
static XPLMDataRef gCameraHeading = NULL;
static XPLMDataRef gCameraPitch = NULL;

// Aircraft position
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;

// Calibration points
static float gCrosshairPointX = 0.0f;
static float gCrosshairPointY = 0.0f;
static float gCrosshairPointZ = 0.0f;
static float gActualHitX = 0.0f;
static float gActualHitY = 0.0f;
static float gActualHitZ = 0.0f;
static int gCrosshairMarked = 0;
static int gActualHitMarked = 0;

// Test functions
static void MarkCrosshairPositionCallback(void* inRefcon);
static void MarkActualHitCallback(void* inRefcon);
static void CalculateCalibrationCallback(void* inRefcon);
static void TestCalibratedTargetCallback(void* inRefcon);
static void ResetCalibrationCallback(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Calibration Target System");
    strcpy(outSig, "calibration.target");
    strcpy(outDesc, "Manual calibration system for accurate FLIR targeting");

    // Initialize camera datarefs
    gCameraX = XPLMFindDataRef("sim/graphics/view/view_x");
    gCameraY = XPLMFindDataRef("sim/graphics/view/view_y");
    gCameraZ = XPLMFindDataRef("sim/graphics/view/view_z");
    gCameraHeading = XPLMFindDataRef("sim/graphics/view/view_heading");
    gCameraPitch = XPLMFindDataRef("sim/graphics/view/view_pitch");
    
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");

    // Register calibration hotkeys
    XPLMRegisterHotKey(XPLM_VK_1, xplm_DownFlag, "Cal: Mark Crosshair Position", MarkCrosshairPositionCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_2, xplm_DownFlag, "Cal: Mark Actual Hit", MarkActualHitCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_3, xplm_DownFlag, "Cal: Calculate Offset", CalculateCalibrationCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_4, xplm_DownFlag, "Cal: Test Calibrated Target", TestCalibratedTargetCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_0, xplm_DownFlag, "Cal: Reset Calibration", ResetCalibrationCallback, NULL);

    XPLMDebugString("CALIBRATION: Plugin loaded\n");
    XPLMDebugString("CALIBRATION: Calibration Process:\n");
    XPLMDebugString("CALIBRATION: 1. Point crosshair at target, press '1' (mark crosshair)\n");
    XPLMDebugString("CALIBRATION: 2. Fire missile, see where it hits, press '2' (mark hit)\n");
    XPLMDebugString("CALIBRATION: 3. Press '3' to calculate offset\n");
    XPLMDebugString("CALIBRATION: 4. Press '4' to test calibrated targeting\n");
    XPLMDebugString("CALIBRATION: 0 = Reset calibration\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void MarkCrosshairPositionCallback(void* inRefcon)
{
    if (!gCameraX || !gCameraY || !gCameraZ || !gCameraHeading || !gCameraPitch) {
        XPLMDebugString("CALIBRATION: Camera datarefs not available\n");
        return;
    }
    
    // Get camera position and calculate where crosshair is pointing
    float camX = XPLMGetDataf(gCameraX);
    float camY = XPLMGetDataf(gCameraY);
    float camZ = XPLMGetDataf(gCameraZ);
    float heading = XPLMGetDataf(gCameraHeading);
    float pitch = XPLMGetDataf(gCameraPitch);
    
    // Simple ray casting to ground
    float headingRad = heading * M_PI / 180.0f;
    float pitchRad = pitch * M_PI / 180.0f;
    
    float rayDirX = sin(headingRad) * cos(pitchRad);
    float rayDirY = sin(pitchRad);
    float rayDirZ = cos(headingRad) * cos(pitchRad);
    
    float t = -camY / rayDirY;
    
    gCrosshairPointX = camX + rayDirX * t;
    gCrosshairPointY = 0.0f;
    gCrosshairPointZ = camZ + rayDirZ * t;
    gCrosshairMarked = 1;
    
    char msg[256];
    snprintf(msg, sizeof(msg), 
        "CALIBRATION: CROSSHAIR MARKED at (%.0f, %.0f, %.0f)\n"
        "CALIBRATION: Now fire missile and see where it hits, then press '2'\n",
        gCrosshairPointX, gCrosshairPointY, gCrosshairPointZ);
    XPLMDebugString(msg);
}

static void MarkActualHitCallback(void* inRefcon)
{
    // For now, use aircraft position as approximation of where missile hit
    // User should fly to where missile actually hit and press this key
    if (!gAircraftX || !gAircraftY || !gAircraftZ) {
        XPLMDebugString("CALIBRATION: Aircraft position not available\n");
        return;
    }
    
    gActualHitX = XPLMGetDataf(gAircraftX);
    gActualHitY = 0.0f; // Ground level
    gActualHitZ = XPLMGetDataf(gAircraftZ);
    gActualHitMarked = 1;
    
    char msg[256];
    snprintf(msg, sizeof(msg), 
        "CALIBRATION: ACTUAL HIT MARKED at (%.0f, %.0f, %.0f)\n"
        "CALIBRATION: Press '3' to calculate offset\n",
        gActualHitX, gActualHitY, gActualHitZ);
    XPLMDebugString(msg);
}

static void CalculateCalibrationCallback(void* inRefcon)
{
    if (!gCrosshairMarked || !gActualHitMarked) {
        XPLMDebugString("CALIBRATION: Need to mark both crosshair (1) and actual hit (2) first\n");
        return;
    }
    
    // Calculate offset between where we aimed vs where missile went
    gCalibrationOffsetX = gActualHitX - gCrosshairPointX;
    gCalibrationOffsetY = gActualHitY - gCrosshairPointY;
    gCalibrationOffsetZ = gActualHitZ - gCrosshairPointZ;
    gCalibrationActive = 1;
    
    float distance = sqrt(gCalibrationOffsetX*gCalibrationOffsetX + 
                         gCalibrationOffsetZ*gCalibrationOffsetZ);
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "CALIBRATION: OFFSET CALCULATED\n"
        "CALIBRATION: Crosshair aimed at: (%.0f, %.0f, %.0f)\n"
        "CALIBRATION: Missile hit at: (%.0f, %.0f, %.0f)\n"
        "CALIBRATION: Offset: (%.0f, %.0f, %.0f) - Distance: %.0fm\n"
        "CALIBRATION: Calibration is now ACTIVE\n"
        "CALIBRATION: Press '4' to test calibrated targeting\n",
        gCrosshairPointX, gCrosshairPointY, gCrosshairPointZ,
        gActualHitX, gActualHitY, gActualHitZ,
        gCalibrationOffsetX, gCalibrationOffsetY, gCalibrationOffsetZ, distance);
    XPLMDebugString(msg);
}

static void TestCalibratedTargetCallback(void* inRefcon)
{
    if (!gCalibrationActive) {
        XPLMDebugString("CALIBRATION: No calibration active - complete calibration process first\n");
        return;
    }
    
    if (!gCameraX || !gCameraY || !gCameraZ || !gCameraHeading || !gCameraPitch) {
        XPLMDebugString("CALIBRATION: Camera datarefs not available\n");
        return;
    }
    
    // Calculate where crosshair is currently pointing
    float camX = XPLMGetDataf(gCameraX);
    float camY = XPLMGetDataf(gCameraY);
    float camZ = XPLMGetDataf(gCameraZ);
    float heading = XPLMGetDataf(gCameraHeading);
    float pitch = XPLMGetDataf(gCameraPitch);
    
    float headingRad = heading * M_PI / 180.0f;
    float pitchRad = pitch * M_PI / 180.0f;
    
    float rayDirX = sin(headingRad) * cos(pitchRad);
    float rayDirY = sin(pitchRad);
    float rayDirZ = cos(headingRad) * cos(pitchRad);
    
    float t = -camY / rayDirY;
    
    float crosshairX = camX + rayDirX * t;
    float crosshairY = 0.0f;
    float crosshairZ = camZ + rayDirZ * t;
    
    // Apply calibration offset to get corrected target
    float correctedTargetX = crosshairX + gCalibrationOffsetX;
    float correctedTargetY = crosshairY + gCalibrationOffsetY;
    float correctedTargetZ = crosshairZ + gCalibrationOffsetZ;
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "CALIBRATION: CALIBRATED TARGETING\n"
        "CALIBRATION: Raw crosshair: (%.0f, %.0f, %.0f)\n"
        "CALIBRATION: Applied offset: (%.0f, %.0f, %.0f)\n"
        "CALIBRATION: Corrected target: (%.0f, %.0f, %.0f)\n"
        "CALIBRATION: Use these coordinates for missile targeting!\n",
        crosshairX, crosshairY, crosshairZ,
        gCalibrationOffsetX, gCalibrationOffsetY, gCalibrationOffsetZ,
        correctedTargetX, correctedTargetY, correctedTargetZ);
    XPLMDebugString(msg);
}

static void ResetCalibrationCallback(void* inRefcon)
{
    gCalibrationOffsetX = 0.0f;
    gCalibrationOffsetY = 0.0f;
    gCalibrationOffsetZ = 0.0f;
    gCalibrationActive = 0;
    gCrosshairMarked = 0;
    gActualHitMarked = 0;
    
    XPLMDebugString("CALIBRATION: Calibration reset - start over with step 1\n");
}