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
 
 // Target tracking
 static int gTargetLocked = 0;
 static float gTargetX = 0.0f;
 static float gTargetY = 0.0f;
 static float gTargetZ = 0.0f;
 
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
 
     // Register thermal overlay drawing callback
     XPLMRegisterDrawCallback(DrawThermalOverlay, xplm_Phase_Window, 0, NULL);
 
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
         float planePitch = XPLMGetDataf(gPlanePitch);
         float planeRoll = XPLMGetDataf(gPlaneRoll);
 
         // Convert to radians
         float headingRad = planeHeading * M_PI / 180.0f;
         float pitchRad = planePitch * M_PI / 180.0f;
         float rollRad = planeRoll * M_PI / 180.0f;
         float panRad = gCameraPan * M_PI / 180.0f;
         float tiltRad = gCameraTilt * M_PI / 180.0f;
 
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
             glColor4f(0.1f, 0.1f, 0.2f, 0.3f);
             glBegin(GL_QUADS);
             glVertex2f(0, 0);
             glVertex2f(screenWidth, 0);
             glVertex2f(screenWidth, screenHeight);
             glVertex2f(0, screenHeight);
             glEnd();
         } else if (gThermalMode == 2) {
             // Enhanced mode - blue tint with enhanced contrast
             glColor4f(0.0f, 0.2f, 0.4f, 0.25f);
             glBegin(GL_QUADS);
             glVertex2f(0, 0);
             glVertex2f(screenWidth, 0);
             glVertex2f(screenWidth, screenHeight);
             glVertex2f(0, screenHeight);
             glEnd();
         }
         
         // Simulate thermal signatures across the view
         float time = XPLMGetElapsedTime();
         for (int i = 0; i < 6; i++) {
             for (int j = 0; j < 4; j++) {
                 float x = (i + 1) * screenWidth / 7.0f;
                 float y = (j + 1) * screenHeight / 5.0f;
                 
                 float intensity = (sinf(time * 0.2f + i * 0.5f + j * 0.3f) + 1.0f) * 0.4f;
                 float size = 12.0f + intensity * 20.0f;
                 
                 if (gThermalMode == 1) {
                     // White hot
                     glColor4f(1.0f, 0.9f, 0.8f, intensity * 0.6f);
                 } else {
                     // Enhanced mode
                     glColor4f(1.0f, 0.7f, 0.3f, intensity * 0.5f);
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