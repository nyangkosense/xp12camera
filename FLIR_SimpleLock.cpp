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
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;

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
    // Legacy function - not used in position-based system
    gTargetDesignated = 0;
}

void DesignateTarget(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle)
{
    // Use the actual camera angles (where user pointed the crosshair)
    // INVERT pan angle: positive pan should turn LEFT in X-Plane coordinate system
    float headingRad = (planeHeading - panAngle) * M_PI / 180.0f;
    float tiltRad = tiltAngle * M_PI / 180.0f;
    
    // Calculate ground intersection distance based on actual tilt
    double targetRange = 5000.0; // Default range
    if (tiltAngle < -5.0f && planeY > 100.0f) {
        targetRange = planeY / fabs(sin(tiltRad)); // Ground intersection
        if (targetRange > 20000.0) targetRange = 20000.0;
        if (targetRange < 500.0) targetRange = 500.0;
    }
    
    // Calculate target coordinates based on actual camera direction
    double deltaX = targetRange * sin(headingRad) * cos(tiltRad);
    double deltaZ = targetRange * cos(headingRad) * cos(tiltRad);
    double deltaY = targetRange * sin(tiltRad); // Negative for downward tilt
    
    gTargetX = planeX + (float)deltaX;
    gTargetY = planeY + (float)deltaY; // Should hit ground level
    gTargetZ = planeZ + (float)deltaZ;
    gTargetDesignated = 1;
    
    // Auto-arm weapons
    if (gMissilesArmed && gBombsArmed && gWeaponsArmed) {
        XPLMSetDatai(gMissilesArmed, 1);
        XPLMSetDatai(gBombsArmed, 1);
        XPLMSetDatai(gWeaponsArmed, 1);
        XPLMDebugString("FLIR: Auto-armed missiles, bombs, and rockets\n");
    }
    
    // Log target designation with detailed calculation info
    char debugMsg[512];
    snprintf(debugMsg, sizeof(debugMsg), 
        "FLIR: CROSSHAIR ANALYSIS\n"
        "FLIR: INPUT - Aircraft:(%.0f,%.0f,%.0f) Heading:%.1f°\n"
        "FLIR: CAMERA - Pan:%.1f° Tilt:%.1f° → Look Direction:%.1f° (INVERTED PAN)\n"
        "FLIR: RANGE - Calculated:%.0fm (Tilt-based ground intersect)\n"
        "FLIR: VECTOR - DeltaX:%.0f DeltaY:%.0f DeltaZ:%.0f\n"
        "FLIR: TARGET - Final:(%.0f,%.0f,%.0f)\n"
        "FLIR: CROSSHAIR → This is where your crosshair should hit!\n", 
        planeX, planeY, planeZ, planeHeading,
        panAngle, tiltAngle, (planeHeading - panAngle),
        targetRange,
        (float)deltaX, (float)deltaY, (float)deltaZ,
        gTargetX, gTargetY, gTargetZ);
    XPLMDebugString(debugMsg);
    
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

void GetCrosshairWorldPosition(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle, float* outX, float* outY, float* outZ)
{
    // Calculate where the crosshair is pointing in world coordinates
    // INVERT pan angle: positive pan should turn LEFT in X-Plane coordinate system
    float headingRad = (planeHeading - panAngle) * M_PI / 180.0f;
    float tiltRad = tiltAngle * M_PI / 180.0f;
    
    // Use same range calculation as DesignateTarget for consistency
    double targetRange = 5000.0; // Default range
    if (tiltAngle < -5.0f && planeY > 100.0f) {
        targetRange = planeY / fabs(sin(tiltRad)); // Ground intersection
        if (targetRange > 20000.0) targetRange = 20000.0;
        if (targetRange < 500.0) targetRange = 500.0;
    }
    
    // Calculate world position where crosshair is pointing
    double deltaX = targetRange * sin(headingRad) * cos(tiltRad);
    double deltaZ = targetRange * cos(headingRad) * cos(tiltRad);
    double deltaY = targetRange * sin(tiltRad);
    
    *outX = planeX + (float)deltaX;
    *outY = planeY + (float)deltaY;
    *outZ = planeZ + (float)deltaZ;
}

void LogCrosshairPosition(float planeX, float planeY, float planeZ, float planeHeading, float panAngle, float tiltAngle)
{
    float crosshairX, crosshairY, crosshairZ;
    GetCrosshairWorldPosition(planeX, planeY, planeZ, planeHeading, panAngle, tiltAngle, &crosshairX, &crosshairY, &crosshairZ);
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "FLIR: CROSSHAIR REALTIME - Pan:%.1f° Tilt:%.1f° → World:(%.0f,%.0f,%.0f)\n", 
        panAngle, tiltAngle, crosshairX, crosshairY, crosshairZ);
    XPLMDebugString(msg);
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
    
    // Use local position coordinates (like working F3 system)
    float targetX = gTargetX;
    float targetY = gTargetY;
    float targetZ = gTargetZ;
    
    float newVX[25] = {0};
    float newVY[25] = {0};
    float newVZ[25] = {0};
    
    float currentVX[25] = {0};
    float currentVY[25] = {0};
    float currentVZ[25] = {0};
    
    XPLMGetDatavf(gWeaponVX, currentVX, 0, weaponCount);
    XPLMGetDatavf(gWeaponVY, currentVY, 0, weaponCount);
    XPLMGetDatavf(gWeaponVZ, currentVZ, 0, weaponCount);
    
    // Apply precision guidance to first 2 weapons only
    for (int i = 0; i < 2 && i < weaponCount; i++) {
        if (weaponX[i] == 0 && weaponY[i] == 0 && weaponZ[i] == 0) {
            // Inactive missile
            newVX[i] = currentVX[i];
            newVY[i] = currentVY[i];
            newVZ[i] = currentVZ[i];
            continue;
        }
        
        // Calculate vector to target (direct position - like F3 system)
        float deltaX = targetX - weaponX[i];
        float deltaY = targetY - weaponY[i];
        float deltaZ = targetZ - weaponZ[i];
        
        float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
        
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
            
            // Log guidance status for first missile only
            if (i == 0) {
                float speed = sqrt(newVX[i]*newVX[i] + newVY[i]*newVY[i] + newVZ[i]*newVZ[i]);
                char guidanceMsg[256];
                snprintf(guidanceMsg, sizeof(guidanceMsg), 
                    "FLIR: Missile[%d] Pos:(%.0f,%.0f,%.0f) → Target:(%.0f,%.0f,%.0f) Dist:%.0fm Speed:%.1fm/s\n", 
                    i, weaponX[i], weaponY[i], weaponZ[i], targetX, targetY, targetZ, distance, speed);
                XPLMDebugString(guidanceMsg);
            }
        }
    }
    
    // Apply velocity-based guidance (proven working method)
    if (gWeaponVX && gWeaponVY && gWeaponVZ) {
        XPLMSetDatavf(gWeaponVX, newVX, 0, weaponCount);
        XPLMSetDatavf(gWeaponVY, newVY, 0, weaponCount);
        XPLMSetDatavf(gWeaponVZ, newVZ, 0, weaponCount);
    }
    
    return 0.1f;
}