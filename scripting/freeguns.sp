#pragma semicolon 1
#pragma newdecls required
#include <sdktools>
#include <dhooks>
#include <tf2_stocks>
#include <tf_econ_data>

#include <freeguns_glow>

#define PLUGIN_VERSION "0.8"

public Plugin myinfo =
{
	name = "Freeguns",
	author = "Stinky Lizard",
	description = "Kill your enemy. Steal their stuff.",
	version = PLUGIN_VERSION,
	url = "freak.tf2.host"
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

KeyValues savedClasses;

ConVar enabledVar;

public void OnPluginStart()
{

	CreateConVar("sm_freeguns_version", PLUGIN_VERSION, "Standard plugin version ConVar. Please don't change me!", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
	enabledVar = CreateConVar("sm_freeguns_enabled", "1", "Enable/disable Freeguns. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);

	#if defined __freeguns_glow_included
		glowVar = CreateConVar("sm_freeguns_glow", "1", "Enable/disable custom weapon glow. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
		allGlowEntities = new ArrayList(sizeof GlowEntity);
	#endif

	GameData hGameConf = new GameData("tf2.freeguns");

	if(hGameConf == INVALID_HANDLE)
		SetFailState("FreeGuns: Gamedata not found!");

	StartPrepSDKCall(SDKCall_Raw);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBaseEntity::GetBaseEntity");
	PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	hSDKCallGetBaseEntity = EndPrepSDKCall();

	//TODO: update function signatures and see if they can be made better

	hCanPickupDroppedWeaponDetour = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_Bool, ThisPointer_CBaseEntity);
	if (!hCanPickupDroppedWeaponDetour)
		SetFailState("Failed to setup detour for CanPickup. (Error code 11)");
	if (!hCanPickupDroppedWeaponDetour.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::CanPickupDroppedWeapon"))
		SetFailState("Failed to set signature for CanPickup detour. (Error code 12)");
	if (!hCanPickupDroppedWeaponDetour.AddParam(HookParamType_ObjectPtr))
		SetFailState("Failed to add param to CanPickup detour. (Error code 13)");

	hPickupWeaponFromOtherDetour = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_Bool, ThisPointer_CBaseEntity);
	if (!hPickupWeaponFromOtherDetour)
		SetFailState("Failed to setup detour for PickupWeapon. (Error code 21)");
	if (!hPickupWeaponFromOtherDetour.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::PickupWeaponFromOther"))
		SetFailState("Failed to set signature for PickupWeapon detour. (Error code 22)");
	if (!hPickupWeaponFromOtherDetour.AddParam(HookParamType_ObjectPtr))
		SetFailState("Failed to add param to PickupWeapon detour. (Error code 23)");

	hGetEntityForLoadoutSlot = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_CBaseEntity, ThisPointer_CBaseEntity);
	if (!hGetEntityForLoadoutSlot)
		SetFailState("Failed to setup detour for GetEnt. (Error code 31)");
	if (!hGetEntityForLoadoutSlot.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::GetEntityForLoadoutSlot"))
		SetFailState("Failed to set signature for GetEnt detour. (Error code 32)");
	if (!hGetEntityForLoadoutSlot.AddParam(HookParamType_Int))
		SetFailState("Failed to add param to GetEnt detour. (Error code 33)");
	if (!hGetEntityForLoadoutSlot.AddParam(HookParamType_Bool))
		SetFailState("Failed to add param to GetEnt detour. (Error code 34)");

	savedClasses = new KeyValues("SavedClasses");
	if (!savedClasses)
		SetFailState("Failed to set up SavedClasses (Error code 40)");

	if (GetConVarBool(enabledVar))
	{
		EnableDetours();
	}

	enabledVar.AddChangeHook(EnabledVarChanged);

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

	if (out != 0)
		LogError("Failed to enable Freeguns detours. (Error code %i)", out);
	return out;
}

int EnableDetours2()
{
	if (!hCanPickupDroppedWeaponDetour.Enable(Hook_Pre, CanPickupDetour_Pre))
		return 14;
	if (!hCanPickupDroppedWeaponDetour.Enable(Hook_Post, CanPickupDetour_Post))
		return 15;

	if (!hPickupWeaponFromOtherDetour.Enable(Hook_Pre, PickupWeaponDetour_Pre))
		return 24;
	if (!hPickupWeaponFromOtherDetour.Enable(Hook_Post, PickupWeaponDetour_Post))
		return 25;
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
	if (!hCanPickupDroppedWeaponDetour.Disable(Hook_Pre, CanPickupDetour_Pre))
		return 64;
	if (!hCanPickupDroppedWeaponDetour.Disable(Hook_Post, CanPickupDetour_Post))
		return 65;

	if (!hPickupWeaponFromOtherDetour.Disable(Hook_Pre, PickupWeaponDetour_Pre))
		return 74;
	if (!hPickupWeaponFromOtherDetour.Disable(Hook_Post, PickupWeaponDetour_Post))
		return 75;
	return 0;
}


// Notes: iPlayer is the index of the player picking up the weapon. Use hReturn and hParams to change the values associated with the function.
MRESReturn CanPickupDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	//to get class from item schema, get item itself first
	Address weaponMemAddress = hParams.GetAddress(1);
	int iWeaponEnt = GetEntityFromAddress(weaponMemAddress);
	SaveClasses(iPlayer, iWeaponEnt);

	// PrintToServer("CanPickPre: Switch to desired class (%i)", GetSavedDesiredClass(iPlayer)); //debug
	TF2_SetPlayerClass(iPlayer, GetSavedDesiredClass(iPlayer), true, false);
	return MRES_Handled;
}
MRESReturn CanPickupDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	// if (hReturn.Value) PrintToServer("CanPickup passed!");
	// else PrintToServer("CanPickup did not pass...");

	// PrintToServer("CanPickPost: Switch back to current class (%i)", GetSavedCurrentClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedCurrentClass(iPlayer), true, false);

	return MRES_Handled;
}

MRESReturn PickupWeaponDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	// PrintToServer("PickupWeaponPre: Switch to desired class (%i)", GetSavedDesiredClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedDesiredClass(iPlayer), true, false);
	return MRES_Handled;
}
MRESReturn PickupWeaponDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	// if (hReturn.Value) PrintToServer("PickupWeapon passed!");
	// else PrintToServer("PickupWeapon did not pass...");

	// PrintToServer("PickupWeaponPost: Switch back to current class (%i)", GetSavedCurrentClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedCurrentClass(iPlayer), true, false);


	//we're done! delete the user's key
	char userIdStr[32];
	IntToString(GetClientUserId(iPlayer), userIdStr, sizeof userIdStr);
	savedClasses.DeleteKey(userIdStr);

	return MRES_Handled;
}

MRESReturn GetEntDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{

	// PrintToServer("GetEntPre: Switch to current class (%i)", GetSavedCurrentClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedCurrentClass(iPlayer), true, false);
	return MRES_Handled;
}
MRESReturn GetEntDetour_Post(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	// if (hReturn.Value != -1)
	// {
	// 	char clsname[65];
	// 	GetEntityClassname(hReturn.Value, clsname, sizeof clsname);
	// 	// PrintToServer("Found item for needed slot (%s)!", clsname);
	// }
	// else PrintToServer("Found nothing for needed slot...");

	// PrintToServer("GetEntPost: Switch back to desired class (%i)", GetSavedDesiredClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedDesiredClass(iPlayer), true, false);

	return MRES_Handled;
}

int GetEntityFromAddress(Address pEntity) {
	return SDKCall(hSDKCallGetBaseEntity, pEntity);
}

//get the class for the weapon from the item schema, and save that and the current class to savedClasses
void SaveClasses(int client, int weapon)
{
	TFClassType curClass = TF2_GetPlayerClass(client);
	TFClassType desiredClass;

	//get the item's definition index
	int iWeaponDef = GetEntProp(weapon, Prop_Send, "m_iItemDefinitionIndex");

	//check each class for if this weapon supports it
	//Civilian/Unknown left out bc i dont know if that actually works ingame
	TFClassType classes[] = {TFClass_Scout, TFClass_Sniper, TFClass_Soldier, TFClass_DemoMan, TFClass_Medic, TFClass_Heavy, TFClass_Pyro, TFClass_Spy, TFClass_Engineer};
	for (int i = 0; i < sizeof classes; i++)
	{
		if (TF2Econ_GetItemLoadoutSlot(iWeaponDef, classes[i]) != -1)
		{
			//there's a slot defined for this class, so it's meant to be for this class
			desiredClass = classes[i];
			break;
		}
	}
	char userIdStr[32];
	IntToString(GetClientUserId(client), userIdStr, sizeof userIdStr);

	savedClasses.JumpToKey(userIdStr, true);
	savedClasses.SetNum("CurrentClass", view_as<int>(curClass));
	savedClasses.SetNum("DesiredClass", view_as<int>(desiredClass));
	savedClasses.Rewind();
}

TFClassType GetSavedCurrentClass(int client)
{
	char userIdStr[32];
	IntToString(GetClientUserId(client), userIdStr, sizeof userIdStr);

	savedClasses.JumpToKey(userIdStr);
	TFClassType out = view_as<TFClassType>(savedClasses.GetNum("CurrentClass"));
	savedClasses.Rewind();
	return out;
}

TFClassType GetSavedDesiredClass(int client)
{
	char userIdStr[32];
	IntToString(GetClientUserId(client), userIdStr, sizeof userIdStr);

	savedClasses.JumpToKey(userIdStr);
	TFClassType out = view_as<TFClassType>(savedClasses.GetNum("DesiredClass"));
	savedClasses.Rewind();
	return out;
}