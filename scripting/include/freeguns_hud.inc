#pragma newdecls required
#pragma semicolon 1
//todo: comment out pragmas b4 release
#define __freeguns_hud_included

ConVar hudVar;

KeyValues panelKv;

void HudOnEntityCreated(int entity, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon") || !enabledVar.BoolValue || !hudVar.BoolValue || !mapLoaded)
		return;

	SDKHook(entity, SDKHook_ThinkPost, OnWeaponThink);
}

public void OnEntityDestroyed(int entity)
{
	for (int client = 1; client < MaxClients; client++)
	{
		if (!IsClientInGame(client) || IsFakeClient(client))
			continue;

		ShowVGUIPanel(client, "targetid", panelKv, false);
	}
}

void OnWeaponThink(int entity)
{

}
