/*
 * Header file for military camera visual effects system
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

#ifndef FLIR_VISUALEFFECTS_H
#define FLIR_VISUALEFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif

void InitializeVisualEffects();
void CleanupVisualEffects();
void RenderVisualEffects(int screenWidth, int screenHeight);
void RenderPostProcessing(int screenWidth, int screenHeight);

void SetMonochromeFilter(int enabled);
void SetThermalMode(int enabled);
void SetIRMode(int enabled);
void SetImageEnhancement(float brightness, float contrast);
void RenderMonochromeFilter(int screenWidth, int screenHeight);
void RenderThermalEffects(int screenWidth, int screenHeight);
void RenderIRFilter(int screenWidth, int screenHeight);
void RenderCameraNoise(int screenWidth, int screenHeight);
void RenderScanLines(int screenWidth, int screenHeight);
void CycleVisualModes();

// Hybrid rendering functions
void RenderHybridEffects(int screenWidth, int screenHeight, int mode);
void RenderSmartMonochrome(int screenWidth, int screenHeight);
void RenderSmartThermal(int screenWidth, int screenHeight);
void RenderSmartIR(int screenWidth, int screenHeight);
void GetVisualEffectsStatus(char* statusBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif

#endif // FLIR_VISUALEFFECTS_H
