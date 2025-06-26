/*
 * FLIR_VisualEffects.h
 * 
 * Header file for military camera visual effects system
 */

#ifndef FLIR_VISUALEFFECTS_H
#define FLIR_VISUALEFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the visual effects system
void InitializeVisualEffects();

// Main rendering function - call from drawing callback
void RenderVisualEffects(int screenWidth, int screenHeight);

// Individual effect controls
void SetMonochromeFilter(int enabled);
void SetThermalMode(int enabled);
void SetIRMode(int enabled);
void SetImageEnhancement(float brightness, float contrast);

// Individual effect renderers
void RenderMonochromeFilter(int screenWidth, int screenHeight);
void RenderThermalEffects(int screenWidth, int screenHeight);
void RenderIRFilter(int screenWidth, int screenHeight);
void RenderCameraNoise(int screenWidth, int screenHeight);
void RenderScanLines(int screenWidth, int screenHeight);

// Quick mode switching
void CycleVisualModes();

// Status display
void GetVisualEffectsStatus(char* statusBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif

#endif // FLIR_VISUALEFFECTS_H