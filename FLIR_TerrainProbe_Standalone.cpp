/*
 * FLIR_TerrainProbe_Standalone.cpp
 * 
 * Standalone terrain probing plugin that works WITH the existing FLIR camera
 * Reads FLIR camera angles from datarefs and provides precision targeting
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMScenery.h"

// Need these for modern flight loop API
static XPLMFlightLoopID gFlightLoopID = NULL;

// Terrain probe
static XPLMProbeRef gTerrainProbe = NULL;

// Aircraft position datarefs
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;
static XPLMDataRef gAircraftPitch = NULL;

// FLIR camera datarefs (published by main FLIR plugin)
static XPLMDataRef gCameraPanDataRef = NULL;
static XPLMDataRef gCameraTiltDataRef = NULL;
static XPLMDataRef gCameraActiveDataRef = NULL;

// Weapon guidance datarefs - slots [0] and [1]
static XPLMDataRef gWeapon0X = NULL;
static XPLMDataRef gWeapon0Y = NULL;
static XPLMDataRef gWeapon0Z = NULL;
static XPLMDataRef gWeapon0VX = NULL;
static XPLMDataRef gWeapon0VY = NULL;
static XPLMDataRef gWeapon0VZ = NULL;

static XPLMDataRef gWeapon1X = NULL;
static XPLMDataRef gWeapon1Y = NULL;
static XPLMDataRef gWeapon1Z = NULL;
static XPLMDataRef gWeapon1VX = NULL;
static XPLMDataRef gWeapon1VY = NULL;
static XPLMDataRef gWeapon1VZ = NULL;

// Target coordinates (set by terrain probe)
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static bool gTargetValid = false;

// Track which missiles are being guided
static bool gGuiding0 = false;
static bool gGuiding1 = false;

// Function prototypes
static void ProbeTerrainTarget(void* inRefcon);
static void LaunchMissile(void* inRefcon);
static float GuideMissileToTarget(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static bool GetFLIRTerrainIntersection(float* outX, float* outY, float* outZ);
static void GuideWeapon(int weaponNum, XPLMDataRef posX, XPLMDataRef posY, XPLMDataRef posZ, XPLMDataRef velX, XPLMDataRef velY, XPLMDataRef velZ, float deltaTime);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Terrain Probe");
    strcpy(outSig, "terrain.probe.v2.standalone");
    strcpy(outDesc, "Precision FLIR targeting using terrain probing");

    // Initialize aircraft datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    gAircraftPitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    
    // Try to find FLIR camera datarefs (published by main FLIR plugin)
    gCameraPanDataRef = XPLMFindDataRef("flir/camera/pan");
    gCameraTiltDataRef = XPLMFindDataRef("flir/camera/tilt");
    gCameraActiveDataRef = XPLMFindDataRef("flir/camera/active");
    
    if (!gCameraPanDataRef || !gCameraTiltDataRef || !gCameraActiveDataRef) {
        XPLMDebugString("TERRAIN PROBE: Warning - FLIR camera datarefs not found. Make sure FLIR camera plugin is loaded first.\n");
    }
    
    // Weapon system datarefs - weapon slots [0] and [1]
    gWeapon0X = XPLMFindDataRef("sim/weapons/x[0]");
    gWeapon0Y = XPLMFindDataRef("sim/weapons/y[0]");
    gWeapon0Z = XPLMFindDataRef("sim/weapons/z[0]");
    gWeapon0VX = XPLMFindDataRef("sim/weapons/vx[0]");
    gWeapon0VY = XPLMFindDataRef("sim/weapons/vy[0]");
    gWeapon0VZ = XPLMFindDataRef("sim/weapons/vz[0]");
    
    gWeapon1X = XPLMFindDataRef("sim/weapons/x[1]");
    gWeapon1Y = XPLMFindDataRef("sim/weapons/y[1]");
    gWeapon1Z = XPLMFindDataRef("sim/weapons/z[1]");
    gWeapon1VX = XPLMFindDataRef("sim/weapons/vx[1]");
    gWeapon1VY = XPLMFindDataRef("sim/weapons/vy[1]");
    gWeapon1VZ = XPLMFindDataRef("sim/weapons/vz[1]");
    
    // Debug: Check if weapon datarefs exist
    if (!gWeapon0X || !gWeapon0Y || !gWeapon0Z) {
        XPLMDebugString("TERRAIN PROBE: Warning - weapon[0] position datarefs not found\n");
    }
    if (!gWeapon0VX || !gWeapon0VY || !gWeapon0VZ) {
        XPLMDebugString("TERRAIN PROBE: Warning - weapon[0] velocity datarefs not found\n");
    }
    if (!gWeapon1X || !gWeapon1Y || !gWeapon1Z) {
        XPLMDebugString("TERRAIN PROBE: Warning - weapon[1] position datarefs not found\n");
    }
    if (!gWeapon1VX || !gWeapon1VY || !gWeapon1VZ) {
        XPLMDebugString("TERRAIN PROBE: Warning - weapon[1] velocity datarefs not found\n");
    }

    // Create terrain probe
    gTerrainProbe = XPLMCreateProbe(xplm_ProbeY);
    if (!gTerrainProbe) {
        XPLMDebugString("TERRAIN PROBE: Failed to create terrain probe!\n");
        return 0;
    }

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_TAB, xplm_DownFlag, "Terrain: Probe Target", ProbeTerrainTarget, NULL);
    XPLMRegisterHotKey(XPLM_VK_SPACE, xplm_DownFlag, "Missile: Launch", LaunchMissile, NULL);
    
    // Register flight loop using modern API for better reliability
    XPLMCreateFlightLoop_t flightLoopParams;
    flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
    flightLoopParams.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    flightLoopParams.callbackFunc = GuideMissileToTarget;
    flightLoopParams.refcon = NULL;
    
    gFlightLoopID = XPLMCreateFlightLoop(&flightLoopParams);
    if (gFlightLoopID) {
        XPLMScheduleFlightLoop(gFlightLoopID, 0.02f, 1); // Start in 0.02 seconds, repeat
        XPLMDebugString("TERRAIN PROBE: Modern flight loop created and scheduled\n");
    } else {
        XPLMDebugString("TERRAIN PROBE: Failed to create modern flight loop, trying legacy...\n");
        // Fallback to legacy API
        XPLMRegisterFlightLoopCallback(GuideMissileToTarget, 0.02f, NULL);
    }

    XPLMDebugString("TERRAIN PROBE: Plugin loaded successfully\n");
    XPLMDebugString("TERRAIN PROBE: Flight loop registered - should start running now\n");
    XPLMDebugString("TERRAIN PROBE: TAB=Probe terrain target, SPACEBAR=Activate guidance for weapons in flight\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) 
{
    if (gFlightLoopID) {
        XPLMDestroyFlightLoop(gFlightLoopID);
        gFlightLoopID = NULL;
    }
    
    if (gTerrainProbe) {
        XPLMDestroyProbe(gTerrainProbe);
        gTerrainProbe = NULL;
    }
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

// Get FLIR camera angles from datarefs
static bool GetFLIRAngles(float* outPan, float* outTilt, bool* outActive)
{
    if (!gCameraPanDataRef || !gCameraTiltDataRef || !gCameraActiveDataRef) {
        return false;
    }
    
    *outPan = XPLMGetDataf(gCameraPanDataRef);
    *outTilt = XPLMGetDataf(gCameraTiltDataRef);
    *outActive = (XPLMGetDatai(gCameraActiveDataRef) != 0);
    
    return true;
}

// Precise binary search terrain intersection - finds exact hit point within 1m
static bool FindPreciseTarget(float startX, float startY, float startZ, float dirX, float dirY, float dirZ, float* outX, float* outY, float* outZ, float* outRange)
{
    if (!gTerrainProbe) return false;
    
    float maxDist = 30000.0f;  // 30km maximum range  
    float minDist = 100.0f;    // Start at 100m minimum (avoid aircraft itself)
    float tolerance = 10.0f;   // 10 meter precision for now (faster debugging)
    
    XPLMProbeInfo_t probeInfo;
    probeInfo.structSize = sizeof(XPLMProbeInfo_t);
    
    // Binary search for precise intersection
    while ((maxDist - minDist) > tolerance) {
        float testDist = (maxDist + minDist) / 2.0f;
        
        // Calculate test point along ray
        float testX = startX + testDist * dirX;
        float testY = startY + testDist * dirY;
        float testZ = startZ + testDist * dirZ;
        
        // Probe terrain at test point
        XPLMProbeResult result = XPLMProbeTerrainXYZ(gTerrainProbe, testX, testY, testZ, &probeInfo);
        
        if (result == xplm_ProbeHitTerrain) {
            float terrainHeight = probeInfo.locationY;
            if (testY <= terrainHeight + 5.0f) {
                // Ray point hits terrain (within 5m tolerance) - we found intersection
                maxDist = testDist;
                break;
            } else {
                // Ray point is above terrain - go further
                minDist = testDist;
            }
        } else {
            // No terrain hit at this point - reduce search range
            maxDist = testDist;
        }
    }
    
    // Final precise probe at intersection point
    float finalDist = (maxDist + minDist) / 2.0f;
    float finalX = startX + finalDist * dirX;
    float finalY = startY + finalDist * dirY;
    float finalZ = startZ + finalDist * dirZ;
    
    XPLMProbeResult finalResult = XPLMProbeTerrainXYZ(gTerrainProbe, finalX, finalY, finalZ, &probeInfo);
    
    if (finalResult == xplm_ProbeHitTerrain) {
        *outX = probeInfo.locationX;
        *outY = probeInfo.locationY;
        *outZ = probeInfo.locationZ;
        *outRange = finalDist;
        return true;
    }
    
    return false;
}

// Use precise binary search to find FLIR terrain intersection
static bool GetFLIRTerrainIntersection(float* outX, float* outY, float* outZ)
{
    // Get FLIR camera angles
    float cameraPan, cameraTilt;
    bool cameraActive;
    
    if (!GetFLIRAngles(&cameraPan, &cameraTilt, &cameraActive)) {
        // Fallback to aircraft direction if FLIR not available
        cameraPan = 0.0f;
        cameraTilt = 0.0f;
        cameraActive = true;
        XPLMDebugString("TERRAIN PROBE: Using aircraft direction (FLIR not available)\n");
    }
    
    // Get aircraft position and orientation
    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);
    float acHeading = XPLMGetDataf(gAircraftHeading);
    float acPitch = XPLMGetDataf(gAircraftPitch);
    
    // Convert to radians
    float headingRad = acHeading * M_PI / 180.0f;
    float pitchRad = acPitch * M_PI / 180.0f;
    float panRad = cameraPan * M_PI / 180.0f;
    float tiltRad = cameraTilt * M_PI / 180.0f;
    
    // Note: Future expansion for full 3D rotation matrices would use these
    // float cosH = cos(headingRad), sinH = sin(headingRad);
    // float cosP = cos(pitchRad), sinP = sin(pitchRad);
    // float cosPan = cos(panRad), sinPan = sin(panRad);
    // float cosTilt = cos(tiltRad), sinTilt = sin(tiltRad);
    
    // Combined rotation: aircraft orientation + gimbal orientation
    // This is a simplified version - proper 3D rotation matrices would be better
    float totalHeading = headingRad + panRad;
    float totalPitch = pitchRad + tiltRad;
    
    // Direction vector calculation - FIXED
    // In X-Plane: +Y is UP, -Y is DOWN
    // When aircraft pitches down (negative pitch), we want to point more downward
    // When FLIR tilts down (negative tilt), we want to point more downward
    
    float dirX = sin(totalHeading) * cos(totalPitch);
    float dirY = sin(totalPitch);  // Positive pitch = up, negative pitch = down
    float dirZ = cos(totalHeading) * cos(totalPitch);
    
    // FLIR typically points downward relative to aircraft, so adjust
    // If total pitch is positive (pointing up), force it to point down
    if (dirY > 0.1f) {  // If pointing significantly upward
        dirY = -0.5f;   // Force downward direction
        XPLMDebugString("TERRAIN PROBE: Forced direction downward (was pointing up)\n");
    }
    
    // Debug direction
    char directionInfo[256];
    snprintf(directionInfo, sizeof(directionInfo), 
        "TERRAIN PROBE: totalPitch=%.1f° dirY=%.3f %s\n",
        totalPitch * 180.0f / M_PI, dirY, (dirY < 0) ? "DOWN" : "UP");
    XPLMDebugString(directionInfo);
    
    char dirMsg[256];
    snprintf(dirMsg, sizeof(dirMsg), "TERRAIN PROBE: Direction vector=(%.3f,%.3f,%.3f) heading=%.1f° pitch=%.1f° pan=%.1f° tilt=%.1f°\n",
        dirX, dirY, dirZ, acHeading, acPitch, cameraPan, cameraTilt);
    XPLMDebugString(dirMsg);
    
    // Use precise binary search to find terrain intersection
    float range;
    if (FindPreciseTarget(acX, acY, acZ, dirX, dirY, dirZ, outX, outY, outZ, &range)) {
        char msg[512];
        snprintf(msg, sizeof(msg), 
            "TERRAIN PROBE: Precise hit at (%.2f, %.2f, %.2f) range=%.1fm pan=%.1f° tilt=%.1f°\n",
            *outX, *outY, *outZ, range, cameraPan, cameraTilt);
        XPLMDebugString(msg);
        return true;
    }
    
    XPLMDebugString("TERRAIN PROBE: No terrain intersection found\n");
    return false;
}

// Precise terrain target designation using probing
static void ProbeTerrainTarget(void* inRefcon)
{
    // Use terrain probing to find precise target coordinates
    if (GetFLIRTerrainIntersection(&gTargetX, &gTargetY, &gTargetZ)) {
        gTargetValid = true;
        
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Target designated at (%.2f, %.2f, %.2f)\n",
            gTargetX, gTargetY, gTargetZ);
        XPLMDebugString(msg);
    } else {
        gTargetValid = false;
        XPLMDebugString("TERRAIN PROBE: Failed to designate target\n");
    }
}

// Activate guidance for missiles in weapon slots
static void LaunchMissile(void* inRefcon)
{
    if (!gTargetValid) {
        XPLMDebugString("TERRAIN PROBE: Cannot launch - no target designated! Press TAB first.\n");
        return;
    }
    
    // Check if weapons exist and activate guidance
    bool foundWeapon = false;
    
    // Check weapon slot [0]
    if (gWeapon0X && gWeapon0Y && gWeapon0Z) {
        float wx = XPLMGetDataf(gWeapon0X);
        float wy = XPLMGetDataf(gWeapon0Y);
        float wz = XPLMGetDataf(gWeapon0Z);
        
        if (wx != 0.0f || wy != 0.0f || wz != 0.0f) {
            gGuiding0 = true;
            foundWeapon = true;
            XPLMDebugString("TERRAIN PROBE: Activating guidance for weapon[0]\n");
        }
    }
    
    // Check weapon slot [1]
    if (gWeapon1X && gWeapon1Y && gWeapon1Z) {
        float wx = XPLMGetDataf(gWeapon1X);
        float wy = XPLMGetDataf(gWeapon1Y);
        float wz = XPLMGetDataf(gWeapon1Z);
        
        if (wx != 0.0f || wy != 0.0f || wz != 0.0f) {
            gGuiding1 = true;
            foundWeapon = true;
            XPLMDebugString("TERRAIN PROBE: Activating guidance for weapon[1]\n");
        }
    }
    
    if (!foundWeapon) {
        XPLMDebugString("TERRAIN PROBE: No weapons found in slots [0] or [1]. Fire weapons first!\n");
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Guidance activated! Target: (%.1f,%.1f,%.1f)\n",
            gTargetX, gTargetY, gTargetZ);
        XPLMDebugString(msg);
    }
}

// Enhanced missile guidance using precise terrain coordinates
static float GuideMissileToTarget(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    // Debug: Immediate confirmation that flight loop is running
    static bool firstRun = true;
    if (firstRun) {
        XPLMDebugString("TERRAIN PROBE: *** FLIGHT LOOP IS RUNNING! ***\n");
        firstRun = false;
    }
    
    // Debug: Check if we even have a target
    static float targetDebugTimer = 0.0f;
    targetDebugTimer += inElapsedSinceLastCall;
    if (targetDebugTimer >= 3.0f) {  // More frequent debugging
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Flight loop running, target valid=%s\n", 
            gTargetValid ? "YES" : "NO");
        XPLMDebugString(msg);
        targetDebugTimer = 0.0f;
    }
    
    if (!gTargetValid || (!gGuiding0 && !gGuiding1)) {
        // No target or no weapons being guided, but continue flight loop
        return 0.02f; // Continue at 50Hz
    }
    
    // Guide weapon[0] if active
    if (gGuiding0 && gWeapon0X && gWeapon0VX) {
        GuideWeapon(0, gWeapon0X, gWeapon0Y, gWeapon0Z, gWeapon0VX, gWeapon0VY, gWeapon0VZ, inElapsedSinceLastCall);
    }
    
    // Guide weapon[1] if active  
    if (gGuiding1 && gWeapon1X && gWeapon1VX) {
        GuideWeapon(1, gWeapon1X, gWeapon1Y, gWeapon1Z, gWeapon1VX, gWeapon1VY, gWeapon1VZ, inElapsedSinceLastCall);
    }
    
    return 0.02f; // Continue at 50Hz
}

// Guide individual weapon to target
static void GuideWeapon(int weaponNum, XPLMDataRef posX, XPLMDataRef posY, XPLMDataRef posZ, XPLMDataRef velX, XPLMDataRef velY, XPLMDataRef velZ, float deltaTime)
{
    // Get current weapon position
    float wx = XPLMGetDataf(posX);
    float wy = XPLMGetDataf(posY);
    float wz = XPLMGetDataf(posZ);
    
    // Check if weapon still exists
    if (wx == 0.0f && wy == 0.0f && wz == 0.0f) {
        // Weapon disappeared - stop guidance
        if (weaponNum == 0) gGuiding0 = false;
        if (weaponNum == 1) gGuiding1 = false;
        
        char msg[128];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Weapon[%d] disappeared - stopping guidance\n", weaponNum);
        XPLMDebugString(msg);
        return;
    }
    
    // Calculate vector to target
    float deltaX = gTargetX - wx;
    float deltaY = gTargetY - wy;
    float deltaZ = gTargetZ - wz;
    float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
    
    // Check if close to target
    if (distance < 50.0f) {
        char msg[128];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: *** WEAPON[%d] HIT TARGET! ***\n", weaponNum);
        XPLMDebugString(msg);
        
        // Stop guidance for this weapon
        if (weaponNum == 0) gGuiding0 = false;
        if (weaponNum == 1) gGuiding1 = false;
        return;
    }
    
    // Proportional navigation guidance
    float maxVelocity = 200.0f; // m/s
    
    // Normalize direction vector
    float dirX = deltaX / distance;
    float dirY = deltaY / distance;
    float dirZ = deltaZ / distance;
    
    // Calculate desired velocity
    float desiredVX = dirX * maxVelocity;
    float desiredVY = dirY * maxVelocity;
    float desiredVZ = dirZ * maxVelocity;
    
    // Get current velocity
    float currentVX = XPLMGetDataf(velX);
    float currentVY = XPLMGetDataf(velY);
    float currentVZ = XPLMGetDataf(velZ);
    
    // Apply guidance with smoothing
    float smoothFactor = 0.3f; // More aggressive for real weapons
    float newVX = currentVX + (desiredVX - currentVX) * smoothFactor;
    float newVY = currentVY + (desiredVY - currentVY) * smoothFactor;
    float newVZ = currentVZ + (desiredVZ - currentVZ) * smoothFactor;
    
    // Set new weapon velocity
    XPLMSetDataf(velX, newVX);
    XPLMSetDataf(velY, newVY);
    XPLMSetDataf(velZ, newVZ);
    
    // Debug output every few seconds
    static float debugTimers[2] = {0.0f, 0.0f};
    debugTimers[weaponNum] += deltaTime;
    if (debugTimers[weaponNum] >= 2.0f) {
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Guiding weapon[%d] dist=%.0fm vel=(%.1f,%.1f,%.1f)\n",
            weaponNum, distance, newVX, newVY, newVZ);
        XPLMDebugString(msg);
        debugTimers[weaponNum] = 0.0f;
    }
}