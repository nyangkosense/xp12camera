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
void DesignateTargetFallback(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle);
void CalculateRayGroundIntersection(float camX, float camY, float camZ, float heading, float pitch, float* outX, float* outY, float* outZ);
void FireWeaponAtTarget();
int IsTargetDesignated();
void LogWeaponSystemStatus();
void TryAlternativeGPSMethods();
void StartActiveGuidance();
void StopActiveGuidance();
float GuidanceFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
void GetCrosshairWorldPosition(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle, float* outX, float* outY, float* outZ);
void LogCrosshairPosition(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle);

#ifdef __cplusplus
}
#endif

#endif // FLIR_SIMPLELOCK_H