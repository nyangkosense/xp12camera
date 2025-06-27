# Integrated FLIR Guidance Plugin - Build & Test Guide

## Overview
This plugin integrates your existing FLIR camera system with precision missile guidance using the proven velocity-based control method.

## Workflow
1. **F9** - Activate FLIR camera
2. **Position crosshair** on target using mouse/arrow keys
3. **SPACEBAR** - Lock target (calculates coordinates from camera angles)
4. **Fire weapons** (use your existing weapon firing method)
5. **F2** - Start precision missile guidance to FLIR target

## Build Instructions

### Prerequisites
- MinGW-w64 cross-compiler installed
- X-Plane 12 SDK in `SDK/` directory
- Your existing FLIR_SimpleLock.cpp file with DesignateTarget() function

### Build Command
```bash
make -f Makefile.integrated
```

### Test Compilation Only
```bash
make -f Makefile.integrated test-compile
```

### Clean Build Files
```bash
make -f Makefile.integrated clean
```

## Installation
1. Build the plugin: `make -f Makefile.integrated`
2. Copy `build/IntegratedGuidance/win_x64/IntegratedGuidance.xpl` to your X-Plane 12 `Resources/plugins/` directory
3. Start X-Plane 12

## Testing the Integration

### Step 1: Verify Plugin Loading
- Check X-Plane's Log.txt for: `INTEGRATED GUIDANCE: Plugin loaded`
- Look for hotkey registrations: F2, F3, F4

### Step 2: Test FLIR Target Lock
1. Load an aircraft with weapons
2. Press **F9** to activate FLIR camera
3. Position crosshair on a ground target
4. Press **SPACEBAR** to lock target
5. Look for debug message: `INTEGRATED GUIDANCE: Target locked at (X, Y, Z)`

### Step 3: Test Missile Guidance
1. After target lock, fire weapons
2. Press **F2** to start guidance
3. Look for debug message: `INTEGRATED GUIDANCE: STARTED → Target (X, Y, Z)`
4. Watch missiles guide to FLIR target

### Step 4: Monitor Guidance Status
- Debug logs appear every 3 seconds showing missile position, velocity, and distance to target
- Messages format: `INTEGRATED GUIDANCE: [missile_id] Pos:(x,y,z) Vel:(vx,vy,vz) Speed:X Dist:Y`

## Key Features

### Camera Integration
- Uses existing `gCameraPan` and `gCameraTilt` variables from FLIR_Camera.cpp
- Calculates target coordinates using same logic as your DesignateTarget() function
- Respects FLIR camera lock state

### Precision Guidance
- Velocity-based missile control using proven sim/weapons/vx/vy/vz datarefs
- Proportional control with distance-based speed adjustment
- Smooth damping to prevent oscillation
- Automatic final approach when close to target

### Safety Features
- Only guides weapons 0 and 1 (first two missiles)
- Range limiting: 50m minimum, 8000m maximum
- Velocity limiting to prevent extreme speeds
- Graceful handling of inactive missiles

## Hotkeys
- **F2**: Start/Stop missile guidance
- **F3**: Force stop guidance
- **F4**: Manual target lock (uses current FLIR direction)

## Troubleshooting

### Build Issues
- Ensure MinGW-w64 is installed: `x86_64-w64-mingw32-g++ --version`
- Check SDK path in Makefile.integrated
- Verify FLIR_SimpleLock.cpp exists and compiles

### Runtime Issues
- Check X-Plane Log.txt for error messages
- Ensure weapons are loaded on aircraft
- Verify FLIR camera is active (F9) before target lock
- Target must be locked (SPACEBAR) before guidance (F2)

### Guidance Not Working
- Check weapon count > 0
- Verify missiles are active (not at 0,0,0 position)
- Ensure target is within 50m-8000m range
- Look for velocity dataref access errors in log

## Debug Information
The plugin provides extensive debug output:
- Target calculation with pan/tilt angles and range
- Missile status every 3 seconds during guidance
- Error messages for missing datarefs or invalid states
- Guidance start/stop notifications

## Integration Notes
This plugin is designed to work alongside your existing FLIR_Camera.cpp plugin. It:
- Accesses camera angles via extern declarations
- Uses your existing DesignateTarget() coordinate calculation
- Integrates with IsTargetDesignated() function
- Maintains compatibility with existing FLIR workflow