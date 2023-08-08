#pragma semicolon 1
#pragma newdecls required

#define __freeguns_model_included

/*
 * This file manages the player model and viewmodel.
 * Separated for cleanliness.
 */

void SetPlayerModelFromWeapon(int client, int weapon)
{
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

	SetVariantString(classModelPath);
	AcceptEntityInput(client, "SetCustomModel");
	SetEntProp(client, Prop_Send, "m_bUseClassAnimations", 1);
		
}