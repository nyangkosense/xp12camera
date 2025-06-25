/*
 * FLIR_Camera.cpp
 * 
 * A realistic FLIR camera system plugin for X-Plane 12
 * Features:
 * - Real camera positioning under aircraft (belly-mounted)
 * - True optical zoom with zoom parameter
 * - Pan/tilt camera controls with mouse
 * - Arbitrary point lock-on system
 * - Military-style targeting reticles
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
 #include "FLIR_LockOn.h"

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

// Mouse control for camera
static int gLastMouseX = 0;
static int gLastMouseY = 0;
static float gMouseSensitivity = 0.2f; // Mouse sensitivity multiplier

// Thermal view settings
static int gThermalMode = 1;         // 0=Off, 1=White Hot, 2=Enhanced

// Dataref to control HUD visibility
static XPLMDataRef gManipulatorDisabled = NULL;
 
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

static void DrawRealisticThermalOverlay(void);
 
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

     // Dataref for HUD control
     gManipulatorDisabled = XPLMFindDataRef("sim/operation/prefs/misc/manipulator_disabled");

     // Initialize lock-on system
     InitializeLockOnSystem();

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

     gThermalToggleKey = XPLMRegisterHotKey(XPLM_VK_T, xplm_DownFlag,
                                           "FLIR Thermal Toggle",
                                           ThermalToggleCallback, NULL);
    
    gFocusLockKey = XPLMRegisterHotKey(XPLM_VK_SPACE, xplm_DownFlag,
                                      "FLIR Focus/Lock Target",
                                      FocusLockCallback, NULL);

     XPLMDebugString("FLIR Camera System: Plugin loaded successfully\n");
     XPLMDebugString("FLIR Camera System: Press F9 to activate camera\n");
     XPLMDebugString("FLIR Camera System: MOUSE for smooth pan/tilt, +/- for zoom, arrows for fine adjust, T for thermal, SPACE for lock-on\n");
     
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
         
         // Take camera control
         XPLMControlCamera(xplm_ControlCameraUntilViewChanges, FLIRCameraFunc, NULL);
         gCameraActive = 1;
        
        // Set manipulator disabled for HUD control
        if (gManipulatorDisabled) {
            XPLMSetDatai(gManipulatorDisabled, 1);
        }
        
        // Register 2D drawing callback for overlays
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
         
         // Re-enable manipulator
         if (gManipulatorDisabled) {
             XPLMSetDatai(gManipulatorDisabled, 0);
         }
         
         // Disable lock-on when camera deactivated
         DisableLockOn();
         
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
         // More precise zoom steps: 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0, 24.0, 32.0, 48.0, 64.0
        if (gZoomLevel < 1.5f) gZoomLevel = 1.5f;
        else if (gZoomLevel < 2.0f) gZoomLevel = 2.0f;
        else if (gZoomLevel < 3.0f) gZoomLevel = 3.0f;
        else if (gZoomLevel < 4.0f) gZoomLevel = 4.0f;
        else if (gZoomLevel < 6.0f) gZoomLevel = 6.0f;
        else if (gZoomLevel < 8.0f) gZoomLevel = 8.0f;
        else if (gZoomLevel < 12.0f) gZoomLevel = 12.0f;
        else if (gZoomLevel < 16.0f) gZoomLevel = 16.0f;
        else if (gZoomLevel < 24.0f) gZoomLevel = 24.0f;
        else if (gZoomLevel < 32.0f) gZoomLevel = 32.0f;
        else if (gZoomLevel < 48.0f) gZoomLevel = 48.0f;
        else if (gZoomLevel < 64.0f) gZoomLevel = 64.0f;
        else gZoomLevel = 64.0f; // Maximum zoom
         char msg[256];
         sprintf(msg, "FLIR Camera System: Zoom %.1fx\n", gZoomLevel);
         XPLMDebugString(msg);
     }
 }
 
 static void ZoomOutCallback(void* inRefcon)
 {
     if (gCameraActive) {
         // Reverse zoom steps
        if (gZoomLevel > 48.0f) gZoomLevel = 48.0f;
        else if (gZoomLevel > 32.0f) gZoomLevel = 32.0f;
        else if (gZoomLevel > 24.0f) gZoomLevel = 24.0f;
        else if (gZoomLevel > 16.0f) gZoomLevel = 16.0f;
        else if (gZoomLevel > 12.0f) gZoomLevel = 12.0f;
        else if (gZoomLevel > 8.0f) gZoomLevel = 8.0f;
        else if (gZoomLevel > 6.0f) gZoomLevel = 6.0f;
        else if (gZoomLevel > 4.0f) gZoomLevel = 4.0f;
        else if (gZoomLevel > 3.0f) gZoomLevel = 3.0f;
        else if (gZoomLevel > 2.0f) gZoomLevel = 2.0f;
        else if (gZoomLevel > 1.5f) gZoomLevel = 1.5f;
        else gZoomLevel = 1.0f; // Minimum zoom
         char msg[256];
         sprintf(msg, "FLIR Camera System: Zoom %.1fx\n", gZoomLevel);
         XPLMDebugString(msg);
     }
 }
 
 static void PanLeftCallback(void* inRefcon)
 {
     if (gCameraActive && !IsLockOnActive()) { // Only when not locked
         gCameraPan -= 0.5f; // Very sensitive control
         if (gCameraPan < -180.0f) gCameraPan += 360.0f;
         char msg[256];
         sprintf(msg, "FLIR Camera System: Pan %.1f degrees\n", gCameraPan);
         XPLMDebugString(msg);
     }
 }
 
 static void PanRightCallback(void* inRefcon)
 {
     if (gCameraActive && !IsLockOnActive()) { // Only when not locked
         gCameraPan += 0.5f; // Very sensitive control
         if (gCameraPan > 180.0f) gCameraPan -= 360.0f;
         char msg[256];
         sprintf(msg, "FLIR Camera System: Pan %.1f degrees\n", gCameraPan);
         XPLMDebugString(msg);
     }
 }
 
 static void TiltUpCallback(void* inRefcon)
 {
     if (gCameraActive && !IsLockOnActive()) { // Only when not locked
         gCameraTilt = fminf(gCameraTilt + 0.5f, 45.0f); // Very sensitive control
         char msg[256];
         sprintf(msg, "FLIR Camera System: Tilt %.1f degrees\n", gCameraTilt);
         XPLMDebugString(msg);
     }
 }
 
 static void TiltDownCallback(void* inRefcon)
 {
     if (gCameraActive && !IsLockOnActive()) { // Only when not locked
         gCameraTilt = fmaxf(gCameraTilt - 0.5f, -90.0f); // Very sensitive control
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

static void FocusLockCallback(void* inRefcon)
{
    if (gCameraActive) {
        if (!IsLockOnActive()) {
            // Lock on to current camera direction at 1000m distance
            SetArbitraryLockPoint(gCameraPan, gCameraTilt, 1000.0f);
            XPLMDebugString("FLIR Camera System: Lock-on activated\n");
        } else {
            // Disable lock-on
            DisableLockOn();
            XPLMDebugString("FLIR Camera System: Lock-on disabled\n");
        }
    }
}
 
 // Camera control function
 static int FLIRCameraFunc(XPLMCameraPosition_t* outCameraPosition,
                           int inIsLosingControl,
                           void* inRefcon)
 {
     if (inIsLosingControl) {
         // We're losing control, clean up
         gCameraActive = 0;
         if (gDrawCallbackRegistered) {
             XPLMUnregisterDrawCallback(DrawThermalOverlay, xplm_Phase_Window, 0, NULL);
             gDrawCallbackRegistered = 0;
         }
         // Re-enable manipulator
         if (gManipulatorDisabled) {
             XPLMSetDatai(gManipulatorDisabled, 0);
         }
         DisableLockOn();
         return 0;
     }
     
     if (!gPlaneX || !gPlaneY || !gPlaneZ || !gPlaneHeading || !gPlanePitch || !gPlaneRoll) {
         return 1; // Keep control but don't update position
     }
     
     // Get current aircraft position and orientation
     float planeX = XPLMGetDataf(gPlaneX);
     float planeY = XPLMGetDataf(gPlaneY);
     float planeZ = XPLMGetDataf(gPlaneZ);
     float planeHeading = XPLMGetDataf(gPlaneHeading);
     float planePitch = XPLMGetDataf(gPlanePitch);
     float planeRoll = XPLMGetDataf(gPlaneRoll);
     
     // Mouse control for camera movement (only when not locked)
     if (!IsLockOnActive()) {
         int mouseX, mouseY;
         XPLMGetMouseLocation(&mouseX, &mouseY);
         
         if (gLastMouseX != 0 || gLastMouseY != 0) {
             float deltaX = (mouseX - gLastMouseX) * gMouseSensitivity;
             float deltaY = (mouseY - gLastMouseY) * gMouseSensitivity;
             
             gCameraPan += deltaX;
             gCameraTilt -= deltaY; // Invert Y for natural control
             
             // Normalize pan angle
             while (gCameraPan > 180.0f) gCameraPan -= 360.0f;
             while (gCameraPan < -180.0f) gCameraPan += 360.0f;
             
             // Clamp tilt
             if (gCameraTilt > 45.0f) gCameraTilt = 45.0f;
             if (gCameraTilt < -90.0f) gCameraTilt = -90.0f;
         }
         
         gLastMouseX = mouseX;
         gLastMouseY = mouseY;
     } else {
         // Update camera angles based on lock-on point
         UpdateCameraToLockPoint(&gCameraPan, &gCameraTilt);
     }
     
     // Calculate camera position (belly-mounted)
     float headingRad = planeHeading * M_PI / 180.0f;
     float pitchRad = planePitch * M_PI / 180.0f;
     float rollRad = planeRoll * M_PI / 180.0f;
     
     // Position camera below and slightly forward of aircraft center
     outCameraPosition->x = planeX + gCameraDistance * sin(headingRad);
     outCameraPosition->y = planeY + gCameraHeight;
     outCameraPosition->z = planeZ + gCameraDistance * cos(headingRad);
     
     // Camera orientation (pan/tilt relative to aircraft heading)
     outCameraPosition->heading = planeHeading + gCameraPan;
     outCameraPosition->pitch = gCameraTilt;
     outCameraPosition->roll = 0.0f; // Keep camera level
     
     // Apply zoom
     outCameraPosition->zoom = gZoomLevel;
     
     return 1; // Keep controlling camera
 }
 
 // Drawing callback for thermal overlay
 static int DrawThermalOverlay(XPLMDrawingPhase inPhase,
                              int inIsBefore,
                              void* inRefcon)
 {
     if (!gCameraActive) return 1;
     
     DrawRealisticThermalOverlay();
     
     return 1;
 }

// Realistic thermal overlay drawing
static void DrawRealisticThermalOverlay(void)
{
    int screenWidth, screenHeight;
    XPLMGetScreenSize(&screenWidth, &screenHeight);
    
    // Set up OpenGL for 2D drawing
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Disable depth testing for overlay
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Camera effect - dark vignette border for camera look
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    float borderSize = 60.0f;
    glBegin(GL_QUADS);
    // Top border
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, borderSize);
    glVertex2f(0, borderSize);
    // Bottom border
    glVertex2f(0, screenHeight - borderSize);
    glVertex2f(screenWidth, screenHeight - borderSize);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    // Left border
    glVertex2f(0, 0);
    glVertex2f(borderSize, 0);
    glVertex2f(borderSize, screenHeight);
    glVertex2f(0, screenHeight);
    // Right border
    glVertex2f(screenWidth - borderSize, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(screenWidth - borderSize, screenHeight);
    glEnd();
    
    // Draw thermal border effect if thermal mode is active
    if (gThermalMode > 0) {
        glColor4f(0.0f, 1.0f, 0.0f, 0.2f); // Subtle green tint
        glBegin(GL_QUADS);
        // Top border
        glVertex2f(borderSize, borderSize);
        glVertex2f(screenWidth - borderSize, borderSize);
        glVertex2f(screenWidth - borderSize, borderSize + 10);
        glVertex2f(borderSize, borderSize + 10);
        // Bottom border
        glVertex2f(borderSize, screenHeight - borderSize - 10);
        glVertex2f(screenWidth - borderSize, screenHeight - borderSize - 10);
        glVertex2f(screenWidth - borderSize, screenHeight - borderSize);
        glVertex2f(borderSize, screenHeight - borderSize);
        glEnd();
    }
    
    // Draw crosshair in center
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    
    // Set color based on lock status
    if (IsLockOnActive()) {
        glColor4f(1.0f, 0.0f, 0.0f, 0.8f); // Red when locked
    } else {
        glColor4f(0.0f, 1.0f, 0.0f, 0.8f); // Green when scanning
    }
    
    glLineWidth(2.0f);
    
    // Central crosshair
    glBegin(GL_LINES);
    // Horizontal line
    glVertex2f(centerX - 20, centerY);
    glVertex2f(centerX + 20, centerY);
    // Vertical line
    glVertex2f(centerX, centerY - 20);
    glVertex2f(centerX, centerY + 20);
    glEnd();
    
    // Military-style targeting brackets [ ] - FIXED SIZE
    float bracketSize = 50.0f;  // Fixed size - doesn't change with zoom
    float bracketLength = 20.0f;
    
    glBegin(GL_LINES);
    
    // Top-left bracket [
    glVertex2f(centerX - bracketSize, centerY - bracketSize);
    glVertex2f(centerX - bracketSize + bracketLength, centerY - bracketSize);
    glVertex2f(centerX - bracketSize, centerY - bracketSize);
    glVertex2f(centerX - bracketSize, centerY - bracketSize + bracketLength);
    
    // Top-right bracket ]
    glVertex2f(centerX + bracketSize, centerY - bracketSize);
    glVertex2f(centerX + bracketSize - bracketLength, centerY - bracketSize);
    glVertex2f(centerX + bracketSize, centerY - bracketSize);
    glVertex2f(centerX + bracketSize, centerY - bracketSize + bracketLength);
    
    // Bottom-left bracket [
    glVertex2f(centerX - bracketSize, centerY + bracketSize);
    glVertex2f(centerX - bracketSize + bracketLength, centerY + bracketSize);
    glVertex2f(centerX - bracketSize, centerY + bracketSize);
    glVertex2f(centerX - bracketSize, centerY + bracketSize - bracketLength);
    
    // Bottom-right bracket ]
    glVertex2f(centerX + bracketSize, centerY + bracketSize);
    glVertex2f(centerX + bracketSize - bracketLength, centerY + bracketSize);
    glVertex2f(centerX + bracketSize, centerY + bracketSize);
    glVertex2f(centerX + bracketSize, centerY + bracketSize - bracketLength);
    
    glEnd();
    
    
    // Corner indicators
    glBegin(GL_LINES);
    // Top corners
    glVertex2f(borderSize + 20, borderSize + 20);
    glVertex2f(borderSize + 40, borderSize + 20);
    glVertex2f(borderSize + 20, borderSize + 20);
    glVertex2f(borderSize + 20, borderSize + 40);
    
    glVertex2f(screenWidth - borderSize - 20, borderSize + 20);
    glVertex2f(screenWidth - borderSize - 40, borderSize + 20);
    glVertex2f(screenWidth - borderSize - 20, borderSize + 20);
    glVertex2f(screenWidth - borderSize - 20, borderSize + 40);
    glEnd();
    
    // Center dot
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    glVertex2f(centerX, centerY);
    glEnd();
    
    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
    glPointSize(1.0f);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}