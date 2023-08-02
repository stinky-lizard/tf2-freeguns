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

bool createWeapon = true;

void HudOnEntityCreated(int entity, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon") || !enabledVar.BoolValue || !hudVar.BoolValue || !mapLoaded)
		return;

	if (createWeapon) RequestFrame(OnNextFrame, entity);

	// SDKHook(entity, SDKHook_ThinkPost, OnWeaponThink);
}

void OnNextFrame(int entity)
{
	PrintToServer("Att: %i", GetEntProp(entity, Prop_Send, "m_iAttributeDefinitionIndex"));
	createWeapon = false;
	int newent = CreateEntityByName("tf_dropped_weapon");
	createWeapon = true;
	SetEntProp(newent, Prop_Send, "m_iItemDefinitionIndex", 30758);
	SetEntProp(newent, Prop_Send, "m_iEntityLevel", GetEntProp(entity, Prop_Send, "m_iEntityLevel"));
	SetEntProp(newent, Prop_Send, "m_iItemIDHigh", GetEntProp(entity, Prop_Send, "m_iItemIDHigh"));
	SetEntProp(newent, Prop_Send, "m_iItemIDLow", GetEntProp(entity, Prop_Send, "m_iItemIDLow"));
	SetEntProp(newent, Prop_Send, "m_iAccountID", GetEntProp(entity, Prop_Send, "m_iAccountID"));
	SetEntProp(newent, Prop_Send, "m_iEntityQuality", GetEntProp(entity, Prop_Send, "m_iEntityQuality"));
	SetEntProp(newent, Prop_Send, "m_bInitialized", GetEntProp(entity, Prop_Send, "m_bInitialized"));
	SetEntProp(newent, Prop_Send, "m_bOnlyIterateItemViewAttributes", GetEntProp(entity, Prop_Send, "m_bOnlyIterateItemViewAttributes"));
	SetEntProp(newent, Prop_Send, "m_iTeamNumber", GetEntProp(entity, Prop_Send, "m_iTeamNumber"));
	float pos[3];
	GetEntPropVector(entity, Prop_Send, "m_vecOrigin", pos);
	if (IsModelPrecached("models/workshop_partner/weapons/c_models/c_prinny_knife/c_prinny_knife.mdl"))
		SetEntityModel(newent, "models/workshop_partner/weapons/c_models/c_prinny_knife/c_prinny_knife.mdl");
	else PrintToServer ("aw :(");
	TeleportEntity(newent, pos);

	SetVariantString("!activator");
	AcceptEntityInput(newent, "SetParent", entity);
	//oh my god holy shit
	SetEntityRenderMode(newent, RENDER_NONE);
	// SetEntPropFloat(newent, Prop_Send, "m_flModelScale", 2.0);

	DispatchSpawn(newent);
}

// public void OnEntityDestroyed(int entity)
// {
// 	for (int client = 1; client < MaxClients; client++)
// 	{
// 		if (!IsClientInGame(client) || IsFakeClient(client))
// 			continue;

// 		ShowVGUIPanel(client, "targetid", panelKv, false);
// 	}
// }

// void OnWeaponThink(int entity)
// {

// }
