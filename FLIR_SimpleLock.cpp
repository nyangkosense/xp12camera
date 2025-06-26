/*
 * FLIR_SimpleLock.cpp
 * 
 * Simple camera lock system for FLIR targeting
 * Freezes camera movement when locked on target
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
#include "XPLMProcessing.h"
#include "FLIR_SimpleLock.h"

static int gLockActive = 0;
static float gLockedPan = 0.0f;
static float gLockedTilt = 0.0f;

static int gTargetDesignated = 0;
static double gTargetLat = 0.0;
static double gTargetLon = 0.0;
static double gTargetAlt = 0.0;

static XPLMDataRef gLatDataRef = NULL;
static XPLMDataRef gLonDataRef = NULL;
static XPLMDataRef gAltDataRef = NULL;
static XPLMCommandRef gGPSLockCommand = NULL;
static XPLMCommandRef gFireWeaponCommand = NULL;
static XPLMCommandRef gFireAnyArmedCommand = NULL;
static XPLMDataRef gMasterArmDataRef = NULL;

static XPLMDataRef gWeaponTargLat = NULL;
static XPLMDataRef gWeaponTargLon = NULL;
static XPLMDataRef gWeaponTargH = NULL;
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponType = NULL;

void InitializeSimpleLock()
{
    gLockActive = 0;
    gTargetDesignated = 0;
    
    gLatDataRef = XPLMFindDataRef("sim/flightmodel/position/latitude");
    gLonDataRef = XPLMFindDataRef("sim/flightmodel/position/longitude");
    gAltDataRef = XPLMFindDataRef("sim/flightmodel/position/elevation");
    gGPSLockCommand = XPLMFindCommand("sim/weapons/GPS_lock_here");
    gFireWeaponCommand = XPLMFindCommand("sim/weapons/fire_air_to_ground");
    gFireAnyArmedCommand = XPLMFindCommand("sim/weapons/fire_any_armed");
    gMasterArmDataRef = XPLMFindDataRef("sim/cockpit2/weapons/master_arm");
    
    gWeaponTargLat = XPLMFindDataRef("sim/weapons/targ_lat");
    gWeaponTargLon = XPLMFindDataRef("sim/weapons/targ_lon");
    gWeaponTargH = XPLMFindDataRef("sim/weapons/targ_h");
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponType = XPLMFindDataRef("sim/weapons/type");
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

void SetTargetCoordinates(double lat, double lon, double alt)
{
    gTargetLat = lat;
    gTargetLon = lon;
    gTargetAlt = alt;
    gTargetDesignated = 1;
}

void DesignateTarget(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle)
{
    if (!gLatDataRef || !gLonDataRef || !gAltDataRef) {
        return;
    }
    
    double planeLat = XPLMGetDatad(gLatDataRef);
    double planeLon = XPLMGetDatad(gLonDataRef);
    double planeAlt = XPLMGetDatad(gAltDataRef);
    
    float headingRad = (planeHeading + panAngle) * M_PI / 180.0f;
    float tiltRad = tiltAngle * M_PI / 180.0f;
    
    double targetRange = 5000.0;
    if (tiltAngle < -10.0f) {
        targetRange = fabs(planeAlt / sin(tiltRad));
        if (targetRange > 50000.0) targetRange = 50000.0;
    }
    
    double deltaLat = (targetRange * cos(headingRad) * cos(tiltRad)) / 111320.0;
    double deltaLon = (targetRange * sin(headingRad) * cos(tiltRad)) / (111320.0 * cos(planeLat * M_PI / 180.0));
    double deltaAlt = targetRange * sin(tiltRad);
    
    gTargetLat = planeLat + deltaLat;
    gTargetLon = planeLon + deltaLon;
    gTargetAlt = planeAlt + deltaAlt;
    gTargetDesignated = 1;
    
    if (gWeaponTargLat && gWeaponTargLon && gWeaponTargH && gWeaponCount) {
        int weaponCount = XPLMGetDatai(gWeaponCount);
        
        float targetLatArray[25] = {0};
        float targetLonArray[25] = {0};
        float targetHArray[25] = {0};
        
        for (int i = 0; i < weaponCount && i < 25; i++) {
            targetLatArray[i] = (float)gTargetLat;
            targetLonArray[i] = (float)gTargetLon;
            targetHArray[i] = (float)gTargetAlt;
        }
        
        XPLMSetDatavf(gWeaponTargLat, targetLatArray, 0, weaponCount);
        XPLMSetDatavf(gWeaponTargLon, targetLonArray, 0, weaponCount);
        XPLMSetDatavf(gWeaponTargH, targetHArray, 0, weaponCount);
        
        char debugMsg[256];
        snprintf(debugMsg, sizeof(debugMsg), "FLIR: TV Human target set for %d weapons at %.6f°N %.6f°W, Alt: %.0fm", 
                weaponCount, gTargetLat, gTargetLon, gTargetAlt);
        XPLMDebugString(debugMsg);
    }
    
    if (gGPSLockCommand) {
        XPLMCommandOnce(gGPSLockCommand);
    }
}

void FireWeaponAtTarget()
{
    if (!gTargetDesignated) {
        XPLMDebugString("FLIR: Cannot fire - no target designated\n");
        return;
    }
    
    if (!gMasterArmDataRef) {
        XPLMDebugString("FLIR: Cannot fire - master arm dataref unavailable\n");
        return;
    }
    
    int masterArm = XPLMGetDatai(gMasterArmDataRef);
    if (!masterArm) {
        XPLMDebugString("FLIR: Cannot fire - master arm not enabled\n");
        return;
    }
    
    if (gFireAnyArmedCommand) {
        XPLMCommandOnce(gFireAnyArmedCommand);
        XPLMDebugString("FLIR: Fire any armed command sent\n");
    }
    
    if (gFireWeaponCommand) {
        XPLMCommandOnce(gFireWeaponCommand);
        XPLMDebugString("FLIR: Fire air-to-ground command sent\n");
    }
    
    char fireMsg[256];
    snprintf(fireMsg, sizeof(fireMsg), "FLIR: Weapon firing attempted at target %.6f°N %.6f°W\n", 
            gTargetLat, gTargetLon);
    XPLMDebugString(fireMsg);
}

int IsTargetDesignated()
{
    return gTargetDesignated;
}