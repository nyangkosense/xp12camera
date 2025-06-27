/*
 * FLIRGuidance.cpp
 * 
 * Integration of FLIR camera targeting with precision missile guidance
 * Workflow: F9 → Crosshair → Spacebar Lock → Auto-retrieve data → Fire → F2 to guide
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

// FLIR camera datarefs (assuming these exist in your system)
static XPLMDataRef gCameraPan = NULL;
static XPLMDataRef gCameraTilt = NULL;

// Target and guidance state
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static int gTargetLocked = 0;
static int gGuidanceActive = 0;
static XPLMFlightLoopID gGuidanceLoop = NULL;

// FLIR lock state (integrate with your existing system)
static int gFLIRLockActive = 0;
static float gLockedPan = 0.0f;
static float gLockedTilt = 0.0f;

// Precision guidance parameters
static float gMaxCorrectionSpeed = 20.0f;
static float gProportionalGain = 1.2f;
static float gDampingFactor = 0.8f;
static float gMinTargetDistance = 50.0f;
static float gMaxTargetDistance = 5000.0f;

// Function declarations
static void ActivateGuidanceCallback(void* inRefcon);
static void LockTargetCallback(void* inRefcon);
static float FLIRGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void CalculateTargetFromFLIR();
static void ApplyPrecisionGuidance();
static void LogGuidanceStatus();

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Integrated Guidance");
    strcpy(outSig, "flir.integrated.guidance");
    strcpy(outDesc, "FLIR camera targeting integrated with precision missile guidance");

    // Initialize weapon datarefs
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

    // Try to find FLIR camera datarefs (these might be custom from your FLIR system)
    gCameraPan = XPLMFindDataRef("flir/camera/pan_angle");   // Adjust these names
    gCameraTilt = XPLMFindDataRef("flir/camera/tilt_angle"); // to match your system

    // Register hotkeys for integration workflow
    XPLMRegisterHotKey(XPLM_VK_SPACE, xplm_DownFlag, "FLIR: Lock Target", LockTargetCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F2, xplm_DownFlag, "FLIR: Start Guidance", ActivateGuidanceCallback, NULL);

    XPLMDebugString("FLIR GUIDANCE: Plugin loaded\n");
    XPLMDebugString("FLIR GUIDANCE: Workflow: F9→Crosshair→SPACE→Fire→F2\n");
    XPLMDebugString("FLIR GUIDANCE: SPACE = Lock FLIR target, F2 = Start missile guidance\n");

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

static void LockTargetCallback(void* inRefcon)
{
    // This replaces/enhances your existing FLIR lock system
    if (!gFLIRLockActive) {
        // Get current camera angles (you'll need to adapt this to your FLIR system)
        if (gCameraPan && gCameraTilt) {
            gLockedPan = XPLMGetDataf(gCameraPan);
            gLockedTilt = XPLMGetDataf(gCameraTilt);
        } else {
            // Fallback - use default angles if FLIR datarefs not found
            gLockedPan = 0.0f;   // Adjust these to your system
            gLockedTilt = -10.0f;
        }
        
        gFLIRLockActive = 1;
        
        // Calculate target coordinates from current FLIR pointing direction
        CalculateTargetFromFLIR();
        
        if (gTargetLocked) {
            char msg[256];
            snprintf(msg, sizeof(msg), "FLIR GUIDANCE: Target locked at (%.0f, %.0f, %.0f)\n", 
                    gTargetX, gTargetY, gTargetZ);
            XPLMDebugString(msg);
            XPLMDebugString("FLIR GUIDANCE: Fire weapon, then press F2 to start guidance\n");
        } else {
            XPLMDebugString("FLIR GUIDANCE: Target lock failed - check aircraft position datarefs\n");
        }
    } else {
        // Unlock target
        gFLIRLockActive = 0;
        gTargetLocked = 0;
        XPLMDebugString("FLIR GUIDANCE: Target unlocked\n");
    }
}

static void ActivateGuidanceCallback(void* inRefcon)
{
    if (!gTargetLocked) {
        XPLMDebugString("FLIR GUIDANCE: No target locked! Use SPACE to lock FLIR target first\n");
        return;
    }
    
    if (!gGuidanceActive) {
        gGuidanceActive = 1;
        
        if (!gGuidanceLoop) {
            XPLMCreateFlightLoop_t flightLoopParams;
            flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
            flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
            flightLoopParams.callbackFunc = FLIRGuidanceCallback;
            flightLoopParams.refcon = NULL;
            
            gGuidanceLoop = XPLMCreateFlightLoop(&flightLoopParams);
        }
        
        if (gGuidanceLoop) {
            XPLMScheduleFlightLoop(gGuidanceLoop, 0.05f, 1);
            
            char msg[256];
            snprintf(msg, sizeof(msg), "FLIR GUIDANCE: Missile guidance STARTED to target (%.0f, %.0f, %.0f)\n", 
                    gTargetX, gTargetY, gTargetZ);
            XPLMDebugString(msg);
        }
    } else {
        gGuidanceActive = 0;
        if (gGuidanceLoop) {
            XPLMScheduleFlightLoop(gGuidanceLoop, 0, 0);
            XPLMDebugString("FLIR GUIDANCE: Missile guidance STOPPED\n");
        }
    }
}

static void CalculateTargetFromFLIR()
{
    // This is based on your existing DesignateTarget() function logic
    if (!gAircraftX || !gAircraftY || !gAircraftZ || !gAircraftHeading) {
        XPLMDebugString("FLIR GUIDANCE: Aircraft position datarefs not available\n");
        return;
    }
    
    float planeX = XPLMGetDataf(gAircraftX);
    float planeY = XPLMGetDataf(gAircraftY);
    float planeZ = XPLMGetDataf(gAircraftZ);
    float planeHeading = XPLMGetDataf(gAircraftHeading); // In degrees
    
    // Use locked camera angles (from your FLIR system)
    float panAngle = gLockedPan;   // Degrees
    float tiltAngle = gLockedTilt; // Degrees
    
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
    gTargetLocked = 1;
    
    char debugMsg[256];
    snprintf(debugMsg, sizeof(debugMsg), 
            "FLIR GUIDANCE: Target calculated - Pan:%.1f° Tilt:%.1f° Range:%.0fm → (%.0f,%.0f,%.0f)\n", 
            panAngle, tiltAngle, targetRange, gTargetX, gTargetY, gTargetZ);
    XPLMDebugString(debugMsg);
}

static void ApplyPrecisionGuidance()
{
    // Same precision guidance logic as before, but using FLIR-calculated target
    if (!gTargetLocked || !gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) return;
    
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
            // Close to target - gentle approach
            newVX[i] = vx[i] * gDampingFactor;
            newVY[i] = vy[i] * gDampingFactor;
            newVZ[i] = vz[i] * gDampingFactor;
            continue;
        }
        
        if (distance > gMaxTargetDistance) {
            // Too far - maintain current velocity
            newVX[i] = vx[i];
            newVY[i] = vy[i];
            newVZ[i] = vz[i];
            continue;
        }
        
        // Normalize direction vector
        float dirX = dx / distance;
        float dirY = dy / distance;
        float dirZ = dz / distance;
        
        // Calculate desired speed based on distance
        float desiredSpeed = fmin(distance * 0.1f, 100.0f);
        if (desiredSpeed < 10.0f) desiredSpeed = 10.0f;
        
        // Calculate desired velocity vector
        float desiredVX = dirX * desiredSpeed;
        float desiredVY = dirY * desiredSpeed;
        float desiredVZ = dirZ * desiredSpeed;
        
        // Calculate velocity correction
        float correctionX = (desiredVX - vx[i]) * gProportionalGain;
        float correctionY = (desiredVY - vy[i]) * gProportionalGain;
        float correctionZ = (desiredVZ - vz[i]) * gProportionalGain;
        
        // Limit correction magnitude
        float correctionMag = sqrt(correctionX*correctionX + correctionY*correctionY + correctionZ*correctionZ);
        if (correctionMag > gMaxCorrectionSpeed) {
            float scale = gMaxCorrectionSpeed / correctionMag;
            correctionX *= scale;
            correctionY *= scale;
            correctionZ *= scale;
        }
        
        // Apply correction with damping
        newVX[i] = (vx[i] + correctionX) * gDampingFactor;
        newVY[i] = (vy[i] + correctionY) * gDampingFactor;
        newVZ[i] = (vz[i] + correctionZ) * gDampingFactor;
    }
    
    // Apply new velocities
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
    
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        if (x[i] != 0 || y[i] != 0 || z[i] != 0) {
            float dx = gTargetX - x[i];
            float dy = gTargetY - y[i];
            float dz = gTargetZ - z[i];
            float distance = sqrt(dx*dx + dy*dy + dz*dz);
            float speed = sqrt(vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i]);
            
            char msg[512];
            snprintf(msg, sizeof(msg), 
                "FLIR GUIDANCE: [%d] Pos:(%.0f,%.0f,%.0f) Vel:(%.1f,%.1f,%.1f) Speed:%.1f Dist:%.0f\n",
                i, x[i], y[i], z[i], vx[i], vy[i], vz[i], speed, distance);
            XPLMDebugString(msg);
        }
    }
}

static float FLIRGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gGuidanceActive) return 0;
    
    ApplyPrecisionGuidance();
    
    // Log status every 3 seconds
    static int logCounter = 0;
    logCounter++;
    if (logCounter % 60 == 0) {
        LogGuidanceStatus();
    }
    
    return 0.05f; // High frequency for smooth control
}