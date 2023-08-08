#pragma semicolon 1
#pragma newdecls required

#define __freeguns_model_included

/*
 * This file manages the player model and viewmodel.
 * Separated for cleanliness.
 */

ConVar modelVar;

int viewmodelWearables[MAXPLAYERS];

void SetPlayerModelFromWeapon(int client, int weapon)
{
	if (!IsValidEntity(client) || !IsValidEntity(weapon)) return;
	
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
		return;
	}

	int unneeded;
	TFClassType desiredClass = GetClassOfWeapon(weapon, unneeded);

	char classModelPath[32];
	switch (desiredClass)
	{
		case TFClass_Scout:
			classModelPath = "models/player/scout.mdl";
		case TFClass_Soldier:
			classModelPath = "models/player/soldier.mdl";
		case TFClass_Pyro:
			classModelPath = "models/player/pyro.mdl";
		case TFClass_DemoMan:
			classModelPath = "models/player/demo.mdl";
		case TFClass_Heavy:
			classModelPath = "models/player/heavy.mdl";
		case TFClass_Engineer:
			classModelPath = "models/player/engineer.mdl";
		case TFClass_Medic:
			classModelPath = "models/player/medic.mdl";
		case TFClass_Sniper:
			classModelPath = "models/player/sniper.mdl";
		case TFClass_Spy:
			classModelPath = "models/player/spy.mdl";
	}

	if (StrEqual(currentModel, classModelPath, false)) return; //dont need to do anything. already taken up enough processing time

	SetVariantString(classModelPath);
	AcceptEntityInput(client, "SetCustomModel");
	SetEntProp(client, Prop_Send, "m_bUseClassAnimations", 1);

}

void OnClientWeaponSwitch(int client, int newWeapon)
{
	if (modelVar.BoolValue)
		SetPlayerModelFromWeapon(client, newWeapon);
}

public void OnClientPutInServer(int client)
{
	SDKHook(client, SDKHook_WeaponSwitchPost, OnClientWeaponSwitch);
}

public void OnClientDisconnect(int client)
{
	SDKUnhook(client, SDKHook_WeaponSwitchPost, OnClientWeaponSwitch);
	if (IsValidEntity(viewmodelWearables[client])) RemoveEntity(viewmodelWearables[client]);
	viewmodelWearables[client] = 0;
}

public void OnPluginEnd()
{
	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client)) continue;
		SDKUnhook(client, SDKHook_WeaponSwitchPost, OnClientWeaponSwitch);
		if (IsValidEntity(viewmodelWearables[client])) RemoveEntity(viewmodelWearables[client]);
		viewmodelWearables[client] = 0;
	}
}