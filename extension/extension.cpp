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

    //interfaces
    sharesys->AddDependency(myself, "bintools.ext", true, true);


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
        g_pSM->LogMessage(myself, "DETOUR: weGood true!");              //DEBUG
        
        //reset for next time
        weGood_CanPickup = false;
        return true;
    }
    else return out; //weGood_CanPickup is already false, no need to reset it
}

int CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup ( int iLoadoutClass ) const
{
    //we've reached the GetLoadoutSlot call without returning, which means we've passed all the basic checks in CanPickup
    //(is alive, isn't taunting, etc.)
    //we also passed the check for if we have an active weapon, which I'm not completely sure if we need... but eh.
    //we dont care what this is doing. we'll overwrite its consequences in the canpickup detour anyways
    g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_CP");                //DEBUG
    g_pSM->LogMessage(myself, "DETOUR: Setting weGood");                //DEBUG
    weGood_CanPickup = true;
    
    if (!g_GetLoadout_hook) g_pSM->LogMessage(myself, "DETOUR: Something's wrong...");                //DEBUG
    
    return g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
}

static int slotToPlaceItemIn_PickupWeapon = -1;

bool CTFPlayerDetours::detour_PickupWeaponFromOther(CTFDroppedWeapon *pDroppedWeapon)
{
    g_pSM->LogMessage(myself, "DETOUR: PRE   PickupWeapon");            //DEBUG
    
    //get the associated weapon
    // const CEconItemView *pItem = pDroppedWeapon->GetItem();
    CEconItemView *pItem;
    ArgBuffer<void*> vstk(pDroppedWeapon);
    CallWrappers::GetItem->Execute(vstk, &pItem);

    // if (!pItem || !pItem->IsValid()) 
    bool IsPItemValid;
    ArgBuffer<void*> vstk(pItem);
    CallWrappers::IsValid->Execute(vstk, &IsPItemValid);
    
    if (!pItem || !IsPItemValid) 
    {    
        g_pSM->LogError(myself, "detour_PickupWeapon: pDroppedWeapon has no valid associated item!");   //DEBUG (only rets false in real game)
        return false;
    }

    //can we use this weapon without further effort? i.e. is this weapon meant for us?
    // int myClassIndex = GetPlayerClass()->GetClassIndex();
    CTFPlayerClass* myClass; 
    ArgBuffer<void*> vstk(this);
    CallWrappers::GetPlayerClass->Execute(vstk, &myClass);
    
    int myClassIndex;
    ArgBuffer<void*> vstk(myClass);
    CallWrappers::GetClassIndex->Execute(vstk, &myClassIndex);


    // bool canUseCurrentClass = pItem->GetStaticData()->CanBeUsedByClass(myClass);
    CTFItemDefinition *pItemStaticData;
    ArgBuffer<void*> vstk(pItem);
    CallWrappers::GetStaticData->Execute(vstk, &pItemStaticData);
    
    bool canUseCurrentClass;
    ArgBuffer<void*, int> vstk(pItemStaticData, myClassIndex);
    CallWrappers::CanBeUsedByClass->Execute(vstk, &canUseCurrentClass);


    //if yes, we can just call the original function and it'll all work out.

    //if no, we have to get the slot the weapon normally goes in and use that instead
    if (!canUseCurrentClass)
    {
        //get the default slot for this weapon (almost always the only slot, minus the shotgun, which will be a primary)
        // slotToPlaceItemIn_PickupWeapon = pItem->GetStaticData()->GetDefaultLoadoutSlot();
        ArgBuffer<void*> vstk(pItemStaticData);
        CallWrappers::GetDefaultLoadoutSlot->Execute(vstk, &slotToPlaceItemIn_PickupWeapon);

        //detour GetLoadoutSlot so we can replace the slot it says with our own
        if (!InitDetour("CTFItemDefinition::GetLoadoutSlot", &g_GetLoadout_hook, (void*)(&CTFItemDefDetours::detour_GetLoadoutSlot_PickupWeapon))) 
            g_pSM->LogError(myself, "Could not initialize detour_GetLoadoutSlot_PickupWeapon!");
    }

    //call original function
    bool out = g_PickupWeapon_hook.thiscall<bool>(this, pDroppedWeapon);
    
    g_pSM->LogMessage(myself, "DETOUR: POST  PickupWeapon");            //DEBUG

    //remove GetLoadoutSlot detour, dont need it anymore for now (not until the next pickup event)
    if (!canUseCurrentClass) g_GetLoadout_hook = {};
    
    //reset the slot var for next time
    slotToPlaceItemIn_PickupWeapon = -1;

    return out;
}

int CTFItemDefDetours::detour_GetLoadoutSlot_PickupWeapon ( int iLoadoutClass ) const
{
    g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_PW");                //DEBUG
    
    //first get the original result
    //(i could skip this... but i want to be careful abt unforeseen consequences)
    bool out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
    
    //are we using our own slot?
    if (slotToPlaceItemIn_PickupWeapon != -1)
    {
        //return that
        out = slotToPlaceItemIn_PickupWeapon;
        //reset it
        slotToPlaceItemIn_PickupWeapon = -1;
    }

    g_pSM->LogMessage(myself, "DETOUR: POST  GetLoadout_PW");                //DEBUG

    return out;
}

void Freeguns::SDK_OnUnload()
{
    g_CanPickup_hook = {};
    g_PickupWeapon_hook = {};
    g_GetLoadout_hook = {};

    //TODO: destroy callers
}

void Freeguns::SDK_OnAllLoaded()
{
    SM_GET_LATE_IFACE(BINTOOLS, g_pBinTools);

    if (!g_pBinTools) return;

    if (CallWrappers::InitCalls()) CallWrappers::wrappersInitialized = true;
    else CallWrappers::wrappersInitialized = false;
    

    // sharesys->AddNatives(myself, MyNatives);
}

bool GetVtableOffset(const char* key, int* value)
{
    if (!g_pGameConf->GetOffset(key, value))
    {
        g_pSM->LogError(myself, "Could not get offset for %s!", key);
        return false;
    }
    g_pSM->LogMessage(myself, "Retrieved offset for &s", key);              //DEBUG
    return true;
}

#define ASSERT_CALLER_INIT(caller) if(!caller) { g_pSM->LogError(myself, "Failed to initialize SDK call for %s!", key); return false; }

bool CallWrappers::InitCalls()
{
    //int CTFPlayerClassShared::GetClassIndex( void ) const;
    if (!GetClassIndex)
    {
        const char* key = "CTFPlayerClassShared::GetClassIndex";
        int offset; 
        if (!GetVtableOffset(key, &offset)) return false;

        PassInfo ret;

        ret.type = PassType_Basic;
        ret.flags = PASSFLAG_BYVAL;
        ret.size = sizeof(int);

        //function takes no parameters, so params is not needed. ret could also be null if it returned void
        //proof:    https://github.com/alliedmodders/sourcemod/blob/92fb9f62ddc0ce63da8440f53edcba74fa3abe95/extensions/tf2/natives.cpp#L378
        //          https://github.com/ValveSoftware/source-sdk-2013/blob/68c8b82fdcb41b8ad5abde9fe1f0654254217b8e/src/game/server/player.cpp#L5277
        GetClassIndex = g_pBinTools->CreateVCall(offset, 0, 0, &ret, NULL, 0);
        ASSERT_CALLER_INIT(GetClassIndex)
    }
    //CEconItemView* CTFDroppedWeapon::GetItem();
    if (!GetItem)
    {
        const char* key = "CTFDroppedWeapon::GetItem";
        int offset;
        if (!GetVtableOffset(key, &offset)) return false;

        PassInfo ret;

        //returns a pointer
        ret.type = PassType_Basic;
        ret.flags = PASSFLAG_BYVAL;
        ret.size = sizeof(void*);

        GetItem = g_pBinTools->CreateVCall(offset, 0, 0, &ret, NULL, 0);
        ASSERT_CALLER_INIT(GetItem)
    }

    // CTFPlayerClass* CTFPlayer::GetPlayerClass( void );
    if (!GetPlayerClass)
    {
        const char* key = "CTFPlayer::GetPlayerClass";
        int offset;
        if (!GetVtableOffset(key, &offset)) return false;

        PassInfo ret;

        ret.type = PassType_Basic;
        ret.flags = PASSFLAG_BYVAL;
        ret.size = sizeof(void*);

        GetPlayerClass = g_pBinTools->CreateVCall(offset, 0, 0, &ret, NULL, 0);
        ASSERT_CALLER_INIT(GetPlayerClass)
    }

    // int CTFItemDefinition::CanBeUsedByClass( int iClass ) const;
    if (!CanBeUsedByClass)
    {
        const char* key = "CTFItemDefinition::CanBeUsedByClass";
        int offset;
        if (!GetVtableOffset(key, &offset)) return false;

        PassInfo ret, params[1];
        
        ret.type = PassType_Basic;
        ret.flags = PASSFLAG_BYVAL;
        ret.size = sizeof(int);

        params[0].type = PassType_Basic;
        params[0].flags = PASSFLAG_BYVAL;
        params[0].size = sizeof(int);

        CanBeUsedByClass = g_pBinTools->CreateVCall(offset, 0, 0, &ret, params, 1);
        ASSERT_CALLER_INIT(CanBeUsedByClass)
    }

    // int CTFItemDefinition::GetDefaultLoadoutSlot( void ) const;
    if (!GetDefaultLoadoutSlot)
    {
        const char* key = "CTFItemDefinition::GetDefaultLoadoutSlot";
        int offset;
        if (!GetVtableOffset(key, &offset)) return false;

        PassInfo ret;

        ret.type = PassType_Basic;
        ret.flags = PASSFLAG_BYVAL;
        ret.size = sizeof(int);

        GetDefaultLoadoutSlot = g_pBinTools->CreateVCall(offset, 0, 0, &ret, NULL, 0);
        ASSERT_CALLER_INIT(GetDefaultLoadoutSlot)
    }

    // bool CEconItemView::IsValid( void ) const;
    if(!IsValid)
    {
        const char* key = "CEconItemView::IsValid";
        int offset;
        if (!GetVtableOffset(key, &offset)) return false;

        PassInfo ret;

        ret.type = PassType_Basic;
        ret.flags = PASSFLAG_BYVAL;
        ret.size = sizeof(bool);

        IsValid = g_pBinTools->CreateVCall(offset, 0, 0, &ret, NULL, 0);
        ASSERT_CALLER_INIT(IsValid)
    }

    // GameItemDefinition_t* CEconItemView::GetStaticData( void ) const;
    if (!GetStaticData)
    {
        const char* key = "CEconItemView::GetStaticData";
        int offset;
        if (!GetVtableOffset(key, &offset)) return false;

        PassInfo ret;

        ret.type = PassType_Basic;
        ret.flags = PASSFLAG_BYVAL;
        ret.size = sizeof(void*);

        GetStaticData = g_pBinTools->CreateVCall(offset, 0, 0, &ret, NULL, 0);
        ASSERT_CALLER_INIT(GetStaticData)
    }


    return true;

}

bool Freeguns::QueryRunning(char *error, size_t maxlength)
{
    SM_CHECK_IFACE(BINTOOLS, g_pBinTools);

    return CallWrappers::wrappersInitialized;
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