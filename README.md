![20250626121507_1](https://github.com/user-attachments/assets/8f6e6e33-3dd6-4355-b80e-4718c5836063)

FLIR Camera Plugin for X-Plane 12
===================================

Realistic military-style FLIR camera simulation with belly-mounted positioning,
optical zoom, thermal overlay effects, and targeting systems.

Features
--------
- Belly-mounted camera positioning under aircraft
- True optical zoom (1x-64x range)
- Pan/tilt controls via mouse or keyboard
- Target lock system with visual feedback
- Multiple visual modes: standard, monochrome, thermal, IR
- Military-style HUD overlay with telemetry
- Camera noise and scan line effects
- Real-time flight data display

Controls
--------
F9      - Toggle FLIR camera on/off
+/-     - Zoom in/out
Arrows  - Pan/tilt camera
Space   - Lock/unlock target
T       - Cycle visual modes
Mouse   - Pan/tilt when unlocked

Files
-----
FLIR_Camera.cpp         - Main plugin and camera control
FLIR_SimpleLock.cpp     - Target lock system
FLIR_VisualEffects.cpp  - Visual effects and filters
FLIR_HUD.lua            - HUD overlay (requires FlyWithLua)

Build
-----
make

Requirements
------------
- X-Plane 12
- FlyWithLua plugin (for HUD)
- OpenGL support

Installation
------------
1. Build plugin with make
2. Copy .xpl to X-Plane/Resources/plugins/
3. Install FLIR_HUD.lua in FlyWithLua scripts folder

License
-------
See license terms in source files.
