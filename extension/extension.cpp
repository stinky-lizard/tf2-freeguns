/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include <string>
#include <iostream>
#include <CDetour/detours.h>
using namespace std;

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

Freeguns g_Freeguns;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_Freeguns);

/*
    Declarations
*/

// cell_t EnableDetours(IPluginContext *pContext, const cell_t *params);
// cell_t DisableDetours(IPluginContext *pContext, const cell_t *params);

bool InitCanPickupDetour();
bool InitPickupWeaponDetour();


IGameConfig *g_pGameConf = NULL;

CDetour *CanPickupDetour = NULL;
CDetour *PickupWeaponDetour = NULL;
// CDetour *TryToPickupDetour;
//declare and define the new function - the "wrapper" around the original
//keep in mind it's not bound or enabled yet! the function is just defined
DETOUR_DECL_MEMBER1(CanPickupDroppedWeapon, bool, const void *, pWeapon)
{
    //pre-original stuff

    //todo: determine needed class from weapon & change player to class
    
    g_pSM->LogMessage(myself, "Hello, world! This is right before CanPickup!");
    
    
    //call original
    bool out = DETOUR_MEMBER_CALL(CanPickupDroppedWeapon)(pWeapon);
    
    g_pSM->LogMessage(myself, "This is right after CanPickup!");
    //post-original stuff
    
    //todo: switch player back to original class
    return out; 
}

DETOUR_DECL_MEMBER1(PickupWeaponFromOther, bool, void *, pDroppedWeapon)
{
    //pre-original stuff

    //todo: determine needed class from weapon & change player to class
    
    g_pSM->LogMessage(myself, "This is right before PickupWeapon!");
    
    
    //call original
    bool out = DETOUR_MEMBER_CALL(PickupWeaponFromOther)(pDroppedWeapon);
    
    
    g_pSM->LogMessage(myself, "This is right after PickupWeapon!");
    //post-original stuff
    
    //todo: switch player back to original class
    return out; 
}

// DETOUR_DECL_MEMBER0(TryToPickupDetourFunc, bool)
// {
//     //pre-original stuff

//     //todo: determine needed class from weapon & change player to class
    
//     g_pSM->LogMessage(myself, "Hello, world! This is right before TryToPickup!");
    
    
//     //call original
//     bool out = DETOUR_MEMBER_CALL(TryToPickupDetourFunc)();
    
//     g_pSM->LogMessage(myself, "This is right after TryToPickup!");
//     //post-original stuff
    
//     //todo: switch player back to original class
//     return out; 
// }


/*
Bind Natives & Hooks 
*/

const sp_nativeinfo_t MyNatives[] = 
{
    // {"enableDetours", EnableDetours},
    // {"disableDetours", DisableDetours},
	{NULL,			NULL},
};

bool Freeguns::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
    //load game conf

    char conf_error[255] = "";
	if (!gameconfs->LoadGameConfigFile("tf2.freeguns", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (conf_error[0])
		{
			snprintf(error, maxlen, "Could not read tf2.freeguns.txt: %s\n", conf_error);
		}
		return false;
	}

    //init and enable detours here
    CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
    
    if (!InitCanPickupDetour()) return false;

    if (!InitPickupWeaponDetour()) return false;
    
    // INIT_DETOUR(TryToPickupDetour, TryToPickupDetourFunc, "CTFPlayer::TryToPickupDroppedWeapon", 31);
    return true;
}

//I could consolidate these functions into a macro to save space, but I think it's easier to read this way. Easier to work with too

//Iniitialize and enable CanPickupDroppedWeapon detour. Use only once after CDetourManager::Init
bool InitCanPickupDetour()
{
    const char* gamedataKey = "CTFPlayer::CanPickupDroppedWeapon";
    
    CanPickupDetour = DETOUR_CREATE_MEMBER(CanPickupDroppedWeapon, gamedataKey);
    
    if (CanPickupDetour == NULL)
    {
        g_pSM->LogError(myself, "Could not initialize %s detour (Error code 11)", gamedataKey);
        return false;
    }
    
    CanPickupDetour->EnableDetour();
    
    if (!(CanPickupDetour->IsEnabled()))
    {
        g_pSM->LogError(myself, "Could not enable %s detour (Error code 12)", gamedataKey);
        return false;
    }
    else
        g_pSM->LogMessage(myself, "Initialized & enabled detour for %s", gamedataKey);
    
    return true;
}

//Iniitialize and enable PickupWeaponFromOther detour. Use only once after CDetourManager::Init
bool InitPickupWeaponDetour()
{
    const char* gamedataKey = "CTFPlayer::PickupWeaponFromOther";
    
    PickupWeaponDetour = DETOUR_CREATE_MEMBER(PickupWeaponFromOther, gamedataKey);
    
    if (PickupWeaponDetour == NULL)
    {
        g_pSM->LogError(myself, "Could not initialize %s detour (Error code 21)", gamedataKey);
        return false;
    }

    PickupWeaponDetour->EnableDetour();
    
    if (!(PickupWeaponDetour->IsEnabled()))
    {
        g_pSM->LogError(myself, "Could not enable %s detour (Error code 22)", gamedataKey);
        return false;
    }
    else
        g_pSM->LogMessage(myself, "Initialized & enabled detour for %s", gamedataKey);

    return true;
}


void Freeguns::SDK_OnUnload()
{
    // if (CanPickupDetour != NULL)
    //     CanPickupDetour->Destroy();
    // if (PickupWeaponDetour != NULL)
    //     PickupWeaponDetour->Destroy();
}

void Freeguns::SDK_OnAllLoaded()
{
    // sharesys->AddNatives(myself, MyNatives);
}

/*
    Native Definitions
*/

// cell_t EnableDetours(IPluginContext *pContext, const cell_t *params)
// {
//     if (PickupWeaponDetour != NULL)
//         PickupWeaponDetour->EnableDetour();
//     if (CanPickupDetour != NULL)
//         CanPickupDetour->EnableDetour();
//     return params[1];
// }

// cell_t DisableDetours(IPluginContext *pContext, const cell_t *params)
// {
//     if (PickupWeaponDetour != NULL)
//         PickupWeaponDetour->DisableDetour();
//     if (CanPickupDetour != NULL)
//         CanPickupDetour->DisableDetour();
//     return params[1];
// }


// cell_t ReferenceFunc(IPluginContext *pContext, const cell_t *params)
// {
// //pContext contains functions for "retrieving or modifying memory in the plugin"
// //params is an array, where index 0 contains the size and the parameters start from index 1
// //i.e. in ReferenceFunc(in1), params = [1, in1], and calling ReferenceFunc(25) makes params [1, 25]

//     g_pSM->LogMessage(myself, "Hello, world! I got this to write: %i", params[1]);
    
//     return params[1];
// }
