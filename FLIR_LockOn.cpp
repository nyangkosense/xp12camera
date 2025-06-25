/*
 * FLIR_LockOn.cpp
 * 
 * Real-world lock-on system for FLIR camera
 * Implements proper world-space target tracking using X-Plane's coordinate system
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMGraphics.h"
#include "FLIR_LockOn.h"

// Lock-on state - using OpenGL local coordinates for precision
static int gLockOnActive = 0;
static double gTargetX = 0.0;          // Target X in OpenGL coordinates (East)
static double gTargetY = 0.0;          // Target Y in OpenGL coordinates (Up)  
static double gTargetZ = 0.0;          // Target Z in OpenGL coordinates (South)
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
    
    // SIMPLE APPROACH: Just store current pan/tilt as the target direction
    // This is what most military systems do - lock the current LOS (Line of Sight)
    
    // Get current aircraft position in OpenGL coordinates
    double planeX = XPLMGetDataf(gPlaneX);
    double planeY = XPLMGetDataf(gPlaneY);
    double planeZ = XPLMGetDataf(gPlaneZ);
    float planeHeading = XPLMGetDataf(gPlaneHeading);
    
    // Calculate camera position (belly-mounted, slightly forward)
    float headingRad = planeHeading * M_PI / 180.0f;
    double cameraX = planeX + 3.0 * sin(headingRad);  // 3m forward (east)
    double cameraY = planeY - 5.0;                    // 5m below
    double cameraZ = planeZ - 3.0 * cos(headingRad);  // 3m forward (north, Z+ is south)
    
    // Calculate absolute camera direction in world space
    float absoluteHeading = planeHeading + currentPan;
    float pitchRad = currentTilt * M_PI / 180.0f;
    
    // Project target point forward from camera
    double horizontalDistance = distance * cos(pitchRad);
    double headingRadAbs = absoluteHeading * M_PI / 180.0f;
    
    // Target calculation - positive angles go clockwise from north
    gTargetX = cameraX + horizontalDistance * sin(headingRadAbs);     // East component  
    gTargetY = cameraY + distance * sin(pitchRad);                    // Up component
    gTargetZ = cameraZ - horizontalDistance * cos(headingRadAbs);     // North component (Z+ is south)
    
    gLockAcquisitionTime = XPLMGetDataf(XPLMFindDataRef("sim/time/total_running_time_sec"));
    gLockOnActive = 1;
    
    char msg[256];
    sprintf(msg, "FLIR Lock-On: Plane hdg=%.1f°, Pan=%.1f°, Abs=%.1f°, Target=(%.1f,%.1f,%.1f)\n", 
            planeHeading, currentPan, absoluteHeading, gTargetX, gTargetY, gTargetZ);
    XPLMDebugString(msg);
}

void UpdateCameraToLockPoint(float* outPan, float* outTilt)
{
    if (!gLockOnActive || !gPlaneX || !gPlaneY || !gPlaneZ || !gPlaneHeading) {
        return;
    }
    
    // Get current aircraft position and heading
    double planeX = XPLMGetDataf(gPlaneX);
    double planeY = XPLMGetDataf(gPlaneY);
    double planeZ = XPLMGetDataf(gPlaneZ);
    float planeHeading = XPLMGetDataf(gPlaneHeading);
    
    // Calculate current camera position (same as in SetArbitraryLockPoint)
    float headingRad = planeHeading * M_PI / 180.0f;
    double cameraX = planeX + 3.0 * sin(headingRad);  // 3m forward (east)
    double cameraY = planeY - 5.0;                    // 5m below  
    double cameraZ = planeZ - 3.0 * cos(headingRad);  // 3m forward (north, Z+ is south)
    
    // Calculate vector from camera to target
    double dx = gTargetX - cameraX;  // East component
    double dy = gTargetY - cameraY;  // Up component
    double dz = gTargetZ - cameraZ;  // South component
    
    // Calculate distance to target
    double horizontalDist = sqrt(dx * dx + dz * dz);
    double totalDist = sqrt(dx * dx + dy * dy + dz * dz);
    
    if (totalDist < 1.0) {
        // Target too close, maintain current angles
        return;
    }
    
    // Convert vector to spherical coordinates
    // X-Plane: Camera faces -Z initially (north), X=East, Z=South
    // For heading: 0° = north = -Z direction, 90° = east = +X direction
    float targetHeading = atan2(dx, -dz) * 180.0f / M_PI;  // atan2(East, North)
    float targetPitch = atan2(dy, horizontalDist) * 180.0f / M_PI;
    
    // Debug coordinate system
    char coordMsg[256];
    sprintf(coordMsg, "FLIR Debug: dx=%.1f(E), dy=%.1f(U), dz=%.1f(S), camera facing %.1f°\n", 
            dx, dy, dz, planeHeading);
    XPLMDebugString(coordMsg);
    
    // Convert absolute heading to relative camera pan
    *outPan = targetHeading - planeHeading;
    *outTilt = targetPitch;
    
    // Normalize pan angle to -180 to +180 degrees
    while (*outPan > 180.0f) *outPan -= 360.0f;
    while (*outPan < -180.0f) *outPan += 360.0f;
    
    // Clamp to camera physical limits  
    if (*outPan > 180.0f) *outPan = 180.0f;
    if (*outPan < -180.0f) *outPan = -180.0f;
    if (*outTilt > 45.0f) *outTilt = 45.0f;
    if (*outTilt < -90.0f) *outTilt = -90.0f;
    
    // Debug output for testing
    char debugMsg[256];
    sprintf(debugMsg, "FLIR Track: Dist=%.1fm, TargetHdg=%.1f°, Pan=%.1f°, Pitch=%.1f°\n", 
            totalDist, targetHeading, *outPan, *outTilt);
    XPLMDebugString(debugMsg);
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
    
    // Calculate current distance to target
    if (gPlaneX && gPlaneY && gPlaneZ) {
        double planeX = XPLMGetDataf(gPlaneX);
        double planeY = XPLMGetDataf(gPlaneY);
        double planeZ = XPLMGetDataf(gPlaneZ);
        
        // Calculate camera position
        float planeHeading = XPLMGetDataf(gPlaneHeading);
        float headingRad = planeHeading * M_PI / 180.0f;
        double cameraX = planeX + 3.0 * sin(headingRad);
        double cameraY = planeY - 5.0;
        double cameraZ = planeZ - 3.0 * cos(headingRad);
        
        // Calculate distance to target
        double dx = gTargetX - cameraX;
        double dy = gTargetY - cameraY;
        double dz = gTargetZ - cameraZ;
        double distance = sqrt(dx * dx + dy * dy + dz * dz);
        
        snprintf(statusBuffer, bufferSize, "LOCK: ON  RNG %.0fm", distance);
    } else {
        strncpy(statusBuffer, "LOCK: ON", bufferSize - 1);
    }
    statusBuffer[bufferSize - 1] = '\0';
}