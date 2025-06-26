/*
 * FLIR_VisualEffects.cpp
 * 
 * Visual effects system for realistic military camera simulation
 * Implements monochrome filters, thermal effects, and military camera aesthetics
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "FLIR_VisualEffects.h"

// OpenGL includes for MinGW
#include <windows.h>
#include <GL/gl.h>

// Visual effects state
static int gMonochromeEnabled = 0;
static int gThermalEnabled = 1;
static int gIREnabled = 0;
static int gNoiseEnabled = 1;
static int gScanLinesEnabled = 1;
static float gBrightness = 1.0f;
static float gContrast = 1.2f;

// Effect parameters
static float gNoiseIntensity = 0.1f;
static float gScanLineOpacity = 0.05f;
static int gFrameCounter = 0;

void InitializeVisualEffects()
{
    srand(time(NULL));
    XPLMDebugString("FLIR Visual Effects: Initialized\n");
}

void SetMonochromeFilter(int enabled)
{
    gMonochromeEnabled = enabled;
    char msg[256];
    sprintf(msg, "FLIR Visual Effects: Monochrome %s\n", enabled ? "ON" : "OFF");
    XPLMDebugString(msg);
}

void SetThermalMode(int enabled)
{
    gThermalEnabled = enabled;
    char msg[256];
    sprintf(msg, "FLIR Visual Effects: Thermal %s\n", enabled ? "ON" : "OFF");
    XPLMDebugString(msg);
}

void SetIRMode(int enabled)
{
    gIREnabled = enabled;
    char msg[256];
    sprintf(msg, "FLIR Visual Effects: IR (B/W) %s\n", enabled ? "ON" : "OFF");
    XPLMDebugString(msg);
}

void SetImageEnhancement(float brightness, float contrast)
{
    gBrightness = brightness;
    gContrast = contrast;
    char msg[256];
    sprintf(msg, "FLIR Visual Effects: Brightness=%.1f, Contrast=%.1f\n", brightness, contrast);
    XPLMDebugString(msg);
}

void RenderVisualEffects(int screenWidth, int screenHeight)
{
    gFrameCounter++;
    
    // Set up OpenGL for 2D screen-space effects
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Disable depth testing for screen overlays
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    
    // Apply monochrome filter
    if (gMonochromeEnabled) {
        RenderMonochromeFilter(screenWidth, screenHeight);
    }
    
    // Apply thermal vision effects
    if (gThermalEnabled) {
        RenderThermalEffects(screenWidth, screenHeight);
    }
    
    // Apply IR black/white filter
    if (gIREnabled) {
        RenderIRFilter(screenWidth, screenHeight);
    }
    
    // Add camera noise/grain
    if (gNoiseEnabled) {
        RenderCameraNoise(screenWidth, screenHeight);
    }
    
    // Add scan lines for CRT-like effect
    if (gScanLinesEnabled) {
        RenderScanLines(screenWidth, screenHeight);
    }
    
    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void RenderMonochromeFilter(int screenWidth, int screenHeight)
{
    // Monochrome overlay using color blending
    glBlendFunc(GL_DST_COLOR, GL_ZERO); // Multiply blend mode
    
    // Create a subtle green monochrome tint (military night vision style)
    glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Add brightness/contrast adjustment
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float brightness_adj = (gBrightness - 1.0f) * 0.3f;
    if (brightness_adj > 0) {
        glColor4f(brightness_adj, brightness_adj, brightness_adj, 0.5f);
    } else {
        glColor4f(0.0f, 0.0f, 0.0f, -brightness_adj * 0.5f);
    }
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
}

void RenderThermalEffects(int screenWidth, int screenHeight)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Thermal overlay with heat signature simulation
    glColor4f(1.0f, 0.4f, 0.0f, 0.15f); // Orange thermal tint
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Add thermal border indicators
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glLineWidth(1.0f);
    
    glBegin(GL_LINES);
    // Temperature scale on left
    for (int i = 0; i < 10; i++) {
        float y = 50 + i * (screenHeight - 100) / 10.0f;
        glVertex2f(10, y);
        glVertex2f(25, y);
    }
    glEnd();
    
    // Add "WHT HOT" indicator
    // (Text would be drawn here if we had font rendering)
}

void RenderCameraNoise(int screenWidth, int screenHeight)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Random noise points for camera grain effect
    glColor4f(1.0f, 1.0f, 1.0f, gNoiseIntensity);
    glPointSize(1.0f);
    
    glBegin(GL_POINTS);
    
    // Generate pseudo-random noise based on frame counter
    srand(gFrameCounter / 2); // Change noise every 2 frames
    
    int noisePoints = (screenWidth * screenHeight) / 2000; // Density control
    for (int i = 0; i < noisePoints; i++) {
        float x = rand() % screenWidth;
        float y = rand() % screenHeight;
        
        // Vary intensity per pixel
        float intensity = (rand() % 100) / 100.0f * gNoiseIntensity;
        glColor4f(intensity, intensity, intensity, intensity);
        glVertex2f(x, y);
    }
    
    glEnd();
    
    // Add electronic interference lines occasionally
    if ((gFrameCounter % 120) < 3) { // Brief interference every 2 seconds
        glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
        glLineWidth(1.0f);
        
        glBegin(GL_LINES);
        for (int i = 0; i < 5; i++) {
            float y = rand() % screenHeight;
            glVertex2f(0, y);
            glVertex2f(screenWidth, y);
        }
        glEnd();
    }
}

void RenderScanLines(int screenWidth, int screenHeight)
{
    if (gScanLineOpacity <= 0.0f) return;
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, gScanLineOpacity);
    glLineWidth(1.0f);
    
    // Horizontal scan lines every 2-3 pixels
    glBegin(GL_LINES);
    for (int y = 2; y < screenHeight; y += 3) {
        glVertex2f(0, y);
        glVertex2f(screenWidth, y);
    }
    glEnd();
}

void RenderIRFilter(int screenWidth, int screenHeight)
{
    // IR Black/White high contrast filter
    glBlendFunc(GL_DST_COLOR, GL_ZERO); // Multiply blend for desaturation
    
    // Create strong black/white contrast effect
    glColor4f(0.4f, 0.4f, 0.4f, 1.0f); // Desaturate and darken
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Add high contrast overlay
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Boost contrast for IR effect
    float contrast = gContrast * 1.5f;
    glColor4f(contrast, contrast, contrast, 0.3f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Add IR-style edge enhancement
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glLineWidth(1.0f);
    
    // Subtle grid pattern for IR sensor simulation
    glBegin(GL_LINES);
    for (int x = 0; x < screenWidth; x += 8) {
        glVertex2f(x, 0);
        glVertex2f(x, screenHeight);
    }
    for (int y = 0; y < screenHeight; y += 8) {
        glVertex2f(0, y);
        glVertex2f(screenWidth, y);
    }
    glEnd();
}

void CycleVisualModes()
{
    static int mode = 0;
    mode = (mode + 1) % 5;
    
    switch (mode) {
        case 0: // Standard
            SetMonochromeFilter(0);
            SetThermalMode(0);
            SetIRMode(0);
            gNoiseEnabled = 0;
            gScanLinesEnabled = 0;
            XPLMDebugString("FLIR Visual Effects: Standard Mode\n");
            break;
            
        case 1: // Monochrome
            SetMonochromeFilter(1);
            SetThermalMode(0);
            SetIRMode(0);
            gNoiseEnabled = 1;
            gScanLinesEnabled = 1;
            XPLMDebugString("FLIR Visual Effects: Monochrome Mode\n");
            break;
            
        case 2: // Thermal
            SetMonochromeFilter(0);
            SetThermalMode(1);
            SetIRMode(0);
            gNoiseEnabled = 1;
            gScanLinesEnabled = 0;
            XPLMDebugString("FLIR Visual Effects: Thermal Mode\n");
            break;
            
        case 3: // IR Black/White
            SetMonochromeFilter(0);
            SetThermalMode(0);
            SetIRMode(1);
            gNoiseEnabled = 1;
            gScanLinesEnabled = 1;
            XPLMDebugString("FLIR Visual Effects: IR Mode\n");
            break;
            
        case 4: // Enhanced
            SetMonochromeFilter(1);
            SetThermalMode(1);
            SetIRMode(0);
            gNoiseEnabled = 1;
            gScanLinesEnabled = 1;
            XPLMDebugString("FLIR Visual Effects: Enhanced Mode\n");
            break;
    }
}

void GetVisualEffectsStatus(char* statusBuffer, int bufferSize)
{
    const char* mode = "STANDARD";
    if (gMonochromeEnabled && gThermalEnabled) mode = "ENHANCED";
    else if (gThermalEnabled) mode = "THERMAL";
    else if (gMonochromeEnabled) mode = "MONO";
    else if (gIREnabled) mode = "IR B/W";
    
    snprintf(statusBuffer, bufferSize, "VFX: %s", mode);
    statusBuffer[bufferSize - 1] = '\0';
}