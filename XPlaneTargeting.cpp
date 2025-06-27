/*
 * XPlaneTargeting.cpp
 * 
 * Try to use X-Plane's built-in targeting system
 * Trigger X-Plane's own F3 targeting and read the results
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMMenus.h"

// X-Plane's weapon targeting commands
static XPLMCommandRef gGPSLockCommand = NULL;
static XPLMCommandRef gTargetHereCommand = NULL;
static XPLMCommandRef gFireCommand = NULL;

// X-Plane's targeting datarefs
static XPLMDataRef gWeaponTargLat = NULL;
static XPLMDataRef gWeaponTargLon = NULL;
static XPLMDataRef gWeaponTargH = NULL;
static XPLMDataRef gWeaponTargX = NULL;
static XPLMDataRef gWeaponTargY = NULL;
static XPLMDataRef gWeaponTargZ = NULL;

// Monitor targeting state
static int gMonitoringActive = 0;
static XPLMFlightLoopID gMonitorLoop = NULL;

// Test functions
static void TriggerXPlaneF3Callback(void* inRefcon);
static void StartMonitoringCallback(void* inRefcon);
static void StopMonitoringCallback(void* inRefcon);
static void ReadCurrentTargetCallback(void* inRefcon);
static float MonitoringFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "X-Plane Targeting Test");
    strcpy(outSig, "xplane.targeting.test");
    strcpy(outDesc, "Try to use X-Plane's built-in targeting system");

    // Find X-Plane's targeting commands
    gGPSLockCommand = XPLMFindCommand("sim/weapons/GPS_lock_here");
    gTargetHereCommand = XPLMFindCommand("sim/weapons/target_here");
    gFireCommand = XPLMFindCommand("sim/weapons/fire_any_armed");
    
    // Find X-Plane's targeting datarefs
    gWeaponTargLat = XPLMFindDataRef("sim/weapons/targ_lat");
    gWeaponTargLon = XPLMFindDataRef("sim/weapons/targ_lon");
    gWeaponTargH = XPLMFindDataRef("sim/weapons/targ_h");
    gWeaponTargX = XPLMFindDataRef("sim/weapons/targ_x");
    gWeaponTargY = XPLMFindDataRef("sim/weapons/targ_y");
    gWeaponTargZ = XPLMFindDataRef("sim/weapons/targ_z");

    // Register test hotkeys
    XPLMRegisterHotKey(XPLM_VK_F3, xplm_DownFlag, "XP: Trigger F3 Target", TriggerXPlaneF3Callback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F5, xplm_DownFlag, "XP: Start Monitoring", StartMonitoringCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F6, xplm_DownFlag, "XP: Stop Monitoring", StopMonitoringCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F7, xplm_DownFlag, "XP: Read Current Target", ReadCurrentTargetCallback, NULL);

    XPLMDebugString("X-PLANE TARGETING: Plugin loaded\n");
    XPLMDebugString("X-PLANE TARGETING: F3=Trigger X-Plane targeting, F5=Start monitoring, F6=Stop, F7=Read target\n");
    XPLMDebugString("X-PLANE TARGETING: Point camera and press F3 to see if X-Plane sets target datarefs\n");

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gMonitoringActive && gMonitorLoop) {
        XPLMScheduleFlightLoop(gMonitorLoop, 0, 0);
    }
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void TriggerXPlaneF3Callback(void* inRefcon)
{
    char msg[512];
    
    // Try triggering X-Plane's built-in targeting commands
    if (gGPSLockCommand) {
        XPLMCommandOnce(gGPSLockCommand);
        snprintf(msg, sizeof(msg), "X-PLANE TARGETING: Triggered GPS_lock_here command\n");
    } else if (gTargetHereCommand) {
        XPLMCommandOnce(gTargetHereCommand);
        snprintf(msg, sizeof(msg), "X-PLANE TARGETING: Triggered target_here command\n");
    } else {
        snprintf(msg, sizeof(msg), "X-PLANE TARGETING: No targeting commands found\n");
    }
    
    XPLMDebugString(msg);
    
    // Read the result immediately
    ReadCurrentTargetCallback(NULL);
}

static void StartMonitoringCallback(void* inRefcon)
{
    if (gMonitoringActive) {
        XPLMDebugString("X-PLANE TARGETING: Monitoring already active\n");
        return;
    }
    
    gMonitoringActive = 1;
    
    if (!gMonitorLoop) {
        XPLMCreateFlightLoop_t flightLoopParams;
        flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
        flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
        flightLoopParams.callbackFunc = MonitoringFlightLoopCallback;
        flightLoopParams.refcon = NULL;
        
        gMonitorLoop = XPLMCreateFlightLoop(&flightLoopParams);
    }
    
    if (gMonitorLoop) {
        XPLMScheduleFlightLoop(gMonitorLoop, 1.0f, 1); // Check every second
        XPLMDebugString("X-PLANE TARGETING: Started monitoring X-Plane target datarefs\n");
    }
}

static void StopMonitoringCallback(void* inRefcon)
{
    if (!gMonitoringActive) {
        XPLMDebugString("X-PLANE TARGETING: Monitoring not active\n");
        return;
    }
    
    gMonitoringActive = 0;
    if (gMonitorLoop) {
        XPLMScheduleFlightLoop(gMonitorLoop, 0, 0);
        XPLMDebugString("X-PLANE TARGETING: Stopped monitoring\n");
    }
}

static void ReadCurrentTargetCallback(void* inRefcon)
{
    char msg[512];
    
    // Read all available target datarefs
    double targLat = gWeaponTargLat ? XPLMGetDatad(gWeaponTargLat) : 0.0;
    double targLon = gWeaponTargLon ? XPLMGetDatad(gWeaponTargLon) : 0.0;
    double targH = gWeaponTargH ? XPLMGetDatad(gWeaponTargH) : 0.0;
    
    float targX = gWeaponTargX ? XPLMGetDataf(gWeaponTargX) : 0.0f;
    float targY = gWeaponTargY ? XPLMGetDataf(gWeaponTargY) : 0.0f;
    float targZ = gWeaponTargZ ? XPLMGetDataf(gWeaponTargZ) : 0.0f;
    
    // Check if any values are non-zero
    int hasValidTarget = (targLat != 0.0 || targLon != 0.0 || targH != 0.0 || 
                         targX != 0.0f || targY != 0.0f || targZ != 0.0f);
    
    if (hasValidTarget) {
        snprintf(msg, sizeof(msg), 
            "X-PLANE TARGETING: TARGET FOUND!\n"
            "X-PLANE TARGETING: GPS: lat=%.6f, lon=%.6f, h=%.1f\n"
            "X-PLANE TARGETING: Local: x=%.0f, y=%.0f, z=%.0f\n"
            "X-PLANE TARGETING: This is X-Plane's target coordinates!\n",
            targLat, targLon, targH, targX, targY, targZ);
    } else {
        snprintf(msg, sizeof(msg), 
            "X-PLANE TARGETING: No target found - all datarefs are zero\n"
            "X-PLANE TARGETING: Try using X-Plane's built-in F3 targeting first\n");
    }
    
    XPLMDebugString(msg);
}

static float MonitoringFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gMonitoringActive) return 0;
    
    // Check for target changes
    static float lastTargX = 0.0f;
    static float lastTargY = 0.0f;
    static float lastTargZ = 0.0f;
    
    float targX = gWeaponTargX ? XPLMGetDataf(gWeaponTargX) : 0.0f;
    float targY = gWeaponTargY ? XPLMGetDataf(gWeaponTargY) : 0.0f;
    float targZ = gWeaponTargZ ? XPLMGetDataf(gWeaponTargZ) : 0.0f;
    
    // Check if target changed
    if (targX != lastTargX || targY != lastTargY || targZ != lastTargZ) {
        if (targX != 0.0f || targY != 0.0f || targZ != 0.0f) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "X-PLANE TARGETING: TARGET CHANGED to (%.0f, %.0f, %.0f)\n",
                targX, targY, targZ);
            XPLMDebugString(msg);
        }
        
        lastTargX = targX;
        lastTargY = targY;
        lastTargZ = targZ;
    }
    
    return 1.0f; // Check again in 1 second
}