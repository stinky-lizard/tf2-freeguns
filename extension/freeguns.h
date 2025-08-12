#ifndef _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_

#include <safetyhook.hpp>

/*
 * Declarations for the Freeguns extension's functionality.
*/

//Re-declarations from hl2sdk-tf2
//this is all kinda messy, but I'm not sure the functions will bind to the originals otherwise

class CEconItemView;    //hoist
class CBaseCombatWeapon;

// class CTFPlayerClassShared
// {
// public:
//     int GetClassIndex( void ) const;
// };

// class CTFPlayerClass : public CTFPlayerClassShared {};


class CTFDroppedWeapon;

class CBaseEntity 
{
    
};
    
class CBaseCombatCharacter : public CBaseEntity
{
public:
    CBaseCombatWeapon* Weapon_GetSlot( int slot ) const;
};

class CTFPlayer : public CBaseCombatCharacter
{
    
    public:
    bool CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
    bool PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
    CBaseEntity *GetEntityForLoadoutSlot( int iLoadoutSlot, bool bForceCheckWearable = false );	
};

class CTFItemDefinition
{
    
    public:
    int GetLoadoutSlot( int iLoadoutClass ) const;
    // int CanBeUsedByClass( int iClass ) const;
    // int GetDefaultLoadoutSlot( void ) const;

};

// typedef CTFItemDefinition	GameItemDefinition_t;


// class CEconItemView
// {
//     public:
//     bool IsValid( void ) const;
//     GameItemDefinition_t	*GetStaticData( void ) const;
    
    
// };


//okay were good

//Safetyhook hook/detour objects

SafetyHookInline g_CanPickup_hook{};
SafetyHookInline g_PickupWeapon_hook{};
SafetyHookInline g_GetLoadout_hook{};
SafetyHookInline g_WeaponGetSlot_hook{};
SafetyHookInline g_GetEnt_hook{};
SafetyHookInline g_Translate_hook{};

//Detour functions to bind to the objects

class CTFPlayerDetours : public CTFPlayer
{

public:
    bool detour_CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
    bool detour_PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
    CBaseEntity* detour_GetEntityForLoadoutSlot( int iLoadoutSlot, bool bForceCheckWearable = false );	
    
};
static bool getEntDetourEnabled = false;

class CTFItemDefDetours : CTFItemDefinition
{

public:
    int detour_GetLoadoutSlot_CanPickup ( int iLoadoutClass ) const; //might as well make this const too
    int detour_GetLoadoutSlot_PickupWeapon ( int iLoadoutClass ) const; 

};

class CBaseCmbtChrDetours : CBaseCombatCharacter
{
public:
    CBaseCombatWeapon* detour_Weapon_GetSlot( int slot ) const;
};

extern const char* detour_TranslateWeaponEntForClass( const char *pszName, int iClass );
static bool translateDetourEnabled = false;

//Wrapper function to bind the detours

bool InitDetour(const char* gamedata, SafetyHookInline *hookObj, void* callback);

//Other stuff needed

IGameConfig *g_pGameConf = NULL;

bool FreegunsEnabled = true;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H
