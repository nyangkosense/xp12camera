/*
 * FLIR_Camera.cpp
 * 
 * A realistic FLIR camera system plugin for X-Plane 12
 * Features:
 * - Real camera positioning under aircraft (belly-mounted)
 * - True optical zoom with zoom parameter
 * - Pan/tilt camera controls
 * - Target acquisition and tracking
 * - Thermal overlay rendering
 * 
 */

 #include <string.h>
 #include <stdio.h>
 #include <math.h>
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 #include "XPLMDisplay.h"
 #include "XPLMUtilities.h"
 #include "XPLMCamera.h"
 #include "XPLMDataAccess.h"
 #include "XPLMGraphics.h"
 #include "XPLMProcessing.h"
 #include "XPLMMenus.h"
 #include "XPLMScenery.h"
 #include "XPLMPlanes.h"
#include "XPLMNavigation.h"
#include "XPLMInstance.h"
#include "XPLMWeather.h"

 // OpenGL includes for MinGW
 #include <windows.h>
 #include <GL/gl.h>
 
 // Plugin globals
 static XPLMHotKeyID gActivateKey = NULL;
 static XPLMHotKeyID gZoomInKey = NULL;
 static XPLMHotKeyID gZoomOutKey = NULL;
 static XPLMHotKeyID gPanLeftKey = NULL;
 static XPLMHotKeyID gPanRightKey = NULL;
 static XPLMHotKeyID gTiltUpKey = NULL;
 static XPLMHotKeyID gTiltDownKey = NULL;
static XPLMHotKeyID gThermalToggleKey = NULL;
 
 // Aircraft position datarefs
 static XPLMDataRef gPlaneX = NULL;
 static XPLMDataRef gPlaneY = NULL;
 static XPLMDataRef gPlaneZ = NULL;
 static XPLMDataRef gPlaneHeading = NULL;
 static XPLMDataRef gPlanePitch = NULL;
 static XPLMDataRef gPlaneRoll = NULL;
 
 // FLIR camera state
 static int gCameraActive = 0;
 static float gZoomLevel = 1.0f;
 static float gCameraPan = 0.0f;      // Left/right rotation (degrees)
 static float gCameraTilt = -15.0f;   // Up/down rotation (degrees)
 static float gCameraHeight = -5.0f;  // Height below aircraft (meters)
 static float gCameraDistance = 3.0f; // Forward/back from aircraft center

// Thermal view settings
static int gThermalMode = 1;         // 0=Off, 1=White Hot, 2=Enhanced
 
 // Target tracking and object detection
 // static int gTargetLocked = 0; // unused
 // static float gTargetX = 0.0f; // unused
 // static float gTargetY = 0.0f; // unused
 // static float gTargetZ = 0.0f; // unused
 // static int gDetectedObjects = 0; // unused

 // Aircraft tracking arrays (up to 20 aircraft)
 static float gAircraftPositions[20][3];  // x, y, z positions
 static float gAircraftEngineTemp[20];    // Engine temperatures
 static float gAircraftDistance[20];      // Distance from camera
static int gAircraftVisible[20];         // Visibility in thermal
static int gAircraftCount = 0;

// Environmental factors
static float gCurrentVisibility = 10000.0f;
static float gCurrentTemp = 15.0f;
static float gCurrentWindSpeed = 0.0f;
static int gIsNight = 0;

// Heat source simulation
typedef struct {
    float x, y, z;           // World position
    float intensity;         // Heat intensity (0-1)
    float size;             // Heat source size
    int type;               // 0=aircraft, 1=ship, 2=vehicle, 3=building
    float lastUpdate;       // Last update time
} HeatSource;

static HeatSource gHeatSources[50];
static int gHeatSourceCount = 0;

 // Additional datarefs for enhanced detection
 static XPLMDataRef gEngineRunning = NULL;
 static XPLMDataRef gEngineN1 = NULL;
static XPLMDataRef gEngineEGT = NULL;
static XPLMDataRef gGroundTemperature = NULL;
static XPLMDataRef gWeatherVisibility = NULL;
static XPLMDataRef gCloudCoverage = NULL;
static XPLMDataRef gAmbientTemperature = NULL;
static XPLMDataRef gWindSpeed = NULL;
static XPLMDataRef gTimeOfDay = NULL;
static XPLMDataRef gAIAircraftX = NULL;
static XPLMDataRef gAIAircraftY = NULL;
static XPLMDataRef gAIAircraftZ = NULL;
static XPLMDataRef gAIAircraftCount = NULL;
 
 // Function declarations
 static void ActivateFLIRCallback(void* inRefcon);
 static void ZoomInCallback(void* inRefcon);
 static void ZoomOutCallback(void* inRefcon);
 static void PanLeftCallback(void* inRefcon);
 static void PanRightCallback(void* inRefcon);
 static void TiltUpCallback(void* inRefcon);
 static void TiltDownCallback(void* inRefcon);
static void ThermalToggleCallback(void* inRefcon);
 
 static int FLIRCameraFunc(XPLMCameraPosition_t* outCameraPosition, 
                           int inIsLosingControl, 
                           void* inRefcon);
 
 static int DrawThermalOverlay(XPLMDrawingPhase inPhase,
                              int inIsBefore,
                              void* inRefcon);

static void UpdateEnvironmentalFactors(void);
static void UpdateHeatSources(void);
static void DetectAircraft(void);
static float CalculateHeatIntensity(float engineTemp, float distance, int engineRunning);
static void DrawRealisticThermalOverlay(void);
static float GetDistanceToCamera(float x, float y, float z);
 
 // Plugin lifecycle functions
 PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
 {
     strcpy(outName, "FLIR Camera System");
     strcpy(outSig, "flir.camera.system");
     strcpy(outDesc, "Realistic FLIR camera with zoom and thermal overlay");
 
     // Get aircraft position datarefs
     gPlaneX = XPLMFindDataRef("sim/flightmodel/position/local_x");
     gPlaneY = XPLMFindDataRef("sim/flightmodel/position/local_y");
     gPlaneZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
     gPlaneHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
     gPlanePitch = XPLMFindDataRef("sim/flightmodel/position/theta");
     gPlaneRoll = XPLMFindDataRef("sim/flightmodel/position/phi");
 
     // Additional datarefs for thermal detection
     gEngineRunning = XPLMFindDataRef("sim/flightmodel/engine/ENGN_running");
     gEngineN1 = XPLMFindDataRef("sim/flightmodel/engine/ENGN_N1_");
    gEngineEGT = XPLMFindDataRef("sim/flightmodel/engine/ENGN_EGT_c");
    gGroundTemperature = XPLMFindDataRef("sim/weather/temperature_sealevel_c");
    gWeatherVisibility = XPLMFindDataRef("sim/weather/visibility_reported_m");
    gCloudCoverage = XPLMFindDataRef("sim/weather/cloud_coverage[0]");
    gAmbientTemperature = XPLMFindDataRef("sim/weather/temperature_ambient_c");
    gWindSpeed = XPLMFindDataRef("sim/weather/wind_speed_kt[0]");
    gTimeOfDay = XPLMFindDataRef("sim/time/local_time_sec");
    gAIAircraftX = XPLMFindDataRef("sim/multiplayer/position/plane1_x");
    gAIAircraftY = XPLMFindDataRef("sim/multiplayer/position/plane1_y");
    gAIAircraftZ = XPLMFindDataRef("sim/multiplayer/position/plane1_z");
    gAIAircraftCount = XPLMFindDataRef("sim/operation/prefs/mult_max");
 
     // Register hotkeys
     gActivateKey = XPLMRegisterHotKey(XPLM_VK_F9, xplm_DownFlag,
                                       "Activate FLIR Camera",
                                       ActivateFLIRCallback, NULL);
     
     gZoomInKey = XPLMRegisterHotKey(XPLM_VK_EQUAL, xplm_DownFlag,
                                     "FLIR Zoom In",
                                     ZoomInCallback, NULL);
     
     gZoomOutKey = XPLMRegisterHotKey(XPLM_VK_MINUS, xplm_DownFlag,
                                      "FLIR Zoom Out",
                                      ZoomOutCallback, NULL);
     
     gPanLeftKey = XPLMRegisterHotKey(XPLM_VK_LEFT, xplm_DownFlag,
                                      "FLIR Pan Left",
                                      PanLeftCallback, NULL);
     
     gPanRightKey = XPLMRegisterHotKey(XPLM_VK_RIGHT, xplm_DownFlag,
                                       "FLIR Pan Right",
                                       PanRightCallback, NULL);
     
     gTiltUpKey = XPLMRegisterHotKey(XPLM_VK_UP, xplm_DownFlag,
                                     "FLIR Tilt Up",
                                     TiltUpCallback, NULL);
     
     gTiltDownKey = XPLMRegisterHotKey(XPLM_VK_DOWN, xplm_DownFlag,
                                       "FLIR Tilt Down",
                                       TiltDownCallback, NULL);
 
     // Register thermal overlay drawing callback - use objects phase for better integration
     XPLMRegisterDrawCallback(DrawThermalOverlay, xplm_Phase_Modern3D, 1, NULL);
 
     XPLMDebugString("FLIR Camera System: Plugin loaded successfully\n");
     XPLMDebugString("FLIR Camera System: Press F9 to activate camera\n");
     XPLMDebugString("FLIR Camera System: Use +/- for zoom, arrows for pan/tilt, T for thermal\n");
     
     gThermalToggleKey = XPLMRegisterHotKey(XPLM_VK_T, xplm_DownFlag,
                                           "FLIR Thermal Toggle",
                                           ThermalToggleCallback, NULL);
 
     return 1;
 }
 
 PLUGIN_API void XPluginStop(void)
 {
     // Unregister hotkeys
     if (gActivateKey) XPLMUnregisterHotKey(gActivateKey);
     if (gZoomInKey) XPLMUnregisterHotKey(gZoomInKey);
     if (gZoomOutKey) XPLMUnregisterHotKey(gZoomOutKey);
     if (gPanLeftKey) XPLMUnregisterHotKey(gPanLeftKey);
     if (gPanRightKey) XPLMUnregisterHotKey(gPanRightKey);
     if (gTiltUpKey) XPLMUnregisterHotKey(gTiltUpKey);
     if (gTiltDownKey) XPLMUnregisterHotKey(gTiltDownKey);
 
     // Stop camera control if active
     if (gCameraActive) {
         XPLMDontControlCamera();
         gCameraActive = 0;
     }
 
     XPLMDebugString("FLIR Camera System: Plugin stopped\n");
 }
 
 PLUGIN_API void XPluginDisable(void) { }
 PLUGIN_API int XPluginEnable(void) { return 1; }
 PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }
 
 // Hotkey callbacks
 static void ActivateFLIRCallback(void* inRefcon)
 {
     if (!gCameraActive) {
         // Activate FLIR camera
         XPLMDebugString("FLIR Camera System: Activating camera\n");
         
         // Switch to external view first
         // XPLMCommandButtonPress(xplm_joy_v_fr1);
         // XPLMCommandButtonRelease(xplm_joy_v_fr1);
         
         // Take camera control
         XPLMControlCamera(xplm_ControlCameraUntilViewChanges, FLIRCameraFunc, NULL);
         gCameraActive = 1;
         
         XPLMDebugString("FLIR Camera System: Camera active - mounted under aircraft\n");
     } else {
         // Deactivate camera
         XPLMDontControlCamera();
         gCameraActive = 0;
         XPLMDebugString("FLIR Camera System: Camera deactivated\n");
     }
 }
 
 static void ZoomInCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gZoomLevel = fminf(gZoomLevel * 1.5f, 16.0f);
         char msg[256];
         sprintf(msg, "FLIR Camera System: Zoom %.1fx\n", gZoomLevel);
         XPLMDebugString(msg);
     }
 }
 
 static void ZoomOutCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gZoomLevel = fmaxf(gZoomLevel / 1.5f, 1.0f);
         char msg[256];
         sprintf(msg, "FLIR Camera System: Zoom %.1fx\n", gZoomLevel);
         XPLMDebugString(msg);
     }
 }
 
 static void PanLeftCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gCameraPan -= 10.0f;
         if (gCameraPan < -180.0f) gCameraPan += 360.0f;
         char msg[256];
         sprintf(msg, "FLIR Camera System: Pan %.0f degrees\n", gCameraPan);
         XPLMDebugString(msg);
     }
 }
 
 static void PanRightCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gCameraPan += 10.0f;
         if (gCameraPan > 180.0f) gCameraPan -= 360.0f;
         char msg[256];
         sprintf(msg, "FLIR Camera System: Pan %.0f degrees\n", gCameraPan);
         XPLMDebugString(msg);
     }
 }
 
 static void TiltUpCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gCameraTilt = fminf(gCameraTilt + 10.0f, 45.0f);
         char msg[256];
         sprintf(msg, "FLIR Camera System: Tilt %.0f degrees\n", gCameraTilt);
         XPLMDebugString(msg);
     }
 }
 
 static void TiltDownCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gCameraTilt = fmaxf(gCameraTilt - 10.0f, -90.0f);
         char msg[256];
         sprintf(msg, "FLIR Camera System: Tilt %.0f degrees\n", gCameraTilt);
         XPLMDebugString(msg);
     }
 }
 
 static void ThermalToggleCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gThermalMode = (gThermalMode + 1) % 3;
         char msg[256];
         const char* modeNames[] = {"Off", "White Hot", "Enhanced"};
         sprintf(msg, "FLIR Camera System: Thermal mode %s\n", modeNames[gThermalMode]);
         XPLMDebugString(msg);
     }
 }
 
 /*
  * FLIRCameraFunc
  * 
  * The main camera control function - positions camera under aircraft
  * like a real belly-mounted FLIR system
  */
 static int FLIRCameraFunc(XPLMCameraPosition_t* outCameraPosition,
                           int inIsLosingControl,
                           void* inRefcon)
 {
     if (outCameraPosition && !inIsLosingControl) {
         // Get current aircraft position and orientation
         float planeX = XPLMGetDataf(gPlaneX);
         float planeY = XPLMGetDataf(gPlaneY);
         float planeZ = XPLMGetDataf(gPlaneZ);
         float planeHeading = XPLMGetDataf(gPlaneHeading);
         // float planePitch = XPLMGetDataf(gPlanePitch); // unused for now
         float planeRoll = XPLMGetDataf(gPlaneRoll);
 
         // Convert to radians
         float headingRad = planeHeading * M_PI / 180.0f;
         // float pitchRad = planePitch * M_PI / 180.0f; // unused
         float rollRad = planeRoll * M_PI / 180.0f;
         // float panRad = gCameraPan * M_PI / 180.0f; // unused
         // float tiltRad = gCameraTilt * M_PI / 180.0f; // unused
 
         // Calculate camera position relative to aircraft
         // Position camera under the aircraft (belly-mounted)
         float localX = gCameraDistance * cosf(headingRad);
         float localZ = gCameraDistance * sinf(headingRad);
         float localY = gCameraHeight; // Below aircraft
 
         // Apply aircraft roll and pitch to camera position
         float rotatedX = localX * cosf(rollRad) - localY * sinf(rollRad);
         float rotatedY = localX * sinf(rollRad) + localY * cosf(rollRad);
         float rotatedZ = localZ;
 
         // Final camera position in world coordinates
         outCameraPosition->x = planeX + rotatedX;
         outCameraPosition->y = planeY + rotatedY;
         outCameraPosition->z = planeZ + rotatedZ;
 
         // Camera orientation - combine aircraft heading with camera pan/tilt
         outCameraPosition->heading = planeHeading + gCameraPan;
         outCameraPosition->pitch = gCameraTilt;
         outCameraPosition->roll = 0.0f; // Keep camera level
 
         // Apply zoom - this is the key for real magnification!
         outCameraPosition->zoom = gZoomLevel;
 
         // Normalize heading
         if (outCameraPosition->heading > 360.0f) outCameraPosition->heading -= 360.0f;
         if (outCameraPosition->heading < 0.0f) outCameraPosition->heading += 360.0f;
     }
 
     if (inIsLosingControl) {
         gCameraActive = 0;
         XPLMDebugString("FLIR Camera System: Lost camera control\n");
     }
 
     return gCameraActive;
 }
 
 /*
  * DrawThermalOverlay
  * 
  * Draws thermal/IR overlay effects on the camera view
  */
 static int DrawThermalOverlay(XPLMDrawingPhase inPhase, int inIsBefore, void* inRefcon)
 {
     if (!gCameraActive) return 1;
    
    // Update environmental factors and heat sources
    UpdateEnvironmentalFactors();
    UpdateHeatSources();
    DetectAircraft();
 
     // Set up OpenGL for 2D drawing
     XPLMSetGraphicsState(0, 0, 0, 1, 1, 0, 0);
 
     // Get screen dimensions
     int screenWidth, screenHeight;
     XPLMGetScreenSize(&screenWidth, &screenHeight);
 
     float centerX = screenWidth / 2.0f;
     float centerY = screenHeight / 2.0f;
 
     // Draw targeting reticle
     glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
     glLineWidth(2.0f);
 
     glBegin(GL_LINES);
     // Crosshair
     glVertex2f(centerX - 25, centerY);
     glVertex2f(centerX + 25, centerY);
     glVertex2f(centerX, centerY - 25);
     glVertex2f(centerX, centerY + 25);
     
     // Targeting brackets
     float bracketSize = 60.0f / gZoomLevel;
     
     // Top-left bracket
     glVertex2f(centerX - bracketSize, centerY + bracketSize);
     glVertex2f(centerX - bracketSize + 20, centerY + bracketSize);
     glVertex2f(centerX - bracketSize, centerY + bracketSize);
     glVertex2f(centerX - bracketSize, centerY + bracketSize - 20);
     
     // Top-right bracket
     glVertex2f(centerX + bracketSize, centerY + bracketSize);
     glVertex2f(centerX + bracketSize - 20, centerY + bracketSize);
     glVertex2f(centerX + bracketSize, centerY + bracketSize);
     glVertex2f(centerX + bracketSize, centerY + bracketSize - 20);
     
     // Bottom-left bracket
     glVertex2f(centerX - bracketSize, centerY - bracketSize);
     glVertex2f(centerX - bracketSize + 20, centerY - bracketSize);
     glVertex2f(centerX - bracketSize, centerY - bracketSize);
     glVertex2f(centerX - bracketSize, centerY - bracketSize + 20);
     
     // Bottom-right bracket
     glVertex2f(centerX + bracketSize, centerY - bracketSize);
     glVertex2f(centerX + bracketSize - 20, centerY - bracketSize);
     glVertex2f(centerX + bracketSize, centerY - bracketSize);
     glVertex2f(centerX + bracketSize, centerY - bracketSize + 20);
     glEnd();
 
     // Draw thermal effects based on thermal mode
     if (gThermalMode > 0) {
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         if (gThermalMode == 1) {
             // White Hot mode - dark background with bright heat sources
             glColor4f(0.0f, 0.0f, 0.1f, 0.7f);
             glBegin(GL_QUADS);
             glVertex2f(0, 0);
             glVertex2f(screenWidth, 0);
             glVertex2f(screenWidth, screenHeight);
             glVertex2f(0, screenHeight);
             glEnd();
         } else if (gThermalMode == 2) {
             // Enhanced mode - blue tint with enhanced contrast
             glColor4f(0.1f, 0.3f, 0.6f, 0.5f);
             glBegin(GL_QUADS);
             glVertex2f(0, 0);
             glVertex2f(screenWidth, 0);
             glVertex2f(screenWidth, screenHeight);
             glVertex2f(0, screenHeight);
             glEnd();
         }
         
         // Draw realistic heat sources based on actual detected objects
         DrawRealisticThermalOverlay();
         
         // Also draw some simulated background thermal noise
         float time = XPLMGetElapsedTime();
         for (int i = 0; i < 6; i++) {
             for (int j = 0; j < 4; j++) {
                 float x = (i + 1) * screenWidth / 7.0f;
                 float y = (j + 1) * screenHeight / 5.0f;
                 
                 float intensity = (sinf(time * 0.2f + i * 0.5f + j * 0.3f) + 1.0f) * 0.4f;
                 float size = 12.0f + intensity * 20.0f;
                 
                 if (gThermalMode == 1) {
                     // White hot
                     glColor4f(1.0f, 1.0f, 0.9f, intensity * 0.9f);
                 } else {
                     // Enhanced mode
                     glColor4f(1.0f, 0.8f, 0.2f, intensity * 0.8f);
                 }
                 
                 glBegin(GL_QUADS);
                 glVertex2f(x - size/2, y - size/2);
                 glVertex2f(x + size/2, y - size/2);
                 glVertex2f(x + size/2, y + size/2);
                 glVertex2f(x - size/2, y + size/2);
                 glEnd();
             }
         }
         
         glDisable(GL_BLEND);
     }
 
     // Draw HUD info
     glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
     
     // Use X-Plane's text drawing (simplified for this example)
     char zoomText[64];
     sprintf(zoomText, "ZOOM: %.1fx", gZoomLevel);
     
     char panText[64];
     sprintf(panText, "PAN: %.0f° TILT: %.0f°", gCameraPan, gCameraTilt);
     
     // Position text in corners (actual text rendering would need XPLMDrawString)
     // This is just showing where the text would go
     
     return 1;
 }
 
 /*
  * Additional utility functions for future enhancements
  */
 
 // Function to detect and track actual X-Plane objects
 /*
static void UpdateTargetTracking(void)
 {
     // Future enhancement: Use X-Plane's object detection APIs
     // to find and track actual aircraft, ships, vehicles in the sim
 }
 
 // Function to simulate different thermal/IR modes
 static void SetThermalMode(int mode)
 {
     // Future enhancement: Different thermal rendering modes
     // 0 = White Hot, 1 = Black Hot, 2 = Rainbow, etc.
 }
*/
/*
 * Enhanced Functions for FLIR Camera Plugin
 * These functions provide realistic thermal detection and environmental factors
 */

/*
 * UpdateEnvironmentalFactors
 * 
 * Updates environmental conditions that affect thermal visibility
 */
static void UpdateEnvironmentalFactors(void)
{
    if (gWeatherVisibility != NULL) {
        gCurrentVisibility = XPLMGetDataf(gWeatherVisibility);
    }
    
    if (gAmbientTemperature != NULL) {
        gCurrentTemp = XPLMGetDataf(gAmbientTemperature);
    }
    
    if (gWindSpeed != NULL) {
        gCurrentWindSpeed = XPLMGetDataf(gWindSpeed);
    }
    
    if (gTimeOfDay != NULL) {
        float timeOfDay = XPLMGetDataf(gTimeOfDay);
        // Night is roughly 18:00 to 06:00 (18*3600 = 64800, 6*3600 = 21600)
        gIsNight = (timeOfDay > 64800.0f || timeOfDay < 21600.0f) ? 1 : 0;
    }
}

/*
 * GetDistanceToCamera
 * 
 * Calculate distance from world position to camera
 */
static float GetDistanceToCamera(float x, float y, float z)
{
    float camX = XPLMGetDataf(gPlaneX);
    float camY = XPLMGetDataf(gPlaneY);
    float camZ = XPLMGetDataf(gPlaneZ);
    
    float dx = x - camX;
    float dy = y - camY;
    float dz = z - camZ;
    
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

/*
 * CalculateHeatIntensity
 * 
 * Calculate thermal intensity based on engine temperature and distance
 */
static float CalculateHeatIntensity(float engineTemp, float distance, int engineRunning)
{
    if (!engineRunning) return 0.1f; // Cold engine still slightly visible
    
    // Base intensity from engine temperature (EGT typically 400-900°C)
    float baseIntensity = (engineTemp - 200.0f) / 700.0f;
    baseIntensity = fmaxf(0.0f, fminf(1.0f, baseIntensity));
    
    // Distance attenuation (max detection ~10km for aircraft)
    float distanceFactor = 1.0f - (distance / 10000.0f);
    distanceFactor = fmaxf(0.0f, distanceFactor);
    
    // Weather effects
    float weatherFactor = gCurrentVisibility / 10000.0f;
    weatherFactor = fmaxf(0.2f, fminf(1.0f, weatherFactor));
    
    // Night enhances thermal contrast
    float nightBonus = gIsNight ? 1.3f : 1.0f;
    
    return baseIntensity * distanceFactor * weatherFactor * nightBonus;
}

/*
 * DetectAircraft
 * 
 * Scan for AI aircraft and other heat sources in the simulation
 */
static void DetectAircraft(void)
{
    gAircraftCount = 0;
    
    // Scan through multiplayer aircraft positions (up to 19 other aircraft)
    for (int i = 0; i < 19 && gAircraftCount < 20; i++) {
        char datarefName[256];
        
        // Get aircraft position
        sprintf(datarefName, "sim/multiplayer/position/plane%d_x", i + 1);
        XPLMDataRef aircraftX = XPLMFindDataRef(datarefName);
        
        sprintf(datarefName, "sim/multiplayer/position/plane%d_y", i + 1);
        XPLMDataRef aircraftY = XPLMFindDataRef(datarefName);
        
        sprintf(datarefName, "sim/multiplayer/position/plane%d_z", i + 1);
        XPLMDataRef aircraftZ = XPLMFindDataRef(datarefName);
        
        if (aircraftX && aircraftY && aircraftZ) {
            float x = XPLMGetDataf(aircraftX);
            float y = XPLMGetDataf(aircraftY);
            float z = XPLMGetDataf(aircraftZ);
            
            // Check if aircraft exists (non-zero position)
            if (x != 0.0f || y != 0.0f || z != 0.0f) {
                gAircraftPositions[gAircraftCount][0] = x;
                gAircraftPositions[gAircraftCount][1] = y;
                gAircraftPositions[gAircraftCount][2] = z;
                gAircraftDistance[gAircraftCount] = GetDistanceToCamera(x, y, z);
                
                // Simulate engine temperature (realistic EGT values)
                gAircraftEngineTemp[gAircraftCount] = 450.0f + (rand() % 300);
                
                // Determine visibility based on distance and conditions
                gAircraftVisible[gAircraftCount] = (gAircraftDistance[gAircraftCount] < gCurrentVisibility * 0.8f) ? 1 : 0;
                
                gAircraftCount++;
            }
        }
    }
}

/*
 * UpdateHeatSources
 * 
 * Update dynamic heat sources in the simulation
 */
static void UpdateHeatSources(void)
{
    float currentTime = XPLMGetElapsedTime();
    gHeatSourceCount = 0;
    
    // Add detected aircraft as heat sources
    for (int i = 0; i < gAircraftCount && gHeatSourceCount < 50; i++) {
        if (gAircraftVisible[i]) {
            gHeatSources[gHeatSourceCount].x = gAircraftPositions[i][0];
            gHeatSources[gHeatSourceCount].y = gAircraftPositions[i][1];
            gHeatSources[gHeatSourceCount].z = gAircraftPositions[i][2];
            gHeatSources[gHeatSourceCount].intensity = CalculateHeatIntensity(
                gAircraftEngineTemp[i], gAircraftDistance[i], 1);
            gHeatSources[gHeatSourceCount].size = 15.0f + (gHeatSources[gHeatSourceCount].intensity * 25.0f);
            gHeatSources[gHeatSourceCount].type = 0; // Aircraft
            gHeatSources[gHeatSourceCount].lastUpdate = currentTime;
            gHeatSourceCount++;
        }
    }
    
    // Add simulated ground heat sources (buildings, vehicles, ships)
    float planeX = XPLMGetDataf(gPlaneX);
    float planeZ = XPLMGetDataf(gPlaneZ);
    
    // Simulate some heat sources around the area
    for (int i = 0; i < 8 && gHeatSourceCount < 50; i++) {
        float angle = (i * 45.0f) * M_PI / 180.0f;
        float distance = 2000.0f + (i * 500.0f);
        
        float x = planeX + distance * cosf(angle);
        float z = planeZ + distance * sinf(angle);
        float y = XPLMGetDataf(gPlaneY) - 100.0f; // Ground level
        
        gHeatSources[gHeatSourceCount].x = x;
        gHeatSources[gHeatSourceCount].y = y;
        gHeatSources[gHeatSourceCount].z = z;
        
        // Vary intensity based on type and time
        float baseIntensity = 0.3f;
        if (gIsNight) baseIntensity += 0.2f; // More visible at night
        
        gHeatSources[gHeatSourceCount].intensity = baseIntensity + 
            0.2f * sinf(currentTime * 0.1f + i);
        gHeatSources[gHeatSourceCount].size = 8.0f + (gHeatSources[gHeatSourceCount].intensity * 12.0f);
        gHeatSources[gHeatSourceCount].type = (i % 3) + 1; // Ships, vehicles, buildings
        gHeatSources[gHeatSourceCount].lastUpdate = currentTime;
        gHeatSourceCount++;
    }
}

/*
 * DrawRealisticThermalOverlay
 * 
 * Render realistic thermal signatures for detected heat sources
 */
static void DrawRealisticThermalOverlay(void)
{
    if (gHeatSourceCount == 0) return;
    
    // Get screen dimensions and camera info
    int screenWidth, screenHeight;
    XPLMGetScreenSize(&screenWidth, &screenHeight);
    
    float camX = XPLMGetDataf(gPlaneX);
    float camY = XPLMGetDataf(gPlaneY);
    float camZ = XPLMGetDataf(gPlaneZ);
    float camHeading = XPLMGetDataf(gPlaneHeading) + gCameraPan;
    float camPitch = gCameraTilt;
    
    // Convert camera angles to radians
    float headingRad = camHeading * M_PI / 180.0f;
    float pitchRad = camPitch * M_PI / 180.0f;
    
    // Camera forward vector
    float forwardX = cosf(headingRad) * cosf(pitchRad);
    float forwardY = sinf(pitchRad);
    float forwardZ = sinf(headingRad) * cosf(pitchRad);
    
    // Camera right vector (for screen mapping)
    float rightX = -sinf(headingRad);
    float rightZ = cosf(headingRad);
    
    // Camera up vector
    float upX = -cosf(headingRad) * sinf(pitchRad);
    float upY = cosf(pitchRad);
    float upZ = -sinf(headingRad) * sinf(pitchRad);
    
    // Field of view factor (approximate)
    float fovFactor = 60.0f / gZoomLevel; // Base FOV 60 degrees
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw each heat source
    for (int i = 0; i < gHeatSourceCount; i++) {
        HeatSource* heat = &gHeatSources[i];
        
        // Vector from camera to heat source
        float dx = heat->x - camX;
        float dy = heat->y - camY;
        float dz = heat->z - camZ;
        float distance = sqrtf(dx*dx + dy*dy + dz*dz);
        
        if (distance < 50.0f) continue; // Too close, skip
        
        // Normalize direction vector
        dx /= distance;
        dy /= distance;
        dz /= distance;
        
        // Check if heat source is in front of camera
        float dotForward = dx * forwardX + dy * forwardY + dz * forwardZ;
        if (dotForward <= 0.1f) continue; // Behind camera or too far to side
        
        // Project to screen coordinates
        float rightDot = dx * rightX + dz * rightZ;
        float upDot = dx * upX + dy * upY + dz * upZ;
        
        // Convert to screen space
        float screenX = screenWidth * 0.5f + (rightDot * screenWidth * 0.5f / fovFactor);
        float screenY = screenHeight * 0.5f + (upDot * screenHeight * 0.5f / fovFactor);
        
        // Check if on screen
        if (screenX < 0 || screenX >= screenWidth || screenY < 0 || screenY >= screenHeight) {
            continue;
        }
        
        // Calculate apparent size based on distance and zoom
        float apparentSize = heat->size * gZoomLevel / (distance * 0.01f);
        apparentSize = fmaxf(3.0f, fminf(apparentSize, 50.0f));
        
        // Color based on heat source type and intensity
        float r, g, b, a;
        if (gThermalMode == 1) {
            // White hot mode
            r = g = b = heat->intensity;
            a = heat->intensity * 0.8f;
            
            // Aircraft show as very bright white
            if (heat->type == 0) {
                r = g = b = 1.0f;
                a = 0.9f;
            }
        } else {
            // Enhanced mode - use color coding
            switch (heat->type) {
                case 0: // Aircraft - bright yellow/orange
                    r = 1.0f;
                    g = 0.8f;
                    b = 0.2f;
                    a = heat->intensity * 0.9f;
                    break;
                case 1: // Ships - cyan
                    r = 0.2f;
                    g = 0.8f;
                    b = 1.0f;
                    a = heat->intensity * 0.7f;
                    break;
                case 2: // Vehicles - red
                    r = 1.0f;
                    g = 0.3f;
                    b = 0.1f;
                    a = heat->intensity * 0.6f;
                    break;
                case 3: // Buildings - orange
                    r = 1.0f;
                    g = 0.5f;
                    b = 0.0f;
                    a = heat->intensity * 0.5f;
                    break;
                default:
                    r = g = b = heat->intensity;
                    a = heat->intensity * 0.6f;
                    break;
            }
        }
        
        glColor4f(r, g, b, a);
        
        // Draw heat signature with gradient effect
        glBegin(GL_QUADS);
        glVertex2f(screenX - apparentSize, screenY - apparentSize);
        glVertex2f(screenX + apparentSize, screenY - apparentSize);
        glVertex2f(screenX + apparentSize, screenY + apparentSize);
        glVertex2f(screenX - apparentSize, screenY + apparentSize);
        glEnd();
        
        // Add heat bloom effect for intense sources
        if (heat->intensity > 0.7f) {
            float bloomSize = apparentSize * 1.5f;
            glColor4f(r, g, b, a * 0.3f);
            glBegin(GL_QUADS);
            glVertex2f(screenX - bloomSize, screenY - bloomSize);
            glVertex2f(screenX + bloomSize, screenY - bloomSize);
            glVertex2f(screenX + bloomSize, screenY + bloomSize);
            glVertex2f(screenX - bloomSize, screenY + bloomSize);
            glEnd();
        }
        
        // Add targeting indicator for aircraft
        if (heat->type == 0 && distance < 5000.0f) {
            glColor4f(0.0f, 1.0f, 0.0f, 0.8f);
            glLineWidth(2.0f);
            
            float targetSize = apparentSize + 10.0f;
            glBegin(GL_LINE_LOOP);
            glVertex2f(screenX - targetSize, screenY - targetSize);
            glVertex2f(screenX + targetSize, screenY - targetSize);
            glVertex2f(screenX + targetSize, screenY + targetSize);
            glVertex2f(screenX - targetSize, screenY + targetSize);
            glEnd();
            
            // Add distance indicator
            char distText[32];
            sprintf(distText, "%.0fm", distance);
            // Note: Actual text rendering would need XPLMDrawString
        }
    }
    
    glDisable(GL_BLEND);
}