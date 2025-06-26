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

static int gMonochromeEnabled = 0;
static int gThermalEnabled = 1;
static int gIREnabled = 0;
static int gNoiseEnabled = 1;
static int gScanLinesEnabled = 1;
static float gBrightness = 1.0f;
static float gContrast = 1.2f;
static float gNoiseIntensity = 0.1f;
static float gScanLineOpacity = 0.05f;
static int gFrameCounter = 0;

void InitializeVisualEffects()
{
    srand(time(NULL));
}

void SetMonochromeFilter(int enabled)
{
    gMonochromeEnabled = enabled;
}

void SetThermalMode(int enabled)
{
    gThermalEnabled = enabled;
}

void SetIRMode(int enabled)
{
    gIREnabled = enabled;
}

void SetImageEnhancement(float brightness, float contrast)
{
    gBrightness = brightness;
    gContrast = contrast;
}

void RenderVisualEffects(int screenWidth, int screenHeight)
{
    gFrameCounter++;
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    
    if (gMonochromeEnabled) {
        RenderMonochromeFilter(screenWidth, screenHeight);
    }
    
    if (gThermalEnabled) {
        RenderThermalEffects(screenWidth, screenHeight);
    }
    
    if (gNoiseEnabled) {
        RenderCameraNoise(screenWidth, screenHeight);
    }
    
    if (gScanLinesEnabled) {
        RenderScanLines(screenWidth, screenHeight);
    }
    
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void RenderMonochromeFilter(int screenWidth, int screenHeight)
{
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glColor4f(0.3f, 1.0f, 0.3f, 1.0f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
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
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glLineWidth(1.0f);
    
    glBegin(GL_LINES);
    for (int i = 0; i < 10; i++) {
        float y = 50 + i * (screenHeight - 100) / 10.0f;
        glVertex2f(10, y);
        glVertex2f(25, y);
    }
    glEnd();
}

void RenderThermalEffects(int screenWidth, int screenHeight)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.4f, 0.0f, 0.15f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glLineWidth(1.0f);
    
    glBegin(GL_LINES);
    for (int i = 0; i < 10; i++) {
        float y = 50 + i * (screenHeight - 100) / 10.0f;
        glVertex2f(10, y);
        glVertex2f(25, y);
    }
    glEnd();
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
    mode = (mode + 1) % 4; // Only 4 modes now
    
    switch (mode) {
        case 0:
            SetMonochromeFilter(0);
            SetThermalMode(0);
            SetIRMode(0);
            gNoiseEnabled = 0;
            gScanLinesEnabled = 0;
            break;
            
        case 1:
            SetMonochromeFilter(1);
            SetThermalMode(0);
            SetIRMode(0);
            gNoiseEnabled = 1;
            gScanLinesEnabled = 1;
            break;
            
        case 2:
            SetMonochromeFilter(0);
            SetThermalMode(1);
            SetIRMode(0);
            gNoiseEnabled = 1;
            gScanLinesEnabled = 0;
            break;
            
        case 3:
            SetMonochromeFilter(1);
            SetThermalMode(0);
            SetIRMode(0);
            gNoiseEnabled = 1;
            gScanLinesEnabled = 1;
            break;
    }
}

void GetVisualEffectsStatus(char* statusBuffer, int bufferSize)
{
    const char* mode = "STANDARD";
    if (gMonochromeEnabled && gThermalEnabled) mode = "ENHANCED";
    else if (gThermalEnabled) mode = "THERMAL";
    else if (gMonochromeEnabled) mode = "MONO";
    
    snprintf(statusBuffer, bufferSize, "VFX: %s", mode);
    statusBuffer[bufferSize - 1] = '\0';
}