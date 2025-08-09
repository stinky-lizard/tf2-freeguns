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
    
    //patch CanPickup
    if (!InitCanPickupPatch()) return false;
    
    //init the detour for pickupweapon in which we will patch it
    if(!InitPickupWeaponDetour()) return false;

    return true;
}


//Iniitialize detours
bool InitCanPickupPatch()
{
    ZydisDecoder decoder{};

#if SAFETYHOOK_ARCH_X86_64
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#elif SAFETYHOOK_ARCH_X86_32
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif

    auto instructionPointer = (void*)(&CTFPlayer::CanPickupDroppedWeapon);

    // while (*instructionPointer != 0xC3) {
    //     ZydisDecodedInstruction ix{};

    //     ZydisDecoderDecodeInstruction(&decoder, nullptr, reinterpret_cast<void*>(instructionPointer), 15, &ix);

    //     // Follow JMPs
    //     if (ix.opcode == 0xE9) {
    //         instructionPointer += ix.length + (int32_t)ix.raw.imm[0].value.s;
    //     } else {
    //         instructionPointer += ix.length;
    //     }
    // }
    instructionPointer += 0xB3; //switch instruction pointer to just after GetActiveWeapon assert and check

    g_CanPickup_hook_1 = safetyhook::create_mid(instructionPointer, patch_CanPickupDroppedWeapon_1);
    
    instructionPointer += 0xA7; //"jump" to end of function

    g_CanPickup_hook_2 = safetyhook::create_mid(instructionPointer, patch_CanPickupDroppedWeapon_2);

    return true;
}

static bool we_good_CanPickup = false;

void patch_CanPickupDroppedWeapon_1(SafetyHookContext& ctx)
{
    //this will run just after GetActiveWeapon... I hope
    we_good_CanPickup = true;
}

void patch_CanPickupDroppedWeapon_2(SafetyHookContext& ctx)
{
    if (we_good_CanPickup)
    {
        g_pSM->LogMessage(myself, "we good");

        #if SAFETYHOOK_OS_WINDOWS
            #if SAFETYHOOK_ARCH_X86_64
                g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);
                ctx.rax = 1;
            #elif SAFETYHOOK_ARCH_X86_32
                g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);
                ctx.eax = 1;
            #endif
        #elif SAFETYHOOK_OS_LINUX
            #if SAFETYHOOK_ARCH_X86_64
                g_pSM->LogMessage(myself, "64 bits. rax = %i", ctx.rax);
                ctx.rax = 1;
            #elif SAFETYHOOK_ARCH_X86_32
                g_pSM->LogMessage(myself, "32 bits. eax = %i", ctx.eax);
                ctx.eax = 1;
            #endif
        #endif
    }
    else g_pSM->LogMessage(myself, "we not good");

}


bool InitPickupWeaponDetour()
{
    const char* gamedata = "CTFPlayer::PickupWeaponFromOther";
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
    g_pSM->LogMessage(myself, "Got sig for %s", gamedata);
    
    void *callback = (void*)(&CTFPlayerDetours::detour_PickupWeaponFromOther);
    
    g_PickupWeapon_hook = safetyhook::create_inline(pAddress, callback);

    g_pSM->LogMessage(myself, "Created InlineHook for %s", gamedata);

    return true;
}


bool CTFPlayerDetours::detour_PickupWeaponFromOther(CTFDroppedWeapon *pDroppedWeapon)
{
    g_pSM->LogMessage(myself, "DETOUR: PRE   PickupWeapon");

    bool out = g_PickupWeapon_hook.thiscall<bool>(this, pDroppedWeapon);
    
    g_pSM->LogMessage(myself, "DETOUR: POST  PickupWeapon");
    
    return out;
}


void Freeguns::SDK_OnUnload()
{
    g_CanPickup_hook_1 = {};
    g_CanPickup_hook_2 = {};
    g_PickupWeapon_hook = {};
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
