
#include <sdkhooks>

//This file includes the code to make the glow effect.
//Separated for cleanliness.

public void OnEntityCreated(int entity, const char[] classname)
{
	if (!StrEqual(classname, "tf_dropped_weapon"))
		return;

	int glowEnt = CreateEntityByName("tf_glow");
	if (!IsValidEntity(glowEnt))
	{
		LogError("Freeguns: failed to create glow entity. (Error number 80)");
		return;
	}

	DispatchKeyValueInt(glowEnt, "Mode", 1);
	SetEntProp(glowEnt, Prop_Send, "m_iMode", 1);

	SetVariantString("190 197 40 255")
	AcceptEntityInput(glowEnt, "SetGlowColor")

	SetVariantString("!activator");
	AcceptEntityInput(glowEnt, "SetParent", entity);

	char entTargetname[128];
	Format(entTargetname, sizeof entTargetname, "tf_dropped_weapon%i", EntIndexToEntRef(entity));
	DispatchKeyValue(entity, "targetname", entTargetname);
	
	DispatchKeyValue(glowEnt, "target", entTargetname);
	
	DispatchSpawn(glowEnt);
}
