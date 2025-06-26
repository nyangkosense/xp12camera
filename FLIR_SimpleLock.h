/*
 * FLIR_SimpleLock.h
 * 
 * Enhanced lock-on system using X-Plane's tracking algorithms
 * Provides smooth target following like X-Plane's built-in external view
 */

#ifndef FLIR_SIMPLELOCK_H
#define FLIR_SIMPLELOCK_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the enhanced lock system
void InitializeSimpleLock();

// Lock to current camera direction (enhanced algorithm)
void LockCurrentDirection(float currentPan, float currentTilt);


// Get locked camera angles (legacy support)
void GetLockedAngles(float* outPan, float* outTilt);

// Disable lock
void DisableSimpleLock();

// Check if lock is active
int IsSimpleLockActive();

// Get lock status string for display
void GetSimpleLockStatus(char* statusBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif

#endif // FLIR_SIMPLELOCK_H