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
//#include <tf_player.h>
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
CDetour *PickupWeaponDetourMgr = NULL;
//declare and define the new function - the "wrapper" around the original
//keep in mind it's not bound or enabled yet! the function is just defined
// DETOUR_DECL_MEMBER1(PickupWeaponDetourFunc, bool, CTFDroppedWeapon *, pDroppedWeapon)
// {
//     //pre-original stuff

//     //todo: determine needed class from weapon & change player to class

//     g_pSM->LogMessage(myself, "Hello, world! This is right before the pickup process!");


//     //call original
//     DETOUR_MEMBER_CALL(PickupWeaponDetourFunc)(pDroppedWeapon);
    
    
//     //post-original stuff

//     //todo: switch player back to original class
// }

/*
    Bind Natives & Hooks 
*/

const sp_nativeinfo_t MyNatives[] = 
{
    {"initDetours", InitDetours},
    {"enableDetours", EnableDetours},
    {"disableDetours", DisableDetours},
	{NULL,			NULL},
};

bool SDK_OnLoad(char *error, size_t maxlen, bool late)
{

}

void Freeguns::SDK_OnAllLoaded()
{
    sharesys->AddNatives(myself, MyNatives);
}

/*
    Native Definitions
*/

cell_t InitDetours(IPluginContext *pContext, const cell_t *params)
{

}

cell_t EnableDetours(IPluginContext *pContext, const cell_t *params)
{

}

cell_t DisableDetours(IPluginContext *pContext, const cell_t *params)
{

}


cell_t ReferenceFunc(IPluginContext *pContext, const cell_t *params)
{
//pContext contains functions for "retrieving or modifying memory in the plugin"
//params is an array, where index 0 contains the size and the parameters start from index 1
//i.e. in ReferenceFunc(in1), params = [1, in1], and calling ReferenceFunc(25) makes params [1, 25]

    g_pSM->LogMessage(myself, "Hello, world! I got this to write: %i", params[1]);
    
    return params[1];
}
