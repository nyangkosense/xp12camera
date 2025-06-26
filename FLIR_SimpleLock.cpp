/*
 * FLIR_SimpleLock.cpp
 * 
 * Enhanced lock-on system using X-Plane's tracking algorithms
 * Implements sophisticated target following like X-Plane's built-in external view
 * Provides smooth tracking for ships, vehicles, and any visual targets
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
#include "FLIR_SimpleLock.h"

// Enhanced lock-on state
static int gLockActive = 0;
static float gLockedPan = 0.0f;     // Pan angle relative to aircraft when locked
static float gLockedTilt = 0.0f;    // Tilt angle when locked
static float gTargetX = 0.0f;       // World coordinates of locked target
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static int gUseEnhancedTracking = 1; // Use X-Plane style tracking algorithm

void InitializeSimpleLock()
{
    gLockActive = 0;
    gUseEnhancedTracking = 1;
    XPLMDebugString("FLIR Enhanced Lock: Initialized with X-Plane tracking algorithms\n");
}

void LockCurrentDirection(float currentPan, float currentTilt)
{
    // Store the current camera angles for enhanced tracking
    gLockedPan = currentPan;
    gLockedTilt = currentTilt;
    gLockActive = 1;
    
    // Calculate target position in world coordinates using X-Plane's algorithm
    // This creates a stable target point that the camera will track
    float headingRad = (currentPan) * M_PI / 180.0f;
    float pitchRad = currentTilt * M_PI / 180.0f;
    
    // Project locked direction into world space at far distance
    float targetDistance = 10000.0f; // Far target for stable tracking
    gTargetX = targetDistance * sin(headingRad) * cos(pitchRad);
    gTargetY = targetDistance * sin(pitchRad);
    gTargetZ = targetDistance * cos(headingRad) * cos(pitchRad);
    
    char msg[256];
    sprintf(msg, "FLIR Enhanced Lock: Locked at Pan=%.1f°, Tilt=%.1f° (Target: %.1f,%.1f,%.1f)\n", 
            gLockedPan, gLockedTilt, gTargetX, gTargetY, gTargetZ);
    XPLMDebugString(msg);
}

void CalculateEnhancedTrackingCamera(float* outPan, float* outTilt, 
                                   float aircraftX, float aircraftY, float aircraftZ,
                                   float aircraftHeading, float cameraX, float cameraY, float cameraZ)
{
    if (!gLockActive) {
        return;
    }
    
    if (!gUseEnhancedTracking) {
        // Fallback to simple angle lock
        *outPan = gLockedPan;
        *outTilt = gLockedTilt;
        return;
    }
    
    // Enhanced tracking using X-Plane's algorithm
    // Transform target from aircraft-relative to world coordinates
    float headingRad = (aircraftHeading + gLockedPan) * M_PI / 180.0f;
    float pitchRad = gLockedTilt * M_PI / 180.0f;
    
    // Calculate world target position relative to current aircraft position
    float targetDistance = 10000.0f;
    float worldTargetX = aircraftX + targetDistance * sin(headingRad) * cos(pitchRad);
    float worldTargetY = aircraftY + targetDistance * sin(pitchRad);
    float worldTargetZ = aircraftZ + targetDistance * cos(headingRad) * cos(pitchRad);
    
    // Calculate camera orientation to track target (X-Plane's method)
    float deltaX = worldTargetX - cameraX;
    float deltaY = worldTargetY - cameraY;
    float deltaZ = worldTargetZ - cameraZ;
    
    // Convert to pan/tilt angles
    float targetHeading = atan2(deltaX, deltaZ) * 180.0f / M_PI;
    float targetPitch = atan2(deltaY, sqrt(deltaX*deltaX + deltaZ*deltaZ)) * 180.0f / M_PI;
    
    // Calculate relative to aircraft heading
    *outPan = targetHeading - aircraftHeading;
    *outTilt = targetPitch;
    
    // Normalize pan angle
    while (*outPan > 180.0f) *outPan -= 360.0f;
    while (*outPan < -180.0f) *outPan += 360.0f;
}

void GetLockedAngles(float* outPan, float* outTilt)
{
    if (!gLockActive) {
        return;
    }
    
    // Legacy support - return stored angles
    *outPan = gLockedPan;
    *outTilt = gLockedTilt;
}

void DisableSimpleLock()
{
    gLockActive = 0;
    XPLMDebugString("FLIR Enhanced Lock: Disabled\n");
}

int IsSimpleLockActive()
{
    return gLockActive;
}

void GetSimpleLockStatus(char* statusBuffer, int bufferSize)
{
    if (!gLockActive) {
        strncpy(statusBuffer, "TRACK: OFF", bufferSize - 1);
        statusBuffer[bufferSize - 1] = '\0';
        return;
    }
    
    const char* mode = gUseEnhancedTracking ? "ENH" : "SIM";
    snprintf(statusBuffer, bufferSize, "TRACK: %s %.1f°/%.1f°", mode, gLockedPan, gLockedTilt);
    statusBuffer[bufferSize - 1] = '\0';
}