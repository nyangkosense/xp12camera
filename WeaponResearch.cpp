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
static XPLMDataRef gWeaponPsi = NULL;
static XPLMDataRef gWeaponPsiCon = NULL;
static XPLMDataRef gWeaponQ1 = NULL;
static XPLMDataRef gWeaponQ2 = NULL;
static XPLMDataRef gWeaponQ3 = NULL;
static XPLMDataRef gWeaponQ4 = NULL;
static XPLMDataRef gWeaponQrad = NULL;
static XPLMDataRef gWeaponRrad = NULL;
static XPLMDataRef gWeaponRuddRat = NULL;

// Test control variables
static int gResearchActive = 0;
static int gTestMode = 0;
static float gTestValue = 0.0f;
static int gCurrentWeaponIndex = 0;
static XPLMFlightLoopID gResearchFlightLoop = NULL;

// Automatic testing variables
static int gAutoTestActive = 0;
static int gAutoTestDatarefIndex = 0;
static float gAutoTestTimer = 0.0f;
static float gAutoTestInterval = 1.0f; // Test each dataref for 1 second (much faster)
static float gAutoTestValues[] = {0.0f, 500.0f, -500.0f, 1000.0f, -1000.0f}; // Reduced to key values

// Weapon mode control
static XPLMDataRef gWeaponMode = NULL;
static XPLMDataRef gWeaponRadarOn = NULL;

// Function declarations
static void ActivateResearchCallback(void* inRefcon);
static void NextTestModeCallback(void* inRefcon);
static void IncreaseTestValueCallback(void* inRefcon);
static void DecreaseTestValueCallback(void* inRefcon);
static void NextWeaponCallback(void* inRefcon);
static void StartAutoTestCallback(void* inRefcon);
static float ResearchFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void LogAllWeaponData();
static void TestCurrentDataref();
static void AutoTestNextDataref();
static void ApplyTestToDataref(int datarefIndex, float value, int weaponIndex);

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
    gWeaponPsi = XPLMFindDataRef("sim/weapons/psi");
    gWeaponPsiCon = XPLMFindDataRef("sim/weapons/psi_con");
    gWeaponQ1 = XPLMFindDataRef("sim/weapons/q1");
    gWeaponQ2 = XPLMFindDataRef("sim/weapons/q2");
    gWeaponQ3 = XPLMFindDataRef("sim/weapons/q3");
    gWeaponQ4 = XPLMFindDataRef("sim/weapons/q4");
    gWeaponQrad = XPLMFindDataRef("sim/weapons/Qrad");
    gWeaponRrad = XPLMFindDataRef("sim/weapons/Rrad");
    gWeaponRuddRat = XPLMFindDataRef("sim/weapons/rudd_rat");
    
    // Weapon mode control datarefs
    gWeaponMode = XPLMFindDataRef("sim/weapons/mode");
    gWeaponRadarOn = XPLMFindDataRef("sim/weapons/radar_on");

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_F10, xplm_DownFlag, "WR: Activate Research", ActivateResearchCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F11, xplm_DownFlag, "WR: Next Test Mode", NextTestModeCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F12, xplm_DownFlag, "WR: Next Weapon", NextWeaponCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_COMMA, xplm_DownFlag, "WR: Decrease Value", DecreaseTestValueCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_PERIOD, xplm_DownFlag, "WR: Increase Value", IncreaseTestValueCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F9, xplm_DownFlag, "WR: Start Auto Test", StartAutoTestCallback, NULL);

    XPLMDebugString("WEAPON RESEARCH: Plugin loaded\n");
    XPLMDebugString("WEAPON RESEARCH: F9=Auto Test, F10=Start/Stop, F11=Next Test Mode, F12=Next Weapon, ,/. = Dec/Inc Value\n");

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
    int modes[25] = {0};
    int radarOn[25] = {0};
    
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
    if (gWeaponMode) XPLMGetDatavi(gWeaponMode, modes, 0, weaponCount);
    if (gWeaponRadarOn) XPLMGetDatavi(gWeaponRadarOn, radarOn, 0, weaponCount);
    
    for (int i = 0; i < weaponCount && i < 25; i++) {
        snprintf(msg, sizeof(msg), 
            "WEAPON RESEARCH: [%d] Type:%d Mode:%d Radar:%d Pos:(%.2f,%.2f,%.2f) Vel:(%.2f,%.2f,%.2f) "
            "Targ:(%.6f,%.6f,%.0f) Dist:(%.0f,%.0f) Elev/Azim:(%.3f,%.3f)\n",
            i, types[i], modes[i], radarOn[i], x[i], y[i], z[i], vx[i], vy[i], vz[i],
            targLat[i], targLon[i], targH[i], distTarg[i], distPoint[i], elevRat[i], azimRat[i]);
        XPLMDebugString(msg);
    }
    
    XPLMDebugString("WEAPON RESEARCH: ================================================\n");
}

static void StartAutoTestCallback(void* inRefcon)
{
    if (gAutoTestActive) {
        gAutoTestActive = 0;
        XPLMDebugString("WEAPON RESEARCH: Automatic testing STOPPED\n");
    } else {
        gAutoTestActive = 1;
        gAutoTestDatarefIndex = 0;
        gAutoTestTimer = 0.0f;
        
        // Set all weapons to internal radar mode for proper control
        int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
        if (weaponCount > 0) {
            // Set weapon mode to internal radar (mode 1)
            if (gWeaponMode) {
                int modeArray[25] = {0};
                for (int i = 0; i < weaponCount && i < 25; i++) {
                    modeArray[i] = 1; // 1 = internal radar
                }
                XPLMSetDatavi(gWeaponMode, modeArray, 0, weaponCount);
                XPLMDebugString("WEAPON RESEARCH: Set all weapons to INTERNAL RADAR mode\n");
            }
            
            // Turn on radar
            if (gWeaponRadarOn) {
                int radarArray[25] = {0};
                for (int i = 0; i < weaponCount && i < 25; i++) {
                    radarArray[i] = 1; // Radar ON
                }
                XPLMSetDatavi(gWeaponRadarOn, radarArray, 0, weaponCount);
                XPLMDebugString("WEAPON RESEARCH: Radar turned ON for all weapons\n");
            }
        }
        
        XPLMDebugString("WEAPON RESEARCH: FAST automatic testing STARTED - 1 second per dataref\n");
    }
}

static void AutoTestNextDataref()
{
    if (!gAutoTestActive) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount == 0) return;
    
    gAutoTestDatarefIndex++;
    
    // Total number of datarefs to test (31 datarefs including mode and radar_on)
    if (gAutoTestDatarefIndex >= 31) {
        gAutoTestDatarefIndex = 0;
        XPLMDebugString("WEAPON RESEARCH: Auto test cycle completed, restarting\n");
    }
    
    static int valueIndex = 0;
    valueIndex = (valueIndex + 1) % 5; // Cycle through test values (reduced to 5)
    
    float testValue = gAutoTestValues[valueIndex];
    
    ApplyTestToDataref(gAutoTestDatarefIndex, testValue, 0); // Test on weapon 0
}

static void ApplyTestToDataref(int datarefIndex, float value, int weaponIndex)
{
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount == 0 || weaponIndex >= weaponCount) return;
    
    float testArray[25] = {0};
    testArray[weaponIndex] = value;
    
    const char* datarefNames[] = {
        "vx", "vy", "vz", "s_frn", "s_sid", "s_top", "targ_lat", "targ_lon", "targ_h",
        "target_index", "dist_targ", "dist_point", "elev_rat", "azim_rat", "the_con",
        "the", "time_point", "X_body_aero", "Y_body_aero", "Z_body_aero", "psi",
        "psi_con", "q1", "q2", "q3", "q4", "Qrad", "Rrad", "rudd_rat", "mode", "radar_on"
    };
    
    char msg[256];
    snprintf(msg, sizeof(msg), "WEAPON RESEARCH: AUTO TEST - Setting %s[%d] = %.1f\n", 
            datarefNames[datarefIndex], weaponIndex, value);
    XPLMDebugString(msg);
    
    switch (datarefIndex) {
        case 0: // vx
            if (gWeaponVX) XPLMSetDatavf(gWeaponVX, testArray, weaponIndex, 1);
            break;
        case 1: // vy
            if (gWeaponVY) XPLMSetDatavf(gWeaponVY, testArray, weaponIndex, 1);
            break;
        case 2: // vz
            if (gWeaponVZ) XPLMSetDatavf(gWeaponVZ, testArray, weaponIndex, 1);
            break;
        case 3: // s_frn
            if (gWeaponSFrn) XPLMSetDatavf(gWeaponSFrn, testArray, weaponIndex, 1);
            break;
        case 4: // s_sid
            if (gWeaponSSid) XPLMSetDatavf(gWeaponSSid, testArray, weaponIndex, 1);
            break;
        case 5: // s_top
            if (gWeaponSTop) XPLMSetDatavf(gWeaponSTop, testArray, weaponIndex, 1);
            break;
        case 6: // targ_lat
            if (gWeaponTargLat) XPLMSetDatavf(gWeaponTargLat, testArray, weaponIndex, 1);
            break;
        case 7: // targ_lon
            if (gWeaponTargLon) XPLMSetDatavf(gWeaponTargLon, testArray, weaponIndex, 1);
            break;
        case 8: // targ_h
            if (gWeaponTargH) XPLMSetDatavf(gWeaponTargH, testArray, weaponIndex, 1);
            break;
        case 9: // target_index
            if (gWeaponTargetIndex) {
                int intArray[25] = {0};
                intArray[weaponIndex] = (int)value;
                XPLMSetDatavi(gWeaponTargetIndex, intArray, weaponIndex, 1);
            }
            break;
        case 10: // dist_targ
            if (gWeaponDistTarg) XPLMSetDatavf(gWeaponDistTarg, testArray, weaponIndex, 1);
            break;
        case 11: // dist_point
            if (gWeaponDistPoint) XPLMSetDatavf(gWeaponDistPoint, testArray, weaponIndex, 1);
            break;
        case 12: // elev_rat
            if (gWeaponElevRat) XPLMSetDatavf(gWeaponElevRat, testArray, weaponIndex, 1);
            break;
        case 13: // azim_rat
            if (gWeaponAzimRat) XPLMSetDatavf(gWeaponAzimRat, testArray, weaponIndex, 1);
            break;
        case 14: // the_con
            if (gWeaponThecon) XPLMSetDatavf(gWeaponThecon, testArray, weaponIndex, 1);
            break;
        case 15: // the
            if (gWeaponThe) XPLMSetDatavf(gWeaponThe, testArray, weaponIndex, 1);
            break;
        case 16: // time_point
            if (gWeaponTimePoint) XPLMSetDatavf(gWeaponTimePoint, testArray, weaponIndex, 1);
            break;
        case 17: // X_body_aero
            if (gWeaponXBodyAero) XPLMSetDatavf(gWeaponXBodyAero, testArray, weaponIndex, 1);
            break;
        case 18: // Y_body_aero
            if (gWeaponYBodyAero) XPLMSetDatavf(gWeaponYBodyAero, testArray, weaponIndex, 1);
            break;
        case 19: // Z_body_aero
            if (gWeaponZBodyAero) XPLMSetDatavf(gWeaponZBodyAero, testArray, weaponIndex, 1);
            break;
        case 20: // psi
            if (gWeaponPsi) XPLMSetDatavf(gWeaponPsi, testArray, weaponIndex, 1);
            break;
        case 21: // psi_con
            if (gWeaponPsiCon) XPLMSetDatavf(gWeaponPsiCon, testArray, weaponIndex, 1);
            break;
        case 22: // q1
            if (gWeaponQ1) XPLMSetDatavf(gWeaponQ1, testArray, weaponIndex, 1);
            break;
        case 23: // q2
            if (gWeaponQ2) XPLMSetDatavf(gWeaponQ2, testArray, weaponIndex, 1);
            break;
        case 24: // q3
            if (gWeaponQ3) XPLMSetDatavf(gWeaponQ3, testArray, weaponIndex, 1);
            break;
        case 25: // q4
            if (gWeaponQ4) XPLMSetDatavf(gWeaponQ4, testArray, weaponIndex, 1);
            break;
        case 26: // Qrad
            if (gWeaponQrad) XPLMSetDatavf(gWeaponQrad, testArray, weaponIndex, 1);
            break;
        case 27: // Rrad
            if (gWeaponRrad) XPLMSetDatavf(gWeaponRrad, testArray, weaponIndex, 1);
            break;
        case 28: // rudd_rat
            if (gWeaponRuddRat) XPLMSetDatavf(gWeaponRuddRat, testArray, weaponIndex, 1);
            break;
        case 29: // mode
            if (gWeaponMode) {
                int intArray[25] = {0};
                intArray[weaponIndex] = (int)value;
                XPLMSetDatavi(gWeaponMode, intArray, weaponIndex, 1);
            }
            break;
        case 30: // radar_on
            if (gWeaponRadarOn) {
                int intArray[25] = {0};
                intArray[weaponIndex] = (int)value;
                XPLMSetDatavi(gWeaponRadarOn, intArray, weaponIndex, 1);
            }
            break;
    }
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
    
    // Handle automatic testing
    if (gAutoTestActive) {
        gAutoTestTimer += inElapsedSinceLastCall;
        
        if (gAutoTestTimer >= gAutoTestInterval) {
            gAutoTestTimer = 0.0f;
            AutoTestNextDataref();
        }
    }
    
    // Log detailed data every 2 seconds (faster logging)
    if (logCounter % 10 == 0) {
        LogAllWeaponData();
    }
    
    return 0.2f; // Continue every 0.2 seconds
}