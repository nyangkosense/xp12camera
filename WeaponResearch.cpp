/*
 * WeaponResearch.cpp
 * 
 * Experimental plugin to understand X-Plane 12 weapon system
 * Tests all weapon datarefs to find which ones actually control missiles
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
#include "XPLMMenus.h"

// All weapon datarefs for testing
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponType = NULL;
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;
static XPLMDataRef gWeaponTargLat = NULL;
static XPLMDataRef gWeaponTargLon = NULL;
static XPLMDataRef gWeaponTargH = NULL;
static XPLMDataRef gWeaponTargetIndex = NULL;
static XPLMDataRef gWeaponDistTarg = NULL;
static XPLMDataRef gWeaponDistPoint = NULL;
static XPLMDataRef gWeaponElevRat = NULL;
static XPLMDataRef gWeaponAzimRat = NULL;
static XPLMDataRef gWeaponSFrn = NULL;
static XPLMDataRef gWeaponSSid = NULL;
static XPLMDataRef gWeaponSTop = NULL;
static XPLMDataRef gWeaponThecon = NULL;
static XPLMDataRef gWeaponThe = NULL;
static XPLMDataRef gWeaponTimePoint = NULL;
static XPLMDataRef gWeaponXBodyAero = NULL;
static XPLMDataRef gWeaponYBodyAero = NULL;
static XPLMDataRef gWeaponZBodyAero = NULL;

// Test control variables
static int gResearchActive = 0;
static int gTestMode = 0;
static float gTestValue = 0.0f;
static int gCurrentWeaponIndex = 0;
static XPLMFlightLoopID gResearchFlightLoop = NULL;

// Function declarations
static void ActivateResearchCallback(void* inRefcon);
static void NextTestModeCallback(void* inRefcon);
static void IncreaseTestValueCallback(void* inRefcon);
static void DecreaseTestValueCallback(void* inRefcon);
static void NextWeaponCallback(void* inRefcon);
static float ResearchFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void LogAllWeaponData();
static void TestCurrentDataref();

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Weapon Research Plugin");
    strcpy(outSig, "weaponresearch.experimental");
    strcpy(outDesc, "Experimental plugin to research X-Plane weapon datarefs");

    // Initialize all weapon datarefs
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponType = XPLMFindDataRef("sim/weapons/type");
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    gWeaponTargLat = XPLMFindDataRef("sim/weapons/targ_lat");
    gWeaponTargLon = XPLMFindDataRef("sim/weapons/targ_lon");
    gWeaponTargH = XPLMFindDataRef("sim/weapons/targ_h");
    gWeaponTargetIndex = XPLMFindDataRef("sim/weapons/target_index");
    gWeaponDistTarg = XPLMFindDataRef("sim/weapons/dist_targ");
    gWeaponDistPoint = XPLMFindDataRef("sim/weapons/dist_point");
    gWeaponElevRat = XPLMFindDataRef("sim/weapons/elev_rat");
    gWeaponAzimRat = XPLMFindDataRef("sim/weapons/azim_rat");
    gWeaponSFrn = XPLMFindDataRef("sim/weapons/s_frn");
    gWeaponSSid = XPLMFindDataRef("sim/weapons/s_sid");
    gWeaponSTop = XPLMFindDataRef("sim/weapons/s_top");
    gWeaponThecon = XPLMFindDataRef("sim/weapons/the_con");
    gWeaponThe = XPLMFindDataRef("sim/weapons/the");
    gWeaponTimePoint = XPLMFindDataRef("sim/weapons/time_point");
    gWeaponXBodyAero = XPLMFindDataRef("sim/weapons/X_body_aero");
    gWeaponYBodyAero = XPLMFindDataRef("sim/weapons/Y_body_aero");
    gWeaponZBodyAero = XPLMFindDataRef("sim/weapons/Z_body_aero");

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_F10, xplm_DownFlag, "WR: Activate Research", ActivateResearchCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F11, xplm_DownFlag, "WR: Next Test Mode", NextTestModeCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F12, xplm_DownFlag, "WR: Next Weapon", NextWeaponCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_COMMA, xplm_DownFlag, "WR: Decrease Value", DecreaseTestValueCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_PERIOD, xplm_DownFlag, "WR: Increase Value", IncreaseTestValueCallback, NULL);

    XPLMDebugString("WEAPON RESEARCH: Plugin loaded\n");
    XPLMDebugString("WEAPON RESEARCH: F10=Start/Stop, F11=Next Test Mode, F12=Next Weapon, ,/. = Dec/Inc Value\n");

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gResearchActive && gResearchFlightLoop) {
        XPLMScheduleFlightLoop(gResearchFlightLoop, 0, 0);
    }
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void ActivateResearchCallback(void* inRefcon)
{
    if (!gResearchActive) {
        gResearchActive = 1;
        gTestValue = 0.0f;
        gCurrentWeaponIndex = 0;
        gTestMode = 0;
        
        if (!gResearchFlightLoop) {
            XPLMCreateFlightLoop_t flightLoopParams;
            flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
            flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
            flightLoopParams.callbackFunc = ResearchFlightLoopCallback;
            flightLoopParams.refcon = NULL;
            
            gResearchFlightLoop = XPLMCreateFlightLoop(&flightLoopParams);
        }
        
        if (gResearchFlightLoop) {
            XPLMScheduleFlightLoop(gResearchFlightLoop, 0.2f, 1);
            XPLMDebugString("WEAPON RESEARCH: Research mode ACTIVE\n");
            LogAllWeaponData();
        }
    } else {
        gResearchActive = 0;
        if (gResearchFlightLoop) {
            XPLMScheduleFlightLoop(gResearchFlightLoop, 0, 0);
            XPLMDebugString("WEAPON RESEARCH: Research mode STOPPED\n");
        }
    }
}

static void NextTestModeCallback(void* inRefcon)
{
    if (!gResearchActive) return;
    
    gTestMode = (gTestMode + 1) % 10;
    gTestValue = 0.0f;
    
    const char* modes[] = {
        "Velocity VX/VY/VZ",
        "Steering S_FRN/S_SID/S_TOP", 
        "Target LAT/LON/H",
        "Target Index",
        "Distance TARG/POINT",
        "Elevation/Azimuth Ratios",
        "Angle THE/THE_CON",
        "Body Aero X/Y/Z",
        "Time Point",
        "Read-Only Monitoring"
    };
    
    char msg[256];
    snprintf(msg, sizeof(msg), "WEAPON RESEARCH: Test Mode %d: %s\n", gTestMode, modes[gTestMode]);
    XPLMDebugString(msg);
}

static void NextWeaponCallback(void* inRefcon)
{
    if (!gResearchActive) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount > 0) {
        gCurrentWeaponIndex = (gCurrentWeaponIndex + 1) % weaponCount;
        char msg[256];
        snprintf(msg, sizeof(msg), "WEAPON RESEARCH: Testing weapon %d of %d\n", gCurrentWeaponIndex, weaponCount);
        XPLMDebugString(msg);
    }
}

static void IncreaseTestValueCallback(void* inRefcon)
{
    if (!gResearchActive) return;
    gTestValue += 1.0f;
    TestCurrentDataref();
}

static void DecreaseTestValueCallback(void* inRefcon)
{
    if (!gResearchActive) return;
    gTestValue -= 1.0f;
    TestCurrentDataref();
}

static void LogAllWeaponData()
{
    XPLMDebugString("WEAPON RESEARCH: ===== COMPLETE WEAPON DATA DUMP =====\n");
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    char msg[512];
    snprintf(msg, sizeof(msg), "WEAPON RESEARCH: Total weapons: %d\n", weaponCount);
    XPLMDebugString(msg);
    
    if (weaponCount == 0) {
        XPLMDebugString("WEAPON RESEARCH: No weapons detected\n");
        return;
    }
    
    // Read all arrays
    float x[25] = {0}, y[25] = {0}, z[25] = {0};
    float vx[25] = {0}, vy[25] = {0}, vz[25] = {0};
    float targLat[25] = {0}, targLon[25] = {0}, targH[25] = {0};
    float distTarg[25] = {0}, distPoint[25] = {0};
    float elevRat[25] = {0}, azimRat[25] = {0};
    int types[25] = {0};
    
    if (gWeaponX) XPLMGetDatavf(gWeaponX, x, 0, weaponCount);
    if (gWeaponY) XPLMGetDatavf(gWeaponY, y, 0, weaponCount);
    if (gWeaponZ) XPLMGetDatavf(gWeaponZ, z, 0, weaponCount);
    if (gWeaponVX) XPLMGetDatavf(gWeaponVX, vx, 0, weaponCount);
    if (gWeaponVY) XPLMGetDatavf(gWeaponVY, vy, 0, weaponCount);
    if (gWeaponVZ) XPLMGetDatavf(gWeaponVZ, vz, 0, weaponCount);
    if (gWeaponTargLat) XPLMGetDatavf(gWeaponTargLat, targLat, 0, weaponCount);
    if (gWeaponTargLon) XPLMGetDatavf(gWeaponTargLon, targLon, 0, weaponCount);
    if (gWeaponTargH) XPLMGetDatavf(gWeaponTargH, targH, 0, weaponCount);
    if (gWeaponDistTarg) XPLMGetDatavf(gWeaponDistTarg, distTarg, 0, weaponCount);
    if (gWeaponDistPoint) XPLMGetDatavf(gWeaponDistPoint, distPoint, 0, weaponCount);
    if (gWeaponElevRat) XPLMGetDatavf(gWeaponElevRat, elevRat, 0, weaponCount);
    if (gWeaponAzimRat) XPLMGetDatavf(gWeaponAzimRat, azimRat, 0, weaponCount);
    if (gWeaponType) XPLMGetDatavi(gWeaponType, types, 0, weaponCount);
    
    for (int i = 0; i < weaponCount && i < 25; i++) {
        snprintf(msg, sizeof(msg), 
            "WEAPON RESEARCH: [%d] Type:%d Pos:(%.2f,%.2f,%.2f) Vel:(%.2f,%.2f,%.2f) "
            "Targ:(%.6f,%.6f,%.0f) Dist:(%.0f,%.0f) Elev/Azim:(%.3f,%.3f)\n",
            i, types[i], x[i], y[i], z[i], vx[i], vy[i], vz[i],
            targLat[i], targLon[i], targH[i], distTarg[i], distPoint[i], elevRat[i], azimRat[i]);
        XPLMDebugString(msg);
    }
    
    XPLMDebugString("WEAPON RESEARCH: ================================================\n");
}

static void TestCurrentDataref()
{
    if (!gResearchActive) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount == 0 || gCurrentWeaponIndex >= weaponCount) return;
    
    float testArray[25] = {0};
    testArray[gCurrentWeaponIndex] = gTestValue;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "WEAPON RESEARCH: Setting weapon[%d] mode %d to value %.2f\n", 
            gCurrentWeaponIndex, gTestMode, gTestValue);
    XPLMDebugString(msg);
    
    switch (gTestMode) {
        case 0: // Velocity
            if (gWeaponVX) XPLMSetDatavf(gWeaponVX, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponVY) XPLMSetDatavf(gWeaponVY, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponVZ) XPLMSetDatavf(gWeaponVZ, testArray, gCurrentWeaponIndex, 1);
            break;
        case 1: // Steering
            if (gWeaponSFrn) XPLMSetDatavf(gWeaponSFrn, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponSSid) XPLMSetDatavf(gWeaponSSid, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponSTop) XPLMSetDatavf(gWeaponSTop, testArray, gCurrentWeaponIndex, 1);
            break;
        case 2: // Target coordinates
            if (gWeaponTargLat) XPLMSetDatavf(gWeaponTargLat, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponTargLon) XPLMSetDatavf(gWeaponTargLon, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponTargH) XPLMSetDatavf(gWeaponTargH, testArray, gCurrentWeaponIndex, 1);
            break;
        case 3: // Target index
            if (gWeaponTargetIndex) {
                int intArray[25] = {0};
                intArray[gCurrentWeaponIndex] = (int)gTestValue;
                XPLMSetDatavi(gWeaponTargetIndex, intArray, gCurrentWeaponIndex, 1);
            }
            break;
        case 4: // Distance
            if (gWeaponDistTarg) XPLMSetDatavf(gWeaponDistTarg, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponDistPoint) XPLMSetDatavf(gWeaponDistPoint, testArray, gCurrentWeaponIndex, 1);
            break;
        case 5: // Elevation/Azimuth
            if (gWeaponElevRat) XPLMSetDatavf(gWeaponElevRat, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponAzimRat) XPLMSetDatavf(gWeaponAzimRat, testArray, gCurrentWeaponIndex, 1);
            break;
        case 6: // Angles
            if (gWeaponThecon) XPLMSetDatavf(gWeaponThecon, testArray, gCurrentWeaponIndex, 1);
            break;
        case 7: // Body Aero
            if (gWeaponXBodyAero) XPLMSetDatavf(gWeaponXBodyAero, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponYBodyAero) XPLMSetDatavf(gWeaponYBodyAero, testArray, gCurrentWeaponIndex, 1);
            if (gWeaponZBodyAero) XPLMSetDatavf(gWeaponZBodyAero, testArray, gCurrentWeaponIndex, 1);
            break;
        case 8: // Time point
            if (gWeaponTimePoint) XPLMSetDatavf(gWeaponTimePoint, testArray, gCurrentWeaponIndex, 1);
            break;
    }
}

static float ResearchFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gResearchActive) return 0;
    
    static int logCounter = 0;
    logCounter++;
    
    // Log detailed data every 5 seconds
    if (logCounter % 25 == 0) {
        LogAllWeaponData();
    }
    
    return 0.2f; // Continue every 0.2 seconds
}