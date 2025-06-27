/*
 * FocusedGuidance.cpp
 * 
 * Focused test using ONLY proven working datarefs:
 * - sim/weapons/vx, vy, vz (velocity control)
 * - sim/weapons/q1, q2, q3, q4 (quaternion orientation)
 * - sim/weapons/x, y, z (position monitoring)
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

// Proven working weapon datarefs
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;
static XPLMDataRef gWeaponQ1 = NULL;
static XPLMDataRef gWeaponQ2 = NULL;
static XPLMDataRef gWeaponQ3 = NULL;
static XPLMDataRef gWeaponQ4 = NULL;

// Control variables
static int gGuidanceActive = 0;
static int gTestMode = 0; // 0=velocity_steer, 1=quaternion_steer, 2=hybrid
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static int gTargetSet = 0;
static XPLMFlightLoopID gGuidanceLoop = NULL;

// Guidance parameters
static float gSteeringStrength = 100.0f; // Velocity steering strength
static float gMaxVelocity = 500.0f; // Maximum velocity component

// Function declarations
static void StartGuidanceCallback(void* inRefcon);
static void SetTargetCallback(void* inRefcon);
static void NextModeCallback(void* inRefcon);
static void IncreaseStrengthCallback(void* inRefcon);
static void DecreaseStrengthCallback(void* inRefcon);
static float GuidanceFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void ApplyVelocitySteering();
static void ApplyQuaternionSteering();
static void ApplyHybridSteering();
static void LogMissileStatus();

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Focused Guidance Test");
    strcpy(outSig, "focusedguidance.test");
    strcpy(outDesc, "Test plugin using proven working weapon datarefs");

    // Initialize proven working datarefs
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    gWeaponQ1 = XPLMFindDataRef("sim/weapons/q1");
    gWeaponQ2 = XPLMFindDataRef("sim/weapons/q2");
    gWeaponQ3 = XPLMFindDataRef("sim/weapons/q3");
    gWeaponQ4 = XPLMFindDataRef("sim/weapons/q4");

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_F5, xplm_DownFlag, "FG: Start/Stop Guidance", StartGuidanceCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F6, xplm_DownFlag, "FG: Set Target Here", SetTargetCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F7, xplm_DownFlag, "FG: Next Mode", NextModeCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_EQUAL, xplm_DownFlag, "FG: Increase Strength", IncreaseStrengthCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_MINUS, xplm_DownFlag, "FG: Decrease Strength", DecreaseStrengthCallback, NULL);

    XPLMDebugString("FOCUSED GUIDANCE: Plugin loaded\n");
    XPLMDebugString("FOCUSED GUIDANCE: F5=Start/Stop, F6=Set Target, F7=Mode, +/- = Strength\n");
    XPLMDebugString("FOCUSED GUIDANCE: Modes: 0=Velocity, 1=Quaternion, 2=Hybrid\n");

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
    if (!gGuidanceActive) {
        gGuidanceActive = 1;
        
        if (!gGuidanceLoop) {
            XPLMCreateFlightLoop_t flightLoopParams;
            flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
            flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
            flightLoopParams.callbackFunc = GuidanceFlightLoopCallback;
            flightLoopParams.refcon = NULL;
            
            gGuidanceLoop = XPLMCreateFlightLoop(&flightLoopParams);
        }
        
        if (gGuidanceLoop) {
            XPLMScheduleFlightLoop(gGuidanceLoop, 0.1f, 1);
            
            const char* modeNames[] = {"VELOCITY", "QUATERNION", "HYBRID"};
            char msg[256];
            snprintf(msg, sizeof(msg), "FOCUSED GUIDANCE: Started %s steering (strength=%.0f)\n", 
                    modeNames[gTestMode], gSteeringStrength);
            XPLMDebugString(msg);
        }
    } else {
        gGuidanceActive = 0;
        if (gGuidanceLoop) {
            XPLMScheduleFlightLoop(gGuidanceLoop, 0, 0);
            XPLMDebugString("FOCUSED GUIDANCE: Guidance stopped\n");
        }
    }
}

static void SetTargetCallback(void* inRefcon)
{
    // Use first active missile position as reference, offset forward
    if (!gWeaponX || !gWeaponY || !gWeaponZ) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    float x[25] = {0}, y[25] = {0}, z[25] = {0};
    XPLMGetDatavf(gWeaponX, x, 0, weaponCount);
    XPLMGetDatavf(gWeaponY, y, 0, weaponCount);
    XPLMGetDatavf(gWeaponZ, z, 0, weaponCount);
    
    // Find first active missile
    for (int i = 0; i < weaponCount && i < 2; i++) { // Check slots 0 and 1
        if (x[i] != 0 || y[i] != 0 || z[i] != 0) {
            // Set target 2000m ahead of missile
            gTargetX = x[i] + 2000.0f;
            gTargetY = y[i];
            gTargetZ = z[i] + 1000.0f;
            gTargetSet = 1;
            
            char msg[256];
            snprintf(msg, sizeof(msg), "FOCUSED GUIDANCE: Target set at (%.0f, %.0f, %.0f)\n", 
                    gTargetX, gTargetY, gTargetZ);
            XPLMDebugString(msg);
            return;
        }
    }
    
    XPLMDebugString("FOCUSED GUIDANCE: No active missile found for target setting\n");
}

static void NextModeCallback(void* inRefcon)
{
    gTestMode = (gTestMode + 1) % 3;
    
    const char* modeNames[] = {"VELOCITY", "QUATERNION", "HYBRID"};
    char msg[256];
    snprintf(msg, sizeof(msg), "FOCUSED GUIDANCE: Switched to %s mode\n", modeNames[gTestMode]);
    XPLMDebugString(msg);
}

static void IncreaseStrengthCallback(void* inRefcon)
{
    gSteeringStrength += 50.0f;
    if (gSteeringStrength > 1000.0f) gSteeringStrength = 1000.0f;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "FOCUSED GUIDANCE: Steering strength: %.0f\n", gSteeringStrength);
    XPLMDebugString(msg);
}

static void DecreaseStrengthCallback(void* inRefcon)
{
    gSteeringStrength -= 50.0f;
    if (gSteeringStrength < 10.0f) gSteeringStrength = 10.0f;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "FOCUSED GUIDANCE: Steering strength: %.0f\n", gSteeringStrength);
    XPLMDebugString(msg);
}

static void ApplyVelocitySteering()
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
    
    // Apply steering to weapons 0 and 1 only
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        if (x[i] == 0 && y[i] == 0 && z[i] == 0) continue; // Skip inactive
        
        // Calculate vector to target
        float dx = gTargetX - x[i];
        float dy = gTargetY - y[i];
        float dz = gTargetZ - z[i];
        
        float distance = sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance > 50.0f) { // Don't steer if very close
            // Normalize direction
            dx /= distance;
            dy /= distance;
            dz /= distance;
            
            // Apply proportional steering (keep current velocity + steering correction)
            newVX[i] = vx[i] + dx * gSteeringStrength;
            newVY[i] = vy[i] + dy * gSteeringStrength;
            newVZ[i] = vz[i] + dz * gSteeringStrength;
            
            // Limit maximum velocity
            float velMag = sqrt(newVX[i]*newVX[i] + newVY[i]*newVY[i] + newVZ[i]*newVZ[i]);
            if (velMag > gMaxVelocity) {
                float scale = gMaxVelocity / velMag;
                newVX[i] *= scale;
                newVY[i] *= scale;
                newVZ[i] *= scale;
            }
        } else {
            // Keep current velocity if close to target
            newVX[i] = vx[i];
            newVY[i] = vy[i];
            newVZ[i] = vz[i];
        }
    }
    
    // Apply new velocities
    XPLMSetDatavf(gWeaponVX, newVX, 0, weaponCount);
    XPLMSetDatavf(gWeaponVY, newVY, 0, weaponCount);
    XPLMSetDatavf(gWeaponVZ, newVZ, 0, weaponCount);
}

static void ApplyQuaternionSteering()
{
    if (!gTargetSet || !gWeaponQ1 || !gWeaponQ2 || !gWeaponQ3 || !gWeaponQ4) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    // Simple quaternion test - rotate missiles slowly
    static float angle = 0.0f;
    angle += 0.1f; // Slow rotation
    
    float q1[25] = {0}, q2[25] = {0}, q3[25] = {0}, q4[25] = {0};
    
    // Apply rotation to weapons 0 and 1
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        // Simple rotation quaternion (rotate around Y axis)
        q1[i] = 0.0f;
        q2[i] = sin(angle / 2.0f);
        q3[i] = 0.0f;
        q4[i] = cos(angle / 2.0f);
    }
    
    XPLMSetDatavf(gWeaponQ1, q1, 0, weaponCount);
    XPLMSetDatavf(gWeaponQ2, q2, 0, weaponCount);
    XPLMSetDatavf(gWeaponQ3, q3, 0, weaponCount);
    XPLMSetDatavf(gWeaponQ4, q4, 0, weaponCount);
}

static void ApplyHybridSteering()
{
    // Combine velocity and quaternion steering
    ApplyVelocitySteering();
    ApplyQuaternionSteering();
}

static void LogMissileStatus()
{
    if (!gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    float x[25] = {0}, y[25] = {0}, z[25] = {0};
    float vx[25] = {0}, vy[25] = {0}, vz[25] = {0};
    float q1[25] = {0}, q2[25] = {0}, q3[25] = {0}, q4[25] = {0};
    
    XPLMGetDatavf(gWeaponX, x, 0, weaponCount);
    XPLMGetDatavf(gWeaponY, y, 0, weaponCount);
    XPLMGetDatavf(gWeaponZ, z, 0, weaponCount);
    XPLMGetDatavf(gWeaponVX, vx, 0, weaponCount);
    XPLMGetDatavf(gWeaponVY, vy, 0, weaponCount);
    XPLMGetDatavf(gWeaponVZ, vz, 0, weaponCount);
    
    if (gWeaponQ1) XPLMGetDatavf(gWeaponQ1, q1, 0, weaponCount);
    if (gWeaponQ2) XPLMGetDatavf(gWeaponQ2, q2, 0, weaponCount);
    if (gWeaponQ3) XPLMGetDatavf(gWeaponQ3, q3, 0, weaponCount);
    if (gWeaponQ4) XPLMGetDatavf(gWeaponQ4, q4, 0, weaponCount);
    
    // Log only weapons 0 and 1
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        if (x[i] != 0 || y[i] != 0 || z[i] != 0) { // Only active missiles
            char msg[512];
            float distToTarget = 0.0f;
            if (gTargetSet) {
                float dx = gTargetX - x[i];
                float dy = gTargetY - y[i];
                float dz = gTargetZ - z[i];
                distToTarget = sqrt(dx*dx + dy*dy + dz*dz);
            }
            
            snprintf(msg, sizeof(msg), 
                "FOCUSED GUIDANCE: [%d] Pos:(%.0f,%.0f,%.0f) Vel:(%.1f,%.1f,%.1f) Q:(%.3f,%.3f,%.3f,%.3f) Dist:%.0f\n",
                i, x[i], y[i], z[i], vx[i], vy[i], vz[i], 
                q1[i], q2[i], q3[i], q4[i], distToTarget);
            XPLMDebugString(msg);
        }
    }
}

static float GuidanceFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gGuidanceActive) return 0;
    
    switch (gTestMode) {
        case 0: // Velocity steering
            ApplyVelocitySteering();
            break;
        case 1: // Quaternion steering
            ApplyQuaternionSteering();
            break;
        case 2: // Hybrid steering
            ApplyHybridSteering();
            break;
    }
    
    // Log status every 2 seconds
    static int logCounter = 0;
    logCounter++;
    if (logCounter % 20 == 0) {
        LogMissileStatus();
    }
    
    return 0.1f; // Continue every 0.1 seconds
}