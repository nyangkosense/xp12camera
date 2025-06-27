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
#include "XPLMGraphics.h"
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
static XPLMDataRef gGPSDestLat = NULL;
static XPLMDataRef gGPSDestLon = NULL;
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponType = NULL;
static XPLMDataRef gWeaponsArmed = NULL;
static XPLMDataRef gMissilesArmed = NULL;
static XPLMDataRef gBombsArmed = NULL;
static XPLMDataRef gGunsArmed = NULL;

static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;
static XPLMDataRef gWeaponThecon = NULL;
static XPLMDataRef gWeaponSFrn = NULL;
static XPLMDataRef gWeaponSSid = NULL;
static XPLMDataRef gWeaponSTop = NULL;
static XPLMDataRef gWeaponTargetIndex = NULL;

static int gActiveGuidance = 0;
static XPLMFlightLoopID gGuidanceFlightLoop = NULL;

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
    gGPSDestLat = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_latitude_deg");
    gGPSDestLon = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_longitude_deg");
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponType = XPLMFindDataRef("sim/weapons/type");
    gWeaponsArmed = XPLMFindDataRef("sim/cockpit/weapons/rockets_armed");
    gMissilesArmed = XPLMFindDataRef("sim/cockpit/weapons/missiles_armed");
    gBombsArmed = XPLMFindDataRef("sim/cockpit/weapons/bombs_armed");
    gGunsArmed = XPLMFindDataRef("sim/cockpit/weapons/guns_armed");
    
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    gWeaponThecon = XPLMFindDataRef("sim/weapons/the_con");
    gWeaponSFrn = XPLMFindDataRef("sim/weapons/s_frn");
    gWeaponSSid = XPLMFindDataRef("sim/weapons/s_sid");
    gWeaponSTop = XPLMFindDataRef("sim/weapons/s_top");
    gWeaponTargetIndex = XPLMFindDataRef("sim/weapons/target_index");
    
    LogWeaponSystemStatus();
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
    
    LogWeaponSystemStatus();
    
    if (gMissilesArmed && gBombsArmed && gWeaponsArmed) {
        XPLMSetDatai(gMissilesArmed, 1);
        XPLMSetDatai(gBombsArmed, 1);
        XPLMSetDatai(gWeaponsArmed, 1);
        XPLMDebugString("FLIR: Auto-armed missiles, bombs, and rockets\n");
    }
    
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
        snprintf(debugMsg, sizeof(debugMsg), "FLIR: Weapon target arrays set for %d weapons at %.6f°N %.6f°W", 
                weaponCount, gTargetLat, gTargetLon);
        XPLMDebugString(debugMsg);
    }
    
    if (gGPSLockCommand) {
        XPLMCommandOnce(gGPSLockCommand);
        XPLMDebugString("FLIR: GPS lock command executed\n");
    } else {
        XPLMDebugString("FLIR: GPS lock command not available\n");
    }
    
    if (gGPSDestLat && gGPSDestLon) {
        double currentGPSLat = XPLMGetDatad(gGPSDestLat);
        double currentGPSLon = XPLMGetDatad(gGPSDestLon);
        char gpsMsg[256];
        snprintf(gpsMsg, sizeof(gpsMsg), "FLIR: GPS destination now: %.6f°N %.6f°W\n", currentGPSLat, currentGPSLon);
        XPLMDebugString(gpsMsg);
    }
    
    char debugMsg[256];
    snprintf(debugMsg, sizeof(debugMsg), "FLIR: Target designated at %.6f°N %.6f°W, Alt: %.0fm (Range: %.0fm)", 
            gTargetLat, gTargetLon, gTargetAlt, targetRange);
    XPLMDebugString(debugMsg);
    XPLMDebugString("FLIR: Target ready - use F5 to fire (guidance will auto-start)\n");
    
    StartActiveGuidance();
}

void FireWeaponAtTarget()
{
    if (!gTargetDesignated) {
        XPLMDebugString("FLIR: No target designated - use Space to lock target first\n");
        return;
    }
    
    if (gActiveGuidance) {
        StopActiveGuidance();
        XPLMDebugString("FLIR: Active guidance STOPPED\n");
    } else {
        StartActiveGuidance();
        XPLMDebugString("FLIR: Active guidance STARTED\n");
    }
}

int IsTargetDesignated()
{
    return gTargetDesignated;
}

void LogWeaponSystemStatus()
{
    XPLMDebugString("FLIR: ===== WEAPON SYSTEM DEBUG =====\n");
    
    if (gWeaponCount) {
        int weaponCount = XPLMGetDatai(gWeaponCount);
        char msg[256];
        snprintf(msg, sizeof(msg), "FLIR: Total weapons detected: %d\n", weaponCount);
        XPLMDebugString(msg);
        
        if (gWeaponType && weaponCount > 0) {
            int weaponTypes[25] = {0};
            XPLMGetDatavi(gWeaponType, weaponTypes, 0, weaponCount);
            
            for (int i = 0; i < weaponCount && i < 25; i++) {
                char typeMsg[256];
                const char* typeName = "Unknown";
                switch (weaponTypes[i]) {
                    case 0: typeName = "None"; break;
                    case 1: typeName = "Gun"; break;
                    case 2: typeName = "Rocket"; break;
                    case 3: typeName = "Missile"; break;
                    case 4: typeName = "Bomb"; break;
                    case 5: typeName = "Flare"; break;
                    case 6: typeName = "Chaff"; break;
                    default: break;
                }
                snprintf(typeMsg, sizeof(typeMsg), "FLIR: Weapon[%d]: Type %d (%s)\n", i, weaponTypes[i], typeName);
                XPLMDebugString(typeMsg);
            }
        }
    } else {
        XPLMDebugString("FLIR: No weapon count dataref available\n");
    }
    
    if (gMasterArmDataRef) {
        int masterArm = XPLMGetDatai(gMasterArmDataRef);
        char msg[256];
        snprintf(msg, sizeof(msg), "FLIR: Master arm status: %s\n", masterArm ? "ARMED" : "SAFE");
        XPLMDebugString(msg);
    } else {
        XPLMDebugString("FLIR: Master arm dataref not available\n");
    }
    
    if (gWeaponsArmed) {
        int rocketsArmed = XPLMGetDatai(gWeaponsArmed);
        char msg[256];
        snprintf(msg, sizeof(msg), "FLIR: Rockets armed: %s\n", rocketsArmed ? "YES" : "NO");
        XPLMDebugString(msg);
    }
    
    if (gMissilesArmed) {
        int missilesArmed = XPLMGetDatai(gMissilesArmed);
        char msg[256];
        snprintf(msg, sizeof(msg), "FLIR: Missiles armed: %s\n", missilesArmed ? "YES" : "NO");
        XPLMDebugString(msg);
    }
    
    if (gBombsArmed) {
        int bombsArmed = XPLMGetDatai(gBombsArmed);
        char msg[256];
        snprintf(msg, sizeof(msg), "FLIR: Bombs armed: %s\n", bombsArmed ? "YES" : "NO");
        XPLMDebugString(msg);
    }
    
    if (gGunsArmed) {
        int gunsArmed = XPLMGetDatai(gGunsArmed);
        char msg[256];
        snprintf(msg, sizeof(msg), "FLIR: Guns armed: %s\n", gunsArmed ? "YES" : "NO");
        XPLMDebugString(msg);
    }
    
    if (gGPSLockCommand) {
        XPLMDebugString("FLIR: GPS lock command available\n");
    } else {
        XPLMDebugString("FLIR: GPS lock command NOT available\n");
    }
    
    if (gFireAnyArmedCommand) {
        XPLMDebugString("FLIR: Fire any armed command available\n");
    } else {
        XPLMDebugString("FLIR: Fire any armed command NOT available\n");
    }
    
    XPLMDebugString("FLIR: ================================\n");
}

void StartActiveGuidance()
{
    if (gActiveGuidance || !gTargetDesignated) {
        return;
    }
    
    gActiveGuidance = 1;
    
    if (!gGuidanceFlightLoop) {
        XPLMCreateFlightLoop_t flightLoopParams;
        flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
        flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
        flightLoopParams.callbackFunc = GuidanceFlightLoopCallback;
        flightLoopParams.refcon = NULL;
        
        gGuidanceFlightLoop = XPLMCreateFlightLoop(&flightLoopParams);
    }
    
    if (gGuidanceFlightLoop) {
        XPLMScheduleFlightLoop(gGuidanceFlightLoop, 0.1f, 1);
        XPLMDebugString("FLIR: Active weapon guidance started\n");
    }
}

void StopActiveGuidance()
{
    if (!gActiveGuidance) {
        return;
    }
    
    gActiveGuidance = 0;
    
    if (gGuidanceFlightLoop) {
        XPLMScheduleFlightLoop(gGuidanceFlightLoop, 0, 0);
        XPLMDebugString("FLIR: Active weapon guidance stopped\n");
    }
}

float GuidanceFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gActiveGuidance || !gTargetDesignated) {
        return 0;
    }
    
    if (!gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponCount) {
        return 0.1f;
    }
    
    int weaponCount = XPLMGetDatai(gWeaponCount);
    if (weaponCount <= 0) {
        return 0.1f;
    }
    
    float weaponX[25] = {0};
    float weaponY[25] = {0};
    float weaponZ[25] = {0};
    
    XPLMGetDatavf(gWeaponX, weaponX, 0, weaponCount);
    XPLMGetDatavf(gWeaponY, weaponY, 0, weaponCount);
    XPLMGetDatavf(gWeaponZ, weaponZ, 0, weaponCount);
    
    double targetLat = gTargetLat;
    double targetLon = gTargetLon;
    double targetAlt = gTargetAlt;
    
    float steeringFrn[25] = {0};
    float steeringSid[25] = {0};
    float steeringTop[25] = {0};
    float theConArray[25] = {0};
    float targetIndexArray[25] = {0};
    float newVX[25] = {0};
    float newVY[25] = {0};
    float newVZ[25] = {0};
    
    float currentVX[25] = {0};
    float currentVY[25] = {0};
    float currentVZ[25] = {0};
    
    XPLMGetDatavf(gWeaponVX, currentVX, 0, weaponCount);
    XPLMGetDatavf(gWeaponVY, currentVY, 0, weaponCount);
    XPLMGetDatavf(gWeaponVZ, currentVZ, 0, weaponCount);
    
    for (int i = 0; i < weaponCount && i < 25; i++) {
        if (weaponX[i] == 0 && weaponY[i] == 0 && weaponZ[i] == 0) {
            continue;
        }
        
        double currentLat = gLatDataRef ? XPLMGetDatad(gLatDataRef) : 37.0;
        double currentLon = gLonDataRef ? XPLMGetDatad(gLonDataRef) : -122.0;
        
        double weaponOffsetX = weaponX[i];
        double weaponOffsetY = weaponY[i]; 
        double weaponOffsetZ = weaponZ[i];
        
        double weaponLat = currentLat + (weaponOffsetZ / 111320.0);
        double weaponLon = currentLon + (weaponOffsetX / (111320.0 * cos(currentLat * M_PI / 180.0)));
        
        double deltaLat = targetLat - weaponLat;
        double deltaLon = targetLon - weaponLon;
        double deltaAlt = targetAlt - weaponOffsetY;
        
        double deltaX = deltaLon * 111320.0 * cos(weaponLat * M_PI / 180.0);
        double deltaY = deltaAlt;
        double deltaZ = deltaLat * 111320.0;
        
        double distance = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
        
        if (distance > 50.0) {  // Minimum distance threshold
            // Use proven precision guidance parameters
            float proportionalGain = 1.0f;
            float dampingFactor = 0.85f;
            float maxCorrectionSpeed = 15.0f;
            
            // Normalize direction to target
            float dirX = (float)(deltaX / distance);
            float dirY = (float)(deltaY / distance);
            float dirZ = (float)(deltaZ / distance);
            
            // Speed control based on distance
            float desiredSpeed = fmin(distance * 0.08f, 120.0f); // Max 120 m/s
            if (desiredSpeed < 15.0f) desiredSpeed = 15.0f;      // Min 15 m/s
            
            // Calculate desired velocity
            float desiredVX = dirX * desiredSpeed;
            float desiredVY = dirY * desiredSpeed;
            float desiredVZ = dirZ * desiredSpeed;
            
            // Calculate correction needed
            float correctionX = (desiredVX - currentVX[i]) * proportionalGain;
            float correctionY = (desiredVY - currentVY[i]) * proportionalGain;
            float correctionZ = (desiredVZ - currentVZ[i]) * proportionalGain;
            
            // Limit correction magnitude for stability
            float correctionMag = sqrt(correctionX*correctionX + correctionY*correctionY + correctionZ*correctionZ);
            if (correctionMag > maxCorrectionSpeed) {
                float scale = maxCorrectionSpeed / correctionMag;
                correctionX *= scale;
                correctionY *= scale;
                correctionZ *= scale;
            }
            
            // Apply smooth correction with damping
            newVX[i] = (currentVX[i] + correctionX) * dampingFactor;
            newVY[i] = (currentVY[i] + correctionY) * dampingFactor;
            newVZ[i] = (currentVZ[i] + correctionZ) * dampingFactor;
            
            double bearing = atan2(deltaX, deltaZ) * 180.0 / M_PI;
            double elevation = atan2(deltaY, sqrt(deltaX * deltaX + deltaZ * deltaZ)) * 180.0 / M_PI;
            
            steeringFrn[i] = (float)(bearing * 0.01);
            steeringSid[i] = (float)(bearing * 0.01);
            steeringTop[i] = (float)(elevation * 0.01);
            theConArray[i] = (float)bearing;
            targetIndexArray[i] = 1.0f;
            
            if (i == 0) {
                char guidanceMsg[256];
                snprintf(guidanceMsg, sizeof(guidanceMsg), "FLIR: Weapon[%d] at %.6f,%.6f -> %.6f,%.6f (dist: %.0fm, bearing: %.1f°)\n", 
                        i, weaponLat, weaponLon, targetLat, targetLon, distance, bearing);
                XPLMDebugString(guidanceMsg);
            }
        }
    }
    
    if (gWeaponVX && gWeaponVY && gWeaponVZ) {
        XPLMSetDatavf(gWeaponVX, newVX, 0, weaponCount);
        XPLMSetDatavf(gWeaponVY, newVY, 0, weaponCount);
        XPLMSetDatavf(gWeaponVZ, newVZ, 0, weaponCount);
    }
    
    if (gWeaponSFrn && gWeaponSSid && gWeaponSTop) {
        XPLMSetDatavf(gWeaponSFrn, steeringFrn, 0, weaponCount);
        XPLMSetDatavf(gWeaponSSid, steeringSid, 0, weaponCount);
        XPLMSetDatavf(gWeaponSTop, steeringTop, 0, weaponCount);
    }
    
    if (gWeaponThecon) {
        XPLMSetDatavf(gWeaponThecon, theConArray, 0, weaponCount);
    }
    
    if (gWeaponTargetIndex) {
        XPLMSetDatavf(gWeaponTargetIndex, targetIndexArray, 0, weaponCount);
    }
    
    return 0.1f;
}