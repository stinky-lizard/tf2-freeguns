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

#include <Zydis.h>

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


    auto instructionPointer = (void*)(g_PickupWeapon_hook.target());
    
    ZydisDecoder decoder{};

    #if SAFETYHOOK_ARCH_X86_64
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    #elif SAFETYHOOK_ARCH_X86_32
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
    #endif
    
    g_pSM->LogMessage(myself, "Searching instructions & patching functions...");
    
    void* pointerForPatch1, *pointerForPatch2, *pointerForPatch3, *pointerForPatch4;
    
    for (int i = 0; i < 50; i++) {
        ZydisDecodedInstruction ix{};
        ZydisDecodedOperand opds[10];
        
        ZydisDecoderDecodeFull(&decoder, reinterpret_cast<void*>(instructionPointer), 15, &ix, opds);
        
        //TEST rax, rax
        if (ix.opcode == 0x85 && ix.length == 3 && opds[0].reg.value == 0x35)
        {
            // g_pSM->LogMessage(myself, "%i: ip = %x:", i, instructionPointer);
            // g_pSM->LogMessage(myself, "length = %i, opcode = %x", ix.length, ix.opcode);
            // g_pSM->LogMessage(myself, "mnemonic: %i", ix.mnemonic);
            // g_pSM->LogMessage(myself, "(TEST is 785)");
            // g_pSM->LogMessage(myself, "opds 0: reg: %x", opds[0].reg.value);

            //it's probably ours
            break;
        }
        instructionPointer += ix.length;
    }
    pointerForPatch1 = instructionPointer;
 
    // instructionPointer += 3;    //this WOULD go to the next instruction... if we didn't just insert a few by making a detour.
        
    for (int i = 0; i < 50; i++) {
        ZydisDecodedInstruction ix{};
        ZydisDecodedOperand opds[10];
        
        ZydisDecoderDecodeFull(&decoder, reinterpret_cast<void*>(instructionPointer), 15, &ix, opds);
        
        //MOV rdi, rax
        if (ix.opcode == 0x89 && ix.length == 3 && opds[0].reg.value == 0x3c)
        {
            // g_pSM->LogMessage(myself, "%i: ip = %x:", i, instructionPointer);
            // g_pSM->LogMessage(myself, "length = %i, opcode = %x", ix.length, ix.opcode);
            // g_pSM->LogMessage(myself, "mnemonic: %i", ix.mnemonic);
            // g_pSM->LogMessage(myself, "(TEST is 785)");
            // g_pSM->LogMessage(myself, "opds 0: reg: %x", opds[0].reg.value);
            
            //it's probably ours
            break;
        }

        instructionPointer += ix.length;
    }
    
    pointerForPatch2 = instructionPointer;

    for (int i = 0; i < 50; i++) {
        ZydisDecodedInstruction ix{};
        ZydisDecodedOperand opds[10];
        
        ZydisDecoderDecodeFull(&decoder, reinterpret_cast<void*>(instructionPointer), 15, &ix, opds);
        
        //TEST rax, rax
        if (ix.opcode == 0x85 && ix.length == 3 && opds[0].reg.value == 0x35) break;
        instructionPointer += ix.length;
    }
    
    pointerForPatch3 = instructionPointer;

    for (int i = 0; i < 50; i++) {
        ZydisDecodedInstruction ix{};
        ZydisDecodedOperand opds[10];
        
        ZydisDecoderDecodeFull(&decoder, reinterpret_cast<void*>(instructionPointer), 15, &ix, opds);
        
        //MOV [rbp+var_58], rax
        if (ix.opcode == 0x89 && ix.length == 4 && opds[0].mem.base == 0x3a)
        {
            g_pSM->LogMessage(myself, "Found hook 4 location.");
            // g_pSM->LogMessage(myself, "+8b: ip = %x:", instructionPointer);
            // g_pSM->LogMessage(myself, "length = %i, opcode = %x", ix.length, ix.opcode);
            // g_pSM->LogMessage(myself, "opds 0: mem: type: %x", opds[0].mem.type);
            // g_pSM->LogMessage(myself, "opds 0: mem: base: %x", opds[0].mem.base);
            // g_pSM->LogMessage(myself, "opds 0: mem: indx: %x", opds[0].mem.index);
            // g_pSM->LogMessage(myself, "opds 0: mem: scal: %x", opds[0].mem.scale);
            // g_pSM->LogMessage(myself, "opds 0: mem: segm: %x", opds[0].mem.segment);
            // g_pSM->LogMessage(myself, "opds 0: mem: hdis: %x", opds[0].mem.disp.has_displacement);
            // g_pSM->LogMessage(myself, "opds 0: mem: hdis: %x", opds[0].mem.disp.value);
            
            break;
        }
        instructionPointer += ix.length;
    }
    
    pointerForPatch4 = instructionPointer;

    g_pSM->LogMessage(myself, "1: %x (%i)", pointerForPatch1, pointerForPatch1);
    g_pSM->LogMessage(myself, "2: %x (%i)", pointerForPatch2, pointerForPatch2);
    g_pSM->LogMessage(myself, "3: %x (%i)", pointerForPatch3, pointerForPatch3);
    g_pSM->LogMessage(myself, "4: %x (%i)", pointerForPatch4, pointerForPatch4);
    //create_mid inserts instructions at the given pointer and pushes everything past it down.
    //... so just do it in reverse, so 1 will push 2, 3, and 4 down, instead of having to work around 1.
    g_PickupWeapon_mid_hook_4 = safetyhook::create_mid(pointerForPatch4, patch_PickupWeaponFromOther_4);
    g_PickupWeapon_mid_hook_3 = safetyhook::create_mid(pointerForPatch3, patch_PickupWeaponFromOther_3);
    g_PickupWeapon_mid_hook_2 = safetyhook::create_mid(pointerForPatch2, patch_PickupWeaponFromOther_2);
    g_PickupWeapon_mid_hook_1 = safetyhook::create_mid(pointerForPatch1, patch_PickupWeaponFromOther_1);

    //FIXME:
    //I'm not sure this way will work to jump past the post-GetEnt check.
    //The problem is that the code we want (outside of the "if (pWeapon)" in the source) is basically right alongside the code we don't.
    //It's not on a separate execution block - to get to what we want we have to go through tons of stuff like UTIL_Remove(pWeapon).
    //So skipping the check means some very important stuff won't happen (like networking)
    //but not skipping the check means we crash.
    
    //The only way I can think of is to write in a jump operation to just past the UTIL_Remove, probably specifically address 0000000000F69DC9 in the assembly.
    //That would probably go best in the GetEnt detour, just after the Weapon_GetSlot call.
    //However, I'm unsure if it would be a permanent change, and if we'd have to turn it off and on.
    //I did find this cool post: https://www.unknowncheats.me/forum/c-and-c-/67884-mid-function-hook-deal.html
    //also this: https://forums.alliedmods.net/showthread.php?t=317175
    

    // if (!InitDetour("CBaseCombatCharacter::Weapon_GetSlot", &g_WeaponGetSlot_hook, (void*)(&CBaseCmbtChrDetours::detour_Weapon_GetSlot))) return false;
    
    //do this in PickupWeapon
    // if (!InitDetour("CTFPlayer::GetEntityForLoadoutSlot", &g_GetEnt_hook, (void*)(&CTFPlayerDetours::detour_GetEntityForLoadoutSlot))) return false;
    
    //also this
    // if (!InitDetour("TranslateWeaponEntForClass", &g_Translate_hook, (void*)(&detour_TranslateWeaponEntForClass))) return false;
    
    return true;
}

void patch_PickupWeaponFromOther_1(SafetyHookContext& ctx)
{
    g_pSM->LogMessage(myself, "patch_PickupWeapon 1 ran!");   //DEBUG
    #if SAFETYHOOK_OS_WINDOWS
    #if SAFETYHOOK_ARCH_X86_64
    g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
    #elif SAFETYHOOK_ARCH_X86_32
    g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
    #endif
    #elif SAFETYHOOK_OS_LINUX
    #if SAFETYHOOK_ARCH_X86_64
    g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
    if (ctx.rax == 0) ctx.rax = 1;
    #elif SAFETYHOOK_ARCH_X86_32
    g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
    #endif
    #endif
}

void patch_PickupWeaponFromOther_2(SafetyHookContext& ctx)
{
    g_pSM->LogMessage(myself, "patch_PickupWeapon 2 ran!");   //DEBUG
    #if SAFETYHOOK_OS_WINDOWS
    #if SAFETYHOOK_ARCH_X86_64
    g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
        #elif SAFETYHOOK_ARCH_X86_32
            g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
        #endif
    #elif SAFETYHOOK_OS_LINUX
        #if SAFETYHOOK_ARCH_X86_64
            g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
            if (ctx.rax == 1) ctx.rax = 0; //don't crash on the dynamic_cast
        #elif SAFETYHOOK_ARCH_X86_32
            g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
        #endif
    #endif
}

void patch_PickupWeaponFromOther_3(SafetyHookContext& ctx)
{
    g_pSM->LogMessage(myself, "patch_PickupWeapon 3 ran!");   //DEBUG
    #if SAFETYHOOK_OS_WINDOWS
    #if SAFETYHOOK_ARCH_X86_64
    g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
    #elif SAFETYHOOK_ARCH_X86_32
    g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
    #endif
    #elif SAFETYHOOK_OS_LINUX
    #if SAFETYHOOK_ARCH_X86_64
    g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
    if (ctx.rax == 0) ctx.rax = 1;
    #elif SAFETYHOOK_ARCH_X86_32
    g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
    #endif
    #endif
}

void patch_PickupWeaponFromOther_4(SafetyHookContext& ctx)
{
    g_pSM->LogMessage(myself, "patch_PickupWeapon 4 ran!");   //DEBUG
    #if SAFETYHOOK_OS_WINDOWS
    #if SAFETYHOOK_ARCH_X86_64
    g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
        #elif SAFETYHOOK_ARCH_X86_32
            g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
        #endif
    #elif SAFETYHOOK_OS_LINUX
        #if SAFETYHOOK_ARCH_X86_64
            g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);    //DEBUG
            if (ctx.rax == 1) ctx.rax = 0; //don't crash on the dynamic_cast
        #elif SAFETYHOOK_ARCH_X86_32
            g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);    //DEBUG
        #endif
    #endif
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
    // g_pSM->LogMessage(myself, "Got sig for %s", gamedata);               //DEBUG
    
    *hookObj = safetyhook::create_inline(pAddress, callback);

    // g_pSM->LogMessage(myself, "Created InlineHook for %s", gamedata);    //DEBUG

    return true;
}

static bool weGood_CanPickup = false;

bool CTFPlayerDetours::detour_CanPickupDroppedWeapon(const CTFDroppedWeapon *pWeapon)
{
    g_pSM->LogMessage(myself, "");              //DEBUG
    g_pSM->LogMessage(myself, "");              //DEBUG
    g_pSM->LogMessage(myself, "");              //DEBUG
    g_pSM->LogMessage(myself, "Starting pickup process.");              //DEBUG
    // g_pSM->LogMessage(myself, "DETOUR: PRE   CanPickup");               //DEBUG
    
    //TODO: get classname of pDroppedWeapon with gamehelpers->GetEntityClassname and return false if it's disallowed

    //GetLoadoutSlot is after all the preliminary checks we want to keep. we don't care that it does all that other stuff
    //well use this as a marker to tell if those checks passed
    if (!InitDetour("CTFItemDefinition::GetLoadoutSlot", &g_GetLoadout_hook, (void*)(&CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup))) 
        g_pSM->LogError(myself, "Could not initialize detour_GetLoadoutSlot_CanPickup!");
    
    //call the original
    bool out = g_CanPickup_hook.thiscall<bool>(this, pWeapon);

    
    // g_pSM->LogMessage(myself, "DETOUR: POST  CanPickup");               //DEBUG

    //reset the GetLoadout hook
    g_GetLoadout_hook = {};

    // if (!weGood_CanPickup) g_pSM->LogMessage(myself, "DETOUR: weGood false...");    //DEBUG

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

//FIXME: for some reason this runs twice (four times for PickupWeapon). does it have to do with when and where it's initialized?
//the others don't run twice...
//i've disabled the hook in the function so it doesn't run a second time, but should prob figure out why this happens

int CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup ( int iLoadoutClass ) const
{
    //we've reached the GetLoadoutSlot call without returning, which means we've passed all the basic checks in CanPickup
    //(is alive, isn't taunting, etc.)
    //we also passed the check for if we have an active weapon, which I'm not completely sure if we need... but eh.
    //we dont care what this is doing. we'll overwrite its consequences in the canpickup detour anyways
    // g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_CP");                //DEBUG
    // g_pSM->LogMessage(myself, "DETOUR: Setting weGood");                //DEBUG
    weGood_CanPickup = true;
    
    // if (!g_GetLoadout_hook) g_pSM->LogMessage(myself, "DETOUR: Something's wrong...");                //DEBUG
    
    int out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
    
    g_GetLoadout_hook = {};
    
    return out; 
}

static bool printMyWeaponSlots = false;         //DEBUG
static bool isDroppedWeaponDisallowed = false;


bool CTFPlayerDetours::detour_PickupWeaponFromOther(CTFDroppedWeapon *pDroppedWeapon)
{
    // g_pSM->LogMessage(myself, "DETOUR: PRE   PickupWeapon");            //DEBUG
    
    
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
        
    if (!InitDetour("CBaseCombatCharacter::Weapon_GetSlot", &g_WeaponGetSlot_hook, (void*)(&CBaseCmbtChrDetours::detour_Weapon_GetSlot)))
        g_pSM->LogError(myself, "Could not initialize detour_Weapon_GetSlot!");


    //detour GetEntityForLoadoutSlot so we can drop another class's weapon and Weapon_GetSlot to get that weapon
    //we do this here instead of OnLoad because it turns out GetEnt is called 10 times per player every frame (holy shit?)
    //it's gotta be more performant to create and delete it than to jump to our detour that much
    if (!InitDetour("CTFPlayer::GetEntityForLoadoutSlot", &g_GetEnt_hook, (void*)(&CTFPlayerDetours::detour_GetEntityForLoadoutSlot)))
        g_pSM->LogError(myself, "Could not initialize detour_GetEntityForLoadoutSlot!");
        
    //don't translate the weapon - fixes some weapons not being pickuppable on unintended classes
    if (!InitDetour("TranslateWeaponEntForClass", &g_Translate_hook, (void*)(&detour_TranslateWeaponEntForClass))) 
        g_pSM->LogError(myself, "Could not initialize detour_TranslateWeaponEntForClass!");


    
    //actually pick up the weapon and drop ours
    bool out = g_PickupWeapon_hook.thiscall<bool>(this, pDroppedWeapon);
    
    // g_pSM->LogMessage(myself, "DETOUR: POST  PickupWeapon");            //DEBUG
    

    if (printMyWeaponSlots)
    {
        printMyWeaponSlots = false;
        const char* classname;
        CBaseCombatWeapon* outWeapon;

        if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 0))) g_pSM->LogMessage(myself, "SLOT 0: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 0: NO WEAPON");   //DEBUG
        if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 1))) g_pSM->LogMessage(myself, "SLOT 1: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 1: NO WEAPON");   //DEBUG
        if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 2))) g_pSM->LogMessage(myself, "SLOT 2: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 2: NO WEAPON");   //DEBUG
        if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 3))) g_pSM->LogMessage(myself, "SLOT 3: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 3: NO WEAPON");   //DEBUG
        if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 4))) g_pSM->LogMessage(myself, "SLOT 4: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 4: NO WEAPON");   //DEBUG
        if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 5))) g_pSM->LogMessage(myself, "SLOT 5: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 5: NO WEAPON");   //DEBUG
        if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 6))) g_pSM->LogMessage(myself, "SLOT 6: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 6: NO WEAPON");   //DEBUG
    }

    //remove GetLoadoutSlot detour, dont need it anymore for now (not until the next pickup event)
    if (g_GetLoadout_hook) g_GetLoadout_hook = {};
    
    if (g_GetEnt_hook) g_GetEnt_hook = {};

    if (g_Translate_hook) g_Translate_hook = {};

    if (g_WeaponGetSlot_hook) g_WeaponGetSlot_hook = {};
    
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
    // g_pSM->LogMessage(myself, "DETOUR: PRE   GetLoadout_PW");                //DEBUG
    
    int slotToDrop_PickupWeapon = SLOTTODROP_PW_DEFAULT;
    
    //first get the original result
    int out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
    
    // g_pSM->LogMessage(myself, "DETOUR: POST  GetLoadout_PW");                //DEBUG
    
    g_pSM->LogMessage(myself, "out (me):         %i", out);                       //DEBUG
    
    //would we not be able to pick this up? 
    if (out == -1)
    {
        // get the loadout slot that this weapon would normally go in
        // ... by checking every class until one of them returns something other than -1
        
        //from 0-9 the classes are: Undefined, Scout, Sniper, Soldier, Demo, Medic, Heavy, Pyro, Spy, Engineer
        for (int i = 1; i <= 9; i++)
        {
            int slotForThisClass = g_GetLoadout_hook.thiscall<int>(this, i);
            
            // g_pSM->LogMessage(myself, "Slot for class %i: %i", i, slotForThisClass);    //DEBUG
            
            if (slotForThisClass != -1)
            {
                //the weapon is meant for this class!
                slotToDrop_PickupWeapon = slotForThisClass;

                //the spy is weird (and kinda engi too)
                if (i == 8 || i == 9) printMyWeaponSlots = true;   //DEBUG

                if (i == 8 && slotForThisClass == 1)
                {
                    //this is a secondary on the spy -- it's a revolver!!!
                    //drop the primary instead because it's a secondary that's selected with 1

                    //FIXME: this might cause a lot of problems. we might have nothing in some slot.

                    slotToDrop_PickupWeapon = 0;
                    // printMyWeaponSlots = true;
                }
                else if (i == 8 && slotForThisClass == 2)
                {
                    //this is a melee on the spy -- it's a knife!
                    //the spy can equip some all-class melees, but those are also equippable by the scout, so the for loop would stop at the scout
                    //this would stop the function after GetEnt doesn't find a weapon in slot -1, but we're skipping past that check.
                    //isDroppedWeaponDisallowed will be checked - if its true, don't skip past that return false in PickupWeapon.
                    //result: GetEnt finds nothing and PickupWeapon returns false.

                    //TODO: find WHY this crashes and fix that.
                    //DEBUG: allow knives for now while testing
                    //TODO: move to top of detour_CanPickup

                    // isDroppedWeaponDisallowed = true;
                    // slotToDrop_PickupWeapon = -1;
                }
                break;
            }
            // otherwise move on to the next
        }

        
        g_pSM->LogMessage(myself, "slotToPlace:      %i", slotToDrop_PickupWeapon);   //DEBUG
        
        out = slotToDrop_PickupWeapon;
        //reset it
        slotToDrop_PickupWeapon = SLOTTODROP_PW_DEFAULT;
    }
    
    if (iLoadoutClass == 8) printMyWeaponSlots = true;   //DEBUG

    if (iLoadoutClass == 8 && out == 1)
    {
        //we're a spy and it goes in our secondary slot! it's a revolver!
        //so for some reason, revolvers list themselves as going in our secondary slot, but when added, they aren't in our secondary slot.
        //they're a primary. i guess thats why you press 1 to get them...

        slotToDrop_PickupWeapon = 0;
        out = slotToDrop_PickupWeapon;
        g_pSM->LogMessage(myself, "slotToPlace 2:    %i", slotToDrop_PickupWeapon);   //DEBUG
    }
    // if (iLoadoutClass == 8 && out == 2)
    // {
    //     //we're picking up a melee as the spy! but that doesn't cause a crash, afaik, so it's okay.
    // }


    g_GetLoadout_hook = {};

    return out;
}

CBaseEntity* CTFPlayerDetours::detour_GetEntityForLoadoutSlot( int iLoadoutSlot, bool bForceCheckWearable)
{
    g_pSM->LogMessage(myself, "GetEnt detour called on %i (%s) with %i", gamehelpers->EntityToBCompatRef(this), gamehelpers->GetEntityClassname(this), iLoadoutSlot);    //DEBUG

    CBaseEntity* out = g_GetEnt_hook.thiscall<CBaseEntity*>(this, iLoadoutSlot, bForceCheckWearable);
    
    if (!out)
    {
        g_pSM->LogMessage(myself, "GetEnt failed, falling back to WeaponGetSlot..."); //DEBUG
        
        //it didn't find anything, probably because the slot is filled by another class' weapon.
        out = (CBaseEntity*) g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, iLoadoutSlot);
        if (!out)
        {
            //uh oh
            //... or we're picking up a weapon into a slot that isn't filled
            g_pSM->LogMessage(myself, "Weapon_GetSlot failed to find weapon!"); //DEBUG
        }
    }
    
    g_GetEnt_hook = {};
    
    if (out) g_pSM->LogMessage(myself, "GetEnt output: %i (%s)", gamehelpers->EntityToBCompatRef(out), gamehelpers->GetEntityClassname(out));   //DEBUG
    else g_pSM->LogMessage(myself, "GetEnt output: No entity found!");  //DEBUG

    return out;
}

CBaseCombatWeapon* CBaseCmbtChrDetours::detour_Weapon_GetSlot( int slot ) const
{
    return g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, slot);
}

//don't translate our weapons
const char* detour_TranslateWeaponEntForClass( const char *pszName, int iClass )
{

    if (strcmp(pszName, "tf_weapon_shotgun") == 0)
    {
        //it doesnt know how to make that
        g_pSM->LogMessage(myself, "Translating shotgun to shotgun_soldier..."); //DEBUG
        pszName = "tf_weapon_shotgun_soldier";
    }
    return pszName;

    g_Translate_hook = {};

    // return g_Translate_hook.call<const char*>(pszName, iClass);  //original
}



void Freeguns::SDK_OnUnload()
{
    if (g_CanPickup_hook) g_CanPickup_hook = {};
    if (g_PickupWeapon_hook) g_PickupWeapon_hook = {};
    if (g_GetLoadout_hook) g_GetLoadout_hook = {};
    if (g_WeaponGetSlot_hook) g_WeaponGetSlot_hook = {};
    if (g_Translate_hook) g_Translate_hook = {};
    if (g_GetEnt_hook) g_GetEnt_hook = {};

    if (g_PickupWeapon_mid_hook_1) g_PickupWeapon_mid_hook_1 = {};
    if (g_PickupWeapon_mid_hook_2) g_PickupWeapon_mid_hook_2 = {};
    if (g_PickupWeapon_mid_hook_3) g_PickupWeapon_mid_hook_3 = {};
    if (g_PickupWeapon_mid_hook_4) g_PickupWeapon_mid_hook_4 = {};

}

const sp_nativeinfo_t MyNatives[] = 
{
    // {"enableDetours", EnableDetours},
    // {"disableDetours", DisableDetours},
	{NULL,			NULL},
};

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