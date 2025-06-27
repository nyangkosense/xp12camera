/*
 * FLIR_CameraShared.cpp
 * 
 * Shared camera variables for integration between FLIR plugins
 * This file contains only the shared variables, not the plugin entry points
 */

// Shared FLIR camera state variables
int gCameraActive = 0;
float gCameraPan = 0.0f;
float gCameraTilt = -15.0f;