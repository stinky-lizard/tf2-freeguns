#ifndef _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_

/*
 * Declarations for the Freeguns extension's functionality.
*/

//Wrapper function to bind the detours

bool InitAllDetours();
bool DeleteAllDetours();


//Natives

cell_t EnableDetours(IPluginContext *pContext, const cell_t *params);
cell_t DisableDetours(IPluginContext *pContext, const cell_t *params);

//Other stuff needed

extern IGameConfig *g_pGameConf;

extern bool freegunsEnabled;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H
