/*
 * FLIR_TerrainFinder.h
 * 
 * Dedicated module for finding terrain coordinates from FLIR crosshair
 * Implements multiple algorithms:
 * - Ray casting with binary search
 * - Terrain probing
 * - Water surface detection
 * - Local coordinate conversion
 * 
 * For Maritime Patrol Aircraft targeting systems
 */

#ifndef FLIR_TERRAIN_FINDER_H
#define FLIR_TERRAIN_FINDER_H

#include "XPLMScenery.h"

// Result structure for terrain findings
typedef struct {
    bool found;                    // True if target was found
    float localX, localY, localZ;  // Local coordinates (OpenGL)
    float range;                   // Distance from start point
    float terrainHeight;           // Terrain elevation at hit point
    bool isWater;                  // True if hit point is water surface
    int iterations;                // Number of search iterations used
    char method[32];               // Which method found the target
} TerrainFindResult;

// Search parameters
typedef struct {
    float minRange;        // Minimum search distance (meters)
    float maxRange;        // Maximum search distance (meters)  
    float precision;       // Search precision (meters)
    int maxIterations;     // Maximum iterations before giving up
    bool waterOnly;        // Only find water surfaces
    bool debugOutput;      // Enable debug logging
} TerrainSearchParams;

// Initialize terrain finder system
bool InitializeTerrainFinder(void);

// Cleanup terrain finder system
void CleanupTerrainFinder(void);

// Find terrain using ray casting with binary search (proven method)
bool FindTerrainByRaycast(float startX, float startY, float startZ, 
                         float dirX, float dirY, float dirZ,
                         const TerrainSearchParams* params,
                         TerrainFindResult* result);

// Find terrain using linear search (faster but less precise)
bool FindTerrainByLinearSearch(float startX, float startY, float startZ,
                              float dirX, float dirY, float dirZ,
                              const TerrainSearchParams* params,
                              TerrainFindResult* result);

// Find terrain using adaptive search (automatically adjusts method)
bool FindTerrainAdaptive(float startX, float startY, float startZ,
                        float dirX, float dirY, float dirZ,
                        const TerrainSearchParams* params,
                        TerrainFindResult* result);

// Convenience function: Find target from aircraft FLIR crosshair
bool FindTargetFromFLIR(float aircraftX, float aircraftY, float aircraftZ,
                       float flirPan, float flirTilt, float aircraftHeading,
                       const TerrainSearchParams* params,
                       TerrainFindResult* result);

// Utility functions
bool IsWaterSurface(float terrainHeight, float probeY);
void LogTerrainResult(const TerrainFindResult* result, const char* context);
TerrainSearchParams GetDefaultSearchParams(void);
TerrainSearchParams GetMaritimeSearchParams(void);

// Test functions for validation
bool TestTerrainFinder(void);
void BenchmarkTerrainMethods(void);

#endif // FLIR_TERRAIN_FINDER_H