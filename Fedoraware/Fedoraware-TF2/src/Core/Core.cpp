#include "Core.h"

#include "../Hooks/HookManager.h"
#include "../Hooks/PatchManager/PatchManager.h"

#include "../Features/NetVarHooks/NetVarHooks.h"
#include "../Features/Visuals/Visuals.h"
#include "../Features/Vars.h"
#include "../Features/TickHandler/TickHandler.h"

#include "../Features/Menu/Menu.h"
#include "../Features/Menu/ConfigManager/ConfigManager.h"
#include "../Features/Items/AttributeChanger/AttributeChanger.h"
#include "../Features/Commands/Commands.h"
#include "../Features/Discord/Discord.h"

#include "../Utils/Events/Events.h"
#include "../Utils/Minidump/Minidump.h"

void LoadDefaultConfig()
{
	// Load default visuals
	g_CFG.LoadVisual(g_CFG.GetCurrentVisuals());
	g_CFG.LoadConfig(g_CFG.GetCurrentConfig());

	g_Draw.RemakeFonts();

	F::Menu.ConfigLoaded = true;
}

void CCore::OnLoaded()
{
	LoadDefaultConfig();

	I::Cvar->ConsoleColorPrintf(Vars::Menu::Colors::MenuAccent.Value, "%s Loaded!\n", Vars::Menu::CheatName.Value.c_str());
	I::EngineClient->ClientCmd_Unrestricted("play vo/demoman_go01.mp3");

	// Check the DirectX version, except i don't want this message
	const int dxLevel = g_ConVars.FindVar("mat_dxlevel")->GetInt();
	if (dxLevel < 80)
	{
		MessageBoxA(nullptr, "Your DirectX version is too low!\nPlease use dxlevel 90 or higher", "dxlevel too low", MB_OK | MB_ICONWARNING);
	}
}

void CCore::Load()
{
	g_SteamInterfaces.Init();
	g_Interfaces.Init();
	g_NetVars.Init();

	// Initialize hooks & memory stuff
	{
		g_HookManager.Init();
		g_PatchManager.Init();
		F::NetHooks.Init();
	}

	g_ConVars.Init();
	F::Ticks.Reset();

	F::Commands.Init();
	F::DiscordRPC.Init();

	g_Events.Setup({
		"vote_cast", "player_changeclass", "player_connect", "player_hurt", "achievement_earned", "player_death", "vote_started", "teamplay_round_start", "player_spawn", "item_pickup"
	}); // all events @ https://github.com/tf2cheater2013/gameevents.txt

	OnLoaded();
}

void CCore::Unload()
{
	I::EngineClient->ClientCmd_Unrestricted("play vo/heavy_no01.mp3");
	G::UnloadWndProcHook = true;
	Vars::Visuals::SkyboxChanger.Value = false;
	Vars::Visuals::ThirdPerson.Value = false;

	Sleep(100);

	g_Events.Destroy();
	g_HookManager.Release();
	g_PatchManager.Restore();

	F::DiscordRPC.Shutdown();

	Sleep(100);

	F::Visuals.RestoreWorldModulation(); //needs to do this after hooks are released cuz UpdateWorldMod in FSN will override it
	I::Cvar->ConsoleColorPrintf(Vars::Menu::Colors::MenuAccent.Value, "%s Unloaded!\n", Vars::Menu::CheatName.Value.c_str());
}

bool CCore::ShouldUnload()
{
	const bool unloadKey = GetAsyncKeyState(VK_F11) & 0x8000;
	return unloadKey && !F::Menu.IsOpen;
}
