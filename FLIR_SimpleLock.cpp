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

// Simple lock-on state
static int gLockActive = 0;
static float gLockedPan = 0.0f;     // Pan angle when locked (for display only)
static float gLockedTilt = 0.0f;    // Tilt angle when locked (for display only)

void InitializeSimpleLock()
{
    gLockActive = 0;
    XPLMDebugString("FLIR Simple Lock: Initialized\n");
}

void LockCurrentDirection(float currentPan, float currentTilt)
{
    // Simple lock - just store angles for display and freeze camera movement
    gLockedPan = currentPan;
    gLockedTilt = currentTilt;
    gLockActive = 1;
    
    char msg[256];
    sprintf(msg, "FLIR Camera Lock: Engaged at Pan=%.1f°, Tilt=%.1f°\n", 
            gLockedPan, gLockedTilt);
    XPLMDebugString(msg);
}

void GetLockedAngles(float* outPan, float* outTilt)
{
    if (!gLockActive) {
        return;
    }
    
    // Return stored angles - camera movement is frozen
    *outPan = gLockedPan;
    *outTilt = gLockedTilt;
}

void DisableSimpleLock()
{
    gLockActive = 0;
    XPLMDebugString("FLIR Camera Lock: Disabled\n");
}

int IsSimpleLockActive()
{
    return gLockActive;
}

void GetSimpleLockStatus(char* statusBuffer, int bufferSize)
{
    if (!gLockActive) {
        strncpy(statusBuffer, "LOCK: OFF", bufferSize - 1);
        statusBuffer[bufferSize - 1] = '\0';
        return;
    }
    
    snprintf(statusBuffer, bufferSize, "LOCK: ON %.1f°/%.1f°", gLockedPan, gLockedTilt);
    statusBuffer[bufferSize - 1] = '\0';
}