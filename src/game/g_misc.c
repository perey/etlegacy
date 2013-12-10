/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 *
 * @file g_misc.c
 */

#include "g_local.h"

extern void AimAtTarget(gentity_t *self);
extern float AngleDifference(float ang1, float ang2);

/*
QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/

/*
QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_camp(gentity_t *self)
{
	G_SetOrigin(self, self->s.origin);
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null(gentity_t *self)
{
	// if it has a targetname, let it stick around for a few frames
	if (!self->targetname || !*self->targetname)
	{
		G_FreeEntity(self);
	}

	self->think     = G_FreeEntity;
	self->nextthink = level.time + (FRAMETIME * 2);
}

/*
QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull(gentity_t *self)
{
	G_SetOrigin(self, self->s.origin);
}

/*
QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point q3map_non-dynamic
Non-displayed light.
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
"q3map_non-dynamic" specifies that this light should not contribute to the world's 'light grid' and therefore will not light dynamic models in the game.(wolf)
*/
void SP_light(gentity_t *self)
{
	G_FreeEntity(self);
}

/*
QUAKED lightJunior (0 0.7 0.3) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point
Non-displayed light that only affects dynamic game models, but does not contribute to lightmaps
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
*/
void SP_lightJunior(gentity_t *self)
{
	G_FreeEntity(self);
}

/*
=================================================================================
TELEPORTERS
=================================================================================
*/
void TeleportPlayer(gentity_t *player, vec3_t origin, vec3_t angles)
{
	VectorCopy(origin, player->client->ps.origin);
	player->client->ps.origin[2] += 1;

	// spit the player out
	/*  AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	    VectorScale( player->client->ps.velocity, 400, player->client->ps.velocity );
	    player->client->ps.pm_time = 160;       // hold time
	    player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;*/
	// disabling, dont want the bots flying everywhere :)

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	G_ResetMarkers(player);

	// set angles
	SetClientViewAngle(player, angles);

	// save results of pmove
	BG_PlayerStateToEntityState(&player->client->ps, &player->s, level.time, qtrue);

	// use the precise origin for linking
	VectorCopy(player->client->ps.origin, player->r.currentOrigin);

	if (player->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap_LinkEntity(player);
	}
}

/*
QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest(gentity_t *ent)
{
}

void use_spotlight(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->r.linked)
	{
		trap_UnlinkEntity(ent);
	}
	else
	{
		gentity_t *tent;

		tent = G_PickTarget(ent->target);
		VectorCopy(tent->s.origin, ent->s.origin2);

		ent->active = 0;
		trap_LinkEntity(ent);
	}
}

void spotlight_finish_spawning(gentity_t *ent)
{
	if (ent->spawnflags & 1)       // START_ON
	{
		ent->active = 0;
		trap_LinkEntity(ent);
	}

	ent->use       = use_spotlight;
	ent->think     = 0;
	ent->nextthink = 0;
}

/*
QUAKED misc_spotlight (1 0 0) (-16 -16 -16) (16 16 16) START_ON BACK_AND_FORTH
"model" - 'base' model that moves with the light.  Default: "models/mapobjects/light/searchlight_pivot.md3"
"target" - .camera (spline) file for light to track.  do not specify file extension.

BACK_AND_FORTH - when end of target spline is hit, reverse direction rather than looping (looping is default)
( /\ not active yet /\ )
*/
void SP_misc_spotlight(gentity_t *ent)
{
	ent->s.eType = ET_EF_SPOTLIGHT;

	ent->think     = spotlight_finish_spawning;
	ent->nextthink = level.time + 100;

	if (ent->model)
	{
		ent->s.modelindex = G_ModelIndex(ent->model);
	}
	else
	{
		ent->s.modelindex = G_ModelIndex("models/mapobjects/light/searchlight_pivot.md3");
	}

	if (ent->target)
	{
		ent->s.density = G_FindConfigstringIndex(ent->target, CS_SPLINES, MAX_SPLINE_CONFIGSTRINGS, qtrue);
	}
}

//===========================================================

/*
QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"     arbitrary .md3 file to display
"modelscale"    scale multiplier (defaults to 1x)
"modelscale_vec"    scale multiplier (defaults to 1 1 1, scales each axis as requested)

"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both, the "modelscale" is ignored
*/
void SP_misc_model(gentity_t *ent)
{
	G_FreeEntity(ent);
}

/*
QUAKED misc_gamemodel (1 0 0) (-16 -16 -16) (16 16 16) ORIENT_LOD START_ANIMATE
md3 placed in the game at runtime (rather than in the bsp)
"model"         arbitrary .md3 file to display
"modelscale"    scale multiplier (defaults to 1x, and scales uniformly)
"modelscale_vec"    scale multiplier (defaults to 1 1 1, scales each axis as requested)
"skin"          .skin file used to define shaders for model
"trunk"         diameter of solid core (used for trace visibility and collision (not ai pathing))
"trunkheight"   height of trunk

"frames"        number of animation frames
"start"         frame to start on
"fps"           fps to animate with

ORIENT_LOD - if flagged, the entity will yaw towards the player when the LOD switches
START_ANIMATE - if flagged, the entity will spawn animating

"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both, the "modelscale" is ignored
*/
void SP_misc_gamemodel(gentity_t *ent)
{
	vec3_t vScale;
	int    trunksize, trunkheight;

	// static gamemodels client side only now :D so server can just wave bye-bye
	if (!ent->scriptName && !ent->targetname && !ent->spawnflags)
	{
		G_FreeEntity(ent);
		return;
	}

	ent->s.eType      = ET_GAMEMODEL;
	ent->s.modelindex = G_ModelIndex(ent->model);

	ent->s.modelindex2 = G_SkinIndex(ent->aiSkin);

	if (ent->spawnflags & 2)
	{
		int num_frames, start_frame, fps;

		G_SpawnInt("frames", "0", &num_frames);
		G_SpawnInt("start", "0", &start_frame);
		G_SpawnInt("fps", "20", &fps);

		if (num_frames == 0)
		{
			G_Error("'misc_model' with ANIMATE spawnflag set has 'frames' set to 0\n");
		}

		ent->s.torsoAnim = num_frames;
		ent->s.frame     = rand() % ent->s.torsoAnim;
		ent->s.loopSound = 0; // non-frozen

		// added start offset, fps, and direction
		ent->s.legsAnim = start_frame + 1;
		ent->s.weapon   = 1000.f / fps;
		// continue loop animation as long as s.teamNum == 0
		ent->s.teamNum = 0;
	}

	if (ent->model)
	{
		char tagname[MAX_QPATH];

		COM_StripExtension(ent->model, tagname, sizeof(tagname));
		Q_strcat(tagname, MAX_QPATH, ".tag");

		ent->tagNumber = trap_LoadTag(tagname);

		//if( !(ent->tagNumber = trap_LoadTag( tagname )) ) {
		//  Com_Error( ERR_DROP, "Failed to load Tag File (%s)", tagname );
		//}
	}

	if (!G_SpawnVector("modelscale_vec", "1 1 1", vScale))
	{
		vec_t scale;

		if (G_SpawnFloat("modelscale", "1", &scale))
		{
			VectorSet(vScale, scale, scale, scale);
		}
	}

	G_SpawnInt("trunk", "0", &trunksize);
	if (!G_SpawnInt("trunkhight", "0", &trunkheight))
	{
		trunkheight = 256;
	}

	if (trunksize)
	{
		float rad;

		ent->clipmask   = CONTENTS_SOLID;
		ent->r.contents = CONTENTS_SOLID;

		ent->r.svFlags |= SVF_CAPSULE;

		rad = (float)trunksize / 2.0f;
		VectorSet(ent->r.mins, -rad, -rad, 0);
		VectorSet(ent->r.maxs, rad, rad, trunkheight);
	}

	// scale is stored in 'angles2'
	VectorCopy(vScale, ent->s.angles2);

	G_SetOrigin(ent, ent->s.origin);
	VectorCopy(ent->s.angles, ent->s.apos.trBase);

	// hmmmmm, think this flag is prolly b0rked
	if (ent->spawnflags & 1)
	{
		ent->s.apos.trType = 1; // misc_gamemodels (since they have no movement) will use type = 0 for static models, type = 1 for auto-aligning models

	}
	trap_LinkEntity(ent);
}

void locateMaster(gentity_t *ent)
{
	ent->target_ent = G_FindByTargetname(NULL, ent->target);
	if (ent->target_ent)
	{
		ent->s.otherEntityNum = ent->target_ent->s.number;
	}
	else
	{
		G_Printf("Couldn't find target(%s) for misc_vis_dummy at %s\n", ent->target, vtos(ent->r.currentOrigin));
		G_FreeEntity(ent);
	}
}

/*
QUAKED misc_vis_dummy (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
For safety, you should have each dummy only point at one entity (however, it's okay to have many dummies pointing at one entity)
*/
void SP_misc_vis_dummy(gentity_t *ent)
{
	if (!ent->target)     // safety check
	{
		G_Printf("No target specified for misc_vis_dummy at %s\n", vtos(ent->r.currentOrigin));
		G_FreeEntity(ent);
		return;
	}

	ent->r.svFlags |= SVF_VISDUMMY;
	G_SetOrigin(ent, ent->s.origin);
	trap_LinkEntity(ent);

	ent->think     = locateMaster;
	ent->nextthink = level.time + 1000;

}

/*
QUAKED misc_vis_dummy_multiple (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
This entity was created to have multiple speakers targeting it
*/
void SP_misc_vis_dummy_multiple(gentity_t *ent)
{
	if (!ent->targetname)
	{
		G_Printf("misc_vis_dummy_multiple needs a targetname at %s\n", vtos(ent->r.currentOrigin));
		G_FreeEntity(ent);
		return;
	}

	ent->r.svFlags |= SVF_VISDUMMY_MULTIPLE;
	G_SetOrigin(ent, ent->s.origin);
	trap_LinkEntity(ent);
}

//===========================================================

/*
QUAKED misc_light_surface (1 .5 0) (-8 -8 -8) (8 8 8)
The surfaces nearest these entities will be the only surfaces lit by the targeting light
This must be within 64 world units of the surface to be lit!
*/
void SP_misc_light_surface(gentity_t *ent)
{
	G_FreeEntity(ent);
}

//===========================================================

void locateCamera(gentity_t *ent)
{
	vec3_t    dir;
	gentity_t *target;
	gentity_t *owner;

	owner = G_PickTarget(ent->target);
	if (!owner)
	{
		G_Printf("Couldn't find target for misc_partal_surface\n");
		G_FreeEntity(ent);
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if (owner->spawnflags & 1)
	{
		ent->s.frame = 25;
	}
	else if (owner->spawnflags & 2)
	{
		ent->s.frame = 75;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy(owner->s.origin, ent->s.origin2);

	// see if the portal_camera has a target
	target = G_PickTarget(owner->target);
	if (target)
	{
		VectorSubtract(target->s.origin, owner->s.origin, dir);
		VectorNormalize(dir);
	}
	else
	{
		G_SetMovedir(owner->s.angles, dir);
	}

	ent->s.eventParm = DirToByte(dir);
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void SP_misc_portal_surface(gentity_t *ent)
{
	VectorClear(ent->r.mins);
	VectorClear(ent->r.maxs);
	trap_LinkEntity(ent);

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType   = ET_PORTAL;

	if (!ent->target)
	{
		VectorCopy(ent->s.origin, ent->s.origin2);
	}
	else
	{
		ent->think     = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera(gentity_t *ent)
{
	float roll;

	VectorClear(ent->r.mins);
	VectorClear(ent->r.maxs);
	trap_LinkEntity(ent);

	G_SpawnFloat("roll", "0", &roll);

	ent->s.clientNum = roll / 360.0 * 256;
}

/*
======================================================================
  SHOOTERS
======================================================================
*/

void Use_Shooter(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	vec3_t dir;
	float  deg;
	vec3_t up, right;

	// see if we have a target
	if (ent->enemy)
	{
		VectorSubtract(ent->enemy->r.currentOrigin, ent->s.origin, dir);
		VectorNormalize(dir);
	}
	else
	{
		VectorCopy(ent->movedir, dir);
	}

	if (ent->s.weapon == WP_MAPMORTAR)
	{
		AimAtTarget(ent);     // store in ent->s.origin2 the direction/force needed to pass through the target
		VectorCopy(ent->s.origin2, dir);
	}

	// randomize a bit
	PerpendicularVector(up, dir);
	CrossProduct(up, dir, right);

	deg = crandom() * ent->random;
	VectorMA(dir, deg, up, dir);

	deg = crandom() * ent->random;
	VectorMA(dir, deg, right, dir);

	VectorNormalize(dir);

	switch (ent->s.weapon)
	{
	case WP_GRENADE_LAUNCHER:
		VectorScale(dir, 700, dir);                   // had to add this as fire_grenade now expects a non-normalized direction vector
		                                              // FIXME: why we do normalize the vector before this switch?
		fire_grenade(ent, ent->s.origin, dir, WP_GRENADE_LAUNCHER);
		break;
	case WP_PANZERFAUST:
		fire_rocket(ent, ent->s.origin, dir);
		VectorScale(ent->s.pos.trDelta, 2, ent->s.pos.trDelta);
		SnapVector(ent->s.pos.trDelta);             // save net bandwidth
		break;

	case WP_MAPMORTAR:
		AimAtTarget(ent);     // store in ent->s.origin2 the direction/force needed to pass through the target
		VectorScale(dir, VectorLength(ent->s.origin2), dir);
		fire_mortar(ent, ent->s.origin, dir);
		break;

	}

	G_AddEvent(ent, EV_FIRE_WEAPON, 0);
}

static void InitShooter_Finish(gentity_t *ent)
{
	ent->enemy     = G_PickTarget(ent->target);
	ent->think     = 0;
	ent->nextthink = 0;
}

void InitShooter(gentity_t *ent, int weapon)
{
	ent->use      = Use_Shooter;
	ent->s.weapon = weapon;

	G_SetMovedir(ent->s.angles, ent->movedir);

	if (!ent->random)
	{
		ent->random = 1.0;
	}

	ent->random = sin(M_PI * ent->random / 180);

	// target might be a moving object, so we can't set movedir for it
	if (ent->target)
	{
		ent->think     = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	trap_LinkEntity(ent);
}

/*
QUAKED shooter_mortar (1 0 0) (-16 -16 -16) (16 16 16) SMOKE_FX FLASH_FX
Lobs a mortar so that it will pass through the info_notnull targeted by this entity
"random" the number of degrees of deviance from the taget. (1.0 default)
if LAUNCH_FX is checked a smoke effect will play at the origin of this entity.
if FLASH_FX is checked a muzzle flash effect will play at the origin of this entity.
*/
void SP_shooter_mortar(gentity_t *ent)
{
	// FIXME/TODO: must have a self->target.  Do a check/print if this is not the case.
	InitShooter(ent, WP_MAPMORTAR);

	if (ent->spawnflags & 1)       // smoke at source
	{
	}
	if (ent->spawnflags & 2)       // muzzle flash at source
	{
	}
}

/*
QUAKED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_rocket(gentity_t *ent)
{
	InitShooter(ent, WP_PANZERFAUST);
}

/*
QUAKED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_grenade(gentity_t *ent)
{
	InitShooter(ent, WP_GRENADE_LAUNCHER);
}

/*
QUAKED corona (0 1 0) (-4 -4 -4) (4 4 4) START_OFF
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
"scale" will designate a multiplier to the default size.  (so 2.0 is 2xdefault size, 0.5 is half)
*/

/*
==============
use_corona
    so level designers can toggle them on/off
==============
*/
void use_corona(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->r.linked)
	{
		trap_UnlinkEntity(ent);
	}
	else
	{
		ent->active = 0;
		trap_LinkEntity(ent);
	}
}

/*
==============
SP_corona
==============
*/
void SP_corona(gentity_t *ent)
{
	float scale;

	if (!ent->scriptName && !ent->targetname && !ent->spawnflags)
	{
		G_FreeEntity(ent);
		return;
	}

	ent->s.eType = ET_CORONA;


	if (ent->dl_color[0] <= 0 &&                 // if it's black or has no color assigned
	    ent->dl_color[1] <= 0 &&
	    ent->dl_color[2] <= 0)
	{
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1; // set white
	}
	ent->dl_color[0] = ent->dl_color[0] * 255;
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	ent->s.dl_intensity = (int)ent->dl_color[0] | ((int)ent->dl_color[1] << 8) | ((int)ent->dl_color[2] << 16);

	G_SpawnFloat("scale", "1", &scale);
	ent->s.density = (int)(scale * 255);

	ent->use = use_corona;

	if (!(ent->spawnflags & 1))
	{
		trap_LinkEntity(ent);
	}
}

// dlights and dlightstyles
// gcc: lots of braces around scalar initializer
//char* predef_lightstyles[] = {
//  {"mmnmmommommnonmmonqnmmo"},

char *predef_lightstyles[] =
{
	"mmnmmommommnonmmonqnmmo",
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
	"ma",
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	"nmonqnmomnmomomono",
	"mmmaaaabcdefgmmmmaaaammmaamm",
	"aaaaaaaazzzzzzzz",
	"mmamammmmammamamaaamammma",
	"abcdefghijklmnopqrrqponmlkjihgfedcba",
	"mmnommomhkmmomnonmmonqnmmo",
	"kmamaamakmmmaakmamakmakmmmma",
	"kmmmakakmmaaamammamkmamakmmmma",
	"mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm",
	"mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm",
	"zzzzzzzzaaaaaaaa",
	"zzzzzzzzaaaaaaaaaaaaaaaa",
	"aaaaaaaazzzzzzzzaaaaaaaa",
	"aaaaaaaaaaaaaaaazzzzzzzz"
};

/*
==============
dlight_finish_spawning
    All the dlights should call this on the same frame, thereby
    being synched, starting their sequences all at the same time.
==============
*/
void dlight_finish_spawning(gentity_t *ent)
{
	G_FindConfigstringIndex(va("%i %s %i %i %i", ent->s.number, ent->dl_stylestring, ent->health, ent->soundLoop, ent->dl_atten), CS_DLIGHTS, MAX_DLIGHT_CONFIGSTRINGS, qtrue);
}

static int dlightstarttime = 0;

/*QUAKED dlight (0 1 0) (-12 -12 -12) (12 12 12) FORCEACTIVE STARTOFF ONETIME
"style": value is an int from 1-19 that contains a pre-defined 'flicker' string.
"stylestring": set your own 'flicker' string.  (ex. "klmnmlk"). NOTE: this should be all lowercase
Stylestring characters run at 10 cps in the game. (meaning the alphabet, at 24 characters, would take 2.4 seconds to cycle)
"offset": change the initial index in a style string.  So val of 3 in the above example would start this light at 'N'.  (used to get dlights using the same style out of sync).
"atten": offset from the alpha values of the stylestring.  stylestring of "ddeeffzz" with an atten of -1 would result in "ccddeeyy"
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
FORCEACTIVE - toggle makes sure this light stays alive in a map even if the user has r_dynamiclight set to 0.
STARTOFF    - means the dlight doesn't spawn in until ent is triggered
ONETIME     - when the dlight is triggered, it will play through it's cycle once, then shut down until triggered again
"shader" name of shader to apply
"sound" sound to loop every cycle (this actually just plays the sound at the beginning of each cycle)

styles:
1 - "mmnmmommommnonmmonqnmmo"
2 - "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba"
3 - "mmmmmaaaaammmmmaaaaaabcdefgabcdefg"
4 - "ma"
5 - "jklmnopqrstuvwxyzyxwvutsrqponmlkj"
6 - "nmonqnmomnmomomono"
7 - "mmmaaaabcdefgmmmmaaaammmaamm"
8 - "aaaaaaaazzzzzzzz"
9 - "mmamammmmammamamaaamammma"
10 - "abcdefghijklmnopqrrqponmlkjihgfedcba"
11 - "mmnommomhkmmomnonmmonqnmmo"
12 - "kmamaamakmmmaakmamakmakmmmma"
13 - "kmmmakakmmaaamammamkmamakmmmma"
14 - "mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm"
15 - "mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm"
16 - "zzzzzzzzaaaaaaaa"
17 - "zzzzzzzzaaaaaaaaaaaaaaaa"
18 - "aaaaaaaazzzzzzzzaaaaaaaa"
19 - "aaaaaaaaaaaaaaaazzzzzzzz"
*/

/*
==============
shutoff_dlight
    the dlight knew when it was triggered to unlink after going through it's cycle once
==============
*/
void shutoff_dlight(gentity_t *ent)
{
	if (!(ent->r.linked))
	{
		return;
	}

	trap_UnlinkEntity(ent);
	ent->think     = 0;
	ent->nextthink = 0;
}

/*
==============
use_dlight
==============
*/
void use_dlight(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->r.linked)
	{
		trap_UnlinkEntity(ent);
	}
	else
	{
		ent->active = 0;
		trap_LinkEntity(ent);

		if (ent->spawnflags & 4)       // ONETIME
		{
			ent->think     = shutoff_dlight;
			ent->nextthink = level.time + (strlen(ent->dl_stylestring)  * 100) - 100;
		}
	}
}

/*
==============
SP_dlight
    ent->dl_stylestring contains the lightstyle string
    ent->health tracks current index into style string
    ent->count tracks length of style string
==============
*/
void SP_dlight(gentity_t *ent)
{
	char *snd, *shader;
	int  i;
	int  offset, style, atten;

	G_SpawnInt("offset", "0", &offset);                // starting index into the stylestring
	G_SpawnInt("style", "0", &style);                      // predefined stylestring
	G_SpawnString("sound", "", &snd);                   //
	G_SpawnInt("atten", "0", &atten);                      //
	G_SpawnString("shader", "", &shader);               // name of shader to use for this dlight image

	if (G_SpawnString("sound", "0", &snd))
	{
		ent->soundLoop = G_SoundIndex(snd);
	}

	if (ent->dl_stylestring && strlen(ent->dl_stylestring))          // if they're specified in a string, use em
	{
	}
	else if (style)
	{
		style               = MAX(1, style);                      // clamp to predefined range
		style               = MIN(19, style);
		ent->dl_stylestring = predef_lightstyles[style - 1];    // these are input as 1-20
	}
	else
	{
		ent->dl_stylestring = "mmmaaa";                          // default to a strobe to call attention to this not being set
	}

	ent->count = strlen(ent->dl_stylestring);

	ent->dl_atten = atten;

	// make the initial offset a valid index into the stylestring
	offset = offset % (ent->count);

	ent->health = offset;                       // set the offset into the string

	ent->think = dlight_finish_spawning;
	if (!dlightstarttime)                          // sync up all the dlights
	{
		dlightstarttime = level.time + 100;
	}
	ent->nextthink = dlightstarttime;

	if (ent->dl_color[0] <= 0 &&                 // if it's black or has no color assigned, make it white
	    ent->dl_color[1] <= 0 &&
	    ent->dl_color[2] <= 0)
	{
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1;
	}

	ent->dl_color[0] = ent->dl_color[0] * 255;  // range 0-255 now so the client doesn't have to on every update
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	i = (int)(ent->dl_stylestring[offset]) - (int)'a';
	i = i * (1000.0f / 24.0f);

	ent->s.constantLight = (int)ent->dl_color[0] | ((int)ent->dl_color[1] << 8) | ((int)ent->dl_color[2] << 16) | (i / 4 << 24);

	ent->use = use_dlight;

	if (!(ent->spawnflags & 2))
	{
		trap_LinkEntity(ent);
	}
}

void flakPuff(vec3_t origin)
{
	gentity_t *tent;
	vec3_t    point;

	VectorCopy(origin, point);
	tent = G_TempEntity(point, EV_SMOKE);
	VectorCopy(point, tent->s.origin);
	VectorSet(tent->s.origin2, 0, 0, 32);
	tent->s.time       = 500;
	tent->s.time2      = 250;
	tent->s.density    = 0;
	tent->s.angles2[0] = 24;
	tent->s.angles2[1] = 32;
	tent->s.angles2[2] = 10;
}

/*
==============
Fire_Lead
==============
*/
void Fire_Lead_Ext(gentity_t *ent, gentity_t *activator, float spread, int damage, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, int mod)
{
	trace_t   tr;
	vec3_t    end;
	float     r;
	float     u;
	gentity_t *tent;
	gentity_t *traceEnt;
	int       seed = rand() & 255;

	r = Q_crandom(&seed) * spread;
	u = Q_crandom(&seed) * spread;

	ent->s.eFlags       |= EF_MG42_ACTIVE;
	activator->s.eFlags |= EF_MG42_ACTIVE;

	VectorMA(muzzle, 8192, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);

	// use activator for historicaltrace, not ent which may be
	// the weapon itself (e.g. for mg42s)
	G_HistoricalTrace(activator, &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

	// bullet debugging using Q3A's railtrail
	if (g_debugBullets.integer & 1)
	{
		tent = G_TempEntity(muzzle, EV_RAILTRAIL);
		VectorCopy(tr.endpos, tent->s.origin2);
		tent->s.otherEntityNum2 = activator->s.number;
	}

	if (tr.surfaceFlags & SURF_NOIMPACT)
	{
		tent                    = G_TempEntity(tr.endpos, EV_MG42BULLET_HIT_WALL);
		tent->s.otherEntityNum  = ent->s.number;
		tent->s.otherEntityNum2 = activator->s.number;
		ent->s.effect1Time      = seed;

		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards(tr.endpos, muzzle);

	// send bullet impact
	if (traceEnt->takedamage && traceEnt->client)
	{
		tent                    = G_TempEntity(tr.endpos, EV_MG42BULLET_HIT_FLESH);
		tent->s.eventParm       = traceEnt->s.number;
		tent->s.otherEntityNum  = ent->s.number;
		tent->s.otherEntityNum2 = activator->s.number;  // store the user id, so the client can position the tracer
		tent->s.effect1Time     = seed;
	}
	else
	{
		// bullet impact should reflect off surface
		vec3_t reflect;
		float  dot;

		tent = G_TempEntity(tr.endpos, EV_MG42BULLET_HIT_WALL);

		dot = DotProduct(forward, tr.plane.normal);
		VectorMA(forward, -2 * dot, tr.plane.normal, reflect);
		VectorNormalize(reflect);

		tent->s.eventParm       = DirToByte(reflect);
		tent->s.otherEntityNum  = ent->s.number;
		tent->s.otherEntityNum2 = activator->s.number;  // store the user id, so the client can position the tracer
		tent->s.effect1Time     = seed;
	}

	if (traceEnt->takedamage)
	{
		G_Damage(traceEnt, ent, activator, forward, tr.endpos, damage, 0, mod);
	}
}

// NOTE: this only effects the external view of the user, when using the mg42, the
// view position is set on the client-side to keep it firm behind the gun with
// interpolation
void clamp_playerbehindgun(gentity_t *self, gentity_t *other, vec3_t dang)
{
	vec3_t forward, right, up;
	vec3_t point;

	AngleVectors(self->s.apos.trBase, forward, right, up);
	if (self->s.eType == ET_AAGUN)
	{
		VectorMA(self->r.currentOrigin, -40, forward, point);
	}
	else
	{
		VectorMA(self->r.currentOrigin, -36, forward, point);
	}

	point[2] = other->r.currentOrigin[2];
	trap_UnlinkEntity(other);
	SnapVector(point);
	VectorCopy(point, other->client->ps.origin);

	// save results of pmove
	BG_PlayerStateToEntityState(&other->client->ps, &other->s, level.time, qfalse);

	// use the precise origin for linking
	VectorCopy(other->client->ps.origin, other->r.currentOrigin);

	// Zero out velocity
	other->client->ps.velocity[0] = other->client->ps.velocity[1] = 0.f;
	other->s.pos.trDelta[0]       = other->s.pos.trDelta[1] = 0.f;

	trap_LinkEntity(other);
}

void clamp_hweapontofirearc(gentity_t *self, vec3_t dang)
{
	float diff;
	//float yawspeed;

	// go back to start position
	VectorCopy(self->s.angles, dang);
	// yawspeed = MG42_IDLEYAWSPEED;

	if (dang[0] < 0 && dang[0] < -(self->varc))
	{
		dang[0] = -(self->varc);
	}

	if (dang[0] > 0 && dang[0] > (self->varc / 2))
	{
		dang[0] = self->varc / 2;
	}

	// sanity check the angles again to make sure we don't go passed the harc
	diff = AngleDifference(self->s.angles[YAW], dang[YAW]);
	if (fabs(diff) > self->harc)
	{
		if (diff > 0)
		{
			dang[YAW] = AngleMod(self->s.angles[YAW] - self->harc);
		}
		else
		{
			dang[YAW] = AngleMod(self->s.angles[YAW] + self->harc);
		}
	}
}

// quad 20mm specs from marauder
// Fire each barrel every 1s, so 250ms per shot total
// Hitscan
// Use some part of entitystate to store firing barrel? or just encode it with event more likely
// Shouldn't really have smoke puff in air, but may add anyway for effect
//

void aagun_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	gentity_t *owner = &g_entities[ent->r.ownerNum];

	if (owner && owner->client)
	{
		ent->r.ownerNum       = ent->s.number;
		ent->s.otherEntityNum = ent->s.number;

		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		owner->client->ps.viewlocked                   = 0;
		owner->active                                  = qfalse;
	}

	trap_LinkEntity(ent);
}

void aagun_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
}

void aagun_track(gentity_t *self, gentity_t *other)
{
	if (!self->active)
	{
		return;
	}

	if (other->active)
	{
		int i;

		// move to the position over the next frame
		VectorSubtract(other->client->ps.viewangles, self->s.apos.trBase, self->s.apos.trDelta);
		for (i = 0; i < 3; i++)
		{
			self->s.apos.trDelta[i] = AngleNormalize180(self->s.apos.trDelta[i]);
		}
		VectorScale(self->s.apos.trDelta, 1000 / 50, self->s.apos.trDelta);
		self->s.apos.trTime     = level.time;
		self->s.apos.trDuration = 50;

		SnapVector(self->s.apos.trDelta);
	}
}

void aagun_think(gentity_t *self)
{
	vec3_t    vec;
	gentity_t *owner;
	int       i;

	if (g_gamestate.integer == GS_INTERMISSION)
	{
		return;
	}

	VectorClear(vec);

	owner = &g_entities[self->r.ownerNum];

	// move to the current angles
	if (self->timestamp > level.time)
	{
		BG_EvaluateTrajectory(&self->s.apos, level.time, self->s.apos.trBase, qfalse, 0);
	}

	//self->s.frame++;

	if (owner->client)
	{
		vec3_t dang;

		VectorSubtract(self->r.currentOrigin, owner->r.currentOrigin, vec);

		if (VectorLengthSquared(vec) < Square(96) && owner->active && owner->health > 0)
		{
			self->active                                   = qtrue;
			owner->client->ps.persistant[PERS_HWEAPON_USE] = 2;
			aagun_track(self, owner);
			self->nextthink = level.time + 50;
			self->timestamp = level.time + 1000;

			for (i = 0; i < 3; i++)
			{
				dang[i] = SHORT2ANGLE(owner->client->pers.cmd.angles[i]);
			}

			// now tell the client to lock the view in the direction of the gun
			owner->client->ps.viewlocked        = 3;
			owner->client->ps.viewlocked_entNum = self->s.number;

			clamp_playerbehindgun(self, owner, dang);
			return;
		}
	}

	self->active = qfalse;

	if (owner->client)
	{
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		owner->client->ps.viewlocked                   = 0; // let them look around
		owner->active                                  = qfalse;
	}

	self->r.ownerNum       = self->s.number;
	self->s.otherEntityNum = self->s.number;

	if (self->timestamp > level.time)
	{
		// slowly rotate back to position
		clamp_hweapontofirearc(self, vec);
		// move to the position over the next frame
		VectorSubtract(vec, self->s.apos.trBase, self->s.apos.trDelta);
		for (i = 0; i < 3; i++)
		{
			self->s.apos.trDelta[i] = AngleNormalize180(self->s.apos.trDelta[i]);
		}
		VectorScale(self->s.apos.trDelta, 1000 / 50, self->s.apos.trDelta);
		self->s.apos.trTime     = level.time;
		self->s.apos.trDuration = 50;
	}
	self->nextthink = level.time + 50;

	SnapVector(self->s.apos.trDelta);
}

void aagun_stopusing(gentity_t *self)
{
	gentity_t *owner = &g_entities[self->r.ownerNum];

	if (owner && owner->client)
	{
		owner->client->ps.eFlags                      &= ~EF_AAGUN_ACTIVE;
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		owner->client->ps.viewlocked                   = 0;
		owner->active                                  = qfalse;

		self->r.ownerNum = self->s.number;
		self->active     = qfalse;
	}
}

void aagun_fire(gentity_t *other)
{
	vec3_t    forward, right, up;
	vec3_t    muzzle;
	gentity_t *self = &g_entities[other->client->ps.viewlocked_entNum];

	AngleVectors(other->client->ps.viewangles, forward, right, up);
	VectorCopy(self->s.pos.trBase, muzzle);

	BG_AdjustAAGunMuzzleForBarrel(muzzle, forward, right, up, self->s.modelindex2);
	self->s.modelindex2 = (self->s.modelindex2 + 1) % 4;

	self->s.eFlags  |= EF_AAGUN_ACTIVE;
	other->s.eFlags |= EF_AAGUN_ACTIVE;

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzle);
	Fire_Lead(self, other, AAGUN_SPREAD, AAGUN_DAMAGE, muzzle, forward, right, up);
}

void aagun_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	/*  if (!self->active) {
	        return;
	    }

	    if (other->active) {
	        vec3_t  dang;
	        int     i;

	        for (i = 0; i < 3; i++) {
	            dang[i] = SHORT2ANGLE(other->client->pers.cmd.angles[i]);
	        }

	        // now tell the client to lock the view in the direction of the gun
	        other->client->ps.viewlocked = 3;
	        other->client->ps.viewlocked_entNum = self->s.number;

	        clamp_playerbehindgun (self, other, dang);
	    }*/
}

void aagun_spawn(gentity_t *gun)
{
	gun->clipmask      = CONTENTS_SOLID;
	gun->r.contents    = CONTENTS_SOLID;
	gun->s.eType       = ET_AAGUN;
	gun->s.dmgFlags    = HINT_MG42;
	gun->s.modelindex  = G_ModelIndex("models/mapobjects/weapons/flak_a.md3");
	gun->s.modelindex2 = 0;    // which barrel should fire next

	gun->s.origin[2] += 24;
	G_SetOrigin(gun, gun->s.origin);
	G_SetAngle(gun, gun->s.angles);

	VectorSet(gun->r.mins, -48, -48, -24);
	VectorSet(gun->r.maxs, 48, 48, 56);

	gun->s.apos.trTime     = 0;
	gun->s.apos.trDuration = 0;
	gun->s.apos.trType     = TR_LINEAR_STOP;

	VectorCopy(gun->s.angles, gun->s.angles2);   // initial angles

	gun->touch = aagun_touch;
	gun->think = aagun_think;
	gun->use   = aagun_use;
	gun->die   = aagun_die;

	gun->nextthink    = level.time + FRAMETIME;
	gun->timestamp    = level.time + 1000;
	gun->s.number     = gun - g_entities;
	gun->s.origin2[0] = gun->harc;
	gun->s.origin2[1] = gun->varc;
	gun->takedamage   = qfalse;

	trap_LinkEntity(gun);
}

/*QUAKED misc_aagun (1 0 0) (-32 -32 0) (64 32 56)
harc = fixed full 360?
varc = fixed 0-45 up?
*/
void SP_aagun(gentity_t *self)
{
	self->harc = 360;
	self->varc = 45;

	aagun_spawn(self);
}

void mg42_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if (!self->active)
	{
		return;
	}

	if (other->active)
	{
		vec3_t dang;
		int    i;

		for (i = 0; i < 3; i++)
		{
			dang[i] = SHORT2ANGLE(other->client->pers.cmd.angles[i]);
		}
		// now tell the client to lock the view in the direction of the gun
		other->client->ps.viewlocked        = 3;
		other->client->ps.viewlocked_entNum = self->s.number;

		// clamp player behind the gun
		clamp_playerbehindgun(self, other, dang);
	}
}

void mg42_fire(gentity_t *other)
{
	gentity_t *self = &g_entities[other->client->ps.viewlocked_entNum];
	vec3_t    forward, right, up;
	vec3_t    muzzle;

	//AngleVectors (self->s.apos.trBase, forward, right, up);
	AngleVectors(other->client->ps.viewangles, forward, right, up);
	VectorCopy(self->s.pos.trBase, muzzle);

	// VectorMA (muzzle, 16, forward, muzzle); // unnecessary and makes it so close-range enemies get missed

	// disabled this as it was causing troubles with murderholes. Why was it there? Maybe
	// for beach to let the mg42 point down and still shoot over teh concrete?
	// FIX: make the HIGH spawnflag actually work
	if (self->spawnflags & 1)
	{
		VectorMA(muzzle, 16, up, muzzle);
	}

	self->s.eFlags  |= EF_MG42_ACTIVE;
	other->s.eFlags |= EF_MG42_ACTIVE;

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzle);

	Fire_Lead(self, other, MG42_SPREAD_MP, MG42_DAMAGE_MP, muzzle, forward, right, up); // FIXME: browning
}

void mg42_track(gentity_t *self, gentity_t *other)
{
	if (!self->active)
	{
		return;
	}

	if (other->active)
	{
		int i;

		// move to the position over the next frame
		VectorSubtract(other->client->ps.viewangles, self->s.apos.trBase, self->s.apos.trDelta);
		for (i = 0; i < 3; i++)
		{
			self->s.apos.trDelta[i] = AngleNormalize180(self->s.apos.trDelta[i]);
		}
		VectorScale(self->s.apos.trDelta, 1000 / 50, self->s.apos.trDelta);
		self->s.apos.trTime     = level.time;
		self->s.apos.trDuration = 50;

		SnapVector(self->s.apos.trDelta);
	}
}

#define USEDIST 128.f

void mg42_think(gentity_t *self)
{
	vec3_t    vec;
	gentity_t *owner;

	if (g_gamestate.integer == GS_INTERMISSION)
	{
		return;
	}

	VectorClear(vec);

	owner = &g_entities[self->r.ownerNum];

	// move to the current angles
	if (self->timestamp > level.time)
	{
		BG_EvaluateTrajectory(&self->s.apos, level.time, self->s.apos.trBase, qfalse, 0);
	}

	// heat handling
	if (owner->client)
	{
		self->mg42weapHeat = owner->client->ps.weapHeat[WP_DUMMY_MG42];
	}

	// overheated mg42 smokes
	if (self->mg42weapHeat >= MAX_MG42_HEAT)
	{
		self->s.eFlags |= EF_OVERHEATING;
		if (self->flameQuotaTime < level.time)
		{
			// try to get smoke time from client
			if (owner->client)
			{
				self->flameQuotaTime = level.time + owner->client->ps.weaponTime;
			}
			else
			{
				self->flameQuotaTime = level.time + 2000;
			}
		}
	}
	else if (self->flameQuotaTime < level.time && (self->s.eFlags & EF_OVERHEATING) == EF_OVERHEATING)
	{
		self->s.eFlags      &= ~EF_OVERHEATING;
		self->flameQuotaTime = 0;
	}

	if (owner->client)
	{
		float len;

		VectorSubtract(self->r.currentOrigin, owner->r.currentOrigin, vec);
		len = VectorLength(vec);

		if (len < USEDIST && owner->active && owner->health > 0)
		{
			// ATVI Wolfenstein Misc #433
			owner->client->ps.pm_flags &= ~PMF_DUCKED;

			self->active                                   = qtrue;
			owner->client->ps.persistant[PERS_HWEAPON_USE] = 1;
			mg42_track(self, owner);
			self->nextthink = level.time + 50;
			self->timestamp = level.time + 1000;

			//owner->client->ps.weapHeat[WP_DUMMY_MG42] = self->mg42weapHeat;

			clamp_playerbehindgun(self, owner, vec3_origin);

			return;
		}
	}

	self->active = qfalse;

	if (owner->client)
	{
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		owner->client->ps.viewlocked                   = 0; // let them look around
		owner->active                                  = qfalse;

		// need this here for players that get killed while on the MG42
		self->backupWeaponTime = owner->client->ps.weaponTime;
	}

	if (self->mg42weapHeat)
	{
		self->mg42weapHeat -= (300.f * FRAMETIME * 0.001);    //%   -= (300.f * 50 * 0.001);

		if (self->mg42weapHeat < 0)
		{
			self->mg42weapHeat = 0;
		}
	}

	if (self->backupWeaponTime)
	{
		self->backupWeaponTime -= 50;

		if (self->backupWeaponTime < 0)
		{
			self->backupWeaponTime = 0;
		}
	}

	self->r.ownerNum       = self->s.number;
	self->s.otherEntityNum = self->s.number;

	if (self->timestamp > level.time)
	{
		int i;

		// slowly rotate back to position
		clamp_hweapontofirearc(self, vec);
		// move to the position over the next frame
		VectorSubtract(vec, self->s.apos.trBase, self->s.apos.trDelta);
		for (i = 0; i < 3; i++)
		{
			self->s.apos.trDelta[i] = AngleNormalize180(self->s.apos.trDelta[i]);
		}
		VectorScale(self->s.apos.trDelta, 1000 / 50, self->s.apos.trDelta);
		self->s.apos.trTime     = level.time;
		self->s.apos.trDuration = 50;
	}
	self->nextthink = level.time + 50;

	SnapVector(self->s.apos.trDelta);
}

// this is to be called for the gun ent, not the tripod
void mg42_stopusing(gentity_t *self)
{
	gentity_t *owner = &g_entities[self->r.ownerNum];

	if (owner && owner->client)
	{
		owner->client->ps.eFlags                      &= ~EF_MG42_ACTIVE; // unset flag
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		self->r.ownerNum                               = self->s.number;
		owner->client->ps.viewlocked                   = 0; // let them look around
		owner->active                                  = qfalse;

		//owner->client->ps.weapHeat[WP_DUMMY_MG42] = 0;
		self->mg42weapHeat           = owner->client->ps.weapHeat[WP_DUMMY_MG42];
		self->backupWeaponTime       = owner->client->ps.weaponTime;
		owner->client->ps.weaponTime = owner->backupWeaponTime;

		self->active = qfalse;

		// set client
		self->r.ownerNum = self->s.number;
	}
}

void mg42_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	gentity_t *gun;
	gentity_t *owner;

	// self->chain not set if no tripod
	if (self->chain)
	{
		gun = self->chain;
	}
	else
	{
		gun = self;
	}

	gun->sound3to2 = attacker->client ? attacker->client->sess.sessionTeam : -1;

	owner = &g_entities[gun->r.ownerNum];

	if (gun && self->health <= 0)
	{
		gun->s.frame    = 2;
		gun->takedamage = qfalse;

		// health is used in repairing later
		gun->health   = 0;
		gun->s.eFlags = EF_SMOKING;         // Make it smoke on client side
		self->health  = 0;
	}

	self->takedamage = qfalse;

	if (owner && owner->client)
	{
		trace_t tr;

		// Restore original position if current position is bad
		trap_Trace(&tr, owner->r.currentOrigin, owner->r.mins, owner->r.maxs, owner->r.currentOrigin, owner->s.number, MASK_PLAYERSOLID);
		if (tr.startsolid)
		{
			VectorCopy(owner->TargetAngles, owner->client->ps.origin);
			VectorCopy(owner->TargetAngles, owner->r.currentOrigin);
			owner->r.contents = CONTENTS_CORPSE;            // this will correct itself in ClientEndFrame
		}
		owner->client->ps.eFlags &= ~EF_MG42_ACTIVE;            // unset flag

		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		owner->active                                  = qfalse;

		self->r.ownerNum             = self->s.number;
		self->s.otherEntityNum       = self->s.number;
		owner->client->ps.viewlocked = 0;   // let them look around

		gun->mg42weapHeat     = 0;
		gun->backupWeaponTime = 0;

		owner->client->ps.weaponTime = owner->backupWeaponTime;

		self->active = qfalse;
		gun->active  = qfalse;
	}


	trap_LinkEntity(self);
}

void mg42_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	gentity_t *owner = &g_entities[ent->r.ownerNum];

	if (owner && owner->client)
	{
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		ent->r.ownerNum                                = ent->s.number;
		ent->s.otherEntityNum                          = ent->s.number;
		owner->client->ps.viewlocked                   = 0; // let them look around
		owner->active                                  = qfalse;

		other->client->ps.weapHeat[WP_DUMMY_MG42] = ent->mg42weapHeat;
		ent->backupWeaponTime                     = owner->client->ps.weaponTime;
		owner->backupWeaponTime                   = owner->client->ps.weaponTime;
	}

	// G_Printf ("mg42 called use function\n");

	trap_LinkEntity(ent);
}

#ifdef FEATURE_OMNIBOT
void UpdateGoalEntity(gentity_t *oldent, gentity_t *newent);
#endif

void mg42_spawn(gentity_t *ent)
{
	// If in knifeonly mode, prevent MG42's from spawning
	if (g_knifeonly.integer != 1)
	{
		gentity_t *base, *gun;
		vec3_t    offset;

		// Need to spawn the base even when no tripod cause the gun itself isn't solid
		base            = G_Spawn();
		base->classname = "misc_mg42base";   // ease tracking

		if (!(ent->spawnflags & 2))       // no tripod
		{
			base->clipmask   = CONTENTS_SOLID;
			base->r.contents = CONTENTS_SOLID;
			base->r.svFlags  = 0;
			base->s.eType    = ET_GENERAL;
			base->takedamage = qtrue;
			base->die        = mg42_die;

			// move track and targetname over to these entities for construction system
			base->track = ent->track;
			G_SetTargetName(base, ent->targetname);

			base->s.modelindex = G_ModelIndex("models/mapobjects/weapons/mg42b.md3");
		}
		else
		{
			base->takedamage = qfalse;
		}

		VectorSet(base->r.mins, -8, -8, -8);
		VectorSet(base->r.maxs, 8, 8, 48);
		VectorCopy(ent->s.origin, offset);
		offset[2] -= 24;
		G_SetOrigin(base, offset);
		base->s.apos.trType     = TR_STATIONARY;
		base->s.apos.trTime     = 0;
		base->s.apos.trDuration = 0;
		base->s.dmgFlags        = HINT_MG42; // identify this for cursorhints
		VectorCopy(ent->s.angles, base->s.angles);
		VectorCopy(base->s.angles, base->s.apos.trBase);
		VectorCopy(base->s.angles, base->s.apos.trDelta);
		base->health    = ent->health;
		base->target    = ent->target; // added so mounting mg42 can trigger targets
		base->sound3to2 = -1;
		trap_LinkEntity(base);

		// copy state over from original entity
		G_SetEntState(base, ent->entstate);

		// Spawn the barrel
		gun               = G_Spawn();
		gun->classname    = "misc_mg42";
		gun->clipmask     = CONTENTS_SOLID;
		gun->r.contents   = CONTENTS_TRIGGER;
		gun->r.svFlags    = 0;
		gun->s.eType      = ET_MG42_BARREL;
		gun->health       = base->health;
		gun->s.modelindex = G_ModelIndex("models/multiplayer/mg42/mg42.md3");
		gun->sound3to2    = -1;

		VectorSet(offset, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] + 24);
		G_SetOrigin(gun, offset);

		VectorSet(gun->r.mins, -24, -24, -8);
		VectorSet(gun->r.maxs, 24, 24, 48);

		gun->s.apos.trTime     = 0;
		gun->s.apos.trDuration = 0;
		gun->s.apos.trType     = TR_LINEAR_STOP;

		VectorCopy(ent->s.angles, gun->s.angles);
		VectorCopy(gun->s.angles, gun->s.apos.trBase);
		VectorCopy(gun->s.angles, gun->s.apos.trDelta);

		VectorCopy(ent->s.angles, gun->s.angles2);

		gun->touch = mg42_touch;
		gun->think = mg42_think;
		gun->use   = mg42_use;
		gun->die   = mg42_die;

		gun->nextthink    = level.time + FRAMETIME;
		gun->timestamp    = level.time + 1000;
		gun->s.number     = gun - g_entities;
		gun->harc         = ent->harc;
		gun->varc         = ent->varc;
		gun->s.origin2[0] = ent->harc;
		gun->s.origin2[1] = ent->varc;
		gun->takedamage   = qtrue;
		G_SetTargetName(gun, ent->targetname);
		gun->damage     = ent->damage;
		gun->accuracy   = ent->accuracy;
		gun->target     = ent->target;
		gun->spawnflags = ent->spawnflags;

		// storing heat now
		gun->mg42weapHeat = 0;

		// move track and targetname over to these entities for construction system
		gun->track = ent->track;

		// copy state over from original entity
		G_SetEntState(gun, ent->entstate);

		if (!(ent->spawnflags & 2))       // no tripod
		{
			gun->mg42BaseEnt = base->s.number;
			base->chain      = gun;
		}
		else
		{
			gun->mg42BaseEnt = -1;
		}

		if (gun->spawnflags & 1)
		{
			gun->s.onFireStart = 1;
		}

		trap_LinkEntity(gun);

#ifdef FEATURE_OMNIBOT
		UpdateGoalEntity(ent, gun);
#endif
	}

	G_FreeEntity(ent);
}

/*QUAKED misc_mg42 (1 0 0) (-16 -16 -24) (16 16 24) HIGH NOTRIPOD
harc = horizonal fire arc Default is 57.5 (left and right)
varc = vertical fire arc Default is 45 (upwards and 22.5 down)
health = how much damage can it take default is 50
damage = determines how much the weapon will inflict if a non player uses it
"accuracy" all guns are 100% accurate an entry of 0.5 would make it 50%
*/
void SP_mg42(gentity_t *self)
{
	char *damage;
	char *accuracy;

	if (!self->harc)
	{
		self->harc = 57.5;
	}
	else
	{
		if (self->harc < 45)
		{
			self->harc = 45;
		}
	}

	if (!self->varc)
	{
		self->varc = 45.0;
	}

	if (!self->health)
	{
		self->health = MG42_MULTIPLAYER_HEALTH;
	}

	self->think     = mg42_spawn;
	self->nextthink = level.time + FRAMETIME;

	if (G_SpawnString("damage", "0", &damage))
	{
		self->damage = atoi(damage);
	}

	G_SpawnString("accuracy", "1.0", &accuracy);

	self->accuracy = atof(accuracy);

	if (!self->accuracy)
	{
		self->accuracy = 1;
	}

	if (!self->damage)
	{
		self->damage = 25;
	}
}

#define FLAK_SPREAD 100
#define FLAK_DAMAGE 36

#define GUN1_IDLE   0
#define GUN2_IDLE   4
#define GUN3_IDLE   8
#define GUN4_IDLE   12

#define GUN1_LASTFIRE   3
#define GUN2_LASTFIRE   7
#define GUN3_LASTFIRE   11
#define GUN4_LASTFIRE   15

// @note Unused
void Flak_Animate(gentity_t *ent)
{
	//G_Printf ("frame %i\n", ent->s.frame);

	if (ent->s.frame == GUN1_IDLE
	    || ent->s.frame == GUN2_IDLE
	    || ent->s.frame == GUN3_IDLE
	    || ent->s.frame == GUN4_IDLE)
	{
		return;
	}

	if (ent->count == 1)
	{
		if (ent->s.frame == GUN1_LASTFIRE)
		{
			ent->s.frame = GUN2_IDLE;
		}
		else if (ent->s.frame > GUN1_IDLE)
		{
			ent->s.frame++;
		}
	}
	else if (ent->count == 2)
	{
		if (ent->s.frame == GUN2_LASTFIRE)
		{
			ent->s.frame = GUN3_IDLE;
		}
		else if (ent->s.frame > GUN2_IDLE)
		{
			ent->s.frame++;
		}
	}
	else if (ent->count == 3)
	{
		if (ent->s.frame == GUN3_LASTFIRE)
		{
			ent->s.frame = GUN4_IDLE;
		}
		else if (ent->s.frame > GUN3_IDLE)
		{
			ent->s.frame++;
		}
	}
	else if (ent->count == 4)
	{
		if (ent->s.frame == GUN4_LASTFIRE)
		{
			ent->s.frame = GUN1_IDLE;
		}
		else if (ent->s.frame > GUN4_IDLE)
		{
			ent->s.frame++;
		}
	}
}

void flak_spawn(gentity_t *ent)
{
	gentity_t *gun;
	vec3_t    offset;

	gun               = G_Spawn();
	gun->classname    = "misc_flak";
	gun->clipmask     = CONTENTS_SOLID;
	gun->r.contents   = CONTENTS_TRIGGER;
	gun->r.svFlags    = 0;
	gun->s.eType      = ET_GENERAL;
	gun->touch        = mg42_touch;
	gun->s.modelindex = G_ModelIndex("models/mapobjects/weapons/flak_a.md3");
	VectorCopy(ent->s.origin, offset);
	G_SetOrigin(gun, offset);
	VectorSet(gun->r.mins, -24, -24, -8);
	VectorSet(gun->r.maxs, 24, 24, 48);
	gun->s.apos.trTime     = 0;
	gun->s.apos.trDuration = 0;
	VectorCopy(ent->s.angles, gun->s.angles);
	VectorCopy(gun->s.angles, gun->s.apos.trBase);
	VectorCopy(gun->s.angles, gun->s.apos.trDelta);
	gun->think         = mg42_think;
	gun->nextthink     = level.time + FRAMETIME;
	gun->s.number      = gun - g_entities;
	gun->harc          = ent->harc;
	gun->varc          = ent->varc;
	gun->s.apos.trType = TR_LINEAR_STOP;    // interpolate the angles
	gun->takedamage    = qtrue;
	G_SetTargetName(gun, ent->targetname);
	gun->mg42BaseEnt = ent->s.number;

	trap_LinkEntity(gun);
}

/*QUAKED misc_flak (1 0 0) (-32 -32 0) (32 32 100)
*/
void SP_misc_flak(gentity_t *self)
{

	if (!self->harc)
	{
		self->harc = 180;
	}
	else
	{
		if (self->harc < 90)
		{
			self->harc = 115;
		}
	}

	if (!self->varc)
	{
		self->varc = 90.0;
	}

	if (!self->health)
	{
		self->health = 100;
	}

	self->think     = flak_spawn;
	self->nextthink = level.time + FRAMETIME;
}

/*QUAKED misc_spawner (.3 .7 .8) (-8 -8 -8) (8 8 8)
use the pickup name
  when this entity gets used it will spawn an item
that matches its spawnitem field
e.i.
spawnitem
9mm
*/

void misc_spawner_think(gentity_t *ent)
{
	gitem_t   *item;
	gentity_t *drop = NULL;

	item = BG_FindItem(ent->spawnitem);

	if (!item)
	{
		G_Printf("-----> WARNING <-------\n");
		G_Printf("misc_spawner used and no item found!\n");
	}

	drop = Drop_Item(ent, item, 0, qfalse);

	if (!drop)
	{
		G_Printf("-----> WARNING <-------\n");
		G_Printf("misc_spawner used at %s failed to drop!\n", vtos(ent->r.currentOrigin));
	}
}

void misc_spawner_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	ent->think     = misc_spawner_think;
	ent->nextthink = level.time + FRAMETIME;

	//VectorCopy (other->r.currentOrigin, ent->r.currentOrigin);
	//VectorCopy (ent->r.currentOrigin, ent->s.pos.trBase);

	//VectorCopy (other->r.currentAngles, ent->r.currentAngles);

	trap_LinkEntity(ent);
}

void SP_misc_spawner(gentity_t *ent)
{
	if (!ent->spawnitem)
	{
		G_Printf("-----> WARNING <-------\n");
		G_Printf("misc_spawner at loc %s has no spawnitem!\n", vtos(ent->s.origin));
		return;
	}

	ent->use = misc_spawner_use;

	trap_LinkEntity(ent);
}

//===========================================================================
//
// Mounted Gun, attached to a moving vehicle of some-sort
//

void firetrail_die(gentity_t *ent)
{
	G_FreeEntity(ent);
}

void firetrail_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->s.eType == ET_RAMJET)
	{
		ent->s.eType = ET_GENERAL;
	}
	else
	{
		ent->s.eType = ET_RAMJET;
	}

	trap_LinkEntity(ent);
}

/*QUAKED misc_firetrails (.4 .9 .7) (-16 -16 -16) (16 16 16)
This entity must target the plane its going to be attached to

  its use function will turn the fire stream effect on and off

  an alert entity call will kill it
*/

void misc_firetrails_think(gentity_t *ent)
{
	gentity_t *left, *right, *airplane;

	airplane = G_FindByTargetname(NULL, ent->target);
	if (!airplane)
	{
		G_Error("can't find airplane with targetname \"%s\" for firetrails\n", ent->target);
	}

	// left fire trail
	left               = G_Spawn();
	left->classname    = "left_firetrail";
	left->r.contents   = 0;
	left->s.eType      = ET_RAMJET;
	left->s.modelindex = G_ModelIndex("models/ammo/rocket/rocket.md3");
	left->tagParent    = airplane;
	Q_strncpyz(left->tagName, "tag_engine1", MAX_QPATH);
	left->use                  = firetrail_use;
	left->AIScript_AlertEntity = firetrail_die;
	G_SetTargetName(left, ent->targetname);
	G_ProcessTagConnect(left, qtrue);
	trap_LinkEntity(left);

	// right fire trail
	right               = G_Spawn();
	right->classname    = "right_firetrail";
	right->r.contents   = 0;
	right->s.eType      = ET_RAMJET;
	right->s.modelindex = G_ModelIndex("models/ammo/rocket/rocket.md3");
	right->tagParent    = airplane;
	Q_strncpyz(right->tagName, "tag_engine2", MAX_QPATH);
	right->use                  = firetrail_use;
	right->AIScript_AlertEntity = firetrail_die;
	G_SetTargetName(right, ent->targetname);
	G_ProcessTagConnect(right, qtrue);
	trap_LinkEntity(right);
}

void SP_misc_firetrails(gentity_t *ent)
{
	ent->think     = misc_firetrails_think;
	ent->nextthink = level.time + 100;
}

/*QUAKED misc_constructiblemarker (1 0.85 0) ?
Used to indicate where constructibles are, is a special entity due to bot
needs. The entity has to be solid and target the trigger_objective_info
belonging to the constructible.

"model2"        optional model
"angles"        angles for the model
"skin"          optional .skin file to use for the model
"description"   name of the construction
*/
void constructiblemarker_setup(gentity_t *ent)
{
	ent->target_ent = G_FindByTargetname(NULL, ent->target);

	if (!ent->target_ent)
	{
		G_Error("'misc_constructiblemarker' has a missing target '%s'\n", ent->target);
	}

	trap_LinkEntity(ent);
}

void SP_misc_constructiblemarker(gentity_t *ent)
{
	char *s;

	ent->s.eType = ET_CONSTRUCTIBLE_MARKER;

	if (ent->model2)
	{
		ent->s.modelindex2 = G_ModelIndex(ent->model2);
	}

	if (ent->aiSkin)
	{
		ent->s.effect1Time = G_SkinIndex(ent->aiSkin);
	}

	if (G_SpawnString("description", "", &s))
	{
		char cs[MAX_INFO_STRING];

		trap_GetConfigstring(CS_CONSTRUCTION_NAMES, cs, sizeof(cs));
		Info_SetValueForKey(cs, va("%i", (int)(ent - g_entities)), s);
		trap_SetConfigstring(CS_CONSTRUCTION_NAMES, cs);
	}

	trap_SetBrushModel(ent, ent->model);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	ent->think     = constructiblemarker_setup;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED misc_landmine (.35 0.85 .35) (-16 -16 0) (16 16 16) AXIS ALLIED
Landmine entity. Make sure it is placed less than 64 units above an appropiate, landmine placement
compatible surface. It will drop down on spawn and then settle itself.

NOTE: there is a team limit of 10 landmines. With this entity you can place more landmines for that team
in the map on start, but none of the engineers will be able to place landmines until there are less
than 10 left.

"angle"     landmine orientation
*/
extern void G_LandmineThink(gentity_t *self);

void landmine_setup(gentity_t *ent)
{
	trace_t tr;
	vec3_t  end;

	VectorSet(ent->r.mins, -16, -16, 0);
	VectorCopy(ent->r.mins, ent->r.absmin);
	VectorSet(ent->r.maxs, 16, 16, 16);
	VectorCopy(ent->r.maxs, ent->r.absmax);

	ent->clipmask = MASK_MISSILESHOT;

	// drop to floor
	VectorCopy(ent->s.origin, end);
	end[2] -= 64;
	trap_Trace(&tr, ent->s.origin, NULL, NULL, end, ent->s.number, ent->clipmask);

	if (tr.startsolid || tr.fraction == 1.f || !(tr.surfaceFlags & (SURF_GRASS | SURF_SNOW | SURF_GRAVEL | SURF_LANDMINE)) ||
	    (tr.entityNum != ENTITYNUM_WORLD && (!g_entities[tr.entityNum].inuse || g_entities[tr.entityNum].s.eType != ET_CONSTRUCTIBLE)))
	{
		G_Printf("^3WARNING: 'misc_landmine' entity at %.2f %.2f %.2f doesn't have a surface to settle on\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2]);
		G_FreeEntity(ent);
		return;
	}

	G_SetOrigin(ent, tr.endpos);
	ent->s.pos.trDelta[2] = 1.f;
	ent->s.time           = ent->s.angles[1] + 90;

	// all fine
	ent->s.eType    = ET_MISSILE;
	ent->r.svFlags  = SVF_BROADCAST;
	ent->s.weapon   = WP_LANDMINE;
	ent->r.ownerNum = ENTITYNUM_WORLD;

	ent->damage       = GetWeaponTableData(WP_LANDMINE)->damage;     // overridden for dynamite
	ent->splashDamage = GetWeaponTableData(WP_LANDMINE)->damage;

	ent->accuracy            = 0;
	ent->classname           = "landmine";
	ent->damage              = 0;
	ent->splashRadius        = 225; // was: 400
	ent->methodOfDeath       = MOD_LANDMINE;
	ent->splashMethodOfDeath = MOD_LANDMINE;
	ent->s.eFlags            = (EF_BOUNCE | EF_BOUNCE_HALF);
	ent->health              = 5;
	ent->takedamage          = qtrue;
	ent->r.contents          = CONTENTS_CORPSE; // (player can walk through)

	ent->splashRadius = GetWeaponTableData(WP_LANDMINE)->damage;

	ent->health        = 0;
	ent->s.modelindex2 = 0;

	ent->nextthink = level.time + FRAMETIME;
	ent->think     = G_LandmineThink;

	ent->damage = 0;

	if (ent->s.teamNum == TEAM_AXIS)     // store team so we can generate red or blue smoke
	{
		ent->s.otherEntityNum2 = 1;
	}
	else
	{
		ent->s.otherEntityNum2 = 0;
	}

	trap_LinkEntity(ent);
}

void SP_misc_landmine(gentity_t *ent)
{
	if (ent->spawnflags & 1)
	{
		ent->s.teamNum = TEAM_AXIS;
	}
	else if (ent->spawnflags & 2)
	{
		ent->s.teamNum = TEAM_ALLIES;
	}
	else
	{
		G_Error("ERROR: misc_landmine without a team\n");
	}

	ent->nextthink = level.time + FRAMETIME * 5;
	ent->think     = landmine_setup;
}

/*QUAKED misc_commandmap_marker (0 0.85 .85) (-16 -16 0) (16 16 16) ONLY_AXIS ONLY_ALLIED ISOBJECTIVE ISHEALTHAMMOCABINET ISCOMMANDPOST
Command map marker entity. When set to state default it shows, any other state and it isn't visible.
-------- KEYS --------
(none)
-------- SPAWNFLAGS --------
(none)
*/
void SP_misc_commandmap_marker(gentity_t *ent)
{
	ent->s.eType = ET_COMMANDMAP_MARKER;
	ent->parent  = NULL;

	G_SetOrigin(ent, ent->s.origin);
}

// system to temporarily ignore certain ents during traces

void G_InitTempTraceIgnoreEnts(void)
{
	memset(level.tempTraceIgnoreEnts, 0, sizeof(level.tempTraceIgnoreEnts));
}

void G_ResetTempTraceIgnoreEnts(void)
{
	int i;

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		if (level.tempTraceIgnoreEnts[i])
		{
			g_entities[i].r.linked = qtrue;

			level.tempTraceIgnoreEnts[i] = qfalse;
		}
	}
}

void G_TempTraceIgnoreEntity(gentity_t *ent)
{
	if (!ent->r.linked)
	{
		return;
	}

	level.tempTraceIgnoreEnts[ent - g_entities] = qtrue;
	ent->r.linked                               = qfalse;
}

void G_TempTraceIgnorePlayersAndBodies(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		G_TempTraceIgnoreEntity(&g_entities[i]);
	}

	for (i = 0; i < BODY_QUEUE_SIZE; i++)
	{
		G_TempTraceIgnoreEntity(level.bodyQue[i]);
	}
}

// returns qtrue if a construction is under way on this ent, even before it hits any stages
qboolean G_ConstructionBegun(gentity_t *ent)
{
	if (G_ConstructionIsPartlyBuilt(ent))
	{
		return qtrue;
	}

	if (ent->s.angles2[0])
	{
		return qtrue;
	}

	return qfalse;
}

// returns qtrue if all stage are built
qboolean G_ConstructionIsFullyBuilt(gentity_t *ent)
{
	if (ent->s.angles2[1] != 1)
	{
		return qfalse;
	}
	return qtrue;
}

// returns qtrue if 1 stage or more is built
qboolean G_ConstructionIsPartlyBuilt(gentity_t *ent)
{
	if (G_ConstructionIsFullyBuilt(ent))
	{
		return qtrue;
	}

	if (ent->count2)
	{
		if (!ent->grenadeFired)
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}

	return qfalse;
}

// returns the constructible for this team that is attached to this toi
gentity_t *G_ConstructionForTeam(gentity_t *toi, team_t team)
{
	gentity_t *targ = toi->target_ent;

	if (!targ || targ->s.eType != ET_CONSTRUCTIBLE)
	{
		return NULL;
	}

	if (targ->spawnflags & 4)
	{
		if (team == TEAM_ALLIES)
		{
			return targ->chain;
		}
	}
	else if (targ->spawnflags & 8)
	{
		if (team == TEAM_AXIS)
		{
			return targ->chain;
		}
	}

	return targ;
}

gentity_t *G_IsConstructible(team_t team, gentity_t *toi)
{
	gentity_t *ent;

	if (!toi || toi->s.eType != ET_OID_TRIGGER)
	{
		return NULL;
	}

	if (!(ent = G_ConstructionForTeam(toi, team)))
	{
		return NULL;
	}

	if (G_ConstructionIsFullyBuilt(ent))
	{
		return NULL;
	}

	if (ent->chain && G_ConstructionBegun(ent->chain))
	{
		return NULL;
	}

	return ent;
}

/*
==============
AngleDifference
==============
*/
float AngleDifference(float ang1, float ang2)
{
	float diff = ang1 - ang2;

	if (ang1 > ang2)
	{
		if (diff > 180.0f)
		{
			diff -= 360.0f;
		}
	}
	else
	{
		if (diff < -180.0f)
		{
			diff += 360.0f;
		}
	}
	return diff;
}

/*
==================
ClientName
==================
*/
char *ClientName(int client, char *name, int size)
{
	char buf[MAX_INFO_STRING];

	if (client < 0 || client >= MAX_CLIENTS)
	{
		G_Printf("^1ClientName: client out of range\n");
		return "[client out of range]";
	}
	trap_GetConfigstring(CS_PLAYERS + client, buf, sizeof(buf));
	strncpy(name, Info_ValueForKey(buf, "n"), size - 1);
	name[size - 1] = '\0';
	Q_CleanStr(name);
	return name;
}

/*
==================
FindClientByName
==================
*/
int FindClientByName(char *name)
{
	int  i, j;
	char buf[MAX_INFO_STRING];

	for (j = 0; j < level.numConnectedClients; j++)
	{
		i = level.sortedClients[j];
		ClientName(i, buf, sizeof(buf));
		if (!Q_stricmp(buf, name))
		{
			return i;
		}
	}

	for (j = 0; j < level.numConnectedClients; j++)
	{
		i = level.sortedClients[j];
		ClientName(i, buf, sizeof(buf));
		if (Q_stristr(buf, name))
		{
			return i;
		}
	}

	return -1;
}
