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


	char classModelPath[32];
	char viewmodelPath[64];
	GetModelOfClass(desiredClass, classModelPath, sizeof classModelPath, viewmodelPath, sizeof viewmodelPath);

	if (StrEqual(currentModel, classModelPath, false)) return Plugin_Continue; //dont need to do anything. already taken up enough processing time

	if (!IsModelPrecached(classModelPath)) PrecacheModel(classModelPath);
	// if (!IsModelPrecached(viewmodelPath)) PrecacheModel(viewmodelPath);

	SetVariantString(classModelPath);
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


public void OnClientPutInServer(int client)
{
	SDKHook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
}

public void OnClientDisconnect(int client)
{
	SDKUnhook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
}

public void OnPluginEnd()
{
	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client)) continue;
		SDKUnhook(client, SDKHook_WeaponSwitch, OnClientWeaponSwitch);
		SetVariantString("");
		AcceptEntityInput(client, "SetCustomModel");
		SetEntProp(client, Prop_Send, "m_bUseClassAnimations", 1);
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