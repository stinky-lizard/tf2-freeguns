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

#include <server_class.h>

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
    g_pSM->LogMessage(myself, "-----------------------------------------");              //DEBUG
    g_pSM->LogMessage(myself, "-----------------------------------------");              //DEBUG
    g_pSM->LogMessage(myself, "-----------------------------------------");              //DEBUG
    g_pSM->LogMessage(myself, "Starting pickup process.");              //DEBUG
    g_pSM->LogMessage(myself, "DETOUR: PRE   CanPickup");               //DEBUG
    
    //GetLoadoutSlot is after all the preliminary checks we want to keep. we don't care that it does all that other stuff
    //well use this as a marker to tell if those checks passed
    if (!InitDetour("CTFItemDefinition::GetLoadoutSlot", &g_GetLoadout_hook, (void*)(&CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup))) 
        g_pSM->LogError(myself, "Could not initialize detour_GetLoadoutSlot_CanPickup!");
    
    //call the original
    bool out = g_CanPickup_hook.thiscall<bool>(this, pWeapon);

    
    g_pSM->LogMessage(myself, "DETOUR: POST  CanPickup");               //DEBUG

    //reset the GetLoadout hook
    g_GetLoadout_hook = {};

    if (!weGood_CanPickup) g_pSM->LogMessage(myself, "DETOUR: weGood false...");    //DEBUG

    //if GetLoadout ran, then all the checks passed, and we're good to pick up the weapon
    if (weGood_CanPickup)
    {
        // g_pSM->LogMessage(myself, "DETOUR: weGood true!");              //DEBUG
        
        //reset for next time
        weGood_CanPickup = false;
        return true;
    }
    else return out; //weGood_CanPickup is already false, no need to reset it
}

//TODO: for some reason this runs twice (four times for PickupWeapon). does it have to do with when and where it's initialized?
//the others don't run twice...
int CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup ( int iLoadoutClass ) const
{
    //we've reached the GetLoadoutSlot call without returning, which means we've passed all the basic checks in CanPickup
    //(is alive, isn't taunting, etc.)
    //we also passed the check for if we have an active weapon, which I'm not completely sure if we need... but eh.
    //we dont care what this is doing. we'll overwrite its consequences in the canpickup detour anyways
    g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_CP");                //DEBUG
    // g_pSM->LogMessage(myself, "DETOUR: Setting weGood");                //DEBUG
    weGood_CanPickup = true;
    
    if (!g_GetLoadout_hook) g_pSM->LogMessage(myself, "DETOUR: Something's wrong...");                //DEBUG
    
    int out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);

    g_GetLoadout_hook = {};
    
    return out; 
}


bool CTFPlayerDetours::detour_PickupWeaponFromOther(CTFDroppedWeapon *pDroppedWeapon)
{
    g_pSM->LogMessage(myself, "DETOUR: PRE   PickupWeapon");            //DEBUG
    
    
    // can we use this weapon without further effort? i.e. is this weapon meant for us?
    //nevermind, GetLoadoutSlot on the dropped weapon will return a nonnegative number if we can pick it up, or -1 if we can't. No need to check ourselves.
    // bool canUseCurrentClass = pItem->GetStaticData()->CanBeUsedByClass(myClass); //this code would be based on this, but ofc, no need
    
    // //if no, we'll have to get the slot the weapon normally goes in and use that instead
    //get the default slot for this weapon (almost always the only slot, minus the shotgun, which will be a primary)
    // slotToPlaceItemIn_PickupWeapon = pItem->GetStaticData()->GetDefaultLoadoutSlot();
    
    //NEVERMIND! i realized we can just call GetLoadoutSlot on every class.
    //Whichever one returns a nonnegative number will be the class we want and the slot it goes in.
    //This is an alternative to:
    // - Getting the item definition index from the SendProps
    // - Calling GetItemSchema() (or GEconItemSchema()?)
    // - Calling CEconItemSchema::GetItemDefinition() with our index
    // - Adding a magic number that's the offset from the start of the item def object to where the default loadout slot is held in memory
    
    //... That might, *might*, be more proper, but this way is CERTAINLY easier. Not sure if it uses more processing time though...
    // Calling GetLoadoutSlot(), a relatively small function, up to 9 times, or creating(?) an item schema object... hmm...
    // either way I'm doing the easier way. At least for now.
    
    // // start of original way here:
    // int itemDefIndex;
    // GetEntProp(pDroppedWeapon, "m_iItemDefinitionIndex", itemDefIndex);
    // // g_pSM->LogMessage(myself, "itemDefIndex: %i", itemDefIndex);  //DEBUG
    
    //detour GetLoadoutSlot so we can replace the slot it says with our own
    if (!InitDetour("CTFItemDefinition::GetLoadoutSlot", &g_GetLoadout_hook, (void*)(&CTFItemDefDetours::detour_GetLoadoutSlot_PickupWeapon))) 
    g_pSM->LogError(myself, "Could not initialize detour_GetLoadoutSlot_PickupWeapon!");
    
    //actually pick up the weapon and drop ours
    bool out = g_PickupWeapon_hook.thiscall<bool>(this, pDroppedWeapon);
    
    g_pSM->LogMessage(myself, "DETOUR: POST  PickupWeapon");            //DEBUG
    
    //remove GetLoadoutSlot detour, dont need it anymore for now (not until the next pickup event)
    g_GetLoadout_hook = {};
    
    return out;
}

// So we could default this to -1, or 0. defaulting to 0 would cause us to drop our primary, but we would still pick up the weapon.
// Would work if we picked up a primary, but a secondary would cause us to just not have a primary and then stuff would get weird.
// Defaulting to -1 would cause GetEntityForLoadoutSlot to not find anything, and return nothing.
// That would cause PickupWeapon to return false in the pre-pickup if ( !pWeapon ) check.
// (or if we skip that, it would pick up the weapon, but not drop our original one, since it didnt find it, and not drop our weapon after if ( pWeapon ))

#define SLOTTODROP_PW_DEFAULT -1

int CTFItemDefDetours::detour_GetLoadoutSlot_PickupWeapon ( int iLoadoutClass ) const
{
    g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_PW");                //DEBUG
    
    int slotToDrop_PickupWeapon = SLOTTODROP_PW_DEFAULT;
    
    //first get the original result
    int out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
    
    // g_pSM->LogMessage(myself, "DETOUR: POST  GetLoadout_PW");                //DEBUG

    g_pSM->LogMessage(myself, "out (me):    %i", out);                       //DEBUG
    
    //would we not be able to pick this up? 
    if (out == -1)
    {
        // get the loadout slot that this weapon would normally go in
        // ... by checking every class until one of them returns something other than -1
        
        //from 0-9 the classes are: Undefined, Scout, Sniper, Soldier, Demo, Medic, Heavy, Pyro, Spy, Engineer
        for (int i = 1; i++; i <= 9)
        {
            int slotForThisClass = g_GetLoadout_hook.thiscall<int>(this, i);

            g_pSM->LogMessage(myself, "Slot for class %i: %i", i, slotForThisClass);    //DEBUG

            if (slotForThisClass != -1)
            {
                //the weapon is meant for this class!
                slotToDrop_PickupWeapon = slotForThisClass;
            }
            // otherwise move on to the next
        }
        g_pSM->LogMessage(myself, "slotToPlace: %i", slotToDrop_PickupWeapon);   //DEBUG
        
        out = slotToDrop_PickupWeapon;
        //reset it
        slotToDrop_PickupWeapon = SLOTTODROP_PW_DEFAULT;
    }
    
    g_GetLoadout_hook = {};
    
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


// native int GetEntProp(int entity, PropType type, const char[] prop, int size=4, int element=0);
static bool GetEntProp(void* pEntity, const char* prop, int& result, bool isEntity, void* entResult, int element)
{
    
	int offset;
    int size = 4;

    int bit_count;
	bool is_unsigned = false;
        
    ServerClass *pServerClass = gamehelpers->FindEntityServerClass((CBaseEntity*)pEntity);
    
    sm_sendprop_info_t info; 
    SendProp *pProp; 

    if (!pServerClass) 
    {
        g_pSM->LogError(myself, "Failed to retrieve server class for %s!", prop);
        return false;
    }

    //get info about prop
    bool infoFound = gamehelpers->FindSendPropInfo(pServerClass->GetName(), prop, &info);
    if (!infoFound) 
    { 
        const char *class_name = gamehelpers->GetEntityClassname((CBaseEntity*)pEntity); 
        g_pSM->LogError(myself,"Property \"%s\" not found (entity %s)", prop, class_name);
        return false; 
    } 

    offset = info.actual_offset; 
    // g_pSM->LogMessage(myself, "offset: %i", offset);    //DEBUG
    pProp = info.prop; 
    bit_count = pProp->m_nBits;
    // g_pSM->LogMessage(myself, "bit_count: %i", bit_count);    //DEBUG

    //get if SPROP_UNSIGNED flag is set
    is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

    
    // unsure what SPROP_VARINT is... cant find what it means or where its declared/defined
    // if (pProp->GetFlags() & SPROP_VARINT)
    // {
    //     bit_count = sizeof(int) * 8;
    // }


    if (isEntity)
    {
        CBaseHandle *hndl;
        hndl = (CBaseHandle *)((uint8_t *)pEntity + offset);

        CBaseEntity *pHandleEntity = gamehelpers->ReferenceToEntity(hndl->GetEntryIndex());

        // if (!pHandleEntity || *hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
        if (!pHandleEntity)
        {
            g_pSM->LogError(myself, "GetEntProp isEntity check didn't pass");
            return false;
        }

        entResult = pHandleEntity;
        return true;

    }

    //bleh
	if (bit_count < 1)
	{
		bit_count = size * 8;
	}
	if (bit_count >= 17)
	{
		result = *(int32_t *)((uint8_t *)pEntity + offset);
        return true;
	}
	else if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			result = *(uint16_t *)((uint8_t *)pEntity + offset);
            return true;
		}
		else
		{
			result = *(int16_t *)((uint8_t *)pEntity + offset);
            return true;
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			result = *(uint8_t *)((uint8_t *)pEntity + offset);
            return true;
		}
		else
		{
			result = *(int8_t *)((uint8_t *)pEntity + offset);
            return true;
		}
	}
	else
	{
		return *(bool *)((uint8_t *)pEntity + offset) ? 1 : 0;
	}

	return 0;
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