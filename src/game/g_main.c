/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

level_locals_t  level;

typedef struct
{
  vmCvar_t  *vmCvar;
  char      *cvarName;
  char      *defaultString;
  int       cvarFlags;
  int       modificationCount; // for tracking changes
  qboolean  trackChange;       // track this variable, and announce if changed
  /* certain cvars can be set in worldspawn, but we don't want those values to
     persist, so keep track of non-worldspawn changes and restore that on map
     end. unfortunately, if the server crashes, the value set in worldspawn may
     persist */
  char      *explicit;
} cvarTable_t;

gentity_t   g_entities[ MAX_GENTITIES ];
gclient_t   g_clients[ MAX_CLIENTS ];

vmCvar_t  g_timelimit;
vmCvar_t  g_suddenDeathTime;

vmCvar_t  g_doWarmup;
vmCvar_t  g_warmup;
vmCvar_t  g_warmupReadyThreshold;
vmCvar_t  g_warmupTimeout1;
vmCvar_t  g_warmupTimeout1Trigger;
vmCvar_t  g_warmupTimeout2;
vmCvar_t  g_warmupTimeout2Trigger;
vmCvar_t  g_warmupBuildableRespawnTime;
vmCvar_t  g_warmupDefensiveBuildableRespawnTime;

vmCvar_t  g_friendlyFire;
vmCvar_t  g_friendlyBuildableFire;
vmCvar_t  g_dretchPunt;
vmCvar_t  g_password;
vmCvar_t  g_needpass;
vmCvar_t  g_maxclients;
vmCvar_t  g_maxGameClients;
vmCvar_t  g_dedicated;
vmCvar_t  g_speed;
vmCvar_t  g_gravity;
vmCvar_t  g_cheats;
vmCvar_t  g_knockback;
vmCvar_t  g_inactivity;
vmCvar_t  g_debugMove;
vmCvar_t  g_debugDamage;
vmCvar_t  g_debugPlayMap;
vmCvar_t  g_motd;
vmCvar_t  g_synchronousClients;
vmCvar_t  g_countdown;
vmCvar_t  g_doCountdown;
vmCvar_t  g_restarted;
vmCvar_t  g_lockTeamsAtStart;
vmCvar_t  g_logFile;
vmCvar_t  g_logFileSync;
vmCvar_t  g_allowVote;
vmCvar_t  g_voteLimit;
vmCvar_t  g_suddenDeathVotePercent;
vmCvar_t  g_suddenDeathVoteDelay;
vmCvar_t  g_teamForceBalance;
vmCvar_t  g_smoothClients;
vmCvar_t  pmove_fixed;
vmCvar_t  pmove_msec;
vmCvar_t  g_minNameChangePeriod;
vmCvar_t  g_maxNameChanges;

vmCvar_t  g_allowShare;

vmCvar_t  g_allowBuildableStacking;
vmCvar_t  g_alienBuildPoints;
vmCvar_t  g_alienBuildPointsReserve;
vmCvar_t  g_alienBuildQueueTime;
vmCvar_t  g_humanBuildPoints;
vmCvar_t  g_humanBuildPointsReserve;
vmCvar_t  g_humanBuildQueueTime;
vmCvar_t  g_humanRepeaterBuildPoints;
vmCvar_t  g_humanRepeaterBuildQueueTime;
vmCvar_t  g_humanRepeaterMaxZones;
vmCvar_t  g_humanStage;
vmCvar_t  g_humanCredits;
vmCvar_t  g_humanMaxStage;
vmCvar_t  g_humanStage2Threshold;
vmCvar_t  g_humanStage3Threshold;
vmCvar_t  g_alienStage;
vmCvar_t  g_alienCredits;
vmCvar_t  g_alienMaxStage;
vmCvar_t  g_alienStage2Threshold;
vmCvar_t  g_alienStage3Threshold;
vmCvar_t  g_teamImbalanceWarnings;
vmCvar_t  g_freeFundPeriod;
vmCvar_t  g_nadeSpamProtection;

vmCvar_t  g_unlagged;

vmCvar_t  g_disabledEquipment;
vmCvar_t  g_disabledClasses;
vmCvar_t  g_disabledBuildables;

vmCvar_t  g_markDeconstruct;

vmCvar_t  g_debugMapRotation;
vmCvar_t  g_currentMapRotation;
vmCvar_t  g_mapRotationNodes;
vmCvar_t  g_mapRotationStack;
vmCvar_t  g_nextMap;
vmCvar_t  g_initialMapRotation;

vmCvar_t  g_debugVoices;
vmCvar_t  g_voiceChats;

vmCvar_t  g_shove;
vmCvar_t  g_antiSpawnBlock;

vmCvar_t  g_mapConfigs;
vmCvar_t  g_sayAreaRange;

vmCvar_t  g_floodMaxDemerits;
vmCvar_t  g_floodMinTime;

vmCvar_t  g_nextLayout;
vmCvar_t  g_layouts[ 9 ];
vmCvar_t  g_layoutAuto;

vmCvar_t  g_emoticonsAllowedInNames;

vmCvar_t  g_admin;
vmCvar_t  g_adminTempBan;
vmCvar_t  g_adminMaxBan;

vmCvar_t  g_playMapEnable;
vmCvar_t  g_playMapPoolConfig;
vmCvar_t  g_playMapQueueConfig;

vmCvar_t  g_privateMessages;
vmCvar_t  g_specChat;
vmCvar_t  g_publicAdminMessages;
vmCvar_t  g_allowTeamOverlay;

vmCvar_t  g_censorship;
vmCvar_t  g_pimpHuman;

vmCvar_t  g_tag;


// copy cvars that can be set in worldspawn so they can be restored later
static char cv_gravity[ MAX_CVAR_VALUE_STRING ];
static char cv_humanMaxStage[ MAX_CVAR_VALUE_STRING ];
static char cv_alienMaxStage[ MAX_CVAR_VALUE_STRING ];
static char cv_humanRepeaterBuildPoints[ MAX_CVAR_VALUE_STRING ];
static char cv_humanBuildPoints[ MAX_CVAR_VALUE_STRING ];
static char cv_alienBuildPoints[ MAX_CVAR_VALUE_STRING ];
static char cv_humanBuildPointsReserve[ MAX_CVAR_VALUE_STRING ];
static char cv_alienBuildPointsReserve[ MAX_CVAR_VALUE_STRING ];

static cvarTable_t   gameCvarTable[ ] =
{
  // don't override the cheat state set by the system
  { &g_cheats, "sv_cheats", "", 0, 0, qfalse },

  // warmup
  { &g_warmup, "g_warmup", "1", 0, 0, qfalse },
  { &g_doWarmup, "g_doWarmup", "1", CVAR_ARCHIVE, 0, qtrue  },
  { &g_warmupReadyThreshold, "g_warmupReadyThreshold", "50", CVAR_ARCHIVE, 0, qtrue },
  { &g_warmupTimeout1, "g_warmupTimeout1", "300", CVAR_ARCHIVE, 0, qtrue },
  { &g_warmupTimeout1Trigger, "g_warmupTimeout1Trigger", "4", CVAR_ARCHIVE, 0, qtrue },
  { &g_warmupTimeout2, "g_warmupTimeout2", "60", CVAR_ARCHIVE, 0, qtrue },
  { &g_warmupTimeout2Trigger, "g_warmupTimeout2Trigger", "66", CVAR_ARCHIVE, 0, qtrue },
  { &g_warmupBuildableRespawnTime, "g_warmupBuildableRespawnTime", "10", CVAR_ARCHIVE, 0, qtrue },
  { &g_warmupDefensiveBuildableRespawnTime, "g_warmupDefensiveBuildableRespawnTime", "30", CVAR_ARCHIVE, 0, qtrue },

  // noset vars
  { NULL, "gamename", GAME_VERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
  { &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
  { &g_lockTeamsAtStart, "g_lockTeamsAtStart", "0", CVAR_ROM, 0, qfalse  },
  { NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
  { NULL, "P", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

  // latched vars
  { &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

  // change anytime vars
  { &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse  },

  { &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
  { &g_suddenDeathTime, "g_suddenDeathTime", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

  { &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

  { &g_friendlyFire, "g_friendlyFire", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
  { &g_friendlyBuildableFire, "g_friendlyBuildableFire", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
  { &g_dretchPunt, "g_dretchPunt", "1", CVAR_ARCHIVE, 0, qtrue  },

  { &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE, 0, qtrue },

  { &g_countdown, "g_countdown", "10", CVAR_ARCHIVE, 0, qtrue  },
  { &g_doCountdown, "g_doCountdown", "0", CVAR_ARCHIVE, 0, qtrue  },
  { &g_logFile, "g_logFile", "games.log", CVAR_ARCHIVE, 0, qfalse  },
  { &g_logFileSync, "g_logFileSync", "0", CVAR_ARCHIVE, 0, qfalse  },

  { &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },

  { &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

  { &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

  { &g_speed, "g_speed", "320", 0, 0, qtrue  },
  { &g_gravity, "g_gravity", "800", 0, 0, qtrue, cv_gravity },
  { &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
  { &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
  { &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
  { &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
  { &g_debugPlayMap, "g_debugPlayMap", "0", 0, 0, qfalse },
  { &g_motd, "g_motd", "", 0, 0, qfalse },

  { &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },
  { &g_voteLimit, "g_voteLimit", "5", CVAR_ARCHIVE, 0, qfalse },
  { &g_suddenDeathVotePercent, "g_suddenDeathVotePercent", "74", CVAR_ARCHIVE, 0, qfalse },
  { &g_suddenDeathVoteDelay, "g_suddenDeathVoteDelay", "180", CVAR_ARCHIVE, 0, qfalse },
  { &g_minNameChangePeriod, "g_minNameChangePeriod", "5", 0, 0, qfalse},
  { &g_maxNameChanges, "g_maxNameChanges", "5", 0, 0, qfalse},

  { &g_allowShare, "g_allowShare", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse},

  { &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
  { &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
  { &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},

  { &g_allowBuildableStacking, "g_allowBuildableStacking", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse},
  { &g_alienBuildPoints, "g_alienBuildPoints", DEFAULT_ALIEN_BUILDPOINTS, 0, 0, qfalse, cv_alienBuildPoints },
  { &g_alienBuildPointsReserve, "g_alienBuildPointsReserve", DEFAULT_ALIEN_BUILDPOINTS_RESERVE, 0, 0, qfalse, cv_alienBuildPointsReserve },
  { &g_alienBuildQueueTime, "g_alienBuildQueueTime", DEFAULT_ALIEN_QUEUE_TIME, CVAR_ARCHIVE, 0, qfalse  },
  { &g_humanBuildPoints, "g_humanBuildPoints", DEFAULT_HUMAN_BUILDPOINTS, 0, 0, qfalse, cv_humanBuildPoints },
  { &g_humanBuildPointsReserve, "g_humanBuildPointsReserve", DEFAULT_HUMAN_BUILDPOINTS_RESERVE, 0, 0, qfalse, cv_humanBuildPointsReserve },
  { &g_humanBuildQueueTime, "g_humanBuildQueueTime", DEFAULT_HUMAN_QUEUE_TIME, CVAR_ARCHIVE, 0, qfalse  },
  { &g_humanRepeaterBuildPoints, "g_humanRepeaterBuildPoints", DEFAULT_HUMAN_REPEATER_BUILDPOINTS, CVAR_ARCHIVE, 0, qfalse, cv_humanRepeaterBuildPoints },
  { &g_humanRepeaterMaxZones, "g_humanRepeaterMaxZones", DEFAULT_HUMAN_REPEATER_MAX_ZONES, CVAR_ARCHIVE, 0, qfalse  },
  { &g_humanRepeaterBuildQueueTime, "g_humanRepeaterBuildQueueTime", DEFAULT_HUMAN_REPEATER_QUEUE_TIME, CVAR_ARCHIVE, 0, qfalse  },
  { &g_humanStage, "g_humanStage", "0", 0, 0, qfalse  },
  { &g_humanCredits, "g_humanCredits", "0", 0, 0, qfalse  },
  { &g_humanMaxStage, "g_humanMaxStage", DEFAULT_HUMAN_MAX_STAGE, 0, 0, qfalse, cv_humanMaxStage },
  { &g_humanStage2Threshold, "g_humanStage2Threshold", DEFAULT_HUMAN_STAGE2_THRESH, 0, 0, qfalse  },
  { &g_humanStage3Threshold, "g_humanStage3Threshold", DEFAULT_HUMAN_STAGE3_THRESH, 0, 0, qfalse  },
  { &g_alienStage, "g_alienStage", "0", 0, 0, qfalse  },
  { &g_alienCredits, "g_alienCredits", "0", 0, 0, qfalse  },
  { &g_alienMaxStage, "g_alienMaxStage", DEFAULT_ALIEN_MAX_STAGE, 0, 0, qfalse, cv_alienMaxStage },
  { &g_alienStage2Threshold, "g_alienStage2Threshold", DEFAULT_ALIEN_STAGE2_THRESH, 0, 0, qfalse  },
  { &g_alienStage3Threshold, "g_alienStage3Threshold", DEFAULT_ALIEN_STAGE3_THRESH, 0, 0, qfalse  },
  { &g_teamImbalanceWarnings, "g_teamImbalanceWarnings", "30", CVAR_ARCHIVE, 0, qfalse  },
  { &g_freeFundPeriod, "g_freeFundPeriod", DEFAULT_FREEKILL_PERIOD, CVAR_ARCHIVE, 0, qtrue },
  { &g_nadeSpamProtection, "g_nadeSpamProtection", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue },

  { &g_unlagged, "g_unlagged", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },

  { &g_disabledEquipment, "g_disabledEquipment", "", CVAR_ROM | CVAR_SYSTEMINFO, 0, qfalse  },
  { &g_disabledClasses, "g_disabledClasses", "", CVAR_ROM | CVAR_SYSTEMINFO, 0, qfalse  },
  { &g_disabledBuildables, "g_disabledBuildables", "", CVAR_ROM | CVAR_SYSTEMINFO, 0, qfalse  },

  { &g_sayAreaRange, "g_sayAreaRange", "1000", CVAR_ARCHIVE, 0, qtrue  },

  { &g_floodMaxDemerits, "g_floodMaxDemerits", "5000", CVAR_ARCHIVE, 0, qfalse  },
  { &g_floodMinTime, "g_floodMinTime", "2000", CVAR_ARCHIVE, 0, qfalse  },

  { &g_markDeconstruct, "g_markDeconstruct", "3", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },

  { &g_debugMapRotation, "g_debugMapRotation", "0", 0, 0, qfalse  },
  { &g_currentMapRotation, "g_currentMapRotation", "-1", 0, 0, qfalse  }, // -1 = NOT_ROTATING
  { &g_mapRotationNodes, "g_mapRotationNodes", "", CVAR_ROM, 0, qfalse  },
  { &g_mapRotationStack, "g_mapRotationStack", "", CVAR_ROM, 0, qfalse  },
  { &g_nextMap, "g_nextMap", "", 0 , 0, qtrue  },
  { &g_initialMapRotation, "g_initialMapRotation", "", CVAR_ARCHIVE, 0, qfalse  },
  { &g_debugVoices, "g_debugVoices", "0", 0, 0, qfalse  },
  { &g_voiceChats, "g_voiceChats", "1", CVAR_ARCHIVE, 0, qfalse },
  { &g_shove, "g_shove", "0.0", CVAR_ARCHIVE, 0, qfalse  },
  { &g_antiSpawnBlock, "g_antiSpawnBlock", "1", CVAR_ARCHIVE, 0, qtrue  },
  { &g_mapConfigs, "g_mapConfigs", "", CVAR_ARCHIVE, 0, qfalse  },
  { NULL, "g_mapConfigsLoaded", "0", CVAR_ROM, 0, qfalse  },

  { &g_nextLayout, "g_nextLayout", "", 0, 0, qfalse  },
  { &g_layouts[ 0 ], "g_layouts", "", 0, 0, qfalse  },
  { &g_layouts[ 1 ], "g_layouts2", "", 0, 0, qfalse  },
  { &g_layouts[ 2 ], "g_layouts3", "", 0, 0, qfalse  },
  { &g_layouts[ 3 ], "g_layouts4", "", 0, 0, qfalse  },
  { &g_layouts[ 4 ], "g_layouts5", "", 0, 0, qfalse  },
  { &g_layouts[ 5 ], "g_layouts6", "", 0, 0, qfalse  },
  { &g_layouts[ 6 ], "g_layouts7", "", 0, 0, qfalse  },
  { &g_layouts[ 7 ], "g_layouts8", "", 0, 0, qfalse  },
  { &g_layouts[ 8 ], "g_layouts9", "", 0, 0, qfalse  },
  { &g_layoutAuto, "g_layoutAuto", "1", CVAR_ARCHIVE, 0, qfalse  },

  { &g_emoticonsAllowedInNames, "g_emoticonsAllowedInNames", "1", CVAR_LATCH|CVAR_ARCHIVE, 0, qfalse  },

  { &g_admin, "g_admin", "admin.dat", CVAR_ARCHIVE, 0, qfalse  },
  { &g_adminTempBan, "g_adminTempBan", "2m", CVAR_ARCHIVE, 0, qfalse  },
  { &g_adminMaxBan, "g_adminMaxBan", "2w", CVAR_ARCHIVE, 0, qfalse  },

  // playmap pool
  
  { &g_playMapEnable, "g_playMapEnable", "0", CVAR_ARCHIVE, 0, qfalse  },
  { &g_playMapPoolConfig, "g_playMapPoolConfig", "playmap_pool.dat", CVAR_ARCHIVE, 0, qfalse  },
  { &g_playMapQueueConfig, "g_playMapQueueConfig", "playmap_queue.dat", CVAR_ARCHIVE, 0, qfalse  },

  { &g_privateMessages, "g_privateMessages", "1", CVAR_ARCHIVE, 0, qfalse  },
  { &g_specChat, "g_specChat", "1", CVAR_ARCHIVE, 0, qfalse  },
  { &g_publicAdminMessages, "g_publicAdminMessages", "1", CVAR_ARCHIVE, 0, qfalse  },
  { &g_allowTeamOverlay, "g_allowTeamOverlay", "1", CVAR_ARCHIVE, 0, qtrue  },

  { &g_censorship, "g_censorship", "", CVAR_ARCHIVE, 0, qfalse  },
  { &g_pimpHuman, "g_pimpHuman", "1", CVAR_ARCHIVE, 0, qfalse  },

  { &g_tag, "g_tag", "gpp", CVAR_INIT, 0, qfalse }
};

static size_t gameCvarTableSize = ARRAY_LEN( gameCvarTable );


void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );

void G_CountSpawns( void );
void G_CalculateBuildPoints( void );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4,
                                       int arg5, int arg6, int arg7, int arg8, int arg9,
                                       int arg10, int arg11 )
{
  switch( command )
  {
    case GAME_INIT:
      G_InitGame( arg0, arg1, arg2 );
      return 0;

    case GAME_SHUTDOWN:
      G_ShutdownGame( arg0 );
      return 0;

    case GAME_CLIENT_CONNECT:
      return (intptr_t)ClientConnect( arg0, arg1 );

    case GAME_CLIENT_THINK:
      ClientThink( arg0 );
      return 0;

    case GAME_CLIENT_USERINFO_CHANGED:
      ClientUserinfoChanged( arg0, qfalse );
      return 0;

    case GAME_CLIENT_DISCONNECT:
      ClientDisconnect( arg0 );
      return 0;

    case GAME_CLIENT_BEGIN:
      ClientBegin( arg0 );
      return 0;

    case GAME_CLIENT_COMMAND:
      ClientCommand( arg0 );
      return 0;

    case GAME_RUN_FRAME:
      G_RunFrame( arg0 );
      return 0;

    case GAME_CONSOLE_COMMAND:
      return ConsoleCommand( );
  }

  return -1;
}


void QDECL G_Printf( const char *fmt, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, fmt );
  Q_vsnprintf( text, sizeof( text ), fmt, argptr );
  va_end( argptr );

  trap_Print( text );
}

void QDECL G_Error( const char *fmt, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, fmt );
  Q_vsnprintf( text, sizeof( text ), fmt, argptr );
  va_end( argptr );

  trap_Error( text );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void )
{
  gentity_t *e, *e2;
  int       i, j;
  int       c, c2;

  c = 0;
  c2 = 0;

  for( i = MAX_CLIENTS, e = g_entities + i; i < level.num_entities; i++, e++ )
  {
    if( !e->team )
      continue;

    if( e->flags & FL_TEAMSLAVE )
      continue;

    e->teammaster = e;
    c++;
    c2++;

    for( j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++ )
    {
      if( !e2->team )
        continue;

      if( e2->flags & FL_TEAMSLAVE )
        continue;

      if( !strcmp( e->team, e2->team ) )
      {
        c2++;
        e2->teamchain = e->teamchain;
        e->teamchain = e2;
        e2->teammaster = e;
        e2->flags |= FL_TEAMSLAVE;

        // make sure that targets only point at the master
        if( e2->targetname )
        {
          e->targetname = e2->targetname;
          e2->targetname = NULL;
        }
      }
    }
  }

  G_Printf( "%i teams with %i entities\n", c, c2 );
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void )
{
  int         i;
  cvarTable_t *cv;

  for( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
  {
    trap_Cvar_Register( cv->vmCvar, cv->cvarName,
      cv->defaultString, cv->cvarFlags );

    if( cv->vmCvar )
      cv->modificationCount = cv->vmCvar->modificationCount;

    if( cv->explicit )
      strcpy( cv->explicit, cv->vmCvar->string );
  }
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void )
{
  int         i;
  cvarTable_t *cv;

  for( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
  {
    if( cv->vmCvar )
    {
      trap_Cvar_Update( cv->vmCvar );

      if( cv->modificationCount != cv->vmCvar->modificationCount )
      {
        cv->modificationCount = cv->vmCvar->modificationCount;

        if( cv->trackChange )
          trap_SendServerCommand( -1, va( "print \"Server: %s changed to %s\n\"",
            cv->cvarName, cv->vmCvar->string ) );

        if( !level.spawning && cv->explicit )
          strcpy( cv->explicit, cv->vmCvar->string );
      }
    }
  }
}

/*
=================
G_RestoreCvars
=================
*/
void G_RestoreCvars( void )
{
  int         i;
  cvarTable_t *cv;

  for( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
  {
    if( cv->vmCvar && cv->explicit )
      trap_Cvar_Set( cv->cvarName, cv->explicit );
  }
}

/*
=================
G_MapConfigs
=================
*/
void G_MapConfigs( const char *mapname )
{

  if( !g_mapConfigs.string[0] )
    return;

  if( trap_Cvar_VariableIntegerValue( "g_mapConfigsLoaded" ) )
    return;

  trap_SendConsoleCommand( EXEC_APPEND,
    va( "exec \"%s/default.cfg\"\n", g_mapConfigs.string ) );

  trap_SendConsoleCommand( EXEC_APPEND,
    va( "exec \"%s/%s.cfg\"\n", g_mapConfigs.string, mapname ) );

  trap_Cvar_Set( "g_mapConfigsLoaded", "1" );
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart )
{
  int i;

  srand( randomSeed );

  G_RegisterCvars( );

  G_Printf( "------- Game Initialization -------\n" );
  G_Printf( "gamename: %s\n", GAME_VERSION );

  BG_InitMemory( );

  // set some level globals
  memset( &level, 0, sizeof( level ) );


  trap_Query( DB_OPEN, "game.db", NULL );
  trap_Query( DB_TIME_GET, level.database_data, NULL );
  unpack_start( level.database_data, DATABASE_DATA_MAX );
  unpack_int( &level.epochStartTime );

  level.time = levelTime;
  level.startTime = levelTime;
  level.alienStage2Time = level.alienStage3Time =
    level.humanStage2Time = level.humanStage3Time = level.startTime;

  // reset the level's 1 minute and 5 minute timeouts
  level.warmup1Time = -1;
  level.warmup2Time = -1;

  level.snd_fry = G_SoundIndex( "sound/misc/fry.wav" ); // FIXME standing in lava / slime

  if( g_logFile.string[ 0 ] )
  {
    if( g_logFileSync.integer )
      trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND_SYNC );
    else
      trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND );

    if( !level.logFile )
      G_Printf( "WARNING: Couldn't open logfile: %s\n", g_logFile.string );
    else
    {
      char serverinfo[ MAX_INFO_STRING ];
      qtime_t qt;

      trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

      G_LogPrintf( "------------------------------------------------------------\n" );
      G_LogPrintf( "InitGame: %s\n", serverinfo );

      trap_RealTime( &qt );
      G_LogPrintf("RealTime: %04i-%02i-%02i %02i:%02i:%02i\n",
            qt.tm_year+1900, qt.tm_mon+1, qt.tm_mday,
            qt.tm_hour, qt.tm_min, qt.tm_sec );

    }
  }
  else
    G_Printf( "Not logging to disk\n" );

  if( g_mapConfigs.string[ 0 ] && !trap_Cvar_VariableIntegerValue( "g_mapConfigsLoaded" ) )
  {
    char map[ MAX_CVAR_VALUE_STRING ] = {""};

    G_Printf( "InitGame: executing map configuration scripts and restarting\n" );
    trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
    G_MapConfigs( map );
    trap_SendConsoleCommand( EXEC_APPEND, "wait\nmap_restart 0\n" );
  }
  else
  {
    // we're done with g_mapConfigs, so reset this for the next map
    trap_Cvar_Set( "g_mapConfigsLoaded", "0" );
  }

  // set this cvar to 0 if it exists, but otherwise avoid its creation
  if( trap_Cvar_VariableIntegerValue( "g_rangeMarkerWarningGiven" ) )
    trap_Cvar_Set( "g_rangeMarkerWarningGiven", "0" );

  G_RegisterCommands( );
  G_admin_readconfig( NULL );
  G_LoadCensors( );

  // initialize the build points reserves
  level.alienBuildPointsReserve = g_alienBuildPointsReserve.integer;
  level.alienBuildPointsReserveLost = 0;
  level.humanBuildPointsReserve = g_humanBuildPointsReserve.integer;
  level.humanBuildPointsReserveLost = 0;

  // initialize all entities for this game
  memset( g_entities, 0, MAX_GENTITIES * sizeof( g_entities[ 0 ] ) );
  level.gentities = g_entities;

  // initialize all clients for this game
  level.maxclients = g_maxclients.integer;
  memset( g_clients, 0, MAX_CLIENTS * sizeof( g_clients[ 0 ] ) );
  level.clients = g_clients;

  // set client fields on player ents
  for( i = 0; i < level.maxclients; i++ )
    g_entities[ i ].client = level.clients + i;

  // always leave room for the max number of clients,
  // even if they aren't all used, so numbers inside that
  // range are NEVER anything but clients
  level.num_entities = MAX_CLIENTS;

  for( i = 0; i < MAX_CLIENTS; i++ )
    g_entities[ i ].classname = "clientslot";

  // let the server system know where the entites are
  trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
    &level.clients[ 0 ].ps, sizeof( level.clients[ 0 ] ) );

  level.emoticonCount = BG_LoadEmoticons( level.emoticons, MAX_EMOTICONS );

  trap_SetConfigstring( CS_INTERMISSION, "0" );
  trap_SetConfigstring( CS_WARMUP, va( "%d", IS_WARMUP ) );

  G_InitPlayerModel( );

  // test to see if a custom buildable layout will be loaded
  G_LayoutSelect( );

  // this has to be flipped after the first UpdateCvars
  level.spawning = qtrue;
  // parse the key/value pairs and spawn gentities
  G_SpawnEntitiesFromString( );

  // load up a custom building layout if there is one
  G_LayoutLoad( level.layout );

  // the map might disable some things
  BG_InitAllowedGameElements( );

  // general initialization
  G_FindTeams( );

  BG_InitClassConfigs( );
  BG_InitBuildableConfigs( );
  G_InitDamageLocations( );
  G_InitMapRotations( );
  G_ReloadPlayMapPool();
  G_ReloadPlayMapQueue();
  G_InitSpawnQueue( &level.alienSpawnQueue );
  G_InitSpawnQueue( &level.humanSpawnQueue );

  if( g_debugMapRotation.integer )
    G_PrintRotations( );

  level.voices = BG_VoiceInit( );
  BG_PrintVoices( level.voices, g_debugVoices.integer );

  //reset stages
  trap_Cvar_Set( "g_alienStage", va( "%d", S1 ) );
  trap_Cvar_Set( "g_humanStage", va( "%d", S1 ) );
  trap_Cvar_Set( "g_alienCredits", 0 );
  trap_Cvar_Set( "g_humanCredits", 0 );
  level.suddenDeathBeginTime = g_suddenDeathTime.integer * 60000;

  G_Printf( "-----------------------------------\n" );

  // So the server counts the spawns without a client attached
  G_CountSpawns( );

  for( i = 0; i < NUM_TEAMS; i++ )
    level.numUnspawnedBuildables[ i ] = 0;

  G_UpdateTeamConfigStrings( );

  if( g_lockTeamsAtStart.integer )
  {
    level.alienTeamLocked = qtrue;
    level.humanTeamLocked = qtrue;
    trap_Cvar_Set( "g_lockTeamsAtStart", "0" );
  }
}

/*
==================
G_ClearVotes

remove all currently active votes
==================
*/
static void G_ClearVotes( void )
{
  int i;
  memset( level.voteTime, 0, sizeof( level.voteTime ) );
  for( i = 0; i < NUM_TEAMS; i++ )
  {
    trap_SetConfigstring( CS_VOTE_TIME + i, "" );
    trap_SetConfigstring( CS_VOTE_STRING + i, "" );
  }
}

/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart )
{
  // in case of a map_restart
  G_ClearVotes( );

  G_RestoreCvars( );

  G_Printf( "==== ShutdownGame ====\n" );

  if( level.logFile )
  {
    G_LogPrintf( "ShutdownGame:\n" );
    G_LogPrintf( "------------------------------------------------------------\n" );
    trap_FS_FCloseFile( level.logFile );
    level.logFile = 0;
  }

  // write all the client session data so we can get it back
  G_WriteSessionData( );

  // Save the playmap files here
  G_SavePlayMapPool();
  G_SavePlayMapQueue();

  G_admin_cleanup( );
  G_namelog_cleanup( );
  G_UnregisterCommands( );

  G_FreePlayerModel( );

  G_ShutdownMapRotations( );

  level.restarted = qfalse;
  level.surrenderTeam = TEAM_NONE;
  trap_SetConfigstring( CS_WINNER, "" );

  trap_Query( DB_CLOSE, NULL, NULL );
}



//===================================================================

void QDECL Com_Error( int level, const char *error, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, error );
  Q_vsnprintf( text, sizeof( text ), error, argptr );
  va_end( argptr );

  G_Error( "%s", text );
}

void QDECL Com_Printf( const char *msg, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, msg );
  Q_vsnprintf( text, sizeof( text ), msg, argptr );
  va_end( argptr );

  G_Printf( "%s", text );
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/


/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b )
{
  gclient_t *ca, *cb;

  ca = &level.clients[ *(int *)a ];
  cb = &level.clients[ *(int *)b ];

  // then sort by score
  if( ca->ps.persistant[ PERS_SCORE ] > cb->ps.persistant[ PERS_SCORE ] )
    return -1;
  if( ca->ps.persistant[ PERS_SCORE ] < cb->ps.persistant[ PERS_SCORE ] )
    return 1;
  else
    return 0;
}

/*
============
G_InitSpawnQueue

Initialise a spawn queue
============
*/
void G_InitSpawnQueue( spawnQueue_t *sq )
{
  int i;

  sq->back = sq->front = 0;
  sq->back = QUEUE_MINUS1( sq->back );

  //0 is a valid clientNum, so use something else
  for( i = 0; i < MAX_CLIENTS; i++ )
    sq->clients[ i ] = -1;
}

/*
============
G_GetSpawnQueueLength

Return the length of a spawn queue
============
*/
int G_GetSpawnQueueLength( spawnQueue_t *sq )
{
  int length = sq->back - sq->front + 1;

  while( length < 0 )
    length += MAX_CLIENTS;

  while( length >= MAX_CLIENTS )
    length -= MAX_CLIENTS;

  return length;
}

/*
============
G_PopSpawnQueue

Remove from front element from a spawn queue
============
*/
int G_PopSpawnQueue( spawnQueue_t *sq )
{
  int clientNum = sq->clients[ sq->front ];

  if( G_GetSpawnQueueLength( sq ) > 0 )
  {
    sq->clients[ sq->front ] = -1;
    sq->front = QUEUE_PLUS1( sq->front );
    G_StopFollowing( g_entities + clientNum );
    g_entities[ clientNum ].client->ps.pm_flags &= ~PMF_QUEUED;

    return clientNum;
  }
  else
    return -1;
}

/*
============
G_PeekSpawnQueue

Look at front element from a spawn queue
============
*/
int G_PeekSpawnQueue( spawnQueue_t *sq )
{
  return sq->clients[ sq->front ];
}

/*
============
G_SearchSpawnQueue

Look to see if clientNum is already in the spawnQueue
============
*/
qboolean G_SearchSpawnQueue( spawnQueue_t *sq, int clientNum )
{
  int i;

  for( i = 0; i < MAX_CLIENTS; i++ )
  {
    if( sq->clients[ i ] == clientNum )
      return qtrue;
  }

  return qfalse;
}

/*
============
G_PushSpawnQueue

Add an element to the back of the spawn queue
============
*/
qboolean G_PushSpawnQueue( spawnQueue_t *sq, int clientNum )
{
  // don't add the same client more than once
  if( G_SearchSpawnQueue( sq, clientNum ) )
    return qfalse;

  sq->back = QUEUE_PLUS1( sq->back );
  sq->clients[ sq->back ] = clientNum;

  g_entities[ clientNum ].client->ps.pm_flags |= PMF_QUEUED;
  return qtrue;
}

/*
============
G_RemoveFromSpawnQueue

remove a specific client from a spawn queue
============
*/
qboolean G_RemoveFromSpawnQueue( spawnQueue_t *sq, int clientNum )
{
  int i = sq->front;

  if( G_GetSpawnQueueLength( sq ) )
  {
    do
    {
      if( sq->clients[ i ] == clientNum )
      {
        //and this kids is why it would have
        //been better to use an LL for internal
        //representation
        do
        {
          sq->clients[ i ] = sq->clients[ QUEUE_PLUS1( i ) ];

          i = QUEUE_PLUS1( i );
        } while( i != QUEUE_PLUS1( sq->back ) );

        sq->back = QUEUE_MINUS1( sq->back );
        g_entities[ clientNum ].client->ps.pm_flags &= ~PMF_QUEUED;

        return qtrue;
      }

      i = QUEUE_PLUS1( i );
    } while( i != QUEUE_PLUS1( sq->back ) );
  }

  return qfalse;
}

/*
============
G_GetPosInSpawnQueue

Get the position of a client in a spawn queue
============
*/
int G_GetPosInSpawnQueue( spawnQueue_t *sq, int clientNum )
{
  int i = sq->front;

  if( G_GetSpawnQueueLength( sq ) )
  {
    do
    {
      if( sq->clients[ i ] == clientNum )
      {
        if( i < sq->front )
          return i + MAX_CLIENTS - sq->front;
        else
          return i - sq->front;
      }

      i = QUEUE_PLUS1( i );
    } while( i != QUEUE_PLUS1( sq->back ) );
  }

  return -1;
}

/*
============
G_PrintSpawnQueue

Print the contents of a spawn queue
============
*/
void G_PrintSpawnQueue( spawnQueue_t *sq )
{
  int i = sq->front;
  int length = G_GetSpawnQueueLength( sq );

  G_Printf( "l:%d f:%d b:%d    :", length, sq->front, sq->back );

  if( length > 0 )
  {
    do
    {
      if( sq->clients[ i ] == -1 )
        G_Printf( "*:" );
      else
        G_Printf( "%d:", sq->clients[ i ] );

      i = QUEUE_PLUS1( i );
    } while( i != QUEUE_PLUS1( sq->back ) );
  }

  G_Printf( "\n" );
}

/*
============
G_SpawnClients

Spawn queued clients
============
*/
void G_SpawnClients( team_t team )
{
  int           clientNum;
  gentity_t     *ent, *spawn;
  vec3_t        spawn_origin, spawn_angles;
  spawnQueue_t  *sq = NULL;
  int           numSpawns = 0;

  if( team == TEAM_ALIENS )
  {
    sq = &level.alienSpawnQueue;
    numSpawns = level.numAlienSpawns;
  }
  else if( team == TEAM_HUMANS )
  {
    sq = &level.humanSpawnQueue;
    numSpawns = level.numHumanSpawns;
  }

  if( G_GetSpawnQueueLength( sq ) > 0 && numSpawns > 0 )
  {
    clientNum = G_PeekSpawnQueue( sq );
    ent = &g_entities[ clientNum ];

    if( ( spawn = G_SelectTremulousSpawnPoint( team,
            ent->client->pers.lastDeathLocation,
            spawn_origin, spawn_angles ) ) )
    {
      clientNum = G_PopSpawnQueue( sq );

      if( clientNum < 0 )
        return;

      ent = &g_entities[ clientNum ];

      ent->client->sess.spectatorState = SPECTATOR_NOT;
      ClientUserinfoChanged( clientNum, qfalse );
      ClientSpawn( ent, spawn, spawn_origin, spawn_angles );
    }
  }
}

/*
============
G_CountSpawns

Counts the number of spawns for each team
============
*/
void G_CountSpawns( void )
{
  int i;
  gentity_t *ent;

  level.numAlienSpawns = 0;
  level.numHumanSpawns = 0;
  for( i = 1, ent = g_entities + i ; i < level.num_entities ; i++, ent++ )
  {
    if( !ent->inuse || ent->s.eType != ET_BUILDABLE || ent->health <= 0 )
      continue;

    if( ent->s.modelindex == BA_A_SPAWN )
      level.numAlienSpawns++;

    if( ent->s.modelindex == BA_H_SPAWN )
    {
      if( !ent->spawned && !ent->dcc )
        continue;

      level.numHumanSpawns++;
    }
  }
}


/*
============
G_TimeTilSuddenDeath
============
*/
#define SUDDENDEATHWARNING 60000
int G_TimeTilSuddenDeath( void )
{
  if( ( !g_suddenDeathTime.integer && level.suddenDeathBeginTime == 0 ) ||
      ( level.suddenDeathBeginTime < 0 ) )
    return SUDDENDEATHWARNING + 1; // Always some time away

  return ( ( level.suddenDeathBeginTime ) - ( level.time - level.startTime ) );
}


#define PLAYER_COUNT_MOD 5.0f

/*
============
G_CalculateBuildPoints

Recalculate the quantity of building points available to the teams
============
*/
void G_CalculateBuildPoints( void )
{
  int               i;

  // BP queue updates
  while( level.alienBuildPointQueue > 0 &&
         level.alienBuildPointsReserve > 0 &&
         level.alienNextQueueTime < level.time )
  {
    level.alienBuildPointQueue--;
    if( !IS_WARMUP && G_TimeTilSuddenDeath( ) > 0 )
      level.alienBuildPointsReserveLost++;
    level.alienNextQueueTime += G_NextQueueTime( level.alienBuildPointQueue,
                                               g_alienBuildPoints.integer,
                                               g_alienBuildQueueTime.integer );

    level.alienBuildPointsReserve = g_alienBuildPointsReserve.integer -
                                    level.alienBuildPointsReserveLost;
  }


  while( level.humanBuildPointQueue > 0 &&
         level.humanBuildPointsReserve > 0 &&
         level.humanNextQueueTime < level.time )
  {
    level.humanBuildPointQueue--;
    if( !IS_WARMUP && G_TimeTilSuddenDeath( ) > 0 )
      level.humanBuildPointsReserveLost++;
    level.humanNextQueueTime += G_NextQueueTime( level.humanBuildPointQueue,
                                               g_humanBuildPoints.integer,
                                               g_humanBuildQueueTime.integer );

    level.humanBuildPointsReserve = g_humanBuildPointsReserve.integer -
                                    level.humanBuildPointsReserveLost;
  }

  

  // Sudden Death checks (not applicable in warmup)
  if( !IS_WARMUP && G_TimeTilSuddenDeath( ) <= 0 && level.suddenDeathWarning < TW_PASSED )
  {
    G_LogPrintf( "Beginning Sudden Death\n" );
    trap_SendServerCommand( -1, "cp \"Sudden Death!\"" );
    trap_SendServerCommand( -1, "print \"Beginning Sudden Death.\n\"" );
    level.suddenDeathWarning = TW_PASSED;
    G_ClearDeconMarks( );

    // empty the build points reserves
    level.alienBuildPointsReserveLost = g_alienBuildPointsReserve.integer;
    level.humanBuildPointsReserveLost = g_humanBuildPointsReserve.integer;

    // Clear blueprints, or else structs that cost 0 BP can still be built after SD
    for( i = 0; i < level.maxclients; i++ )
    {
      if( g_entities[ i ].client->ps.stats[ STAT_BUILDABLE ] != BA_NONE )
        g_entities[ i ].client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
    }
  }
  else if( !IS_WARMUP && G_TimeTilSuddenDeath( ) <= SUDDENDEATHWARNING &&
    level.suddenDeathWarning < TW_IMMINENT )
  {
    trap_SendServerCommand( -1, va( "cp \"Sudden Death in %d seconds!\"",
          (int)( G_TimeTilSuddenDeath( ) / 1000 ) ) );
    trap_SendServerCommand( -1, va( "print \"Sudden Death will begin in %d seconds.\n\"",
          (int)( G_TimeTilSuddenDeath( ) / 1000 ) ) );
    level.suddenDeathWarning = TW_IMMINENT;
  }

  level.humanBuildPoints = g_humanBuildPoints.integer - level.humanBuildPointQueue;
  level.alienBuildPoints = g_alienBuildPoints.integer - level.alienBuildPointQueue;

  // Iterate through entities
  for( i = MAX_CLIENTS; i < level.num_entities; i++ )
  {
    gentity_t         *ent = &g_entities[ i ];
    buildable_t       buildable;
    int               cost;

    if( ent->s.eType != ET_BUILDABLE || ent->s.eFlags & EF_DEAD )
      continue;

    // Subtract the BP from the appropriate pool
    buildable = ent->s.modelindex;
    cost = BG_Buildable( buildable )->buildPoints;

    if( ent->buildableTeam == TEAM_ALIENS )
      level.alienBuildPoints -= cost;
    if( ent->buildableTeam == TEAM_HUMANS )
      level.humanBuildPoints -= cost;
  }

  if( level.humanBuildPoints < 0 )
    level.humanBuildPoints = 0;

  if( level.alienBuildPoints < 0 )
    level.alienBuildPoints = 0;
}

/*
============
G_CalculateStages
============
*/
void G_CalculateStages( void )
{
  float         alienPlayerCountMod     = level.averageNumAlienClients / PLAYER_COUNT_MOD;
  float         humanPlayerCountMod     = level.averageNumHumanClients / PLAYER_COUNT_MOD;
  int           alienNextStageThreshold, humanNextStageThreshold;
  static int    lastAlienStageModCount  = 1;
  static int    lastHumanStageModCount  = 1;
  static int    alienTriggerStage       = 0;
  static int    humanTriggerStage       = 0;

  if( alienPlayerCountMod < 0.1f )
    alienPlayerCountMod = 0.1f;

  if( humanPlayerCountMod < 0.1f )
    humanPlayerCountMod = 0.1f;

  if( g_alienCredits.integer >=
      (int)( ceil( (float)g_alienStage2Threshold.integer * alienPlayerCountMod ) ) &&
      g_alienStage.integer == S1 && g_alienMaxStage.integer > S1 )
  {
    trap_Cvar_Set( "g_alienStage", va( "%d", S2 ) );
    level.alienStage2Time = level.time;
    lastAlienStageModCount = g_alienStage.modificationCount;
    G_LogPrintf("Stage: A 2: Aliens reached Stage 2\n");
  }

  if( g_alienCredits.integer >=
      (int)( ceil( (float)g_alienStage3Threshold.integer * alienPlayerCountMod ) ) &&
      g_alienStage.integer == S2 && g_alienMaxStage.integer > S2 )
  {
    trap_Cvar_Set( "g_alienStage", va( "%d", S3 ) );
    level.alienStage3Time = level.time;
    lastAlienStageModCount = g_alienStage.modificationCount;
    G_LogPrintf("Stage: A 3: Aliens reached Stage 3\n");
  }

  if( g_humanCredits.integer >=
      (int)( ceil( (float)g_humanStage2Threshold.integer * humanPlayerCountMod ) ) &&
      g_humanStage.integer == S1 && g_humanMaxStage.integer > S1 )
  {
    trap_Cvar_Set( "g_humanStage", va( "%d", S2 ) );
    level.humanStage2Time = level.time;
    lastHumanStageModCount = g_humanStage.modificationCount;
    G_LogPrintf("Stage: H 2: Humans reached Stage 2\n");
  }

  if( g_humanCredits.integer >=
      (int)( ceil( (float)g_humanStage3Threshold.integer * humanPlayerCountMod ) ) &&
      g_humanStage.integer == S2 && g_humanMaxStage.integer > S2 )
  {
    trap_Cvar_Set( "g_humanStage", va( "%d", S3 ) );
    level.humanStage3Time = level.time;
    lastHumanStageModCount = g_humanStage.modificationCount;
    G_LogPrintf("Stage: H 3: Humans reached Stage 3\n");
  }

  if( g_alienStage.modificationCount > lastAlienStageModCount )
  {
    while( alienTriggerStage < MIN( g_alienStage.integer, S3 ) )
      G_Checktrigger_stages( TEAM_ALIENS, ++alienTriggerStage );

    if( g_alienStage.integer == S2 )
      level.alienStage2Time = level.time;
    else if( g_alienStage.integer == S3 )
      level.alienStage3Time = level.time;

    lastAlienStageModCount = g_alienStage.modificationCount;
  }

  if( g_humanStage.modificationCount > lastHumanStageModCount )
  {
    while( humanTriggerStage < MIN( g_humanStage.integer, S3 ) )
      G_Checktrigger_stages( TEAM_HUMANS, ++humanTriggerStage );

    if( g_humanStage.integer == S2 )
      level.humanStage2Time = level.time;
    else if( g_humanStage.integer == S3 )
      level.humanStage3Time = level.time;

    lastHumanStageModCount = g_humanStage.modificationCount;
  }

  if( g_alienStage.integer == S1 && g_alienMaxStage.integer > S1 )
    alienNextStageThreshold = (int)( ceil( (float)g_alienStage2Threshold.integer * alienPlayerCountMod ) );
  else if( g_alienStage.integer == S2 && g_alienMaxStage.integer > S2 )
    alienNextStageThreshold = (int)( ceil( (float)g_alienStage3Threshold.integer * alienPlayerCountMod ) );
  else
    alienNextStageThreshold = -1;

  if( g_humanStage.integer == S1 && g_humanMaxStage.integer > S1 )
    humanNextStageThreshold = (int)( ceil( (float)g_humanStage2Threshold.integer * humanPlayerCountMod ) );
  else if( g_humanStage.integer == S2 && g_humanMaxStage.integer > S2 )
    humanNextStageThreshold = (int)( ceil( (float)g_humanStage3Threshold.integer * humanPlayerCountMod ) );
  else
    humanNextStageThreshold = -1;

  // save a lot of bandwidth by rounding thresholds up to the nearest 100
  if( alienNextStageThreshold > 0 )
    alienNextStageThreshold = ceil( (float)alienNextStageThreshold / 100 ) * 100;

  if( humanNextStageThreshold > 0 )
    humanNextStageThreshold = ceil( (float)humanNextStageThreshold / 100 ) * 100;

  trap_SetConfigstring( CS_ALIEN_STAGES, va( "%d %d %d",
        ( IS_WARMUP ? S3 : g_alienStage.integer ),
        ( IS_WARMUP ? 99999 : g_alienCredits.integer ),
        ( IS_WARMUP ? 0 : alienNextStageThreshold ) ) );

  trap_SetConfigstring( CS_HUMAN_STAGES, va( "%d %d %d",
        ( IS_WARMUP ? S3 : g_humanStage.integer ),
        ( IS_WARMUP ? 99999 : g_humanCredits.integer ),
        ( IS_WARMUP ? 0 : humanNextStageThreshold ) ) );
}

/*
============
CalculateAvgPlayers

Calculates the average number of players playing this game
============
*/
void G_CalculateAvgPlayers( void )
{
  //there are no clients or only spectators connected, so
  //reset the number of samples in order to avoid the situation
  //where the average tends to 0
  if( !level.numAlienClients )
  {
    level.numAlienSamples = 0;
    trap_Cvar_Set( "g_alienCredits", "0" );
  }

  if( !level.numHumanClients )
  {
    level.numHumanSamples = 0;
    trap_Cvar_Set( "g_humanCredits", "0" );
  }

  //calculate average number of clients for stats
  level.averageNumAlienClients =
    ( ( level.averageNumAlienClients * level.numAlienSamples )
      + level.numAlienClients ) /
    (float)( level.numAlienSamples + 1 );
  level.numAlienSamples++;

  level.averageNumHumanClients =
    ( ( level.averageNumHumanClients * level.numHumanSamples )
      + level.numHumanClients ) /
    (float)( level.numHumanSamples + 1 );
  level.numHumanSamples++;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void )
{
  int       i;
  char      P[ MAX_CLIENTS + 1 ] = {""};

  level.numConnectedClients = 0;
  level.numPlayingClients = 0;
  memset( level.numVotingClients, 0, sizeof( level.numVotingClients ) );
  level.numAlienClients = 0;
  level.numHumanClients = 0;
  level.numAlienClientsAlive = 0;
  level.numHumanClientsAlive = 0;

  for( i = 0; i < level.maxclients; i++ )
  {
    P[ i ] = '-';
    if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
    {
      level.sortedClients[ level.numConnectedClients ] = i;
      level.numConnectedClients++;
      P[ i ] = (char)'0' + level.clients[ i ].pers.teamSelection;

      level.numVotingClients[ TEAM_NONE ]++;

      if( level.clients[ i ].pers.connected != CON_CONNECTED )
        continue;

      if( level.clients[ i ].pers.teamSelection != TEAM_NONE )
      {
        level.numPlayingClients++;
        if( level.clients[ i ].pers.teamSelection == TEAM_ALIENS )
        {
          level.numAlienClients++;
          if( level.clients[ i ].sess.spectatorState == SPECTATOR_NOT )
            level.numAlienClientsAlive++;
        }
        else if( level.clients[ i ].pers.teamSelection == TEAM_HUMANS )
        {
          level.numHumanClients++;
          if( level.clients[ i ].sess.spectatorState == SPECTATOR_NOT )
            level.numHumanClientsAlive++;
        }
      }
    }
  }
  level.numNonSpectatorClients = level.numAlienClientsAlive +
    level.numHumanClientsAlive;
  level.numVotingClients[ TEAM_ALIENS ] = level.numAlienClients;
  level.numVotingClients[ TEAM_HUMANS ] = level.numHumanClients;
  P[ i ] = '\0';
  trap_Cvar_Set( "P", P );

  qsort( level.sortedClients, level.numConnectedClients,
    sizeof( level.sortedClients[ 0 ] ), SortRanks );

  // see if it is time to end the level
  CheckExitRules( );

  // if we are at the intermission, send the new info to everyone
  if( level.intermissiontime )
    SendScoreboardMessageToAllClients( );
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void )
{
  int   i;

  for( i = 0; i < level.maxclients; i++ )
  {
    if( level.clients[ i ].pers.connected == CON_CONNECTED )
      ScoreboardMessage( g_entities + i );
  }
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent )
{
  // take out of follow mode if needed
  if( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
    G_StopFollowing( ent );

  // move to the spot
  VectorCopy( level.intermission_origin, ent->s.pos.trBase );
  VectorCopy( level.intermission_origin, ent->r.currentOrigin );
  VectorCopy( level.intermission_origin, ent->client->ps.origin );
  VectorCopy( level.intermission_angle, ent->s.apos.trBase );
  VectorCopy( level.intermission_angle, ent->client->ps.viewangles );
  ent->client->ps.pm_type = PM_INTERMISSION;

  // clean up powerup info
  memset( ent->client->ps.misc, 0, sizeof( ent->client->ps.misc ) );

  ent->client->ps.eFlags = 0;
  ent->s.eFlags = 0;
  ent->s.eType = ET_GENERAL;
  ent->s.loopSound = 0;
  ent->s.event = 0;
  ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void )
{
  gentity_t *ent, *target;
  vec3_t    dir;

  // find the intermission spot
  ent = G_Find( NULL, FOFS( classname ), "info_player_intermission" );

  if( !ent )
  { // the map creator forgot to put in an intermission point...
    G_SelectSpawnPoint( vec3_origin, level.intermission_origin, level.intermission_angle );
  }
  else
  {
    VectorCopy( ent->r.currentOrigin, level.intermission_origin );
    VectorCopy( ent->r.currentAngles, level.intermission_angle );
    // if it has a target, look towards it
    if( ent->target )
    {
      target = G_PickTarget( ent->target );

      if( target )
      {
        VectorSubtract( target->r.currentOrigin, level.intermission_origin, dir );
        vectoangles( dir, level.intermission_angle );
      }
    }
  }

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void )
{
  int     i;
  gentity_t *client;

  if( level.intermissiontime )
    return;   // already active

  level.intermissiontime = level.time;

  G_ClearVotes( );

  G_UpdateTeamConfigStrings( );

  FindIntermissionPoint( );

  // move all clients to the intermission point
  for( i = 0; i < level.maxclients; i++ )
  {
    client = g_entities + i;

    if( !client->inuse )
      continue;

    // respawn if dead
    if( client->health <= 0 )
      respawn(client);

    MoveClientToIntermission( client );
  }

  // send the current scoring to all clients
  SendScoreboardMessageToAllClients( );
}


/*
=============
ExitLevel

When the intermission has been exited, the server is either moved
to a new map based on the map rotation or the current map restarted
=============
*/
void ExitLevel( void )
{
  int       i;
  gclient_t *cl;

  if ( G_MapExists( g_nextMap.string ) )
  {
    G_MapConfigs( g_nextMap.string );
    trap_SendConsoleCommand( EXEC_APPEND, va( "%smap \"%s\"\n",
      ( g_cheats.integer ? "dev" : "" ), g_nextMap.string ) );
  }
  else if( G_PlayMapActive( ) )
    G_NextPlayMap();
  else if( G_MapRotationActive( ) )
    G_AdvanceMapRotation( 0 );
  else // Otherwise just restart current map
  {
    char map[ MAX_CVAR_VALUE_STRING ];
    trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
    G_MapConfigs( map );
    trap_SendConsoleCommand( EXEC_APPEND, "map_restart\n" );
  }

  trap_Cvar_Set( "g_nextMap", "" );
  trap_Cvar_Set( "g_warmup", "1" );
  trap_SetConfigstring( CS_WARMUP, va( "%d", IS_WARMUP ) );
  
  level.restarted = qtrue;
  level.changemap = NULL;
  level.intermissiontime = 0;

  // reset all the scores so we don't enter the intermission again
  for( i = 0; i < g_maxclients.integer; i++ )
  {
    cl = level.clients + i;
    if( cl->pers.connected != CON_CONNECTED )
      continue;

    cl->ps.persistant[ PERS_SCORE ] = 0;
  }

  // we need to do this here before changing to CON_CONNECTING
  G_WriteSessionData( );

  // change all client states to connecting, so the early players into the
  // next level will know the others aren't done reconnecting
  for( i = 0; i < g_maxclients.integer; i++ )
  {
    if( level.clients[ i ].pers.connected == CON_CONNECTED )
      level.clients[ i ].pers.connected = CON_CONNECTING;
  }

}

/*
=================
G_AdminMessage

Print to all active server admins, and to the logfile, and to the server console
=================
*/
void G_AdminMessage( gentity_t *ent, const char *msg )
{
  char    string[ 1024 ];
  int     i;

  Com_sprintf( string, sizeof( string ), "chat %d %d \"%s\"",
    (int)( ent ? ent - g_entities : -1 ),
    G_admin_permission( ent, ADMF_ADMINCHAT ) ? SAY_ADMINS : SAY_ADMINS_PUBLIC,
    msg );

  // Send to all appropriate clients
  for( i = 0; i < level.maxclients; i++ )
    if( G_admin_permission( &g_entities[ i ], ADMF_ADMINCHAT ) )
       trap_SendServerCommand( i, string );

  // Send to the logfile and server console
  G_LogPrintf( "%s: %d \"%s" S_COLOR_WHITE "\": " S_COLOR_MAGENTA "%s\n",
    G_admin_permission( ent, ADMF_ADMINCHAT ) ? "AdminMsg" : "AdminMsgPublic",
    (int)( ent ? ent - g_entities : -1 ), ent ? ent->client->pers.netname : "console",
    msg );
}


/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open, and to the server console
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... )
{
  va_list argptr;
  char    string[ 1024 ], decolored[ 1024 ];
  int     min, tens, sec;

  sec = ( level.time - level.startTime ) / 1000;

  min = sec / 60;
  sec -= min * 60;
  tens = sec / 10;
  sec -= tens * 10;

  Com_sprintf( string, sizeof( string ), "%3i:%i%i ", min, tens, sec );

  va_start( argptr, fmt );
  Q_vsnprintf( string + 7, sizeof( string ) - 7, fmt, argptr );
  va_end( argptr );

  if( g_dedicated.integer )
  {
    G_UnEscapeString( string, decolored, sizeof( decolored ) );
    G_Printf( "%s", decolored + 7 );
  }

  if( !level.logFile )
    return;

  G_DecolorString( string, decolored, sizeof( decolored ) );
  trap_FS_Write( decolored, strlen( decolored ), level.logFile );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string )
{
  int         i, numSorted;
  gclient_t   *cl;
  gentity_t   *ent;
  char        map[ MAX_QPATH ];

  trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
  pack_start( level.database_data, DATABASE_DATA_MAX );
  pack_text2( map );
  pack_text2( (char *)string );
  pack_int( level.numConnectedClients );
  pack_int( level.epochStartTime );
  trap_Query( DB_MAPSTAT_ADD, level.database_data, NULL );

  level.exited = qtrue;

  G_LogPrintf( "Exit: %s\n", string );

  level.intermissionQueued = level.time;

  // this will keep the clients from playing any voice sounds
  // that will get cut off when the queued intermission starts
  trap_SetConfigstring( CS_INTERMISSION, "1" );

  // don't send more than 32 scores (FIXME?)
  numSorted = level.numConnectedClients;
  if( numSorted > 32 )
    numSorted = 32;

  for( i = 0; i < numSorted; i++ )
  {
    int   ping;

    cl = &level.clients[ level.sortedClients[ i ] ];

    if( cl->ps.stats[ STAT_TEAM ] == TEAM_NONE )
      continue;

    if( cl->pers.connected == CON_CONNECTING )
      continue;

    ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

    G_LogPrintf( "score: %i  ping: %i  client: %i %s\n",
      cl->ps.persistant[ PERS_SCORE ], ping, level.sortedClients[ i ],
      cl->pers.netname );

  }

  for( i = 1, ent = g_entities + i ; i < level.num_entities ; i++, ent++ )
  {
    if( !ent->inuse )
      continue;

    if( !Q_stricmp( ent->classname, "trigger_win" ) )
    {
      if( level.lastWin == ent->stageTeam )
        ent->use( ent, ent, ent );
    }
  }
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void )
{
  int       ready, notReady;
  int       i;
  gclient_t *cl;
  clientList_t readyMasks;

  //if no clients are connected, just exit
  if( level.numConnectedClients == 0 )
  {
    ExitLevel( );
    return;
  }

  // see which players are ready
  ready = 0;
  notReady = 0;
  Com_Memset( &readyMasks, 0, sizeof( readyMasks ) );
  for( i = 0; i < g_maxclients.integer; i++ )
  {
    cl = level.clients + i;

    if( cl->pers.connected != CON_CONNECTED )
      continue;

    if( cl->ps.stats[ STAT_TEAM ] == TEAM_NONE )
      continue;

    if( cl->readyToExit )
    {
      ready++;

      Com_ClientListAdd( &readyMasks, i );
    }
    else
      notReady++;
  }

  trap_SetConfigstring( CS_CLIENTS_READY, Com_ClientListString( &readyMasks ) );

  // never exit in less than five seconds
  if( level.time < level.intermissiontime + 5000 )
    return;

  // never let intermission go on for over 1 minute
  if( level.time > level.intermissiontime + 60000 )
  {
    ExitLevel( );
    return;
  }

  // if nobody wants to go, clear timer
  if( ready == 0 && notReady > 0 )
  {
    level.readyToExit = qfalse;
    return;
  }

  // if everyone wants to go, go now
  if( notReady == 0 )
  {
    ExitLevel( );
    return;
  }

  // the first person to ready starts the thirty second timeout
  if( !level.readyToExit )
  {
    level.readyToExit = qtrue;
    level.exitTime = level.time;
  }

  // if we have waited thirty seconds since at least one player
  // wanted to exit, go ahead
  if( level.time < level.exitTime + 30000 )
    return;

  ExitLevel( );
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void )
{
  int   a, b;

  if( level.numPlayingClients < 2 )
    return qfalse;

  a = level.clients[ level.sortedClients[ 0 ] ].ps.persistant[ PERS_SCORE ];
  b = level.clients[ level.sortedClients[ 1 ] ].ps.persistant[ PERS_SCORE ];

  return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void )
{
  // if at the intermission, wait for all non-bots to
  // signal ready, then go to next level
  if( level.intermissiontime )
  {
    CheckIntermissionExit( );
    return;
  }

  if( level.intermissionQueued )
  {
    if( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME )
    {
      level.intermissionQueued = 0;
      BeginIntermission( );
    }

    return;
  }

  if( g_timelimit.integer )
  {
    if( level.time - level.startTime >= g_timelimit.integer * 60000 )
    {
      level.lastWin = TEAM_NONE;
      trap_SendServerCommand( -1, "print \"Timelimit hit\n\"" );
      trap_SetConfigstring( CS_WINNER, "Stalemate" );
      LogExit( "Timelimit hit." );
      return;
    }
    else if( level.time - level.startTime >= ( g_timelimit.integer - 5 ) * 60000 &&
          level.timelimitWarning < TW_IMMINENT )
    {
      trap_SendServerCommand( -1, "cp \"5 minutes remaining!\"" );
      level.timelimitWarning = TW_IMMINENT;
    }
    else if( level.time - level.startTime >= ( g_timelimit.integer - 1 ) * 60000 &&
          level.timelimitWarning < TW_PASSED )
    {
      trap_SendServerCommand( -1, "cp \"1 minute remaining!\"" );
      level.timelimitWarning = TW_PASSED;
    }
  }

  if( level.uncondHumanWin ||
      ( !level.uncondAlienWin &&
        ( level.time > level.startTime + 1000 ) &&
        ( level.numAlienSpawns == 0 ) &&
        ( level.numAlienClientsAlive == 0 ) ) )
  {
    // We do not want any team to win in warmup
    if( IS_WARMUP )
    {
      if( level.lastLayoutReset > ( level.time - 5000 ) )
        return;
      // FIXME: this is awfully ugly
      G_LevelRestart( qfalse );
      trap_SendServerCommand( -1, "print \"A mysterious force restores balance in the universe.\n\"");
      return;
    }

    //humans win
    level.lastWin = TEAM_HUMANS;
    trap_SendServerCommand( -1, "print \"Humans win\n\"");
    trap_SetConfigstring( CS_WINNER, "Humans Win" );
    LogExit( "Humans win." );
  }
  else if( level.uncondAlienWin ||
           ( ( level.time > level.startTime + 1000 ) &&
             ( level.numHumanSpawns == 0 ) &&
             ( level.numHumanClientsAlive == 0 ) ) )
  {
    // We do not want any team to win in warmup
    if( IS_WARMUP )
    {
      if( level.lastLayoutReset > ( level.time - 5000 ) )
        return;
      // FIXME: this is awfully ugly
      G_LevelRestart( qfalse );
      trap_SendServerCommand( -1, "print \"A mysterious force restores balance in the universe.\n\"");
      return;
    }

    //aliens win
    level.lastWin = TEAM_ALIENS;
    trap_SendServerCommand( -1, "print \"Aliens win\n\"");
    trap_SetConfigstring( CS_WINNER, "Aliens Win" );
    LogExit( "Aliens win." );
  }
}

/*
==================
G_LevelRestart

Restart level and optionally specify non-warmup
==================
*/
void G_LevelRestart( qboolean stopWarmup )
{
  char      map[ MAX_CVAR_VALUE_STRING ];
  int       i;
  gclient_t *cl;

  for( i = 0; i < g_maxclients.integer; i++ )
  {
    cl = level.clients + i;
    if( cl->pers.connected != CON_CONNECTED )
      continue;

    if( cl->pers.teamSelection == TEAM_NONE )
      continue;

    cl->sess.restartTeam = cl->pers.teamSelection;
  }

  trap_Cvar_Set( "g_nextLayout", level.layout );
  if( stopWarmup )
  {
    trap_Cvar_Set( "g_warmup", "0" );
    trap_SetConfigstring( CS_WARMUP, va( "%d", IS_WARMUP ) );
  }
  trap_Cvar_Update( &g_cheats );
  trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
  G_MapConfigs( map );
  trap_SendConsoleCommand( EXEC_APPEND, "map_restart\n" );

  if( !stopWarmup )
    level.lastLayoutReset = level.time;

}

/*
==================
G_LevelReady

Check for readyness of players and get out of warmup to start the real game if
required percentages are met.
==================
*/
void G_LevelReady( void )
{
  int       i, numAliens = 0, numHumans = 0;
  float     percentAliens, percentHumans, threshold;
  qboolean  startGame;

  // do not proceed if not in warmup
  if( !g_warmup.integer )
    return;

  // reset number of players ready to zero
  level.readyToPlay[ TEAM_HUMANS ] = level.readyToPlay[ TEAM_ALIENS ] = 0;

  // update number of clients ready to play
  for( i = 0; i < level.maxclients; i++ )
  {
    if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
    {
      // spectators are not counted
      if( level.clients[ i ].pers.teamSelection == TEAM_NONE )
        continue;

      if( level.clients[ i ].pers.teamSelection == TEAM_ALIENS )
      {
        numAliens++;
        if( level.clients[ i ].pers.readyToPlay )
          level.readyToPlay[ TEAM_ALIENS ]++;
      }
      else if( level.clients[ i ].pers.teamSelection == TEAM_HUMANS )
      {
        numHumans++;
        if( level.clients[ i ].pers.readyToPlay )
          level.readyToPlay[ TEAM_HUMANS ]++;
      }
    }
  }

  // calculate percentage ready for both teams
  percentAliens = ( (float) level.readyToPlay[ TEAM_ALIENS ] /
      ( numAliens > 0 ? (float) numAliens : 1.0 ) ) * 100.0;
  percentHumans = ( (float) level.readyToPlay[ TEAM_HUMANS ] /
      ( numHumans > 0 ? (float) numHumans : 1.0 ) ) * 100.0;

  // if cvar g_warmupReadyThreshold is > 50 use it as threshold, if not use 50
  threshold = g_warmupReadyThreshold.integer > 50 ?
    (float) g_warmupReadyThreshold.integer : 50.0;

  // default condition for ending warmup is when >= (threshold)% of both teams
  // are ready
  startGame = ( percentAliens >= threshold && percentHumans >= threshold );

  // if previous conditions are not met and there are at least
  // (g_warmupTimeout1Trigger) players who are not spectators, start a
  // (g_warmupTimeout1) second countdown until warmup timeout
  if( !startGame && ( numAliens > 0 ) && ( numHumans > 0 ) &&
      ( numAliens + numHumans ) >= g_warmupTimeout1Trigger.integer )
  {
    if( ( level.warmup1Time > -1 ) &&
        ( level.time - level.warmup1Time ) >= ( g_warmupTimeout1.integer * 1000 ) )
    {
      startGame = qtrue;
    }
    else if ( level.warmup1Time == -1 ) // start countdown
    {
      trap_SendServerCommand( -1, va( "print \"There are at least %d "
            " non-spectating players. Pre-game warmup will expire in %d"
            " seconds.\n\"", g_warmupTimeout1Trigger.integer,
            g_warmupTimeout1.integer ) );
      level.warmup1Time = level.time;
    }
  }
  else
  {
    // reset the 5 minute timeout
    level.warmup1Time = -1;
  }

  // if previous conditions are not met and at least (g_warmupTimeout2Trigger)%
  // of players in one team is ready, start a (g_warmupTimeout2) second
  // countdown until warmup timeout
  if( !startGame && ( numAliens > 1 ) && ( numHumans > 1 ) &&
      ( percentAliens >= (float) g_warmupTimeout2Trigger.integer ||
        percentHumans >= (float) g_warmupTimeout2Trigger.integer ) )
  {
    if( ( level.warmup2Time > -1 ) &&
        ( level.time - level.warmup2Time ) >= ( g_warmupTimeout2.integer * 1000 ) )
    {
      startGame = qtrue;
    }
    else if ( level.warmup2Time == -1 ) // start countdown
    {
      trap_SendServerCommand( -1, va( "print \"At least %d percent of players"
            " in a team have become ready. Pre-game warmup will expire in %d"
            " seconds.\n\"", g_warmupTimeout2Trigger.integer,
            g_warmupTimeout2.integer ) );
      level.warmup2Time = level.time;
    }
  }
  else
  {
    // reset the 1 minute timeout
    level.warmup2Time = -1;
  }

  trap_SetConfigstring( CS_WARMUP_READY, va( "%.0f %d %d %.2f %d %d %d %d",
        percentAliens, level.readyToPlay[ TEAM_ALIENS ], numAliens,
        percentHumans, level.readyToPlay[ TEAM_HUMANS ], numHumans,
        ( level.warmup1Time > -1 ?  ( level.time - level.warmup1Time ) / 1000 :
          -1 ),
        ( level.warmup2Time > -1 ?  ( level.time - level.warmup2Time ) / 1000 :
          -1 ) ) );

  // only continue to start game when conditions are met
  if( !startGame )
    return;

  G_LevelRestart( qtrue );
}

/*
==================
G_Vote
==================
*/
void G_Vote( gentity_t *ent, team_t team, qboolean voting )
{
  if( !level.voteTime[ team ] )
    return;

  if( voting && ent->client->pers.voted & ( 1 << team ) )
    return;

  if( !voting && !( ent->client->pers.voted & ( 1 << team ) ) )
    return;

  ent->client->pers.voted |= 1 << team;

  if( ent->client->pers.vote & ( 1 << team ) )
  {
    if( voting )
      level.voteYes[ team ]++;
    else
      level.voteYes[ team ]--;

    trap_SetConfigstring( CS_VOTE_YES + team,
      va( "%d", level.voteYes[ team ] ) );
  }
  else
  {
    if( voting )
      level.voteNo[ team ]++;
    else
      level.voteNo[ team ]--;

    trap_SetConfigstring( CS_VOTE_NO + team,
      va( "%d", level.voteNo[ team ] ) );
  }
}


/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


void G_ExecuteVote( team_t team )
{
  level.voteExecuteTime[ team ] = 0;

  if( !Q_stricmpn( level.voteString[ team ], "map_restart", 11 ) )
  {
    char map[ MAX_QPATH ];
    trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
    G_MapConfigs( map );
  }
  else if( !Q_stricmpn( level.voteString[ team ], "map", 3 ) )
  {
    char map[ MAX_QPATH ];
    char *p;
    Q_strncpyz( map, strchr( level.voteString[ team ], '"' ) + 1, sizeof( map ) );
    if( ( p = strchr( map, '"' ) ) )
      *p = '\0';
    G_MapConfigs( map );
  }

  trap_SendConsoleCommand( EXEC_APPEND, va( "%s%s\n",
    ( !Q_stricmp( level.voteString[ team ], "map" ) && g_cheats.integer ? "dev" : "" ),
    level.voteString[ team ] ) );

  if( !Q_stricmpn( level.voteString[ team ], "map", 3 ) )
    level.restarted = qtrue;
}

/*
==================
G_CheckVote
==================
*/
void G_CheckVote( team_t team )
{
  float    votePassThreshold = (float)level.voteThreshold[ team ] / 100.0f;
  qboolean pass = qfalse;
  char     *msg;
  int      i;
  int      abstained;
  int      numCountedVotingClients = 0;

  if( level.voteExecuteTime[ team ] &&
      level.voteExecuteTime[ team ] < level.time )
  {
    G_ExecuteVote( team );
  }

  if( !level.voteTime[ team ] )
    return;

  // Recalculate number of voting clients if there are players in teams
  // and only count client as a voting client if they are either
  // (a) a non-spectator, or
  // (b) a spectator who has voted
  for( i = 0; i < level.maxclients; i++ )
  {
    if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
    {
      switch( team )
      {
        case TEAM_ALIENS:
        case TEAM_HUMANS:
          if( level.clients[ i ].pers.teamSelection == team )
            numCountedVotingClients++;
          break;
        default:
          if( level.clients[ i ].pers.teamSelection != TEAM_NONE ||
              ( level.clients[ i ].pers.teamSelection == TEAM_NONE &&
                level.clients[ i ].pers.voted & ( 1 << TEAM_NONE ) ) )
            numCountedVotingClients++;
          break;
      }
    }
  }

  if( ( level.time - level.voteTime[ team ] >= VOTE_TIME ) ||
      ( level.voteYes[ team ] + level.voteNo[ team ] == level.numVotingClients[ team ] ) )
  {
    pass = ( level.voteYes[ team ] &&
        (float)level.voteYes[ team ] / numCountedVotingClients > votePassThreshold );
  }
  else
  {
    if( (float)level.voteYes[ team ] >
        (float)level.numVotingClients[ team ] * votePassThreshold )
    {
      pass = qtrue;
    }
    else if( (float)level.voteNo[ team ] <=
        (float)level.numVotingClients[ team ] * ( 1.0f - votePassThreshold ) )
    {
      return;
    }
  }

  if( pass )
    level.voteExecuteTime[ team ] = level.time + level.voteDelay[ team ];

  G_LogPrintf( "EndVote: %s %s %d %d %d\n",
      team == TEAM_NONE ? "global" : BG_TeamName( team ),
      pass ? "pass" : "fail",
      level.voteYes[ team ], level.voteNo[ team ], level.numVotingClients[ team ] );

  abstained = numCountedVotingClients - level.voteYes[ team ] -
    level.voteNo[ team ];

  msg = va( "print \"%sote %sed (%d for, %d against and %d abstained)\n\"",
      team == TEAM_NONE ? "V" : "Team v", pass ? "pass" : "fail",
      level.voteYes[ team ], level.voteNo[ team ], abstained );

  if( team == TEAM_NONE )
    trap_SendServerCommand( -1, msg );
  else
    G_TeamCommand( team, msg );

  level.voteTime[ team ] = 0;
  level.voteYes[ team ] = 0;
  level.voteNo[ team ] = 0;

  for( i = 0; i < level.maxclients; i++ )
    level.clients[ i ].pers.voted &= ~( 1 << team );

  trap_SetConfigstring( CS_VOTE_TIME + team, "" );
  trap_SetConfigstring( CS_VOTE_STRING + team, "" );
  trap_SetConfigstring( CS_VOTE_YES + team, "0" );
  trap_SetConfigstring( CS_VOTE_NO + team, "0" );
}


/*
==================
G_EndVote
==================
*/
void G_EndVote( team_t team, qboolean cancel )
{
  int i;

  for( i = 0; i < level.maxclients; i++ )
  {
    if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
    {
      if ( team == TEAM_NONE || level.clients[ i ].pers.teamSelection == team )
      {
        if( cancel )
          level.clients[ i ].pers.vote &= ~( 1 << team );
        else
          level.clients[ i ].pers.vote |= 1 << team;

        level.clients[ i ].pers.voted |= 1 << team;
      }
    }
  }

  level.voteNo[ team ] = cancel ? level.numVotingClients[ team ] : 0;
  level.voteYes[ team ] = cancel ? 0 : level.numVotingClients[ team ];

  G_CheckVote( team );
}

/*
==================
CheckCvars
==================
*/
void CheckCvars( void )
{
  static int lastPasswordModCount   = -1;
  static int lastMarkDeconModCount  = -1;
  static int lastSDTimeModCount = -1;
  static int lastNumZones = 0;

  if( g_password.modificationCount != lastPasswordModCount )
  {
    lastPasswordModCount = g_password.modificationCount;

    if( *g_password.string && Q_stricmp( g_password.string, "none" ) )
      trap_Cvar_Set( "g_needpass", "1" );
    else
      trap_Cvar_Set( "g_needpass", "0" );
  }

  // Unmark any structures for deconstruction when
  // the server setting is changed
  if( g_markDeconstruct.modificationCount != lastMarkDeconModCount )
  {
    lastMarkDeconModCount = g_markDeconstruct.modificationCount;
    G_ClearDeconMarks( );
  }

  // If we change g_suddenDeathTime during a map, we need to update
  // when sd will begin
  if( g_suddenDeathTime.modificationCount != lastSDTimeModCount )
  {
    lastSDTimeModCount = g_suddenDeathTime.modificationCount;
    level.suddenDeathBeginTime = g_suddenDeathTime.integer * 60000;
  }

  // If the number of zones changes, we need a new array
  if( g_humanRepeaterMaxZones.integer != lastNumZones )
  {
    buildPointZone_t  *newZones;
    size_t            newsize = g_humanRepeaterMaxZones.integer * sizeof( buildPointZone_t );
    size_t            oldsize = lastNumZones * sizeof( buildPointZone_t );

    newZones = BG_Alloc( newsize );
    if( level.buildPointZones )
    {
      Com_Memcpy( newZones, level.buildPointZones, MIN( oldsize, newsize ) );
      BG_Free( level.buildPointZones );
    }

    level.buildPointZones = newZones;
    lastNumZones = g_humanRepeaterMaxZones.integer;
  }

  level.frameMsec = trap_Milliseconds( );
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink( gentity_t *ent )
{
  float thinktime;

  thinktime = ent->nextthink;
  if( thinktime <= 0 )
    return;

  if( thinktime > level.time )
    return;

  ent->nextthink = 0;
  if( !ent->think )
    G_Error( "NULL ent->think" );

  ent->think( ent );
}

/*
=============
G_EvaluateAcceleration

Calculates the acceleration for an entity
=============
*/
void G_EvaluateAcceleration( gentity_t *ent, int msec )
{
  vec3_t  deltaVelocity;
  vec3_t  deltaAccel;

  VectorSubtract( ent->s.pos.trDelta, ent->oldVelocity, deltaVelocity );
  VectorScale( deltaVelocity, 1.0f / (float)msec, ent->acceleration );

  VectorSubtract( ent->acceleration, ent->oldAccel, deltaAccel );
  VectorScale( deltaAccel, 1.0f / (float)msec, ent->jerk );

  VectorCopy( ent->s.pos.trDelta, ent->oldVelocity );
  VectorCopy( ent->acceleration, ent->oldAccel );
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime )
{
  int        i;
  gentity_t  *ent;
  int        msec;
  static int ptime3000 = 0;

  // if we are waiting for the level to restart, do nothing
  if( level.restarted )
    return;

  if( level.pausedTime )
  {
    msec = levelTime - level.time - level.pausedTime;
    level.pausedTime = levelTime - level.time;

    ptime3000 += msec;
    while( ptime3000 > 3000 )
    {
      ptime3000 -= 3000;
      trap_SendServerCommand( -1, "cp \"The game has been paused. Please wait.\"" );

      if( level.pausedTime >= 110000  && level.pausedTime <= 119000 )
        trap_SendServerCommand( -1, va( "print \"Server: Game will auto-unpause in %d seconds\n\"",
          (int) ( (float) ( 120000 - level.pausedTime ) / 1000.0f ) ) );
    }

    // Prevents clients from getting lagged-out messages
    for( i = 0; i < level.maxclients; i++ )
    {
      if( level.clients[ i ].pers.connected == CON_CONNECTED )
        level.clients[ i ].ps.commandTime = levelTime;
    }

    if( level.pausedTime > 120000 )
    {
      trap_SendServerCommand( -1, "print \"Server: The game has been unpaused automatically (2 minute max)\n\"" );
      trap_SendServerCommand( -1, "cp \"The game has been unpaused!\"" );
      level.pausedTime = 0;
    }

    return;
  }

  level.framenum++;
  level.previousTime = level.time;
  level.time = levelTime;
  msec = level.time - level.previousTime;

  // get any cvar changes
  G_UpdateCvars( );
  CheckCvars( );
  // now we are done spawning
  level.spawning = qfalse;

  //
  // go through all allocated objects
  //
  ent = &g_entities[ 0 ];

  for( i = 0; i < level.num_entities; i++, ent++ )
  {
    if( !ent->inuse )
      continue;

    // clear events that are too old
    if( level.time - ent->eventTime > EVENT_VALID_MSEC )
    {
      if( ent->s.event )
      {
        ent->s.event = 0; // &= EV_EVENT_BITS;
        if ( ent->client )
        {
          ent->client->ps.externalEvent = 0;
          //ent->client->ps.events[0] = 0;
          //ent->client->ps.events[1] = 0;
        }
      }

      if( ent->freeAfterEvent )
      {
        // tempEntities or dropped items completely go away after their event
        G_FreeEntity( ent );
        continue;
      }
      else if( ent->unlinkAfterEvent )
      {
        // items that will respawn will hide themselves after their pickup event
        ent->unlinkAfterEvent = qfalse;
        trap_UnlinkEntity( ent );
      }
    }

    // temporary entities don't think
    if( ent->freeAfterEvent )
      continue;

    // calculate the acceleration of this entity
    if( ent->evaluateAcceleration )
      G_EvaluateAcceleration( ent, msec );

    if( !ent->r.linked && ent->neverFree )
      continue;

    if( ent->s.eType == ET_MISSILE )
    {
      G_RunMissile( ent );
      continue;
    }

    if( ent->s.eType == ET_BUILDABLE )
    {
      G_BuildableThink( ent, msec );
      continue;
    }

    if( ent->s.eType == ET_CORPSE || ent->physicsObject )
    {
      G_Physics( ent, msec );
      continue;
    }

    if( ent->s.eType == ET_MOVER )
    {
      G_RunMover( ent );
      continue;
    }

    if( i < MAX_CLIENTS )
    {
      G_RunClient( ent );
      continue;
    }

    G_RunThink( ent );
  }

  // perform final fixups on the players
  ent = &g_entities[ 0 ];

  for( i = 0; i < level.maxclients; i++, ent++ )
  {
    if( ent->inuse )
      ClientEndFrame( ent );
  }

  // save position information for all active clients
  G_UnlaggedStore( );

  G_CountSpawns( );
  if( !g_doCountdown.integer || level.countdownTime <= level.time )
  {
    G_CalculateBuildPoints( );
    G_CalculateStages( );
    G_SpawnClients( TEAM_ALIENS );
    G_SpawnClients( TEAM_HUMANS );
    G_CalculateAvgPlayers( );
    G_UpdateZaps( msec );
  }
  G_UpdateBuildableRangeMarkers( );

  // check for warmup timeout
  G_LevelReady( );

  // see if it is time to end the level
  CheckExitRules( );

  // update to team status?
  CheckTeamStatus( );

  // cancel vote if timed out
  for( i = 0; i < NUM_TEAMS; i++ )
    G_CheckVote( i );

  level.frameMsec = trap_Milliseconds();
}

