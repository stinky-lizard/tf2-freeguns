#pragma semicolon 1
#pragma newdecls required
#include <sdktools>
#include <dhooks>
#include <tf2_stocks>
#include <tf_econ_data>

DynamicDetour hCanPickupDroppedWeaponDetour;
DynamicDetour hPickupWeaponFromOtherDetour;
DynamicDetour hGetEntityForLoadoutSlot;
Handle hSDKCallGetBaseEntity;

KeyValues savedClasses;

public void OnPluginStart()
{
	GameData hGameConf = new GameData("tf2.freeguns");

	if(hGameConf == INVALID_HANDLE)
		SetFailState("FreeGuns: Gamedata not found!");

	StartPrepSDKCall(SDKCall_Raw);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBaseEntity::GetBaseEntity");
	PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	hSDKCallGetBaseEntity = EndPrepSDKCall();

	hCanPickupDroppedWeaponDetour = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_Bool, ThisPointer_CBaseEntity);
	if (!hCanPickupDroppedWeaponDetour)
		SetFailState("Failed to setup detour for CanPickup (Error code 11)");
	if (!hCanPickupDroppedWeaponDetour.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::CanPickupDroppedWeapon"))
		SetFailState("Failed to set signature for CanPickup detour (Error code 12)");
	if (!hCanPickupDroppedWeaponDetour.AddParam(HookParamType_ObjectPtr))
		SetFailState("Failed to add param to CanPickup detour (Error code 13)");
	if (!hCanPickupDroppedWeaponDetour.Enable(Hook_Pre, CanPickupDetour_Pre))
		SetFailState("Failed to enable CanPickup pre detour (Error code 14)");
	if (!hCanPickupDroppedWeaponDetour.Enable(Hook_Post, CanPickupDetour_Post))
		SetFailState("Failed to enable CanPickup post detour (Error code 15)");

	//TODO: update PickupWeaponFromOther function signature and see if it can be made better
	hPickupWeaponFromOtherDetour = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_Bool, ThisPointer_CBaseEntity);
	if (!hPickupWeaponFromOtherDetour)
		SetFailState("Failed to setup detour for PickupWeapon (Error code 21)");
	if (!hPickupWeaponFromOtherDetour.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::PickupWeaponFromOther"))
		SetFailState("Failed to set signature for PickupWeapon detour (Error code 22)");
	if (!hPickupWeaponFromOtherDetour.AddParam(HookParamType_ObjectPtr))
		SetFailState("Failed to add param to PickupWeapon detour (Error code 23)");
	if (!hPickupWeaponFromOtherDetour.Enable(Hook_Pre, PickupWeaponDetour_Pre))
		SetFailState("Failed to enable PickupWeapon pre detour (Error code 24)");
	if (!hPickupWeaponFromOtherDetour.Enable(Hook_Post, PickupWeaponDetour_Post))
		SetFailState("Failed to enable PickupWeapon post detour (Error code 25)");

	hGetEntityForLoadoutSlot = new DynamicDetour(Address_Null, CallConv_THISCALL, ReturnType_CBaseEntity, ThisPointer_CBaseEntity);
	if (!hGetEntityForLoadoutSlot)
		SetFailState("Failed to setup detour for GetEnt (Error code 31)");
	if (!hGetEntityForLoadoutSlot.SetFromConf(hGameConf, SDKConf_Signature, "CTFPlayer::GetEntityForLoadoutSlot"))
		SetFailState("Failed to set signature for GetEnt detour (Error code 32)");
	if (!hGetEntityForLoadoutSlot.AddParam(HookParamType_Int))
		SetFailState("Failed to add param to GetEnt detour (Error code 33)");
	if (!hGetEntityForLoadoutSlot.AddParam(HookParamType_Bool))
		SetFailState("Failed to add param to GetEnt detour (Error code 34)");

	savedClasses = new KeyValues("SavedClasses");
	if (!savedClasses)
		SetFailState("Failed to set up SavedClasses (Error code 40)");

}


// Notes: iPlayer is the index of the player picking up the weapon. Use hReturn and hParams to change the values associated with the function.
public MRESReturn CanPickupDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	//to get class from item schema, get item itself first
	Address weaponMemAddress = hParams.GetAddress(1);
	int iWeaponEnt = GetEntityFromAddress(weaponMemAddress);
	SaveClasses(iPlayer, iWeaponEnt);

	PrintToServer("CanPickPre: Switch to desired class (%i)", GetSavedDesiredClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedDesiredClass(iPlayer), true, false);
	return MRES_Handled;
}

public MRESReturn CanPickupDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	if (hReturn.Value) PrintToServer("CanPickup passed!");
	else PrintToServer("CanPickup did not pass...");

	PrintToServer("CanPickPost: Switch back to current class (%i)", GetSavedCurrentClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedCurrentClass(iPlayer), true, false);

	return MRES_Handled;
}

public MRESReturn PickupWeaponDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	PrintToServer("PickupWeaponPre: Switch to desired class (%i)", GetSavedDesiredClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedDesiredClass(iPlayer), true, false);
	return MRES_Handled;
}
public MRESReturn PickupWeaponDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	if (hReturn.Value) PrintToServer("PickupWeapon passed!");
	else PrintToServer("PickupWeapon did not pass...");

	PrintToServer("PickupWeaponPost: Switch back to current class (%i)", GetSavedCurrentClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedCurrentClass(iPlayer), true, false);

	return MRES_Handled;
}

public MRESReturn GetEntDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{

	PrintToServer("GetEntPre: Switch to current class (%i)", GetSavedCurrentClass(iPlayer));
	TF2_SetPlayerClass(iPlayer, GetSavedCurrentClass(iPlayer), true, false);
	return MRES_Handled;
}

public MRESReturn GetEntDetour_Post(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	if (hReturn.Value != -1)
	{
		char clsname[65];
		GetEntityClassname(hReturn.Value, clsname, sizeof clsname);
		PrintToServer("Found item for needed slot (%s)!", clsname);
	}
	else PrintToServer("Found nothing for needed slot...");

	PrintToServer("GetEntPost: Switch back to desired class (%i)", GetSavedDesiredClass(iPlayer));
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