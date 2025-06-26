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

void InitializeVisualEffects();
void RenderVisualEffects(int screenWidth, int screenHeight);

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
void GetVisualEffectsStatus(char* statusBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif

#endif // FLIR_VISUALEFFECTS_H