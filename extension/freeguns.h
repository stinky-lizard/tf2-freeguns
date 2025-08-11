#ifndef _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_

#include <safetyhook.hpp>

/*
 * Declarations for the Freeguns extension's functionality.
*/

//Re-declarations from hl2sdk-tf2
//this is all kinda messy, but I'm not sure the functions will bind to the originals otherwise

class CEconItemView;    //hoist

class CTFPlayerClassShared
{
public:
    int GetClassIndex( void ) const;
};

class CTFPlayerClass : public CTFPlayerClassShared {};

class CTFDroppedWeapon
{
    public:
    CEconItemView *GetItem();
};

class CTFPlayer
{
    
    public:
    bool CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon );
    bool PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon );
    CTFPlayerClass *GetPlayerClass( void );
    
};

class CTFItemDefinition
{
    
    public:
    int GetLoadoutSlot( int iLoadoutClass ) const;
    int CanBeUsedByClass( int iClass ) const;
    int GetDefaultLoadoutSlot( void ) const;

};

typedef CTFItemDefinition	GameItemDefinition_t;

class CEconItemView
{
    public:
    bool IsValid( void ) const;
    GameItemDefinition_t	*GetStaticData( void ) const;
    
    
};

class CBaseEntity;

//okay were good

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

//Wrapper function to bind the detours

bool InitDetour(const char* gamedata, SafetyHookInline *hookObj, void* callback);

//Other stuff needed

IGameConfig *g_pGameConf = NULL;

    //copied from sdk-hacks.h, basehandle.h, and const.h

#define	MAX_EDICT_BITS				11			// # of bits needed to represent max edicts     //hey this is the object limit! how about that

#define NUM_SERIAL_NUM_BITS		16 // (32 - NUM_ENT_ENTRY_BITS)
#define ENT_ENTRY_MASK			(( 1 << NUM_SERIAL_NUM_BITS) - 1)
#define INVALID_EHANDLE_INDEX	0xFFFFFFFF
#define NUM_ENT_ENTRY_BITS		(MAX_EDICT_BITS + 2)
#define NUM_ENT_ENTRIES			(1 << NUM_ENT_ENTRY_BITS)

class CBaseHandle
{
    //this is uh. commented out in dhooks but idk why.
    //hopefully fine to uncomment
public:
	bool IsValid() const {return m_Index != INVALID_EHANDLE_INDEX;}
	int GetEntryIndex() const
	{
		if ( !IsValid() )
			return NUM_ENT_ENTRIES-1;
		return m_Index & ENT_ENTRY_MASK;
	}
private:
	unsigned long	m_Index;
};


#endif // _INCLUDE_SOURCEMOD_EXTENSION_FREEGUNS_H_