/*
 * WeaponResearch.h
 * 
 * Header for experimental weapon research plugin
 */

#ifndef WEAPONRESEARCH_H
#define WEAPONRESEARCH_H

#ifdef __cplusplus
extern "C" {
#endif

// Plugin interface functions
PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc);
PLUGIN_API void XPluginStop(void);
PLUGIN_API void XPluginDisable(void);
PLUGIN_API int XPluginEnable(void);
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void* inParam);

#ifdef __cplusplus
}
#endif

#endif // WEAPONRESEARCH_H