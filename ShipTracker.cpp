/*
 * ShipTracker.cpp
 * 
 * Track AI ships and traffic in X-Plane
 * Get their exact positions for targeting tests
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// AI traffic datarefs
static XPLMDataRef gAICount = NULL;
static XPLMDataRef gAILat = NULL;
static XPLMDataRef gAILon = NULL;
static XPLMDataRef gAIAlt = NULL;
static XPLMDataRef gAIX = NULL;
static XPLMDataRef gAIY = NULL;
static XPLMDataRef gAIZ = NULL;
static XPLMDataRef gAIHeading = NULL;
static XPLMDataRef gAIType = NULL;

// Ship-specific datarefs (if they exist)
static XPLMDataRef gShipCount = NULL;
static XPLMDataRef gShipLat = NULL;
static XPLMDataRef gShipLon = NULL;
static XPLMDataRef gShipX = NULL;
static XPLMDataRef gShipY = NULL;
static XPLMDataRef gShipZ = NULL;

// Ground vehicle datarefs
static XPLMDataRef gGroundVehicleCount = NULL;
static XPLMDataRef gGroundVehicleLat = NULL;
static XPLMDataRef gGroundVehicleLon = NULL;
static XPLMDataRef gGroundVehicleX = NULL;
static XPLMDataRef gGroundVehicleY = NULL;
static XPLMDataRef gGroundVehicleZ = NULL;

// Aircraft position for reference
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftLat = NULL;
static XPLMDataRef gAircraftLon = NULL;

// Monitoring
static int gMonitoringActive = 0;
static XPLMFlightLoopID gMonitorLoop = NULL;

static void FindShipsCallback(void* inRefcon);
static void FindAITrafficCallback(void* inRefcon);
static void FindGroundVehiclesCallback(void* inRefcon);
static void StartShipMonitoringCallback(void* inRefcon);
static void StopShipMonitoringCallback(void* inRefcon);
static void TargetNearestShipCallback(void* inRefcon);
static float ShipMonitoringFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Ship Tracker");
    strcpy(outSig, "ship.tracker");
    strcpy(outDesc, "Track AI ships and traffic for targeting tests");

    // Initialize AI traffic datarefs
    gAICount = XPLMFindDataRef("sim/multiplayer/position/plane_count");
    gAILat = XPLMFindDataRef("sim/multiplayer/position/plane_lat");
    gAILon = XPLMFindDataRef("sim/multiplayer/position/plane_lon");
    gAIAlt = XPLMFindDataRef("sim/multiplayer/position/plane_alt");
    gAIX = XPLMFindDataRef("sim/multiplayer/position/plane_x");
    gAIY = XPLMFindDataRef("sim/multiplayer/position/plane_y");
    gAIZ = XPLMFindDataRef("sim/multiplayer/position/plane_z");
    gAIHeading = XPLMFindDataRef("sim/multiplayer/position/plane_heading");
    gAIType = XPLMFindDataRef("sim/multiplayer/position/plane_icao");
    
    // Try to find ship-specific datarefs
    gShipCount = XPLMFindDataRef("sim/water/ship_count");
    gShipLat = XPLMFindDataRef("sim/water/ship_lat");
    gShipLon = XPLMFindDataRef("sim/water/ship_lon");
    gShipX = XPLMFindDataRef("sim/water/ship_x");
    gShipY = XPLMFindDataRef("sim/water/ship_y");
    gShipZ = XPLMFindDataRef("sim/water/ship_z");
    
    // Try ground vehicle datarefs
    gGroundVehicleCount = XPLMFindDataRef("sim/ground/vehicle_count");
    gGroundVehicleLat = XPLMFindDataRef("sim/ground/vehicle_lat");
    gGroundVehicleLon = XPLMFindDataRef("sim/ground/vehicle_lon");
    gGroundVehicleX = XPLMFindDataRef("sim/ground/vehicle_x");
    gGroundVehicleY = XPLMFindDataRef("sim/ground/vehicle_y");
    gGroundVehicleZ = XPLMFindDataRef("sim/ground/vehicle_z");
    
    // Aircraft position
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftLat = XPLMFindDataRef("sim/flightmodel/position/latitude");
    gAircraftLon = XPLMFindDataRef("sim/flightmodel/position/longitude");

    // Register tracking hotkeys
    XPLMRegisterHotKey(XPLM_VK_F1, xplm_DownFlag, "Ship: Find Ships", FindShipsCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F2, xplm_DownFlag, "Ship: Find AI Traffic", FindAITrafficCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F3, xplm_DownFlag, "Ship: Find Ground Vehicles", FindGroundVehiclesCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F4, xplm_DownFlag, "Ship: Start Monitoring", StartShipMonitoringCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F5, xplm_DownFlag, "Ship: Stop Monitoring", StopShipMonitoringCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F6, xplm_DownFlag, "Ship: Target Nearest Ship", TargetNearestShipCallback, NULL);

    XPLMDebugString("SHIP TRACKER: Plugin loaded\n");
    XPLMDebugString("SHIP TRACKER: F1=Ships, F2=AI Traffic, F3=Ground, F4=Start Monitor, F5=Stop, F6=Target Ship\n");
    XPLMDebugString("SHIP TRACKER: Enable AI traffic in X-Plane settings first\n");

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

static void FindShipsCallback(void* inRefcon)
{
    char msg[512];
    
    if (gShipCount) {
        int shipCount = XPLMGetDatai(gShipCount);
        snprintf(msg, sizeof(msg), 
            "SHIP TRACKER: SHIP SEARCH\n"
            "SHIP TRACKER: Ship count: %d\n",
            shipCount);
        
        if (shipCount > 0 && gShipLat && gShipLon && gShipX && gShipY && gShipZ) {
            // Read individual values instead of arrays for simplicity
            for (int i = 0; i < shipCount && i < 20; i++) {
                double shipLat = XPLMGetDatad(gShipLat); // Note: might need array indexing
                double shipLon = XPLMGetDatad(gShipLon);
                float shipX = XPLMGetDataf(gShipX);
                float shipY = XPLMGetDataf(gShipY);
                float shipZ = XPLMGetDataf(gShipZ);
                
                char shipMsg[256];
                snprintf(shipMsg, sizeof(shipMsg), 
                    "SHIP TRACKER: Ship[%d]: GPS(%.6f,%.6f) Local(%.0f,%.0f,%.0f)\n",
                    i, shipLat, shipLon, shipX, shipY, shipZ);
                XPLMDebugString(shipMsg);
            }
        }
    } else {
        snprintf(msg, sizeof(msg), 
            "SHIP TRACKER: Ship-specific datarefs NOT FOUND\n"
            "SHIP TRACKER: Ships might be in AI traffic datarefs instead\n");
    }
    
    XPLMDebugString(msg);
}

static void FindAITrafficCallback(void* inRefcon)
{
    char msg[512];
    
    if (gAICount) {
        int aiCount = XPLMGetDatai(gAICount);
        snprintf(msg, sizeof(msg), 
            "SHIP TRACKER: AI TRAFFIC SEARCH\n"
            "SHIP TRACKER: AI traffic count: %d\n",
            aiCount);
        
        if (aiCount > 0 && gAILat && gAILon && gAIX && gAIY && gAIZ) {
            float aiLat[20];
            float aiLon[20];
            float aiAlt[20];
            float aiX[20];
            float aiY[20];
            float aiZ[20];
            
            // Use XPLMGetDatavf for all float arrays
            XPLMGetDatavf(gAILat, aiLat, 0, aiCount);
            XPLMGetDatavf(gAILon, aiLon, 0, aiCount);
            XPLMGetDatavf(gAIAlt, aiAlt, 0, aiCount);
            XPLMGetDatavf(gAIX, aiX, 0, aiCount);
            XPLMGetDatavf(gAIY, aiY, 0, aiCount);
            XPLMGetDatavf(gAIZ, aiZ, 0, aiCount);
            
            for (int i = 0; i < aiCount && i < 20; i++) {
                // Check if this might be a ship (low altitude)
                char aiMsg[256];
                const char* type = (aiAlt[i] < 100.0f) ? "SHIP/GROUND" : "AIRCRAFT";
                snprintf(aiMsg, sizeof(aiMsg), 
                    "SHIP TRACKER: AI[%d] %s: GPS(%.6f,%.6f) Alt:%.0f Local(%.0f,%.0f,%.0f)\n",
                    i, type, aiLat[i], aiLon[i], aiAlt[i], aiX[i], aiY[i], aiZ[i]);
                XPLMDebugString(aiMsg);
            }
        }
    } else {
        snprintf(msg, sizeof(msg), 
            "SHIP TRACKER: AI traffic datarefs NOT FOUND\n");
    }
    
    XPLMDebugString(msg);
}

static void FindGroundVehiclesCallback(void* inRefcon)
{
    char msg[512];
    
    if (gGroundVehicleCount) {
        int vehicleCount = XPLMGetDatai(gGroundVehicleCount);
        snprintf(msg, sizeof(msg), 
            "SHIP TRACKER: GROUND VEHICLE SEARCH\n"
            "SHIP TRACKER: Ground vehicle count: %d\n",
            vehicleCount);
        
        // Process ground vehicles if found
        if (vehicleCount > 0) {
            snprintf(msg, sizeof(msg), 
                "SHIP TRACKER: Found %d ground vehicles - implement processing\n",
                vehicleCount);
        }
    } else {
        snprintf(msg, sizeof(msg), 
            "SHIP TRACKER: Ground vehicle datarefs NOT FOUND\n");
    }
    
    XPLMDebugString(msg);
}

static void StartShipMonitoringCallback(void* inRefcon)
{
    if (gMonitoringActive) {
        XPLMDebugString("SHIP TRACKER: Monitoring already active\n");
        return;
    }
    
    gMonitoringActive = 1;
    
    if (!gMonitorLoop) {
        XPLMCreateFlightLoop_t flightLoopParams;
        flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
        flightLoopParams.phase = xplm_FlightLoop_Phase_AfterFlightModel;
        flightLoopParams.callbackFunc = ShipMonitoringFlightLoopCallback;
        flightLoopParams.refcon = NULL;
        
        gMonitorLoop = XPLMCreateFlightLoop(&flightLoopParams);
    }
    
    if (gMonitorLoop) {
        XPLMScheduleFlightLoop(gMonitorLoop, 5.0f, 1); // Check every 5 seconds
        XPLMDebugString("SHIP TRACKER: Started monitoring ships and AI traffic\n");
    }
}

static void StopShipMonitoringCallback(void* inRefcon)
{
    if (!gMonitoringActive) {
        XPLMDebugString("SHIP TRACKER: Monitoring not active\n");
        return;
    }
    
    gMonitoringActive = 0;
    if (gMonitorLoop) {
        XPLMScheduleFlightLoop(gMonitorLoop, 0, 0);
        XPLMDebugString("SHIP TRACKER: Stopped monitoring\n");
    }
}

static void TargetNearestShipCallback(void* inRefcon)
{
    // Find nearest ship/AI traffic and provide coordinates for targeting
    if (!gAircraftX || !gAircraftY || !gAircraftZ) {
        XPLMDebugString("SHIP TRACKER: Aircraft position not available\n");
        return;
    }
    
    float aircraftX = XPLMGetDataf(gAircraftX);
    float aircraftY = XPLMGetDataf(gAircraftY);
    float aircraftZ = XPLMGetDataf(gAircraftZ);
    
    float nearestDistance = 999999.0f;
    float nearestX = 0.0f, nearestY = 0.0f, nearestZ = 0.0f;
    int nearestFound = 0;
    
    // Check AI traffic for ships (low altitude objects)
    if (gAICount && gAIX && gAIY && gAIZ && gAIAlt) {
        int aiCount = XPLMGetDatai(gAICount);
        if (aiCount > 0) {
            float aiX[20], aiY[20], aiZ[20], aiAlt[20];
            int readCount = (aiCount < 20) ? aiCount : 20;
            
            XPLMGetDatavf(gAIX, aiX, 0, readCount);
            XPLMGetDatavf(gAIY, aiY, 0, readCount);
            XPLMGetDatavf(gAIZ, aiZ, 0, readCount);
            XPLMGetDatavf(gAIAlt, aiAlt, 0, readCount);
            
            for (int i = 0; i < readCount; i++) {
                // Consider low altitude objects as potential ships
                if (aiAlt[i] < 100.0f) {
                    float distance = sqrt((aiX[i] - aircraftX) * (aiX[i] - aircraftX) + 
                                        (aiY[i] - aircraftY) * (aiY[i] - aircraftY) + 
                                        (aiZ[i] - aircraftZ) * (aiZ[i] - aircraftZ));
                    
                    if (distance < nearestDistance) {
                        nearestDistance = distance;
                        nearestX = aiX[i];
                        nearestY = aiY[i];
                        nearestZ = aiZ[i];
                        nearestFound = 1;
                    }
                }
            }
        }
    }
    
    if (nearestFound) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "SHIP TRACKER: NEAREST TARGET FOUND\n"
            "SHIP TRACKER: Target at (%.0f, %.0f, %.0f) - Distance: %.0fm\n"
            "SHIP TRACKER: Use these coordinates for missile targeting!\n",
            nearestX, nearestY, nearestZ, nearestDistance);
        XPLMDebugString(msg);
    } else {
        XPLMDebugString("SHIP TRACKER: No ships/targets found - enable AI traffic in X-Plane\n");
    }
}

static float ShipMonitoringFlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    if (!gMonitoringActive) return 0;
    
    // Periodically report ship positions
    static int reportCounter = 0;
    reportCounter++;
    
    if (reportCounter >= 6) { // Every 30 seconds (6 * 5 seconds)
        TargetNearestShipCallback(NULL);
        reportCounter = 0;
    }
    
    return 5.0f; // Check again in 5 seconds
}