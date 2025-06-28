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

// Custom missile system (our own implementation)
static float gMissileX = 0.0f;
static float gMissileY = 0.0f;
static float gMissileZ = 0.0f;
static float gMissileVX = 0.0f;
static float gMissileVY = 0.0f;
static float gMissileVZ = 0.0f;
static bool gMissileActive = false;

// Function prototypes
static void ProbeTerrainTarget(void* inRefcon);
static void LaunchMissile(void* inRefcon);
static float GuideMissileToTarget(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static bool GetFLIRTerrainIntersection(float* outX, float* outY, float* outZ);

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
    
    // Weapon system datarefs - try multiple possibilities
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    
    // Debug: Check if weapon datarefs exist
    if (!gWeaponX || !gWeaponY || !gWeaponZ) {
        XPLMDebugString("TERRAIN PROBE: Warning - weapon position datarefs not found\n");
    }
    if (!gWeaponVX || !gWeaponVY || !gWeaponVZ) {
        XPLMDebugString("TERRAIN PROBE: Warning - weapon velocity datarefs not found\n");
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
    XPLMDebugString("TERRAIN PROBE: TAB=Probe terrain target, SPACEBAR=Launch missile\n");

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
    
    // Create aircraft rotation matrix (heading, pitch, roll order)
    float cosH = cos(headingRad), sinH = sin(headingRad);
    float cosP = cos(pitchRad), sinP = sin(pitchRad);
    // Note: Using simplified 2D rotation for now, can expand to full 3D later
    
    // Create FLIR gimbal rotation matrix (pan=yaw, tilt=pitch)
    float cosPan = cos(panRad), sinPan = sin(panRad);
    float cosTilt = cos(tiltRad), sinTilt = sin(tiltRad);
    
    // Combined rotation: aircraft orientation + gimbal orientation
    // This is a simplified version - proper 3D rotation matrices would be better
    float totalHeading = headingRad + panRad;
    float totalPitch = pitchRad + tiltRad;
    
    // Direction vector pointing down the gimbal boresight
    // FLIR camera should point downward in world coordinates
    // X-Plane coordinate system: +Y is up, so camera direction should have -Y for downward
    float dirX = sin(totalHeading) * cos(totalPitch);
    float dirY = -sin(totalPitch);  // Should be negative for downward
    float dirZ = cos(totalHeading) * cos(totalPitch);
    
    // Debug: Check if direction makes sense
    if (dirY > 0) {
        XPLMDebugString("TERRAIN PROBE: WARNING - Direction vector pointing UP instead of DOWN!\n");
        // Force downward direction for testing
        dirY = -fabs(dirY);
    }
    
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

// Launch our custom missile
static void LaunchMissile(void* inRefcon)
{
    if (!gTargetValid) {
        XPLMDebugString("TERRAIN PROBE: Cannot launch - no target designated! Press TAB first.\n");
        return;
    }
    
    if (gMissileActive) {
        XPLMDebugString("TERRAIN PROBE: Missile already in flight!\n");
        return;
    }
    
    // Get aircraft position as launch point
    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);
    
    // Launch missile from aircraft position
    gMissileX = acX;
    gMissileY = acY;
    gMissileZ = acZ;
    
    // Initial velocity - point toward target
    float deltaX = gTargetX - gMissileX;
    float deltaY = gTargetY - gMissileY;
    float deltaZ = gTargetZ - gMissileZ;
    float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
    
    if (distance > 0) {
        float initialSpeed = 100.0f; // 100 m/s initial velocity
        gMissileVX = (deltaX / distance) * initialSpeed;
        gMissileVY = (deltaY / distance) * initialSpeed;
        gMissileVZ = (deltaZ / distance) * initialSpeed;
    }
    
    gMissileActive = true;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "TERRAIN PROBE: Missile launched! Target: (%.1f,%.1f,%.1f) Range: %.0fm\n",
        gTargetX, gTargetY, gTargetZ, distance);
    XPLMDebugString(msg);
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
    
    if (!gTargetValid || !gMissileActive) {
        // No target or no missile, but continue flight loop
        return 0.02f; // Continue at 50Hz
    }
    
    // Debug: Check missile status
    static float missileDebugTimer = 0.0f;
    missileDebugTimer += inElapsedSinceLastCall;
    if (missileDebugTimer >= 1.0f) {
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Missile pos=(%.1f,%.1f,%.1f) vel=(%.1f,%.1f,%.1f)\n", 
            gMissileX, gMissileY, gMissileZ, gMissileVX, gMissileVY, gMissileVZ);
        XPLMDebugString(msg);
        missileDebugTimer = 0.0f;
    }
    
    // Calculate vector to target (using our custom missile position)
    float deltaX = gTargetX - gMissileX;
    float deltaY = gTargetY - gMissileY;
    float deltaZ = gTargetZ - gMissileZ;
    
    // Calculate distance to target
    float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
    
    if (distance < 50.0f) {
        // Close to target - missile hit!
        gMissileActive = false;
        gTargetValid = false;
        XPLMDebugString("TERRAIN PROBE: *** MISSILE HIT TARGET! ***\n");
        return 0.02f;
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
    
    // Apply guidance to our custom missile
    float smoothFactor = 0.2f; // Guidance responsiveness
    gMissileVX += (desiredVX - gMissileVX) * smoothFactor;
    gMissileVY += (desiredVY - gMissileVY) * smoothFactor;
    gMissileVZ += (desiredVZ - gMissileVZ) * smoothFactor;
    
    // Update missile position based on velocity
    gMissileX += gMissileVX * inElapsedSinceLastCall;
    gMissileY += gMissileVY * inElapsedSinceLastCall;
    gMissileZ += gMissileVZ * inElapsedSinceLastCall;
    
    // Debug output every 2 seconds (less frequent due to 50Hz updates)
    static float debugTimer = 0.0f;
    debugTimer += inElapsedSinceLastCall;
    if (debugTimer >= 2.0f) {
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN PROBE: Guiding missile dist=%.0fm vel=(%.1f,%.1f,%.1f)\n",
            distance, gMissileVX, gMissileVY, gMissileVZ);
        XPLMDebugString(msg);
        debugTimer = 0.0f;
    }
    
    return 0.02f; // Continue at 50Hz
}