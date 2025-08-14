
#include <extension.h>
#include <freeguns.h>
#include <detours.h>


bool InitAllDetours()
{
    
    if (!InitDetour("CTFPlayer::CanPickupDroppedWeapon", &g_CanPickup_hook, (void*)(&CTFPlayerDetours::detour_CanPickupDroppedWeapon))) return false;
    
    if (!InitDetour("CTFPlayer::PickupWeaponFromOther", &g_PickupWeapon_hook, (void*)(&CTFPlayerDetours::detour_PickupWeaponFromOther))) return false;

    // if (!InitDetour("CBaseCombatCharacter::Weapon_GetSlot", &g_WeaponGetSlot_hook, (void*)(&CBaseCmbtChrDetours::detour_Weapon_GetSlot))) return false;
    
    //do this in PickupWeapon
    // if (!InitDetour("CTFPlayer::GetEntityForLoadoutSlot", &g_GetEnt_hook, (void*)(&CTFPlayerDetours::detour_GetEntityForLoadoutSlot))) return false;
    
    //also this
    // if (!InitDetour("TranslateWeaponEntForClass", &g_Translate_hook, (void*)(&detour_TranslateWeaponEntForClass))) return false;
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
    // g_pSM->LogMessage(myself, "Got sig for %s", gamedata);               //DEBUG
    
    *hookObj = safetyhook::create_inline(pAddress, callback);
    
    // g_pSM->LogMessage(myself, "Created InlineHook for %s", gamedata);    //DEBUG
    
    return true;
}


bool DeleteAllDetours()
{
    if (g_CanPickup_hook) g_CanPickup_hook = {};
    if (g_PickupWeapon_hook) g_PickupWeapon_hook = {};
    if (g_GetLoadout_hook) g_GetLoadout_hook = {};
    if (g_WeaponGetSlot_hook) g_WeaponGetSlot_hook = {};
    if (g_Translate_hook) g_Translate_hook = {};
    if (g_GetEnt_hook) g_GetEnt_hook = {};

    return true;
}

static bool weGood_CanPickup = false;

bool CTFPlayerDetours::detour_CanPickupDroppedWeapon(const CTFDroppedWeapon *pWeapon)
{
    // g_pSM->LogMessage(myself, "");              //DEBUG
    // g_pSM->LogMessage(myself, "");              //DEBUG
    // g_pSM->LogMessage(myself, "");              //DEBUG
    // g_pSM->LogMessage(myself, "Starting pickup process.");              //DEBUG
    // g_pSM->LogMessage(myself, "DETOUR: PRE   CanPickup");               //DEBUG
    
    //TODO: get classname of pDroppedWeapon with gamehelpers->GetEntityClassname and return false if it's disallowed
    
    //GetLoadoutSlot is after all the preliminary checks we want to keep. we don't care that it does all that other stuff
    //well use this as a marker to tell if those checks passed
    if (!InitDetour("CTFItemDefinition::GetLoadoutSlot", &g_GetLoadout_hook, (void*)(&CTFItemDefDetours::detour_GetLoadoutSlot_CanPickup))) 
    g_pSM->LogError(myself, "Could not initialize detour_GetLoadoutSlot_CanPickup!");
    
    //call the original
    if (!g_CanPickup_hook) return false;
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

bool CTFItemDefDetours::IsDroppedWeaponAllowed(int myClass) const
{   
    //class 8 is spy, slot 2 is melee. would this weapon be a melee on spy (i.e. is it a knife?)
    if (!g_GetLoadout_hook) return true;
    if (g_GetLoadout_hook.thiscall<int>(this, 8) == 2)
    {
        //would this weapon not be equipable by other classes (i.e. is it not an allclass?)
        //(spy can equip a couple allclass weapons as knives)
        if (g_GetLoadout_hook.thiscall<int>(this, 1) == -1)
        {
            //it's a knife
            //if we're spy, it's fine
            //if not, it's disallowed
            // g_pSM->LogMessage(myself, "Weapon is a knife, return %s!", myClass == 8 ? "true" : "false"); //DEBUG
            
            // return myClass == 8;
            return true;   //DEBUG_TOREMOVE
        }
    }
    //TODO: do we want to disable sappers and buildings?
    
    return true;
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
    
    if (!(this->IsDroppedWeaponAllowed(iLoadoutClass))) weGood_CanPickup = false;
    
    // if (!g_GetLoadout_hook) g_pSM->LogMessage(myself, "DETOUR: Something's wrong...");                //DEBUG
    
    if (!g_GetLoadout_hook) return -1;
    int out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
    
    g_GetLoadout_hook = {};
    
    return out; 
}

// static bool printMyWeaponSlots = false;         //DEBUG
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
    if (!g_PickupWeapon_hook) return false;
    bool out = g_PickupWeapon_hook.thiscall<bool>(this, pDroppedWeapon);
    
    // g_pSM->LogMessage(myself, "DETOUR: POST  PickupWeapon");            //DEBUG
    

    // if (printMyWeaponSlots)
    // {
    //     printMyWeaponSlots = false;
    //     const char* classname;
    //     CBaseCombatWeapon* outWeapon;

    //     if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 0))) g_pSM->LogMessage(myself, "SLOT 0: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 0: NO WEAPON");   //DEBUG
    //     if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 1))) g_pSM->LogMessage(myself, "SLOT 1: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 1: NO WEAPON");   //DEBUG
    //     if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 2))) g_pSM->LogMessage(myself, "SLOT 2: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 2: NO WEAPON");   //DEBUG
    //     if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 3))) g_pSM->LogMessage(myself, "SLOT 3: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 3: NO WEAPON");   //DEBUG
    //     if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 4))) g_pSM->LogMessage(myself, "SLOT 4: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 4: NO WEAPON");   //DEBUG
    //     if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 5))) g_pSM->LogMessage(myself, "SLOT 5: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 5: NO WEAPON");   //DEBUG
    //     if ((outWeapon = g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, 6))) g_pSM->LogMessage(myself, "SLOT 6: %s", gamehelpers->GetEntityClassname(outWeapon)); else g_pSM->LogMessage(myself, "SLOT 6: NO WEAPON");   //DEBUG
    // }

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
    if (!g_GetLoadout_hook) return -1;
    int out = g_GetLoadout_hook.thiscall<int>(this, iLoadoutClass);
    
    // g_pSM->LogMessage(myself, "DETOUR: POST  GetLoadout_PW");                //DEBUG
    
    // g_pSM->LogMessage(myself, "out (me):         %i", out);                       //DEBUG
    
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
                // if (i == 8 || i == 9) printMyWeaponSlots = true;   //DEBUG

                if (i == 8 && slotForThisClass == 1)
                {
                    //this is a secondary on the spy -- it's a revolver!!!
                    //drop the primary instead because it's a secondary that's selected with 1

                    //FIXME: this might cause a lot of problems. we might have nothing in some slot.

                    slotToDrop_PickupWeapon = 0;
                    // printMyWeaponSlots = true;
                }
                break;
            }
            // otherwise move on to the next
        }

        
        // g_pSM->LogMessage(myself, "slotToPlace:      %i", slotToDrop_PickupWeapon);   //DEBUG
        
        out = slotToDrop_PickupWeapon;
        //reset it
        slotToDrop_PickupWeapon = SLOTTODROP_PW_DEFAULT;
    }
    
    // if (iLoadoutClass == 8) printMyWeaponSlots = true;   //DEBUG

    if (iLoadoutClass == 8 && out == 1)
    {
        //we're a spy and it goes in our secondary slot! it's a revolver!
        //so for some reason, revolvers list themselves as going in our secondary slot, but when added, they aren't in our secondary slot.
        //they're a primary. i guess thats why you press 1 to get them...

        slotToDrop_PickupWeapon = 0;
        out = slotToDrop_PickupWeapon;
        // g_pSM->LogMessage(myself, "slotToPlace 2:    %i", slotToDrop_PickupWeapon);   //DEBUG
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
    // g_pSM->LogMessage(myself, "GetEnt detour called on %i (%s) with %i", gamehelpers->EntityToBCompatRef(this), gamehelpers->GetEntityClassname(this), iLoadoutSlot);    //DEBUG

    if (!g_GetEnt_hook) return nullptr;
    CBaseEntity* out = g_GetEnt_hook.thiscall<CBaseEntity*>(this, iLoadoutSlot, bForceCheckWearable);
    
    if (!out)
    {
        // g_pSM->LogMessage(myself, "GetEnt failed, falling back to WeaponGetSlot..."); //DEBUG
        
        //it didn't find anything, probably because the slot is filled by another class' weapon.
        if (!g_WeaponGetSlot_hook) return nullptr;
        out = (CBaseEntity*) g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, iLoadoutSlot);
        // if (!out)
        // {
        //     //uh oh
        //     //... or we're picking up a weapon into a slot that isn't filled
        //     g_pSM->LogMessage(myself, "Weapon_GetSlot failed to find weapon!"); //DEBUG
        // }
    }
    
    g_GetEnt_hook = {};
    
    // if (out) g_pSM->LogMessage(myself, "GetEnt output: %i (%s)", gamehelpers->EntityToBCompatRef(out), gamehelpers->GetEntityClassname(out));   //DEBUG
    // else g_pSM->LogMessage(myself, "GetEnt output: No entity found!");  //DEBUG

    return out;
}

CBaseCombatWeapon* CBaseCmbtChrDetours::detour_Weapon_GetSlot( int slot ) const
{
    if (!g_WeaponGetSlot_hook) return nullptr;
    return g_WeaponGetSlot_hook.thiscall<CBaseCombatWeapon*>(this, slot);
}

//don't translate our weapons
const char* detour_TranslateWeaponEntForClass( const char *pszName, int iClass )
{

    if (strcmp(pszName, "tf_weapon_shotgun") == 0)
    {
        //it doesnt know how to make that
        // g_pSM->LogMessage(myself, "Translating shotgun to shotgun_soldier..."); //DEBUG
        pszName = "tf_weapon_shotgun_soldier";
    }
    return pszName;

    g_Translate_hook = {};

    // return g_Translate_hook.call<const char*>(pszName, iClass);  //original
}
