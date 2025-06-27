/*
 * WeaponTargetTest.cpp
 * 
 * Test X-Plane's built-in weapon targeting datarefs
 * See what X-Plane provides when it does its own targeting
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// X-Plane weapon targeting datarefs
static XPLMDataRef gWeaponTargLat = NULL;
static XPLMDataRef gWeaponTargLon = NULL;
static XPLMDataRef gWeaponTargH = NULL;
static XPLMDataRef gWeaponTargX = NULL;
static XPLMDataRef gWeaponTargY = NULL;
static XPLMDataRef gWeaponTargZ = NULL;

// GPS targeting datarefs
static XPLMDataRef gGPSDestLat = NULL;
static XPLMDataRef gGPSDestLon = NULL;
static XPLMDataRef gGPSDestH = NULL;

// Aircraft position for conversion
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftLat = NULL;
static XPLMDataRef gAircraftLon = NULL;

// Test functions
static void TestWeaponTargetsCallback(void* inRefcon);
static void TestGPSTargetsCallback(void* inRefcon);
static void TestAllTargetingCallback(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Weapon Target Test");
    strcpy(outSig, "weapon.target.test");
    strcpy(outDesc, "Test X-Plane's built-in weapon targeting datarefs");

    // Initialize weapon targeting datarefs
    gWeaponTargLat = XPLMFindDataRef("sim/weapons/targ_lat");
    gWeaponTargLon = XPLMFindDataRef("sim/weapons/targ_lon");
    gWeaponTargH = XPLMFindDataRef("sim/weapons/targ_h");
    gWeaponTargX = XPLMFindDataRef("sim/weapons/targ_x");
    gWeaponTargY = XPLMFindDataRef("sim/weapons/targ_y");
    gWeaponTargZ = XPLMFindDataRef("sim/weapons/targ_z");
    
    // GPS targeting datarefs
    gGPSDestLat = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_latitude_deg");
    gGPSDestLon = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_longitude_deg");
    gGPSDestH = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_altitude_m");
    
    // Aircraft position
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftLat = XPLMFindDataRef("sim/flightmodel/position/latitude");
    gAircraftLon = XPLMFindDataRef("sim/flightmodel/position/longitude");

    // Register test hotkeys
    XPLMRegisterHotKey(XPLM_VK_F6, xplm_DownFlag, "Test: Weapon Targets", TestWeaponTargetsCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F7, xplm_DownFlag, "Test: GPS Targets", TestGPSTargetsCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F12, xplm_DownFlag, "Test: All Targeting", TestAllTargetingCallback, NULL);

    XPLMDebugString("WEAPON TARGET TEST: Plugin loaded\n");
    XPLMDebugString("WEAPON TARGET TEST: F6=Weapon Targets, F7=GPS, F12=All\n");
    XPLMDebugString("WEAPON TARGET TEST: Try X-Plane's built-in F3 targeting first\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void TestWeaponTargetsCallback(void* inRefcon)
{
    char msg[512];
    
    // Test weapon targeting datarefs
    double targLat = gWeaponTargLat ? XPLMGetDatad(gWeaponTargLat) : 0.0;
    double targLon = gWeaponTargLon ? XPLMGetDatad(gWeaponTargLon) : 0.0;
    double targH = gWeaponTargH ? XPLMGetDatad(gWeaponTargH) : 0.0;
    
    float targX = gWeaponTargX ? XPLMGetDataf(gWeaponTargX) : 0.0f;
    float targY = gWeaponTargY ? XPLMGetDataf(gWeaponTargY) : 0.0f;
    float targZ = gWeaponTargZ ? XPLMGetDataf(gWeaponTargZ) : 0.0f;
    
    snprintf(msg, sizeof(msg), 
        "WEAPON TARGET TEST: X-PLANE WEAPON TARGETING\n"
        "WEAPON TARGET TEST: targ_lat/lon/h: %.6f, %.6f, %.1f\n"
        "WEAPON TARGET TEST: targ_x/y/z: %.0f, %.0f, %.0f\n"
        "WEAPON TARGET TEST: Use X-Plane's F3 to set target first!\n",
        targLat, targLon, targH, targX, targY, targZ);
    
    XPLMDebugString(msg);
}

static void TestGPSTargetsCallback(void* inRefcon)
{
    char msg[512];
    
    // Test GPS targeting datarefs
    double gpsLat = gGPSDestLat ? XPLMGetDatad(gGPSDestLat) : 0.0;
    double gpsLon = gGPSDestLon ? XPLMGetDatad(gGPSDestLon) : 0.0;
    double gpsH = gGPSDestH ? XPLMGetDatad(gGPSDestH) : 0.0;
    
    snprintf(msg, sizeof(msg), 
        "WEAPON TARGET TEST: GPS DESTINATION\n"
        "WEAPON TARGET TEST: GPS dest: %.6f, %.6f, %.1f\n"
        "WEAPON TARGET TEST: This is GPS waypoint destination\n",
        gpsLat, gpsLon, gpsH);
    
    XPLMDebugString(msg);
}

static void TestAllTargetingCallback(void* inRefcon)
{
    char msg[1024];
    
    // Get all available targeting data
    double weapLat = gWeaponTargLat ? XPLMGetDatad(gWeaponTargLat) : 0.0;
    double weapLon = gWeaponTargLon ? XPLMGetDatad(gWeaponTargLon) : 0.0;
    double weapH = gWeaponTargH ? XPLMGetDatad(gWeaponTargH) : 0.0;
    
    float weapX = gWeaponTargX ? XPLMGetDataf(gWeaponTargX) : 0.0f;
    float weapY = gWeaponTargY ? XPLMGetDataf(gWeaponTargY) : 0.0f;
    float weapZ = gWeaponTargZ ? XPLMGetDataf(gWeaponTargZ) : 0.0f;
    
    float aircraftX = gAircraftX ? XPLMGetDataf(gAircraftX) : 0.0f;
    float aircraftY = gAircraftY ? XPLMGetDataf(gAircraftY) : 0.0f;
    float aircraftZ = gAircraftZ ? XPLMGetDataf(gAircraftZ) : 0.0f;
    
    double aircraftLat = gAircraftLat ? XPLMGetDatad(gAircraftLat) : 0.0;
    double aircraftLon = gAircraftLon ? XPLMGetDatad(gAircraftLon) : 0.0;
    
    snprintf(msg, sizeof(msg), 
        "WEAPON TARGET TEST: COMPLETE TARGETING DATA\n"
        "WEAPON TARGET TEST: Aircraft Local: (%.0f, %.0f, %.0f)\n"
        "WEAPON TARGET TEST: Aircraft GPS: (%.6f, %.6f)\n"
        "WEAPON TARGET TEST: Weapon Target Local: (%.0f, %.0f, %.0f)\n"
        "WEAPON TARGET TEST: Weapon Target GPS: (%.6f, %.6f, %.1f)\n"
        "WEAPON TARGET TEST: Distance: %.0f meters\n",
        aircraftX, aircraftY, aircraftZ,
        aircraftLat, aircraftLon,
        weapX, weapY, weapZ,
        weapLat, weapLon, weapH,
        sqrt((weapX-aircraftX)*(weapX-aircraftX) + (weapY-aircraftY)*(weapY-aircraftY) + (weapZ-aircraftZ)*(weapZ-aircraftZ)));
    
    XPLMDebugString(msg);
}