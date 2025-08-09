#ifndef _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_

#include <safetyhook.hpp>

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

SafetyHookInline g_CanPickup_hook{};
SafetyHookInline g_PickupWeapon_hook{};

//Detour functions to bind to the objects

class CTFPlayerDetours : public CTFPlayer
{
    public:
    bool detour_CanPickupDroppedWeapon(const CTFDroppedWeapon *pWeapon);
    bool detour_PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
    
};

//Wrapper function to bind the detours

bool InitDetour(const char* gamedata, SafetyHookInline *hookObj, void* callback);

//Other stuff needed

IGameConfig *g_pGameConf = NULL;


#endif // _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_