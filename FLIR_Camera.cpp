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
static XPLMHotKeyID gFocusLockKey = NULL;
 
 // Aircraft position datarefs
 static XPLMDataRef gPlaneX = NULL;
 static XPLMDataRef gPlaneY = NULL;
 static XPLMDataRef gPlaneZ = NULL;
 static XPLMDataRef gPlaneHeading = NULL;
 static XPLMDataRef gPlanePitch = NULL;
 static XPLMDataRef gPlaneRoll = NULL;
 
 // FLIR camera state
 static int gCameraActive = 0;
static int gDrawCallbackRegistered = 0;
 static float gZoomLevel = 1.0f;
 static float gCameraPan = 0.0f;      // Left/right rotation (degrees)
 static float gCameraTilt = -15.0f;   // Up/down rotation (degrees)
 static float gCameraHeight = -5.0f;  // Height below aircraft (meters)
 static float gCameraDistance = 3.0f; // Forward/back from aircraft center

// Thermal view settings
static int gThermalMode = 1;         // 0=Off, 1=White Hot, 2=Enhanced
 
 // Target tracking and object detection
 static int gTargetLocked = 0;
static float gFocusDistance = 1000.0f; // Distance to focused target
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
static XPLMDataRef gLocalDate = NULL;
static XPLMDataRef gZuluTime = NULL;
static XPLMDataRef gLatitude = NULL;
static XPLMDataRef gLongitude = NULL;
static XPLMDataRef gAltitude = NULL;
 
 // Function declarations
 static void ActivateFLIRCallback(void* inRefcon);
 static void ZoomInCallback(void* inRefcon);
 static void ZoomOutCallback(void* inRefcon);
 static void PanLeftCallback(void* inRefcon);
 static void PanRightCallback(void* inRefcon);
 static void TiltUpCallback(void* inRefcon);
 static void TiltDownCallback(void* inRefcon);
static void ThermalToggleCallback(void* inRefcon);
static void FocusLockCallback(void* inRefcon);
 
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
    
    // Additional datarefs for HUD display
    gLocalDate = XPLMFindDataRef("sim/time/local_date_days");
    gZuluTime = XPLMFindDataRef("sim/time/zulu_time_sec");
    gLatitude = XPLMFindDataRef("sim/flightmodel/position/latitude");
    gLongitude = XPLMFindDataRef("sim/flightmodel/position/longitude");
    gAltitude = XPLMFindDataRef("sim/flightmodel/position/elevation");
 
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
     // Register 2D drawing callback for overlay - per SDK guidance for overlays/annotations  
    // Only register when camera is active to minimize performance overhead
    // Drawing callback will be registered dynamically when camera is activated
 
     XPLMDebugString("FLIR Camera System: Plugin loaded successfully\n");
     XPLMDebugString("FLIR Camera System: Press F9 to activate camera\n");
     XPLMDebugString("FLIR Camera System: Use +/- for zoom, arrows for pan/tilt, T for thermal, SPACE for focus lock\n");
     
     gThermalToggleKey = XPLMRegisterHotKey(XPLM_VK_T, xplm_DownFlag,
                                           "FLIR Thermal Toggle",
                                           ThermalToggleCallback, NULL);
    
    gFocusLockKey = XPLMRegisterHotKey(XPLM_VK_SPACE, xplm_DownFlag,
                                      "FLIR Focus/Lock Target",
                                      FocusLockCallback, NULL);
 
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
    if (gThermalToggleKey) XPLMUnregisterHotKey(gThermalToggleKey);
    if (gFocusLockKey) XPLMUnregisterHotKey(gFocusLockKey);
 
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
        
        // Register 2D drawing callback for overlays (per SDK guidance)
        if (!gDrawCallbackRegistered) {
            XPLMRegisterDrawCallback(DrawThermalOverlay, xplm_Phase_Window, 0, NULL);
            gDrawCallbackRegistered = 1;
            XPLMDebugString("FLIR Camera System: 2D overlay callback registered\n");
        }
         
         XPLMDebugString("FLIR Camera System: Camera active - mounted under aircraft\n");
     } else {
         // Deactivate camera
         XPLMDontControlCamera();
         gCameraActive = 0;
         // Unregister drawing callback to save performance
        if (gDrawCallbackRegistered) {
            XPLMUnregisterDrawCallback(DrawThermalOverlay, xplm_Phase_Window, 0, NULL);
            gDrawCallbackRegistered = 0;
            XPLMDebugString("FLIR Camera System: 2D overlay callback unregistered\n");
        }
        
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
         gCameraPan -= 2.0f; // More precise control
         if (gCameraPan < -180.0f) gCameraPan += 360.0f;
         char msg[256];
         sprintf(msg, "FLIR Camera System: Pan %.1f degrees\n", gCameraPan);
         XPLMDebugString(msg);
     }
 }
 
 static void PanRightCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gCameraPan += 2.0f; // More precise control
         if (gCameraPan > 180.0f) gCameraPan -= 360.0f;
         char msg[256];
         sprintf(msg, "FLIR Camera System: Pan %.1f degrees\n", gCameraPan);
         XPLMDebugString(msg);
     }
 }
 
 static void TiltUpCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gCameraTilt = fminf(gCameraTilt + 2.0f, 45.0f); // More precise control
         char msg[256];
         sprintf(msg, "FLIR Camera System: Tilt %.1f degrees\n", gCameraTilt);
         XPLMDebugString(msg);
     }
 }
 
 static void TiltDownCallback(void* inRefcon)
 {
     if (gCameraActive) {
         gCameraTilt = fmaxf(gCameraTilt - 2.0f, -90.0f); // More precise control
         char msg[256];
         sprintf(msg, "FLIR Camera System: Tilt %.1f degrees\n", gCameraTilt);
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
    
    // Debug output to verify callback is being called
    static int callCount = 0;
    if (callCount < 5) {
        char debugMsg[256];
        sprintf(debugMsg, "FLIR Camera System: DrawThermalOverlay called, thermal mode: %d\n", gThermalMode);
        XPLMDebugString(debugMsg);
        callCount++;
    }
    
    // Update environmental factors and heat sources
    UpdateEnvironmentalFactors();
    UpdateHeatSources();
    DetectAircraft();
 
     // Set up OpenGL for 2D drawing
     // Get screen dimensions
    int screenWidth, screenHeight;
    XPLMGetScreenSize(&screenWidth, &screenHeight);
    
    // For Window phase, X-Plane already sets up proper 2D coordinates
     XPLMSetGraphicsState(0, 0, 0, 1, 1, 0, 0);
     
     // Window phase provides proper 2D setup - no manual matrix setup needed
 
     // For Window phase, coordinate system is already set up by X-Plane
 
     float centerX = screenWidth / 2.0f;
     float centerY = screenHeight / 2.0f;
 
     // Draw targeting reticle
     if (callCount < 3) {
         char debugMsg[256];
         sprintf(debugMsg, "FLIR: Drawing crosshair at %.0f,%.0f (screen %dx%d)\n", centerX, centerY, screenWidth, screenHeight);
         XPLMDebugString(debugMsg);
     }
     
     glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
     glLineWidth(5.0f); // Make lines thicker to be more visible
 
     glBegin(GL_LINES);
     // Crosshair - make it bigger and more visible
     glVertex2f(centerX - 50, centerY);
     glVertex2f(centerX + 50, centerY);
     glVertex2f(centerX, centerY - 50);
     glVertex2f(centerX, centerY + 50);
     
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
 
     // Draw thermal effects based on thermal mode - using same reliable approach as crosshair
     if (gThermalMode > 0) {
         if (callCount < 3) {
            char debugMsg[256];
            sprintf(debugMsg, "FLIR: Drawing thermal overlay mode %d\n", gThermalMode);
            XPLMDebugString(debugMsg);
         }
         
         // Full-screen thermal overlay using same drawing method as crosshair
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         // Draw thermal background overlay
         if (gThermalMode == 1) {
             // White Hot mode - dark blue/black background
             glColor4f(0.05f, 0.1f, 0.2f, 0.7f); // More opaque for realistic IR view
         } else {
             // Enhanced mode - greenish IR background  
             glColor4f(0.1f, 0.3f, 0.1f, 0.6f);
         }
         
         glBegin(GL_QUADS);
         glVertex2f(0, 0);
         glVertex2f(screenWidth, 0);
         glVertex2f(screenWidth, screenHeight);
         glVertex2f(0, screenHeight);
         glEnd();
         
         // Add IR noise effect using same line drawing approach
         float time = XPLMGetElapsedTime();
         glLineWidth(1.0f);
         
         // Generate IR noise pattern
         for (int y = 0; y < screenHeight; y += 4) {
             for (int x = 0; x < screenWidth; x += 6) {
                 float noise = (sinf(time * 2.0f + x * 0.01f + y * 0.008f) + 
                               cosf(time * 1.5f + x * 0.015f + y * 0.012f)) * 0.5f;
                 
                 if (gThermalMode == 1) {
                     // White hot noise
                     float intensity = 0.1f + noise * 0.15f;
                     glColor4f(intensity, intensity, intensity * 0.9f, 0.8f);
                 } else {
                     // Enhanced mode noise - green tint
                     float intensity = 0.15f + noise * 0.2f;
                     glColor4f(intensity * 0.3f, intensity, intensity * 0.4f, 0.7f);
                 }
                 
                 // Draw noise pixels as small lines
                 glBegin(GL_LINES);
                 glVertex2f(x, y);
                 glVertex2f(x + 2, y);
                 glEnd();
             }
         }
         
         // Add scan lines for authentic IR look
         glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
         glLineWidth(1.0f);
         for (int y = 0; y < screenHeight; y += 3) {
             glBegin(GL_LINES);
             glVertex2f(0, y);
             glVertex2f(screenWidth, y);
             glEnd();
         }
         
         // Only draw realistic heat sources from actual detection, no fake test rectangles
        
        // Draw realistic heat sources if we have any
        if (gHeatSourceCount > 0) {
            DrawRealisticThermalOverlay();
        }
         
         // Also draw some simulated background thermal noise
         // Use the existing time variable from above
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
 
     // Draw realistic military FLIR HUD overlay
     glColor4f(0.0f, 1.0f, 0.0f, 0.8f);
     glLineWidth(1.0f);
     
     // Minimal corner reticles only (like real FLIR systems)
     float reticleSize = 15.0f;
     glBegin(GL_LINES);
     // Top-left reticle
     glVertex2f(30, 30); glVertex2f(30 + reticleSize, 30);
     glVertex2f(30, 30); glVertex2f(30, 30 + reticleSize);
     // Top-right reticle
     glVertex2f(screenWidth - 30 - reticleSize, 30); glVertex2f(screenWidth - 30, 30);
     glVertex2f(screenWidth - 30, 30); glVertex2f(screenWidth - 30, 30 + reticleSize);
     // Bottom-left reticle
     glVertex2f(30, screenHeight - 30 - reticleSize); glVertex2f(30, screenHeight - 30);
     glVertex2f(30, screenHeight - 30); glVertex2f(30 + reticleSize, screenHeight - 30);
     // Bottom-right reticle
     glVertex2f(screenWidth - 30 - reticleSize, screenHeight - 30); glVertex2f(screenWidth - 30, screenHeight - 30);
     glVertex2f(screenWidth - 30, screenHeight - 30); glVertex2f(screenWidth - 30, screenHeight - 30 - reticleSize);
     glEnd();
     
     // Professional FLIR HUD using actual text rendering
     // Get current aircraft data
     float latitude = XPLMGetDataf(gLatitude);
     float longitude = XPLMGetDataf(gLongitude);
     float altitude = XPLMGetDataf(gAltitude);
     float zuluTime = XPLMGetDataf(gZuluTime);
     
     // Convert Zulu time to hours:minutes
     int hours = (int)(zuluTime / 3600.0f) % 24;
     int minutes = (int)((zuluTime - hours * 3600) / 60.0f);
     
     // Use X-Plane's text rendering for professional look
     glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
     
     // Top status line with actual data (using XPLMDrawString for real text)
     char statusLine[256];
     sprintf(statusLine, "FLIR  %02d:%02d UTC  LAT:%.4f LON:%.4f  ALT:%dft  ZOOM:%.1fx", 
             hours, minutes, latitude, longitude, (int)(altitude * 3.28084f), gZoomLevel);
     
     // Draw the status line using X-Plane's text rendering
     glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
     XPLMDrawString(NULL, 30, screenHeight - 30, statusLine, NULL, xplmFont_Proportional);
     
     // Thermal mode indicator
     char thermalMode[64];
     const char* modeNames[] = {"OFF", "WHT", "ENH"};
     sprintf(thermalMode, "THERMAL: %s", modeNames[gThermalMode]);
     XPLMDrawString(NULL, 30, screenHeight - 50, thermalMode, NULL, xplmFont_Proportional);
     
     // Camera orientation display
     char cameraInfo[128];
     sprintf(cameraInfo, "PAN:%.1f  TILT:%.1f", gCameraPan, gCameraTilt);
     XPLMDrawString(NULL, 30, screenHeight - 70, cameraInfo, NULL, xplmFont_Proportional);
     
     // Zoom level indicator (bottom-left, simple and clean)
     glLineWidth(2.0f);
     glBegin(GL_LINES);
     // Simple zoom bar
     float zoomBar = gZoomLevel * 20.0f;
     glVertex2f(30, 40);
     glVertex2f(30 + zoomBar, 40);
     glVertex2f(30, 35);
     glVertex2f(30 + zoomBar, 35);
     glEnd();
     
     // Pan/Tilt position indicators (bottom-right)
     glLineWidth(1.0f);
     glBegin(GL_LINES);
     // Pan indicator - horizontal line that moves
     float panPos = (gCameraPan + 180.0f) / 360.0f * 100.0f;
     glVertex2f(screenWidth - 150, 40);
     glVertex2f(screenWidth - 50, 40);
     glVertex2f(screenWidth - 150 + panPos, 35);
     glVertex2f(screenWidth - 150 + panPos, 45);
     
     // Tilt indicator - vertical line that moves  
     float tiltPos = (gCameraTilt + 90.0f) / 135.0f * 30.0f;
     glVertex2f(screenWidth - 30, 30);
     glVertex2f(screenWidth - 30, 60);
     glVertex2f(screenWidth - 35, 30 + tiltPos);
     glVertex2f(screenWidth - 25, 30 + tiltPos);
     glEnd();
     
     // Target lock indicator (minimal and professional)
     if (gTargetLocked) {
         glColor4f(1.0f, 0.0f, 0.0f, 0.9f); // Red for locked
         glLineWidth(2.0f);
         
         // Simple lock indicator box around center crosshair
         glBegin(GL_LINE_LOOP);
         glVertex2f(centerX - 80, centerY - 80);
         glVertex2f(centerX + 80, centerY - 80);
         glVertex2f(centerX + 80, centerY + 80);
         glVertex2f(centerX - 80, centerY + 80);
         glEnd();
         
         // Lock indicator corners
         glBegin(GL_LINES);
         // Corner brackets
         float lockSize = 20.0f;
         // Top-left
         glVertex2f(centerX - 80, centerY + 80 - lockSize); glVertex2f(centerX - 80, centerY + 80);
         glVertex2f(centerX - 80, centerY + 80); glVertex2f(centerX - 80 + lockSize, centerY + 80);
         // Top-right
         glVertex2f(centerX + 80 - lockSize, centerY + 80); glVertex2f(centerX + 80, centerY + 80);
         glVertex2f(centerX + 80, centerY + 80); glVertex2f(centerX + 80, centerY + 80 - lockSize);
         // Bottom-left
         glVertex2f(centerX - 80, centerY - 80 + lockSize); glVertex2f(centerX - 80, centerY - 80);
         glVertex2f(centerX - 80, centerY - 80); glVertex2f(centerX - 80 + lockSize, centerY - 80);
         // Bottom-right
         glVertex2f(centerX + 80 - lockSize, centerY - 80); glVertex2f(centerX + 80, centerY - 80);
         glVertex2f(centerX + 80, centerY - 80); glVertex2f(centerX + 80, centerY - 80 + lockSize);
         glEnd();
         
         // Distance readout (bottom of lock box)
         if (gFocusDistance > 0) {
             // Simple distance indicator bar
             float distBar = fminf(gFocusDistance / 50.0f, 60.0f); // Scale distance to bar length
             glBegin(GL_LINES);
             glVertex2f(centerX - 30, centerY - 100);
             glVertex2f(centerX - 30 + distBar, centerY - 100);
             glEnd();
         }
     }
     
     // Range circles around center
     glColor4f(0.0f, 1.0f, 0.0f, 0.3f);
     glLineWidth(1.0f);
     for (int i = 1; i <= 3; i++) {
         float radius = 80.0f * i / gZoomLevel;
         if (radius < screenWidth / 4) {
             glBegin(GL_LINE_LOOP);
             for (int angle = 0; angle < 360; angle += 10) {
                 float x = centerX + radius * cosf(angle * M_PI / 180.0f);
                 float y = centerY + radius * sinf(angle * M_PI / 180.0f);
                 glVertex2f(x, y);
             }
             glEnd();
         }
     }
     
     // Heading indicator (top center)
     glColor4f(0.0f, 1.0f, 0.0f, 0.9f);
     glLineWidth(2.0f);
     float headingX = centerX;
     float headingY = screenHeight - 40;
     float currentHeading = XPLMGetDataf(gPlaneHeading);
     
     glBegin(GL_LINES);
     // Heading scale
     for (int h = 0; h < 360; h += 30) {
         float angle = (h - currentHeading - gCameraPan) * M_PI / 180.0f;
         float x1 = headingX + 100 * sinf(angle);
         float x2 = headingX + 110 * sinf(angle);
         if (x1 > 50 && x1 < screenWidth - 50) {
             glVertex2f(x1, headingY);
             glVertex2f(x2, headingY);
         }
     }
     glEnd();
     
     // Center heading marker
     glBegin(GL_LINES);
     glVertex2f(headingX, headingY - 10);
     glVertex2f(headingX, headingY + 10);
     glEnd();
     
     // OpenGL state managed by X-Plane for Window phase
     
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

static void FocusLockCallback(void* inRefcon)
{
    if (gCameraActive) {
        if (!gTargetLocked) {
            // Find nearest target to focus on
            float nearestDistance = 999999.0f;
            int nearestTarget = -1;
            
            // Check heat sources for nearest aircraft
            for (int i = 0; i < gHeatSourceCount; i++) {
                if (gHeatSources[i].type == 0) { // Aircraft
                    float distance = GetDistanceToCamera(gHeatSources[i].x, gHeatSources[i].y, gHeatSources[i].z);
                    if (distance < nearestDistance && distance > 100.0f) { // Minimum 100m distance
                        nearestDistance = distance;
                        nearestTarget = i;
                    }
                }
            }
            
            if (nearestTarget >= 0) {
                gTargetLocked = 1;
                gFocusDistance = nearestDistance;
                char msg[256];
                sprintf(msg, "FLIR Camera System: Target LOCKED at %.0fm\n", gFocusDistance);
                XPLMDebugString(msg);
            } else {
                // No target found, just lock current view
                gTargetLocked = 1;
                gFocusDistance = 1000.0f;
                XPLMDebugString("FLIR Camera System: Focus LOCKED on current view\n");
            }
        } else {
            // Unlock target
            gTargetLocked = 0;
            XPLMDebugString("FLIR Camera System: Target UNLOCKED\n");
        }
    }
}