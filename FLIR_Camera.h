/*
 * FLIR_Camera.h
 * 
 * Header file for accessing FLIR camera variables from other plugins
 */

#ifndef FLIR_CAMERA_H
#define FLIR_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

// External access to FLIR camera state variables
extern int gCameraActive;
extern float gCameraPan;
extern float gCameraTilt;

#ifdef __cplusplus
}
#endif

#endif // FLIR_CAMERA_H