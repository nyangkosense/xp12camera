/*
 * FLIR_TerrainProbe.cpp
 * 
 * Advanced FLIR targeting using X-Plane's terrain probing system
 * This solves the coordinate accuracy issue by using XPLMProbeTerrainXYZ
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
#include "XPLMScenery.h"  // For terrain probing

// External FLIR camera variables (from main FLIR plugin)
extern float gCameraPan;
extern float gCameraTilt;
extern bool gCameraActive;

// Terrain probe
static XPLMProbeRef gTerrainProbe = NULL;

// Aircraft position datarefs
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;
static XPLMDataRef gAircraftPitch = NULL;
static XPLMDataRef gAircraftRoll = NULL;

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
    strcpy(outName, "FLIR Terrain Probe Targeting");
    strcpy(outSig, "flir.terrain.probe");
    strcpy(outDesc, "Precision FLIR targeting using terrain probing");

    // Initialize datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    gAircraftPitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    gAircraftRoll = XPLMFindDataRef("sim/flightmodel/position/phi");
    
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
    
    // Register flight loop for missile guidance
    XPLMRegisterFlightLoopCallback(GuideMissileToTarget, 0.1f, NULL);

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

// Use terrain probing to find precise ground intersection
static bool GetFLIRTerrainIntersection(float* outX, float* outY, float* outZ)
{
    if (!gTerrainProbe) return false;
    
    // Get aircraft position and orientation
    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);
    float acHeading = XPLMGetDataf(gAircraftHeading);
    float acPitch = XPLMGetDataf(gAircraftPitch);
    float acRoll = XPLMGetDataf(gAircraftRoll);
    
    // Convert to radians
    float headingRad = acHeading * M_PI / 180.0f;
    float pitchRad = acPitch * M_PI / 180.0f;
    // float rollRad = acRoll * M_PI / 180.0f; // Not used in simplified calculation
    
    // Add FLIR camera angles (relative to aircraft)
    float panRad = gCameraPan * M_PI / 180.0f;
    float tiltRad = gCameraTilt * M_PI / 180.0f;
    
    // Calculate FLIR camera direction vector
    // This is simplified - in reality we'd need full 3D rotation matrices
    float totalHeading = headingRad + panRad;
    float totalPitch = pitchRad + tiltRad;
    
    // Direction vector from camera position
    float dirX = sin(totalHeading) * cos(totalPitch);
    float dirY = sin(totalPitch);
    float dirZ = cos(totalHeading) * cos(totalPitch);
    
    // Cast ray from aircraft position in FLIR direction
    // We'll probe multiple points along the ray to find terrain intersection
    float stepSize = 100.0f; // 100 meter steps
    float maxRange = 50000.0f; // 50km max range
    
    for (float range = stepSize; range <= maxRange; range += stepSize) {
        float probeX = acX + (dirX * range);
        float probeY = acY + (dirY * range);
        float probeZ = acZ + (dirZ * range);
        
        // Probe terrain at this point
        XPLMProbeInfo_t probeInfo;
        probeInfo.structSize = sizeof(XPLMProbeInfo_t);
        
        XPLMProbeResult result = XPLMProbeTerrainXYZ(gTerrainProbe, probeX, probeY, probeZ, &probeInfo);
        
        if (result == xplm_ProbeHitTerrain) {
            // Check if our ray passes close to the terrain at this range
            float terrainY = probeInfo.locationY;
            
            // If our ray Y coordinate is close to terrain Y, we have intersection
            if (fabs(probeY - terrainY) < 50.0f) { // Within 50 meters
                *outX = probeInfo.locationX;
                *outY = probeInfo.locationY;
                *outZ = probeInfo.locationZ;
                
                char msg[256];
                snprintf(msg, sizeof(msg), "TERRAIN PROBE: Hit terrain at (%.2f, %.2f, %.2f) range=%.0fm %s\n",
                    *outX, *outY, *outZ, range, probeInfo.is_wet ? "WATER" : "LAND");
                XPLMDebugString(msg);
                
                return true;
            }
        }
    }
    
    XPLMDebugString("TERRAIN PROBE: No terrain intersection found\n");
    return false;
}

// Precise terrain target designation using probing
static void ProbeTerrainTarget(void* inRefcon)
{
    if (!gCameraActive) {
        XPLMDebugString("TERRAIN PROBE: FLIR camera not active\n");
        return;
    }
    
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
    float maxVelocity = 200.0f; // m/s
    
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
    
    float smoothFactor = 0.2f; // Adjust for guidance responsiveness
    float newVX = currentVX + (desiredVX - currentVX) * smoothFactor;
    float newVY = currentVY + (desiredVY - currentVY) * smoothFactor;
    float newVZ = currentVZ + (desiredVZ - currentVZ) * smoothFactor;
    
    // Set weapon velocity
    XPLMSetDataf(gWeaponVX, newVX);
    XPLMSetDataf(gWeaponVY, newVY);
    XPLMSetDataf(gWeaponVZ, newVZ);
    
    // Debug output every 2 seconds
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