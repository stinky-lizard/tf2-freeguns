#pragma semicolon 1
#pragma newdecls required
#include <sdktools>
#include <tf2_stocks>

#include <sdkhooks>

//no longer works in x64?
//#include <tf_econ_data>

#define PLUGIN_VERSION "2.0"

public Plugin myinfo =
{
	name = "[TF2] Freeguns",
	author = "Stinky Lizard",
	description = "Kill your enemy. Steal their stuff. No class restrictions. Change settings with the sm_freeguns cvars.",
	version = PLUGIN_VERSION,
	url = "github.com/stinky-lizard/tf2-freeguns"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	EngineVersion g_engineversion = GetEngineVersion();
	if (g_engineversion != Engine_TF2)
	{
		SetFailState("Freeguns was made for use with Team Fortress 2 only.");
		return APLRes_Failure;
	}
	return APLRes_Success;
}

Handle hSDKCallGetBaseEntity;
Handle hSDKCallUpdateHands;

KeyValues savedData;

ConVar enabledVar;

#include <freeguns_hud>
#include <freeguns>
// #define DEBUG

public void OnEntityCreated(int entity, const char[] classname)
{

	#if defined __freeguns_hud_included
		HudOnEntityCreated(entity, classname);
	#endif
}

public void OnPluginStart()
{

	CreateConVar("sm_freeguns_version", PLUGIN_VERSION, "Standard plugin version ConVar. Please don't change me!", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
	enabledVar = CreateConVar("sm_freeguns_enabled", "0", "Enable/disable Freeguns. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);

	RegConsoleCmd("sm_actionbutton", OnActionButtonCmd, "Print how to find your action button binding.");
	
	#if defined __freeguns_hud_included
		hudVar = CreateConVar("sm_freeguns_hud", "1", "Enable/disable custom weapon HUD element. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);
		LoadTranslations("freeguns.phrases");
	#endif

	GameData hGameConf = new GameData("tf2.freeguns");

	if(hGameConf == INVALID_HANDLE) SetFailState("FreeGuns: Gamedata not found!");

	// StartPrepSDKCall(SDKCall_Raw);
	// PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBaseEntity::GetBaseEntity");
	// PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	// hSDKCallGetBaseEntity = EndPrepSDKCall();
	// if (!hSDKCallGetBaseEntity) SetFailState("Failed to setup SDKCall for GetBaseEntity. (Error code 101)");

	// StartPrepSDKCall(SDKCall_Entity);
	// PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CTFWeaponBase::UpdateHands");
	// hSDKCallUpdateHands = EndPrepSDKCall();
	// if (!hSDKCallUpdateHands) SetFailState("Failed to setup SDKCall for UpdateHands. (Error code 102)");

	//TODO: make detours extension

	savedData = new KeyValues("SavedData");
	if (!savedData) SetFailState("Failed to set up SavedData (Error code 40)");

	enabledVar.AddChangeHook(EnabledVarChanged);
}

Action OnActionButtonCmd(int client, int args)
{
	if (GetUserAdmin(client) == INVALID_ADMIN_ID)
		ReplyToCommand(client, "Run in your console `key_findbinding +use_action_slot_item` to find your Action Button!");
	else PrintToChatAll("Run in your console `key_findbinding +use_action_slot_item` to find your Action Button!");
	return Plugin_Handled;
}

void EnabledVarChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (StringToInt(oldValue) == 0 && StringToInt(newValue) != 0)
	{
		//it was false, now it's true
		LogMessage("Enabling Freeguns.");
		// EnableDetours();
	}
	else if (StringToInt(oldValue) != 0 && StringToInt(newValue) == 0)
	{
		//it was true, now it's false
		LogMessage("Disabling Freeguns.");
		// DisableDetours();
	}
}

int GetEntityFromAddress(Address pEntity)
{
	return SDKCall(hSDKCallGetBaseEntity, pEntity);
}

//get the class for the weapon from the item schema, and save that and the current class to savedData
void SaveClasses(int client, int weapon, bool disabled = false)
{
	TFClassType curClass = TF2_GetPlayerClass(client);
	TFClassType desiredClass;

	int loadoutSlotUsedByPickup;

	if (disabled)
		desiredClass = curClass;
	else
		desiredClass = GetClassOfDroppedWeapon(weapon, loadoutSlotUsedByPickup, curClass);

	char userIdStr[32];
	IntToString(GetClientUserId(client), userIdStr, sizeof userIdStr);

	savedData.JumpToKey("Classes", true);
	savedData.JumpToKey(userIdStr, true);
	savedData.SetNum("CurrentClass", view_as<int>(curClass));
	savedData.SetNum("DesiredClass", view_as<int>(desiredClass));

	//finally, we need to save the class of the weapon we currently have (in the relevant slot, not the one that's out), since that might not necessarily be our player class
	//if we have a foreign weapon, GetEntityForLoadoutSlot won't find a weapon that matches our class (because we don't have one).

	int unneeded;
	int weaponSlotUsedByPickup = GetWeaponSlotFromLoadoutSlot(loadoutSlotUsedByPickup);

	int weaponToDrop = GetPlayerWeaponSlot(client, weaponSlotUsedByPickup);
	if (weaponToDrop != -1)
		savedData.SetNum("CurrentWeaponClass", view_as<int>(GetClassOfDroppedWeapon(weaponToDrop, unneeded, curClass)));
	#if defined DEBUG
		else
			//they don't have a weapon in this slot, so they don't have a weapon to drop
			PrintToServer("No weapon in slot %i", weaponSlotUsedByPickup);
			//CurrentWeaponClass will be set to 0, which technically IS a class, it's just TFClass_Unknown
			//since no item is allowed on TFClass_Unknown, the pickup will be stopped by GetEntityForLoadoutSlot (since the class doesn't match)
			//which I guess is the best possible outcome. It works intuitively (GetEntityForLoadoutSlot stops the process because there is no entity!)
	#endif


	savedData.Rewind();
}

int GetWeaponSlotFromLoadoutSlot(int loadoutSlot)
{
	int weaponSlot = 99; //in case this is called on something other than these

	//have to translate the relevant LOADOUT slot to the relevant WEAPON slot. They are not defined the same way, and some values match differently.
	//(i.e. the pda's, sappers, and revolver)
	//Since we're only dealing with weapons you can pick up, we only really need to deal with the three main weapon slots.

	char weaponSlotName[64];
	//TF2Econ_TranslateLoadoutSlotIndexToName(loadoutSlot, weaponSlotName, sizeof weaponSlotName);
	if (StrEqual(weaponSlotName, "primary")) weaponSlot = TFWeaponSlot_Primary;
	if (StrEqual(weaponSlotName, "secondary")) weaponSlot = TFWeaponSlot_Secondary;
	if (StrEqual(weaponSlotName, "melee")) weaponSlot = TFWeaponSlot_Melee;
	if (StrEqual(weaponSlotName, "building")) weaponSlot = TFWeaponSlot_Building;

	#if defined DEBUG
		PrintToServer("LoadoutSlot: %i", loadoutSlot);
		PrintToServer("WeaponSlot: %i", weaponSlot);
		PrintToServer("WeaponSlotName: %s", weaponSlotName);
	#endif

	return weaponSlot;
}

TFClassType GetClassOfDroppedWeapon(int weapon, int& loadoutSlot, TFClassType currentClass)
{
	TFClassType out;

	//get the item's definition index
	int iWeaponDef = GetEntProp(weapon, Prop_Send, "m_iItemDefinitionIndex");

	// if (currentClass != TFClass_Unknown)
	// {
	// 	int tempSlot = TF2Econ_GetItemLoadoutSlot(iWeaponDef, currentClass);
	// 	if (tempSlot != -1)
	// 	{
	// 		//the weapon works with their current class
	// 		loadoutSlot = tempSlot;
	// 		return currentClass;
	// 	}
	//}

	//check each class for if this weapon supports it
	//Civilian/Unknown left out bc i dont know if that actually works ingame
	TFClassType classes[] = {TFClass_Scout, TFClass_Sniper, TFClass_Soldier, TFClass_DemoMan, TFClass_Medic, TFClass_Heavy, TFClass_Pyro, TFClass_Spy, TFClass_Engineer};
	// for (int i = 0; i < sizeof classes; i++)
	// {
	// 	int tempSlot = TF2Econ_GetItemLoadoutSlot(iWeaponDef, classes[i]);
	// 	if (tempSlot != -1)
	// 	{
	// 		//there's a slot defined for this class, so it's meant to be for this class
	// 		out = classes[i];
	// 		loadoutSlot = tempSlot;
	// 		break;
	// 	}
	// }
	return out;
}

//Accepts only "CurrentClass", "CurrentWeaponClass", or "DesiredClass".
TFClassType GetSavedClass(int client, const char[] key)
{
	char userIdStr[32];
	IntToString(GetClientUserId(client), userIdStr, sizeof userIdStr);

	savedData.JumpToKey("Classes");
	savedData.JumpToKey(userIdStr);
	TFClassType out = view_as<TFClassType>(savedData.GetNum(key));
	savedData.Rewind();
	return out;
}

bool DroppedWeaponIsDisabled(int droppedWeaponEnt)
{
	if (!IsValidEntity(droppedWeaponEnt)) return false;
	char classname[64];
	GetEntityClassname(droppedWeaponEnt, classname, sizeof classname);
	if (!StrEqual(classname, "tf_dropped_weapon")) return false;

	int weaponItemDefinitionIndex = GetEntProp(droppedWeaponEnt, Prop_Send, "m_iItemDefinitionIndex");
	char weaponItemDefClassname[64];
	//TF2Econ_GetItemClassName(weaponItemDefinitionIndex, weaponItemDefClassname, sizeof weaponItemDefClassname);
	if
	(
		StrEqual(weaponItemDefClassname, "tf_weapon_revolver")
		|| StrEqual(weaponItemDefClassname, "tf_weapon_sapper")
		|| StrEqual(weaponItemDefClassname, "tf_weapon_builder")
		|| StrEqual(weaponItemDefClassname, "tf_weapon_knife")
	)
	{
		//revolver, sapper, and builder are disabled
		return true;
	}
	return false;
}

bool DoesClientHaveWeaponToDrop(int client, int droppedWeaponEnt)
{
	if (!IsValidEntity(droppedWeaponEnt)) return false;
	char classname[64];
	GetEntityClassname(droppedWeaponEnt, classname, sizeof classname);
	if (!StrEqual(classname, "tf_dropped_weapon")) return false;

	int loadoutSlot;
	GetClassOfDroppedWeapon(droppedWeaponEnt, loadoutSlot, TF2_GetPlayerClass(client));
	int weaponSlot = GetWeaponSlotFromLoadoutSlot(loadoutSlot);


	//todo fuck. have to use GetEntityForLoadoutSlot
	// int weapon = GetPlayerWeaponSlot(client, weaponSlot);
	// char classname2[64];
	// if ()
	// GetEntityClassname(weapon, classname2, 64);
	// PrintToChatAll("%i : %s", weaponSlot, classname2);

	return GetPlayerWeaponSlot(client, weaponSlot) != -1;
}