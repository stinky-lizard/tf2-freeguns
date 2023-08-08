#pragma semicolon 1
#pragma newdecls required

#define __freeguns_model_included

/*
 * This file manages the player model and viewmodel.
 * Separated for cleanliness.
 */

ConVar modelVar;

int viewmodelWearables[MAXPLAYERS];

#define DEBUG3

#if defined DEBUG3
	Action DeleteWearableTimer(Handle timer, int wearableRef)
	{
		if (IsValidEntity(wearableRef)) RemoveEntity(wearableRef);
		return Plugin_Handled;
	}
	#define EF_NODRAW 0x020    // don't draw entity
	#define EF_BONEMERGE 0x001
	#define EF_BONEMERGE_FASTCULL 0x080
#endif

Action SetPlayerModelFromWeapon(int client, int weapon)
{
	if (!IsValidEntity(client) || !IsValidEntity(weapon)) return Plugin_Continue;
#if defined DEBUG3
	// int clientViewModelWearable = CreateEntityByName("tf_wearable_vm");
	// SetEntPropEnt(clientViewModelWearable, Prop_Send, "m_hOwnerEntity", client);
	// SetEntProp(clientViewModelWearable, Prop_Send, "m_iTeamNum", view_as<int>(TF2_GetClientTeam(client)));
	// DispatchKeyValueVector(clientViewModelWearable, "origin", {0.0,0.0,0.0});
	// SetVariantString("!activator");
	// AcceptEntityInput(clientViewModelWearable, "SetParent", client);
	// // SetEntityModel(clientViewModelWearable, "models/weapons/c_models/c_heavy_arms.mdl");
	// SetEntProp(clientViewModelWearable, Prop_Send, "m_nModelIndex", PrecacheModel("models/weapons/c_models/c_pyro_arms.mdl"));
	// SetEntProp(clientViewModelWearable, Prop_Send, "m_bValidatedAttachedEntity", true);
	// // SetEntProp(clientViewModelWearable, Prop_Send, "m_fEffects", EF_BONEMERGE | EF_BONEMERGE_FASTCULL);
	// DispatchSpawn(clientViewModelWearable);
	// CreateTimer(5.0, DeleteWearableTimer, EntIndexToEntRef(clientViewModelWearable));
#endif

	char currentModel[32];
	GetClientModel(client, currentModel, sizeof currentModel);

	char classnameString[32];
	strcopy(classnameString, sizeof classnameString, currentModel);
	ReplaceString(classnameString, sizeof classnameString, "models/player/", "", false);
	ReplaceString(classnameString, sizeof classnameString, ".mdl", "", false);

	TFClassType currentClassLook = TF2_GetClass(classnameString);

	if (TF2Econ_GetItemLoadoutSlot(GetEntProp(weapon, Prop_Send, "m_iItemDefinitionIndex"), currentClassLook) != -1)
	{
		//this weapon works with what they look like! dont need to do anything
		return Plugin_Continue;
	}

	int unneeded;
	TFClassType desiredClass = GetClassOfWeapon(weapon, unneeded);


	char classModelPath[32];
	char viewmodelPath[64];
	switch (desiredClass)
	{
		case TFClass_Scout:
		{
			classModelPath = "models/player/scout.mdl";
			viewmodelPath = "models/weapons/c_models/c_scout_arms.mdl";
		}
		case TFClass_Soldier:
		{
			classModelPath = "models/player/soldier.mdl";
			viewmodelPath = "models/weapons/c_models/c_soldier_arms.mdl";
		}
		case TFClass_Pyro:
		{
			classModelPath = "models/player/pyro.mdl";
			viewmodelPath = "models/weapons/c_models/c_pyro_arms.mdl";
		}
		case TFClass_DemoMan:
		{
			classModelPath = "models/player/demo.mdl";
			viewmodelPath = "models/weapons/c_models/c_demo_arms.mdl";
		}
		case TFClass_Heavy:
		{
			classModelPath = "models/player/heavy.mdl";
			viewmodelPath = "models/weapons/c_models/c_heavy_arms.mdl";
		}
		case TFClass_Engineer:
		{
			classModelPath = "models/player/engineer.mdl";
			viewmodelPath = "models/weapons/c_models/c_engineer_arms.mdl";
		}
		case TFClass_Medic:
		{
			classModelPath = "models/player/medic.mdl";
			viewmodelPath = "models/weapons/c_models/c_medic_arms.mdl";
		}
		case TFClass_Sniper:
		{
			classModelPath = "models/player/sniper.mdl";
			viewmodelPath = "models/weapons/c_models/c_sniper_arms.mdl";
		}
		case TFClass_Spy:
		{
			classModelPath = "models/player/spy.mdl";
			viewmodelPath = "models/weapons/c_models/c_spy_arms.mdl";
		}
	}

	if (StrEqual(currentModel, classModelPath, false)) return Plugin_Continue; //dont need to do anything. already taken up enough processing time

	if (!IsModelPrecached(classModelPath)) PrecacheModel(classModelPath);
	if (!IsModelPrecached(viewmodelPath)) PrecacheModel(viewmodelPath);

	SetVariantString(classModelPath);
	AcceptEntityInput(client, "SetCustomModel");
	SetEntProp(client, Prop_Send, "m_bUseClassAnimations", 1);
	int viewmodel = GetEntPropEnt(client, Prop_Send, "m_hViewModel");
	SetEntProp(viewmodel, Prop_Send, "m_fEffects", GetEntProp(viewmodel, Prop_Send, "m_fEffects") | EF_NODRAW);
	//kinda works bot not really
	// char weaponmodel[PLATFORM_MAX_PATH];
	// GetEntPropString(weapon, Prop_Data, "m_ModelName", weaponmodel, sizeof weaponmodel);
	// SetEntityModel(viewmodel, viewmodelPath);

	return Plugin_Continue;
}

void OnClientWeaponSwitch(int client, int newWeapon)
{
	if (modelVar.BoolValue)
		SetPlayerModelFromWeapon(client, newWeapon);
}

public void OnClientPutInServer(int client)
{
	SDKHook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
}

public void OnClientDisconnect(int client)
{
	SDKUnhook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
	if (viewmodelWearables[client] != 0 && IsValidEntity(viewmodelWearables[client])) RemoveEntity(viewmodelWearables[client]);
	viewmodelWearables[client] = 0;
}

public void OnPluginEnd()
{
	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client)) continue;
		SDKUnhook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
		if (viewmodelWearables[client] != 0 && IsValidEntity(viewmodelWearables[client])) RemoveEntity(viewmodelWearables[client]);
		viewmodelWearables[client] = 0;
	}
}