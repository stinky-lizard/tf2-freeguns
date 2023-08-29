#pragma semicolon 1
#pragma newdecls required
#include <sdktools>
#include <dhooks>
#include <tf2_stocks>

#include <sdkhooks>

#include <tf_econ_data>

#define PLUGIN_VERSION "1.2.7-beta1"

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

DynamicDetour hCanPickupDroppedWeaponDetour;
DynamicDetour hPickupWeaponFromOtherDetour;
DynamicDetour hGetEntityForLoadoutSlot;
Handle hSDKCallGetBaseEntity;
Handle hSDKCallUpdateHands;

KeyValues savedData;

ConVar enabledVar;

#include <freeguns_glow>
#include <freeguns_hud>
// #include <freeguns_model>
#define DEBUG

public void OnEntityCreated(int entity, const char[] classname)
{

	#if defined __freeguns_glow_included
		GlowOnEntityCreated(entity, classname);
	#endif
	#if defined __freeguns_hud_included
		HudOnEntityCreated(entity, classname);
	#endif
}

public void OnPluginStart()
{

	CreateConVar("sm_freeguns_version", PLUGIN_VERSION, "Standard plugin version ConVar. Please don't change me!", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
	enabledVar = CreateConVar("sm_freeguns_enabled", "1", "Enable/disable Freeguns. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);

	RegConsoleCmd("sm_actionbutton", OnActionButtonCmd, "Print how to find your action button binding.");

	#if defined __freeguns_glow_included
		glowVar = CreateConVar("sm_freeguns_glow", "1", "Enable/disable custom weapon glow. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);
		glowLookVar = CreateConVar("sm_freeguns_glow_lookonly", "1", "Only glow when looking directly at a weapon.", FCVAR_REPLICATED);
		glowTimerVar = CreateConVar("sm_freeguns_glow_timer", "0.3", "How often (in seconds) to update weapon glows. Make smaller to make it look nicer, or make larger to help with performance.", FCVAR_REPLICATED, true, 0.1);
		glowRadVar = CreateConVar("sm_freeguns_glow_radius", "500", "How far (in units) dropped weapons should glow for players. Increase this to convey more info to the player, or reduce to potentially reduce visual noise/clutter.", FCVAR_REPLICATED);
		allGlowEntities = new ArrayList(sizeof GlowEntity);
	#endif

	#if defined __freeguns_hud_included
		hudVar = CreateConVar("sm_freeguns_hud", "1", "Enable/disable custom weapon HUD element. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);
		LoadTranslations("freeguns.phrases");
	#endif

	#if defined __freeguns_model_included
		modelVar = CreateConVar("sm_freeguns_model", "1", "Enable/disable model switching when changing weapons. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);
		for (int i = 1; i < MaxClients; i++)
		{
			if (!IsClientInGame(i)) continue;
			SDKHook(i, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
			SDKHook(i, SDKHook_WeaponSwitchPost, OnClientWeaponSwitchPost);
			SetWearable(i);
		}
	#endif

	HookEvent("post_inventory_application", OnInventoryApplied);

	GameData hGameConf = new GameData("tf2.freeguns");

	if(hGameConf == INVALID_HANDLE) SetFailState("FreeGuns: Gamedata not found!");

	StartPrepSDKCall(SDKCall_Raw);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBaseEntity::GetBaseEntity");
	PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	hSDKCallGetBaseEntity = EndPrepSDKCall();
	if (!hSDKCallGetBaseEntity) SetFailState("Failed to setup SDKCall for GetBaseEntity. (Error code 101)");

	StartPrepSDKCall(SDKCall_Entity);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CTFWeaponBase::UpdateHands");
	hSDKCallUpdateHands = EndPrepSDKCall();
	if (!hSDKCallUpdateHands) SetFailState("Failed to setup SDKCall for UpdateHands. (Error code 102)");

	//TODO: update function signatures and see if they can be made better

	hCanPickupDroppedWeaponDetour = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_Bool, ThisPointer_CBaseEntity);
	if (!hCanPickupDroppedWeaponDetour) SetFailState("Failed to setup detour for CanPickup. (Error code 11)");
	if (!hCanPickupDroppedWeaponDetour.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::CanPickupDroppedWeapon")) SetFailState("Failed to set signature for CanPickup detour. (Error code 12)");
	if (!hCanPickupDroppedWeaponDetour.AddParam(HookParamType_ObjectPtr)) SetFailState("Failed to add param to CanPickup detour. (Error code 13)");

	hPickupWeaponFromOtherDetour = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_Bool, ThisPointer_CBaseEntity);
	if (!hPickupWeaponFromOtherDetour) SetFailState("Failed to setup detour for PickupWeapon. (Error code 21)");
	if (!hPickupWeaponFromOtherDetour.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::PickupWeaponFromOther")) SetFailState("Failed to set signature for PickupWeapon detour. (Error code 22)");
	if (!hPickupWeaponFromOtherDetour.AddParam(HookParamType_ObjectPtr)) SetFailState("Failed to add param to PickupWeapon detour. (Error code 23)");

	hGetEntityForLoadoutSlot = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_CBaseEntity, ThisPointer_CBaseEntity);
	if (!hGetEntityForLoadoutSlot) SetFailState("Failed to setup detour for GetEnt. (Error code 31)");
	if (!hGetEntityForLoadoutSlot.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::GetEntityForLoadoutSlot")) SetFailState("Failed to set signature for GetEnt detour. (Error code 32)");
	if (!hGetEntityForLoadoutSlot.AddParam(HookParamType_Int)) SetFailState("Failed to add param to GetEnt detour. (Error code 33)");
	if (!hGetEntityForLoadoutSlot.AddParam(HookParamType_Bool)) SetFailState("Failed to add param to GetEnt detour. (Error code 34)");

	savedData = new KeyValues("SavedData");
	if (!savedData) SetFailState("Failed to set up SavedData (Error code 40)");

	if (GetConVarBool(enabledVar)) EnableDetours();

	enabledVar.AddChangeHook(EnabledVarChanged);
}

public void OnClientDisconnect(int client)
{
	#if defined __freeguns_model_included
		ModelOnClientDisconnect(client);
	#endif
	#if defined __freeguns_glow_included
		GlowOnClientDisconnect(client);
	#endif
}


void OnInventoryApplied(Event event, const char[] name, bool dontBroadcast)
{
	int userId = event.GetInt("userid", -1);
	if (userId == -1) return;

	int client = GetClientOfUserId(userId);
	if (!IsClientInGame(client) || client == 0) return;
	CreateTimer(0.5, Timer_PostInventory, client);
}

Action Timer_PostInventory(Handle timer, int client)
{
	#if defined __freeguns_model_included
	if (modelVar.BoolValue)
		SetWearable(client);
	#endif

	return Plugin_Handled;
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
		EnableDetours();
	}
	else if (StringToInt(oldValue) != 0 && StringToInt(newValue) == 0)
	{
		//it was true, now it's false
		LogMessage("Disabling Freeguns.");
		DisableDetours();
	}
}

int EnableDetours()
{
	int out = EnableDetours2();

	if (out != 0) LogError("Failed to enable Freeguns detours. (Error code %i)", out);

	return out;
}

int EnableDetours2()
{
	if (!hCanPickupDroppedWeaponDetour.Enable(Hook_Pre, CanPickupDetour_Pre)) return 14;
	if (!hCanPickupDroppedWeaponDetour.Enable(Hook_Post, CanPickupDetour_Post)) return 15;

	if (!hPickupWeaponFromOtherDetour.Enable(Hook_Pre, PickupWeaponDetour_Pre)) return 24;
	if (!hPickupWeaponFromOtherDetour.Enable(Hook_Post, PickupWeaponDetour_Post)) return 25;

	return 0;
}

int DisableDetours()
{
	int out = DisableDetours2();

	if (out != 0)
		LogError("Failed to disable Freeguns detours. (Error code %i)", out);
	return out;
}

int DisableDetours2()
{
	if (!hCanPickupDroppedWeaponDetour.Disable(Hook_Pre, CanPickupDetour_Pre)) return 64;
	if (!hCanPickupDroppedWeaponDetour.Disable(Hook_Post, CanPickupDetour_Post)) return 65;

	if (!hPickupWeaponFromOtherDetour.Disable(Hook_Pre, PickupWeaponDetour_Pre)) return 74;
	if (!hPickupWeaponFromOtherDetour.Disable(Hook_Post, PickupWeaponDetour_Post)) return 75;

	return 0;
}


// Notes: iPlayer is the index of the player picking up the weapon. Use hReturn and hParams to change the values associated with the function.
MRESReturn CanPickupDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	#if defined DEBUG
		PrintToServer("---------------New Pickup Process Start---------------");
	#endif

	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	//to get class from item schema, get item itself first
	Address weaponMemAddress = hParams.GetAddress(1);
	int weaponEntity = GetEntityFromAddress(weaponMemAddress);

	//All revolvers are broken. disable it if its one of those
	int weaponItemDefinitionIndex = GetEntProp(weaponEntity, Prop_Send, "m_iItemDefinitionIndex");
	char weaponItemDefClassname[64];

	bool disabled = false;
	if
	(
		TF2Econ_GetItemClassName(weaponItemDefinitionIndex, weaponItemDefClassname, sizeof weaponItemDefClassname)
		&& StrEqual(weaponItemDefClassname, "tf_weapon_revolver")
	)
	{
		#if defined DEBUG
			PrintToServer("CanPickPre: Weapon is a revolver! Disabling pickup");
		#endif
		if (TF2_GetPlayerClass(iPlayer) != TFClass_Spy)
		{
			disabled = true;
			hReturn.Value = false;
		}
	}

	SaveClasses(iPlayer, weaponEntity, disabled);
	#if defined DEBUG
		PrintToServer("CanPickPre: Switch to desired class (%i)", GetSavedClass(iPlayer, "DesiredClass"));
	#endif

	TF2_SetPlayerClass(iPlayer, GetSavedClass(iPlayer, "DesiredClass"), _, false);
	return MRES_Handled;
}
MRESReturn CanPickupDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	#if defined DEBUG
		if (hReturn.Value) PrintToServer("CanPickup passed!");
		else PrintToServer("CanPickup did not pass...");

		PrintToServer("CanPickPost: Switch back to current class (%i)", GetSavedClass(iPlayer, "CurrentClass"));
	#endif

	TF2_SetPlayerClass(iPlayer, GetSavedClass(iPlayer, "CurrentClass"), _, false);

	return MRES_Handled;
}

MRESReturn PickupWeaponDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	#if defined DEBUG
		PrintToServer("PickupWeaponPre: Switch to desired class (%i)", GetSavedClass(iPlayer, "DesiredClass"));
	#endif

	TF2_SetPlayerClass(iPlayer, GetSavedClass(iPlayer, "DesiredClass"), _, false);
	return MRES_Handled;
}
MRESReturn PickupWeaponDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	#if defined DEBUG
		if (hReturn.Value) PrintToServer("PickupWeapon passed!");
		else PrintToServer("PickupWeapon did not pass...");

		PrintToServer("PickupWeaponPost: Switch back to current class (%i)", GetSavedClass(iPlayer, "CurrentClass"));
	#endif

	TF2_SetPlayerClass(iPlayer, GetSavedClass(iPlayer, "CurrentClass"), _, false);

	//we're done! delete the user's key
	char userIdStr[32];
	IntToString(GetClientUserId(iPlayer), userIdStr, sizeof userIdStr);
	savedData.JumpToKey("Classes");
	savedData.DeleteKey(userIdStr);
	savedData.Rewind();

	//for sigsegv
	Address weaponMemAddress = hParams.GetAddress(1);
	int iDroppedWeaponEnt = GetEntityFromAddress(weaponMemAddress);

	int iEquippedWeaponEnt, equippedWeaponSlot;

	GetClassOfWeapon(iDroppedWeaponEnt, equippedWeaponSlot, TF2_GetPlayerClass(iPlayer));

	iEquippedWeaponEnt = GetPlayerWeaponSlot(iPlayer, equippedWeaponSlot);

	#if defined DEBUG
		PrintToServer("Weapon is in slot %i!", equippedWeaponSlot);
		char classname[64];
		GetEntityClassname(iEquippedWeaponEnt, classname, sizeof classname);
		if (hReturn.Value) PrintToServer("Re-equipping %i : %s!", iEquippedWeaponEnt, classname);
	#endif

	//can't just equip iWeaponEnt bc that's the tf_dropped_weapon

	//for some fucking reason, using EquipPlayerWeapon fucks with sigsegv
	//but its what allows vm wearables
	if (hReturn.Value && IsValidEntity(iEquippedWeaponEnt)) SDKCall(hSDKCallUpdateHands, iEquippedWeaponEnt);
	// if (hReturn.Value) EquipPlayerWeapon(iPlayer, iEquippedWeaponEnt);

	return MRES_Handled;
}

MRESReturn GetEntDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	#if defined DEBUG
		PrintToServer("GetEntPre: Switch to current weapon class (%i)", GetSavedClass(iPlayer, "CurrentWeaponClass"));
	#endif

	TF2_SetPlayerClass(iPlayer, GetSavedClass(iPlayer, "CurrentWeaponClass"), _, false);

	//ough fuck you rafradek why does this give the vm wearable
	if (hParams.Get(1) > -1 && hParams.Get(1) < 3)
	{
		//we dont need the fucking wearables!
		hReturn.Value = GetPlayerWeaponSlot(iPlayer, hParams.Get(1));
		return MRES_Override;
	}

	return MRES_Handled;
}
MRESReturn GetEntDetour_Post(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	#if defined DEBUG
		if (hReturn.Value != -1)
		{
			char clsname[65];
			GetEntityClassname(hReturn.Value, clsname, sizeof clsname);
			PrintToServer("Found item for needed slot %i : %s!", hParams.Get(1), clsname);
			//for rafradek/sigsegv-mvm
			if (StrEqual("tf_wearable_vm", clsname))
			{
				char model[PLATFORM_MAX_PATH];
				GetEntPropString(hReturn.Value, Prop_Data, "m_ModelName", model, sizeof model);
				PrintToServer("VM Wearable Model: %s", model);
			}
			for (int i = 0; i < 32; i++)
			{
				int ent;
				ent = GetPlayerWeaponSlot(iPlayer, i);
				if (ent == -1) continue;
				GetEntityClassname(ent, clsname, sizeof clsname);
				PrintToServer("Item in slot %i : %s", i , clsname);

			}
		}
		else PrintToServer("Found nothing for needed slot %i...", hParams.Get(1));

		PrintToServer("GetEntPost: Switch back to desired class (%i)", GetSavedClass(iPlayer, "DesiredClass"));
	#endif

	TF2_SetPlayerClass(iPlayer, GetSavedClass(iPlayer, "DesiredClass"), _, false);

	#if defined __freeguns_model_included
		//viewmodel changes back after 1 second, fix that
		// CreateTimer(1.0, PostPickupFixViewmodel, EntIndexToEntRef(iPlayer));
	#endif

	return MRES_Handled;
}

#if defined __freeguns_model_included
	Action PostPickupFixViewmodel(Handle timer, int client)
	{
		int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
		OnClientWeaponSwitchPost(EntRefToEntIndex(client), weapon);
		return Plugin_Handled;
	}
#endif

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
		desiredClass = GetClassOfWeapon(weapon, loadoutSlotUsedByPickup, curClass);

	char userIdStr[32];
	IntToString(GetClientUserId(client), userIdStr, sizeof userIdStr);

	savedData.JumpToKey("Classes", true);
	savedData.JumpToKey(userIdStr, true);
	savedData.SetNum("CurrentClass", view_as<int>(curClass));
	savedData.SetNum("DesiredClass", view_as<int>(desiredClass));

	//finally, we need to save the class of the weapon we currently have (in the relevant slot, not the one that's out), since that might not necessarily be our player class
	//if we have a foreign weapon, GetEntityForLoadoutSlot won't find a weapon that matches our class (because we don't have one).

	int unneeded, weaponSlotUsedByPickup;

	//have to translate the relevant LOADOUT slot to the relevant WEAPON slot. They are not defined the same way, and some values match differently.
	//Since we're only dealing with weapons you can pick up, we only really need to deal with the three main weapon slots.

	// //While this might be optimizable, since it seems like the loadout slots and the weapon slots are the same for the three main ones,
	// //I'm not sure about that. This is safer.
	// char weaponSlotName[64];
	// TF2Econ_TranslateLoadoutSlotIndexToName(loadoutSlotUsedByPickup, weaponSlotName, sizeof weaponSlotName);
	// if (StrEqual(weaponSlotName, "primary")) weaponSlotUsedByPickup = TFWeaponSlot_Primary;
	// if (StrEqual(weaponSlotName, "secondary")) weaponSlotUsedByPickup = TFWeaponSlot_Secondary;
	// if (StrEqual(weaponSlotName, "melee")) weaponSlotUsedByPickup = TFWeaponSlot_Melee;

	//For the three main ones, loadout slot is the same as weapon slot.
	//I'm confident that won't change, it's too ingrained in the game.
	weaponSlotUsedByPickup = loadoutSlotUsedByPickup;

	#if defined DEBUG
		PrintToServer("LoadoutSlot: %i", loadoutSlotUsedByPickup);
		PrintToServer("WeaponSlot: %i", weaponSlotUsedByPickup);
		// PrintToServer("WeaponSlotName: %s", weaponSlotName);
	#endif

	int weaponToDrop = GetPlayerWeaponSlot(client, weaponSlotUsedByPickup);
	if (weaponToDrop != -1)
		savedData.SetNum("CurrentWeaponClass", view_as<int>(GetClassOfWeapon(weaponToDrop, unneeded, curClass)));
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

TFClassType GetClassOfWeapon(int weapon, int& loadoutSlot, TFClassType currentClass = TFClass_Unknown)
{
	TFClassType out;

	//get the item's definition index
	int iWeaponDef = GetEntProp(weapon, Prop_Send, "m_iItemDefinitionIndex");

	if (currentClass != TFClass_Unknown)
	{
		int tempSlot = TF2Econ_GetItemLoadoutSlot(iWeaponDef, currentClass);
		if (tempSlot != -1)
		{
			//the weapon works with their current class
			loadoutSlot = tempSlot;
			return currentClass;
		}
	}

	//check each class for if this weapon supports it
	//Civilian/Unknown left out bc i dont know if that actually works ingame
	TFClassType classes[] = {TFClass_Scout, TFClass_Sniper, TFClass_Soldier, TFClass_DemoMan, TFClass_Medic, TFClass_Heavy, TFClass_Pyro, TFClass_Spy, TFClass_Engineer};
	for (int i = 0; i < sizeof classes; i++)
	{
		int tempSlot = TF2Econ_GetItemLoadoutSlot(iWeaponDef, classes[i]);
		if (tempSlot != -1)
		{
			//there's a slot defined for this class, so it's meant to be for this class
			out = classes[i];
			loadoutSlot = tempSlot;
			break;
		}
	}
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