/*
 * FLIR_HybridGuidance.cpp
 * 
 * Combines mouse steering with automatic FLIR crosshair guidance
 * - Auto mode: Missiles fly toward FLIR crosshair center
 * - Manual mode: Direct mouse control
 * - Hybrid mode: Auto + manual fine adjustments
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

// Guidance modes
typedef enum {
    GUIDANCE_OFF = 0,
    GUIDANCE_AUTO_CROSSHAIR = 1,    // Auto-steer to crosshair
    GUIDANCE_MANUAL_MOUSE = 2,      // Manual mouse control
    GUIDANCE_HYBRID = 3             // Auto + manual override
} GuidanceMode;

static GuidanceMode gGuidanceMode = GUIDANCE_OFF;

// Mouse control state
static int gLastMouseX = 0;
static int gLastMouseY = 0;
static bool gMouseCentered = false;

// FLIR camera datarefs
static XPLMDataRef gCameraPanDataRef = NULL;
static XPLMDataRef gCameraTiltDataRef = NULL;
static XPLMDataRef gCameraActiveDataRef = NULL;

// X-Plane screen center world coordinates
static XPLMDataRef gClick3DX = NULL;
static XPLMDataRef gClick3DY = NULL;
static XPLMDataRef gClick3DZ = NULL;

// Weapon arrays
static XPLMDataRef gWeaponX = NULL;
static XPLMDataRef gWeaponY = NULL;
static XPLMDataRef gWeaponZ = NULL;
static XPLMDataRef gWeaponVX = NULL;
static XPLMDataRef gWeaponVY = NULL;
static XPLMDataRef gWeaponVZ = NULL;

// Aircraft position and orientation
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;
static XPLMDataRef gAircraftPitch = NULL;
static XPLMDataRef gAircraftRoll = NULL;

// Control parameters
static float gMouseSensitivity = 1.5f;
static float gAutoGuidanceStrength = 0.3f; // How aggressively to steer toward crosshair
static float gMaxSteeringForce = 40.0f;
static float gWeaponSpeed = 120.0f;

// Target coordinates (calculated from FLIR)
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static bool gTargetValid = false;

// Function prototypes
static void SetGuidanceMode(GuidanceMode mode);
static void ToggleGuidanceMode(void* inRefcon);
static void ActivateAutoCrosshair(void* inRefcon);
static void ActivateManualMouse(void* inRefcon);
static void ActivateHybridMode(void* inRefcon);
static void DeactivateGuidance(void* inRefcon);
static void IncreaseSensitivity(void* inRefcon);
static void DecreaseSensitivity(void* inRefcon);
static void IncreaseAutoGuidance(void* inRefcon);
static void DecreaseAutoGuidance(void* inRefcon);

static float HybridGuidanceFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
static bool CalculateCrosshairTarget(float* outX, float* outY, float* outZ);
static void ApplyAutoCrosshairGuidance(float deltaTime);
static void ApplyMouseSteering(float deltaX, float deltaY);
static void ApplyHybridGuidance(float deltaTime, float mouseDeltaX, float mouseDeltaY);
static void DebugGuidanceState(void);
static void CenterMouse(void);

static const char* GetModeString(GuidanceMode mode);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Hybrid Guidance System");
    strcpy(outSig, "flir.hybrid.guidance");
    strcpy(outDesc, "Auto crosshair + manual mouse weapon guidance");

    // Find FLIR camera datarefs
    gCameraPanDataRef = XPLMFindDataRef("flir/camera/pan");
    gCameraTiltDataRef = XPLMFindDataRef("flir/camera/tilt");
    gCameraActiveDataRef = XPLMFindDataRef("flir/camera/active");
    
    // Find X-Plane screen center world coordinate datarefs
    gClick3DX = XPLMFindDataRef("sim/graphics/view/click_3d_x");
    gClick3DY = XPLMFindDataRef("sim/graphics/view/click_3d_y"); 
    gClick3DZ = XPLMFindDataRef("sim/graphics/view/click_3d_z");

    // Find weapon system datarefs
    gWeaponX = XPLMFindDataRef("sim/weapons/x");
    gWeaponY = XPLMFindDataRef("sim/weapons/y");
    gWeaponZ = XPLMFindDataRef("sim/weapons/z");
    gWeaponVX = XPLMFindDataRef("sim/weapons/vx");
    gWeaponVY = XPLMFindDataRef("sim/weapons/vy");
    gWeaponVZ = XPLMFindDataRef("sim/weapons/vz");
    
    // Aircraft position and orientation
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    gAircraftPitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    gAircraftRoll = XPLMFindDataRef("sim/flightmodel/position/phi");
    
    // Debug dataref availability
    XPLMDebugString("HYBRID GUIDANCE: Checking dataref availability...\\n");
    
    if (!gCameraPanDataRef || !gCameraTiltDataRef || !gCameraActiveDataRef) {
        XPLMDebugString("HYBRID GUIDANCE: WARNING - FLIR camera datarefs not found!\\n");
    } else {
        XPLMDebugString("HYBRID GUIDANCE: FLIR camera datarefs found\\n");
    }
    
    if (!gWeaponX || !gWeaponY || !gWeaponZ || !gWeaponVX || !gWeaponVY || !gWeaponVZ) {
        XPLMDebugString("HYBRID GUIDANCE: WARNING - Weapon datarefs not found!\\n");
    } else {
        XPLMDebugString("HYBRID GUIDANCE: Weapon datarefs found\\n");
    }

    // Register hotkeys
    XPLMRegisterHotKey(XPLM_VK_1, xplm_DownFlag, "Hybrid: Auto Crosshair Mode", ActivateAutoCrosshair, NULL);
    XPLMRegisterHotKey(XPLM_VK_2, xplm_DownFlag, "Hybrid: Manual Mouse Mode", ActivateManualMouse, NULL);
    XPLMRegisterHotKey(XPLM_VK_3, xplm_DownFlag, "Hybrid: Combined Mode", ActivateHybridMode, NULL);
    XPLMRegisterHotKey(XPLM_VK_0, xplm_DownFlag, "Hybrid: Deactivate", DeactivateGuidance, NULL);
    XPLMRegisterHotKey(XPLM_VK_TAB, xplm_DownFlag, "Hybrid: Toggle Mode", ToggleGuidanceMode, NULL);
    XPLMRegisterHotKey(XPLM_VK_PERIOD, xplm_DownFlag, "Hybrid: Increase Mouse Sensitivity", IncreaseSensitivity, NULL);
    XPLMRegisterHotKey(XPLM_VK_COMMA, xplm_DownFlag, "Hybrid: Decrease Mouse Sensitivity", DecreaseSensitivity, NULL);
    XPLMRegisterHotKey(XPLM_VK_RBRACE, xplm_DownFlag, "Hybrid: Increase Auto Strength", IncreaseAutoGuidance, NULL);
    XPLMRegisterHotKey(XPLM_VK_LBRACE, xplm_DownFlag, "Hybrid: Decrease Auto Strength", DecreaseAutoGuidance, NULL);
    
    // Register flight loop
    XPLMCreateFlightLoop_t flightLoopParams;
    flightLoopParams.structSize = sizeof(XPLMCreateFlightLoop_t);
    flightLoopParams.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    flightLoopParams.callbackFunc = HybridGuidanceFlightLoop;
    flightLoopParams.refcon = NULL;
    
    gFlightLoopID = XPLMCreateFlightLoop(&flightLoopParams);
    if (gFlightLoopID) {
        XPLMScheduleFlightLoop(gFlightLoopID, 0.02f, 1); // 50Hz
        XPLMDebugString("HYBRID GUIDANCE: Flight loop created and scheduled\\n");
    }

    XPLMDebugString("HYBRID GUIDANCE: Plugin loaded successfully\\n");
    XPLMDebugString("HYBRID GUIDANCE: 1=Auto, 2=Manual, 3=Hybrid, 0=Off, TAB=Toggle\\n");
    XPLMDebugString("HYBRID GUIDANCE: ,/. = Mouse sensitivity, {/} = Auto strength\\n");

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

// Get mode name for debugging
static const char* GetModeString(GuidanceMode mode)
{
    switch (mode) {
        case GUIDANCE_OFF: return "OFF";
        case GUIDANCE_AUTO_CROSSHAIR: return "AUTO_CROSSHAIR";
        case GUIDANCE_MANUAL_MOUSE: return "MANUAL_MOUSE";
        case GUIDANCE_HYBRID: return "HYBRID";
        default: return "UNKNOWN";
    }
}

// Set guidance mode with logging
static void SetGuidanceMode(GuidanceMode mode)
{
    if (mode == gGuidanceMode) return;
    
    gGuidanceMode = mode;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "HYBRID GUIDANCE: Mode changed to %s\\n", GetModeString(mode));
    XPLMDebugString(msg);
    
    // Reset mouse when switching to modes that need it
    if (mode == GUIDANCE_MANUAL_MOUSE || mode == GUIDANCE_HYBRID) {
        CenterMouse();
    }
}

// Center mouse cursor position
static void CenterMouse(void)
{
    XPLMGetMouseLocation(&gLastMouseX, &gLastMouseY);
    gMouseCentered = true;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "HYBRID GUIDANCE: Mouse position centered at (%d, %d)\\n", gLastMouseX, gLastMouseY);
    XPLMDebugString(msg);
}

// Mode activation functions
static void ActivateAutoCrosshair(void* inRefcon)
{
    SetGuidanceMode(GUIDANCE_AUTO_CROSSHAIR);
}

static void ActivateManualMouse(void* inRefcon)
{
    SetGuidanceMode(GUIDANCE_MANUAL_MOUSE);
}

static void ActivateHybridMode(void* inRefcon)
{
    SetGuidanceMode(GUIDANCE_HYBRID);
}

static void DeactivateGuidance(void* inRefcon)
{
    SetGuidanceMode(GUIDANCE_OFF);
}

static void ToggleGuidanceMode(void* inRefcon)
{
    GuidanceMode nextMode = (GuidanceMode)((gGuidanceMode + 1) % 4);
    SetGuidanceMode(nextMode);
}

// Sensitivity controls
static void IncreaseSensitivity(void* inRefcon)
{
    gMouseSensitivity = fminf(gMouseSensitivity + 0.2f, 5.0f);
    char msg[128];
    snprintf(msg, sizeof(msg), "HYBRID GUIDANCE: Mouse sensitivity: %.1f\\n", gMouseSensitivity);
    XPLMDebugString(msg);
}

static void DecreaseSensitivity(void* inRefcon)
{
    gMouseSensitivity = fmaxf(gMouseSensitivity - 0.2f, 0.1f);
    char msg[128];
    snprintf(msg, sizeof(msg), "HYBRID GUIDANCE: Mouse sensitivity: %.1f\\n", gMouseSensitivity);
    XPLMDebugString(msg);
}

static void IncreaseAutoGuidance(void* inRefcon)
{
    gAutoGuidanceStrength = fminf(gAutoGuidanceStrength + 0.1f, 1.0f);
    char msg[128];
    snprintf(msg, sizeof(msg), "HYBRID GUIDANCE: Auto strength: %.1f\\n", gAutoGuidanceStrength);
    XPLMDebugString(msg);
}

static void DecreaseAutoGuidance(void* inRefcon)
{
    gAutoGuidanceStrength = fmaxf(gAutoGuidanceStrength - 0.1f, 0.1f);
    char msg[128];
    snprintf(msg, sizeof(msg), "HYBRID GUIDANCE: Auto strength: %.1f\\n", gAutoGuidanceStrength);
    XPLMDebugString(msg);
}

// Calculate where center screen is pointing using X-Plane's built-in coordinates
static bool CalculateCrosshairTarget(float* outX, float* outY, float* outZ)
{
    if (!gCameraActiveDataRef || !XPLMGetDatai(gCameraActiveDataRef)) {
        return false;
    }
    
    // Use X-Plane's built-in screen center world coordinates
    if (!gClick3DX || !gClick3DY || !gClick3DZ) {
        return false;
    }
    
    *outX = XPLMGetDataf(gClick3DX);
    *outY = XPLMGetDataf(gClick3DY);
    *outZ = XPLMGetDataf(gClick3DZ);
    
    // Debug output
    static float debugTimer = 0.0f;
    debugTimer += 0.02f;
    if (debugTimer >= 3.0f) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "HYBRID GUIDANCE: Screen center target -> (%.0f,%.0f,%.0f)\\n",
            *outX, *outY, *outZ);
        XPLMDebugString(msg);
        debugTimer = 0.0f;
    }
    
    return true;
}

// Apply automatic guidance toward crosshair
static void ApplyAutoCrosshairGuidance(float deltaTime)
{
    if (!CalculateCrosshairTarget(&gTargetX, &gTargetY, &gTargetZ)) {
        return;
    }
    
    gTargetValid = true;
    
    // Get weapon data
    float weaponX[10], weaponY[10], weaponZ[10];
    float weaponVX[10], weaponVY[10], weaponVZ[10];
    
    int numWeapons = XPLMGetDatavf(gWeaponX, weaponX, 0, 10);
    XPLMGetDatavf(gWeaponY, weaponY, 0, 10);
    XPLMGetDatavf(gWeaponZ, weaponZ, 0, 10);
    XPLMGetDatavf(gWeaponVX, weaponVX, 0, 10);
    XPLMGetDatavf(gWeaponVY, weaponVY, 0, 10);
    XPLMGetDatavf(gWeaponVZ, weaponVZ, 0, 10);
    
    bool foundWeapon = false;
    
    // Guide weapons toward target
    for (int i = 0; i < numWeapons; i++) {
        if (weaponX[i] != 0.0f || weaponY[i] != 0.0f || weaponZ[i] != 0.0f) {
            foundWeapon = true;
            
            // Calculate direction to target
            float deltaX = gTargetX - weaponX[i];
            float deltaY = gTargetY - weaponY[i];
            float deltaZ = gTargetZ - weaponZ[i];
            float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
            
            if (distance > 10.0f) {
                // Proportional navigation toward crosshair
                float dirX = deltaX / distance;
                float dirY = deltaY / distance;
                float dirZ = deltaZ / distance;
                
                float desiredVX = dirX * gWeaponSpeed;
                float desiredVY = dirY * gWeaponSpeed;
                float desiredVZ = dirZ * gWeaponSpeed;
                
                // Apply steering with adjustable strength
                weaponVX[i] = weaponVX[i] + (desiredVX - weaponVX[i]) * gAutoGuidanceStrength;
                weaponVY[i] = weaponVY[i] + (desiredVY - weaponVY[i]) * gAutoGuidanceStrength;
                weaponVZ[i] = weaponVZ[i] + (desiredVZ - weaponVZ[i]) * gAutoGuidanceStrength;
            }
        }
    }
    
    if (foundWeapon) {
        XPLMSetDatavf(gWeaponVX, weaponVX, 0, numWeapons);
        XPLMSetDatavf(gWeaponVY, weaponVY, 0, numWeapons);
        XPLMSetDatavf(gWeaponVZ, weaponVZ, 0, numWeapons);
    }
}

// Apply manual mouse steering (same as before)
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
    float acHeading = XPLMGetDataf(gAircraftHeading);
    float headingRad = acHeading * M_PI / 180.0f;
    
    // Convert mouse movement to steering forces
    float steerRight = deltaX * gMouseSensitivity;
    float steerUp = deltaY * gMouseSensitivity;
    
    // Convert to world coordinates
    float steerX = steerRight * cos(headingRad);
    float steerY = steerUp;
    float steerZ = steerRight * sin(headingRad);
    
    // Apply to weapons
    for (int i = 0; i < numWeapons; i++) {
        if (weaponX[i] != 0.0f || weaponY[i] != 0.0f || weaponZ[i] != 0.0f) {
            weaponVX[i] += steerX;
            weaponVY[i] += steerY;
            weaponVZ[i] += steerZ;
            
            // Limit speed
            float totalSpeed = sqrt(weaponVX[i]*weaponVX[i] + weaponVY[i]*weaponVY[i] + weaponVZ[i]*weaponVZ[i]);
            if (totalSpeed > gWeaponSpeed * 1.5f) {
                float scale = (gWeaponSpeed * 1.5f) / totalSpeed;
                weaponVX[i] *= scale;
                weaponVY[i] *= scale;
                weaponVZ[i] *= scale;
            }
        }
    }
    
    XPLMSetDatavf(gWeaponVX, weaponVX, 0, numWeapons);
    XPLMSetDatavf(gWeaponVY, weaponVY, 0, numWeapons);
    XPLMSetDatavf(gWeaponVZ, weaponVZ, 0, numWeapons);
}

// Apply hybrid guidance (auto + manual override)
static void ApplyHybridGuidance(float deltaTime, float mouseDeltaX, float mouseDeltaY)
{
    // First apply auto guidance
    ApplyAutoCrosshairGuidance(deltaTime);
    
    // Then apply manual adjustments on top
    if ((fabs(mouseDeltaX) > 2.0f || fabs(mouseDeltaY) > 2.0f) && gMouseCentered) {
        ApplyMouseSteering(mouseDeltaX * 0.5f, mouseDeltaY * 0.5f); // Reduced sensitivity for fine tuning
    }
}

// Debug current state
static void DebugGuidanceState(void)
{
    char msg[512];
    snprintf(msg, sizeof(msg), 
        "HYBRID GUIDANCE: Mode=%s, MouseSens=%.1f, AutoStr=%.1f, Target=%s\\n",
        GetModeString(gGuidanceMode), gMouseSensitivity, gAutoGuidanceStrength, 
        gTargetValid ? "VALID" : "INVALID");
    XPLMDebugString(msg);
    
    if (gTargetValid) {
        snprintf(msg, sizeof(msg), "HYBRID GUIDANCE: Target coords: (%.0f, %.0f, %.0f)\\n", 
            gTargetX, gTargetY, gTargetZ);
        XPLMDebugString(msg);
    }
}

// Main flight loop
static float HybridGuidanceFlightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
    // Debug every 5 seconds
    static float debugTimer = 0.0f;
    debugTimer += inElapsedSinceLastCall;
    if (debugTimer >= 5.0f) {
        if (gGuidanceMode != GUIDANCE_OFF) {
            DebugGuidanceState();
        }
        debugTimer = 0.0f;
    }
    
    // Only process if guidance is active
    if (gGuidanceMode == GUIDANCE_OFF) {
        return 0.02f;
    }
    
    // Get mouse movement for modes that need it
    float mouseDeltaX = 0.0f, mouseDeltaY = 0.0f;
    if (gGuidanceMode == GUIDANCE_MANUAL_MOUSE || gGuidanceMode == GUIDANCE_HYBRID) {
        if (gMouseCentered) {
            int currentMouseX, currentMouseY;
            XPLMGetMouseLocation(&currentMouseX, &currentMouseY);
            
            mouseDeltaX = (float)(currentMouseX - gLastMouseX);
            mouseDeltaY = (float)(currentMouseY - gLastMouseY);
            
            gLastMouseX = currentMouseX;
            gLastMouseY = currentMouseY;
        }
    }
    
    // Apply guidance based on mode
    switch (gGuidanceMode) {
        case GUIDANCE_AUTO_CROSSHAIR:
            ApplyAutoCrosshairGuidance(inElapsedSinceLastCall);
            break;
            
        case GUIDANCE_MANUAL_MOUSE:
            if (fabs(mouseDeltaX) > 2.0f || fabs(mouseDeltaY) > 2.0f) {
                ApplyMouseSteering(mouseDeltaX, mouseDeltaY);
            }
            break;
            
        case GUIDANCE_HYBRID:
            ApplyHybridGuidance(inElapsedSinceLastCall, mouseDeltaX, mouseDeltaY);
            break;
            
        default:
            break;
    }
    
    return 0.02f; // Continue at 50Hz
}