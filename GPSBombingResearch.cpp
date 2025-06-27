/*
 * GPSBombingResearch.cpp
 * 
 * Research how X-Plane's existing GPS bombing systems work
 * Find the datarefs and commands used by military aircraft
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// GPS and navigation datarefs
static XPLMDataRef gGPSDestLat = NULL;
static XPLMDataRef gGPSDestLon = NULL;
static XPLMDataRef gGPSDestAlt = NULL;
static XPLMDataRef gGPSDestId = NULL;
static XPLMDataRef gGPSMode = NULL;
static XPLMDataRef gGPSNav1 = NULL;
static XPLMDataRef gGPSNav2 = NULL;

// FMS/autopilot datarefs that might be used for targeting
static XPLMDataRef gFMSLat = NULL;
static XPLMDataRef gFMSLon = NULL;
static XPLMDataRef gFMSAlt = NULL;
static XPLMDataRef gFMSActive = NULL;

// Military targeting datarefs
static XPLMDataRef gMilitaryTargetLat = NULL;
static XPLMDataRef gMilitaryTargetLon = NULL;
static XPLMDataRef gMilitaryTargetAlt = NULL;
static XPLMDataRef gTargetBearing = NULL;
static XPLMDataRef gTargetDistance = NULL;

// Weapon system datarefs
static XPLMDataRef gWeaponMode = NULL;
static XPLMDataRef gWeaponSelector = NULL;
static XPLMDataRef gBombingMode = NULL;
static XPLMDataRef gGuidanceMode = NULL;

// Aircraft type
static XPLMDataRef gAircraftICAO = NULL;
static XPLMDataRef gAircraftName = NULL;

static void ResearchGPSSystemCallback(void* inRefcon);
static void ResearchFMSSystemCallback(void* inRefcon);
static void ResearchMilitarySystemCallback(void* inRefcon);
static void ResearchWeaponSystemCallback(void* inRefcon);
static void ResearchAircraftTypeCallback(void* inRefcon);
static void SetGPSDestinationCallback(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "GPS Bombing Research");
    strcpy(outSig, "gps.bombing.research");
    strcpy(outDesc, "Research how X-Plane's GPS bombing systems work");

    // Initialize GPS datarefs
    gGPSDestLat = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_latitude_deg");
    gGPSDestLon = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_longitude_deg");
    gGPSDestAlt = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_altitude_m");
    gGPSDestId = XPLMFindDataRef("sim/cockpit2/radios/indicators/gps_dme_id");
    gGPSMode = XPLMFindDataRef("sim/cockpit2/radios/actuators/gps_power");
    gGPSNav1 = XPLMFindDataRef("sim/cockpit2/radios/actuators/nav1_frequency_hz");
    gGPSNav2 = XPLMFindDataRef("sim/cockpit2/radios/actuators/nav2_frequency_hz");
    
    // Initialize FMS datarefs
    gFMSLat = XPLMFindDataRef("sim/cockpit2/radios/indicators/fms_latitude_deg");
    gFMSLon = XPLMFindDataRef("sim/cockpit2/radios/indicators/fms_longitude_deg");
    gFMSAlt = XPLMFindDataRef("sim/cockpit2/radios/indicators/fms_altitude_ft");
    gFMSActive = XPLMFindDataRef("sim/cockpit2/radios/actuators/fms_power");
    
    // Try to find military targeting datarefs
    gMilitaryTargetLat = XPLMFindDataRef("sim/weapons/target_latitude");
    gMilitaryTargetLon = XPLMFindDataRef("sim/weapons/target_longitude");
    gMilitaryTargetAlt = XPLMFindDataRef("sim/weapons/target_altitude");
    gTargetBearing = XPLMFindDataRef("sim/weapons/target_bearing");
    gTargetDistance = XPLMFindDataRef("sim/weapons/target_distance");
    
    // Weapon system datarefs
    gWeaponMode = XPLMFindDataRef("sim/weapons/weapon_mode");
    gWeaponSelector = XPLMFindDataRef("sim/weapons/weapon_selector");
    gBombingMode = XPLMFindDataRef("sim/weapons/bombing_mode");
    gGuidanceMode = XPLMFindDataRef("sim/weapons/guidance_mode");
    
    // Aircraft identification
    gAircraftICAO = XPLMFindDataRef("sim/aircraft/view/acf_ICAO");
    gAircraftName = XPLMFindDataRef("sim/aircraft/view/acf_descrip");

    // Register research hotkeys
    XPLMRegisterHotKey(XPLM_VK_F1, xplm_DownFlag, "Research: GPS System", ResearchGPSSystemCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F2, xplm_DownFlag, "Research: FMS System", ResearchFMSSystemCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F3, xplm_DownFlag, "Research: Military System", ResearchMilitarySystemCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F4, xplm_DownFlag, "Research: Weapon System", ResearchWeaponSystemCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F5, xplm_DownFlag, "Research: Aircraft Type", ResearchAircraftTypeCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F6, xplm_DownFlag, "Test: Set GPS Destination", SetGPSDestinationCallback, NULL);

    XPLMDebugString("GPS BOMBING RESEARCH: Plugin loaded\n");
    XPLMDebugString("GPS BOMBING RESEARCH: F1=GPS, F2=FMS, F3=Military, F4=Weapon, F5=Aircraft, F6=Set GPS\n");
    XPLMDebugString("GPS BOMBING RESEARCH: Load a military aircraft with GPS bombing capability\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void ResearchGPSSystemCallback(void* inRefcon)
{
    char msg[512];
    
    double gpsLat = gGPSDestLat ? XPLMGetDatad(gGPSDestLat) : 0.0;
    double gpsLon = gGPSDestLon ? XPLMGetDatad(gGPSDestLon) : 0.0;
    double gpsAlt = gGPSDestAlt ? XPLMGetDatad(gGPSDestAlt) : 0.0;
    int gpsMode = gGPSMode ? XPLMGetDatai(gGPSMode) : 0;
    
    snprintf(msg, sizeof(msg), 
        "GPS RESEARCH: GPS SYSTEM STATUS\n"
        "GPS RESEARCH: GPS Destination: %.6f, %.6f, %.1fm\n"
        "GPS RESEARCH: GPS Mode/Power: %d\n"
        "GPS RESEARCH: GPS ID available: %s\n",
        gpsLat, gpsLon, gpsAlt, gpsMode,
        gGPSDestId ? "YES" : "NO");
    
    XPLMDebugString(msg);
}

static void ResearchFMSSystemCallback(void* inRefcon)
{
    char msg[512];
    
    double fmsLat = gFMSLat ? XPLMGetDatad(gFMSLat) : 0.0;
    double fmsLon = gFMSLon ? XPLMGetDatad(gFMSLon) : 0.0;
    double fmsAlt = gFMSAlt ? XPLMGetDatad(gFMSAlt) : 0.0;
    int fmsActive = gFMSActive ? XPLMGetDatai(gFMSActive) : 0;
    
    snprintf(msg, sizeof(msg), 
        "GPS RESEARCH: FMS SYSTEM STATUS\n"
        "GPS RESEARCH: FMS Position: %.6f, %.6f, %.1fft\n"
        "GPS RESEARCH: FMS Active: %d\n",
        fmsLat, fmsLon, fmsAlt, fmsActive);
    
    XPLMDebugString(msg);
}

static void ResearchMilitarySystemCallback(void* inRefcon)
{
    char msg[512];
    
    double milLat = gMilitaryTargetLat ? XPLMGetDatad(gMilitaryTargetLat) : 0.0;
    double milLon = gMilitaryTargetLon ? XPLMGetDatad(gMilitaryTargetLon) : 0.0;
    double milAlt = gMilitaryTargetAlt ? XPLMGetDatad(gMilitaryTargetAlt) : 0.0;
    float bearing = gTargetBearing ? XPLMGetDataf(gTargetBearing) : 0.0f;
    float distance = gTargetDistance ? XPLMGetDataf(gTargetDistance) : 0.0f;
    
    snprintf(msg, sizeof(msg), 
        "GPS RESEARCH: MILITARY TARGETING SYSTEM\n"
        "GPS RESEARCH: Military Target: %.6f, %.6f, %.1fm\n"
        "GPS RESEARCH: Target Bearing: %.1f°, Distance: %.1fm\n"
        "GPS RESEARCH: Available datarefs: lat=%s, lon=%s, alt=%s\n",
        milLat, milLon, milAlt, bearing, distance,
        gMilitaryTargetLat ? "YES" : "NO",
        gMilitaryTargetLon ? "YES" : "NO",
        gMilitaryTargetAlt ? "YES" : "NO");
    
    XPLMDebugString(msg);
}

static void ResearchWeaponSystemCallback(void* inRefcon)
{
    char msg[512];
    
    int weaponMode = gWeaponMode ? XPLMGetDatai(gWeaponMode) : -1;
    int weaponSelector = gWeaponSelector ? XPLMGetDatai(gWeaponSelector) : -1;
    int bombingMode = gBombingMode ? XPLMGetDatai(gBombingMode) : -1;
    int guidanceMode = gGuidanceMode ? XPLMGetDatai(gGuidanceMode) : -1;
    
    snprintf(msg, sizeof(msg), 
        "GPS RESEARCH: WEAPON SYSTEM MODES\n"
        "GPS RESEARCH: Weapon Mode: %d (available: %s)\n"
        "GPS RESEARCH: Weapon Selector: %d (available: %s)\n"
        "GPS RESEARCH: Bombing Mode: %d (available: %s)\n"
        "GPS RESEARCH: Guidance Mode: %d (available: %s)\n",
        weaponMode, gWeaponMode ? "YES" : "NO",
        weaponSelector, gWeaponSelector ? "YES" : "NO",
        bombingMode, gBombingMode ? "YES" : "NO",
        guidanceMode, gGuidanceMode ? "YES" : "NO");
    
    XPLMDebugString(msg);
}

static void ResearchAircraftTypeCallback(void* inRefcon)
{
    char msg[512];
    char icao[16] = "";
    char name[256] = "";
    
    if (gAircraftICAO) {
        XPLMGetDatab(gAircraftICAO, icao, 0, 15);
        icao[15] = '\0';
    }
    
    if (gAircraftName) {
        XPLMGetDatab(gAircraftName, name, 0, 255);
        name[255] = '\0';
    }
    
    snprintf(msg, sizeof(msg), 
        "GPS RESEARCH: AIRCRAFT INFORMATION\n"
        "GPS RESEARCH: ICAO: '%s'\n"
        "GPS RESEARCH: Name: '%s'\n"
        "GPS RESEARCH: Try loading F-16, F/A-18, A-10, or other military aircraft\n",
        icao, name);
    
    XPLMDebugString(msg);
}

static void SetGPSDestinationCallback(void* inRefcon)
{
    // Try to set GPS destination to current camera position
    if (gGPSDestLat && gGPSDestLon) {
        // For now, set a test coordinate
        XPLMSetDatad(gGPSDestLat, 40.7128);   // New York latitude
        XPLMSetDatad(gGPSDestLon, -74.0060);  // New York longitude
        
        XPLMDebugString("GPS RESEARCH: Set GPS destination to New York (test)\n");
        XPLMDebugString("GPS RESEARCH: Check if weapons can now target this GPS coordinate\n");
    } else {
        XPLMDebugString("GPS RESEARCH: GPS destination datarefs not available\n");
    }
}