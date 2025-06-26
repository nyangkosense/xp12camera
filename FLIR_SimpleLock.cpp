/*
 * FLIR_SimpleLock.cpp
 * 
 * Simple arbitrary point lock-on system
 * Just locks the current camera direction relative to aircraft heading
 * Perfect for tracking moving ships, vehicles, or any visual target
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "FLIR_SimpleLock.h"

// Lock-on state
static int gLockActive = 0;
static float gLockedPan = 0.0f;     // Pan angle relative to aircraft when locked
static float gLockedTilt = 0.0f;    // Tilt angle when locked

void InitializeSimpleLock()
{
    gLockActive = 0;
    XPLMDebugString("FLIR Simple Lock: Initialized\n");
}

void LockCurrentDirection(float currentPan, float currentTilt)
{
    // Simply store the current camera angles
    gLockedPan = currentPan;
    gLockedTilt = currentTilt;
    gLockActive = 1;
    
    char msg[256];
    sprintf(msg, "FLIR Simple Lock: Locked at Pan=%.1f°, Tilt=%.1f°\n", 
            gLockedPan, gLockedTilt);
    XPLMDebugString(msg);
}

void GetLockedAngles(float* outPan, float* outTilt)
{
    if (!gLockActive) {
        return;
    }
    
    // Simply return the locked angles - camera stays fixed relative to aircraft
    *outPan = gLockedPan;
    *outTilt = gLockedTilt;
}

void DisableSimpleLock()
{
    gLockActive = 0;
    XPLMDebugString("FLIR Simple Lock: Disabled\n");
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