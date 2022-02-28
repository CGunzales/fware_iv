#include "Misc.h"

#include "../Vars.h"
#include "../ChatInfo/ChatInfo.h"
#include "../../Utils/Timer/Timer.hpp"
#include "../PlayerResource/PlayerResource.h"

//#define GET_INDEX_USERID(userid) g_Interfaces.Engine->GetPlayerForUserID(userid)

extern int attackStringW;
extern int attackStringH;

void CMisc::Run(CUserCmd* pCmd)
{
	if (const auto& pLocal = g_EntityCache.m_pLocal)
	{
		AutoJump(pCmd, pLocal);
		AutoStrafe(pCmd, pLocal);
		NoiseMakerSpam(pLocal);
		ExtendFreeze(pLocal);
	}

	ChatSpam();
	CheatsBypass();
	NoPush();
	PingReducer();
	ServerHitbox(); // super secret deathpole feature!!!!
}

void CMisc::ServerHitbox()
{
	// draw our serverside hitbox on local servers, used to test fakelag & antiaim
	if (g_Interfaces.Input->CAM_IsThirdPerson() && Vars::Visuals::ThirdPersonServerHitbox.m_Var)
	{
		using GetServerAnimating_t = void* (*)(int);
		static auto GetServerAnimating = reinterpret_cast<GetServerAnimating_t>(g_Pattern.Find(
			XorStr(L"server.dll").c_str(), XorStr(L"55 8B EC 8B 55 ? 85 D2 7E ? A1").c_str()));
		using DrawServerHitboxes_t = void(__thiscall*)(void*, float, bool);
		static auto DrawServerHitboxes = reinterpret_cast<DrawServerHitboxes_t>(g_Pattern.Find(
			XorStr(L"server.dll").c_str(),
			XorStr(
				L"55 8B EC 83 EC ? 57 8B F9 80 BF ? ? ? ? ? 0F 85 ? ? ? ? 83 BF ? ? ? ? ? 75 ? E8 ? ? ? ? 85 C0 74 ? 8B CF E8 ? ? ? ? 8B 97")
			.c_str()));
		auto pLocal = g_Interfaces.EntityList->GetClientEntity(g_Interfaces.Engine->GetLocalPlayer());
		if (pLocal && pLocal->IsAlive())
		{
			void* server_animating = GetServerAnimating(pLocal->GetIndex());
			if (server_animating)
			{
				DrawServerHitboxes(server_animating, g_Interfaces.GlobalVars->interval_per_tick * 2.f, false);
			}
		}
	}
}

/*
void CMisc::InstantRespawnMVM() {
	if (g_Interfaces.Engine->IsInGame() && g_Interfaces.Engine->GetLocalPlayer() && !g_EntityCache.m_pLocal->IsAlive() && Vars::Misc::MVMRes.m_Var) {
		auto kv = new KeyValues("MVM_Revive_Response");
		kv->SetInt("accepted", 1);
		g_Interfaces.Engine->ServerCmdKeyValues(kv);
	}
}*/

void CMisc::CheatsBypass()
{
	static bool cheatset = false;
	ConVar* sv_cheats = g_Interfaces.CVars->FindVar("sv_cheats");
	if (Vars::Misc::CheatsBypass.m_Var && sv_cheats)
	{
		sv_cheats->SetValue(1);
		cheatset = true;
	}
	else
	{
		if (cheatset)
		{
			sv_cheats->SetValue(0);
			cheatset = false;
		}
	}
}

void CMisc::PingReducer() {
	const ConVar* cl_cmdrate = g_Interfaces.CVars->FindVar("cl_cmdrate");
	CNetChannel* netChannel = g_Interfaces.Engine->GetNetChannelInfo();
	if (cl_cmdrate == nullptr || netChannel == nullptr) { return; }

	static Timer updateRateTimer{ };
	if (updateRateTimer.Run(500)) {
		if (Vars::Misc::PingReducer.m_Var) {
			const int currentPing = g_PR->GetPing(g_Interfaces.Engine->GetLocalPlayer());
			NET_SetConVar cmd("cl_cmdrate", (Vars::Misc::PingTarget.m_Var <= currentPing) ? "-1" : std::to_string(cl_cmdrate->GetInt()).c_str());
			netChannel->SendNetMsg(cmd);
		}
		else {
			NET_SetConVar cmd("cl_cmdrate", std::to_string(cl_cmdrate->GetInt()).c_str());
			netChannel->SendNetMsg(cmd);
		}
	}
}

void CMisc::ExtendFreeze(CBaseEntity* pLocal)
{
	if (Vars::Misc::ExtendFreeze.m_Var && g_Interfaces.Engine->IsInGame() && !pLocal->IsAlive()) {
		static Timer cmdTimer{ };
		if (cmdTimer.Run(2000)) {
			g_Interfaces.Engine->ClientCmd_Unrestricted("extendfreeze");
		}
	}
}

void CMisc::EdgeJump(CUserCmd* pCmd, const int nOldFlags)
{
	if ((nOldFlags & FL_ONGROUND) && Vars::Misc::EdgeJump.m_Var)
	{
		if (Vars::Misc::EdgeJumpKey.m_Var == 0x0)
		{
			if (const auto& pLocal = g_EntityCache.m_pLocal)
			{
				if (pLocal->IsAlive() && !pLocal->IsOnGround() && !pLocal->IsSwimming())
					pCmd->buttons |= IN_JUMP;
			}
		}
		else
		{
			if (const auto& pLocal = g_EntityCache.m_pLocal)
			{
				if (pLocal->IsAlive() && !pLocal->IsOnGround() && !pLocal->IsSwimming() && GetAsyncKeyState(
					Vars::Misc::EdgeJumpKey.m_Var))
					pCmd->buttons |= IN_JUMP;
			}
		}
	}
}

void CMisc::NoPush()
{
	ConVar* noPush = g_Interfaces.CVars->FindVar("tf_avoidteammates_pushaway");
	noPush->SetValue(Vars::Misc::NoPush.m_Var ? 0 : 1);
}

void CMisc::AutoJump(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	if (!Vars::Misc::AutoJump.m_Var
		|| !pLocal->IsAlive()
		|| pLocal->IsSwimming()
		|| pLocal->IsInBumperKart()
		|| pLocal->IsAGhost())
		return;

	static bool bJumpState = false;

	if (pCmd->buttons & IN_JUMP)
	{
		if (!bJumpState && !pLocal->IsOnGround())
		{
			pCmd->buttons &= ~IN_JUMP;
		}

		else if (bJumpState)
		{
			bJumpState = false;
		}
	}

	else if (!bJumpState)
	{
		bJumpState = true;
	}
}

static float angleDiffRad(float a1, float a2) noexcept
{
	float delta = Utils::NormalizeRad(a1 - a2);
	if (a1 > a2)
	{
		if (delta >= PI) { delta -= PI * 2; }	
	}
	else
	{
		if (delta <= -PI) { delta += PI * 2; }
	}
	return delta;
}

void CMisc::AutoStrafe(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	if (Vars::Misc::AutoStrafe.m_Var == 1) // Normal
	{
		if (pLocal->IsAlive() && !pLocal->IsSwimming() && !pLocal->IsOnGround() && (pCmd->mousedx > 1 || pCmd->
			mousedx < -1))

			pCmd->sidemove = pCmd->mousedx > 1 ? 450.f : -450.f;
	}
	if (Vars::Misc::AutoStrafe.m_Var == 2) // Directional
	{
#
		static bool was_jumping = false;
		bool is_jumping = pCmd->buttons & IN_JUMP;


		if (!pLocal->IsOnGround() && (!is_jumping || was_jumping) && !pLocal->IsSwimming())
		{
			const float speed = pLocal->GetVelocity().Length2D();
			auto vel = pLocal->GetVelocity();

			if (speed < 2.0f)
				return;

			constexpr auto perfectDelta = [](float speed) noexcept
			{
				if (const auto& pLocal = g_EntityCache.m_pLocal)
				{
					static auto speedVar = pLocal->GetMaxSpeed();
					static auto airVar = g_Interfaces.CVars->FindVar(_("sv_airaccelerate"));

					static auto wishSpeed = 30.0f;

					const auto term = wishSpeed / airVar->GetFloat() / speedVar * 100.f / speed;

					if (term < 1.0f && term > -1.0f)
						return acosf(term);
				}
				return 0.0f;
			};

			const float pDelta = perfectDelta(speed);
			if (pDelta)
			{
				const float yaw = DEG2RAD(pCmd->viewangles.y);
				const float velDir = atan2f(vel.y, vel.x) - yaw;
				const float wishAng = atan2f(-pCmd->sidemove, pCmd->forwardmove);
				const float delta = angleDiffRad(velDir, wishAng);

				g_Draw.String(FONT_MENU, g_ScreenSize.c, (g_ScreenSize.h / 2) - 50, {255, 64, 64, 255},
				              ALIGN_CENTERHORIZONTAL, _(L"Was jumping: %i"), was_jumping);
				g_Draw.String(FONT_MENU, g_ScreenSize.c, (g_ScreenSize.h / 2) - 70, {255, 64, 64, 255},
				              ALIGN_CENTERHORIZONTAL, _(L"Is jumping: %i"), is_jumping);

				float moveDir = delta < 0.0f ? velDir + pDelta : velDir - pDelta;

				pCmd->forwardmove = cosf(moveDir) * 450.f;
				pCmd->sidemove = -sinf(moveDir) * 450.f;
			}
		}
		was_jumping = is_jumping;
	}
}

void CMisc::InitSpamKV(void* pKV)
{
	char chCommand[30] = "use_action_slot_item_server";
	using HashFunc_t = int(__cdecl*)(const char*, bool);

	static DWORD dwHashFunctionLocation = g_Pattern.Find(_(L"client.dll"), _(L"FF 15 ? ? ? ? 83 C4 08 89 06 8B C6"));
	static auto SymbForString = (HashFunc_t)**(PDWORD*)(dwHashFunctionLocation + 0x2);

	int nAddr = 0;
	while (nAddr < 29)
	{
		switch (nAddr)
		{
		case 0:
			*(PDWORD)((DWORD)pKV + nAddr) = SymbForString(chCommand, true);
			break;
		case 16:
			*(PDWORD)((DWORD)pKV + nAddr) = 0x10000;
			break;
		default:
			*(PDWORD)((DWORD)pKV + nAddr) = 0;
			break;
		}

		nAddr += 4;
	}
}

void CMisc::NoiseMakerSpam(CBaseEntity* pLocal)
{
	if (!Vars::Misc::NoisemakerSpam.m_Var)
		return;

	if (pLocal->GetUsingActionSlot())
		return;

	if (pLocal->GetNextNoiseMakerTime() < g_Interfaces.GlobalVars->curtime)
	{
		if (const auto pKV = Utils::InitKeyValue())
		{
			InitSpamKV(pKV);
			g_Interfaces.Engine->ServerCmdKeyValues(pKV);
		}
	}
}

void CMisc::BypassPure()
{
	if (Vars::Misc::BypassPure.m_Var)
	{
		static DWORD dwAddress = 0x0;

		while (!dwAddress)
			dwAddress = g_Pattern.Find(_(L"engine.dll"), _(L"A1 ? ? ? ? 56 33 F6 85 C0"));

		static DWORD* pPure = nullptr;

		while (!pPure)
		{
			if (reinterpret_cast<DWORD**>(dwAddress + 0x01))
				pPure = *reinterpret_cast<DWORD**>(dwAddress + 0x01);
		}

		if (*pPure)
			*pPure = 0x0;
	}
}

const std::string spam_Fed[] = {
	_("Fedoraware - github.com/M-FeD/Fedoraware"),
	_("Fedoraware - Best free and open-source cheat!"),
	_("Fedoraware - One tip ahead of the game!"),
	_("Fedoraware - Now available @ github.com/M-FeD!"),
	_("Fedoraware - Based on SEOwned public source!")
};

const std::string spam_Lbox[] = {
	_("GET GOOD, GET LMAOBOX!"),
	_("LMAOBOX - WAY TO THE TOP"),
	_("WWW.LMAOBOX.NET - BEST FREE TF2 HACK!")
};

const std::string spam_CH[] = {
	_("Cathook - more fun than a ball of yarn!"),
	_("GNU/Linux is the best OS!"),
	_("Visit https://cathook.club for more information!"),
	_("Cathook - Free and Open-Source tf2 cheat!"),
	_("Cathook - ca(n)t stop me meow!")
};

std::string GetSpam(const int nMode)
{
	std::string str;

	switch (nMode)
	{
	case 2: str = spam_Lbox[Utils::RandIntSimple(0, ARRAYSIZE(spam_Lbox) - 1)];
		break;
	case 3: str = spam_CH[Utils::RandIntSimple(0, ARRAYSIZE(spam_CH) - 1)];
		break;
	default: str = spam_Fed[Utils::RandIntSimple(0, ARRAYSIZE(spam_Fed) - 1)];
		break;
	}

	Utils::ReplaceSpecials(str);
	return "say " + str;
}

void CMisc::ChatSpam()
{
	if (Vars::Misc::ChatSpam.m_Var == 0)
		return;

	float flCurTime = g_Interfaces.Engine->Time();
	static float flNextSend = 0.0f;

	if (flCurTime > flNextSend)
	{
		g_Interfaces.Engine->ClientCmd_Unrestricted(GetSpam(Vars::Misc::ChatSpam.m_Var).c_str());
		flNextSend = (flCurTime + 4.0f);
	}
}

void CMisc::AutoRocketJump(CUserCmd* pCmd)
{
	if (!Vars::Misc::AutoRocketJump.m_Var || !g_GlobalInfo.m_bWeaponCanAttack || !GetAsyncKeyState(VK_RBUTTON))
		return;

	if (g_Interfaces.EngineVGui->IsGameUIVisible() || g_Interfaces.Surface->IsCursorVisible())
		return;

	if (const auto& pLocal = g_EntityCache.m_pLocal)
	{
		if (pLocal->GetClassNum() != CLASS_SOLDIER || !pLocal->IsOnGround() || pLocal->IsDucking())
			return;

		if (const auto& pWeapon = g_EntityCache.m_pLocalWeapon)
		{
			if (pWeapon->IsInReload())
			{
				pCmd->buttons |= IN_ATTACK;
				return;
			}
			if (pCmd->buttons & IN_ATTACK)
				pCmd->buttons &= ~IN_ATTACK;

			if (g_GlobalInfo.m_nCurItemDefIndex == Soldier_m_TheBeggarsBazooka
				|| g_GlobalInfo.m_nCurItemDefIndex == Soldier_m_TheCowMangler5000
				|| pWeapon->GetSlot() != SLOT_PRIMARY)
				return;

			if (pLocal->GetViewOffset().z < 60.05f)
			{
				pCmd->buttons |= IN_ATTACK | IN_JUMP;

				Vec3 vVelocity = pLocal->GetVelocity();
				Vec3 vAngles = { vVelocity.IsZero() ? 89.0f : 45.0f, Math::VelocityToAngles(vVelocity).y - 180.0f, 0.0f };

				if (g_GlobalInfo.m_nCurItemDefIndex != Soldier_m_TheOriginal && !vVelocity.IsZero())
				{
					Vec3 vForward = {}, vRight = {}, vUp = {};
					Math::AngleVectors(vAngles, &vForward, &vRight, &vUp);
					Math::VectorAngles((vForward * 23.5f) + (vRight * -5.6f) + (vUp * -3.0f), vAngles);
				}

				Math::ClampAngles(vAngles);
				pCmd->viewangles = vAngles;
				g_GlobalInfo.m_bSilentTime = true;
			}

			else pCmd->buttons |= IN_DUCK;
		}
	}
}

void CMisc::SteamRPC()
{
	if (!Vars::Misc::Steam::EnableRPC.m_Var)
	{
		if (steamCleared == false) //stupid way to return back to normal rpc
		{
			g_SteamInterfaces.Friends015->SetRichPresence("steam_display", "");
			//this will only make it say "Team Fortress 2" until the player leaves/joins some server. its bad but its better than making 1000 checks to recreate the original
			steamCleared = true;
		}
		return;
	}

	steamCleared = false;
	g_SteamInterfaces.Friends015->SetRichPresence("steam_display", "#TF_RichPresence_Display");

	/*
	"TF_RichPresence_State_MainMenu"              "Main Menu"
	"TF_RichPresence_State_SearchingGeneric"      "Searching for a Match"
	"TF_RichPresence_State_SearchingMatchGroup"   "Searching - %matchgrouploc_token%"
	"TF_RichPresence_State_PlayingGeneric"        "In Match - %currentmap%"
	"TF_RichPresence_State_LoadingGeneric"        "Joining Match"
	"TF_RichPresence_State_PlayingMatchGroup"     "%matchgrouploc_token% - %currentmap%" <--!!!! used
	"TF_RichPresence_State_LoadingMatchGroup"     "Joining %matchgrouploc_token%"
	"TF_RichPresence_State_PlayingCommunity"      "Community - %currentmap%"
	"TF_RichPresence_State_LoadingCommunity"      "Joining Community Server"
	*/
	if (!g_Interfaces.Engine->IsInGame() && Vars::Misc::Steam::OverrideMenu.m_Var)
		g_SteamInterfaces.Friends015->SetRichPresence("state", "MainMenu");
	else
	{
		g_SteamInterfaces.Friends015->SetRichPresence("state", "PlayingMatchGroup");

		switch (Vars::Misc::Steam::MatchGroup.m_Var)
		{
		case 0:
			g_SteamInterfaces.Friends015->SetRichPresence("matchgrouploc", "SpecialEvent");
			break;
		case 1:
			g_SteamInterfaces.Friends015->SetRichPresence("matchgrouploc", "MannUp");
			break;
		case 2:
			g_SteamInterfaces.Friends015->SetRichPresence("matchgrouploc", "Competitive6v6");
			break;
		case 3:
			g_SteamInterfaces.Friends015->SetRichPresence("matchgrouploc", "Casual");
			break;
		case 4:
			g_SteamInterfaces.Friends015->SetRichPresence("matchgrouploc", "BootCamp");
			break;
		default:
			g_SteamInterfaces.Friends015->SetRichPresence("matchgrouploc", "SpecialEvent");
			break;
		}
	}

	/*
	"TF_RichPresence_MatchGroup_Competitive6v6"   "Competitive"
	"TF_RichPresence_MatchGroup_Casual"           "Casual"
	"TF_RichPresence_MatchGroup_SpecialEvent"     "Special Event"
	"TF_RichPresence_MatchGroup_MannUp"           "MvM Mann Up"
	"TF_RichPresence_MatchGroup_BootCamp"         "MvM Boot Camp"
	*/
	switch (Vars::Misc::Steam::MapText.m_Var)
	{
	case 0:
		if (Vars::Misc::Steam::CustomText.m_Var.empty())
			g_SteamInterfaces.Friends015->SetRichPresence("currentmap", "Fedoraware");
		else
			g_SteamInterfaces.Friends015->SetRichPresence("currentmap", Vars::Misc::Steam::CustomText.m_Var.c_str());
		break;
	case 1:
		g_SteamInterfaces.Friends015->SetRichPresence("currentmap", "Fedoraware");
		break;
	case 2:
		g_SteamInterfaces.Friends015->SetRichPresence("currentmap", "Figoraware");
		break;
	case 3:
		g_SteamInterfaces.Friends015->SetRichPresence("currentmap", "Meowhook.club");
		break;
	case 4:
		g_SteamInterfaces.Friends015->SetRichPresence("currentmap", "Rathook.cc");
		break;
	case 5:
		g_SteamInterfaces.Friends015->SetRichPresence("currentmap", "Nitro.tf");
		break;
	default:
		g_SteamInterfaces.Friends015->SetRichPresence("currentmap", "Fedoraware");
		break;
	}

	g_SteamInterfaces.Friends015->SetRichPresence("steam_player_group_size",
	                                              std::to_string(Vars::Misc::Steam::GroupSize.m_Var).c_str());
}

// Myzarfin added this
void Notify::Think()
{
	int x{1}, y{1}, size{20};
	Color_t color;
	float left;

	if (m_notify_text.size() > (MAX_NOTIFY_SIZE + 1))
		m_notify_text.erase(m_notify_text.begin());

	for (size_t i{}; i < m_notify_text.size(); ++i)
	{
		auto notify = m_notify_text[i];

		notify->m_time -= g_Interfaces.GlobalVars->absoluteframetime;

		if (notify->m_time <= 0.f)
		{
			m_notify_text.erase(m_notify_text.begin() + i);
		}
	}

	if (m_notify_text.empty())
		return;

	for (size_t i{}; i < m_notify_text.size(); ++i)
	{
		auto notify = m_notify_text[i];

		left = notify->m_time;
		color = notify->m_color;

		if (left < .5f)
		{
			float f = left;
			Math::Clamp(f, 0.f, .5f);

			f /= .5f;

			color.a = static_cast<int>(f * 255.f);

			if (i == 0 && f < 0.2f)
				y -= size * (1.f - f / 0.2f);
		}

		else
			color.a = 255;

		const size_t cSize = strlen(notify->m_text.c_str()) + 1;
		auto wc = new wchar_t[cSize];
		mbstowcs(wc, notify->m_text.c_str(), cSize);

		int w, h;
		// g_Interfaces.Surface->GetTextSize(FONT_INDICATORS, wc, w, h);

		g_Interfaces.Surface->GetTextSize(FONT_INDICATORS, wc, w, h);
		// there was no need to do what u did to the font system mfed

		g_Draw.Line(x, y, x, y + 19, {
			            Colors::NotifOutline.r, Colors::NotifOutline.g, Colors::NotifOutline.b, color.a
		            });
		g_Draw.GradientRectA(x + 1, y, w / 3 + 9, y + 19,
		                     {
			                     Colors::NotifBG.r, Colors::NotifBG.g,
			                     Colors::NotifBG.b, color.a
		                     }, {
			                     Colors::NotifBG.r, Colors::NotifBG.g,
			                     Colors::NotifBG.b, 1
		                     }, true);
		g_Draw.String(FONT_INDICATORS, x + 6, y + 2,
		              {Colors::NotifText.r, Colors::NotifText.g, Colors::NotifText.b, color.a},
		              ALIGN_DEFAULT, notify->m_text.c_str());

		y += size;
	}
}
