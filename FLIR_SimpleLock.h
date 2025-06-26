/*
 * FLIR_SimpleLock.h
 * 
 * Header file for simple arbitrary point lock-on system
 */

#ifndef FLIR_SIMPLELOCK_H
#define FLIR_SIMPLELOCK_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the simple lock system
void InitializeSimpleLock();

// Lock to current camera direction
void LockCurrentDirection(float currentPan, float currentTilt);

// Get locked camera angles
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