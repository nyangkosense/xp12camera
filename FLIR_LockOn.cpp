/*
 * FLIR_LockOn.cpp
 * 
 * Real-world lock-on system for FLIR camera
 * Implements proper target acquisition and tracking like military systems
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
static float gTargetBearing = 0.0f;    // Absolute bearing to target (degrees)
static float gTargetElevation = 0.0f;  // Elevation angle to target (degrees)
static float gTargetRange = 0.0f;      // Range to target (meters)
static float gLockAcquisitionTime = 0.0f; // Time when lock was acquired

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
    
    // Get current aircraft position and heading
    float planeHeading = XPLMGetDataf(gPlaneHeading);
    
    // Calculate absolute target bearing and elevation
    // Real-world systems store bearing/elevation relative to magnetic north
    gTargetBearing = planeHeading + currentPan;
    gTargetElevation = currentTilt;
    gTargetRange = distance;
    gLockAcquisitionTime = XPLMGetDataf(XPLMFindDataRef("sim/time/total_running_time_sec"));
    
    // Normalize bearing to 0-360 degrees
    while (gTargetBearing >= 360.0f) gTargetBearing -= 360.0f;
    while (gTargetBearing < 0.0f) gTargetBearing += 360.0f;
    
    gLockOnActive = 1;
    
    char msg[256];
    sprintf(msg, "FLIR Lock-On: Target acquired - Bearing %.1f°, Elevation %.1f°, Range %.0fm\n", 
            gTargetBearing, gTargetElevation, gTargetRange);
    XPLMDebugString(msg);
}

void UpdateCameraToLockPoint(float* outPan, float* outTilt)
{
    if (!gLockOnActive || !gPlaneHeading) {
        return;
    }
    
    // Get current aircraft heading
    float currentHeading = XPLMGetDataf(gPlaneHeading);
    
    // Calculate required camera pan to maintain lock on target bearing
    // Pan = Target Bearing - Aircraft Heading
    *outPan = gTargetBearing - currentHeading;
    
    // Normalize pan angle to -180 to +180 degrees for natural camera movement
    while (*outPan > 180.0f) *outPan -= 360.0f;
    while (*outPan < -180.0f) *outPan += 360.0f;
    
    // Elevation stays constant (target doesn't move up/down)
    *outTilt = gTargetElevation;
    
    // Clamp to camera physical limits
    if (*outPan > 180.0f) *outPan = 180.0f;
    if (*outPan < -180.0f) *outPan = -180.0f;
    if (*outTilt > 45.0f) *outTilt = 45.0f;
    if (*outTilt < -90.0f) *outTilt = -90.0f;
    
    // Optional: Add slight tracking drift for realism
    float currentTime = XPLMGetDataf(XPLMFindDataRef("sim/time/total_running_time_sec"));
    float lockDuration = currentTime - gLockAcquisitionTime;
    
    // Small drift after 30 seconds (realistic for long-range tracking)
    if (lockDuration > 30.0f) {
        float drift = (lockDuration - 30.0f) * 0.01f; // 0.01 degrees per second
        *outPan += drift * sin(currentTime * 0.1f); // Small oscillation
    }
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
    
    snprintf(statusBuffer, bufferSize, "LOCK: BRG %.0f° EL %.0f° RNG %.0fm", 
             gTargetBearing, gTargetElevation, gTargetRange);
    statusBuffer[bufferSize - 1] = '\0';
}