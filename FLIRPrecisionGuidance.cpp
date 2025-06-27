/*
 * FLIRPrecisionGuidance.cpp
 * 
 * Integration of working precision guidance with FLIR camera targeting
 * Uses proven velocity-based control (vx/vy/vz) that actually moves missiles
 * Reads FLIR camera angles directly from FLIR_Camera.cpp variables
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

// External access to FLIR camera variables (from FLIR_Camera.cpp)
extern int gCameraActive;
extern float gCameraPan;
extern float gCameraTilt;

// Weapon datarefs (proven working)
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;

// Aircraft position and heading
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;

// Precision guidance control
static int gGuidanceActive = 0;
static XPLMFlightLoopID gGuidanceLoop = NULL;

// Target definition from FLIR camera
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static int gTargetSet = 0;

// Refined control parameters (proven working from tests)
static float gMaxCorrectionSpeed = 15.0f;  // Maximum velocity correction per update (m/s)
static float gProportionalGain = 1.0f;     // Balanced response
static float gDampingFactor = 0.85f;       // Smooth control
static float gMinTargetDistance = 50.0f;   // Stop steering when close
static float gMaxTargetDistance = 8000.0f; // Extended range

// Function declarations
static void LockFLIRTargetCallback(void* inRefcon);
static void ActivatePrecisionGuidanceCallback(void* inRefcon);
static void StopGuidanceCallback(void* inRefcon);
static float PrecisionGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void CalculateTargetFromFLIR();
static void ApplyPrecisionGuidance();
static void LogGuidanceStatus();

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Precision Guidance");
    strcpy(outSig, "flir.precision.guidance");
    strcpy(outDesc, "FLIR camera targeting with working precision missile guidance");

    // Initialize weapon datarefs (proven working)
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    
    // Initialize aircraft datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");

    // Register hotkeys for FLIR precision guidance
    XPLMRegisterHotKey(XPLM_VK_F3, xplm_DownFlag, "FPG: Lock FLIR Target", LockFLIRTargetCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F4, xplm_DownFlag, "FPG: Start Precision Guidance", ActivatePrecisionGuidanceCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F5, xplm_DownFlag, "FPG: Stop Guidance", StopGuidanceCallback, NULL);

    XPLMDebugString("FLIR PRECISION GUIDANCE: Plugin loaded\\n");
    XPLMDebugString("FLIR PRECISION GUIDANCE: F9→Crosshair→F3→Fire→F4\\n");
    XPLMDebugString("FLIR PRECISION GUIDANCE: F3=Lock FLIR target, F4=Start guidance, F5=Stop\\n");

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

static void LockFLIRTargetCallback(void* inRefcon)
{
    // Check if FLIR camera is active
    if (!gCameraActive) {
        XPLMDebugString("FLIR PRECISION GUIDANCE: FLIR camera not active! Press F9 first\\n");
        return;
    }
    
    // Calculate target from current FLIR camera direction
    CalculateTargetFromFLIR();
    
    if (gTargetSet) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
                "FLIR PRECISION GUIDANCE: Target locked at (%.0f, %.0f, %.0f)\\n", 
                gTargetX, gTargetY, gTargetZ);
        XPLMDebugString(msg);
        XPLMDebugString("FLIR PRECISION GUIDANCE: Fire weapons, then press F4 to start precision guidance\\n");
    } else {
        XPLMDebugString("FLIR PRECISION GUIDANCE: Target lock failed - check aircraft position\\n");
    }
}

static void ActivatePrecisionGuidanceCallback(void* inRefcon)
{
    if (!gTargetSet) {
        XPLMDebugString("FLIR PRECISION GUIDANCE: No target locked! Use F3 to lock FLIR target first\\n");
        return;
    }
    
    if (!gGuidanceActive) {
        gGuidanceActive = 1;
        
        if (!gGuidanceLoop) {
            XPLMCreateFlightLoop_t flightLoopParams;
            flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
            flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
            flightLoopParams.callbackFunc = PrecisionGuidanceCallback;
            flightLoopParams.refcon = NULL;
            
            gGuidanceLoop = XPLMCreateFlightLoop(&flightLoopParams);
        }
        
        if (gGuidanceLoop) {
            XPLMScheduleFlightLoop(gGuidanceLoop, 0.05f, 1); // 20Hz for smooth control
            
            char msg[256];
            snprintf(msg, sizeof(msg), 
                    "FLIR PRECISION GUIDANCE: STARTED → Target (%.0f, %.0f, %.0f)\\n", 
                    gTargetX, gTargetY, gTargetZ);
            XPLMDebugString(msg);
        }
    } else {
        gGuidanceActive = 0;
        if (gGuidanceLoop) {
            XPLMScheduleFlightLoop(gGuidanceLoop, 0, 0);
            XPLMDebugString("FLIR PRECISION GUIDANCE: STOPPED\\n");
        }
    }
}

static void StopGuidanceCallback(void* inRefcon)
{
    if (!gGuidanceActive) {
        XPLMDebugString("FLIR PRECISION GUIDANCE: Guidance not active\\n");
        return;
    }
    
    gGuidanceActive = 0;
    if (gGuidanceLoop) {
        XPLMScheduleFlightLoop(gGuidanceLoop, 0, 0);
        XPLMDebugString("FLIR PRECISION GUIDANCE: STOPPED\\n");
    }
}

static void CalculateTargetFromFLIR()
{
    // Get aircraft position and heading
    if (!gAircraftX || !gAircraftY || !gAircraftZ || !gAircraftHeading) {
        XPLMDebugString("FLIR PRECISION GUIDANCE: Aircraft position unavailable\\n");
        gTargetSet = 0;
        return;
    }
    
    float planeX = XPLMGetDataf(gAircraftX);
    float planeY = XPLMGetDataf(gAircraftY);
    float planeZ = XPLMGetDataf(gAircraftZ);
    float planeHeading = XPLMGetDataf(gAircraftHeading);
    
    // Use current FLIR camera angles (directly from FLIR_Camera.cpp)
    float panAngle = gCameraPan;   // Degrees
    float tiltAngle = gCameraTilt; // Degrees
    
    // Convert to radians
    float headingRad = (planeHeading + panAngle) * M_PI / 180.0f;
    float tiltRad = tiltAngle * M_PI / 180.0f;
    
    // Calculate target range based on tilt angle (same logic as your DesignateTarget)
    double targetRange = 5000.0; // Default range
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
    gTargetSet = 1;
    
    char debugMsg[256];
    snprintf(debugMsg, sizeof(debugMsg), 
            "FLIR PRECISION GUIDANCE: Target calc - Pan:%.1f° Tilt:%.1f° Range:%.0fm\\n", 
            panAngle, tiltAngle, targetRange);
    XPLMDebugString(debugMsg);
}

static void ApplyPrecisionGuidance()
{
    if (!gTargetSet || !gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) return;
    
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
                "FLIR PRECISION GUIDANCE: [%d] Pos:(%.0f,%.0f,%.0f) Vel:(%.1f,%.1f,%.1f) Speed:%.1f Dist:%.0f\\n",
                i, x[i], y[i], z[i], vx[i], vy[i], vz[i], speed, distance);
            XPLMDebugString(msg);
        }
    }
}

static float PrecisionGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
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