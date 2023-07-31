
#include <sdkhooks>

#define __freeguns_glow_included
//This file includes the code to make the glow effect.
//Separated for cleanliness.

#define EF_BONEMERGE 0x001
#define EF_BONEMERGE_FASTCULL 0x080

ConVar glowVar;
//I wished there's a more elegant solution than this, but it seems that's all for naught :(
ArrayList allGlowEntities;

bool mapLoaded;

enum struct GlowEntity
{
	int entRef;
	int parentRef;
	int playersWithSight[MAXPLAYERS];
}


public void OnMapStart() { mapLoaded = true; }
public void OnMapEnd() { mapLoaded = false; }

public void OnEntityCreated(int weaponEnt, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon") || !glowVar.BoolValue || !mapLoaded)
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

	CreateTimer(0.5, CalculateLineOfSight, glowEnt.entRef, TIMER_REPEAT);
}

Action CalculateLineOfSight(Handle timer, int glowEntRef)
{
	GlowEntity glowEnt;
	int glowEntArrIndex;

	if (!GetGlowEntOfEntRef(glowEntRef, glowEnt, glowEntArrIndex))
		//there's no GlowEntity, so we don't have anywhere to put the players with sight.
		return Plugin_Stop;

	if (!IsValidEntity(glowEntRef))
	{
		// glowEnt.entRef = 0;	//unneeded?
		// glowEnt.parentRef = 0;
		// for (int i = 0; i < sizeof glowEnt.playersWithSight; i++) { glowEnt.playersWithSight[i] = 0; }
		allGlowEntities.Erase(glowEntArrIndex);
		return Plugin_Stop;
	}

	float eyepos[3], glowpos[3];

	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client) || IsFakeClient(client))
			continue;
		//Do they have LOS?
		GetClientEyePosition(client, eyepos);

		GetEntPropVector(glowEnt.parentRef, Prop_Send, "m_vecOrigin", glowpos);

		// glowpos[0] += (eyepos[0] - glowpos[0]) * 0.02;
		// glowpos[1] += (eyepos[1] - glowpos[1]) * 0.02;
		// glowpos[2] += (eyepos[2] - glowpos[2]) * 0.02;

		PrintToServer("[%f %f %f]", eyepos[0], eyepos[1], eyepos[2]); //debug
		PrintToServer("[%f %f %f]", glowpos[0], glowpos[1], glowpos[2]);
		Handle hTrace = TR_TraceRayEx(eyepos, glowpos, MASK_VISIBLE, RayType_EndPoint);
		if (TR_DidHit(hTrace))
		{
			PrintToServer("%i does not have sight", client);
			//if it hit, they don't have sight
			int index = FindClientInSightArray(glowEnt, client);
			if (index != -1)
				glowEnt.playersWithSight[index] = 0;
		}
		else
		{
			PrintToServer("%i does have sight", client);
			int userid = GetClientUserId(client);

			//sanity check
			if (userid == 0) { LogError("Bad news: UserID of client %i is 0!", client); delete hTrace; continue; }
			if (FindClientInSightArray(glowEnt, client) == -1)
			{

				for (int i = 0; i < sizeof glowEnt.playersWithSight; i++)
				{
					if (glowEnt.playersWithSight[i] == 0)
					{
						PrintToServer("Adding them to index %i", i);
						glowEnt.playersWithSight[i] = userid;
						break;
					}
				}
			}
		}

		delete hTrace;
	}

	return Plugin_Continue;
}

Action OnGlowTransmit(int glowEntIndex, int client)
{
	SetEdictFlags(glowEntIndex, GetEdictFlags(glowEntIndex) & ~FL_EDICT_ALWAYS);

	//This is where we disable certain players from seeing it.
	//May be incredibly resource intensive...

	SetVariantString("190 197 40 255");

	GlowEntity glowEnt;
	int unneeded;
	GetGlowEntOfEntRef(EntIndexToEntRef(glowEntIndex), glowEnt, unneeded);

	//Are they registered as having LOS?
	if (FindClientInSightArray(glowEnt, client) == -1)
		SetVariantString("190 197 40 0");

	//Are they alive?
	if (!IsPlayerAlive(client))
		SetVariantString("190 197 40 0");

	//Are they a disguised or invisible spy?
	if (TF2_IsPlayerInCondition(client, TFCond_Disguised) || TF2_IsPlayerInCondition(client, TFCond_Stealthed))
		SetVariantString("190 197 40 0");

	AcceptEntityInput(glowEntIndex, "SetGlowColor");

	// If all of these questions are true, then they can see it.
	return Plugin_Continue;
}

int FindClientInSightArray(const GlowEntity glowEnt, int client)
{
	int userid = GetClientUserId(client);
	for (int i = 0; i < sizeof glowEnt.playersWithSight; i++)
	{
		if (glowEnt.playersWithSight[i] == userid)
			return i;
	}
	return -1;
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