#pragma newdecls required
#pragma semicolon 1
//todo: comment out pragmas b4 release
#define __freeguns_hud_included

ConVar hudVar, timerVar;

#if !defined __freeguns_glow_included
	bool mapLoaded;
	public void OnMapStart() { mapLoaded = true; }
	public void OnMapEnd() { mapLoaded = false; }
#endif

void HudOnEntityCreated(int entity, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon") || !enabledVar.BoolValue || !hudVar.BoolValue || !mapLoaded)
		return;

	RequestFrame(OnNextFrame, EntIndexToEntRef(entity));

	CreateTimer(timerVar.FloatValue, CheckWeaponLookedAt, EntIndexToEntRef(entity), TIMER_REPEAT);
}

void OnNextFrame(int entityRef)
{
	SaveWeaponData(entityRef);

	//change the item to technically be a saxxy
	//the saxxy is an all-class weapon (one of 3 in the game)
	//so every player will be able to see the glow and hud! no matter their class!
	//could also use golden frying pan or prinny machete
	SetEntProp(entityRef, Prop_Send, "m_iItemDefinitionIndex", 423);
}

Action CheckWeaponLookedAt(Handle timer, int entityRef)
{
	if (!IsValidEntity(entityRef)) return Plugin_Stop;
	int entity = EntRefToEntIndex(entityRef);

	for (int client = 1; client < MaxClients; client++)
	{
		if (IsClientInGame(client) && !IsFakeClient(client))
		{
			if (GetClientAimTarget(client, false) == entity)
			{
				//they're looking at us
				//is this new?
				//make the name right
				//add the glow
			}

		}

	}

	return Plugin_Continue;
}

void SaveWeaponData(int weaponRef)
{
	char refStr[32];
	IntToString(weaponRef, refStr, sizeof refStr);

	savedData.JumpToKey("Weapons", true);
	savedData.JumpToKey(refStr, true);
	savedData.SetNum("ItemDefIndex", GetEntProp(weaponRef, Prop_Send, "m_iItemDefinitionIndex"));
	savedData.SetNum("LookedAt", 0);
	
	savedData.Rewind();
}

bool GetWeaponLookedAt(int weaponRef)
{
	bool out;

	char refStr[32];
	IntToString(weaponRef, refStr, sizeof refStr);

	savedData.JumpToKey("Weapons");
	savedData.JumpToKey(refStr);
	if (savedData.GetNum())
	savedData.Rewind();

	return out;
}

void ReturnWeaponToOriginalItem(int weaponRef)
{
	char refStr[32];
	IntToString(weaponRef, refStr, sizeof refStr);

	savedData.JumpToKey("Weapons");
	if (savedData.JumpToKey(refStr))
		SetEntProp(weaponRef, Prop_Send, "m_iItemDefinitionIndex", savedData.GetNum("ItemDefIndex", 423)); //423 is the saxxy

	savedData.Rewind();
}