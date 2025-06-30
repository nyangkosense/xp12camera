/*
 * X-Plane 12 FLIR camera simulation plugin with realistic belly-mounted camera positioning,
 * optical zoom, pan/tilt controls, military-style targeting reticles, and thermal overlay effects
 *
 * MIT License
 * 
 * Copyright (c) 2025 sebastian <sebastian@eingabeausgabe.io>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <GL/gl.h>

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
#include "FLIR_SimpleLock.h"
#include "FLIR_VisualEffects.h"
static XPLMHotKeyID gActivateKey = NULL;
static XPLMHotKeyID gZoomInKey = NULL;
static XPLMHotKeyID gZoomOutKey = NULL;
static XPLMHotKeyID gPanLeftKey = NULL;
static XPLMHotKeyID gPanRightKey = NULL;
static XPLMHotKeyID gTiltUpKey = NULL;
static XPLMHotKeyID gTiltDownKey = NULL;
static XPLMHotKeyID gThermalToggleKey = NULL;
static XPLMHotKeyID gFocusLockKey = NULL;

static XPLMDataRef gPlaneX = NULL;
static XPLMDataRef gPlaneY = NULL;
static XPLMDataRef gPlaneZ = NULL;
static XPLMDataRef gPlaneHeading = NULL;
static XPLMDataRef gPlanePitch = NULL;
static XPLMDataRef gPlaneRoll = NULL;
static XPLMDataRef gManipulatorDisabled = NULL;

static int gCameraActive = 0;
static int gDrawCallbackRegistered = 0;
static float gZoomLevel = 1.0f;
static float gCameraPan = 0.0f;
static float gCameraTilt = -15.0f;
static float gCameraHeight = -5.0f;
static float gCameraDistance = 3.0f;
static int gLastMouseX = 0;
static int gLastMouseY = 0;
static float gMouseSensitivity = 0.2f;
static float gBasePanSpeed = 0.5f;
static float gBaseTiltSpeed = 0.5f;
static void ActivateFLIRCallback(void* inRefcon);
static void ZoomInCallback(void* inRefcon);
static void ZoomOutCallback(void* inRefcon);
static void PanLeftCallback(void* inRefcon);
static void PanRightCallback(void* inRefcon);
static void TiltUpCallback(void* inRefcon);
static void TiltDownCallback(void* inRefcon);
static void ThermalToggleCallback(void* inRefcon);
static void FocusLockCallback(void* inRefcon);
static int FLIRCameraFunc(XPLMCameraPosition_t* outCameraPosition, int inIsLosingControl, void* inRefcon);
static int DrawThermalOverlay(XPLMDrawingPhase inPhase, int inIsBefore, void* inRefcon);
static void DrawRealisticThermalOverlay(void);
static float GetZoomBasedSensitivity(float baseSpeed);
 
PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Camera System");
    strcpy(outSig, "flir.camera.system");
    strcpy(outDesc, "Realistic FLIR camera with zoom and thermal overlay");

    gPlaneX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gPlaneY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gPlaneZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gPlaneHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    gPlanePitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    gPlaneRoll = XPLMFindDataRef("sim/flightmodel/position/phi");
    gManipulatorDisabled = XPLMFindDataRef("sim/operation/prefs/misc/manipulator_disabled");

    InitializeSimpleLock();
    InitializeVisualEffects();
    gActivateKey = XPLMRegisterHotKey(XPLM_VK_F9, xplm_DownFlag, "Activate FLIR Camera", ActivateFLIRCallback, NULL);
    gZoomInKey = XPLMRegisterHotKey(XPLM_VK_EQUAL, xplm_DownFlag, "FLIR Zoom In", ZoomInCallback, NULL);
    gZoomOutKey = XPLMRegisterHotKey(XPLM_VK_MINUS, xplm_DownFlag, "FLIR Zoom Out", ZoomOutCallback, NULL);
    gPanLeftKey = XPLMRegisterHotKey(XPLM_VK_LEFT, xplm_DownFlag, "FLIR Pan Left", PanLeftCallback, NULL);
    gPanRightKey = XPLMRegisterHotKey(XPLM_VK_RIGHT, xplm_DownFlag, "FLIR Pan Right", PanRightCallback, NULL);
    gTiltUpKey = XPLMRegisterHotKey(XPLM_VK_UP, xplm_DownFlag, "FLIR Tilt Up", TiltUpCallback, NULL);
    gTiltDownKey = XPLMRegisterHotKey(XPLM_VK_DOWN, xplm_DownFlag, "FLIR Tilt Down", TiltDownCallback, NULL);
    gThermalToggleKey = XPLMRegisterHotKey(XPLM_VK_T, xplm_DownFlag, "FLIR Visual Effects Toggle", ThermalToggleCallback, NULL);
    gFocusLockKey = XPLMRegisterHotKey(XPLM_VK_SPACE, xplm_DownFlag, "FLIR Focus/Lock Target", FocusLockCallback, NULL);

    return 1;
}
PLUGIN_API void XPluginStop(void)
{
    if (gActivateKey) XPLMUnregisterHotKey(gActivateKey);
    if (gZoomInKey) XPLMUnregisterHotKey(gZoomInKey);
    if (gZoomOutKey) XPLMUnregisterHotKey(gZoomOutKey);
    if (gPanLeftKey) XPLMUnregisterHotKey(gPanLeftKey);
    if (gPanRightKey) XPLMUnregisterHotKey(gPanRightKey);
    if (gTiltUpKey) XPLMUnregisterHotKey(gTiltUpKey);
    if (gTiltDownKey) XPLMUnregisterHotKey(gTiltDownKey);
    if (gThermalToggleKey) XPLMUnregisterHotKey(gThermalToggleKey);
    if (gFocusLockKey) XPLMUnregisterHotKey(gFocusLockKey);

    if (gCameraActive) {
        XPLMDontControlCamera();
        gCameraActive = 0;
    }
    
    CleanupShaders();
}
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }
 
static void ActivateFLIRCallback(void* inRefcon)
{
    if (!gCameraActive) {
        XPLMControlCamera(xplm_ControlCameraUntilViewChanges, FLIRCameraFunc, NULL);
        gCameraActive = 1;
        
        if (gManipulatorDisabled) {
            XPLMSetDatai(gManipulatorDisabled, 1);
        }
        
        if (!gDrawCallbackRegistered) {
            XPLMRegisterDrawCallback(DrawThermalOverlay, xplm_Phase_Window, 0, NULL);
            gDrawCallbackRegistered = 1;
        }
    } else {
        XPLMDontControlCamera();
        gCameraActive = 0;
        
        if (gManipulatorDisabled) {
            XPLMSetDatai(gManipulatorDisabled, 0);
        }
        
        DisableSimpleLock();
        
        if (gDrawCallbackRegistered) {
            XPLMUnregisterDrawCallback(DrawThermalOverlay, xplm_Phase_Window, 0, NULL);
            gDrawCallbackRegistered = 0;
        }
    }
}
 
static void ZoomInCallback(void* inRefcon)
{
    if (gCameraActive) {
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
        else gZoomLevel = 64.0f;
    }
}
 
static void ZoomOutCallback(void* inRefcon)
{
    if (gCameraActive) {
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
        else gZoomLevel = 1.0f;
    }
}
 
static float GetZoomBasedSensitivity(float baseSpeed)
{
    float zoomFactor = gZoomLevel / 64.0f;
    float sensitivity = baseSpeed * (1.0f - (zoomFactor * 0.95f));
    return fmaxf(sensitivity, baseSpeed * 0.05f);
}

static void PanLeftCallback(void* inRefcon)
{
    if (gCameraActive && !IsSimpleLockActive()) {
        float speed = GetZoomBasedSensitivity(gBasePanSpeed);
        gCameraPan -= speed;
        if (gCameraPan < -180.0f) gCameraPan += 360.0f;
    }
}
 
static void PanRightCallback(void* inRefcon)
{
    if (gCameraActive && !IsSimpleLockActive()) {
        float speed = GetZoomBasedSensitivity(gBasePanSpeed);
        gCameraPan += speed;
        if (gCameraPan > 180.0f) gCameraPan -= 360.0f;
    }
}
 
static void TiltUpCallback(void* inRefcon)
{
    if (gCameraActive && !IsSimpleLockActive()) {
        float speed = GetZoomBasedSensitivity(gBaseTiltSpeed);
        gCameraTilt = fminf(gCameraTilt + speed, 45.0f);
    }
}
 
static void TiltDownCallback(void* inRefcon)
{
    if (gCameraActive && !IsSimpleLockActive()) {
        float speed = GetZoomBasedSensitivity(gBaseTiltSpeed);
        gCameraTilt = fmaxf(gCameraTilt - speed, -90.0f);
    }
}
 
static void ThermalToggleCallback(void* inRefcon)
{
    if (gCameraActive) {
        CycleVisualModes();
    }
}

static void FocusLockCallback(void* inRefcon)
{
    if (gCameraActive) {
        if (!IsSimpleLockActive()) {
            LockCurrentDirection(gCameraPan, gCameraTilt);
        } else {
            DisableSimpleLock();
        }
    }
}
 
static int FLIRCameraFunc(XPLMCameraPosition_t* outCameraPosition, int inIsLosingControl, void* inRefcon)
{
    if (inIsLosingControl) {
        gCameraActive = 0;
        if (gDrawCallbackRegistered) {
            XPLMUnregisterDrawCallback(DrawThermalOverlay, xplm_Phase_Window, 0, NULL);
            gDrawCallbackRegistered = 0;
        }
        if (gManipulatorDisabled) {
            XPLMSetDatai(gManipulatorDisabled, 0);
        }
        DisableSimpleLock();
        return 0;
    }
    
    if (!gPlaneX || !gPlaneY || !gPlaneZ || !gPlaneHeading || !gPlanePitch || !gPlaneRoll) {
        return 1;
    }
    
    float planeX = XPLMGetDataf(gPlaneX);
    float planeY = XPLMGetDataf(gPlaneY);
    float planeZ = XPLMGetDataf(gPlaneZ);
    float planeHeading = XPLMGetDataf(gPlaneHeading);
    
    float headingRad = planeHeading * M_PI / 180.0f;
    
    outCameraPosition->x = planeX + gCameraDistance * sin(headingRad);
    outCameraPosition->y = planeY + gCameraHeight;
    outCameraPosition->z = planeZ + gCameraDistance * cos(headingRad);
    
    if (!IsSimpleLockActive()) {
        int mouseX, mouseY;
        XPLMGetMouseLocation(&mouseX, &mouseY);
        
        if (gLastMouseX != 0 || gLastMouseY != 0) {
            float zoomBasedMouseSensitivity = GetZoomBasedSensitivity(gMouseSensitivity);
            float deltaX = (mouseX - gLastMouseX) * zoomBasedMouseSensitivity;
            float deltaY = (mouseY - gLastMouseY) * zoomBasedMouseSensitivity;
            
            gCameraPan += deltaX;
            gCameraTilt -= deltaY;
            
            while (gCameraPan > 180.0f) gCameraPan -= 360.0f;
            while (gCameraPan < -180.0f) gCameraPan += 360.0f;
            
            if (gCameraTilt > 45.0f) gCameraTilt = 45.0f;
            if (gCameraTilt < -90.0f) gCameraTilt = -90.0f;
        }
        
        gLastMouseX = mouseX;
        gLastMouseY = mouseY;
    } else {
        GetLockedAngles(&gCameraPan, &gCameraTilt);
    }
    
    outCameraPosition->heading = planeHeading + gCameraPan;
    outCameraPosition->pitch = gCameraTilt;
    outCameraPosition->roll = 0.0f;
    outCameraPosition->zoom = gZoomLevel;
    
    return 1;
}
 
static int DrawThermalOverlay(XPLMDrawingPhase inPhase, int inIsBefore, void* inRefcon)
{
    if (!gCameraActive) return 1;
    
    DrawRealisticThermalOverlay();
    
    return 1;
}

static void DrawRealisticThermalOverlay(void)
{
    int screenWidth, screenHeight;
    XPLMGetScreenSize(&screenWidth, &screenHeight);
    
    RenderVisualEffects(screenWidth, screenHeight);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    
    if (IsSimpleLockActive()) {
        glColor4f(1.0f, 0.0f, 0.0f, 0.9f);
    } else {
        glColor4f(0.0f, 1.0f, 0.0f, 0.9f);
    }
    
    glLineWidth(2.0f);
    
    glBegin(GL_LINES);
    glVertex2f(centerX - 20, centerY);
    glVertex2f(centerX + 20, centerY);
    glVertex2f(centerX, centerY - 20);
    glVertex2f(centerX, centerY + 20);
    glEnd();
    
    float bracketSize = 50.0f;
    float bracketLength = 20.0f;
    
    glBegin(GL_LINES);
    glVertex2f(centerX - bracketSize, centerY - bracketSize);
    glVertex2f(centerX - bracketSize + bracketLength, centerY - bracketSize);
    glVertex2f(centerX - bracketSize, centerY - bracketSize);
    glVertex2f(centerX - bracketSize, centerY - bracketSize + bracketLength);
    
    glVertex2f(centerX + bracketSize, centerY - bracketSize);
    glVertex2f(centerX + bracketSize - bracketLength, centerY - bracketSize);
    glVertex2f(centerX + bracketSize, centerY - bracketSize);
    glVertex2f(centerX + bracketSize, centerY - bracketSize + bracketLength);
    
    glVertex2f(centerX - bracketSize, centerY + bracketSize);
    glVertex2f(centerX - bracketSize + bracketLength, centerY + bracketSize);
    glVertex2f(centerX - bracketSize, centerY + bracketSize);
    glVertex2f(centerX - bracketSize, centerY + bracketSize - bracketLength);
    
    glVertex2f(centerX + bracketSize, centerY + bracketSize);
    glVertex2f(centerX + bracketSize - bracketLength, centerY + bracketSize);
    glVertex2f(centerX + bracketSize, centerY + bracketSize);
    glVertex2f(centerX + bracketSize, centerY + bracketSize - bracketLength);
    glEnd();
    
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glVertex2f(centerX, centerY);
    glEnd();
    
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
    glPointSize(1.0f);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
