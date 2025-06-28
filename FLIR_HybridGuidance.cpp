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
#include "XPLMScenery.h"

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

// Terrain probing for ray casting
static XPLMProbeRef gTerrainProbe = NULL;

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
static float gAutoGuidanceStrength = 0.8f; // How aggressively to steer toward crosshair (higher = more magnet-like)
static float gMaxSteeringForce = 40.0f;
static float gWeaponSpeed = 120.0f;

// Missile physics parameters (tuned for precision targeting)
static float gMaxTurnRate = 120.0f; // degrees per second (very aggressive)
static float gTargetLeadTime = 0.1f; // seconds ahead to predict (very short for precision)
static float gGravityCompensation = 9.81f; // m/s² upward bias to counter gravity
static float gProportionalGain = 0.5f; // P term for precise tracking (reduced for stability)
static float gIntegralGain = 0.1f; // I term to eliminate steady-state error (reduced)
static float gDerivativeGain = 0.05f; // D term for smooth approach (reduced) (shorter for tighter following)

// Target coordinates (calculated from FLIR)
static float gTargetX = 0.0f;
static float gTargetY = 0.0f;
static float gTargetZ = 0.0f;
static bool gTargetValid = false;

// Target movement tracking for lead calculation
static float gTargetVX = 0.0f;
static float gTargetVY = 0.0f;
static float gTargetVZ = 0.0f;

// PID error tracking for precision guidance
static float gErrorIntegralX = 0.0f;
static float gErrorIntegralY = 0.0f;
static float gErrorIntegralZ = 0.0f;
static float gPrevErrorX = 0.0f;
static float gPrevErrorY = 0.0f;
static float gPrevErrorZ = 0.0f;

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
    
    // Create terrain probe for ray casting
    gTerrainProbe = XPLMCreateProbe(xplm_ProbeY);
    if (gTerrainProbe) {
        XPLMDebugString("HYBRID GUIDANCE: Terrain probe created successfully\\n");
    } else {
        XPLMDebugString("HYBRID GUIDANCE: ERROR - Failed to create terrain probe!\\n");
    }

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
    
    if (gTerrainProbe) {
        XPLMDestroyProbe(gTerrainProbe);
        gTerrainProbe = NULL;
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

// Calculate crosshair direction vector relative to missile position
static bool CalculateCrosshairDirection(float missileX, float missileY, float missileZ, float* outDirX, float* outDirY, float* outDirZ)
{
    if (!gCameraActiveDataRef || !XPLMGetDatai(gCameraActiveDataRef)) {
        return false;
    }
    
    if (!gCameraPanDataRef || !gCameraTiltDataRef) {
        return false;
    }
    
    // Get FLIR angles
    float pan = XPLMGetDataf(gCameraPanDataRef);
    float tilt = XPLMGetDataf(gCameraTiltDataRef);
    
    // Get aircraft position and orientation
    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);
    float acHeading = XPLMGetDataf(gAircraftHeading);
    float acPitch = XPLMGetDataf(gAircraftPitch);
    float acRoll = XPLMGetDataf(gAircraftRoll);
    
    // Convert to radians
    float headingRad = acHeading * M_PI / 180.0f;
    float panRad = pan * M_PI / 180.0f;
    float tiltRad = tilt * M_PI / 180.0f;
    float pitchRad = acPitch * M_PI / 180.0f;
    float rollRad = acRoll * M_PI / 180.0f;
    
    // X-Plane coordinate system: X=East, Y=Up, Z=South
    // Aircraft typically flies North = negative Z direction
    // Start with forward direction, then apply pan/tilt
    
    // Base direction (forward from aircraft) 
    float baseDirX = 0.0f;      // No sideways component initially
    float baseDirY = 0.0f;      // No vertical component initially  
    float baseDirZ = -1.0f;     // Forward = North = negative Z in X-Plane
    
    // Apply FLIR pan (rotation around Y axis)
    // Positive pan = turn right = positive X direction
    float dirX = baseDirX * cos(panRad) - baseDirZ * sin(panRad);
    float dirY = baseDirY;
    float dirZ = baseDirX * sin(panRad) + baseDirZ * cos(panRad);
    
    // Apply FLIR tilt (rotation around X axis)
    float finalDirX = dirX;
    float finalDirY = dirY * cos(-tiltRad) - dirZ * sin(-tiltRad);
    float finalDirZ = dirY * sin(-tiltRad) + dirZ * cos(-tiltRad);
    
    // Transform by aircraft heading (X-Plane: 0°=North, 90°=East, 180°=South, 270°=West)
    // Positive heading = clockwise rotation when viewed from above
    *outDirX = finalDirX * cos(headingRad) + finalDirZ * sin(headingRad);
    *outDirY = finalDirY;
    *outDirZ = -finalDirX * sin(headingRad) + finalDirZ * cos(headingRad);
    
    // Normalize direction vector
    float magnitude = sqrt((*outDirX)*(*outDirX) + (*outDirY)*(*outDirY) + (*outDirZ)*(*outDirZ));
    if (magnitude > 0.001f) {
        *outDirX /= magnitude;
        *outDirY /= magnitude;
        *outDirZ /= magnitude;
    }
    
    // Debug direction calculation
    static float dirDebugTimer = 0.0f;
    dirDebugTimer += 0.02f; // Assuming 50Hz calls
    if (dirDebugTimer >= 2.0f) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "FLIR Direction: Heading=%.1f° Pan=%.1f° Tilt=%.1f° -> Dir(%.3f,%.3f,%.3f)\\n",
            acHeading, pan, tilt, *outDirX, *outDirY, *outDirZ);
        XPLMDebugString(msg);
        dirDebugTimer = 0.0f;
    }
    
    return true;
}

// Ray casting with binary search to find terrain intersection (proven method from forum)
static bool RaycastToTerrain(float startX, float startY, float startZ, float dirX, float dirY, float dirZ, float* outX, float* outY, float* outZ)
{
    if (!gTerrainProbe) {
        XPLMDebugString("RAYCAST: ERROR - No terrain probe available\\n");
        return false;
    }
    
    // Binary search parameters (from forum post)
    float minRange = 100.0f;      // Start at 100m
    float maxRange = 30000.0f;    // Max 30km (maritime patrol range)
    float precision = 1.0f;       // 1 meter precision
    int maxIterations = 50;       // Safety limit
    
    XPLMProbeInfo_t probeInfo;
    probeInfo.structSize = sizeof(XPLMProbeInfo_t);
    
    char debugMsg[256];
    snprintf(debugMsg, sizeof(debugMsg), 
        "RAYCAST: Start(%.1f,%.1f,%.1f) Dir(%.3f,%.3f,%.3f) Range(%.0f-%.0f)\\n",
        startX, startY, startZ, dirX, dirY, dirZ, minRange, maxRange);
    XPLMDebugString(debugMsg);
    
    int iteration = 0;
    float currentRange = maxRange;
    bool foundTerrain = false;
    
    // Binary search for terrain intersection
    while ((maxRange - minRange) > precision && iteration < maxIterations) {
        currentRange = (minRange + maxRange) / 2.0f;
        
        // Calculate test point along ray
        float testX = startX + dirX * currentRange;
        float testY = startY + dirY * currentRange;
        float testZ = startZ + dirZ * currentRange;
        
        // Probe terrain at this point
        XPLMProbeResult result = XPLMProbeTerrainXYZ(gTerrainProbe, testX, testY, testZ, &probeInfo);
        
        bool isUnderTerrain = (testY < probeInfo.locationY);
        
        if (iteration < 5 || iteration % 10 == 0) { // Log first few and every 10th iteration
            snprintf(debugMsg, sizeof(debugMsg), 
                "RAYCAST: Iter=%d Range=%.1f Test(%.1f,%.1f,%.1f) Terrain=%.1f Under=%s\\n",
                iteration, currentRange, testX, testY, testZ, probeInfo.locationY, isUnderTerrain ? "YES" : "NO");
            XPLMDebugString(debugMsg);
        }
        
        if (result == xplm_ProbeHitTerrain) {
            foundTerrain = true;
            
            if (isUnderTerrain) {
                // We're under terrain, back up
                maxRange = currentRange;
            } else {
                // We're above terrain, go further
                minRange = currentRange;
            }
        } else {
            // No terrain hit, go further
            minRange = currentRange;
        }
        
        iteration++;
    }
    
    if (foundTerrain) {
        // Final intersection point
        *outX = startX + dirX * currentRange;
        *outY = startY + dirY * currentRange;
        *outZ = startZ + dirZ * currentRange;
        
        snprintf(debugMsg, sizeof(debugMsg), 
            "RAYCAST: SUCCESS after %d iterations - Target(%.1f,%.1f,%.1f) Range=%.1fm\\n",
            iteration, *outX, *outY, *outZ, currentRange);
        XPLMDebugString(debugMsg);
        
        return true;
    } else {
        snprintf(debugMsg, sizeof(debugMsg), 
            "RAYCAST: FAILED after %d iterations - No terrain intersection found\\n", iteration);
        XPLMDebugString(debugMsg);
        
        return false;
    }
}

// Apply automatic guidance toward crosshair direction with proper missile physics
static void ApplyAutoCrosshairGuidance(float deltaTime)
{
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
    
    // Guide weapons toward raycast target with proportional navigation
    for (int i = 0; i < numWeapons; i++) {
        if (weaponX[i] != 0.0f || weaponY[i] != 0.0f || weaponZ[i] != 0.0f) {
            foundWeapon = true;
            
            // Get aircraft position for ray start
            float acX = XPLMGetDataf(gAircraftX);
            float acY = XPLMGetDataf(gAircraftY);
            float acZ = XPLMGetDataf(gAircraftZ);
            
            // Get FLIR direction
            float crosshairDirX, crosshairDirY, crosshairDirZ;
            if (!CalculateCrosshairDirection(acX, acY, acZ, &crosshairDirX, &crosshairDirY, &crosshairDirZ)) {
                continue;
            }
            
            // Ray cast to find target on terrain/water
            float targetX, targetY, targetZ;
            if (!RaycastToTerrain(acX, acY, acZ, crosshairDirX, crosshairDirY, crosshairDirZ, &targetX, &targetY, &targetZ)) {
                continue; // No target found, skip this missile
            }
            
            // Calculate direction from missile to target
            float deltaX = targetX - weaponX[i];
            float deltaY = targetY - weaponY[i];
            float deltaZ = targetZ - weaponZ[i];
            float distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
            
            if (distance < 50.0f) {
                continue; // Too close, skip
            }
            
            // Desired direction (normalized)
            float desiredDirX = deltaX / distance;
            float desiredDirY = deltaY / distance;
            float desiredDirZ = deltaZ / distance;
            
            // Current missile velocity and direction
            float currentVX = weaponVX[i];
            float currentVY = weaponVY[i];
            float currentVZ = weaponVZ[i];
            float currentSpeed = sqrt(currentVX*currentVX + currentVY*currentVY + currentVZ*currentVZ);
            if (currentSpeed < 10.0f) currentSpeed = gWeaponSpeed; // Initial speed
            
            float currentDirX = currentSpeed > 1.0f ? currentVX / currentSpeed : 0.0f;
            float currentDirY = currentSpeed > 1.0f ? currentVY / currentSpeed : 0.0f;
            float currentDirZ = currentSpeed > 1.0f ? currentVZ / currentSpeed : 0.0f;
            
            // Error is difference between desired and current direction
            float errorX = desiredDirX - currentDirX;
            float errorY = desiredDirY - currentDirY;
            float errorZ = desiredDirZ - currentDirZ;
            
            // Integral term (accumulate error over time)
            gErrorIntegralX += errorX * deltaTime;
            gErrorIntegralY += errorY * deltaTime;
            gErrorIntegralZ += errorZ * deltaTime;
            
            // Derivative term (rate of error change)
            float errorDX = (errorX - gPrevErrorX) / deltaTime;
            float errorDY = (errorY - gPrevErrorY) / deltaTime;
            float errorDZ = (errorZ - gPrevErrorZ) / deltaTime;
            
            // PID control output
            float pidOutputX = gProportionalGain * errorX + gIntegralGain * gErrorIntegralX + gDerivativeGain * errorDX;
            float pidOutputY = gProportionalGain * errorY + gIntegralGain * gErrorIntegralY + gDerivativeGain * errorDY;
            float pidOutputZ = gProportionalGain * errorZ + gIntegralGain * gErrorIntegralZ + gDerivativeGain * errorDZ;
            
            // Add gravity compensation to Y component
            pidOutputY += gGravityCompensation / gWeaponSpeed; // Normalize by speed
            
            // Calculate new desired velocity
            float newDesiredVX = currentVX + pidOutputX * currentSpeed;
            float newDesiredVY = currentVY + pidOutputY * currentSpeed;
            float newDesiredVZ = currentVZ + pidOutputZ * currentSpeed;
            
            // Calculate velocity change (limited by turn rate)
            float deltaVX = newDesiredVX - weaponVX[i];
            float deltaVY = newDesiredVY - weaponVY[i];
            float deltaVZ = newDesiredVZ - weaponVZ[i];
            
            // Limit turn rate (realistic missile physics)
            float maxDeltaV = gMaxTurnRate * M_PI / 180.0f * currentSpeed * deltaTime;
            float deltaVMag = sqrt(deltaVX*deltaVX + deltaVY*deltaVY + deltaVZ*deltaVZ);
            
            if (deltaVMag > maxDeltaV) {
                float scale = maxDeltaV / deltaVMag;
                deltaVX *= scale;
                deltaVY *= scale;
                deltaVZ *= scale;
            }
            
            // Apply steering
            weaponVX[i] += deltaVX;
            weaponVY[i] += deltaVY;
            weaponVZ[i] += deltaVZ;
            
            // Maintain approximately constant speed
            float newSpeed = sqrt(weaponVX[i]*weaponVX[i] + weaponVY[i]*weaponVY[i] + weaponVZ[i]*weaponVZ[i]);
            if (newSpeed > 1.0f) {
                float speedScale = gWeaponSpeed / newSpeed;
                weaponVX[i] *= speedScale;
                weaponVY[i] *= speedScale;
                weaponVZ[i] *= speedScale;
            }
            
            // Store previous error for next derivative calculation
            gPrevErrorX = errorX;
            gPrevErrorY = errorY;
            gPrevErrorZ = errorZ;
        }
    }
    
    if (foundWeapon) {
        XPLMSetDatavf(gWeaponVX, weaponVX, 0, numWeapons);
        XPLMSetDatavf(gWeaponVY, weaponVY, 0, numWeapons);
        XPLMSetDatavf(gWeaponVZ, weaponVZ, 0, numWeapons);
        
        // Debug weapon guidance
        static float weaponDebugTimer = 0.0f;
        weaponDebugTimer += deltaTime;
        if (weaponDebugTimer >= 2.0f) {
            // Test raycast from aircraft position
            float acX = XPLMGetDataf(gAircraftX);
            float acY = XPLMGetDataf(gAircraftY);
            float acZ = XPLMGetDataf(gAircraftZ);
            
            float crosshairDirX, crosshairDirY, crosshairDirZ;
            if (CalculateCrosshairDirection(acX, acY, acZ, &crosshairDirX, &crosshairDirY, &crosshairDirZ)) {
                float targetX, targetY, targetZ;
                if (RaycastToTerrain(acX, acY, acZ, crosshairDirX, crosshairDirY, crosshairDirZ, &targetX, &targetY, &targetZ)) {
                    float range = sqrt((targetX-acX)*(targetX-acX) + (targetY-acY)*(targetY-acY) + (targetZ-acZ)*(targetZ-acZ));
                    char msg[256];
                    snprintf(msg, sizeof(msg), 
                        "GUIDANCE: Aircraft(%.0f,%.0f,%.0f) -> Target(%.0f,%.0f,%.0f) Range=%.0fm\\n",
                        acX, acY, acZ, targetX, targetY, targetZ, range);
                    XPLMDebugString(msg);
                }
            }
            weaponDebugTimer = 0.0f;
        }
    } else {
        // Debug when no weapons found
        static float noWeaponTimer = 0.0f;
        noWeaponTimer += deltaTime;
        if (noWeaponTimer >= 2.0f) {
            XPLMDebugString("HYBRID GUIDANCE: No active weapons found!\\n");
            noWeaponTimer = 0.0f;
        }
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