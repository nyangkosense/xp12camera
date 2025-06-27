/*
 * IntegratedGuidance.cpp
 * 
 * Integration with existing FLIR camera system for missile guidance
 * Uses gCameraPan and gCameraTilt from FLIR_Camera.cpp
 * Workflow: F9 → Crosshair → SPACEBAR → Fire → F2 for guidance
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
#include "FLIR_SimpleLock.h"

// Weapon datarefs (proven working)
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;

// Aircraft position and heading
static XPLMDataRef gPlaneX = NULL;
static XPLMDataRef gPlaneY = NULL;
static XPLMDataRef gPlaneZ = NULL;
static XPLMDataRef gPlaneHeading = NULL;

// FLIR camera datarefs (we'll create these in the FLIR plugin)
static XPLMDataRef gFLIRPan = NULL;
static XPLMDataRef gFLIRTilt = NULL;
static XPLMDataRef gFLIRActive = NULL;

// Integrated guidance state
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static int gTargetReady = 0;
static int gGuidanceActive = 0;
static XPLMFlightLoopID gGuidanceLoop = NULL;

// Precision guidance parameters (refined from testing)
static float gMaxCorrectionSpeed = 15.0f;  // Gentle corrections
static float gProportionalGain = 1.0f;     // Balanced response
static float gDampingFactor = 0.85f;       // Smooth control
static float gMinTargetDistance = 50.0f;   // Stop steering when close
static float gMaxTargetDistance = 8000.0f; // Extended range

// Function declarations
static void ActivateGuidanceCallback(void* inRefcon);
static void StopGuidanceCallback(void* inRefcon);
static void ManualTargetCallback(void* inRefcon);
static float IntegratedGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void ApplyPrecisionGuidance();
static void LogGuidanceStatus();
static void CalculateTargetFromFLIR();

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Integrated FLIR Guidance");
    strcpy(outSig, "integrated.flir.guidance");
    strcpy(outDesc, "Integrated FLIR camera targeting with precision missile guidance");

    // Initialize weapon datarefs
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    
    // Initialize aircraft datarefs
    gPlaneX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gPlaneY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gPlaneZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gPlaneHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    
    // Try to find FLIR camera datarefs (will be NULL if FLIR plugin not loaded)
    gFLIRPan = XPLMFindDataRef("flir/camera/pan");
    gFLIRTilt = XPLMFindDataRef("flir/camera/tilt");
    gFLIRActive = XPLMFindDataRef("flir/camera/active");

    // Register hotkeys for guidance control
    XPLMRegisterHotKey(XPLM_VK_F2, xplm_DownFlag, "IG: Start Guidance", ActivateGuidanceCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F3, xplm_DownFlag, "IG: Stop Guidance", StopGuidanceCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F4, xplm_DownFlag, "IG: Manual Target Lock", ManualTargetCallback, NULL);

    XPLMDebugString("INTEGRATED GUIDANCE: Plugin loaded\n");
    XPLMDebugString("INTEGRATED GUIDANCE: F9→Crosshair→SPACE→Fire→F2\n");
    XPLMDebugString("INTEGRATED GUIDANCE: F2=Start, F3=Stop, F4=Manual Target\n");

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gGuidanceActive && gGuidanceLoop) {
        XPLMScheduleFlightLoop(gGuidanceLoop, 0, 0);
    }
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void ActivateGuidanceCallback(void* inRefcon)
{
    if (gGuidanceActive) {
        XPLMDebugString("INTEGRATED GUIDANCE: Guidance already active\n");
        return;
    }
    
    // Check if target is available from FLIR system
    if (!IsTargetDesignated()) {
        XPLMDebugString("INTEGRATED GUIDANCE: No FLIR target! Use F9→Crosshair→SPACE first\n");
        return;
    }
    
    // Get target from FLIR system (it's already calculated by DesignateTarget)
    // For now, calculate it fresh using current camera position
    CalculateTargetFromFLIR();
    
    if (!gTargetReady) {
        XPLMDebugString("INTEGRATED GUIDANCE: Target calculation failed\n");
        return;
    }
    
    gGuidanceActive = 1;
    
    if (!gGuidanceLoop) {
        XPLMCreateFlightLoop_t flightLoopParams;
        flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
        flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
        flightLoopParams.callbackFunc = IntegratedGuidanceCallback;
        flightLoopParams.refcon = NULL;
        
        gGuidanceLoop = XPLMCreateFlightLoop(&flightLoopParams);
    }
    
    if (gGuidanceLoop) {
        XPLMScheduleFlightLoop(gGuidanceLoop, 0.05f, 1); // 20Hz for smooth control
        
        char msg[256];
        snprintf(msg, sizeof(msg), 
                "INTEGRATED GUIDANCE: STARTED → Target (%.0f, %.0f, %.0f)\n", 
                gTargetX, gTargetY, gTargetZ);
        XPLMDebugString(msg);
    }
}

static void StopGuidanceCallback(void* inRefcon)
{
    if (!gGuidanceActive) {
        XPLMDebugString("INTEGRATED GUIDANCE: Guidance not active\n");
        return;
    }
    
    gGuidanceActive = 0;
    if (gGuidanceLoop) {
        XPLMScheduleFlightLoop(gGuidanceLoop, 0, 0);
        XPLMDebugString("INTEGRATED GUIDANCE: STOPPED\n");
    }
}

static void ManualTargetCallback(void* inRefcon)
{
    // Manual target lock using current FLIR camera direction
    int cameraActive = 0;
    if (gFLIRActive) {
        cameraActive = XPLMGetDatai(gFLIRActive);
    }
    
    if (!cameraActive) {
        XPLMDebugString("INTEGRATED GUIDANCE: FLIR camera not active! Press F9 first\n");
        return;
    }
    
    CalculateTargetFromFLIR();
    
    if (gTargetReady) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
                "INTEGRATED GUIDANCE: Manual target locked (%.0f, %.0f, %.0f)\n", 
                gTargetX, gTargetY, gTargetZ);
        XPLMDebugString(msg);
        XPLMDebugString("INTEGRATED GUIDANCE: Fire weapons, then press F2 to start guidance\n");
    } else {
        XPLMDebugString("INTEGRATED GUIDANCE: Manual target lock failed\n");
    }
}

static void CalculateTargetFromFLIR()
{
    // Use the same logic as your DesignateTarget function
    if (!gPlaneX || !gPlaneY || !gPlaneZ || !gPlaneHeading) {
        XPLMDebugString("INTEGRATED GUIDANCE: Aircraft position unavailable\n");
        gTargetReady = 0;
        return;
    }
    
    float planeX = XPLMGetDataf(gPlaneX);
    float planeY = XPLMGetDataf(gPlaneY);
    float planeZ = XPLMGetDataf(gPlaneZ);
    float planeHeading = XPLMGetDataf(gPlaneHeading);
    
    // Use current FLIR camera angles from datarefs or fallback values
    float panAngle = 0.0f;
    float tiltAngle = -15.0f;
    
    if (gFLIRPan && gFLIRTilt) {
        panAngle = XPLMGetDataf(gFLIRPan);
        tiltAngle = XPLMGetDataf(gFLIRTilt);
    } else {
        XPLMDebugString("INTEGRATED GUIDANCE: FLIR datarefs not found, using default angles\\n");
    }
    
    // Convert to radians
    float headingRad = (planeHeading + panAngle) * M_PI / 180.0f;
    float tiltRad = tiltAngle * M_PI / 180.0f;
    
    // Calculate target range based on tilt (same as your DesignateTarget)
    double targetRange = 5000.0; // Default 5km
    if (tiltAngle < -10.0f) {
        targetRange = fabs(planeY / sin(tiltRad));
        if (targetRange > 50000.0) targetRange = 50000.0;
        if (targetRange < 1000.0) targetRange = 1000.0;
    }
    
    // Calculate target coordinates
    double deltaX = targetRange * sin(headingRad) * cos(tiltRad);
    double deltaY = targetRange * sin(tiltRad);
    double deltaZ = targetRange * cos(headingRad) * cos(tiltRad);
    
    gTargetX = planeX + (float)deltaX;
    gTargetY = planeY + (float)deltaY;
    gTargetZ = planeZ + (float)deltaZ;
    gTargetReady = 1;
    
    char debugMsg[256];
    snprintf(debugMsg, sizeof(debugMsg), 
            "INTEGRATED GUIDANCE: Target calc - Pan:%.1f° Tilt:%.1f° Range:%.0fm\n", 
            panAngle, tiltAngle, targetRange);
    XPLMDebugString(debugMsg);
}

static void ApplyPrecisionGuidance()
{
    if (!gTargetReady || !gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    float x[25] = {0}, y[25] = {0}, z[25] = {0};
    float vx[25] = {0}, vy[25] = {0}, vz[25] = {0};
    
    XPLMGetDatavf(gWeaponX, x, 0, weaponCount);
    XPLMGetDatavf(gWeaponY, y, 0, weaponCount);
    XPLMGetDatavf(gWeaponZ, z, 0, weaponCount);
    XPLMGetDatavf(gWeaponVX, vx, 0, weaponCount);
    XPLMGetDatavf(gWeaponVY, vy, 0, weaponCount);
    XPLMGetDatavf(gWeaponVZ, vz, 0, weaponCount);
    
    float newVX[25] = {0}, newVY[25] = {0}, newVZ[25] = {0};
    
    // Apply precision guidance to weapons 0 and 1 only
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        if (x[i] == 0 && y[i] == 0 && z[i] == 0) {
            // Inactive missile
            newVX[i] = vx[i];
            newVY[i] = vy[i];
            newVZ[i] = vz[i];
            continue;
        }
        
        // Calculate vector to FLIR target
        float dx = gTargetX - x[i];
        float dy = gTargetY - y[i];
        float dz = gTargetZ - z[i];
        
        float distance = sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance < gMinTargetDistance) {
            // Very close - gentle final approach
            newVX[i] = vx[i] * gDampingFactor;
            newVY[i] = vy[i] * gDampingFactor;
            newVZ[i] = vz[i] * gDampingFactor;
            continue;
        }
        
        if (distance > gMaxTargetDistance) {
            // Too far - maintain trajectory
            newVX[i] = vx[i];
            newVY[i] = vy[i];
            newVZ[i] = vz[i];
            continue;
        }
        
        // Normalize direction to target
        float dirX = dx / distance;
        float dirY = dy / distance;
        float dirZ = dz / distance;
        
        // Speed control based on distance
        float desiredSpeed = fmin(distance * 0.08f, 120.0f); // Max 120 m/s
        if (desiredSpeed < 15.0f) desiredSpeed = 15.0f;      // Min 15 m/s
        
        // Calculate desired velocity
        float desiredVX = dirX * desiredSpeed;
        float desiredVY = dirY * desiredSpeed;
        float desiredVZ = dirZ * desiredSpeed;
        
        // Calculate correction needed
        float correctionX = (desiredVX - vx[i]) * gProportionalGain;
        float correctionY = (desiredVY - vy[i]) * gProportionalGain;
        float correctionZ = (desiredVZ - vz[i]) * gProportionalGain;
        
        // Limit correction magnitude for stability
        float correctionMag = sqrt(correctionX*correctionX + correctionY*correctionY + correctionZ*correctionZ);
        if (correctionMag > gMaxCorrectionSpeed) {
            float scale = gMaxCorrectionSpeed / correctionMag;
            correctionX *= scale;
            correctionY *= scale;
            correctionZ *= scale;
        }
        
        // Apply smooth correction with damping
        newVX[i] = (vx[i] + correctionX) * gDampingFactor;
        newVY[i] = (vy[i] + correctionY) * gDampingFactor;
        newVZ[i] = (vz[i] + correctionZ) * gDampingFactor;
    }
    
    // Set new velocities
    XPLMSetDatavf(gWeaponVX, newVX, 0, weaponCount);
    XPLMSetDatavf(gWeaponVY, newVY, 0, weaponCount);
    XPLMSetDatavf(gWeaponVZ, newVZ, 0, weaponCount);
}

static void LogGuidanceStatus()
{
    if (!gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    float x[25] = {0}, y[25] = {0}, z[25] = {0};
    float vx[25] = {0}, vy[25] = {0}, vz[25] = {0};
    
    XPLMGetDatavf(gWeaponX, x, 0, weaponCount);
    XPLMGetDatavf(gWeaponY, y, 0, weaponCount);
    XPLMGetDatavf(gWeaponZ, z, 0, weaponCount);
    XPLMGetDatavf(gWeaponVX, vx, 0, weaponCount);
    XPLMGetDatavf(gWeaponVY, vy, 0, weaponCount);
    XPLMGetDatavf(gWeaponVZ, vz, 0, weaponCount);
    
    // Log active missiles only
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        if (x[i] != 0 || y[i] != 0 || z[i] != 0) {
            float dx = gTargetX - x[i];
            float dy = gTargetY - y[i];
            float dz = gTargetZ - z[i];
            float distance = sqrt(dx*dx + dy*dy + dz*dz);
            float speed = sqrt(vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i]);
            
            char msg[512];
            snprintf(msg, sizeof(msg), 
                "INTEGRATED GUIDANCE: [%d] Pos:(%.0f,%.0f,%.0f) Vel:(%.1f,%.1f,%.1f) Speed:%.1f Dist:%.0f\n",
                i, x[i], y[i], z[i], vx[i], vy[i], vz[i], speed, distance);
            XPLMDebugString(msg);
        }
    }
}

static float IntegratedGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gGuidanceActive) return 0;
    
    // Apply precision guidance
    ApplyPrecisionGuidance();
    
    // Log status every 3 seconds
    static int logCounter = 0;
    logCounter++;
    if (logCounter % 60 == 0) { // 60 * 0.05s = 3 seconds
        LogGuidanceStatus();
    }
    
    return 0.05f; // 20Hz for smooth control
}