/*
 * FLIR_LockOn.h
 * 
 * Header file for FLIR camera arbitrary point lock-on system
 */

#ifndef FLIR_LOCKON_H
#define FLIR_LOCKON_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the lock-on system
void InitializeLockOnSystem();

// Set lock-on point based on current camera direction and distance
void SetArbitraryLockPoint(float currentPan, float currentTilt, float distance);

// Update camera angles to maintain lock on point
void UpdateCameraToLockPoint(float* outPan, float* outTilt);

// Disable lock-on
void DisableLockOn();

// Check if lock-on is active
int IsLockOnActive();

// Get lock-on status string for display
void GetLockOnStatus(char* statusBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif

#endif // FLIR_LOCKON_H