/*
 * FLIR_TerrainTest.cpp
 * 
 * Standalone plugin to test terrain finding functionality
 * Use this to validate terrain detection before integrating with guidance system
 */

#include "FLIR_TerrainFinder.h"
#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"
#include "XPLMMenus.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Test hotkeys
static XPLMHotKeyID gTestRaycastKey = NULL;
static XPLMHotKeyID gTestLinearKey = NULL;
static XPLMHotKeyID gTestFLIRKey = NULL;
static XPLMHotKeyID gBenchmarkKey = NULL;

// Aircraft datarefs
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;

// FLIR datarefs (if available)
static XPLMDataRef gFLIRPan = NULL;
static XPLMDataRef gFLIRTilt = NULL;

// Test callbacks
static void TestRaycastCallback(void* inRefcon);
static void TestLinearCallback(void* inRefcon);
static void TestFLIRCallback(void* inRefcon);
static void BenchmarkCallback(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Terrain Test");
    strcpy(outSig, "flir.terrain.test");
    strcpy(outDesc, "Test terrain finding algorithms");

    // Initialize terrain finder
    if (!InitializeTerrainFinder()) {
        XPLMDebugString("TERRAIN_TEST: Failed to initialize terrain finder!\n");
        return 0;
    }

    // Find aircraft datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");

    // Try to find FLIR datarefs (may not exist)
    gFLIRPan = XPLMFindDataRef("flir/camera/pan");
    gFLIRTilt = XPLMFindDataRef("flir/camera/tilt");

    // Register test hotkeys
    gTestRaycastKey = XPLMRegisterHotKey(XPLM_VK_F5, xplm_DownFlag, "Test Raycast Terrain", TestRaycastCallback, NULL);
    gTestLinearKey = XPLMRegisterHotKey(XPLM_VK_F6, xplm_DownFlag, "Test Linear Terrain", TestLinearCallback, NULL);
    gTestFLIRKey = XPLMRegisterHotKey(XPLM_VK_F7, xplm_DownFlag, "Test FLIR Terrain", TestFLIRCallback, NULL);
    gBenchmarkKey = XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, "Benchmark Terrain", BenchmarkCallback, NULL);

    XPLMDebugString("TERRAIN_TEST: Plugin loaded\n");
    XPLMDebugString("TERRAIN_TEST: F5=Raycast, F6=Linear, F7=FLIR, F8=Benchmark\n");

    // Run self-test
    TestTerrainFinder();

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    CleanupTerrainFinder();

    if (gTestRaycastKey) XPLMUnregisterHotKey(gTestRaycastKey);
    if (gTestLinearKey) XPLMUnregisterHotKey(gTestLinearKey);
    if (gTestFLIRKey) XPLMUnregisterHotKey(gTestFLIRKey);
    if (gBenchmarkKey) XPLMUnregisterHotKey(gBenchmarkKey);

    XPLMDebugString("TERRAIN_TEST: Plugin stopped\n");
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

// Test raycast method from aircraft position downward
static void TestRaycastCallback(void* inRefcon)
{
    if (!gAircraftX || !gAircraftY || !gAircraftZ) {
        XPLMDebugString("TERRAIN_TEST: Aircraft position datarefs not available\n");
        return;
    }

    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);

    XPLMDebugString("TERRAIN_TEST: Testing raycast method...\n");

    TerrainSearchParams params = GetDefaultSearchParams();
    TerrainFindResult result;

    // Test straight down
    bool success = FindTerrainByRaycast(acX, acY, acZ, 0.0f, -1.0f, 0.0f, &params, &result);

    if (success) {
        LogTerrainResult(&result, "Raycast Test");
        
        float actualRange = sqrt((result.localX - acX)*(result.localX - acX) + 
                                (result.localY - acY)*(result.localY - acY) + 
                                (result.localZ - acZ)*(result.localZ - acZ));
        
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "TERRAIN_TEST: Altitude above terrain: %.1fm\n", actualRange);
        XPLMDebugString(msg);
    } else {
        XPLMDebugString("TERRAIN_TEST: Raycast test failed\n");
    }
}

// Test linear method from aircraft position downward  
static void TestLinearCallback(void* inRefcon)
{
    if (!gAircraftX || !gAircraftY || !gAircraftZ) {
        XPLMDebugString("TERRAIN_TEST: Aircraft position datarefs not available\n");
        return;
    }

    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);

    XPLMDebugString("TERRAIN_TEST: Testing linear method...\n");

    TerrainSearchParams params = GetDefaultSearchParams();
    TerrainFindResult result;

    // Test straight down
    bool success = FindTerrainByLinearSearch(acX, acY, acZ, 0.0f, -1.0f, 0.0f, &params, &result);

    if (success) {
        LogTerrainResult(&result, "Linear Test");
    } else {
        XPLMDebugString("TERRAIN_TEST: Linear test failed\n");
    }
}

// Test FLIR-based terrain finding
static void TestFLIRCallback(void* inRefcon)
{
    if (!gAircraftX || !gAircraftY || !gAircraftZ || !gAircraftHeading) {
        XPLMDebugString("TERRAIN_TEST: Aircraft datarefs not available\n");
        return;
    }

    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);
    float acHeading = XPLMGetDataf(gAircraftHeading);

    // Get FLIR angles (use defaults if not available)
    float flirPan = 0.0f;
    float flirTilt = -15.0f; // 15 degrees down
    
    if (gFLIRPan && gFLIRTilt) {
        flirPan = XPLMGetDataf(gFLIRPan);
        flirTilt = XPLMGetDataf(gFLIRTilt);
        XPLMDebugString("TERRAIN_TEST: Using actual FLIR angles\n");
    } else {
        XPLMDebugString("TERRAIN_TEST: Using simulated FLIR angles (Pan=0°, Tilt=-15°)\n");
    }

    XPLMDebugString("TERRAIN_TEST: Testing FLIR-based terrain finding...\n");

    TerrainSearchParams params = GetMaritimeSearchParams();
    TerrainFindResult result;

    bool success = FindTargetFromFLIR(acX, acY, acZ, flirPan, flirTilt, acHeading, &params, &result);

    if (success) {
        LogTerrainResult(&result, "FLIR Test");
        
        // Calculate bearing and range for maritime context
        float deltaX = result.localX - acX;
        float deltaZ = result.localZ - acZ;
        float bearing = atan2(deltaX, deltaZ) * 180.0f / 3.14159f;
        float groundRange = sqrt(deltaX*deltaX + deltaZ*deltaZ);
        
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "TERRAIN_TEST: Maritime data - Bearing:%.1f° Range:%.0fm Water:%s\n",
            bearing, groundRange, result.isWater ? "YES" : "NO");
        XPLMDebugString(msg);
    } else {
        XPLMDebugString("TERRAIN_TEST: FLIR test failed\n");
    }
}

// Run benchmark tests
static void BenchmarkCallback(void* inRefcon)
{
    XPLMDebugString("TERRAIN_TEST: Running benchmark tests...\n");
    BenchmarkTerrainMethods();
}