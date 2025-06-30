/*
 * Simple camera lock system that freezes FLIR camera movement when locked on target
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

static int gLockActive = 0;
static float gLockedPan = 0.0f;
static float gLockedTilt = 0.0f;

void InitializeSimpleLock()
{
    gLockActive = 0;
}

void LockCurrentDirection(float currentPan, float currentTilt)
{
    gLockedPan = currentPan;
    gLockedTilt = currentTilt;
    gLockActive = 1;
}

void GetLockedAngles(float* outPan, float* outTilt)
{
    if (!gLockActive) {
        return;
    }
    
    *outPan = gLockedPan;
    *outTilt = gLockedTilt;
}

void DisableSimpleLock()
{
    gLockActive = 0;
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