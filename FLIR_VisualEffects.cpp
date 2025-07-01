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
#include <GL/glext.h>

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
static float gGamma = 2.2f;
static float gGain = 1.5f;
static float gBias = 0.1f;
static int gEdgeEnhancement = 0;
static float gEdgeIntensity = 0.8f;
static int gUseShaders = 0;
static unsigned int gShaderProgram = 0;
static unsigned int gVAO = 0;
static unsigned int gVBO = 0;
static unsigned int gFramebuffer = 0;
static unsigned int gColorTexture = 0;
static int gShaderMode = 0;

// OpenGL function pointers
static PFNGLCREATESHADERPROC glCreateShader = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
static PFNGLDELETESHADERPROC glDeleteShader = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
static PFNGLATTACHSHADERPROC glAttachShader = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
static PFNGLUNIFORM1IPROC glUniform1i = NULL;
static PFNGLUNIFORM1FPROC glUniform1f = NULL;
static PFNGLUNIFORM2FPROC glUniform2f = NULL;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
static PFNGLGENBUFFERSPROC glGenBuffers = NULL;
static PFNGLBINDBUFFERPROC glBindBuffer = NULL;
static PFNGLBUFFERDATAPROC glBufferData = NULL;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = NULL;

static void LoadOpenGLExtensions()
{
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
}

static char* LoadShaderFile(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = (char*)malloc(length + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    
    return content;
}

int LoadShader(const char* vertexPath, const char* fragmentPath)
{
    if (!glCreateShader) {
        return 0;
    }
    
    char* vertexSource = LoadShaderFile(vertexPath);
    char* fragmentSource = LoadShaderFile(fragmentPath);
    
    if (!vertexSource || !fragmentSource) {
        if (vertexSource) free(vertexSource);
        if (fragmentSource) free(fragmentSource);
        return 0;
    }
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const char**)&vertexSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        glDeleteShader(vertexShader);
        free(vertexSource);
        free(fragmentSource);
        return 0;
    }
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const char**)&fragmentSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        free(vertexSource);
        free(fragmentSource);
        return 0;
    }
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        free(vertexSource);
        free(fragmentSource);
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    free(vertexSource);
    free(fragmentSource);
    
    return program;
}

void InitializeShaders()
{
    LoadOpenGLExtensions();
    
    if (!glCreateShader) {
        return;
    }
    
    gShaderProgram = LoadShader("shaders/ir_filter.vert", "shaders/ir_filter.frag");
    if (gShaderProgram) {
        gUseShaders = 1;
        
        float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f
        };
        
        glGenVertexArrays(1, &gVAO);
        glGenBuffers(1, &gVBO);
        
        glBindVertexArray(gVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void CleanupShaders()
{
    if (gShaderProgram) {
        glDeleteProgram(gShaderProgram);
    }
}

void UseShaderMode(int shaderMode)
{
    gShaderMode = shaderMode;
}

void InitializeVisualEffects()
{
    srand(time(NULL));
    InitializeShaders();
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

void SetContrastCurve(float gamma, float gain, float bias)
{
    gGamma = gamma;
    gGain = gain;
    gBias = bias;
}

void SetEdgeEnhancement(int enabled, float intensity)
{
    gEdgeEnhancement = enabled;
    gEdgeIntensity = intensity;
}

void RenderVisualEffects(int screenWidth, int screenHeight)
{
    gFrameCounter++;
    
    if (gUseShaders && gShaderProgram) {
        RenderWithShader(screenWidth, screenHeight);
        return;
    }
    
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
    
    if (gIREnabled) {
        RenderIRFilter(screenWidth, screenHeight);
    }
    
    if (gNoiseEnabled) {
        RenderCameraNoise(screenWidth, screenHeight);
    }
    
    if (gScanLinesEnabled) {
        RenderScanLines(screenWidth, screenHeight);
    }
    
    RenderContrastEnhancement(screenWidth, screenHeight);
    
    if (gEdgeEnhancement) {
        RenderEdgeEnhancement(screenWidth, screenHeight);
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
    // Much more subtle thermal overlay
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.4f, 0.0f, 0.08f); // Very light orange tint
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    // Add very subtle thermal noise
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    
    srand(gFrameCounter / 4); // Slower thermal fluctuation
    int thermalPoints = (screenWidth * screenHeight) / 8000; // Much fewer points
    for (int i = 0; i < thermalPoints; i++) {
        float x = rand() % screenWidth;
        float y = rand() % screenHeight;
        
        float intensity = (rand() % 20) / 100.0f * 0.1f; // Much more subtle
        glColor4f(intensity, intensity * 0.5f, intensity * 0.2f, 0.15f);
        glVertex2f(x, y);
    }
    glEnd();
    
    // Simple temperature scale indicator
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
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
    
    // More realistic noise pattern
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    
    // Much more subtle temporal noise
    srand(gFrameCounter / 2);
    int temporalNoisePoints = (screenWidth * screenHeight) / 8000; // Reduced
    for (int i = 0; i < temporalNoisePoints; i++) {
        float x = rand() % screenWidth;
        float y = rand() % screenHeight;
        
        float intensity = (rand() % 100) / 100.0f * gNoiseIntensity * 0.3f; // Much reduced
        glColor4f(intensity, intensity, intensity, intensity * 0.4f);
        glVertex2f(x, y);
    }
    
    // Very subtle fixed pattern noise
    srand(12345); // Fixed seed for consistent pattern
    int fixedNoisePoints = (screenWidth * screenHeight) / 15000; // Reduced
    for (int i = 0; i < fixedNoisePoints; i++) {
        float x = rand() % screenWidth;
        float y = rand() % screenHeight;
        
        float intensity = (rand() % 30) / 100.0f * gNoiseIntensity * 0.2f; // Much reduced
        glColor4f(intensity, intensity, intensity, intensity * 0.3f);
        glVertex2f(x, y);
    }
    
    // Very few dead pixels
    srand(54321);
    int deadPixels = screenWidth * screenHeight / 100000; // Much reduced
    for (int i = 0; i < deadPixels; i++) {
        float x = rand() % screenWidth;
        float y = rand() % screenHeight;
        
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f); // Less opaque
        glVertex2f(x, y);
    }
    
    glEnd();
    
    // Random scan line interference (less frequent)
    if ((gFrameCounter % 180) < 2) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
        glLineWidth(1.0f);
        
        glBegin(GL_LINES);
        srand(gFrameCounter);
        for (int i = 0; i < 3; i++) {
            float y = rand() % screenHeight;
            float alpha = 0.1f + (rand() % 20) / 100.0f;
            glColor4f(alpha, alpha, alpha, alpha);
            glVertex2f(0, y);
            glVertex2f(screenWidth, y);
        }
        glEnd();
    }
    
    // Occasional "hot pixel" clusters
    if ((gFrameCounter % 300) < 5) {
        srand(gFrameCounter / 10);
        int clusters = 2 + rand() % 4;
        for (int c = 0; c < clusters; c++) {
            float centerX = rand() % screenWidth;
            float centerY = rand() % screenHeight;
            
            glBegin(GL_POINTS);
            glPointSize(2.0f);
            for (int p = 0; p < 8; p++) {
                float x = centerX + (rand() % 6) - 3;
                float y = centerY + (rand() % 6) - 3;
                float intensity = 0.6f + (rand() % 40) / 100.0f;
                glColor4f(intensity, intensity, intensity, 0.7f);
                glVertex2f(x, y);
            }
            glEnd();
            glPointSize(1.0f);
        }
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
    
    glColor4f(0.2f, 0.2f, 0.2f, 1.0f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float aggressive_contrast = gContrast * 2.5f;
    if (aggressive_contrast > 1.0f) {
        glColor4f(0.0f, 0.0f, 0.0f, (aggressive_contrast - 1.0f) * 0.6f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(screenWidth, 0);
        glVertex2f(screenWidth, screenHeight);
        glVertex2f(0, screenHeight);
        glEnd();
    }
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.05f);
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

void CycleVisualModes()
{
    static int mode = 0;
    mode = (mode + 1) % 5;
    
    if (gUseShaders) {
        UseShaderMode(mode);
        switch (mode) {
            case 0:
                SetContrastCurve(1.0f, 1.0f, 0.0f);
                SetEdgeEnhancement(0, 0.8f);
                break;
            case 1:
                SetContrastCurve(1.8f, 1.3f, 0.05f);
                SetEdgeEnhancement(1, 0.6f);
                break;
            case 2:
                SetContrastCurve(2.0f, 1.4f, 0.1f);
                SetEdgeEnhancement(0, 0.8f);
                break;
            case 3:
                SetContrastCurve(0.4f, 3.5f, -0.2f);
                SetEdgeEnhancement(1, 1.0f);
                break;
            case 4:
                SetContrastCurve(0.3f, 4.0f, -0.3f);
                SetEdgeEnhancement(1, 0.9f);
                break;
        }
        gNoiseEnabled = (mode > 0) ? 1 : 0;
        gScanLinesEnabled = (mode == 1 || mode == 4) ? 1 : 0;
    } else {
        switch (mode) {
            case 0:
                SetMonochromeFilter(0);
                SetThermalMode(0);
                SetIRMode(0);
                SetEdgeEnhancement(0, 0.8f);
                SetContrastCurve(1.0f, 1.0f, 0.0f);
                gNoiseEnabled = 0;
                gScanLinesEnabled = 0;
                break;
                
            case 1:
                SetMonochromeFilter(1);
                SetThermalMode(0);
                SetIRMode(0);
                SetEdgeEnhancement(1, 0.6f);
                SetContrastCurve(1.8f, 1.3f, 0.05f);
                gNoiseEnabled = 1;
                gScanLinesEnabled = 1;
                break;
                
            case 2:
                SetMonochromeFilter(0);
                SetThermalMode(1);
                SetIRMode(0);
                SetEdgeEnhancement(0, 0.8f);
                SetContrastCurve(2.0f, 1.4f, 0.1f);
                gNoiseEnabled = 1;
                gScanLinesEnabled = 0;
                break;
                
            case 3:
                SetMonochromeFilter(0);
                SetThermalMode(0);
                SetIRMode(1);
                SetEdgeEnhancement(1, 1.0f);
                SetContrastCurve(2.5f, 2.0f, 0.15f);
                gNoiseEnabled = 1;
                gScanLinesEnabled = 0;
                break;
                
            case 4:
                SetMonochromeFilter(1);
                SetThermalMode(0);
                SetIRMode(1);
                SetEdgeEnhancement(1, 0.9f);
                SetContrastCurve(2.2f, 1.8f, 0.12f);
                gNoiseEnabled = 1;
                gScanLinesEnabled = 1;
                break;
        }
    }
}

void RenderContrastEnhancement(int screenWidth, int screenHeight)
{
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    
    float contrast_factor = powf(gContrast * gGain, 1.0f / gGamma);
    glColor4f(contrast_factor, contrast_factor, contrast_factor, 1.0f);
    
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(screenWidth, 0);
    glVertex2f(screenWidth, screenHeight);
    glVertex2f(0, screenHeight);
    glEnd();
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    if (gBias > 0.0f) {
        glColor4f(gBias, gBias, gBias, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(screenWidth, 0);
        glVertex2f(screenWidth, screenHeight);
        glVertex2f(0, screenHeight);
        glEnd();
    }
}

void RenderEdgeEnhancement(int screenWidth, int screenHeight)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(gEdgeIntensity, gEdgeIntensity, gEdgeIntensity, 0.3f);
    glLineWidth(1.0f);
    
    int step = 4;
    glBegin(GL_LINES);
    for (int x = step; x < screenWidth - step; x += step * 2) {
        for (int y = step; y < screenHeight - step; y += step * 2) {
            if ((x + y) % (step * 4) == 0) {
                glVertex2f(x - 1, y);
                glVertex2f(x + 1, y);
                glVertex2f(x, y - 1);
                glVertex2f(x, y + 1);
            }
        }
    }
    glEnd();
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderWithShader(int screenWidth, int screenHeight)
{
    if (!gUseShaders || !gShaderProgram) {
        return;
    }
    
    static float time = 0.0f;
    time += 0.016f;
    
    glUseProgram(gShaderProgram);
    
    int modeLocation = glGetUniformLocation(gShaderProgram, "mode");
    int contrastLocation = glGetUniformLocation(gShaderProgram, "contrast");
    int brightnessLocation = glGetUniformLocation(gShaderProgram, "brightness");
    int gammaLocation = glGetUniformLocation(gShaderProgram, "gamma");
    int gainLocation = glGetUniformLocation(gShaderProgram, "gain");
    int biasLocation = glGetUniformLocation(gShaderProgram, "bias");
    int edgeLocation = glGetUniformLocation(gShaderProgram, "edgeIntensity");
    int timeLocation = glGetUniformLocation(gShaderProgram, "time");
    int screenSizeLocation = glGetUniformLocation(gShaderProgram, "screenSize");
    int textureLocation = glGetUniformLocation(gShaderProgram, "screenTexture");
    
    glUniform1i(modeLocation, gShaderMode);
    glUniform1f(contrastLocation, gContrast);
    glUniform1f(brightnessLocation, gBrightness);
    glUniform1f(gammaLocation, gGamma);
    glUniform1f(gainLocation, gGain);
    glUniform1f(biasLocation, gBias);
    glUniform1f(edgeLocation, gEdgeIntensity);
    glUniform1f(timeLocation, time);
    glUniform2f(screenSizeLocation, (float)screenWidth, (float)screenHeight);
    glUniform1i(textureLocation, 0);
    
    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    
    glUseProgram(0);
}

void GetVisualEffectsStatus(char* statusBuffer, int bufferSize)
{
    const char* mode = "STANDARD";
    if (gUseShaders) {
        switch(gShaderMode) {
            case 0: mode = "STANDARD"; break;
            case 1: mode = "MONO_S"; break;
            case 2: mode = "THERMAL_S"; break;
            case 3: mode = "IR_S"; break;
            case 4: mode = "ENHANCED_IR_S"; break;
            default: mode = "UNKNOWN_S"; break;
        }
    } else {
        if (gMonochromeEnabled && gThermalEnabled) mode = "ENHANCED";
        else if (gThermalEnabled) mode = "THERMAL";
        else if (gMonochromeEnabled) mode = "MONO";
    }
    
    snprintf(statusBuffer, bufferSize, "VFX: %s", mode);
    statusBuffer[bufferSize - 1] = '\0';
}
