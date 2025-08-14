#pragma semicolon 1
#pragma newdecls required
#include <sdktools>
#include <tf2_stocks>

#include <sdkhooks>

//no longer works in x64?
//#include <tf_econ_data>

#define PLUGIN_VERSION "2.0"

public Plugin myinfo =
{
	name = "[TF2] Freeguns",
	author = "Stinky Lizard",
	description = "Kill your enemy. Steal their stuff. No class restrictions. Change settings with the sm_freeguns cvars.",
	version = PLUGIN_VERSION,
	url = "github.com/stinky-lizard/tf2-freeguns"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	EngineVersion g_engineversion = GetEngineVersion();
	if (g_engineversion != Engine_TF2)
	{
		SetFailState("Freeguns was made for use with Team Fortress 2 only.");
		return APLRes_Failure;
	}
	return APLRes_Success;
}

Handle hSDKCallGetBaseEntity;
Handle hSDKCallUpdateHands;

ConVar enabledVar;

#include <freeguns_hud>
#include <freeguns>
// #define DEBUG

public void OnEntityCreated(int entity, const char[] classname)
{

	#if defined __freeguns_hud_included
		HudOnEntityCreated(entity, classname);
	#endif
}

public void OnPluginStart()
{

	CreateConVar("sm_freeguns_version", PLUGIN_VERSION, "Standard plugin version ConVar. Please don't change me!", FCVAR_REPLICATED|FCVAR_NOTIFY|FCVAR_DONTRECORD);
	enabledVar = CreateConVar("sm_freeguns_enabled", "1", "Enable/disable Freeguns. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);

	RegConsoleCmd("sm_actionbutton", OnActionButtonCmd, "Print how to find your action button binding.");
	
	#if defined __freeguns_hud_included
		hudVar = CreateConVar("sm_freeguns_hud", "1", "Enable/disable custom weapon HUD element. Change to 1 to enable, or 0 to disable.", FCVAR_REPLICATED|FCVAR_NOTIFY);
		LoadTranslations("freeguns.phrases");
	#endif

	GameData hGameConf = new GameData("tf2.freeguns");

	if(hGameConf == INVALID_HANDLE) SetFailState("FreeGuns: Gamedata not found!");

	StartPrepSDKCall(SDKCall_Raw);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CBaseEntity::GetBaseEntity");
	PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	hSDKCallGetBaseEntity = EndPrepSDKCall();
	if (!hSDKCallGetBaseEntity) SetFailState("Failed to setup SDKCall for GetBaseEntity. (Error code 101)");

	StartPrepSDKCall(SDKCall_Entity);
	PrepSDKCall_SetFromConf(hGameConf, SDKConf_Virtual, "CTFWeaponBase::UpdateHands");
	hSDKCallUpdateHands = EndPrepSDKCall();
	if (!hSDKCallUpdateHands) SetFailState("Failed to setup SDKCall for UpdateHands. (Error code 102)");

	if (GetConVarBool(enabledVar)) EnableDetours();

	enabledVar.AddChangeHook(EnabledVarChanged);
}

Action OnActionButtonCmd(int client, int args)
{
	if (GetUserAdmin(client) == INVALID_ADMIN_ID)
		ReplyToCommand(client, "Run in your console `key_findbinding +use_action_slot_item` to find your Action Button!");
	else PrintToChatAll("Run in your console `key_findbinding +use_action_slot_item` to find your Action Button!");
	return Plugin_Handled;
}

void EnabledVarChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if (StringToInt(oldValue) == 0 && StringToInt(newValue) != 0)
	{
		//it was false, now it's true
		LogMessage("Enabling Freeguns.");
		EnableDetours();
	}
	else if (StringToInt(oldValue) != 0 && StringToInt(newValue) == 0)
	{
		//it was true, now it's false
		LogMessage("Disabling Freeguns.");
		DisableDetours();
	}
}

void EnableDetours()
{
	if (native_enableDetours() == -1)
	{
		SetFailState("Could not enable Freeguns detours! (Error code 11)");
	}
}

void DisableDetours()
{
	native_disableDetours();
}

bool DroppedWeaponIsDisabled(int droppedWeaponEnt)
{
	if (!IsValidEntity(droppedWeaponEnt)) return false;
	char classname[64];
	GetEntityClassname(droppedWeaponEnt, classname, sizeof classname);
	if (!StrEqual(classname, "tf_dropped_weapon")) return false;

	//TODO: fix this... not sure how though???

	// int weaponItemDefinitionIndex = GetEntProp(droppedWeaponEnt, Prop_Send, "m_iItemDefinitionIndex");
	// char weaponItemDefClassname[64];
	// //TF2Econ_GetItemClassName(weaponItemDefinitionIndex, weaponItemDefClassname, sizeof weaponItemDefClassname);
	// if
	// (
	// 	StrEqual(weaponItemDefClassname, "tf_weapon_revolver")
	// 	|| StrEqual(weaponItemDefClassname, "tf_weapon_sapper")
	// 	|| StrEqual(weaponItemDefClassname, "tf_weapon_builder")
	// 	|| StrEqual(weaponItemDefClassname, "tf_weapon_knife")
	// )
	// {
	// 	//revolver, sapper, and builder are disabled
	// 	return true;
	// }
	return false;
}
