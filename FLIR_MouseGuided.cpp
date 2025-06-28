/*
 * FLIR_MouseGuided.cpp
 * 
 * Direct mouse control for weapon guidance - steer missiles like a joystick
 * Much more intuitive than trying to calculate target coordinates
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

// Flight loop
static XPLMFlightLoopID gFlightLoopID = NULL;

// Mouse control state
static bool gMouseGuidanceActive = false;
static int gLastMouseX = 0;
static int gLastMouseY = 0;
static int gScreenCenterX = 0;
static int gScreenCenterY = 0;
static bool gMouseCentered = false;

// Weapon arrays
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;
static XPLMDataRef gWeaponCount = NULL;

// Aircraft position for reference
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;

// FLIR camera status
static XPLMDataRef gCameraActiveDataRef = NULL;

// Control sensitivity
static float gMouseSensitivity = 2.0f;
static float gMaxSteeringForce = 50.0f; // m/s steering adjustment
static float gWeaponSpeed = 150.0f; // Base weapon speed

// Function prototypes
static void ActivateMouseGuidance(void* inRefcon);
static void DeactivateMouseGuidance(void* inRefcon);
static void IncreaseMouseSensitivity(void* inRefcon);
static void DecreaseMouseSensitivity(void* inRefcon);
static float MouseGuidanceFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static void ApplyMouseSteering(float deltaX, float deltaY);
static void DebugMouseGuidance(void);
static void CenterMouse(void);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Mouse Guided Weapons");
    strcpy(outSig, "flir.mouse.guided");
    strcpy(outDesc, "Direct mouse control for weapon steering");

    // Find weapon system datarefs
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    
    // Aircraft position
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    
    // FLIR camera status
    gCameraActiveDataRef = XPLMFindDataRef("flir/camera/active");
    
    // Get screen dimensions
    XPLMGetScreenSize(&gScreenCenterX, &gScreenCenterY);
    gScreenCenterX /= 2;
    gScreenCenterY /= 2;
    
    // Debug dataref availability
    XPLMDebugString("MOUSE GUIDANCE: Checking dataref availability...\\n");
    
    if (!gWeaponX || !gWeaponY || !gWeaponZ) {
        XPLMDebugString("MOUSE GUIDANCE: WARNING - Weapon position datarefs not found!\\n");
    } else {
        XPLMDebugString("MOUSE GUIDANCE: Weapon position datarefs found\\n");
    }
    
    if (!gWeaponVX || !gWeaponVY || !gWeaponVZ) {
        XPLMDebugString("MOUSE GUIDANCE: WARNING - Weapon velocity datarefs not found!\\n");
    } else {
        XPLMDebugString("MOUSE GUIDANCE: Weapon velocity datarefs found\\n");
    }

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_M, xplm_DownFlag, "Mouse: Activate Guidance", ActivateMouseGuidance, NULL);
    XPLMRegisterHotKey(XPLM_VK_N, xplm_DownFlag, "Mouse: Deactivate Guidance", DeactivateMouseGuidance, NULL);
    XPLMRegisterHotKey(XPLM_VK_PERIOD, xplm_DownFlag, "Mouse: Increase Sensitivity", IncreaseMouseSensitivity, NULL);
    XPLMRegisterHotKey(XPLM_VK_COMMA, xplm_DownFlag, "Mouse: Decrease Sensitivity", DecreaseMouseSensitivity, NULL);
    
    // Register flight loop
    XPLMCreateFlightLoop_t flightLoopParams;
    flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
    flightLoopParams.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    flightLoopParams.callbackFunc = MouseGuidanceFlightLoop;
    flightLoopParams.refcon = NULL;
    
    gFlightLoopID = XPLMCreateFlightLoop(&flightLoopParams);
    if (gFlightLoopID) {
        XPLMScheduleFlightLoop(gFlightLoopID, 0.02f, 1); // 50Hz for responsive control
        XPLMDebugString("MOUSE GUIDANCE: Flight loop created and scheduled\\n");
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "MOUSE GUIDANCE: Screen center: (%d, %d)\\n", gScreenCenterX, gScreenCenterY);
    XPLMDebugString(msg);

    XPLMDebugString("MOUSE GUIDANCE: Plugin loaded successfully\\n");
    XPLMDebugString("MOUSE GUIDANCE: M=Activate, N=Deactivate, ,/. = Sensitivity\\n");

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gFlightLoopID) {
        XPLMDestroyFlightLoop(gFlightLoopID);
        gFlightLoopID = NULL;
    }
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

// Center mouse cursor on screen
static void CenterMouse(void)
{
    // Note: X-Plane doesn't provide a way to set mouse position
    // User will need to manually center mouse when activating
    XPLMGetMouseLocation(&gLastMouseX, &gLastMouseY);
    gMouseCentered = true;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "MOUSE GUIDANCE: Mouse position recorded: (%d, %d)\\n", gLastMouseX, gLastMouseY);
    XPLMDebugString(msg);
}

// Activate mouse guidance
static void ActivateMouseGuidance(void* inRefcon)
{
    if (gMouseGuidanceActive) {
        XPLMDebugString("MOUSE GUIDANCE: Already active\\n");
        return;
    }
    
    // Check if FLIR camera is active
    if (gCameraActiveDataRef && XPLMGetDatai(gCameraActiveDataRef)) {
        XPLMDebugString("MOUSE GUIDANCE: FLIR camera is active - good for guidance\\n");
    } else {
        XPLMDebugString("MOUSE GUIDANCE: WARNING - FLIR camera not active, but proceeding anyway\\n");
    }
    
    gMouseGuidanceActive = true;
    CenterMouse();
    
    char msg[256];
    snprintf(msg, sizeof(msg), "MOUSE GUIDANCE: *** ACTIVATED *** Sensitivity: %.1f\\n", gMouseSensitivity);
    XPLMDebugString(msg);
    XPLMDebugString("MOUSE GUIDANCE: Move mouse to steer missiles. Center mouse position is neutral.\\n");
}

// Deactivate mouse guidance
static void DeactivateMouseGuidance(void* inRefcon)
{
    if (!gMouseGuidanceActive) {
        XPLMDebugString("MOUSE GUIDANCE: Already inactive\\n");
        return;
    }
    
    gMouseGuidanceActive = false;
    gMouseCentered = false;
    
    XPLMDebugString("MOUSE GUIDANCE: *** DEACTIVATED ***\\n");
}

// Increase mouse sensitivity
static void IncreaseMouseSensitivity(void* inRefcon)
{
    gMouseSensitivity = fminf(gMouseSensitivity + 0.5f, 10.0f);
    char msg[256];
    snprintf(msg, sizeof(msg), "MOUSE GUIDANCE: Sensitivity increased to %.1f\\n", gMouseSensitivity);
    XPLMDebugString(msg);
}

// Decrease mouse sensitivity  
static void DecreaseMouseSensitivity(void* inRefcon)
{
    gMouseSensitivity = fmaxf(gMouseSensitivity - 0.5f, 0.1f);
    char msg[256];
    snprintf(msg, sizeof(msg), "MOUSE GUIDANCE: Sensitivity decreased to %.1f\\n", gMouseSensitivity);
    XPLMDebugString(msg);
}

// Apply mouse movement to weapon steering
static void ApplyMouseSteering(float deltaX, float deltaY)
{
    if (!gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) {
        return;
    }
    
    // Get weapon data
    float weaponX[10], weaponY[10], weaponZ[10];
    float weaponVX[10], weaponVY[10], weaponVZ[10];
    
    int numWeapons = XPLMGetDatavf(gWeaponX, weaponX, 0, 10);
    XPLMGetDatavf(gWeaponY, weaponY, 0, 10);
    XPLMGetDatavf(gWeaponZ, weaponZ, 0, 10);
    XPLMGetDatavf(gWeaponVX, weaponVX, 0, 10);
    XPLMGetDatavf(gWeaponVY, weaponVY, 0, 10);
    XPLMGetDatavf(gWeaponVZ, weaponVZ, 0, 10);
    
    // Get aircraft heading for relative steering
    float acHeading = 0.0f;
    if (gAircraftHeading) {
        acHeading = XPLMGetDataf(gAircraftHeading);
    }
    float headingRad = acHeading * M_PI / 180.0f;
    
    // Convert mouse movement to world coordinates
    // deltaX: positive = right, negative = left
    // deltaY: positive = up, negative = down
    
    // Calculate steering forces in aircraft-relative coordinates
    float steerRight = deltaX * gMouseSensitivity; // Right/left steering
    float steerUp = deltaY * gMouseSensitivity;    // Up/down steering
    
    // Convert to world coordinates
    float steerX = steerRight * cos(headingRad) - 0 * sin(headingRad); // East/West component
    float steerY = steerUp; // Vertical component
    float steerZ = steerRight * sin(headingRad) + 0 * cos(headingRad); // North/South component
    
    bool foundWeapon = false;
    
    // Apply steering to all active weapons
    for (int i = 0; i < numWeapons; i++) {
        // Check if weapon exists
        if (weaponX[i] != 0.0f || weaponY[i] != 0.0f || weaponZ[i] != 0.0f) {
            foundWeapon = true;
            
            // Maintain base speed while adding steering
            float currentSpeed = sqrt(weaponVX[i]*weaponVX[i] + weaponVY[i]*weaponVY[i] + weaponVZ[i]*weaponVZ[i]);
            
            // If weapon is too slow, boost it
            if (currentSpeed < 50.0f) {
                currentSpeed = gWeaponSpeed;
            }
            
            // Apply steering as velocity adjustments
            weaponVX[i] += steerX;
            weaponVY[i] += steerY;
            weaponVZ[i] += steerZ;
            
            // Limit steering magnitude
            float totalSpeed = sqrt(weaponVX[i]*weaponVX[i] + weaponVY[i]*weaponVY[i] + weaponVZ[i]*weaponVZ[i]);
            if (totalSpeed > gWeaponSpeed * 1.5f) {
                float scale = (gWeaponSpeed * 1.5f) / totalSpeed;
                weaponVX[i] *= scale;
                weaponVY[i] *= scale;
                weaponVZ[i] *= scale;
            }
        }
    }
    
    if (foundWeapon) {
        // Update weapon velocities
        XPLMSetDatavf(gWeaponVX, weaponVX, 0, numWeapons);
        XPLMSetDatavf(gWeaponVY, weaponVY, 0, numWeapons);
        XPLMSetDatavf(gWeaponVZ, weaponVZ, 0, numWeapons);
        
        // Debug output (limited)
        static float debugTimer = 0.0f;
        debugTimer += 0.02f; // Flight loop interval
        if (debugTimer >= 1.0f) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "MOUSE GUIDANCE: Steering dX=%.1f dY=%.1f -> force(%.1f,%.1f,%.1f)\\n",
                deltaX, deltaY, steerX, steerY, steerZ);
            XPLMDebugString(msg);
            debugTimer = 0.0f;
        }
    }
}

// Debug current guidance state
static void DebugMouseGuidance(void)
{
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "MOUSE GUIDANCE: Active=%s, Sensitivity=%.1f, Mouse=(%d,%d), Center=(%d,%d)\\n",
        gMouseGuidanceActive ? "YES" : "NO", gMouseSensitivity,
        gLastMouseX, gLastMouseY, gScreenCenterX, gScreenCenterY);
    XPLMDebugString(msg);
    
    // Check weapons
    if (gWeaponX && gWeaponY && gWeaponZ) {
        float weaponX[5], weaponY[5], weaponZ[5];
        int numRead = XPLMGetDatavf(gWeaponX, weaponX, 0, 5);
        XPLMGetDatavf(gWeaponY, weaponY, 0, 5);
        XPLMGetDatavf(gWeaponZ, weaponZ, 0, 5);
        
        int activeWeapons = 0;
        for (int i = 0; i < numRead; i++) {
            if (weaponX[i] != 0.0f || weaponY[i] != 0.0f || weaponZ[i] != 0.0f) {
                activeWeapons++;
            }
        }
        
        snprintf(msg, sizeof(msg), "MOUSE GUIDANCE: Active weapons: %d\\n", activeWeapons);
        XPLMDebugString(msg);
    }
}

// Main flight loop for mouse guidance
static float MouseGuidanceFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    // Debug every 5 seconds
    static float debugTimer = 0.0f;
    debugTimer += inElapsedSinceLastCall;
    if (debugTimer >= 5.0f) {
        if (gMouseGuidanceActive) {
            DebugMouseGuidance();
        }
        debugTimer = 0.0f;
    }
    
    // Only process if mouse guidance is active
    if (!gMouseGuidanceActive || !gMouseCentered) {
        return 0.02f; // Continue at 50Hz
    }
    
    // Get current mouse position
    int currentMouseX, currentMouseY;
    XPLMGetMouseLocation(&currentMouseX, &currentMouseY);
    
    // Calculate mouse movement delta
    float deltaX = (float)(currentMouseX - gLastMouseX);
    float deltaY = (float)(currentMouseY - gLastMouseY);
    
    // Apply deadzone
    if (fabs(deltaX) < 2.0f) deltaX = 0.0f;
    if (fabs(deltaY) < 2.0f) deltaY = 0.0f;
    
    // Apply steering if there's movement
    if (deltaX != 0.0f || deltaY != 0.0f) {
        ApplyMouseSteering(deltaX, deltaY);
    }
    
    // Update last mouse position
    gLastMouseX = currentMouseX;
    gLastMouseY = currentMouseY;
    
    return 0.02f; // Continue at 50Hz for responsive control
}