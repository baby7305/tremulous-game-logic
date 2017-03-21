/*
===========================================================================
Copyright (C) 2008-2009 Amanieu d'Antras

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

#define PORTAL_MINRANGE 20.0f
#define PORTAL_MAXRANGE 100.0f
#define PORTAL_OFFSET 4.0f

/*
===============
G_Portal_Effect

Cool effects for the portals when they are used or cleared
===============
*/
static void G_Portal_Effect( portal_t portalindex, float speedMod )
{
	gentity_t *portal = level.humanPortals.portals[portalindex];
	gentity_t *effect = G_Spawn( );
	int       speed = (int)((float)(PORTALGUN_SPEED) * speedMod);

	VectorCopy( portal->r.currentOrigin , effect->r.currentOrigin );
	effect->s.eType = ET_MISSILE;
	effect->s.weapon = WP_PORTAL_GUN;
	if( portalindex == PORTAL_RED )
		effect->s.generic1 = WPM_PRIMARY;
	else
		effect->s.generic1 = WPM_SECONDARY;

	effect->s.pos.trType = TR_ACCEL;
  effect->s.pos.trDuration = speed * 2;
  effect->s.pos.trTime = level.time;
  VectorCopy( effect->r.currentOrigin, effect->s.pos.trBase );
  VectorScale( portal->s.origin2, speed, effect->s.pos.trDelta );
  SnapVector( effect->s.pos.trDelta );      // save net bandwidth

	G_AddEvent( effect, EV_MISSILE_MISS, DirToByte( portal->s.origin2 ) );
	effect->freeAfterEvent = qtrue;
	trap_LinkEntity( effect );
}

/*
===============
G_Portal_Clear

Delete a portal
===============
*/
void G_Portal_Clear( portal_t portalindex )
{
	gentity_t *self = level.humanPortals.portals[portalindex];
	if (!self)
		return;

		
	G_Portal_Effect( portalindex, 0.25 );
	level.humanPortals.createTime[ portalindex ] = 0;
	trap_SetConfigstring( ( CS_HUMAN_PORTAL_CREATETIME + portalindex ),
												va( "%i", 0 ) );
	level.humanPortals.portals[portalindex] = NULL;
	G_FreeEntity( self );
}

/*
===============
G_Portal_Touch

Send someone over to the other portal.
===============
*/
static void G_Portal_Touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	gentity_t *portal;
	vec3_t origin, dir, end, angles;
	trace_t tr;
	int speed, i;

	if (!other->client)
		return;

	portal = level.humanPortals.portals[ !self->s.modelindex2 ];
	if (!portal)
		return;

	// Check if there is room to spawn
	VectorCopy(portal->r.currentOrigin, origin);
	VectorCopy(portal->s.origin2, dir);
	for (i = PORTAL_MINRANGE; i < PORTAL_MAXRANGE; i++)
    {
		VectorMA(origin, i, dir, end);
		trap_Trace(&tr, origin, NULL, NULL, end, portal->s.number, MASK_SHOT);
		if (tr.fraction != 1.0f)
        {
			return;
        }
		trap_Trace(&tr, end, other->r.mins, other->r.maxs, end, -1, MASK_PLAYERSOLID | CONTENTS_TELEPORTER);
		if (tr.fraction == 1.0f)
        {
			break;
        }
	}

	if (i == PORTAL_MAXRANGE)
		return;

	// Teleport!
	trap_UnlinkEntity(other);
	VectorCopy(end, other->client->ps.origin);
	speed = VectorLength(other->client->ps.velocity);
	VectorScale(portal->s.origin2, speed, other->client->ps.velocity);
	other->client->ps.eFlags ^= EF_TELEPORT_BIT;
	G_UnlaggedClear(other);
	if (dir[0] || dir[1])
    {
		if (other->client->portalTime < level.time)
        {
			vectoangles(dir, angles);
			G_SetClientViewAngle(other, angles);
		}
		other->client->portalTime = level.time + 250;
	}
	BG_PlayerStateToEntityState(&other->client->ps, &other->s, qtrue);
	VectorCopy(other->client->ps.origin, other->r.currentOrigin);
	trap_LinkEntity(other);
	for( i = 0; i < PORTAL_NUM; i++ )
		G_Portal_Effect( i, 0.0625 );
}

/*
===============
G_Portal_Create

This is used to spawn a portal.
===============
*/
void G_Portal_Create(gentity_t *ent, vec3_t origin, vec3_t normal, portal_t portalindex)
{
	gentity_t *portal;
	vec3_t range = { PORTAL_MINRANGE, PORTAL_MINRANGE, PORTAL_MINRANGE };

    if ( ent->health <= 0 || !ent->client || ent->client->ps.pm_type == PM_DEAD )
      return;

	// Create the portal
	portal = G_Spawn();
	portal->r.contents = CONTENTS_TRIGGER | CONTENTS_TELEPORTER;
	portal->s.eType = ET_TELEPORTAL;
	portal->touch = G_Portal_Touch;
	portal->s.modelindex = BA_H_SPAWN;
	portal->s.modelindex2 = portalindex;
	portal->s.frame = 3;
	VectorCopy(range, portal->r.maxs);
	VectorScale(range, -1, portal->r.mins);
	VectorMA(origin, PORTAL_OFFSET, normal, origin);
	G_SetOrigin(portal, origin);
	VectorCopy(normal, portal->s.origin2);
	trap_LinkEntity(portal);

	// Attach it to the client
	G_Portal_Clear( portalindex );
	portal->parent = ent;
	level.humanPortals.portals[ portalindex ] = portal;
	level.humanPortals.lifetime[ portalindex ] = level.time + PORTAL_LIFETIME;
	level.humanPortals.createTime[ portalindex ] = PORTAL_CREATED_REPEAT;
	trap_SetConfigstring( ( CS_HUMAN_PORTAL_CREATETIME + portalindex ),
												va( "%i", level.humanPortals.createTime[ portalindex ] ) );
}