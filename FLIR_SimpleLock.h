/*
 * FLIR_SimpleLock.h
 * 
 * Simple camera lock system for FLIR targeting
 */

#ifndef FLIR_SIMPLELOCK_H
#define FLIR_SIMPLELOCK_H

#ifdef __cplusplus
extern "C" {
#endif

void InitializeSimpleLock();
void LockCurrentDirection(float currentPan, float currentTilt);
void GetLockedAngles(float* outPan, float* outTilt);
void DisableSimpleLock();
int IsSimpleLockActive();
void GetSimpleLockStatus(char* statusBuffer, int bufferSize);

void SetTargetCoordinates(double lat, double lon, double alt);
void DesignateTarget(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle);
void FireWeaponAtTarget();
int IsTargetDesignated();
void LogWeaponSystemStatus();
void TryAlternativeGPSMethods();

#ifdef __cplusplus
}
#endif

#endif // FLIR_SIMPLELOCK_H