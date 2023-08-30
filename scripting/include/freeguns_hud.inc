#pragma newdecls required
#pragma semicolon 1


#define __freeguns_hud_included

ConVar hudVar;

#if !defined __freeguns_glow_included
	bool mapLoaded;
	public void OnMapStart() { mapLoaded = true; }
	public void OnMapEnd() { mapLoaded = false; }
#endif

#define WORLDTEXTHEIGHT 30.0

void HudOnEntityCreated(int entity, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon") || !enabledVar.BoolValue || !hudVar.BoolValue || !mapLoaded)
		return;

	RequestFrame(DroppedWeaponNextFrame, EntIndexToEntRef(entity));
}

void DroppedWeaponNextFrame(int entityref)
{
	if (!IsValidEntity(entityref)) return;

	int worldtext = CreateEntityByName("point_worldtext");

	char messagetext[256], topspacer[32], bottomspacer[32], clientname[MAX_NAME_LENGTH], qualityname[16], itemname[64];

	TF2Econ_GetQualityName(GetEntProp(entityref, Prop_Send, "m_iEntityQuality"), qualityname, sizeof qualityname);
	qualityname[0] = CharToUpper(qualityname[0]);

	TF2Econ_GetItemName(GetEntProp(entityref, Prop_Send, "m_iItemDefinitionIndex"), itemname, sizeof itemname);

	if (StrContains(itemname, "TF_") == 0)
		//it doesn't have a name in items_game
		Format(itemname, sizeof itemname, "%T", itemname, LANG_SERVER);

	//remove The at the beginning of weapon names
	if (StrContains(itemname, "the", false) == 0)
		ReplaceStringEx(itemname, sizeof itemname, "the ", "", _, _, false);

	int accountid = GetEntProp(entityref, Prop_Send, "m_iAccountID");
	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientConnected(client)) continue;
		if (GetSteamAccountID(client) == accountid)
		{
			GetClientName(client, clientname, sizeof clientname);
			break;
		}
	}

	if (GetEntProp(entityref, Prop_Send, "m_iEntityQuality") == 0)
	{
		int len = strlen(clientname) + strlen(itemname) + 4; //[clientname]'s [itemname]!
		CreateSpacers(len, topspacer, bottomspacer);
		Format(messagetext, sizeof messagetext, "%sPress Action Button (def. H) to pick up\n%s%s's %s!", topspacer, bottomspacer, clientname, itemname);
	}
	else
	{
		int len = strlen(clientname) + strlen(qualityname) + strlen(itemname) + 5;
		CreateSpacers(len, topspacer, bottomspacer);
		Format(messagetext, sizeof messagetext, "%sPress Action Button (def. H) to pick up\n%s%s's %s %s!", topspacer, bottomspacer, clientname, qualityname, itemname);
	}

	DispatchKeyValue(worldtext, "message", messagetext);
	SetVariantString("!activator");
	AcceptEntityInput(worldtext, "SetParent", entityref);
	SetEntPropVector(worldtext, Prop_Send, "m_vecOrigin", {0.0, 0.0, WORLDTEXTHEIGHT});
	DispatchKeyValueInt(worldtext, "font", 8);
	DispatchKeyValueInt(worldtext, "textsize", 4);
	DispatchKeyValueInt(worldtext, "orientation", 1);
	DispatchKeyValue(worldtext, "color", "255 255 0 255");

	if (DroppedWeaponIsDisabled(entityref))
	{
		DispatchKeyValue(worldtext, "color", "255 30 30 255");
		int len = strlen("Sorry, the  is disabled!") + strlen(itemname);
		topspacer = "";
		bottomspacer = "";
		CreateSpacers(len, topspacer, bottomspacer, true);
		Format(messagetext, sizeof messagetext, "%sSorry, the %s is disabled!\n%s(Technical limitations)", topspacer, itemname, bottomspacer);
		DispatchKeyValue(worldtext, "message", messagetext);
	}

	DispatchSpawn(worldtext);

	SDKHook(worldtext, SDKHook_SetTransmit, OnHudTransmit);

}

void CreateSpacers(int len, char[] topspacer, char[] bottomspacer, bool disabledText = false)
{
	int toplen, bottomlen;
	if (disabledText)
	{
		toplen = len;
		bottomlen = strlen("(Technical limitations)");
	}
	else
	{
		toplen = strlen("Press Action Button (def. H) to pick up");
		bottomlen = len;
	}

	if (bottomlen > toplen)
	{
		for (int i = 0; i < (bottomlen - toplen) / 2 && i < 32; i++)
			topspacer[i] = ' ';
	}
	else if (bottomlen < toplen)
	{
		for (int i = 0; i < (toplen - bottomlen) / 2 && i < 32; i++)
			bottomspacer[i] = ' ';
	}
}

Action OnHudTransmit(int worldtext, int client)
{
	if (!IsValidEntity(worldtext) || !IsValidEntity(client)) return Plugin_Continue; //whatever
	int weapon = GetEntPropEnt(worldtext, Prop_Data, "m_hParent");

	if (!IsValidEntity(weapon))
	{
		//somehow this worldtext has no parent. no reason to keep it around
		RemoveEntity(worldtext);
	}

	if (CanClientPickUpWeapon(client, weapon))
	{
		MoveWorldtextAboveParent(worldtext, weapon);
		return Plugin_Continue;
	}
	else return Plugin_Handled;
}

void MoveWorldtextAboveParent(int worldtext, int weapon)
{

	float weaponpos[3];
	GetEntPropVector(weapon, Prop_Data, "m_vecAbsOrigin", weaponpos);
	weaponpos[2] += WORLDTEXTHEIGHT;
	AcceptEntityInput(worldtext, "ClearParent");
	SetEntPropVector(worldtext, Prop_Send, "m_vecOrigin", weaponpos);
	SetVariantString("!activator");
	AcceptEntityInput(worldtext, "SetParent", weapon);
}

//if the client is looking at the weapon and is in range

bool CanClientPickUpWeapon(int client, int weapon)
{
	if (!IsClientInRange(client, weapon)) return false;
	else if	(!IsPlayerAlive(client)) return false;
	else if (TF2_IsPlayerInCondition(client, TFCond_Disguised) || TF2_IsPlayerInCondition(client, TFCond_Stealthed)) return false;
	else if (GetClientAimTarget(client, false) != weapon) return false;
	else if (DoesClientHaveWeaponToDrop(client, weapon)) return true;
	else return false;
}

bool IsClientInRange(int client, int weapon)
{
	if (!IsValidEntity(weapon) || !IsValidEntity(client)) return false;
	float clientPos[3], weaponPos[3];

	GetClientEyePosition(client, clientPos);
	GetEntPropVector(weapon, Prop_Send, "m_vecOrigin", weaponPos);

	if (FloatAbs(clientPos[0] - weaponPos[0]) > 155 || FloatAbs(clientPos[1] - weaponPos[1]) > 155) return false;
	if (GetVectorDistance(clientPos, weaponPos, true) > 22500) return false; //22500 is 150 squared

	return true;
}