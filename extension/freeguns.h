#ifndef _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_

#include <safetyhook.hpp>
#include <IBinTools.h>
#include <sm_argbuffer.h>

/*
 * Declarations for the Freeguns extension's functionality.
*/

//Re-declarations from hl2sdk-tf2

/*
 * FOR REFERENCE! //DEBUG
*/

class CEconItemView;    //hoist

class CTFPlayerClassShared
{
// public:
//     int GetClassIndex( void ) const;
};

class CTFPlayerClass : public CTFPlayerClassShared {};

class CTFDroppedWeapon
{
    // public:
    // CEconItemView *GetItem();
};

class CTFPlayer
{
    
    public:
    bool CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
    bool PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
    // CTFPlayerClass *GetPlayerClass( void );
    
};

class CTFItemDefinition
{
    
    public:
    int GetLoadoutSlot( int iLoadoutClass ) const;
    // int CanBeUsedByClass( int iClass ) const;
    // int GetDefaultLoadoutSlot( void ) const;

};

typedef CTFItemDefinition	GameItemDefinition_t;

class CEconItemView
{
    // public:
    // bool IsValid( void ) const;
    // GameItemDefinition_t	*GetStaticData( void ) const;
    
    
};

//okay were good

class CTFDroppedWeapon;

class CTFPlayer
{
    
    public:
    bool CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
    bool PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
    
};

class CTFItemDefinition
{
    
    public:
    int GetLoadoutSlot( int iLoadoutClass ) const;

};

//Safetyhook hook/detour objects

SafetyHookInline g_CanPickup_hook{};
SafetyHookInline g_PickupWeapon_hook{};
SafetyHookInline g_GetLoadout_hook{};

//Detour functions to bind to the objects

class CTFPlayerDetours : public CTFPlayer
{

public:
    bool detour_CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
    bool detour_PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
    
};

class CTFItemDefDetours : CTFItemDefinition
{

public:
    int detour_GetLoadoutSlot_CanPickup ( int iLoadoutClass ) const; //might as well make this const too
    int detour_GetLoadoutSlot_PickupWeapon ( int iLoadoutClass ) const; 

};


//sdk calls

class CallWrappers
{
    public:
    static ICallWrapper *GetClassIndex;
    static ICallWrapper *GetItem;
    static ICallWrapper *GetPlayerClass;
    static ICallWrapper *CanBeUsedByClass;
    static ICallWrapper *GetDefaultLoadoutSlot;
    static ICallWrapper *IsValid;
    static ICallWrapper *GetStaticData;
    
    static bool InitCalls();

    static bool wrappersInitialized;
};

//functions to bind the detours

bool InitDetour(const char* gamedata, SafetyHookInline *hookObj, void* callback);


//Other stuff needed

IGameConfig *g_pGameConf = NULL;
IBinTools *g_pBinTools = NULL;



#endif // _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_