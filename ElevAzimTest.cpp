/*
 * ElevAzimTest.cpp
 * 
 * Focused test plugin for elevation/azimuth rate missile control
 * Tests realistic circling and guidance patterns
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

// Weapon datarefs
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;
static XPLMDataRef gWeaponElevRat = NULL;
static XPLMDataRef gWeaponAzimRat = NULL;
static XPLMDataRef gWeaponMode = NULL;
static XPLMDataRef gWeaponRadarOn = NULL;

// Aircraft position for reference
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;

// Test control variables
static int gTestActive = 0;
static int gTestMode = 0; // 0=circle, 1=spiral, 2=target_track
static float gTestTimer = 0.0f;
static XPLMFlightLoopID gTestFlightLoop = NULL;

// Target for guidance tests
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static int gTargetSet = 0;

// Test parameters
static float gCircleSpeed = 0.5f; // radians per second
static float gElevRate = 0.0f; // current elevation rate
static float gAzimRate = 0.0f; // current azimuth rate

// Function declarations
static void StartTestCallback(void* inRefcon);
static void NextTestModeCallback(void* inRefcon);
static void SetTargetCallback(void* inRefcon);
static void IncreaseRateCallback(void* inRefcon);
static void DecreaseRateCallback(void* inRefcon);
static float TestFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void LogMissileData();
static void ApplyCircleTest(float deltaTime);
static void ApplySpiralTest(float deltaTime);
static void ApplyTargetTrackTest(float deltaTime);
static void SetMissileRates(float elevRate, float azimRate);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Elevation/Azimuth Rate Test");
    strcpy(outSig, "elevazimtest.guidance");
    strcpy(outDesc, "Test plugin for missile elevation/azimuth rate control");

    // Initialize weapon datarefs
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    gWeaponElevRat = XPLMFindDataRef("sim/weapons/elev_rat");
    gWeaponAzimRat = XPLMFindDataRef("sim/weapons/azim_rat");
    gWeaponMode = XPLMFindDataRef("sim/weapons/mode");
    gWeaponRadarOn = XPLMFindDataRef("sim/weapons/radar_on");
    
    // Aircraft position datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_F7, xplm_DownFlag, "EAT: Start/Stop Test", StartTestCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, "EAT: Next Test Mode", NextTestModeCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F6, xplm_DownFlag, "EAT: Set Target Here", SetTargetCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_EQUAL, xplm_DownFlag, "EAT: Increase Rate", IncreaseRateCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_MINUS, xplm_DownFlag, "EAT: Decrease Rate", DecreaseRateCallback, NULL);

    XPLMDebugString("ELEV/AZIM TEST: Plugin loaded\n");
    XPLMDebugString("ELEV/AZIM TEST: F6=Set Target, F7=Start/Stop, F8=Next Mode, +/- = Inc/Dec Rate\n");
    XPLMDebugString("ELEV/AZIM TEST: Modes: 0=Circle, 1=Spiral, 2=Target Track\n");

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gTestActive && gTestFlightLoop) {
        XPLMScheduleFlightLoop(gTestFlightLoop, 0, 0);
    }
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void StartTestCallback(void* inRefcon)
{
    if (!gTestActive) {
        gTestActive = 1;
        gTestTimer = 0.0f;
        
        // Set weapons to internal radar mode
        int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
        if (weaponCount > 0 && gWeaponMode && gWeaponRadarOn) {
            int modeArray[25] = {0};
            int radarArray[25] = {0};
            
            for (int i = 0; i < weaponCount && i < 25; i++) {
                modeArray[i] = 1; // Internal radar
                radarArray[i] = 1; // Radar on
            }
            
            XPLMSetDatavi(gWeaponMode, modeArray, 0, weaponCount);
            XPLMSetDatavi(gWeaponRadarOn, radarArray, 0, weaponCount);
            XPLMDebugString("ELEV/AZIM TEST: Set weapons to internal radar mode\n");
        }
        
        if (!gTestFlightLoop) {
            XPLMCreateFlightLoop_t flightLoopParams;
            flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
            flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
            flightLoopParams.callbackFunc = TestFlightLoopCallback;
            flightLoopParams.refcon = NULL;
            
            gTestFlightLoop = XPLMCreateFlightLoop(&flightLoopParams);
        }
        
        if (gTestFlightLoop) {
            XPLMScheduleFlightLoop(gTestFlightLoop, 0.1f, 1);
            
            const char* modeNames[] = {"CIRCLE", "SPIRAL", "TARGET TRACK"};
            char msg[256];
            snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Started %s test\n", modeNames[gTestMode]);
            XPLMDebugString(msg);
        }
    } else {
        gTestActive = 0;
        if (gTestFlightLoop) {
            XPLMScheduleFlightLoop(gTestFlightLoop, 0, 0);
            XPLMDebugString("ELEV/AZIM TEST: Test stopped\n");
        }
        // Reset rates to zero
        SetMissileRates(0.0f, 0.0f);
    }
}

static void NextTestModeCallback(void* inRefcon)
{
    gTestMode = (gTestMode + 1) % 3;
    gTestTimer = 0.0f;
    
    const char* modeNames[] = {"CIRCLE", "SPIRAL", "TARGET TRACK"};
    char msg[256];
    snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Switched to %s mode\n", modeNames[gTestMode]);
    XPLMDebugString(msg);
}

static void SetTargetCallback(void* inRefcon)
{
    if (gAircraftX && gAircraftY && gAircraftZ) {
        gTargetX = XPLMGetDataf(gAircraftX);
        gTargetY = XPLMGetDataf(gAircraftY);
        gTargetZ = XPLMGetDataf(gAircraftZ);
        gTargetSet = 1;
        
        char msg[256];
        snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Target set at (%.0f, %.0f, %.0f)\n", 
                gTargetX, gTargetY, gTargetZ);
        XPLMDebugString(msg);
    }
}

static void IncreaseRateCallback(void* inRefcon)
{
    if (gTestMode == 0) { // Circle
        gCircleSpeed += 0.1f;
        char msg[256];
        snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Circle speed: %.2f rad/s\n", gCircleSpeed);
        XPLMDebugString(msg);
    } else {
        gElevRate += 0.5f;
        gAzimRate += 0.5f;
        char msg[256];
        snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Manual rates: Elev=%.1f, Azim=%.1f\n", gElevRate, gAzimRate);
        XPLMDebugString(msg);
        SetMissileRates(gElevRate, gAzimRate);
    }
}

static void DecreaseRateCallback(void* inRefcon)
{
    if (gTestMode == 0) { // Circle
        gCircleSpeed -= 0.1f;
        if (gCircleSpeed < 0.1f) gCircleSpeed = 0.1f;
        char msg[256];
        snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Circle speed: %.2f rad/s\n", gCircleSpeed);
        XPLMDebugString(msg);
    } else {
        gElevRate -= 0.5f;
        gAzimRate -= 0.5f;
        char msg[256];
        snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Manual rates: Elev=%.1f, Azim=%.1f\n", gElevRate, gAzimRate);
        XPLMDebugString(msg);
        SetMissileRates(gElevRate, gAzimRate);
    }
}

static void SetMissileRates(float elevRate, float azimRate)
{
    if (!gWeaponElevRat || !gWeaponAzimRat) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    float elevArray[25] = {0};
    float azimArray[25] = {0};
    
    // Apply to first active missile only
    elevArray[0] = elevRate;
    azimArray[0] = azimRate;
    
    XPLMSetDatavf(gWeaponElevRat, elevArray, 0, 1);
    XPLMSetDatavf(gWeaponAzimRat, azimArray, 0, 1);
}

static void ApplyCircleTest(float deltaTime)
{
    gTestTimer += deltaTime;
    
    // Generate smooth circular motion
    float angle = gTestTimer * gCircleSpeed;
    float elevRate = sin(angle) * 2.0f; // ±2 degrees/second
    float azimRate = cos(angle) * 2.0f; // ±2 degrees/second
    
    SetMissileRates(elevRate, azimRate);
    
    static int logCounter = 0;
    logCounter++;
    if (logCounter % 50 == 0) { // Log every 5 seconds
        char msg[256];
        snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Circle - Angle=%.1f°, Elev=%.2f, Azim=%.2f\n", 
                angle * 180.0f / M_PI, elevRate, azimRate);
        XPLMDebugString(msg);
        LogMissileData();
    }
}

static void ApplySpiralTest(float deltaTime)
{
    gTestTimer += deltaTime;
    
    // Generate spiral pattern - increasing turn rate
    float spiralRate = gTestTimer * 0.1f; // Gradually increase
    float elevRate = sin(gTestTimer) * spiralRate;
    float azimRate = cos(gTestTimer) * spiralRate;
    
    SetMissileRates(elevRate, azimRate);
    
    static int logCounter = 0;
    logCounter++;
    if (logCounter % 50 == 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Spiral - Rate=%.2f, Elev=%.2f, Azim=%.2f\n", 
                spiralRate, elevRate, azimRate);
        XPLMDebugString(msg);
        LogMissileData();
    }
}

static void ApplyTargetTrackTest(float deltaTime)
{
    if (!gTargetSet) {
        SetMissileRates(0.0f, 0.0f);
        return;
    }
    
    // Get first active missile position
    if (!gWeaponX || !gWeaponY || !gWeaponZ) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    float missileX[25] = {0};
    float missileY[25] = {0};
    float missileZ[25] = {0};
    
    XPLMGetDatavf(gWeaponX, missileX, 0, weaponCount);
    XPLMGetDatavf(gWeaponY, missileY, 0, weaponCount);
    XPLMGetDatavf(gWeaponZ, missileZ, 0, weaponCount);
    
    // Skip if missile not active
    if (missileX[0] == 0 && missileY[0] == 0 && missileZ[0] == 0) {
        return;
    }
    
    // Calculate vector to target
    float dx = gTargetX - missileX[0];
    float dy = gTargetY - missileY[0];
    float dz = gTargetZ - missileZ[0];
    
    float distance = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (distance > 10.0f) { // Don't apply steering if very close
        // Calculate required turn rates (proportional control)
        float horizontalDist = sqrt(dx*dx + dz*dz);
        
        // Elevation rate (pitch) - based on vertical error
        float elevRate = atan2(dy, horizontalDist) * 2.0f; // Convert to rate
        
        // Azimuth rate (yaw) - based on horizontal bearing error
        float azimRate = atan2(dx, dz) * 2.0f; // Convert to rate
        
        // Limit rates to reasonable values
        if (elevRate > 5.0f) elevRate = 5.0f;
        if (elevRate < -5.0f) elevRate = -5.0f;
        if (azimRate > 5.0f) azimRate = 5.0f;
        if (azimRate < -5.0f) azimRate = -5.0f;
        
        SetMissileRates(elevRate, azimRate);
        
        static int logCounter = 0;
        logCounter++;
        if (logCounter % 25 == 0) { // Log every 2.5 seconds
            char msg[256];
            snprintf(msg, sizeof(msg), "ELEV/AZIM TEST: Track - Dist=%.0fm, Elev=%.2f, Azim=%.2f\n", 
                    distance, elevRate, azimRate);
            XPLMDebugString(msg);
            LogMissileData();
        }
    }
}

static void LogMissileData()
{
    if (!gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) return;
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : 0;
    if (weaponCount <= 0) return;
    
    float x[25] = {0}, y[25] = {0}, z[25] = {0};
    float vx[25] = {0}, vy[25] = {0}, vz[25] = {0};
    float elev[25] = {0}, azim[25] = {0};
    
    XPLMGetDatavf(gWeaponX, x, 0, weaponCount);
    XPLMGetDatavf(gWeaponY, y, 0, weaponCount);
    XPLMGetDatavf(gWeaponZ, z, 0, weaponCount);
    XPLMGetDatavf(gWeaponVX, vx, 0, weaponCount);
    XPLMGetDatavf(gWeaponVY, vy, 0, weaponCount);
    XPLMGetDatavf(gWeaponVZ, vz, 0, weaponCount);
    
    if (gWeaponElevRat) XPLMGetDatavf(gWeaponElevRat, elev, 0, weaponCount);
    if (gWeaponAzimRat) XPLMGetDatavf(gWeaponAzimRat, azim, 0, weaponCount);
    
    for (int i = 0; i < weaponCount && i < 3; i++) { // Log first 3 missiles
        if (x[i] != 0 || y[i] != 0 || z[i] != 0) { // Only active missiles
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "ELEV/AZIM TEST: [%d] Pos:(%.0f,%.0f,%.0f) Vel:(%.1f,%.1f,%.1f) Rates:(%.2f,%.2f)\n",
                i, x[i], y[i], z[i], vx[i], vy[i], vz[i], elev[i], azim[i]);
            XPLMDebugString(msg);
        }
    }
}

static float TestFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gTestActive) return 0;
    
    switch (gTestMode) {
        case 0: // Circle
            ApplyCircleTest(inElapsedSinceLastCall);
            break;
        case 1: // Spiral
            ApplySpiralTest(inElapsedSinceLastCall);
            break;
        case 2: // Target track
            ApplyTargetTrackTest(inElapsedSinceLastCall);
            break;
    }
    
    return 0.1f; // Continue every 0.1 seconds
}