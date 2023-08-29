#pragma semicolon 1
#pragma newdecls required

#define __freeguns_glow_included
//This file includes the code to make the glow effect.
//Separated for cleanliness.

#if !defined __freeguns_hud_included
	bool mapLoaded;
	public void OnMapStart() { mapLoaded = true; }
	public void OnMapEnd() { mapLoaded = false; }
#endif
	//if hud included is already defined, then these are already declared/defined there
	//usually glow would be included first, but if it's included second then hud would have already done this

ConVar glowVar, glowTimerVar, glowRadVar, glowLookVar;
//I wished there's a more elegant solution than this, but it seems that's all for naught :(
ArrayList allGlowEntities;


enum struct GlowEntity
{
	int entRef;
	int parentRef;
	int playersWithSight[MAXPLAYERS];
}

void GlowOnEntityCreated(int weaponEnt, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon") || !enabledVar.BoolValue || !glowVar.BoolValue || !mapLoaded)
		return;

	GlowEntity glowEnt;

	int glowEntIndex = CreateEntityByName("tf_glow");
	glowEnt.entRef = EntIndexToEntRef(glowEntIndex);

	if (!IsValidEntity(glowEnt.entRef))
	{
		LogError("Freeguns: failed to create glow entity. (Error code 81)");
		return;
	}

	//doesn't work :(
	//// DispatchKeyValueInt(glowEnt, "Mode", 1);
	//// SetEntProp(glowEnt, Prop_Send, "m_iMode", 1);

	SetVariantString("190 197 40 255");
	AcceptEntityInput(glowEnt.entRef, "SetGlowColor");

	char entTargetname[128];
	Format(entTargetname, sizeof entTargetname, "tf_dropped_weapon%i", EntIndexToEntRef(weaponEnt));
	DispatchKeyValue(weaponEnt, "targetname", entTargetname);

	DispatchKeyValue(glowEnt.entRef, "target", entTargetname);

	float weapOrigin[3];
	GetEntPropVector(weaponEnt, Prop_Send, "m_vecOrigin", weapOrigin);
	TeleportEntity(glowEnt.entRef, weapOrigin);

	SetVariantString("!activator");
	AcceptEntityInput(glowEnt.entRef, "SetParent", weaponEnt);
	glowEnt.parentRef = EntIndexToEntRef(weaponEnt);

	DispatchSpawn(glowEnt.entRef);

	//save the GlowEntity
	allGlowEntities.PushArray(glowEnt);

	//Section for visibility
	SetEdictFlags(glowEnt.entRef, GetEdictFlags(glowEnt.entRef) & ~FL_EDICT_ALWAYS);
	SDKHook(glowEnt.entRef, SDKHook_SetTransmit, OnGlowTransmit);

	if (!glowLookVar.BoolValue)
	{
		CreateTimer(glowTimerVar.FloatValue, CalculateLineOfSight, glowEnt.entRef, TIMER_REPEAT);
	}
}

Action CalculateLineOfSight(Handle timer, int glowEntRef)
{
	GlowEntity glowEnt;
	int glowEntArrIndex;

	//have to get the GlowEntity from its ref bc it may have changed

	if (!GetGlowEntOfEntRef(glowEntRef, glowEnt, glowEntArrIndex))
		//there's no GlowEntity, so we don't have anywhere to put the players with sight.
		return Plugin_Stop;

	if (!IsValidEntity(glowEntRef))
	{
		allGlowEntities.Erase(glowEntArrIndex);
		return Plugin_Stop;
	}

	float eyepos[3], glowpos[3];

	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client) || IsFakeClient(client))
			continue;

		GetClientEyePosition(client, eyepos);
		GetEntPropVector(glowEnt.parentRef, Prop_Send, "m_vecOrigin", glowpos);
		
		//Are they close enough?
		//Don't show it if they're too far. If they are, it'll just be too much visual noise

		//Question: Search in a box or search in a sphere?
		//A box is faster, and is probably good enough
		if (FloatAbs(eyepos[0] - glowpos[0]) > glowRadVar.FloatValue || FloatAbs(eyepos[1] - glowpos[1]) > glowRadVar.FloatValue)
		{
			//they're too far; they shouldn't see it
			RemoveClientFromSightArray(client, glowEnt);
			allGlowEntities.SetArray(glowEntArrIndex, glowEnt);
			return Plugin_Continue;
		}

		//Do they have LOS?
		Handle hTrace = TR_TraceRayFilterEx(eyepos, glowpos, MASK_SOLID, RayType_EndPoint, ClientViewsFilter);

		int entindex = TR_GetEntityIndex(hTrace);
		if (entindex == -1)
		{
			//the ray didn't find anything. but it didn't find the worldspawn, so it prob just didnt reach the weapon
			RayHitWeapon(client, glowEnt);
		}
		else if (entindex == EntRefToEntIndex(glowEnt.parentRef))
		{
			RayHitWeapon(client, glowEnt);
		}
		else
		{
			//if this hit, they don't have sight
			RemoveClientFromSightArray(client, glowEnt);
		}

		delete hTrace;
	}

	//save any changes done to the GlowEntity
	allGlowEntities.SetArray(glowEntArrIndex, glowEnt);

	return Plugin_Continue;
}

void RemoveClientFromSightArray(int client, GlowEntity glowEnt, bool force = false)
{	
	if (force)
	{
		glowEnt.playersWithSight[client] = 0;
		return;
	}
	else if (IsClientInSightArray(glowEnt, client))
		glowEnt.playersWithSight[client] = 0;
}

void RayHitWeapon(int client, GlowEntity glowEnt)
{
	//the ray hit the weapon first, not the world, so they have LOS
	int userid = GetClientUserId(client);

	//sanity check
	if (userid == 0) { LogError("Bad news: UserID of client %i is 0!", client); return; }
	if (!IsClientInSightArray(glowEnt, client))
	{
		glowEnt.playersWithSight[client] = userid;
	}
}

Action OnGlowTransmit(int glowEntIndex, int client)
{
	if (!IsValidEntity(glowEntIndex) || !IsValidEntity(client))	return Plugin_Continue; //just let it be
	SetEdictFlags(glowEntIndex, GetEdictFlags(glowEntIndex) & ~FL_EDICT_ALWAYS);

	//This is where we disable certain players from seeing it.
	//May be incredibly resource intensive...

	GlowEntity glowEnt;
	int unneeded;
	GetGlowEntOfEntRef(EntIndexToEntRef(glowEntIndex), glowEnt, unneeded);

	//requested by twilight
	if (glowLookVar.BoolValue)
	{
		int weapon = GetEntPropEnt(glowEntIndex, Prop_Data, "m_hParent");
		if (CanClientPickUpWeapon(client, weapon))
			SetEntProp(glowEntIndex, Prop_Send, "m_bDisabled", 0);
		else
			SetEntProp(glowEntIndex, Prop_Send, "m_bDisabled", 1);
		
		return Plugin_Continue;
	}

	//Are they registered as having LOS?
	if (!IsClientInSightArray(glowEnt, client))
		SetEntProp(glowEntIndex, Prop_Send, "m_bDisabled", 1);

	//Are they alive?
	else if (!IsPlayerAlive(client))
		SetEntProp(glowEntIndex, Prop_Send, "m_bDisabled", 1);

	//Are they a disguised or invisible spy?
	else if (TF2_IsPlayerInCondition(client, TFCond_Disguised) || TF2_IsPlayerInCondition(client, TFCond_Stealthed))
		SetEntProp(glowEntIndex, Prop_Send, "m_bDisabled", 1);
		
	else SetEntProp(glowEntIndex, Prop_Send, "m_bDisabled", 0);

	// If all of these questions are true, then they can see it.
	return Plugin_Continue;
}

void GlowOnClientDisconnect(int client)
{
	for (int i = 0; i < allGlowEntities.Length; i++)
	{
		GlowEntity glowEnt;
		allGlowEntities.GetArray(i, glowEnt);
		if (IsClientInSightArray(glowEnt, client))
		{
			RemoveClientFromSightArray(client, glowEnt);
			allGlowEntities.SetArray(i, glowEnt);
		}
	}
}

bool IsClientInSightArray(GlowEntity glowEnt, int client)
{
	if (!IsClientConnected(client))
	{
		return false;
	}
	else if (glowEnt.playersWithSight[client] == 0)
	{
		return false;
	}
	else if (GetClientOfUserId(glowEnt.playersWithSight[client]) != client)
	{
		//uh oh! this client slot has a different player in it!
		LogError("Warning: GlowEnt sight array wrong!");
		return false;
	}
	else return true;
}

//get the GlowEntity object that corresponds to this tf_glow refrence from allGlowEntities.
//can't just pass the array index around because stuff will be deleted and shifted around in the array.
bool GetGlowEntOfEntRef(int glowEntRef, GlowEntity buffer, int &arrIndex)
{
	arrIndex = allGlowEntities.FindValue(glowEntRef, 0); //GlowEntity.entRef is the first value

	if (arrIndex == -1)
	{
		//uh oh. There is no GlowEntity for this tf_glow.
		LogError("No GlowEntity for tf_glow!");
		return false;
	}

	allGlowEntities.GetArray(arrIndex, buffer);
	return true;
}

bool ClientViewsFilter(int entity, int contentsmask)
{
	if (!IsValidEntity(entity)) return false;

	char clsname[64];
	GetEntityClassname(entity, clsname, sizeof clsname);

	//if it's the world itself
	if (entity == 0) return true;
	//if it's a weapon
	else if (StrEqual(clsname, "tf_dropped_weapon")) return true;
	//ignore everything else
	else return false;
}