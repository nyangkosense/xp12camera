/*
 * PrecisionGuidance.cpp
 * 
 * Refined missile guidance with granular, proportional control
 * Based on analysis of working velocity control from test logs
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

// Aircraft position for reference
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;

// Precision guidance control
static int gGuidanceActive = 0;
static XPLMFlightLoopID gGuidanceLoop = NULL;

// Target definition
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static int gTargetSet = 0;

// Refined control parameters
static float gMaxCorrectionSpeed = 20.0f;  // Maximum velocity correction per update (m/s)
static float gProportionalGain = 0.5f;     // How aggressively to steer (0.1 = gentle, 1.0 = aggressive)
static float gDampingFactor = 0.8f;        // Velocity damping to prevent oscillation
static float gMinTargetDistance = 50.0f;   // Stop steering when this close to target
static float gMaxTargetDistance = 5000.0f; // Maximum effective guidance range

// Function declarations
static void StartGuidanceCallback(void* inRefcon);
static void StopGuidanceCallback(void* inRefcon);
static void SetTargetHereCallback(void* inRefcon);
static void SetTargetAheadCallback(void* inRefcon);
static void IncreaseGainCallback(void* inRefcon);
static void DecreaseGainCallback(void* inRefcon);
static float PrecisionGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void ApplyPrecisionGuidance();
static void LogGuidanceStatus();

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Precision Missile Guidance");
    strcpy(outSig, "precision.guidance");
    strcpy(outDesc, "Refined granular missile guidance with proportional control");

    // Initialize datarefs
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_F1, xplm_DownFlag, "PG: Start Guidance", StartGuidanceCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F2, xplm_DownFlag, "PG: Stop Guidance", StopGuidanceCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F3, xplm_DownFlag, "PG: Set Target Here", SetTargetHereCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F4, xplm_DownFlag, "PG: Set Target Ahead", SetTargetAheadCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_EQUAL, xplm_DownFlag, "PG: Increase Gain", IncreaseGainCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_MINUS, xplm_DownFlag, "PG: Decrease Gain", DecreaseGainCallback, NULL);

    XPLMDebugString("PRECISION GUIDANCE: Plugin loaded\n");
    XPLMDebugString("PRECISION GUIDANCE: F1=Start, F2=Stop, F3=Target Here, F4=Target Ahead, +/- = Gain\n");
    XPLMDebugString("PRECISION GUIDANCE: Uses gentle proportional control with velocity damping\n");

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

static void StartGuidanceCallback(void* inRefcon)
{
    if (gGuidanceActive) return;
    
    if (!gTargetSet) {
        XPLMDebugString("PRECISION GUIDANCE: No target set! Use F3 or F4 to set target first\n");
        return;
    }
    
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
        XPLMScheduleFlightLoop(gGuidanceLoop, 0.05f, 1); // High frequency for smooth control
        
        char msg[256];
        snprintf(msg, sizeof(msg), "PRECISION GUIDANCE: Started (Target: %.0f,%.0f,%.0f, Gain: %.2f)\n", 
                gTargetX, gTargetY, gTargetZ, gProportionalGain);
        XPLMDebugString(msg);
    }
}

static void StopGuidanceCallback(void* inRefcon)
{
    if (!gGuidanceActive) return;
    
    gGuidanceActive = 0;
    if (gGuidanceLoop) {
        XPLMScheduleFlightLoop(gGuidanceLoop, 0, 0);
        XPLMDebugString("PRECISION GUIDANCE: Guidance stopped\n");
    }
}

static void SetTargetHereCallback(void* inRefcon)
{
    if (!gAircraftX || !gAircraftY || !gAircraftZ) return;
    
    gTargetX = XPLMGetDataf(gAircraftX);
    gTargetY = XPLMGetDataf(gAircraftY);
    gTargetZ = XPLMGetDataf(gAircraftZ);
    gTargetSet = 1;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "PRECISION GUIDANCE: Target set at aircraft position (%.0f, %.0f, %.0f)\n", 
            gTargetX, gTargetY, gTargetZ);
    XPLMDebugString(msg);
}

static void SetTargetAheadCallback(void* inRefcon)
{
    if (!gAircraftX || !gAircraftY || !gAircraftZ) return;
    
    float aircraftX = XPLMGetDataf(gAircraftX);
    float aircraftY = XPLMGetDataf(gAircraftY);
    float aircraftZ = XPLMGetDataf(gAircraftZ);
    
    // Set target 3000m ahead of aircraft
    gTargetX = aircraftX + 3000.0f;
    gTargetY = aircraftY;
    gTargetZ = aircraftZ;
    gTargetSet = 1;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "PRECISION GUIDANCE: Target set 3000m ahead (%.0f, %.0f, %.0f)\n", 
            gTargetX, gTargetY, gTargetZ);
    XPLMDebugString(msg);
}

static void IncreaseGainCallback(void* inRefcon)
{
    gProportionalGain += 0.1f;
    if (gProportionalGain > 2.0f) gProportionalGain = 2.0f;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "PRECISION GUIDANCE: Proportional gain: %.2f\n", gProportionalGain);
    XPLMDebugString(msg);
}

static void DecreaseGainCallback(void* inRefcon)
{
    gProportionalGain -= 0.1f;
    if (gProportionalGain < 0.1f) gProportionalGain = 0.1f;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "PRECISION GUIDANCE: Proportional gain: %.2f\n", gProportionalGain);
    XPLMDebugString(msg);
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
            // Inactive missile - copy current velocity
            newVX[i] = vx[i];
            newVY[i] = vy[i];
            newVZ[i] = vz[i];
            continue;
        }
        
        // Calculate vector to target
        float dx = gTargetX - x[i];
        float dy = gTargetY - y[i];
        float dz = gTargetZ - z[i];
        
        float distance = sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance < gMinTargetDistance) {
            // Very close to target - maintain current velocity (no more steering)
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
        
        // Calculate desired velocity magnitude based on distance
        // Closer = slower, farther = faster (but capped)
        float desiredSpeed = fmin(distance * 0.1f, 100.0f); // Max 100 m/s
        if (desiredSpeed < 10.0f) desiredSpeed = 10.0f;     // Min 10 m/s
        
        // Calculate desired velocity vector
        float desiredVX = dirX * desiredSpeed;
        float desiredVY = dirY * desiredSpeed;
        float desiredVZ = dirZ * desiredSpeed;
        
        // Calculate velocity correction (difference between desired and current)
        float correctionX = desiredVX - vx[i];
        float correctionY = desiredVY - vy[i];
        float correctionZ = desiredVZ - vz[i];
        
        // Apply proportional gain
        correctionX *= gProportionalGain;
        correctionY *= gProportionalGain;
        correctionZ *= gProportionalGain;
        
        // Limit correction magnitude to prevent sudden changes
        float correctionMag = sqrt(correctionX*correctionX + correctionY*correctionY + correctionZ*correctionZ);
        if (correctionMag > gMaxCorrectionSpeed) {
            float scale = gMaxCorrectionSpeed / correctionMag;
            correctionX *= scale;
            correctionY *= scale;
            correctionZ *= scale;
        }
        
        // Apply correction to current velocity (incremental change)
        newVX[i] = vx[i] + correctionX;
        newVY[i] = vy[i] + correctionY;
        newVZ[i] = vz[i] + correctionZ;
        
        // Apply damping to reduce oscillation
        newVX[i] *= gDampingFactor;
        newVY[i] *= gDampingFactor;
        newVZ[i] *= gDampingFactor;
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
    
    // Log status for weapons 0 and 1
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        if (x[i] != 0 || y[i] != 0 || z[i] != 0) { // Only active missiles
            float dx = gTargetX - x[i];
            float dy = gTargetY - y[i];
            float dz = gTargetZ - z[i];
            float distance = sqrt(dx*dx + dy*dy + dz*dz);
            float speed = sqrt(vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i]);
            
            char msg[512];
            snprintf(msg, sizeof(msg), 
                "PRECISION GUIDANCE: [%d] Pos:(%.0f,%.0f,%.0f) Vel:(%.1f,%.1f,%.1f) Speed:%.1f Dist:%.0f\n",
                i, x[i], y[i], z[i], vx[i], vy[i], vz[i], speed, distance);
            XPLMDebugString(msg);
        }
    }
}

static float PrecisionGuidanceCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gGuidanceActive) return 0;
    
    ApplyPrecisionGuidance();
    
    // Log status every 3 seconds
    static int logCounter = 0;
    logCounter++;
    if (logCounter % 60 == 0) { // 60 * 0.05s = 3 seconds
        LogGuidanceStatus();
    }
    
    return 0.05f; // High frequency for smooth control
}