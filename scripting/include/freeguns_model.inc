#pragma semicolon 1
#pragma newdecls required

#define __freeguns_model_included

/*
 * This file manages the player model and viewmodel.
 * Separated for cleanliness.
 */

ConVar modelVar;
ConVar viewmodelVar;

int viewmodelWearableRefs[MAXPLAYERS][2];

Action SetPlayerModelFromWeapon(int client, int weapon)
{
	if (!IsValidEntity(client) || !IsValidEntity(weapon)) return Plugin_Continue;

	char currentModel[32];
	GetClientModel(client, currentModel, sizeof currentModel);

	char classnameString[32];
	strcopy(classnameString, sizeof classnameString, currentModel);
	ReplaceString(classnameString, sizeof classnameString, "models/player/", "", false);
	ReplaceString(classnameString, sizeof classnameString, ".mdl", "", false);

	TFClassType currentClassLook = TF2_GetClass(classnameString);

	bool origClass = false;
	if (TF2Econ_GetItemLoadoutSlot(GetEntProp(weapon, Prop_Send, "m_iItemDefinitionIndex"), currentClassLook) != -1)
	{
		//this weapon works with what they look like! dont need to do anything
		if (TF2Econ_GetItemLoadoutSlot(GetEntProp(weapon, Prop_Send, "m_iItemDefinitionIndex"), TF2_GetPlayerClass(client)) != -1)
		{
			//if the weapon also works with the player's native class, then switch back to that for this
			origClass = true;
		}
		else return Plugin_Continue;
	}

	int unneeded;
	TFClassType desiredClass = origClass ?  TF2_GetPlayerClass(client) : GetClassOfWeapon(weapon, unneeded);

	char worldmodelPath[32];
	char viewmodelPath[64];
	GetModelOfClass(desiredClass, worldmodelPath, sizeof worldmodelPath, viewmodelPath, sizeof viewmodelPath);

	if (StrEqual(currentModel, worldmodelPath, false)) return Plugin_Continue; //dont need to do anything. already taken up enough processing time

	if (!IsModelPrecached(worldmodelPath)) PrecacheModel(worldmodelPath);

	SetVariantString(worldmodelPath);
	AcceptEntityInput(client, "SetCustomModel");
	SetEntProp(client, Prop_Send, "m_bUseClassAnimations", 1);

	return Plugin_Continue;
}

Action OnClientWeaponSwitch(int client, int weapon)
{
	if (modelVar.BoolValue)
		return SetPlayerModelFromWeapon(client, weapon);
	else return Plugin_Continue;
}

void OnClientWeaponSwitchPost(int client, int weapon)
{
	if (!viewmodelVar.BoolValue) return;

	int unneeded;
	TFClassType desiredClass = GetClassOfWeapon(weapon, unneeded);

	char classModelPath[PLATFORM_MAX_PATH];
	char viewmodelPath[PLATFORM_MAX_PATH];
	GetModelOfClass(desiredClass, classModelPath, sizeof classModelPath, viewmodelPath, sizeof viewmodelPath);

	if (!IsModelPrecached(viewmodelPath)) PrecacheModel(viewmodelPath);

	SetEntityModel(weapon, viewmodelPath);
	// Animations are borked unless you set these.
	//Thanks to Zabaniya001 at https://github.com/Zabaniya001/TF2-ProperClassWeaponAnimations
	SetEntProp(weapon, Prop_Send, "m_nCustomViewmodelModelIndex", GetEntProp(weapon, Prop_Send, "m_nModelIndex"));
	SetEntProp(weapon, Prop_Send, "m_iViewModelIndex", GetEntProp(weapon, Prop_Send, "m_nModelIndex"));

	return;

	int viewmodel = GetEntPropEnt(client, Prop_Send, "m_hViewModel");
	// GetEntPropString(viewmodel, Prop_Data, "m_ModelName", str, sizeof str);
	// SetEntityModel(weapon, viewmodelPath);

	//change viewmodel anims
	//todo: needed?
	// int seq = GetEntProp(viewmodel, Prop_Send, "m_nSequence");
	// SetEntityModel(viewmodel, viewmodelPath);
	// SetEntProp(viewmodel, Prop_Send, "m_nSequence", seq);


	//hide viewmodel to get rid of flickering
	#define EF_NODRAW 0x020
	SetEntProp(viewmodel, Prop_Send, "m_fEffects", GetEntProp(viewmodel, Prop_Send, "m_fEffects") | EF_NODRAW);

	//re-add viewmodel w/o flickering
	//arms wearable
	int wearable = viewmodelWearableRefs[client][0];
	if (!IsValidEntity(EntRefToEntIndex(wearable)) || wearable == 0) //0 = uninitialized
	{
		wearable = CreateEntityByName("tf_wearable_vm");
		viewmodelWearableRefs[client][0] = EntIndexToEntRef(wearable);

		#define EF_BONEMERGE 0x001
		#define EF_BONEMERGE_FASTCULL 0x080
		SetEntProp(wearable, Prop_Send, "m_fEffects",  EF_BONEMERGE | EF_BONEMERGE_FASTCULL);
		SetEntProp(wearable, Prop_Send, "m_bValidatedAttachedEntity", 1);
		SetEntProp(wearable, Prop_Send, "m_iAccountID", GetSteamAccountID(client));
		SetEntPropEnt(wearable, Prop_Send, "m_hOwnerEntity", client);

		DispatchSpawn(wearable);
	}

	SetEntProp(wearable, Prop_Send, "m_iTeamNum", GetClientTeam(client));
	SetEntProp(wearable, Prop_Send, "m_nSkin", GetClientTeam(client));

	SetEntityModel(wearable, viewmodelPath);

	//todo: needed?
	SetVariantString("!activator");
	ActivateEntity(wearable);

	SetEntPropEnt(wearable, Prop_Send, "m_hWeaponAssociatedWith", weapon);

	TF2Util_EquipPlayerWearable(client, wearable);

	//weapon wearable
	char weaponModelPath[PLATFORM_MAX_PATH];
	GetEntPropString(weapon, Prop_Data, "m_ModelName", weaponModelPath, sizeof weaponModelPath);
	int weaponWearable = viewmodelWearableRefs[client][1];
	if (!IsValidEntity(EntRefToEntIndex(weaponWearable)) || weaponWearable == 0)
	{
		weaponWearable = CreateEntityByName("tf_wearable_vm");
		viewmodelWearableRefs[client][1] = EntIndexToEntRef(weaponWearable);
		DispatchSpawn(weaponWearable);
	}

	SetEntityModel(weaponWearable, weaponModelPath);

	SetEntPropEnt(weaponWearable, Prop_Send, "m_hWeaponAssociatedWith", weapon);

	TF2Util_EquipPlayerWearable(client, weaponWearable);
}

public void OnClientPutInServer(int client)
{
	SDKHook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
	SDKHook(client, SDKHook_WeaponSwitchPost, OnClientWeaponSwitchPost);
}

public void OnClientDisconnect(int client)
{
	SDKUnhook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
	SDKUnhook(client, SDKHook_WeaponSwitchPost, OnClientWeaponSwitchPost);

	DeleteWearables(client);
}

public void OnPluginEnd()
{
	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client)) continue;
		SDKUnhook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
		SDKUnhook(client, SDKHook_WeaponSwitchPost, OnClientWeaponSwitchPost);
		DeleteWearables(client);
	}
}

void DeleteWearables(int client)
{
	if (viewmodelWearableRefs[client][0] != 0 && IsValidEntity(viewmodelWearableRefs[client][0])) RemoveEntity(viewmodelWearableRefs[client][0]);
	if (viewmodelWearableRefs[client][1] != 0 && IsValidEntity(viewmodelWearableRefs[client][1])) RemoveEntity(viewmodelWearableRefs[client][1]);
	viewmodelWearableRefs[client][0] = 0;
	viewmodelWearableRefs[client][1] = 0;
}

void GetModelOfClass(TFClassType class, char[] worldModel, int worldModelLen, char[] viewModel, int viewModelLen)
{

	switch (class)
	{
		case TFClass_Scout:
		{
			strcopy(worldModel, worldModelLen, "models/player/scout.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_scout_arms.mdl");
		}
		case TFClass_Soldier:
		{
			strcopy(worldModel, worldModelLen, "models/player/soldier.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_soldier_arms.mdl");
		}
		case TFClass_Pyro:
		{
			strcopy(worldModel, worldModelLen, "models/player/pyro.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_pyro_arms.mdl");
		}
		case TFClass_DemoMan:
		{
			strcopy(worldModel, worldModelLen, "models/player/demo.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_demo_arms.mdl");
		}
		case TFClass_Heavy:
		{
			strcopy(worldModel, worldModelLen, "models/player/heavy.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_heavy_arms.mdl");
		}
		case TFClass_Engineer:
		{
			strcopy(worldModel, worldModelLen, "models/player/engineer.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_engineer_arms.mdl");
		}
		case TFClass_Medic:
		{
			strcopy(worldModel, worldModelLen, "models/player/medic.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_medic_arms.mdl");
		}
		case TFClass_Sniper:
		{
			strcopy(worldModel, worldModelLen, "models/player/sniper.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_sniper_arms.mdl");
		}
		case TFClass_Spy:
		{
			strcopy(worldModel, worldModelLen, "models/player/spy.mdl");
			strcopy(viewModel, viewModelLen, "models/weapons/c_models/c_spy_arms.mdl");
		}
	}
}