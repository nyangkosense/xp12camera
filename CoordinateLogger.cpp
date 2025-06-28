/*
 * CoordinateLogger.cpp
 * 
 * Simple coordinate logging to file
 * Press a key to dump all coordinate data to a text file
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// Camera and aircraft datarefs
static XPLMDataRef gCameraX = NULL;
static XPLMDataRef gCameraY = NULL;
static XPLMDataRef gCameraZ = NULL;
static XPLMDataRef gCameraHeading = NULL;
static XPLMDataRef gCameraPitch = NULL;
static XPLMDataRef gCameraRoll = NULL;

static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftLat = NULL;
static XPLMDataRef gAircraftLon = NULL;
static XPLMDataRef gAircraftHeading = NULL;

// Screen and mouse datarefs (if available)
static XPLMDataRef gScreenWidth = NULL;
static XPLMDataRef gScreenHeight = NULL;

static void LogCoordinatesToFile(void* inRefcon);
static void LogRealtimeCoordinates(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Coordinate Logger");
    strcpy(outSig, "coordinate.logger");
    strcpy(outDesc, "Log coordinate data to text file");

    // Initialize datarefs
    gCameraX = XPLMFindDataRef("sim/graphics/view/view_x");
    gCameraY = XPLMFindDataRef("sim/graphics/view/view_y");
    gCameraZ = XPLMFindDataRef("sim/graphics/view/view_z");
    gCameraHeading = XPLMFindDataRef("sim/graphics/view/view_heading");
    gCameraPitch = XPLMFindDataRef("sim/graphics/view/view_pitch");
    gCameraRoll = XPLMFindDataRef("sim/graphics/view/view_roll");
    
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftLat = XPLMFindDataRef("sim/flightmodel/position/latitude");
    gAircraftLon = XPLMFindDataRef("sim/flightmodel/position/longitude");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    
    gScreenWidth = XPLMFindDataRef("sim/graphics/view/window_width");
    gScreenHeight = XPLMFindDataRef("sim/graphics/view/window_height");

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_F10, xplm_DownFlag, "Log: Dump Coordinates", LogCoordinatesToFile, NULL);
    XPLMRegisterHotKey(XPLM_VK_F11, xplm_DownFlag, "Log: Realtime Coords", LogRealtimeCoordinates, NULL);

    XPLMDebugString("COORDINATE LOGGER: Plugin loaded\n");
    XPLMDebugString("COORDINATE LOGGER: F10=Log to file, F11=Realtime display\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void LogCoordinatesToFile(void* inRefcon)
{
    FILE* file = fopen("coordinate_dump.txt", "a");
    if (!file) {
        XPLMDebugString("COORDINATE LOGGER: Failed to open coordinate_dump.txt\n");
        return;
    }
    
    // Get current time
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // Get all coordinate data
    float camX = gCameraX ? XPLMGetDataf(gCameraX) : 0.0f;
    float camY = gCameraY ? XPLMGetDataf(gCameraY) : 0.0f;
    float camZ = gCameraZ ? XPLMGetDataf(gCameraZ) : 0.0f;
    float camHeading = gCameraHeading ? XPLMGetDataf(gCameraHeading) : 0.0f;
    float camPitch = gCameraPitch ? XPLMGetDataf(gCameraPitch) : 0.0f;
    float camRoll = gCameraRoll ? XPLMGetDataf(gCameraRoll) : 0.0f;
    
    float acX = gAircraftX ? XPLMGetDataf(gAircraftX) : 0.0f;
    float acY = gAircraftY ? XPLMGetDataf(gAircraftY) : 0.0f;
    float acZ = gAircraftZ ? XPLMGetDataf(gAircraftZ) : 0.0f;
    double acLat = gAircraftLat ? XPLMGetDatad(gAircraftLat) : 0.0;
    double acLon = gAircraftLon ? XPLMGetDatad(gAircraftLon) : 0.0;
    float acHeading = gAircraftHeading ? XPLMGetDataf(gAircraftHeading) : 0.0f;
    
    int screenW = gScreenWidth ? (int)XPLMGetDataf(gScreenWidth) : 0;
    int screenH = gScreenHeight ? (int)XPLMGetDataf(gScreenHeight) : 0;
    
    // Get mouse position
    int mouseX, mouseY;
    XPLMGetMouseLocation(&mouseX, &mouseY);
    
    // Calculate ray-ground intersection
    float headingRad = camHeading * M_PI / 180.0f;
    float pitchRad = camPitch * M_PI / 180.0f;
    
    float rayX = sin(headingRad) * cos(pitchRad);
    float rayY = sin(pitchRad);
    float rayZ = cos(headingRad) * cos(pitchRad);
    
    float t = (rayY != 0.0f) ? (-camY / rayY) : 10000.0f;
    
    float hitX = camX + rayX * t;
    float hitY = 0.0f;
    float hitZ = camZ + rayZ * t;
    
    // Write to file
    fprintf(file, "\n=== COORDINATE DUMP %s", asctime(timeinfo));
    fprintf(file, "CAMERA: Position (%.2f, %.2f, %.2f)\n", camX, camY, camZ);
    fprintf(file, "CAMERA: Angles (%.2f°, %.2f°, %.2f°)\n", camHeading, camPitch, camRoll);
    fprintf(file, "AIRCRAFT: Position (%.2f, %.2f, %.2f)\n", acX, acY, acZ);
    fprintf(file, "AIRCRAFT: GPS (%.6f, %.6f) Heading %.2f°\n", acLat, acLon, acHeading);
    fprintf(file, "SCREEN: %dx%d Mouse (%d, %d)\n", screenW, screenH, mouseX, mouseY);
    fprintf(file, "RAY_HIT: Ground intersection (%.2f, %.2f, %.2f)\n", hitX, hitY, hitZ);
    fprintf(file, "RAY_DISTANCE: %.2f meters\n", t);
    fprintf(file, "=====================================\n");
    
    fclose(file);
    
    XPLMDebugString("COORDINATE LOGGER: Coordinates logged to coordinate_dump.txt\n");
}

static void LogRealtimeCoordinates(void* inRefcon)
{
    // Get mouse position
    int mouseX, mouseY;
    XPLMGetMouseLocation(&mouseX, &mouseY);
    
    // Get camera data
    float camX = gCameraX ? XPLMGetDataf(gCameraX) : 0.0f;
    float camY = gCameraY ? XPLMGetDataf(gCameraY) : 0.0f;
    float camZ = gCameraZ ? XPLMGetDataf(gCameraZ) : 0.0f;
    float camHeading = gCameraHeading ? XPLMGetDataf(gCameraHeading) : 0.0f;
    float camPitch = gCameraPitch ? XPLMGetDataf(gCameraPitch) : 0.0f;
    
    int screenW = gScreenWidth ? (int)XPLMGetDataf(gScreenWidth) : 0;
    int screenH = gScreenHeight ? (int)XPLMGetDataf(gScreenHeight) : 0;
    
    // Calculate ray hit
    float headingRad = camHeading * M_PI / 180.0f;
    float pitchRad = camPitch * M_PI / 180.0f;
    
    float rayX = sin(headingRad) * cos(pitchRad);
    float rayY = sin(pitchRad);
    float rayZ = cos(headingRad) * cos(pitchRad);
    
    float t = (rayY != 0.0f) ? (-camY / rayY) : 10000.0f;
    
    float hitX = camX + rayX * t;
    float hitY = 0.0f;
    float hitZ = camZ + rayZ * t;
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "REALTIME: Screen %dx%d Mouse(%d,%d) Camera(%.0f,%.0f,%.0f) Angles(%.1f°,%.1f°) → Hit(%.0f,%.0f,%.0f)\n",
        screenW, screenH, mouseX, mouseY, camX, camY, camZ, camHeading, camPitch, hitX, hitY, hitZ);
    
    XPLMDebugString(msg);
}