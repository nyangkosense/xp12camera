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
static unsigned char* gProcessedBuffer = NULL;
static int gBufferWidth = 0;
static int gBufferHeight = 0;
static int gProcessingCounter = 0;
static int gProcessingSkip = 5; // Process every 6th frame for caching
static float gProcessingScale = 0.25f; // Process at quarter resolution
static int gUseHybridMode = 1; // Use hybrid shader+overlay approach

// Forward declarations
void ProcessEOIROptimized(unsigned char* input, unsigned char* output, int width, int height, int mode);
void RenderHybridEffects(int screenWidth, int screenHeight, int mode);

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
    if (gProcessedBuffer) {
        free(gProcessedBuffer);
        gProcessedBuffer = NULL;
    }
}

// Safety function to allocate pixel buffers
int AllocatePixelBuffer(int width, int height)
{
    int fullSize = width * height * 3; // RGB for full resolution
    int processedSize = fullSize;
    
    if (!gPixelBuffer || gBufferWidth != width || gBufferHeight != height) {
        if (gPixelBuffer) {
            free(gPixelBuffer);
        }
        if (gProcessedBuffer) {
            free(gProcessedBuffer);
        }
        
        gPixelBuffer = (unsigned char*)malloc(fullSize);
        gProcessedBuffer = (unsigned char*)malloc(processedSize);
        
        if (!gPixelBuffer || !gProcessedBuffer) {
            CleanupVisualEffects();
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

// Optimized post-processing function
void RenderPostProcessing(int screenWidth, int screenHeight)
{
    // Safety check: skip if too small or too large
    if (screenWidth < 100 || screenHeight < 100 || 
        screenWidth > 4096 || screenHeight > 4096) {
        return;
    }
    
    // Skip frames for better performance
    gProcessingCounter++;
    int shouldProcess = (gProcessingCounter % (gProcessingSkip + 1)) == 0;
    
    // Determine processing mode
    int processingMode = 0;
    if (gMonochromeEnabled) processingMode = 1;
    else if (gThermalEnabled) processingMode = 2;
    else if (gIREnabled) processingMode = 3;
    
    if (processingMode == 0) return; // No processing needed
    
    // Allocate buffers if needed
    if (!AllocatePixelBuffer(screenWidth, screenHeight)) {
        return;
    }
    
    // Only do expensive processing every few frames
    if (shouldProcess) {
        // Clear any OpenGL errors
        while (glGetError() != GL_NO_ERROR) { }
        
        // Read framebuffer
        glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, gPixelBuffer);
        
        // Check for errors
        if (glGetError() != GL_NO_ERROR) {
            return;
        }
        
        // Process with optimized function
        ProcessEOIROptimized(gPixelBuffer, gProcessedBuffer, screenWidth, screenHeight, processingMode);
    }
    
    // Always draw the (possibly cached) processed result
    glRasterPos2f(0, 0);
    glDrawPixels(screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, gProcessedBuffer);
    
    // Check for errors
    if (glGetError() != GL_NO_ERROR) {
        gPostProcessingEnabled = 0; // Disable on error
    }
}

// Much faster processing function with fake heat signatures
void ProcessEOIROptimized(unsigned char* input, unsigned char* output, int width, int height, int mode)
{
    int totalPixels = width * height;
    int step = 1; // Process every pixel, but optimize the loop
    
    // Pre-calculate noise once per frame
    static int noiseFrame = 0;
    static float frameNoise = 0.0f;
    if (noiseFrame != gFrameCounter) {
        frameNoise = ((rand() % 21) - 10) / 3000.0f; // Even more subtle
        noiseFrame = gFrameCounter;
    }
    
    for (int i = 0; i < totalPixels; i += step) {
        int idx = i * 3;
        int x = i % width;
        int y = i / width;
        
        // Original RGB values
        unsigned char r = input[idx];
        unsigned char g = input[idx + 1];
        unsigned char b = input[idx + 2];
        
        // Fast integer-based grayscale conversion
        int gray = (r * 77 + g * 151 + b * 28) >> 8; // /256
        
        // Fake heat signature logic based on color analysis
        int heatBonus = 0;
        
        // Sky detection (blue-ish areas are cold)
        if (b > r && b > g && b > 100) {
            heatBonus = -30; // Sky is cold
        }
        // Vegetation detection (green areas are cooler)  
        else if (g > r && g > b && g > 80) {
            heatBonus = -15; // Vegetation is cooler
        }
        // Ground/concrete detection (neutral colors)
        else if (abs(r - g) < 20 && abs(g - b) < 20 && gray > 60) {
            heatBonus = 10; // Ground/concrete slightly warm
        }
        // Bright objects (could be hot engines, lights, etc)
        else if (gray > 200) {
            heatBonus = 25; // Bright objects assumed warm
        }
        // Very dark objects (shadows, cold areas)
        else if (gray < 40) {
            heatBonus = -20; // Deep shadows are cold
        }
        
        // Ground level is warmer than sky (simple atmospheric model)
        float skyFactor = (float)y / height; // 0 = top, 1 = bottom
        heatBonus += (int)(skyFactor * 15); // Ground +15, sky +0
        
        // Apply heat simulation based on mode
        switch (mode) {
            case 1: // Monochrome - enhanced but not too harsh
                gray += heatBonus / 2; // Subtle heat effect
                gray = (gray * 5) >> 2; // *1.25 instead of *1.5
                if (gray < 0) gray = 20; // Don't go pure black
                if (gray > 255) gray = 255;
                
                // Green tint for night vision
                output[idx] = (unsigned char)((gray * 180) >> 8);     // R * 0.7
                output[idx + 1] = (unsigned char)gray;               // G
                output[idx + 2] = (unsigned char)((gray * 180) >> 8); // B * 0.7
                break;
                
            case 2: // Thermal - with heat signatures
                gray += heatBonus; // Full heat effect
                if (gray < 0) gray = 30; // Minimum visible level
                if (gray > 255) gray = 255;
                
                // Don't fully invert - partial inversion looks more realistic
                gray = 200 - (gray * 3 >> 2); // Partial invert and enhance
                if (gray < 40) gray = 40; // Keep some visibility
                if (gray > 255) gray = 255;
                
                output[idx] = output[idx + 1] = output[idx + 2] = (unsigned char)gray;
                break;
                
            case 3: // Enhanced IR - high contrast but not crushing
                gray += heatBonus;
                if (gray < 0) gray = 25;
                if (gray > 255) gray = 255;
                
                // Less harsh threshold
                if (gray > 140) gray = 240;
                else if (gray > 80) gray = 160;
                else if (gray > 40) gray = 80;
                else gray = 30; // Minimum visibility
                
                output[idx] = output[idx + 1] = output[idx + 2] = (unsigned char)gray;
                break;
                
            default:
                output[idx] = input[idx];
                output[idx + 1] = input[idx + 1];
                output[idx + 2] = input[idx + 2];
                break;
        }
    }
}

// Hybrid approach: Smart overlays that mimic post-processing visually
void RenderHybridEffects(int screenWidth, int screenHeight, int mode)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    
    switch (mode) {
        case 1: // Monochrome mode
            RenderSmartMonochrome(screenWidth, screenHeight);
            break;
        case 2: // Thermal mode  
            RenderSmartThermal(screenWidth, screenHeight);
            break;
        case 3: // Enhanced IR mode
            RenderSmartIR(screenWidth, screenHeight);
            break;
    }
    
    // Add overlays
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

// Smart monochrome that mimics the post-processing look
void RenderSmartMonochrome(int screenWidth, int screenHeight)
{
    // Base desaturation with green tint
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glColor4f(0.7f, 1.0f, 0.7f, 1.0f); // Green night vision tint
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Contrast enhancement
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f); // Darken mid-tones
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Smooth atmospheric gradient (sky to ground)
    glBegin(GL_QUADS);
    // Sky (top) - darker/cooler
    glColor4f(0.0f, 0.0f, 0.0f, 0.25f);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    
    // Horizon (middle) - neutral
    glColor4f(0.0f, 0.0f, 0.0f, 0.05f);
    glVertex2f(screenWidth, screenHeight * 0.5f);
    glVertex2f(0, screenHeight * 0.5f);
    glEnd();
    
    glBegin(GL_QUADS);
    // Horizon (middle) - neutral  
    glColor4f(0.1f, 0.15f, 0.1f, 0.05f);
    glVertex2f(0, screenHeight * 0.5f);
    glVertex2f(screenWidth, screenHeight * 0.5f);
    
    // Ground (bottom) - warmer
    glColor4f(0.15f, 0.2f, 0.15f, 0.15f);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
}

// Smart thermal that mimics the post-processing look
void RenderSmartThermal(int screenWidth, int screenHeight)
{
    // Base inversion effect
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
    glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Prevent pure black - add minimum brightness
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.2f, 0.2f, 0.2f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Smooth thermal gradient (cold sky to warm ground)
    glBegin(GL_QUADS);
    // Cold sky (top)
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    
    // Horizon (middle) - neutral
    glColor4f(0.0f, 0.0f, 0.0f, 0.05f);
    glVertex2f(screenWidth, screenHeight * 0.5f);
    glVertex2f(0, screenHeight * 0.5f);
    glEnd();
    
    glBegin(GL_QUADS);
    // Horizon (middle) - neutral
    glColor4f(0.1f, 0.1f, 0.1f, 0.05f);
    glVertex2f(0, screenHeight * 0.5f);
    glVertex2f(screenWidth, screenHeight * 0.5f);
    
    // Warm ground (bottom)
    glColor4f(0.2f, 0.2f, 0.2f, 0.2f);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
}

// Smart IR that mimics the post-processing look
void RenderSmartIR(int screenWidth, int screenHeight)
{
    // High contrast base
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glColor4f(0.3f, 0.3f, 0.3f, 1.0f); // Darken everything
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Harsh contrast enhancement
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f); // Brighten highlights
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Minimum visibility floor
    glColor4f(0.15f, 0.15f, 0.15f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Add grid pattern for digital look
    glColor4f(1.0f, 1.0f, 1.0f, 0.03f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int x = 0; x < screenWidth; x += 16) {
        glVertex2f(x, 0);
        glVertex2f(x, screenHeight);
    }
    for (int y = 0; y < screenHeight; y += 16) {
        glVertex2f(0, y);
        glVertex2f(screenWidth, y);
    }
    glEnd();
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
    
    // Determine processing mode
    int processingMode = 0;
    if (gMonochromeEnabled) processingMode = 1;
    else if (gThermalEnabled) processingMode = 2;
    else if (gIREnabled) processingMode = 3;
    
    // Try hybrid approach first (much faster but same visual quality)
    if (gUseHybridMode && processingMode > 0) {
        RenderHybridEffects(screenWidth, screenHeight, processingMode);
        return;
    }
    
    // Fallback to post-processing (slower but works)
    if (gPostProcessingEnabled && processingMode > 0) {
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
