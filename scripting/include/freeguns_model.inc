#pragma semicolon 1
#pragma newdecls required

#define __freeguns_model_included

/*
 * This file manages the player model and viewmodel.
 * Separated for cleanliness.
 */

ConVar modelVar;

Action SetPlayerModelFromWeapon(int client, int weapon)
{
	if (!IsValidEntity(client) || !IsValidEntity(weapon)) return Plugin_Continue;

	//what class should they look like?
	TFClassType desiredClass;
	if (!getDesiredClass(client, weapon, desiredClass)) return Plugin_Continue;

	//what is the model of that class?
	char worldmodelPath[PLATFORM_MAX_PATH];
	char viewmodelPath[PLATFORM_MAX_PATH];
	GetModelOfClass(desiredClass, worldmodelPath, sizeof worldmodelPath, viewmodelPath, sizeof viewmodelPath);

	//are they already this model?
	char currentModel[PLATFORM_MAX_PATH];
	GetClientModel(client, currentModel, sizeof currentModel);
	if (StrEqual(currentModel, worldmodelPath, false)) return Plugin_Continue; //dont need to do anything. already taken up enough processing time

	//actually set model
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
	if (!modelVar.BoolValue) return;

	//what class should they look like?
	TFClassType desiredClass;
	if (!getDesiredClass(client, weapon, desiredClass)) return;

	//what is the model of that class?
	char worldmodelPath[PLATFORM_MAX_PATH];
	char viewmodelPath[PLATFORM_MAX_PATH];
	GetModelOfClass(desiredClass, worldmodelPath, sizeof worldmodelPath, viewmodelPath, sizeof viewmodelPath);

	//skipping checking if they're already this model b/c i'm not sure what ent is what model holy shit its weird

	//actually set model
	if (!IsModelPrecached(viewmodelPath)) PrecacheModel(viewmodelPath);

	SetEntityModel(weapon, viewmodelPath);
	// Animations are borked unless you set these.
	//Thanks to Zabaniya001 at https://github.com/Zabaniya001/TF2-ProperClassWeaponAnimations
	SetEntProp(weapon, Prop_Send, "m_nCustomViewmodelModelIndex", GetEntProp(weapon, Prop_Send, "m_nModelIndex"));
	SetEntProp(weapon, Prop_Send, "m_iViewModelIndex", GetEntProp(weapon, Prop_Send, "m_nModelIndex"));
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
}

public void OnPluginEnd()
{
	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client)) continue;
		SDKUnhook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
		SDKUnhook(client, SDKHook_WeaponSwitchPost, OnClientWeaponSwitchPost);
	}
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

bool getDesiredClass(int client, int weapon, TFClassType& out_desiredClass)
{
	char currentModel[PLATFORM_MAX_PATH];
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
			//if the weapon also works with the player's real class, then switch back to that for this
			origClass = true;
		}
		else return false;
	}

	int unneeded;
	out_desiredClass = origClass ?  TF2_GetPlayerClass(client) : GetClassOfWeapon(weapon, unneeded);

	return true;
}
