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

}
