/*
 * Visual effects system implementing monochrome filters, thermal effects, and military camera aesthetics for realistic FLIR simulation
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
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "FLIR_VisualEffects.h"

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
static int gPostProcessingEnabled = 1;
static unsigned char* gPixelBuffer = NULL;
static int gBufferWidth = 0;
static int gBufferHeight = 0;

void InitializeVisualEffects()
{
    srand(time(NULL));
}

void CleanupVisualEffects()
{
    if (gPixelBuffer) {
        free(gPixelBuffer);
        gPixelBuffer = NULL;
    }
}

// Safety function to allocate pixel buffer
int AllocatePixelBuffer(int width, int height)
{
    int newSize = width * height * 3; // RGB
    
    if (!gPixelBuffer || gBufferWidth != width || gBufferHeight != height) {
        if (gPixelBuffer) {
            free(gPixelBuffer);
        }
        
        gPixelBuffer = (unsigned char*)malloc(newSize);
        if (!gPixelBuffer) {
            return 0; // Failed to allocate
        }
        
        gBufferWidth = width;
        gBufferHeight = height;
    }
    
    return 1; // Success
}

// Convert RGB to grayscale with EO/IR processing
void ProcessEOIR(unsigned char* pixels, int width, int height, int mode)
{
    for (int i = 0; i < width * height; i++) {
        int idx = i * 3;
        unsigned char r = pixels[idx];
        unsigned char g = pixels[idx + 1];
        unsigned char b = pixels[idx + 2];
        
        // Convert to grayscale (0.0 - 1.0 range)
        float gray = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
        
        // Apply EO/IR processing based on mode
        switch (mode) {
            case 1: // Monochrome with enhancement
                // Aggressive contrast curve
                gray = powf(gray, 0.6f);
                gray = gray * 1.8f - 0.4f;
                gray = fmaxf(0.0f, fminf(1.0f, gray));
                
                // Crush blacks and blow highlights for military look
                if (gray < 0.25f) gray = gray * 0.3f;        // Crush darks
                else if (gray > 0.75f) gray = 0.8f + (gray - 0.75f) * 0.8f; // Compress highlights
                break;
                
            case 2: // Thermal simulation
                // Thermal processing with inversion
                gray = powf(gray, 0.5f) * 1.6f;
                gray = 1.0f - gray; // Invert for thermal (hot = white)
                gray = fmaxf(0.0f, fminf(1.0f, gray));
                break;
                
            case 3: // Enhanced IR
                // Extreme contrast for IR look
                gray = powf(gray, 0.4f);
                gray = gray * 2.5f - 0.8f;
                gray = fmaxf(0.0f, fminf(1.0f, gray));
                
                // Quantize to simulate limited bit depth
                gray = floorf(gray * 32.0f) / 32.0f;
                break;
                
            default:
                // Standard - minimal processing
                break;
        }
        
        // Add very subtle noise for realism
        if (mode > 0) {
            float noise = ((rand() % 21) - 10) / 2000.0f; // -0.005 to +0.005
            gray += noise;
            gray = fmaxf(0.0f, fminf(1.0f, gray));
        }
        
        // Convert back to RGB
        unsigned char finalGray = (unsigned char)(gray * 255.0f);
        
        // For monochrome, add slight green tint
        if (mode == 1) {
            pixels[idx] = (unsigned char)(finalGray * 0.7f);     // R
            pixels[idx + 1] = finalGray;                         // G  
            pixels[idx + 2] = (unsigned char)(finalGray * 0.7f); // B
        } else {
            pixels[idx] = finalGray;     // R
            pixels[idx + 1] = finalGray; // G
            pixels[idx + 2] = finalGray; // B
        }
    }
}

// Main post-processing function with safety checks
void RenderPostProcessing(int screenWidth, int screenHeight)
{
    // Safety check: skip if too small or too large
    if (screenWidth < 100 || screenHeight < 100 || 
        screenWidth > 4096 || screenHeight > 4096) {
        return;
    }
    
    // Allocate buffer if needed
    if (!AllocatePixelBuffer(screenWidth, screenHeight)) {
        return; // Failed to allocate, skip processing
    }
    
    // Clear any OpenGL errors before we start
    while (glGetError() != GL_NO_ERROR) { }
    
    // Read the current framebuffer
    glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, gPixelBuffer);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        return; // Failed to read pixels, skip processing
    }
    
    // Determine processing mode based on current visual effects
    int processingMode = 0;
    if (gMonochromeEnabled) processingMode = 1;
    else if (gThermalEnabled) processingMode = 2;
    else if (gIREnabled) processingMode = 3;
    
    // Only process if we have an active mode
    if (processingMode > 0) {
        ProcessEOIR(gPixelBuffer, screenWidth, screenHeight, processingMode);
    }
    
    // Draw the processed image back
    glRasterPos2f(0, 0);
    glDrawPixels(screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, gPixelBuffer);
    
    // Check for errors again
    error = glGetError();
    if (error != GL_NO_ERROR) {
        // If drawing failed, disable post-processing for safety
        gPostProcessingEnabled = 0;
    }
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
    
    // Try post-processing first (real image processing)
    if (gPostProcessingEnabled && (gMonochromeEnabled || gThermalEnabled || gIREnabled)) {
        RenderPostProcessing(screenWidth, screenHeight);
        
        // Still add overlays like noise and scan lines
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        
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
        
        return;
    }
    
    // Fallback to overlay mode if post-processing fails
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
    
    glColor4f(1.0f, 1.0f, 1.0f, gNoiseIntensity);
    glPointSize(1.0f);
    
    glBegin(GL_POINTS);
    
    srand(gFrameCounter / 2);
    
    int noisePoints = (screenWidth * screenHeight) / 2000;
    for (int i = 0; i < noisePoints; i++) {
        float x = rand() % screenWidth;
        float y = rand() % screenHeight;
        
        float intensity = (rand() % 100) / 100.0f * gNoiseIntensity;
        glColor4f(intensity, intensity, intensity, intensity);
        glVertex2f(x, y);
    }
    
    glEnd();
    
    if ((gFrameCounter % 120) < 3) {
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
    
    glBegin(GL_LINES);
    for (int y = 2; y < screenHeight; y += 3) {
        glVertex2f(0, y);
        glVertex2f(screenWidth, y);
    }
    glEnd();
}

void RenderIRFilter(int screenWidth, int screenHeight)
{
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    
    glColor4f(0.4f, 0.4f, 0.4f, 1.0f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float contrast = gContrast * 1.5f;
    glColor4f(contrast, contrast, contrast, 0.3f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glLineWidth(1.0f);
    
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
    mode = (mode + 1) % 4;
    
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
