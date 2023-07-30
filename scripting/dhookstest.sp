#pragma semicolon 1
#pragma newdecls required
#include <sdktools>
#include <dhooks>
#include <tf2_stocks>

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
		SetFailState("Failed to set up SavedClasses (Error code 40)")

}

// Notes: iPlayer is the index of the player picking up the weapon. Use hReturn and hParams to change the values associated with the function.
public MRESReturn CanPickupDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	TFClassType curClass = TF2_GetPlayerClass(iPlayer);
	TFClassType desiredClass;

	//get desired class from item schema
	//first, get the item itself
	Address weaponMemAddress = hParams.GetAddress(1);
	int iWeaponEnt = GetEntityFromAddress(weaponMemAddress);

	//next, get the item's definition index
	int iWeaponDef = GetEntProp(iWeaponEnt, Prop_Send, "m_iItemDefinitionIndex");

	//TODO: TF2 Econ plugin. Get the weapon's class (or the first class, if multiclass)



	PrintToServer("CanPickPre: Switch to scout");
	TF2_SetPlayerClass(iPlayer, TFClass_Scout, true, false);
	return MRES_Handled;
}

public MRESReturn CanPickupDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	if (hReturn.Value) PrintToServer("CanPickup passed!");
	else PrintToServer("CanPickup did not pass...");

	PrintToServer("CanPickPost: Switch back to soldier");
	TF2_SetPlayerClass(iPlayer, TFClass_Soldier, true, false);

	return MRES_Handled;
}

public MRESReturn PickupWeaponDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Enable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Enable(Hook_Post, GetEntDetour_Post);

	PrintToServer("PickupWeaponPre: Switch to scout");
	TF2_SetPlayerClass(iPlayer, TFClass_Scout, true, false);
	return MRES_Handled;
}
public MRESReturn PickupWeaponDetour_Post (int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	hGetEntityForLoadoutSlot.Disable(Hook_Pre, GetEntDetour_Pre);
	hGetEntityForLoadoutSlot.Disable(Hook_Post, GetEntDetour_Post);

	if (hReturn.Value) PrintToServer("PickupWeapon passed!");
	else PrintToServer("PickupWeapon did not pass...");

	PrintToServer("PickupWeaponPost: Switch back to soldier");
	TF2_SetPlayerClass(iPlayer, TFClass_Soldier, true, false);

	return MRES_Handled;
}

public MRESReturn GetEntDetour_Pre(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{

	PrintToServer("GetEntPre: Switch to %s", desiredClass);
	TF2_SetPlayerClass(iPlayer, TFClass_Soldier, true, false);
	return MRES_Handled;
}

public MRESReturn GetEntDetour_Post(int iPlayer, DHookReturn hReturn, DHookParam hParams)
{
	if (hReturn.Value != -1)
	{
		char clsname[65];
		GetEntityClassname(hReturn.Value, clsname, sizeof clsname);
		PrintToServer("Found item for desired slot (%s)!", clsname);
	}
	else PrintToServer("Found nothing for desired slot...");

	PrintToServer("GetEntPost: Switch back to scout");
	TF2_SetPlayerClass(iPlayer, TFClass_Scout, true, false);

	return MRES_Handled;
}

int GetEntityFromAddress(Address pEntity) {
	return SDKCall(hSDKCallGetBaseEntity, pEntity);
}