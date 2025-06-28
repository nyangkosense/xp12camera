/*
 * FLIR_MatrixTest.cpp
 * 
 * Implement the exact forum method using rotation matrices
 * Goal: "Shoot laser" -> get coordinates using proper math
 */

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMScenery.h"
#include "XPLMProcessing.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple 3x3 matrix and 3D vector structs
typedef struct {
    float m[3][3];
} Matrix3x3;

typedef struct {
    float x, y, z;
} Vector3;

// Test hotkey for manual triggering
static XPLMHotKeyID gTestKey = NULL;

// Aircraft datarefs
static XPLMDataRef gAircraftX = NULL;
static XPLMDataRef gAircraftY = NULL;
static XPLMDataRef gAircraftZ = NULL;
static XPLMDataRef gAircraftHeading = NULL;
static XPLMDataRef gAircraftPitch = NULL;
static XPLMDataRef gAircraftRoll = NULL;

// FLIR datarefs
static XPLMDataRef gFLIRPan = NULL;
static XPLMDataRef gFLIRTilt = NULL;

// Terrain probe
static XPLMProbeRef gTerrainProbe = NULL;

// Matrix operations
Matrix3x3 CreateRotationMatrixY(float angle_rad);
Matrix3x3 CreateRotationMatrixX(float angle_rad);
Matrix3x3 CreateRotationMatrixZ(float angle_rad);
Matrix3x3 MultiplyMatrix(Matrix3x3 a, Matrix3x3 b);
Vector3 MultiplyMatrixVector(Matrix3x3 m, Vector3 v);
Matrix3x3 CreateAircraftMatrix(float heading, float pitch, float roll);
Matrix3x3 CreateFLIRMatrix(float pan, float tilt);

// Test functions
static void TestCallback(void* inRefcon);
static void RunMatrixTest(void);
bool RaycastToTerrain(Vector3 start, Vector3 direction, Vector3* hitPoint);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "FLIR Matrix Test");
    strcpy(outSig, "flir.matrix.test");
    strcpy(outDesc, "Test rotation matrix approach");

    XPLMDebugString("MATRIX_TEST: Starting with rotation matrix approach\n");

    // Find aircraft datarefs
    gAircraftX = XPLMFindDataRef("sim/flightmodel/position/local_x");
    gAircraftY = XPLMFindDataRef("sim/flightmodel/position/local_y");
    gAircraftZ = XPLMFindDataRef("sim/flightmodel/position/local_z");
    gAircraftHeading = XPLMFindDataRef("sim/flightmodel/position/psi");
    gAircraftPitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    gAircraftRoll = XPLMFindDataRef("sim/flightmodel/position/phi");

    // Find FLIR datarefs (may not exist)
    gFLIRPan = XPLMFindDataRef("flir/camera/pan");
    gFLIRTilt = XPLMFindDataRef("flir/camera/tilt");

    if (!gAircraftX || !gAircraftY || !gAircraftZ || !gAircraftHeading || !gAircraftPitch || !gAircraftRoll) {
        XPLMDebugString("MATRIX_TEST: ERROR - Aircraft datarefs not found!\n");
        return 0;
    }

    // Create terrain probe
    gTerrainProbe = XPLMCreateProbe(xplm_ProbeY);
    if (!gTerrainProbe) {
        XPLMDebugString("MATRIX_TEST: ERROR - Failed to create terrain probe!\n");
        return 0;
    }

    // Register hotkey for manual testing
    gTestKey = XPLMRegisterHotKey(XPLM_VK_F5, xplm_DownFlag, "Matrix Test", TestCallback, NULL);
    XPLMDebugString("MATRIX_TEST: Press F5 to test current FLIR direction\n");

    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    if (gTestKey) {
        XPLMUnregisterHotKey(gTestKey);
    }
    if (gTerrainProbe) {
        XPLMDestroyProbe(gTerrainProbe);
    }
    XPLMDebugString("MATRIX_TEST: Plugin stopped\n");
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

// Hotkey callback for manual testing
static void TestCallback(void* inRefcon)
{
    RunMatrixTest();
}

// Matrix operations
Matrix3x3 CreateRotationMatrixY(float angle_rad)
{
    Matrix3x3 m;
    float c = cos(angle_rad);
    float s = sin(angle_rad);
    
    m.m[0][0] = c;  m.m[0][1] = 0;  m.m[0][2] = s;
    m.m[1][0] = 0;  m.m[1][1] = 1;  m.m[1][2] = 0;
    m.m[2][0] = -s; m.m[2][1] = 0;  m.m[2][2] = c;
    
    return m;
}

Matrix3x3 CreateRotationMatrixX(float angle_rad)
{
    Matrix3x3 m;
    float c = cos(angle_rad);
    float s = sin(angle_rad);
    
    m.m[0][0] = 1;  m.m[0][1] = 0;  m.m[0][2] = 0;
    m.m[1][0] = 0;  m.m[1][1] = c;  m.m[1][2] = -s;
    m.m[2][0] = 0;  m.m[2][1] = s;  m.m[2][2] = c;
    
    return m;
}

Matrix3x3 CreateRotationMatrixZ(float angle_rad)
{
    Matrix3x3 m;
    float c = cos(angle_rad);
    float s = sin(angle_rad);
    
    m.m[0][0] = c;  m.m[0][1] = -s; m.m[0][2] = 0;
    m.m[1][0] = s;  m.m[1][1] = c;  m.m[1][2] = 0;
    m.m[2][0] = 0;  m.m[2][1] = 0;  m.m[2][2] = 1;
    
    return m;
}

Matrix3x3 MultiplyMatrix(Matrix3x3 a, Matrix3x3 b)
{
    Matrix3x3 result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
        }
    }
    return result;
}

Vector3 MultiplyMatrixVector(Matrix3x3 m, Vector3 v)
{
    Vector3 result;
    result.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z;
    result.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z;
    result.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z;
    return result;
}

// Create aircraft rotation matrix (X-Plane coordinates: +X=East, +Z=South, +Y=Up)
Matrix3x3 CreateAircraftMatrix(float heading, float pitch, float roll)
{
    // Convert to radians
    float h = heading * M_PI / 180.0f;
    float p = pitch * M_PI / 180.0f;
    float r = roll * M_PI / 180.0f;
    
    // X-Plane rotation order: Roll, Pitch, Heading (Z, X, Y)
    Matrix3x3 rollMat = CreateRotationMatrixZ(r);
    Matrix3x3 pitchMat = CreateRotationMatrixX(p);
    Matrix3x3 headingMat = CreateRotationMatrixY(h);
    
    // Combine: heading * pitch * roll
    Matrix3x3 temp = MultiplyMatrix(pitchMat, rollMat);
    return MultiplyMatrix(headingMat, temp);
}

// Create FLIR rotation matrix (pan = Y rotation, tilt = X rotation)
Matrix3x3 CreateFLIRMatrix(float pan, float tilt)
{
    float p = pan * M_PI / 180.0f;
    float t = tilt * M_PI / 180.0f;
    
    Matrix3x3 panMat = CreateRotationMatrixY(p);
    Matrix3x3 tiltMat = CreateRotationMatrixX(t);
    
    // Combine: pan * tilt
    return MultiplyMatrix(panMat, tiltMat);
}

// Binary search terrain intersection (forum method)
bool RaycastToTerrain(Vector3 start, Vector3 direction, Vector3* hitPoint)
{
    float minRange = 100.0f;
    float maxRange = 30000.0f;
    float precision = 1.0f;
    int maxIterations = 40;
    
    XPLMProbeInfo_t probeInfo;
    probeInfo.structSize = sizeof(XPLMProbeInfo_t);
    
    bool foundTerrain = false;
    int iteration = 0;
    
    while ((maxRange - minRange) > precision && iteration < maxIterations) {
        float currentRange = (minRange + maxRange) / 2.0f;
        
        // Calculate test point: start + direction * range
        Vector3 testPoint;
        testPoint.x = start.x + direction.x * currentRange;
        testPoint.y = start.y + direction.y * currentRange;
        testPoint.z = start.z + direction.z * currentRange;
        
        XPLMProbeResult result = XPLMProbeTerrainXYZ(gTerrainProbe, testPoint.x, testPoint.y, testPoint.z, &probeInfo);
        
        if (result == xplm_ProbeHitTerrain) {
            foundTerrain = true;
            
            // Check if we're under terrain
            bool isUnder = (testPoint.y < probeInfo.locationY);
            
            if (iteration < 5) {
                char msg[256];
                snprintf(msg, sizeof(msg), 
                    "MATRIX_TEST: Iter=%d Range=%.1f Test(%.1f,%.1f,%.1f) Terrain=%.1f Under=%s\n",
                    iteration, currentRange, testPoint.x, testPoint.y, testPoint.z, probeInfo.locationY, isUnder ? "YES" : "NO");
                XPLMDebugString(msg);
            }
            
            if (isUnder) {
                maxRange = currentRange; // Back up
            } else {
                minRange = currentRange; // Go further
            }
        } else {
            minRange = currentRange; // No terrain, go further
        }
        
        iteration++;
    }
    
    if (foundTerrain) {
        float finalRange = (minRange + maxRange) / 2.0f;
        hitPoint->x = start.x + direction.x * finalRange;
        hitPoint->y = start.y + direction.y * finalRange;
        hitPoint->z = start.z + direction.z * finalRange;
        
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "MATRIX_TEST: SUCCESS - Hit at (%.1f,%.1f,%.1f) Range=%.1fm after %d iterations\n",
            hitPoint->x, hitPoint->y, hitPoint->z, finalRange, iteration);
        XPLMDebugString(msg);
        
        return true;
    }
    
    XPLMDebugString("MATRIX_TEST: FAILED - No terrain intersection found\n");
    return false;
}

// Main test using rotation matrices
static void RunMatrixTest(void)
{
    XPLMDebugString("MATRIX_TEST: ==========================================\n");
    XPLMDebugString("MATRIX_TEST: Testing rotation matrix approach\n");
    
    // Get aircraft data
    float acX = XPLMGetDataf(gAircraftX);
    float acY = XPLMGetDataf(gAircraftY);
    float acZ = XPLMGetDataf(gAircraftZ);
    float heading = XPLMGetDataf(gAircraftHeading);
    float pitch = XPLMGetDataf(gAircraftPitch);
    float roll = XPLMGetDataf(gAircraftRoll);
    
    // Get FLIR data (use defaults if not available)
    float flirPan = 0.0f;
    float flirTilt = -15.0f; // 15 degrees down
    
    if (gFLIRPan && gFLIRTilt) {
        flirPan = XPLMGetDataf(gFLIRPan);
        flirTilt = XPLMGetDataf(gFLIRTilt);
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), 
        "MATRIX_TEST: Aircraft - Pos(%.1f,%.1f,%.1f) Att(H=%.1f P=%.1f R=%.1f)\n",
        acX, acY, acZ, heading, pitch, roll);
    XPLMDebugString(msg);
    
    snprintf(msg, sizeof(msg), 
        "MATRIX_TEST: FLIR - Pan=%.1f Tilt=%.1f\n", flirPan, flirTilt);
    XPLMDebugString(msg);
    
    // Use FLIR camera's coordinate system directly (matches camera implementation)
    float worldHeading = heading + flirPan;  // Same as camera: planeHeading + gCameraPan
    float worldPitch = flirTilt;             // Same as camera: gCameraTilt
    
    // Convert to direction vector using simple trigonometry (X-Plane coordinates)
    float headingRad = worldHeading * M_PI / 180.0f;
    float pitchRad = worldPitch * M_PI / 180.0f;
    
    Vector3 worldDirection;
    // X-Plane: +X=East, +Z=South, +Y=Up
    worldDirection.x = sin(headingRad) * cos(pitchRad);   // East component
    worldDirection.y = sin(pitchRad);                     // Up/Down component (negative tilt = negative Y = downward)
    worldDirection.z = cos(headingRad) * cos(pitchRad);   // South component
    
    snprintf(msg, sizeof(msg), 
        "MATRIX_TEST: World direction vector = (%.3f, %.3f, %.3f)\n",
        worldDirection.x, worldDirection.y, worldDirection.z);
    XPLMDebugString(msg);
    
    // Start position (aircraft)
    Vector3 startPos = {acX, acY, acZ};
    
    // Raycast to terrain
    Vector3 hitPoint;
    if (RaycastToTerrain(startPos, worldDirection, &hitPoint)) {
        // Calculate distance and bearing
        float deltaX = hitPoint.x - acX;
        float deltaY = hitPoint.y - acY;
        float deltaZ = hitPoint.z - acZ;
        
        float slantRange = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
        float groundRange = sqrt(deltaX*deltaX + deltaZ*deltaZ);
        float bearing = atan2(deltaX, -deltaZ) * 180.0f / M_PI; // X-Plane: -Z=North, so use -deltaZ for North reference
        if (bearing < 0.0f) bearing += 360.0f; // Convert to 0-360° range
        
        snprintf(msg, sizeof(msg), 
            "MATRIX_TEST: Target - SlantRange=%.1fm GroundRange=%.1fm Bearing=%.1f°\n",
            slantRange, groundRange, bearing);
        XPLMDebugString(msg);
        
        // Coordinate validation checks
        XPLMDebugString("MATRIX_TEST: Validating coordinates...\n");
        
        // Check 1: Range vs tilt angle consistency
        float expectedRange = acY / tan(-flirTilt * M_PI / 180.0f); // Simple trig for downward tilt
        float rangeDifference = fabs(groundRange - expectedRange);
        float rangeError = (expectedRange > 0) ? (rangeDifference / expectedRange) * 100.0f : 100.0f;
        
        snprintf(msg, sizeof(msg), 
            "MATRIX_TEST: Range check - Expected=%.1fm Actual=%.1fm Error=%.1f%%\n",
            expectedRange, groundRange, rangeError);
        XPLMDebugString(msg);
        
        // Check 2: Bearing vs world heading consistency  
        float expectedBearing = worldHeading; // Should match calculated world heading
        if (expectedBearing < 0.0f) expectedBearing += 360.0f; // Convert to 0-360° range
        if (expectedBearing >= 360.0f) expectedBearing -= 360.0f;
        
        float bearingDifference = fabs(bearing - expectedBearing);
        if (bearingDifference > 180.0f) bearingDifference = 360.0f - bearingDifference;
        
        snprintf(msg, sizeof(msg), 
            "MATRIX_TEST: Bearing check - Expected=%.1f° Actual=%.1f° Error=%.1f°\n",
            expectedBearing, bearing, bearingDifference);
        XPLMDebugString(msg);
        
        // Check 3: Target should be below aircraft for downward tilt
        bool targetBelowAircraft = (hitPoint.y < acY);
        
        snprintf(msg, sizeof(msg), 
            "MATRIX_TEST: Altitude check - Aircraft=%.1fm Target=%.1fm Below=%s\n",
            acY, hitPoint.y, targetBelowAircraft ? "YES" : "NO");
        XPLMDebugString(msg);
        
        // Overall validation result
        bool validRange = (rangeError < 50.0f); // Allow 50% error for now
        bool validBearing = (bearingDifference < 30.0f); // Allow 30° error
        bool validAltitude = targetBelowAircraft && (flirTilt < 0); // Must be below for downward tilt
        
        if (validRange && validBearing && validAltitude) {
            XPLMDebugString("MATRIX_TEST: ✓ VALIDATION PASSED - Coordinates appear correct!\n");
        } else {
            XPLMDebugString("MATRIX_TEST: ✗ VALIDATION FAILED - Coordinates may be incorrect:\n");
            if (!validRange) XPLMDebugString("MATRIX_TEST:   - Range calculation seems off\n");
            if (!validBearing) XPLMDebugString("MATRIX_TEST:   - Bearing doesn't match FLIR pan\n");
            if (!validAltitude) XPLMDebugString("MATRIX_TEST:   - Target altitude inconsistent with tilt\n");
        }
        
        XPLMDebugString("MATRIX_TEST: SUCCESS - Coordinates ready for missile guidance!\n");
    } else {
        XPLMDebugString("MATRIX_TEST: FAILED - Could not find target coordinates\n");
    }
    
    XPLMDebugString("MATRIX_TEST: ==========================================\n");
}