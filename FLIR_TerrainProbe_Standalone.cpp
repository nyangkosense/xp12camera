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

// Weapon guidance datarefs
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;

// Target coordinates (set by terrain probe)
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static bool gTargetValid = false;

// Function prototypes
static void ProbeTerrainTarget(void* inRefcon);
static float GuideMissileToTarget(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static bool GetFLIRTerrainIntersection(float* outX, float* outY, float* outZ);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Terrain Probe");
    strcpy(outSig, "flir.terrain.probe.standalone");
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
    
    // Weapon system datarefs
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");

    // Create terrain probe
    gTerrainProbe = XPLMCreateProbe(xplm_ProbeY);
    if (!gTerrainProbe) {
        XPLMDebugString("TERRAIN PROBE: Failed to create terrain probe!\n");
        return 0;
    }

    // Register hotkey for target designation
    XPLMRegisterHotKey(XPLM_VK_SPACE, xplm_DownFlag, "FLIR: Probe Target", ProbeTerrainTarget, NULL);
    
    // Register flight loop for missile guidance at 50Hz for precision
    XPLMRegisterFlightLoopCallback(GuideMissileToTarget, 0.02f, NULL);

    XPLMDebugString("TERRAIN PROBE: Plugin loaded successfully\n");
    XPLMDebugString("TERRAIN PROBE: SPACEBAR=Probe terrain target under FLIR crosshair\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) 
{
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
    float minDist = 0.0f;
    float tolerance = 1.0f;    // 1 meter precision
    
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
            if (testY < probeInfo.locationY) {
                // Ray point is underground - back up
                maxDist = testDist;
            } else {
                // Ray point is above ground - go further  
                minDist = testDist;
            }
        } else {
            // No terrain hit - reduce range
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
    
    // Add FLIR camera angles (relative to aircraft)
    float panRad = cameraPan * M_PI / 180.0f;
    float tiltRad = cameraTilt * M_PI / 180.0f;
    
    // Calculate FLIR camera direction vector
    float totalHeading = headingRad + panRad;
    float totalPitch = pitchRad + tiltRad;
    
    // Direction vector from camera position
    float dirX = sin(totalHeading) * cos(totalPitch);
    float dirY = sin(totalPitch);
    float dirZ = cos(totalHeading) * cos(totalPitch);
    
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

// Enhanced missile guidance using precise terrain coordinates
static float GuideMissileToTarget(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gTargetValid) return -1.0f;
    
    // Get current weapon position
    float weaponX = XPLMGetDataf(gWeaponX);
    float weaponY = XPLMGetDataf(gWeaponY);
    float weaponZ = XPLMGetDataf(gWeaponZ);
    
    // Check if weapon exists and is in flight
    if (weaponX == 0.0f && weaponY == 0.0f && weaponZ == 0.0f) {
        return -1.0f; // No weapon in flight
    }
    
    // Calculate vector to target
    float deltaX = gTargetX - weaponX;
    float deltaY = gTargetY - weaponY;
    float deltaZ = gTargetZ - weaponZ;
    
    // Calculate distance to target
    float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
    
    if (distance < 50.0f) {
        // Close to target - stop guidance
        gTargetValid = false;
        XPLMDebugString("TERRAIN PROBE: Missile reached target!\n");
        return -1.0f;
    }
    
    // Proportional navigation guidance  
    float maxVelocity = 150.0f; // m/s
    
    // Normalize direction vector
    float dirX = deltaX / distance;
    float dirY = deltaY / distance;
    float dirZ = deltaZ / distance;
    
    // Calculate desired velocity
    float desiredVX = dirX * maxVelocity;
    float desiredVY = dirY * maxVelocity;
    float desiredVZ = dirZ * maxVelocity;
    
    // Apply smoothing for more realistic guidance
    float currentVX = XPLMGetDataf(gWeaponVX);
    float currentVY = XPLMGetDataf(gWeaponVY);
    float currentVZ = XPLMGetDataf(gWeaponVZ);
    
    float smoothFactor = 0.1f; // Lower value for 50Hz - more responsive guidance
    float newVX = currentVX + (desiredVX - currentVX) * smoothFactor;
    float newVY = currentVY + (desiredVY - currentVY) * smoothFactor;
    float newVZ = currentVZ + (desiredVZ - currentVZ) * smoothFactor;
    
    // Set weapon velocity
    XPLMSetDataf(gWeaponVX, newVX);
    XPLMSetDataf(gWeaponVY, newVY);
    XPLMSetDataf(gWeaponVZ, newVZ);
    
    // Debug output every 2 seconds (less frequent due to 50Hz updates)
    static float debugTimer = 0.0f;
    debugTimer += inElapsedSinceLastCall;
    if (debugTimer >= 2.0f) {
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Guiding missile dist=%.0fm vel=(%.1f,%.1f,%.1f)\n",
            distance, newVX, newVY, newVZ);
        XPLMDebugString(msg);
        debugTimer = 0.0f;
    }
    
    return -1.0f; // Continue flight loop
}