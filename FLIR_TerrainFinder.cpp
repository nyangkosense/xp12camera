/*
 * FLIR_TerrainFinder.cpp
 * 
 * Dedicated terrain coordinate finding implementation
 * Multiple algorithms for robust target detection
 */

#include "FLIR_TerrainFinder.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global terrain probe
static XPLMProbeRef gTerrainProbe = NULL;
static bool gTerrainFinderInitialized = false;

// Statistics for performance monitoring
static struct {
    int totalSearches;
    int successfulFinds;
    int raycastUses;
    int linearUses;
    float avgIterations;
    float avgRange;
} gTerrainStats = {0};

// Initialize terrain finder system
bool InitializeTerrainFinder(void)
{
    if (gTerrainFinderInitialized) {
        return true;
    }
    
    // Create terrain probe for Y-axis (elevation) probing
    gTerrainProbe = XPLMCreateProbe(xplm_ProbeY);
    
    if (!gTerrainProbe) {
        XPLMDebugString("TERRAIN_FINDER: ERROR - Failed to create terrain probe!\n");
        return false;
    }
    
    // Reset statistics
    memset(&gTerrainStats, 0, sizeof(gTerrainStats));
    
    gTerrainFinderInitialized = true;
    XPLMDebugString("TERRAIN_FINDER: Initialized successfully\n");
    return true;
}

// Cleanup terrain finder system  
void CleanupTerrainFinder(void)
{
    if (gTerrainProbe) {
        XPLMDestroyProbe(gTerrainProbe);
        gTerrainProbe = NULL;
    }
    
    // Log final statistics
    if (gTerrainStats.totalSearches > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "TERRAIN_FINDER: Final Stats - Searches:%d Success:%d AvgIter:%.1f AvgRange:%.0fm\n",
            gTerrainStats.totalSearches, gTerrainStats.successfulFinds, 
            gTerrainStats.avgIterations, gTerrainStats.avgRange);
        XPLMDebugString(msg);
    }
    
    gTerrainFinderInitialized = false;
    XPLMDebugString("TERRAIN_FINDER: Cleaned up\n");
}

// Check if a point represents water surface
bool IsWaterSurface(float terrainHeight, float probeY)
{
    // Water detection heuristics:
    // 1. Terrain height near sea level (within 10m of 0)
    // 2. Probe point is close to terrain height (within 5m)
    
    bool nearSeaLevel = (terrainHeight >= -10.0f && terrainHeight <= 10.0f);
    bool closeToTerrain = (fabs(probeY - terrainHeight) <= 5.0f);
    
    return nearSeaLevel && closeToTerrain;
}

// Get default search parameters
TerrainSearchParams GetDefaultSearchParams(void)
{
    TerrainSearchParams params;
    params.minRange = 100.0f;      // 100m minimum
    params.maxRange = 10000.0f;    // 10km maximum  
    params.precision = 2.0f;       // 2m precision
    params.maxIterations = 40;     // Max 40 iterations
    params.waterOnly = false;      // Find any terrain
    params.debugOutput = true;     // Enable debug logging
    return params;
}

// Get maritime patrol optimized parameters
TerrainSearchParams GetMaritimeSearchParams(void)
{
    TerrainSearchParams params;
    params.minRange = 500.0f;      // 500m minimum (ship detection range)
    params.maxRange = 30000.0f;    // 30km maximum (maritime patrol range)
    params.precision = 5.0f;       // 5m precision (adequate for ships)
    params.maxIterations = 50;     // More iterations for long range
    params.waterOnly = true;       // Water surface only
    params.debugOutput = true;     // Enable debug logging
    return params;
}

// Ray casting with binary search (proven method from forum)
bool FindTerrainByRaycast(float startX, float startY, float startZ, 
                         float dirX, float dirY, float dirZ,
                         const TerrainSearchParams* params,
                         TerrainFindResult* result)
{
    if (!gTerrainProbe) {
        XPLMDebugString("TERRAIN_FINDER: ERROR - Not initialized\n");
        return false;
    }
    
    // Initialize result
    memset(result, 0, sizeof(TerrainFindResult));
    strcpy(result->method, "raycast");
    
    gTerrainStats.totalSearches++;
    gTerrainStats.raycastUses++;
    
    if (params->debugOutput) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "TERRAIN_FINDER: Raycast Start(%.1f,%.1f,%.1f) Dir(%.3f,%.3f,%.3f) Range(%.0f-%.0f)\n",
            startX, startY, startZ, dirX, dirY, dirZ, params->minRange, params->maxRange);
        XPLMDebugString(msg);
    }
    
    XPLMProbeInfo_t probeInfo;
    probeInfo.structSize = sizeof(XPLMProbeInfo_t);
    
    float minRange = params->minRange;
    float maxRange = params->maxRange;
    int iteration = 0;
    bool foundValidTerrain = false;
    
    // Binary search for terrain intersection
    while ((maxRange - minRange) > params->precision && iteration < params->maxIterations) {
        float currentRange = (minRange + maxRange) / 2.0f;
        
        // Calculate test point along ray
        float testX = startX + dirX * currentRange;
        float testY = startY + dirY * currentRange;
        float testZ = startZ + dirZ * currentRange;
        
        // Probe terrain at this point
        XPLMProbeResult probeResult = XPLMProbeTerrainXYZ(gTerrainProbe, testX, testY, testZ, &probeInfo);
        
        bool isUnderTerrain = (testY < probeInfo.locationY);
        bool isWater = IsWaterSurface(probeInfo.locationY, testY);
        
        if (params->debugOutput && (iteration < 3 || iteration % 10 == 0)) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "TERRAIN_FINDER: Iter=%d Range=%.1f Test(%.1f,%.1f,%.1f) Terrain=%.1f Under=%s Water=%s\n",
                iteration, currentRange, testX, testY, testZ, probeInfo.locationY, 
                isUnderTerrain ? "YES" : "NO", isWater ? "YES" : "NO");
            XPLMDebugString(msg);
        }
        
        if (probeResult == xplm_ProbeHitTerrain) {
            foundValidTerrain = true;
            
            // Check if this meets our search criteria
            if (!params->waterOnly || isWater) {
                if (isUnderTerrain) {
                    // We're under terrain, back up
                    maxRange = currentRange;
                } else {
                    // We're above terrain, go further
                    minRange = currentRange;
                }
            } else {
                // Not water but we need water, continue searching
                minRange = currentRange;
            }
        } else {
            // No terrain hit, go further
            minRange = currentRange;
        }
        
        iteration++;
    }
    
    result->iterations = iteration;
    gTerrainStats.avgIterations = (gTerrainStats.avgIterations * (gTerrainStats.totalSearches - 1) + iteration) / gTerrainStats.totalSearches;
    
    if (foundValidTerrain) {
        float finalRange = (minRange + maxRange) / 2.0f;
        
        // Calculate final intersection point
        result->localX = startX + dirX * finalRange;
        result->localY = startY + dirY * finalRange;
        result->localZ = startZ + dirZ * finalRange;
        result->range = finalRange;
        result->found = true;
        
        // Get final terrain info
        XPLMProbeTerrainXYZ(gTerrainProbe, result->localX, result->localY, result->localZ, &probeInfo);
        result->terrainHeight = probeInfo.locationY;
        result->isWater = IsWaterSurface(probeInfo.locationY, result->localY);
        
        gTerrainStats.successfulFinds++;
        gTerrainStats.avgRange = (gTerrainStats.avgRange * (gTerrainStats.successfulFinds - 1) + finalRange) / gTerrainStats.successfulFinds;
        
        if (params->debugOutput) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "TERRAIN_FINDER: SUCCESS after %d iterations - Target(%.1f,%.1f,%.1f) Range=%.1fm Water=%s\n",
                iteration, result->localX, result->localY, result->localZ, finalRange, result->isWater ? "YES" : "NO");
            XPLMDebugString(msg);
        }
        
        return true;
    } else {
        if (params->debugOutput) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "TERRAIN_FINDER: FAILED after %d iterations - No valid terrain found\n", iteration);
            XPLMDebugString(msg);
        }
        
        return false;
    }
}

// Linear search method (faster but less precise)
bool FindTerrainByLinearSearch(float startX, float startY, float startZ,
                              float dirX, float dirY, float dirZ,
                              const TerrainSearchParams* params,
                              TerrainFindResult* result)
{
    if (!gTerrainProbe) {
        return false;
    }
    
    memset(result, 0, sizeof(TerrainFindResult));
    strcpy(result->method, "linear");
    
    gTerrainStats.totalSearches++;
    gTerrainStats.linearUses++;
    
    XPLMProbeInfo_t probeInfo;
    probeInfo.structSize = sizeof(XPLMProbeInfo_t);
    
    float stepSize = params->precision * 2.0f; // Larger steps for speed
    int iteration = 0;
    
    for (float range = params->minRange; range <= params->maxRange; range += stepSize) {
        float testX = startX + dirX * range;
        float testY = startY + dirY * range;
        float testZ = startZ + dirZ * range;
        
        XPLMProbeResult probeResult = XPLMProbeTerrainXYZ(gTerrainProbe, testX, testY, testZ, &probeInfo);
        iteration++;
        
        if (probeResult == xplm_ProbeHitTerrain) {
            bool isUnderTerrain = (testY <= probeInfo.locationY + 1.0f); // 1m tolerance
            bool isWater = IsWaterSurface(probeInfo.locationY, testY);
            
            if (isUnderTerrain && (!params->waterOnly || isWater)) {
                result->localX = testX;
                result->localY = probeInfo.locationY; // Use terrain height
                result->localZ = testZ;
                result->range = range;
                result->terrainHeight = probeInfo.locationY;
                result->isWater = isWater;
                result->iterations = iteration;
                result->found = true;
                
                gTerrainStats.successfulFinds++;
                
                if (params->debugOutput) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), 
                        "TERRAIN_FINDER: Linear SUCCESS - Target(%.1f,%.1f,%.1f) Range=%.1fm Water=%s\n",
                        result->localX, result->localY, result->localZ, range, result->isWater ? "YES" : "NO");
                    XPLMDebugString(msg);
                }
                
                return true;
            }
        }
        
        if (iteration >= params->maxIterations) {
            break;
        }
    }
    
    result->iterations = iteration;
    
    if (params->debugOutput) {
        XPLMDebugString("TERRAIN_FINDER: Linear search FAILED\n");
    }
    
    return false;
}

// Adaptive search (automatically chooses best method)
bool FindTerrainAdaptive(float startX, float startY, float startZ,
                        float dirX, float dirY, float dirZ,
                        const TerrainSearchParams* params,
                        TerrainFindResult* result)
{
    // Decision logic:
    // - Short range (< 5km): Use linear search (faster)
    // - Long range (>= 5km): Use raycast (more precise)
    // - High precision required (< 1m): Always use raycast
    
    bool usePreciseMethod = (params->maxRange >= 5000.0f) || (params->precision < 1.0f);
    
    if (usePreciseMethod) {
        return FindTerrainByRaycast(startX, startY, startZ, dirX, dirY, dirZ, params, result);
    } else {
        return FindTerrainByLinearSearch(startX, startY, startZ, dirX, dirY, dirZ, params, result);
    }
}

// Convert FLIR angles to world direction vector  
static void CalculateFLIRDirection(float flirPan, float flirTilt, float aircraftHeading, 
                                  float* outDirX, float* outDirY, float* outDirZ)
{
    // Convert angles to radians
    float panRad = flirPan * M_PI / 180.0f;
    float tiltRad = flirTilt * M_PI / 180.0f;
    float headingRad = aircraftHeading * M_PI / 180.0f;
    
    // FLIR direction in aircraft local coordinates (Z forward, Y up, X right)
    float localDirX = 0.0f;
    float localDirY = 0.0f; 
    float localDirZ = 1.0f; // Forward
    
    // Apply FLIR pan (rotation around Y axis)
    float dirX = localDirX * cos(panRad) + localDirZ * sin(panRad);
    float dirY = localDirY;
    float dirZ = -localDirX * sin(panRad) + localDirZ * cos(panRad);
    
    // Apply FLIR tilt (rotation around X axis)
    float finalDirX = dirX;
    float finalDirY = dirY * cos(-tiltRad) - dirZ * sin(-tiltRad);
    float finalDirZ = dirY * sin(-tiltRad) + dirZ * cos(-tiltRad);
    
    // Transform by aircraft heading (rotation around Y axis)
    *outDirX = finalDirX * cos(headingRad) - finalDirZ * sin(headingRad);
    *outDirY = finalDirY;
    *outDirZ = finalDirX * sin(headingRad) + finalDirZ * cos(headingRad);
    
    // Normalize
    float magnitude = sqrt((*outDirX)*(*outDirX) + (*outDirY)*(*outDirY) + (*outDirZ)*(*outDirZ));
    if (magnitude > 0.001f) {
        *outDirX /= magnitude;
        *outDirY /= magnitude;
        *outDirZ /= magnitude;
    }
}

// Convenience function: Find target from FLIR crosshair
bool FindTargetFromFLIR(float aircraftX, float aircraftY, float aircraftZ,
                       float flirPan, float flirTilt, float aircraftHeading,
                       const TerrainSearchParams* params,
                       TerrainFindResult* result)
{
    float dirX, dirY, dirZ;
    CalculateFLIRDirection(flirPan, flirTilt, aircraftHeading, &dirX, &dirY, &dirZ);
    
    if (params->debugOutput) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "TERRAIN_FINDER: FLIR Pan=%.1f° Tilt=%.1f° Heading=%.1f° -> Dir(%.3f,%.3f,%.3f)\n",
            flirPan, flirTilt, aircraftHeading, dirX, dirY, dirZ);
        XPLMDebugString(msg);
    }
    
    return FindTerrainAdaptive(aircraftX, aircraftY, aircraftZ, dirX, dirY, dirZ, params, result);
}

// Log terrain result with context
void LogTerrainResult(const TerrainFindResult* result, const char* context)
{
    if (result->found) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "TERRAIN_FINDER: %s - Found target at (%.1f,%.1f,%.1f) Range=%.1fm Water=%s Method=%s Iters=%d\n",
            context, result->localX, result->localY, result->localZ, result->range, 
            result->isWater ? "YES" : "NO", result->method, result->iterations);
        XPLMDebugString(msg);
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg), "TERRAIN_FINDER: %s - No target found\n", context);
        XPLMDebugString(msg);
    }
}

// Test terrain finder functionality
bool TestTerrainFinder(void)
{
    XPLMDebugString("TERRAIN_FINDER: Running self-tests...\n");
    
    // Test with known coordinates (should find terrain)
    TerrainSearchParams params = GetDefaultSearchParams();
    TerrainFindResult result;
    
    // Test straight down from 1000m altitude
    bool success = FindTerrainByRaycast(0.0f, 1000.0f, 0.0f, 0.0f, -1.0f, 0.0f, &params, &result);
    
    if (success) {
        XPLMDebugString("TERRAIN_FINDER: Self-test PASSED\n");
        LogTerrainResult(&result, "Self-test");
        return true;
    } else {
        XPLMDebugString("TERRAIN_FINDER: Self-test FAILED\n");
        return false;
    }
}

// Benchmark different terrain finding methods
void BenchmarkTerrainMethods(void)
{
    XPLMDebugString("TERRAIN_FINDER: Starting benchmark...\n");
    
    TerrainSearchParams params = GetDefaultSearchParams();
    params.debugOutput = false; // Reduce log spam
    
    TerrainFindResult result;
    int raycastSuccesses = 0;
    int linearSuccesses = 0;
    
    // Test multiple directions
    for (int i = 0; i < 8; i++) {
        float angle = i * M_PI / 4.0f; // 45° increments
        float dirX = sin(angle);
        float dirY = -0.1f; // Slight downward angle
        float dirZ = cos(angle);
        
        // Test raycast method
        if (FindTerrainByRaycast(0.0f, 500.0f, 0.0f, dirX, dirY, dirZ, &params, &result)) {
            raycastSuccesses++;
        }
        
        // Test linear method
        if (FindTerrainByLinearSearch(0.0f, 500.0f, 0.0f, dirX, dirY, dirZ, &params, &result)) {
            linearSuccesses++;
        }
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), 
        "TERRAIN_FINDER: Benchmark complete - Raycast: %d/8 Linear: %d/8\n",
        raycastSuccesses, linearSuccesses);
    XPLMDebugString(msg);
}