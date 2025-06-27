/*
 * WeaponDebug.cpp
 * 
 * Debug weapon firing issues
 * Check weapon states and commands
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "XPLMDisplay.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// Weapon datarefs
static XPLMDataRef gWeaponCount = NULL;
static XPLMDataRef gWeaponType = NULL;
static XPLMDataRef gMasterArm = NULL;
static XPLMDataRef gMissilesArmed = NULL;
static XPLMDataRef gBombsArmed = NULL;
static XPLMDataRef gWeaponsArmed = NULL;
static XPLMDataRef gGunsArmed = NULL;

// Weapon commands
static XPLMCommandRef gFireAnyArmed = NULL;
static XPLMCommandRef gFireAirToGround = NULL;
static XPLMCommandRef gFireMissile = NULL;

static void DebugWeaponStatusCallback(void* inRefcon);
static void TestFireCommandCallback(void* inRefcon);

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    strcpy(outName, "Weapon Debug");
    strcpy(outSig, "weapon.debug");
    strcpy(outDesc, "Debug weapon firing issues");

    // Initialize weapon datarefs
    gWeaponCount = XPLMFindDataRef("sim/weapons/weapon_count");
    gWeaponType = XPLMFindDataRef("sim/weapons/type");
    gMasterArm = XPLMFindDataRef("sim/cockpit2/weapons/master_arm");
    gMissilesArmed = XPLMFindDataRef("sim/cockpit/weapons/missiles_armed");
    gBombsArmed = XPLMFindDataRef("sim/cockpit/weapons/bombs_armed");
    gWeaponsArmed = XPLMFindDataRef("sim/cockpit/weapons/rockets_armed");
    gGunsArmed = XPLMFindDataRef("sim/cockpit/weapons/guns_armed");
    
    // Initialize weapon commands
    gFireAnyArmed = XPLMFindCommand("sim/weapons/fire_any_armed");
    gFireAirToGround = XPLMFindCommand("sim/weapons/fire_air_to_ground");
    gFireMissile = XPLMFindCommand("sim/weapons/fire_missile");

    // Register debug hotkeys
    XPLMRegisterHotKey(XPLM_VK_F8, xplm_DownFlag, "Debug: Weapon Status", DebugWeaponStatusCallback, NULL);
    XPLMRegisterHotKey(XPLM_VK_F9, xplm_DownFlag, "Debug: Test Fire Command", TestFireCommandCallback, NULL);

    XPLMDebugString("WEAPON DEBUG: Plugin loaded\n");
    XPLMDebugString("WEAPON DEBUG: F8=Check weapon status, F9=Test fire command\n");

    return 1;
}

PLUGIN_API void XPluginStop(void) { }
PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam) { }

static void DebugWeaponStatusCallback(void* inRefcon)
{
    char msg[1024];
    
    int weaponCount = gWeaponCount ? XPLMGetDatai(gWeaponCount) : -1;
    int masterArm = gMasterArm ? XPLMGetDatai(gMasterArm) : -1;
    int missilesArmed = gMissilesArmed ? XPLMGetDatai(gMissilesArmed) : -1;
    int bombsArmed = gBombsArmed ? XPLMGetDatai(gBombsArmed) : -1;
    int weaponsArmed = gWeaponsArmed ? XPLMGetDatai(gWeaponsArmed) : -1;
    int gunsArmed = gGunsArmed ? XPLMGetDatai(gGunsArmed) : -1;
    
    snprintf(msg, sizeof(msg), 
        "WEAPON DEBUG: COMPLETE WEAPON STATUS\n"
        "WEAPON DEBUG: Weapon count: %d\n"
        "WEAPON DEBUG: Master arm: %d (%s)\n"
        "WEAPON DEBUG: Missiles armed: %d (%s)\n"
        "WEAPON DEBUG: Bombs armed: %d (%s)\n"
        "WEAPON DEBUG: Rockets armed: %d (%s)\n"
        "WEAPON DEBUG: Guns armed: %d (%s)\n",
        weaponCount,
        masterArm, (masterArm == 1 ? "ARMED" : masterArm == 0 ? "SAFE" : "UNKNOWN"),
        missilesArmed, (missilesArmed == 1 ? "ARMED" : missilesArmed == 0 ? "SAFE" : "UNKNOWN"),
        bombsArmed, (bombsArmed == 1 ? "ARMED" : bombsArmed == 0 ? "SAFE" : "UNKNOWN"),
        weaponsArmed, (weaponsArmed == 1 ? "ARMED" : weaponsArmed == 0 ? "SAFE" : "UNKNOWN"),
        gunsArmed, (gunsArmed == 1 ? "ARMED" : gunsArmed == 0 ? "SAFE" : "UNKNOWN"));
    
    XPLMDebugString(msg);
    
    // Check weapon types
    if (gWeaponType && weaponCount > 0) {
        int weaponTypes[25] = {0};
        XPLMGetDatavi(gWeaponType, weaponTypes, 0, weaponCount);
        
        for (int i = 0; i < weaponCount && i < 25; i++) {
            char typeMsg[128];
            const char* typeName = "Unknown";
            switch (weaponTypes[i]) {
                case 0: typeName = "None"; break;
                case 1: typeName = "Gun"; break;
                case 2: typeName = "Rocket"; break;
                case 3: typeName = "Missile"; break;
                case 4: typeName = "Bomb"; break;
                case 5: typeName = "Flare"; break;
                case 6: typeName = "Chaff"; break;
            }
            snprintf(typeMsg, sizeof(typeMsg), "WEAPON DEBUG: Weapon[%d]: Type %d (%s)\n", i, weaponTypes[i], typeName);
            XPLMDebugString(typeMsg);
        }
    }
    
    // Check command availability
    XPLMDebugString("WEAPON DEBUG: COMMAND AVAILABILITY\n");
    snprintf(msg, sizeof(msg), 
        "WEAPON DEBUG: fire_any_armed: %s\n"
        "WEAPON DEBUG: fire_air_to_ground: %s\n"
        "WEAPON DEBUG: fire_missile: %s\n",
        gFireAnyArmed ? "AVAILABLE" : "NOT FOUND",
        gFireAirToGround ? "AVAILABLE" : "NOT FOUND",
        gFireMissile ? "AVAILABLE" : "NOT FOUND");
    XPLMDebugString(msg);
}

static void TestFireCommandCallback(void* inRefcon)
{
    XPLMDebugString("WEAPON DEBUG: TESTING FIRE COMMANDS\n");
    
    if (gFireAnyArmed) {
        XPLMCommandOnce(gFireAnyArmed);
        XPLMDebugString("WEAPON DEBUG: Executed fire_any_armed command\n");
    } else {
        XPLMDebugString("WEAPON DEBUG: fire_any_armed command not available\n");
    }
    
    // Wait a moment and check status again
    DebugWeaponStatusCallback(NULL);
}