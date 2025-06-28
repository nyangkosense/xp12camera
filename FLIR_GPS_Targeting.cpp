/*
 * FLIR_GPS_Targeting.cpp
 * 
 * Uses X-Plane's built-in GPS targeting system for weapon guidance
 * This should be much more accurate than manual coordinate calculation
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

// Flight loop
static XPLMFlightLoopID gFlightLoopID = NULL;

// FLIR camera datarefs (from main FLIR plugin)
static XPLMDataRef gCameraPanDataRef = NULL;
static XPLMDataRef gCameraTiltDataRef = NULL;
static XPLMDataRef gCameraActiveDataRef = NULL;

// GPS targeting datarefs
static XPLMDataRef gWeaponTargLat = NULL;
static XPLMDataRef gWeaponTargLon = NULL;
static XPLMDataRef gWeaponTargH = NULL;
static XPLMDataRef gWeaponTargX = NULL;
static XPLMDataRef gWeaponTargY = NULL;
static XPLMDataRef gWeaponTargZ = NULL;

// Weapon arrays
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponMode = NULL;
static XPLMDataRef gWeaponRadar = NULL;

// Aircraft position
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;

// Commands
static XPLMCommandRef gGPSLockCommand = NULL;

// State tracking
static bool gTargetLocked = false;
static bool gGuidanceActive = false;
static float gLastTargetX = 0.0f;
static float gLastTargetY = 0.0f;
static float gLastTargetZ = 0.0f;

// Function prototypes
static void LockGPSTarget(void* inRefcon);
static void ActivateGuidance(void* inRefcon);
static void DebugGimbalPosition(void* inRefcon);
static float GuidanceFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void DebugWeaponSystem(void);
static void DebugGPSTargeting(void);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR GPS Targeting System");
    strcpy(outSig, "flir.gps.targeting");
    strcpy(outDesc, "Uses X-Plane GPS lock for weapon guidance");

    // Find FLIR camera datarefs
    gCameraPanDataRef = XPLMFindDataRef("flir/camera/pan");
    gCameraTiltDataRef = XPLMFindDataRef("flir/camera/tilt");
    gCameraActiveDataRef = XPLMFindDataRef("flir/camera/active");
    
    // Find GPS targeting datarefs
    gWeaponTargLat = XPLMFindDataRef("sim/weapons/targ_lat");
    gWeaponTargLon = XPLMFindDataRef("sim/weapons/targ_lon");
    gWeaponTargH = XPLMFindDataRef("sim/weapons/targ_h");
    gWeaponTargX = XPLMFindDataRef("sim/weapons/targ_x");
    gWeaponTargY = XPLMFindDataRef("sim/weapons/targ_y");
    gWeaponTargZ = XPLMFindDataRef("sim/weapons/targ_z");
    
    // Find weapon system datarefs
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponMode = XPLMFindDataRef("sim/weapons/mode");
    gWeaponRadar = XPLMFindDataRef("sim/weapons/radar_on");
    
    // Aircraft position
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    
    // Find GPS lock command
    gGPSLockCommand = XPLMFindCommand("sim/weapons/GPS_lock_here");
    
    // Debug dataref availability
    XPLMDebugString("GPS TARGETING: Checking dataref availability...\\n");
    
    if (!gWeaponTargX || !gWeaponTargY || !gWeaponTargZ) {
        XPLMDebugString("GPS TARGETING: WARNING - Target position datarefs not found!\\n");
    } else {
        XPLMDebugString("GPS TARGETING: Target position datarefs found\\n");
    }
    
    if (!gWeaponX || !gWeaponY || !gWeaponZ) {
        XPLMDebugString("GPS TARGETING: WARNING - Weapon position datarefs not found!\\n");
    } else {
        XPLMDebugString("GPS TARGETING: Weapon position datarefs found\\n");
    }
    
    if (!gWeaponVX || !gWeaponVY || !gWeaponVZ) {
        XPLMDebugString("GPS TARGETING: WARNING - Weapon velocity datarefs not found!\\n");
    } else {
        XPLMDebugString("GPS TARGETING: Weapon velocity datarefs found\\n");
    }
    
    if (!gGPSLockCommand) {
        XPLMDebugString("GPS TARGETING: WARNING - GPS lock command not found!\\n");
    } else {
        XPLMDebugString("GPS TARGETING: GPS lock command found\\n");
    }

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_L, xplm_DownFlag, "GPS: Lock Target", LockGPSTarget, NULL);
    XPLMRegisterHotKey(XPLM_VK_G, xplm_DownFlag, "GPS: Activate Guidance", ActivateGuidance, NULL);
    XPLMRegisterHotKey(XPLM_VK_D, xplm_DownFlag, "GPS: Debug Gimbal", DebugGimbalPosition, NULL);
    
    // Register flight loop
    XPLMCreateFlightLoop_t flightLoopParams;
    flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
    flightLoopParams.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    flightLoopParams.callbackFunc = GuidanceFlightLoop;
    flightLoopParams.refcon = NULL;
    
    gFlightLoopID = XPLMCreateFlightLoop(&flightLoopParams);
    if (gFlightLoopID) {
        XPLMScheduleFlightLoop(gFlightLoopID, 0.1f, 1); // 10Hz
        XPLMDebugString("GPS TARGETING: Flight loop created and scheduled\\n");
    }

    XPLMDebugString("GPS TARGETING: Plugin loaded successfully\\n");
    XPLMDebugString("GPS TARGETING: L=Lock GPS target, G=Activate guidance, D=Debug gimbal\\n");

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gFlightLoopID) {
        XPLMDestroyFlightLoop(gFlightLoopID);
        gFlightLoopID = NULL;
    }
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

// Debug current FLIR gimbal position
static void DebugGimbalPosition(void* inRefcon)
{
    XPLMDebugString("GPS TARGETING: === GIMBAL DEBUG ===\\n");
    
    // Check FLIR camera status
    if (gCameraActiveDataRef) {
        int cameraActive = XPLMGetDatai(gCameraActiveDataRef);
        char msg[256];
        snprintf(msg, sizeof(msg), "GPS TARGETING: FLIR Camera Active: %s\\n", cameraActive ? "YES" : "NO");
        XPLMDebugString(msg);
        
        if (cameraActive) {
            if (gCameraPanDataRef && gCameraTiltDataRef) {
                float pan = XPLMGetDataf(gCameraPanDataRef);
                float tilt = XPLMGetDataf(gCameraTiltDataRef);
                
                snprintf(msg, sizeof(msg), "GPS TARGETING: Gimbal Position - Pan: %.2f°, Tilt: %.2f°\\n", pan, tilt);
                XPLMDebugString(msg);
            } else {
                XPLMDebugString("GPS TARGETING: ERROR - Cannot read gimbal angles\\n");
            }
        }
    } else {
        XPLMDebugString("GPS TARGETING: ERROR - FLIR camera status not available\\n");
    }
    
    // Debug aircraft position
    if (gAircraftX && gAircraftY && gAircraftZ) {
        float acX = XPLMGetDataf(gAircraftX);
        float acY = XPLMGetDataf(gAircraftY);
        float acZ = XPLMGetDataf(gAircraftZ);
        
        char msg[256];
        snprintf(msg, sizeof(msg), "GPS TARGETING: Aircraft Position: (%.1f, %.1f, %.1f)\\n", acX, acY, acZ);
        XPLMDebugString(msg);
    }
    
    XPLMDebugString("GPS TARGETING: === END GIMBAL DEBUG ===\\n");
}

// Use X-Plane's GPS lock system
static void LockGPSTarget(void* inRefcon)
{
    XPLMDebugString("GPS TARGETING: === LOCKING GPS TARGET ===\\n");
    
    // Debug current FLIR gimbal first
    DebugGimbalPosition(NULL);
    
    if (!gGPSLockCommand) {
        XPLMDebugString("GPS TARGETING: ERROR - GPS lock command not available\\n");
        return;
    }
    
    // Execute GPS lock command
    XPLMCommandOnce(gGPSLockCommand);
    XPLMDebugString("GPS TARGETING: GPS lock command executed\\n");
    
    // Give X-Plane a moment to process
    // (We'll read the results in the next flight loop iteration)
    gTargetLocked = true;
    
    XPLMDebugString("GPS TARGETING: Target lock initiated - will read coordinates next flight loop\\n");
}

// Debug weapon system status
static void DebugWeaponSystem(void)
{
    XPLMDebugString("GPS TARGETING: === WEAPON SYSTEM DEBUG ===\\n");
    
    // Check weapon count
    if (gWeaponCount) {
        int weaponCount = XPLMGetDatai(gWeaponCount);
        char msg[256];
        snprintf(msg, sizeof(msg), "GPS TARGETING: Weapon count: %d\\n", weaponCount);
        XPLMDebugString(msg);
    }
    
    // Check weapon positions (first 5 slots)
    if (gWeaponX && gWeaponY && gWeaponZ) {
        float weaponX[5], weaponY[5], weaponZ[5];
        int numRead = XPLMGetDatavf(gWeaponX, weaponX, 0, 5);
        XPLMGetDatavf(gWeaponY, weaponY, 0, 5);
        XPLMGetDatavf(gWeaponZ, weaponZ, 0, 5);
        
        char msg[256];
        snprintf(msg, sizeof(msg), "GPS TARGETING: Read %d weapon positions\\n", numRead);
        XPLMDebugString(msg);
        
        for (int i = 0; i < numRead && i < 5; i++) {
            if (weaponX[i] != 0.0f || weaponY[i] != 0.0f || weaponZ[i] != 0.0f) {
                snprintf(msg, sizeof(msg), "GPS TARGETING: Weapon[%d]: (%.1f, %.1f, %.1f)\\n", 
                    i, weaponX[i], weaponY[i], weaponZ[i]);
                XPLMDebugString(msg);
            }
        }
    }
    
    XPLMDebugString("GPS TARGETING: === END WEAPON DEBUG ===\\n");
}

// Debug GPS targeting system
static void DebugGPSTargeting(void)
{
    XPLMDebugString("GPS TARGETING: === GPS TARGET DEBUG ===\\n");
    
    // Read GPS target coordinates
    if (gWeaponTargX && gWeaponTargY && gWeaponTargZ) {
        float targX = XPLMGetDataf(gWeaponTargX);
        float targY = XPLMGetDataf(gWeaponTargY);
        float targZ = XPLMGetDataf(gWeaponTargZ);
        
        char msg[256];
        snprintf(msg, sizeof(msg), "GPS TARGETING: Target coords (local): (%.2f, %.2f, %.2f)\\n", 
            targX, targY, targZ);
        XPLMDebugString(msg);
        
        // Check if coordinates have changed
        if (targX != gLastTargetX || targY != gLastTargetY || targZ != gLastTargetZ) {
            snprintf(msg, sizeof(msg), "GPS TARGETING: *** TARGET COORDINATES CHANGED! ***\\n");
            XPLMDebugString(msg);
            snprintf(msg, sizeof(msg), "GPS TARGETING: Old: (%.2f, %.2f, %.2f)\\n", 
                gLastTargetX, gLastTargetY, gLastTargetZ);
            XPLMDebugString(msg);
            snprintf(msg, sizeof(msg), "GPS TARGETING: New: (%.2f, %.2f, %.2f)\\n", 
                targX, targY, targZ);
            XPLMDebugString(msg);
            
            gLastTargetX = targX;
            gLastTargetY = targY;
            gLastTargetZ = targZ;
        }
    }
    
    // Read GPS lat/lon/altitude
    if (gWeaponTargLat && gWeaponTargLon && gWeaponTargH) {
        double targLat = XPLMGetDatad(gWeaponTargLat);
        double targLon = XPLMGetDatad(gWeaponTargLon);
        double targH = XPLMGetDatad(gWeaponTargH);
        
        char msg[256];
        snprintf(msg, sizeof(msg), "GPS TARGETING: Target coords (GPS): %.6f°, %.6f°, %.1fm\\n", 
            targLat, targLon, targH);
        XPLMDebugString(msg);
    }
    
    XPLMDebugString("GPS TARGETING: === END GPS DEBUG ===\\n");
}

// Activate weapon guidance to GPS target
static void ActivateGuidance(void* inRefcon)
{
    XPLMDebugString("GPS TARGETING: === ACTIVATING GUIDANCE ===\\n");
    
    if (!gTargetLocked) {
        XPLMDebugString("GPS TARGETING: ERROR - No GPS target locked! Press L first.\\n");
        return;
    }
    
    // Debug current system state
    DebugGPSTargeting();
    DebugWeaponSystem();
    
    gGuidanceActive = true;
    XPLMDebugString("GPS TARGETING: Guidance activated!\\n");
}

// Flight loop for weapon guidance
static float GuidanceFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    static float debugTimer = 0.0f;
    debugTimer += inElapsedSinceLastCall;
    
    // Debug GPS targeting every 3 seconds
    if (debugTimer >= 3.0f) {
        if (gTargetLocked) {
            DebugGPSTargeting();
        }
        
        if (gGuidanceActive) {
            DebugWeaponSystem();
        }
        
        debugTimer = 0.0f;
    }
    
    // If guidance is active, guide weapons to GPS target
    if (gGuidanceActive && gTargetLocked) {
        // Get target coordinates
        if (gWeaponTargX && gWeaponTargY && gWeaponTargZ) {
            float targX = XPLMGetDataf(gWeaponTargX);
            float targY = XPLMGetDataf(gWeaponTargY);
            float targZ = XPLMGetDataf(gWeaponTargZ);
            
            // Check if we have a valid target
            if (targX != 0.0f || targY != 0.0f || targZ != 0.0f) {
                // Guide weapons (implement simple proportional navigation)
                if (gWeaponX && gWeaponY && gWeaponZ && gWeaponVX && gWeaponVY && gWeaponVZ) {
                    float weaponX[10], weaponY[10], weaponZ[10];
                    float weaponVX[10], weaponVY[10], weaponVZ[10];
                    
                    int numRead = XPLMGetDatavf(gWeaponX, weaponX, 0, 10);
                    XPLMGetDatavf(gWeaponY, weaponY, 0, 10);
                    XPLMGetDatavf(gWeaponZ, weaponZ, 0, 10);
                    XPLMGetDatavf(gWeaponVX, weaponVX, 0, 10);
                    XPLMGetDatavf(gWeaponVY, weaponVY, 0, 10);
                    XPLMGetDatavf(gWeaponVZ, weaponVZ, 0, 10);
                    
                    bool foundWeapon = false;
                    
                    for (int i = 0; i < numRead; i++) {
                        // Check if weapon exists
                        if (weaponX[i] != 0.0f || weaponY[i] != 0.0f || weaponZ[i] != 0.0f) {
                            foundWeapon = true;
                            
                            // Calculate guidance vector
                            float deltaX = targX - weaponX[i];
                            float deltaY = targY - weaponY[i];
                            float deltaZ = targZ - weaponZ[i];
                            float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
                            
                            if (distance > 10.0f) { // Not yet at target
                                // Proportional navigation
                                float maxSpeed = 150.0f; // m/s
                                float dirX = deltaX / distance;
                                float dirY = deltaY / distance;
                                float dirZ = deltaZ / distance;
                                
                                float desiredVX = dirX * maxSpeed;
                                float desiredVY = dirY * maxSpeed;
                                float desiredVZ = dirZ * maxSpeed;
                                
                                // Apply smoothing
                                float smoothFactor = 0.3f;
                                weaponVX[i] = weaponVX[i] + (desiredVX - weaponVX[i]) * smoothFactor;
                                weaponVY[i] = weaponVY[i] + (desiredVY - weaponVY[i]) * smoothFactor;
                                weaponVZ[i] = weaponVZ[i] + (desiredVZ - weaponVZ[i]) * smoothFactor;
                                
                                // Debug guidance
                                static float guidanceDebugTimer = 0.0f;
                                guidanceDebugTimer += inElapsedSinceLastCall;
                                if (guidanceDebugTimer >= 2.0f) {
                                    char msg[256];
                                    snprintf(msg, sizeof(msg), 
                                        "GPS TARGETING: Guiding weapon[%d] dist=%.0fm vel=(%.1f,%.1f,%.1f)\\n",
                                        i, distance, weaponVX[i], weaponVY[i], weaponVZ[i]);
                                    XPLMDebugString(msg);
                                    guidanceDebugTimer = 0.0f;
                                }
                            } else {
                                // Close to target
                                char msg[128];
                                snprintf(msg, sizeof(msg), "GPS TARGETING: Weapon[%d] hit target!\\n", i);
                                XPLMDebugString(msg);
                            }
                        }
                    }
                    
                    // Update weapon velocities
                    XPLMSetDatavf(gWeaponVX, weaponVX, 0, numRead);
                    XPLMSetDatavf(gWeaponVY, weaponVY, 0, numRead);
                    XPLMSetDatavf(gWeaponVZ, weaponVZ, 0, numRead);
                    
                    if (!foundWeapon) {
                        static float noWeaponTimer = 0.0f;
                        noWeaponTimer += inElapsedSinceLastCall;
                        if (noWeaponTimer >= 5.0f) {
                            XPLMDebugString("GPS TARGETING: No weapons in flight\\n");
                            noWeaponTimer = 0.0f;
                        }
                    }
                }
            }
        }
    }
    
    return 0.1f; // Continue at 10Hz
}