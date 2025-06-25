/*
 * FLIR_LockOn.cpp
 * 
 * Arbitrary point focus system for FLIR camera
 * This module handles camera locking to specific world coordinates
 * without AI target scanning - focuses on arbitrary points in space
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "FLIR_LockOn.h"

// Lock-on state
static int gLockOnActive = 0;
static float gLockedWorldX = 0.0f;
static float gLockedWorldY = 0.0f; 
static float gLockedWorldZ = 0.0f;
static float gLockOnPan = 0.0f;    // Camera pan when locked
static float gLockOnTilt = 0.0f;   // Camera tilt when locked

// Aircraft position datarefs
static XPLMDataRef gPlaneX = NULL;
static XPLMDataRef gPlaneY = NULL;
static XPLMDataRef gPlaneZ = NULL;
static XPLMDataRef gPlaneHeading = NULL;
static XPLMDataRef gPlanePitch = NULL;
static XPLMDataRef gPlaneRoll = NULL;

void InitializeLockOnSystem()
{
    // Get aircraft position datarefs
    gPlaneX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gPlaneY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gPlaneZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gPlaneHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    gPlanePitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    gPlaneRoll = XPLMFindDataRef("sim/flightmodel/position/phi");
    
    XPLMDebugString("FLIR Lock-On System: Initialized\n");
}

void SetArbitraryLockPoint(float currentPan, float currentTilt, float distance)
{
    if (!gPlaneX || !gPlaneY || !gPlaneZ || !gPlaneHeading) {
        XPLMDebugString("FLIR Lock-On: Aircraft datarefs not available\n");
        return;
    }
    
    // Get current aircraft position
    float planeX = XPLMGetDataf(gPlaneX);
    float planeY = XPLMGetDataf(gPlaneY);
    float planeZ = XPLMGetDataf(gPlaneZ);
    float planeHeading = XPLMGetDataf(gPlaneHeading);
    
    // Convert camera angles to radians
    float panRad = (planeHeading + currentPan) * M_PI / 180.0f;
    float tiltRad = currentTilt * M_PI / 180.0f;
    
    // Calculate world coordinates of the lock point
    // Project forward from camera position based on current pan/tilt
    gLockedWorldX = planeX + distance * sin(panRad) * cos(tiltRad);
    gLockedWorldY = planeY + distance * sin(tiltRad);
    gLockedWorldZ = planeZ + distance * cos(panRad) * cos(tiltRad);
    
    // Store the camera angles at lock time
    gLockOnPan = currentPan;
    gLockOnTilt = currentTilt;
    
    gLockOnActive = 1;
    
    char msg[256];
    sprintf(msg, "FLIR Lock-On: Locked to point (%.1f, %.1f, %.1f)\n", 
            gLockedWorldX, gLockedWorldY, gLockedWorldZ);
    XPLMDebugString(msg);
}

void UpdateCameraToLockPoint(float* outPan, float* outTilt)
{
    if (!gLockOnActive || !gPlaneX || !gPlaneY || !gPlaneZ || !gPlaneHeading) {
        return;
    }
    
    // Get current aircraft position
    float planeX = XPLMGetDataf(gPlaneX);
    float planeY = XPLMGetDataf(gPlaneY);
    float planeZ = XPLMGetDataf(gPlaneZ);
    float planeHeading = XPLMGetDataf(gPlaneHeading);
    
    // Calculate vector from aircraft to locked point
    float dx = gLockedWorldX - planeX;
    float dy = gLockedWorldY - planeY;
    float dz = gLockedWorldZ - planeZ;
    
    // Calculate distance to locked point
    float horizontalDist = sqrt(dx * dx + dz * dz);
    float totalDist = sqrt(dx * dx + dy * dy + dz * dz);
    
    if (totalDist < 1.0f) {
        // Too close, maintain last known angles
        *outPan = gLockOnPan;
        *outTilt = gLockOnTilt;
        return;
    }
    
    // Calculate required camera angles
    float targetHeading = atan2(dx, dz) * 180.0f / M_PI;
    float targetTilt = atan2(dy, horizontalDist) * 180.0f / M_PI;
    
    // Convert to camera-relative angles
    *outPan = targetHeading - planeHeading;
    *outTilt = targetTilt;
    
    // Normalize pan angle to -180 to +180
    while (*outPan > 180.0f) *outPan -= 360.0f;
    while (*outPan < -180.0f) *outPan += 360.0f;
    
    // Clamp tilt to reasonable limits
    if (*outTilt > 90.0f) *outTilt = 90.0f;
    if (*outTilt < -90.0f) *outTilt = -90.0f;
}

void DisableLockOn()
{
    gLockOnActive = 0;
    XPLMDebugString("FLIR Lock-On: Disabled\n");
}

int IsLockOnActive()
{
    return gLockOnActive;
}

void GetLockOnStatus(char* statusBuffer, int bufferSize)
{
    if (!gLockOnActive) {
        strncpy(statusBuffer, "LOCK: OFF", bufferSize - 1);
        statusBuffer[bufferSize - 1] = '\0';
        return;
    }
    
    // Calculate distance to locked point
    if (gPlaneX && gPlaneY && gPlaneZ) {
        float planeX = XPLMGetDataf(gPlaneX);
        float planeY = XPLMGetDataf(gPlaneY);
        float planeZ = XPLMGetDataf(gPlaneZ);
        
        float dx = gLockedWorldX - planeX;
        float dy = gLockedWorldY - planeY;
        float dz = gLockedWorldZ - planeZ;
        float distance = sqrt(dx * dx + dy * dy + dz * dz);
        
        snprintf(statusBuffer, bufferSize, "LOCK: ON %.0fm", distance);
    } else {
        strncpy(statusBuffer, "LOCK: ON", bufferSize - 1);
    }
    statusBuffer[bufferSize - 1] = '\0';
}