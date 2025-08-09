#ifndef _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_

#include <safetyhook.hpp>
#include <Zydis.h>

/*
 * Declarations for the Freeguns extension's functionality.
*/

//Re-declarations from tf_player.h

class CTFDroppedWeapon;

class CTFPlayer
{
    public:
    bool CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
    bool PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
};

//Safetyhook hook/detour objects

SafetyHookMid g_CanPickup_hook_1{};
SafetyHookMid g_CanPickup_hook_2{};
SafetyHookInline g_PickupWeapon_hook{};

//Detour functions to bind to the objects

//In class for extending CTFPlayer and (this)
class CTFPlayerDetours : public CTFPlayer
{
    public:
    bool detour_PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
};

//Out of class bc doesn't need it (and create_mid wants it)
void patch_CanPickupDroppedWeapon_1(SafetyHookContext& ctx);
void patch_CanPickupDroppedWeapon_2(SafetyHookContext& ctx);

//Wrapper functions to bind the detours

bool InitCanPickupPatch();
bool InitPickupWeaponDetour();

//Other stuff needed

IGameConfig *g_pGameConf = NULL;


#endif // _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_
