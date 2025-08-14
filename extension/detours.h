#ifndef _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_DETOURS_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_DETOURS_H_



#include <safetyhook.hpp>


/*
 * Declarations (and some definitions) for detours.cpp.  
 */


//Re-declarations from hl2sdk-tf2
//this is all kinda messy, but I'm not sure the functions will bind to the originals otherwise

class CEconItemView;    //hoist


// class CTFPlayerClassShared
// {
// public:
//     int GetClassIndex( void ) const;
// };

// class CTFPlayerClass : public CTFPlayerClassShared {};


class CBaseEntity {};

class CTFDroppedWeapon : public CBaseEntity {};

class CBaseCombatWeapon : public CBaseEntity {};

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

class CTFItemDefDetours : CTFItemDefinition
{

public:
    int detour_GetLoadoutSlot_CanPickup ( int iLoadoutClass ) const; //might as well make this const too
    int detour_GetLoadoutSlot_PickupWeapon ( int iLoadoutClass ) const; 
 
    //Returns true if the weapon is allowed to be picked up.
    //Returns false if it's disabled; e.g. it causes crashes.
    bool IsDroppedWeaponAllowed(int myClass) const;

};

class CBaseCmbtChrDetours : CBaseCombatCharacter
{
public:
    CBaseCombatWeapon* detour_Weapon_GetSlot( int slot ) const;
};

extern const char* detour_TranslateWeaponEntForClass( const char *pszName, int iClass );



bool InitDetour(const char* gamedata, SafetyHookInline *hookObj, void* callback);


#endif  //_INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_DETOURS_H_
