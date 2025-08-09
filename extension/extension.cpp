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
#include "freeguns.h"
#include <string>
#include <iostream>
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
    
    if (!InitDetour("CTFPlayer::CanPickupDroppedWeapon", &g_CanPickup_hook, (void*)(&CTFPlayerDetours::detour_CanPickupDroppedWeapon))) return false;
    
    if (!InitDetour("CTFPlayer::PickupWeaponFromOther", &g_PickupWeapon_hook, (void*)(&CTFPlayerDetours::detour_PickupWeaponFromOther))) return false;
    
    return true;
}


//Iniitialize detours
bool InitDetour(const char* gamedata, SafetyHookInline *hookObj, void* callback)
{
    void *pAddress;

    if (!g_pGameConf->GetMemSig(gamedata, &pAddress))
	{
		g_pSM->LogError(myself, "Signature for %s not found in gamedata", gamedata);
		return false;
	}

	if (!pAddress)
	{
		g_pSM->LogError(myself, "Sigscan for %s failed", gamedata);
		return false;
	}
    g_pSM->LogMessage(myself, "Got sig for %s", gamedata);               //DEBUG
    
    *hookObj = safetyhook::create_inline(pAddress, callback);

    g_pSM->LogMessage(myself, "Created InlineHook for %s", gamedata);    //DEBUG

    return true;
}

static bool weGood_CanPickup = false;

bool CTFPlayerDetours::detour_CanPickupDroppedWeapon(const CTFDroppedWeapon *pWeapon)
{
    g_pSM->LogMessage(myself, "---");              //DEBUG
    g_pSM->LogMessage(myself, "Starting pickup process.");              //DEBUG
    g_pSM->LogMessage(myself, "DETOUR: PRE   CanPickup");               //DEBUG
    
    if (!InitDetour("CTFItemDefinition::GetLoadoutSlot", &g_GetLoadout_hook, (void*)(&CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup))) 
        g_pSM->LogError(myself, "Could not initialize detour_GetLoadoutSlot_CanPickup!");
    

    bool out = g_CanPickup_hook.thiscall<bool>(this, pWeapon);

    
    g_pSM->LogMessage(myself, "DETOUR: POST  CanPickup");               //DEBUG

    g_GetLoadout_hook = {};
    
    if (!weGood_CanPickup) g_pSM->LogMessage(myself, "DETOUR: weGood false...");    //DEBUG

    if (weGood_CanPickup)
    {
        //this particular one worked out, reset for next time
        g_pSM->LogMessage(myself, "DETOUR: weGood true!");              //DEBUG
        weGood_CanPickup = false;
        return true;
    }
    else return out; //already false
}

int CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup ( int iLoadoutClass ) const
{
    //we've reached the GetLoadoutSlot call without returning, which means we've passed all the basic checks in CanPickup
    //(is alive, isn't taunting, etc.)
    //we also passed the check for if we have an active weapon, which I'm not completely sure if we need... but eh.
    g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_CP");                //DEBUG
    g_pSM->LogMessage(myself, "DETOUR: Setting weGood");                //DEBUG
    weGood_CanPickup = true;
    
    if (!g_GetLoadout_hook) g_pSM->LogMessage(myself, "DETOUR: Something's wrong...");                //DEBUG
    
    return g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
}

bool CTFPlayerDetours::detour_PickupWeaponFromOther(CTFDroppedWeapon *pDroppedWeapon)
{
    g_pSM->LogMessage(myself, "DETOUR: PRE   PickupWeapon");            //DEBUG
    
    if (!InitDetour("CTFItemDefinition::GetLoadoutSlot", &g_GetLoadout_hook, (void*)(&CTFItemDefDetours::detour_GetLoadoutSlot_PickupWeapon))) 
        g_pSM->LogError(myself, "Could not initialize detour_GetLoadoutSlot_PickupWeapon!");

    bool out = g_PickupWeapon_hook.thiscall<bool>(this, pDroppedWeapon);
    
    g_pSM->LogMessage(myself, "DETOUR: POST  PickupWeapon");            //DEBUG
    
    return out;
}

int CTFItemDefDetours::detour_GetLoadoutSlot_PickupWeapon ( int iLoadoutClass ) const
{
    g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_PW");                //DEBUG
    
    bool out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
    
    
    g_pSM->LogMessage(myself, "DETOUR: POST  GetLoadout_PW");                //DEBUG

    return out;
}

void Freeguns::SDK_OnUnload()
{
    g_CanPickup_hook = {};
    g_PickupWeapon_hook = {};
    g_GetLoadout_hook = {};
}

void Freeguns::SDK_OnAllLoaded()
{
    // sharesys->AddNatives(myself, MyNatives);
}

/*
    Native Definitions
*/

// cell_t ReferenceFunc(IPluginContext *pContext, const cell_t *params)
// {
// //pContext contains functions for "retrieving or modifying memory in the plugin"
// //params is an array, where index 0 contains the size and the parameters start from index 1
// //i.e. in ReferenceFunc(in1), params = [1, in1], and calling ReferenceFunc(25) makes params [1, 25]

//     g_pSM->LogMessage(myself, "Hello, world! I got this to write: %i", params[1]);
    
//     return params[1];
// }