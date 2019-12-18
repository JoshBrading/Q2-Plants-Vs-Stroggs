
#include "g_local.h"
#include <stdlib.h>

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;

int	sm_meat_index;
int	snd_fry;
int meansOfDeath;
int wave_count;

edict_t		*g_edicts;

cvar_t	*deathmatch;
cvar_t	*coop;
cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*password;
cvar_t	*spectator_password;
cvar_t	*needpass;
cvar_t	*maxclients;
cvar_t	*maxspectators;
cvar_t	*maxentities;
cvar_t	*g_select_empty;
cvar_t	*dedicated;

cvar_t	*filterban;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

cvar_t	*sv_maplist;

void SpawnEntities (char *mapname, char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void WriteGame (char *filename, qboolean autosave);
void ReadGame (char *filename);
void WriteLevel (char *filename);
void ReadLevel (char *filename);
void InitGame (void);
void G_RunFrame (void);


//===================================================================


void ShutdownGame (void)
{
	gi.dprintf ("==== ShutdownGame ====\n");

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}


/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	gi.error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	gi.dprintf ("%s", text);
}

#endif

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int		i;
	edict_t	*ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame (ent);
	}

}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	ent = G_Spawn ();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (void)
{
	edict_t		*ent;
	char *s, *t, *f;
	static const char *seps = " ,\n\r";

	// stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	// see if it's in the map list
	if (*sv_maplist->string) {
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);
		while (t != NULL) {
			if (Q_stricmp(t, level.mapname) == 0) {
				// it's in the list, go to the next one
				t = strtok(NULL, seps);
				if (t == NULL) { // end of list, go to first one
					if (f == NULL) // there isn't a first one, same level
						BeginIntermission (CreateTargetChangeLevel (level.mapname) );
					else
						BeginIntermission (CreateTargetChangeLevel (f) );
				} else
					BeginIntermission (CreateTargetChangeLevel (t) );
				free(s);
				return;
			}
			if (!f)
				f = t;
			t = strtok(NULL, seps);
		}
		free(s);
	}

	if (level.nextmap[0]) // go to a specific map
		BeginIntermission (CreateTargetChangeLevel (level.nextmap) );
	else {	// search for a changelevel
		ent = G_Find (NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
		}
		BeginIntermission (ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass (void)
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified) 
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_stricmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules (void)
{
	int			i;
	gclient_t	*cl;

	if (level.intermissiontime)
		return;

	if (!deathmatch->value)
		return;

	if (timelimit->value)
	{
		if (level.time >= timelimit->value*60)
		{
			gi.bprintf (PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel ();
			return;
		}
	}

	if (fraglimit->value)
	{
		for (i=0 ; i<maxclients->value ; i++)
		{
			cl = game.clients + i;
			if (!g_edicts[i+1].inuse)
				continue;

			if (cl->resp.score >= fraglimit->value)
			{
				gi.bprintf (PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel ();
				return;
			}
		}
	}
}


/*
=============
ExitLevel
=============
*/
void ExitLevel (void)
{
	int		i;
	edict_t	*ent;
	char	command [256];

	Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames ();

	// clear some things before going to next level
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;
	}

}
void Zrandomize(edict_t *zombie)
{
	const char *classes[3];
	classes[0] = "monster_berserk";
	classes[1] = "monster_floater";
	classes[2] = "monster_chick";
	classes[3] = "monster_soldier";

	const char *types[4];
	types[0] = 0;
	types[1] = 0;
	types[2] = 0;
	types[3] = 0;

	int num = (rand() % 3);
	num = 0;
	zombie->classname = classes[num];
	zombie->type = types[num];
}
void ZombieSpawnSequence(int num_zombies)
{
	int zombie_count;
	vec3_t Zspawn = { 2730, 300, -10 };

	for (int col = 0; col < 10; col++)
	{
		for (int row = 4; row >= 1; row--)
		{
			edict_t *zombie;
			if (zombie_count == num_zombies)
				return;

			zombie = G_Spawn();
			Zrandomize(zombie);
			zombie->row = row;
			zombie->PvSTeam = "zombie";
			zombie->type = "zombie";
			VectorCopy(Zspawn, zombie->s.origin);
			ED_CallSpawn(zombie);
			zombie->max_health = 100;
			zombie->health = 100;
			Zspawn[0] -= 60;

			zombie_count++;
		}
		Zspawn[0] = 2730;
		Zspawn[1] -= 60;
	}
}
void ZombieSpawns()
{
	//gi.dprintf("wave_count increased to %i\n", wave_count);
	//	if (wave_count == 1)

	if (wave_count == 1){
		ZombieSpawnSequence(3);
	}
	else if (wave_count == 2)
		ZombieSpawnSequence(5);
	else if (wave_count == 3)
		ZombieSpawnSequence(7);
	else if (wave_count == 4)
		ZombieSpawnSequence(9);
	else if (wave_count == 5)
		ZombieSpawnSequence(11);
	else{
		gi.dprintf("You win!");
	}
}
void CheckWave(){
	edict_t	*ent = NULL;
	vec3_t origin = { 2550, 810, -120 };
	while ((ent = findradius(ent, origin, 1024)) != NULL)
	{
		if (ent->PvSTeam == "zombie" || ent->type == "zombie") // For some reason if type is set to "zombie" it cant see PvSTeam = "zombie"?
			return;
	}
	if (wave_count < 5)
		wave_count++;
	ZombieSpawns();
}
void CheckHome(){
	edict_t	*ent = NULL;
	vec3_t origin = { 2550, 810, -120 };
	int home = 0;
	while ((ent = findradius(ent, origin, 1024)) != NULL)
	{
		if (ent->type == "home" || ent->type == "homesungen"){
			gi.dprintf("home plant");
			home++;
		}
	}
	if (home == 4)
		return;
	gi.AddCommandString("map p\n"); // p would be subsituted with the map name
}
/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
void G_RunFrame (void)
{
	int		i;
	edict_t	*ent;

	if (game_started){
		CheckWave();
		CheckHome();
	}

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	// choose a client for monsters to target this frame
	AI_SetSightClient ();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame (ent);
			continue;
		}

		G_RunEntity (ent);
	}

	// see if it is time to end a deathmatch
	CheckDMRules ();

	// see if needpass needs updated
	CheckNeedPass ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();


}

void HousePlantSpawns() // weird function name, basically if the zombies kill these dudes the have entered 
{						// the players house and "killed" the player ending the game
	vec3_t Pspawn1 = { 2550, 810, -120 }, Pspawn2 = { 2610, 810, -120 }, Pspawn3 = { 2670, 810, -120 }, Pspawn4 = { 2730, 810, -120 };
	edict_t *plant1, *plant2, *plant3, *plant4;

	plant1 = G_Spawn();
	plant1->classname = "monster_berserk";
	plant1->PvSTeam = "plant";
	plant1->type = "home";
	plant1->row = 1;
	VectorCopy(Pspawn1, plant1->s.origin);
	ED_CallSpawn(plant1);

	plant2 = G_Spawn();
	plant2->classname = "monster_berserk";
	plant2->PvSTeam = "plant";
	plant2->type = "home";
	plant2->row = 2;
	VectorCopy(Pspawn2, plant2->s.origin);
	ED_CallSpawn(plant2);

	plant3 = G_Spawn();
	plant3->classname = "monster_berserk";
	plant3->PvSTeam = "plant";
	plant3->type = "home";
	plant3->row = 3;
	VectorCopy(Pspawn3, plant3->s.origin);
	ED_CallSpawn(plant3);

	plant4 = G_Spawn();
	plant4->classname = "monster_berserk";
	plant4->PvSTeam = "plant";
	plant4->type = "homesungen";
	plant4->row = 4;
	VectorCopy(Pspawn4, plant4->s.origin);
	ED_CallSpawn(plant4);

	plant1->max_health = 1;
	plant1->health = 1;

	plant2->max_health = 1;
	plant2->health = 1;

	plant3->max_health = 1;
	plant3->health = 1;

	plant4->max_health = 1;
	plant4->health = 1;
}

void BuildBoardLoop(){
	// (2520, 300, -10) is top left of board
	//	 x     y     z
	// (2700, 840, -10) is bottom right of board
	vec3_t Bspawn1 = { 2790, 870, -160 };
	int side_count = 3; // spawns this + 1 cause its 4am and this isnt important enough to fix
	int change = (570 / side_count);
	for (int i = 0; i < side_count + 1; i++){
		for (int j = 0; j < 2; j++){
			edict_t *boarder1;
			boarder1 = G_Spawn();
			boarder1->classname = "monster_floater";
			VectorCopy(Bspawn1, boarder1->s.origin);
			ED_CallSpawn(boarder1);
			Bspawn1[0] = 2490;
		}
		Bspawn1[0] = 2790;
		Bspawn1[1] -= change;
	}
}