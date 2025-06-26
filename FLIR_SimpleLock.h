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

#ifdef __cplusplus
}
#endif

#endif // FLIR_SIMPLELOCK_H