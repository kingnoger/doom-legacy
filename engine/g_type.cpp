
#include "tnl/tnlBitStream.h"
#include "tnl/tnlGhostConnection.h"

#include "command.h"
#include "cvars.h"

#include "g_type.h"
#include "g_game.h"
#include "g_team.h"
#include "g_player.h"

#include "w_wad.h"


TNL_IMPLEMENT_CLASS(GameType);

GameType::GameType()
{
  gt_name = "Coop";
  gt_version = 0;
  e.game = &game;
}


void GameType::WriteServerQueryResponse(BitStream &s)
{
  s.write(game.demoversion);
  s.writeString(VERSIONSTRING);
  s.writeString(cv_servername.str);
  s.write(game.Players.size());
  s.write(game.maxplayers);
  s.writeString(gt_name.c_str());
  s.write(gt_version);

  // TODO more basic info? current mapname? server load?
}


void GameType::WriteServerInfo(BitStream &s)
{
  WriteServerQueryResponse(s); // first some basic data

  consvar_t::SaveNetVars(s);
  fc.WriteNetInfo(s); // file names, sizes and md5 sums

  // TODO how long it has been running, how long to go,  gamestate, tick, serverplayer?
}


// scope query on server
void GameType::performScopeQuery(GhostConnection *c)
{
  CONS_Printf("doing scope query\n");
  for (GameInfo::player_iter_t t = e.game->Players.begin(); t != e.game->Players.end(); t++)
    {
      c->objectInScope(t->second); // player information is always in scope
      /*
	if (!cv_hiddenplayers.value)
        c->objectInScope(t->second->pawn);
      */
    }

  // other stuff
}



void GameType::Frag(PlayerInfo *killer, PlayerInfo *victim)
{
  killer->Frags[victim->number]++;
  
  // scoring rule
  if (killer->team == 0)
    if (killer != victim)
      killer->score++;
    else
      killer->score--;
  else if (killer->team != victim->team)
    {
      e.game->teams[killer->team]->score++;
      killer->score++;
    }
  else
    {
      e.game->teams[killer->team]->score--;
      killer->score--;
    }
}




/*
  // subclass Map, give Map class virtual methods: Setup...

cv_deathmatch functionality::

kept:
 dm mapobject spawning / dm starts?
 giving ammo
 giving weapon pieces
 heretic weapon PL2 ammo use (damn!)
 chaos and banishment device destinations
 some Hexen monster and weapon properties
 hud/stbar look

removed:
 messagesystem changes
 health item autouse (use baby skill if you want it!)

new consvars:
 leaveplacedweapons

gametype hooks:
 setting consvars when starting game
 spawnplayer (dm keys!)
 finale/intermission behavior (handles also teleport_exitmap)


cv_teamplay::

 teamstarts
 scoring, scoreboards
 intermission
 
*/
