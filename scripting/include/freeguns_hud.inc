#pragma newdecls required
#pragma semicolon 1
//todo: comment out pragmas b4 release
#define __freeguns_hud_included

ConVar hudVar;

#if !defined __freeguns_glow_included
	bool mapLoaded;
	public void OnMapStart() { mapLoaded = true; }
	public void OnMapEnd() { mapLoaded = false; }
#endif

#define WORLDTEXTHEIGHT 40.0

void HudOnEntityCreated(int entity, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon") || !enabledVar.BoolValue || !hudVar.BoolValue || !mapLoaded)
		return;

	RequestFrame(DroppedWeaponNextFrame, EntIndexToEntRef(entity));
}

void DroppedWeaponNextFrame(int entityref)
{
	int worldtext = CreateEntityByName("point_worldtext");

	char messagetext[256], clientname[MAX_NAME_LENGTH], qualityname[16], itemname[64];

	TF2Econ_GetQualityName(GetEntProp(entityref, Prop_Send, "m_iEntityQuality"), qualityname, sizeof qualityname);
	qualityname[0] = CharToUpper(qualityname[0]);

	TF2Econ_GetItemName(GetEntProp(entityref, Prop_Send, "m_iItemDefinitionIndex"), itemname, sizeof itemname);

	//remove The at the beginning of weapons
	if (StrContains(itemname, "the", false) == 0)
		ReplaceStringEx(itemname, sizeof itemname, "the ", "", _, _, false);

	int accountid = GetEntProp(entityref, Prop_Send, "m_iAccountID");
	for (int client = 1; client < MaxClients; client++)
	{
		if (GetSteamAccountID(client) == accountid)
		{
			GetClientName(client, clientname, sizeof clientname);
			break;
		}
	}

	if (GetEntProp(entityref, Prop_Send, "m_iEntityQuality") == 0)
		Format(messagetext, sizeof messagetext, "Press Action Button (def. H) to pick up\n%s's %s!", clientname, itemname);
	else
		Format(messagetext, sizeof messagetext, "Press Action Button (def. H) to pick up\n%s's %s %s!", clientname, qualityname, itemname);

	DispatchKeyValue(worldtext, "message", messagetext);
	SetVariantString("!activator");
	AcceptEntityInput(worldtext, "SetParent", entityref);
	SetEntPropVector(worldtext, Prop_Send, "m_vecOrigin", {0.0, 0.0, WORLDTEXTHEIGHT});
	DispatchKeyValueInt(worldtext, "font", 8);
	DispatchKeyValueInt(worldtext, "textsize", 6);
	DispatchKeyValueInt(worldtext, "orientation", 1);
	DispatchKeyValue(worldtext, "color", "255 255 0 255");
	DispatchSpawn(worldtext);

	SDKHook(worldtext, SDKHook_SetTransmit, OnHudTransmit);

}

Action OnHudTransmit(int worldtext, int client)
{
	int weapon = GetEntPropEnt(worldtext, Prop_Data, "m_hParent");

	if (CanClientPickUpWeapon(client, weapon))
	{
		MoveWorldtextAboveParent(worldtext, weapon);
		return Plugin_Continue;
	}
	else return Plugin_Handled;
}

float lastTimeMoved;

void MoveWorldtextAboveParent(int worldtext, int weapon)
{
	if (GetGameTime() > lastTimeMoved + 0.2)
	{
		lastTimeMoved = GetGameTime();

		float weaponpos[3];
		GetEntPropVector(weapon, Prop_Data, "m_vecAbsOrigin", weaponpos);
		weaponpos[2] += WORLDTEXTHEIGHT;
		AcceptEntityInput(worldtext, "ClearParent");
		SetEntPropVector(worldtext, Prop_Send, "m_vecOrigin", weaponpos);
		SetVariantString("!activator");
		AcceptEntityInput(worldtext, "SetParent", weapon);

	}
}


bool CanClientPickUpWeapon(int client, int weapon)
{
	if (!IsClientInRange(client, weapon)) return false;
	if (GetClientAimTarget(client, false) == weapon) return true;
	else return false;
}

bool IsClientInRange(int client, int weapon)
{
	float clientPos[3], weaponPos[3];

	GetClientEyePosition(client, clientPos);
	GetEntPropVector(weapon, Prop_Send, "m_vecOrigin", weaponPos);

	if (FloatAbs(clientPos[0] - weaponPos[0]) > 155 || FloatAbs(clientPos[1] - weaponPos[1]) > 155) return false;
	if (GetVectorDistance(clientPos, weaponPos, true) > 22500) return false; //22500 is 150 squared

	return true;
}