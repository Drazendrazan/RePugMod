#include "precompiled.h"

CCaptain gCaptain;

void CCaptain::Clear()
{
	this->m_Picking = UNASSIGNED;

	gTask.Remove(TERRORIST);
	gTask.Remove(CT);

	memset(this->m_Captain, -1, sizeof(m_Captain));

	gTask.Remove(PUG_TASK_LIST);
}

void CCaptain::Init()
{
	this->Clear();

	if (gPlayer.GetNum() >= (int)(gCvars.GetPlayersMin()->value / 2))
	{
		CBasePlayer* Players[32] = { NULL };

		auto Num = gPlayer.GetList(Players);

		for (int i = 0; i < Num; i++)
		{
			auto Player = Players[i];

			if (Player)
			{
				if (Player->m_iTeam != UNASSIGNED)
				{
					Player->CSPlayer()->JoinTeam(SPECTATOR);
				}
			}
		}

		this->SetCaptain(gPlayer.GetRandom(SPECTATOR), TERRORIST);

		this->SetCaptain(gPlayer.GetRandom(SPECTATOR), CT);
		
		gTask.Create(PUG_TASK_LIST, 0.5f, true, (void*)this->List);

		this->Menu((TeamName)RANDOM_LONG(TERRORIST, CT)); 
	} 
	else
	{
		this->Stop();

		gUtil.SayText(0, PRINT_TEAM_RED, _T("The choice of the teams failed: \3Not enough players.\1"));
	}
}

void CCaptain::Stop()
{
	this->Clear();

	gPugMod.NextState(3.0f);
}

void CCaptain::ClientDisconnected(edict_t * pEntity)
{
	if (!FNullEnt(pEntity))
	{
		TeamName Team = this->GetCaptain(ENTINDEX(pEntity));

		if (Team != UNASSIGNED)
		{
			gTask.Remove(TERRORIST);
			gTask.Remove(CT);

			if (gPlayer.GetNum(SPECTATOR) > 0) 
			{
				this->SetCaptain(gPlayer.GetRandom(SPECTATOR), Team);

				this->Menu(this->m_Picking);
			}
			else
			{
				this->Stop();
			} 
		}
	}
}

void CCaptain::SetCaptain(CBasePlayer* Player, TeamName Team)
{
	if (Player)
	{ 
		this->m_Captain[Team] = Player->entindex();

		Player->CSPlayer()->JoinTeam(Team);

		if (!Player->IsAlive())
		{
			Player->RoundRespawn();
		}

		gUtil.SayText(NULL, Player->entindex(), _T("\3%s\1 is captain of \3%s\1"), STRING(Player->edict()->v.netname), PUG_MOD_TEAM_STR[Team]);
	}
}

TeamName CCaptain::GetCaptain(int EntityIndex)
{
	if (this->m_Captain[TERRORIST] == EntityIndex)
	{
		return TERRORIST;
	}

	if (this->m_Captain[CT] == EntityIndex)
	{
		return CT;
	}

	return UNASSIGNED;
}

void CCaptain::GetPlayer(CBasePlayer* Player, CBasePlayer* Target)
{
	if (Player)
	{
		gTask.Remove(TERRORIST);
		gTask.Remove(CT);

		if (Target)
		{
			Target->CSPlayer()->JoinTeam(Player->m_iTeam);
	
			if (!Target->IsAlive()) 
			{
				Target->RoundRespawn();
			}

			this->List();

			gUtil.SayText(NULL, Player->entindex(), _T("\3%s\1 choosed \3%s\1"), STRING(Player->edict()->v.netname), STRING(Target->edict()->v.netname));

			if (this->CheckPlayerCount())
			{
				this->Menu(this->m_Picking == CT ? TERRORIST : CT);
			}
			else
			{
				this->Stop();
			}
		}	
	}
}

bool CCaptain::CheckPlayerCount()
{
	if (g_pGameRules)
	{
		int InGame, NumTerrorist, NumCT, NumSpectator;

		gPlayer.GetNum(true, InGame, NumTerrorist, NumCT, NumSpectator);

		if (NumSpectator)
		{
			int PlayersMin = (int)(gCvars.GetPlayersMin()->value / 2);

			if (NumTerrorist < PlayersMin || NumCT < PlayersMin)
			{
				return true;
			}
		}
	}

	return false;
}

bool CCaptain::GetPicking(int EntityIndex)
{
	if (this->m_Picking == this->GetCaptain(EntityIndex))
	{
		return true;
	}

	return false;
}

void CCaptain::Menu(TeamName Team)
{
	auto Player = UTIL_PlayerByIndexSafe(this->m_Captain[Team]);

	if (Player)
	{
		this->m_Picking = Team;

		gTask.Remove(TERRORIST);
		gTask.Remove(CT);

		if (gPlayer.GetNum(SPECTATOR) == 1 || Player->IsBot())
		{
			this->GetPlayer(Player, gPlayer.GetRandom(SPECTATOR)); 
		}
		else
		{
			auto EntityIndex = Player->entindex();

			gMenu[EntityIndex].Create(PUG_MOD_TEAM_STR[SPECTATOR], false, this->MenuHandle);

			CBasePlayer* Players[32] = { NULL };

			auto Num = gPlayer.GetList(Players, SPECTATOR);

			for(int i = 0;i < Num;i++)
			{
				gMenu[EntityIndex].AddItem(Players[i]->entindex(), STRING(Players[i]->edict()->v.netname));
			}

			gMenu[EntityIndex].Show(EntityIndex);

			gTask.Create(Team, 10.0f, false, this->GetRandomPlayer, (void*)EntityIndex);
		}
	}
}

void CCaptain::MenuHandle(int EntityIndex, int ItemIndex, bool Disabled, const char* Option)
{
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	if (Player)
	{
		auto Target = UTIL_PlayerByIndexSafe(ItemIndex);

		if (Target)
		{
			gCaptain.GetPlayer(Player, Target);
		}
	}
}

void CCaptain::GetRandomPlayer(int EntityIndex)
{
	auto Player = UTIL_PlayerByIndexSafe(EntityIndex);

	if (Player)
	{
		if (!Player->IsDormant())
		{
			if (Player->has_disconnected == false)
			{
				if (gCaptain.GetPicking(EntityIndex))
				{
					gMenu[EntityIndex].Hide(EntityIndex);

					if (gPlayer.GetNum(SPECTATOR))
					{
						gCaptain.GetPlayer(Player, gPlayer.GetRandom(SPECTATOR));
					}
				}
			}
		}
	}
}

void CCaptain::List()
{
	char szList[SPECTATOR + 1][320] = { 0 };

	int Count[SPECTATOR + 1] = { 0 };

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto Player = UTIL_PlayerByIndexSafe(i);

		if (Player)
		{
			if (!FNullEnt(Player->edict()))
			{
				if (!Player->IsDormant())
				{
					if (Player->m_iTeam != UNASSIGNED)
					{
						if (gCaptain.GetCaptain(Player->entindex()) != UNASSIGNED)
						{
							snprintf
							(
								szList[Player->m_iTeam],
								sizeof(szList[Player->m_iTeam]),
								gCaptain.GetPicking(Player->entindex()) ? _T("%s%s (C) *\n") : _T("%s%s (C)\n"),
								szList[Player->m_iTeam],
								STRING(Player->edict()->v.netname)
							);
						}
						else
						{
							snprintf
							(
								szList[Player->m_iTeam],
								sizeof(szList[Player->m_iTeam]),
								"%s%s\n", szList[Player->m_iTeam],
								STRING(Player->edict()->v.netname)
							);
						}

						Count[Player->m_iTeam]++;
					}
				}
			}
		}
	}

	for (int i = 0; i < 5 - Count[CT]; i++)
	{
		snprintf(szList[CT], sizeof(szList[CT]), "%s\n", szList[CT]);
	}

	gUtil.HudMessage(NULL, gUtil.HudParam(0, 255, 0, 0.75, 0.02, 0, 0.0, 53.0, 0.0, 0.0, 1), PUG_MOD_TEAM_STR[TERRORIST]);

	gUtil.HudMessage(NULL, gUtil.HudParam(255, 255, 255, 0.75, 0.02, 0, 0.0, 53.0, 0.0, 0.0, 2), "\n%s", szList[TERRORIST]);

	if (Count[SPECTATOR])
	{
		gUtil.HudMessage(NULL, gUtil.HudParam(0, 255, 0, 0.75, 0.28, 0, 0.0, 53.0, 0.0, 0.0, 3), "%s\n\n\n\n\n\n%s", PUG_MOD_TEAM_STR[CT], PUG_MOD_TEAM_STR[SPECTATOR]);

		gUtil.HudMessage(NULL, gUtil.HudParam(255, 255, 255, 0.75, 0.28, 0, 0.0, 53.0, 0.0, 0.0, 4), "\n%s\n%s", szList[CT], szList[SPECTATOR]);
	}
	else
	{
		gUtil.HudMessage(NULL, gUtil.HudParam(0, 255, 0, 0.75, 0.28, 0, 0.0, 53.0, 0.0, 0.0, 3), PUG_MOD_TEAM_STR[CT]);

		gUtil.HudMessage(NULL, gUtil.HudParam(255, 255, 255, 0.75, 0.28, 0, 0.0, 53.0, 0.0, 0.0, 4), "\n%s", szList[CT]);
	}
}
