/*
 * FLIR_CoordinateTest.cpp
 * 
 * Step 1: Test basic coordinate systems and ray casting
 * Goal: Verify we can get sensible 3D coordinates from simple ray casting
 */

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMScenery.h"
#include "XPLMProcessing.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

// Flight loop for automatic testing
static XPLMFlightLoopID gTestFlightLoop = NULL;
static bool gTestCompleted = false;

// Aircraft position datarefs
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;

// Terrain probe
static XPLMProbeRef gTerrainProbe = NULL;

// Flight loop callback
static float TestCoordinatesFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void RunCoordinateTest(void);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Coordinate Test");
    strcpy(outSig, "flir.coordinate.test");
    strcpy(outDesc, "Test coordinate systems and ray casting");

    // Find aircraft datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");

    if (!gAircraftX || !gAircraftY || !gAircraftZ || !gAircraftHeading) {
        XPLMDebugString("COORD_TEST: ERROR - Aircraft datarefs not found!\n");
        return 0;
    }

    // Create terrain probe
    gTerrainProbe = XPLMCreateProbe(xplm_ProbeY);
    if (!gTerrainProbe) {
        XPLMDebugString("COORD_TEST: ERROR - Failed to create terrain probe!\n");
        return 0;
    }

    // Register flight loop for automatic testing
    XPLMCreateFlightLoop_t flightLoopParams;
    flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
    flightLoopParams.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    flightLoopParams.callbackFunc = TestCoordinatesFlightLoop;
    flightLoopParams.refcon = NULL;
    
    gTestFlightLoop = XPLMCreateFlightLoop(&flightLoopParams);
    if (gTestFlightLoop) {
        XPLMScheduleFlightLoop(gTestFlightLoop, 3.0f, 1); // Start test after 3 seconds
        XPLMDebugString("COORD_TEST: Plugin loaded - Will run coordinate test automatically in 3 seconds\n");
    } else {
        XPLMDebugString("COORD_TEST: ERROR - Failed to create flight loop\n");
    }
    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gTestFlightLoop) {
        XPLMDestroyFlightLoop(gTestFlightLoop);
        gTestFlightLoop = NULL;
    }

    if (gTerrainProbe) {
        XPLMDestroyProbe(gTerrainProbe);
        gTerrainProbe = NULL;
    }

    XPLMDebugString("COORD_TEST: Plugin stopped\n");
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

// Flight loop function - runs the test automatically once
static float TestCoordinatesFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gTestCompleted) {
        gTestCompleted = true;
        RunCoordinateTest();
    }
    return 0; // Don't schedule again
}

// Test coordinate systems with simple straight-down ray
static void RunCoordinateTest(void)
{
    XPLMDebugString("COORD_TEST: =================================================\n");
    XPLMDebugString("COORD_TEST: Starting coordinate system test...\n");

    // Get aircraft position
    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);
    float acHeading = XPLMGetDataf(gAircraftHeading);

    char msg[256];
    snprintf(msg, sizeof(msg), 
        "COORD_TEST: Aircraft Position - X=%.2f Y=%.2f Z=%.2f Heading=%.1f°\n",
        acX, acY, acZ, acHeading);
    XPLMDebugString(msg);

    // Test 0: Basic probe functionality test
    XPLMDebugString("COORD_TEST: Test 0 - Basic probe test\n");
    XPLMProbeInfo_t basicProbeInfo;
    basicProbeInfo.structSize = sizeof(XPLMProbeInfo_t);
    
    // Test probe at aircraft position (should always work)
    XPLMProbeResult basicResult = XPLMProbeTerrainXYZ(gTerrainProbe, acX, acY, acZ, &basicProbeInfo);
    
    const char* basicResultStr = "UNKNOWN";
    if (basicResult == xplm_ProbeHitTerrain) basicResultStr = "HIT_TERRAIN";
    else if (basicResult == xplm_ProbeMissed) basicResultStr = "MISSED";
    else if (basicResult == xplm_ProbeError) basicResultStr = "ERROR";
    
    snprintf(msg, sizeof(msg), 
        "COORD_TEST: Basic probe at aircraft - Result=%s TerrainY=%.2f\n",
        basicResultStr, basicProbeInfo.locationY);
    XPLMDebugString(msg);
    
    if (basicResult != xplm_ProbeHitTerrain) {
        XPLMDebugString("COORD_TEST: ERROR - Basic probe failed! Terrain system may not be working.\n");
        return;
    }

    // Test 1: Simple straight down ray
    XPLMDebugString("COORD_TEST: Test 1 - Ray straight down\n");
    
    XPLMProbeInfo_t probeInfo;
    probeInfo.structSize = sizeof(XPLMProbeInfo_t);

    // Ray direction: straight down
    float rayDirX = 0.0f;
    float rayDirY = -1.0f;  // Straight down
    float rayDirZ = 0.0f;

    snprintf(msg, sizeof(msg), 
        "COORD_TEST: Ray Start(%.2f,%.2f,%.2f) Direction(%.2f,%.2f,%.2f)\n",
        acX, acY, acZ, rayDirX, rayDirY, rayDirZ);
    XPLMDebugString(msg);

    // Binary search for ground intersection
    float minRange = 10.0f;    // Start 10m below aircraft
    float maxRange = 10000.0f; // Max 10km down
    float precision = 1.0f;    // 1m precision
    int maxIterations = 50;

    bool foundGround = false;
    int iteration = 0;

    while ((maxRange - minRange) > precision && iteration < maxIterations) {
        float currentRange = (minRange + maxRange) / 2.0f;
        
        // Calculate test point
        float testX = acX + rayDirX * currentRange;
        float testY = acY + rayDirY * currentRange;
        float testZ = acZ + rayDirZ * currentRange;

        // Probe terrain
        XPLMProbeResult result = XPLMProbeTerrainXYZ(gTerrainProbe, testX, testY, testZ, &probeInfo);

        // Detailed result analysis
        const char* resultStr = "UNKNOWN";
        if (result == xplm_ProbeHitTerrain) resultStr = "HIT_TERRAIN";
        else if (result == xplm_ProbeMissed) resultStr = "MISSED";
        else if (result == xplm_ProbeError) resultStr = "ERROR";

        bool isUnderGround = (testY < probeInfo.locationY);

        if (iteration < 10) { // Log more iterations for debugging
            snprintf(msg, sizeof(msg), 
                "COORD_TEST: Iter=%d Range=%.1f Test(%.2f,%.2f,%.2f) Result=%s Terrain=%.2f Under=%s\n",
                iteration, currentRange, testX, testY, testZ, resultStr, probeInfo.locationY, isUnderGround ? "YES" : "NO");
            XPLMDebugString(msg);
        }

        if (result == xplm_ProbeHitTerrain) {
            foundGround = true;
            
            if (isUnderGround) {
                maxRange = currentRange; // Go up
            } else {
                minRange = currentRange; // Go down
            }
        } else {
            // Log probe failures
            if (iteration < 5) {
                snprintf(msg, sizeof(msg), 
                    "COORD_TEST: Probe failed at iteration %d - Result=%s\n", iteration, resultStr);
                XPLMDebugString(msg);
            }
            minRange = currentRange; // No terrain hit, go further
        }

        iteration++;
    }

    if (foundGround) {
        float finalRange = (minRange + maxRange) / 2.0f;
        float groundX = acX + rayDirX * finalRange;
        float groundY = acY + rayDirY * finalRange;
        float groundZ = acZ + rayDirZ * finalRange;

        // Get final terrain height
        XPLMProbeTerrainXYZ(gTerrainProbe, groundX, groundY, groundZ, &probeInfo);

        snprintf(msg, sizeof(msg), 
            "COORD_TEST: SUCCESS - Ground found at (%.2f,%.2f,%.2f) TerrainHeight=%.2f AGL=%.1fm\n",
            groundX, groundY, groundZ, probeInfo.locationY, finalRange);
        XPLMDebugString(msg);

        // Test 2: Verify coordinates make sense
        XPLMDebugString("COORD_TEST: Test 2 - Coordinate verification\n");

        float deltaX = groundX - acX;
        float deltaY = groundY - acY;
        float deltaZ = groundZ - acZ;

        snprintf(msg, sizeof(msg), 
            "COORD_TEST: Delta from aircraft - dX=%.2f dY=%.2f dZ=%.2f\n",
            deltaX, deltaY, deltaZ);
        XPLMDebugString(msg);

        // Should be: dX≈0, dY<0 (negative), dZ≈0 for straight down
        if (fabs(deltaX) < 5.0f && deltaY < -10.0f && fabs(deltaZ) < 5.0f) {
            XPLMDebugString("COORD_TEST: ✓ Coordinates look correct for straight-down ray\n");
        } else {
            XPLMDebugString("COORD_TEST: ✗ WARNING - Coordinates don't look right for straight-down ray\n");
        }

        // Test 3: Distance calculation
        float distance3D = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
        float distanceVertical = fabs(deltaY);

        snprintf(msg, sizeof(msg), 
            "COORD_TEST: Distance - 3D=%.1fm Vertical=%.1fm\n",
            distance3D, distanceVertical);
        XPLMDebugString(msg);

    } else {
        XPLMDebugString("COORD_TEST: FAILED - No ground found\n");
    }

    XPLMDebugString("COORD_TEST: Test complete\n");
    XPLMDebugString("COORD_TEST: =================================================\n");
}