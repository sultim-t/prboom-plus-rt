/*
** thingdef.cpp
**
** Code pointers for Actor definitions
**
**---------------------------------------------------------------------------
** Copyright 2002-2005 Christoph Oelckers
** Copyright 2004-2005 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "tarray.h"
#include "thingdef.h"

extern "C"
{
#include "p_mobj.h"
#include "s_sound.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_enemy.h"
#include "m_random.h"
#include "math.h"
}


TArray<int> StateParameters;
TArray<FDropItem *> DropItemList;

//==========================================================================
//
//
//
//==========================================================================

int CheckIndex(mobj_t *mo, unsigned int paramsize)
{
	if (mo->state->misc1 == STATEPARAM_ID) return -1;

	size_t index = (size_t)mo->state->misc2;
	if (index > StateParameters.Size() - paramsize) return -1;
	return (int)index;
}


//==========================================================================
//
// Simple flag changers
//
//==========================================================================
void A_SetSolid(mobj_t * self)
{
	self->flags |= MF_SOLID;
}

void A_UnsetSolid(mobj_t * self)
{
	self->flags &= ~MF_SOLID;
}

void A_SetFloat(mobj_t * self)
{
	self->flags |= MF_FLOAT;
}

void A_UnsetFloat(mobj_t * self)
{
	self->flags &= ~(MF_FLOAT|MF_INFLOAT);
}

//==========================================================================
//
// This has been changed to use the parameter array instead of using the
// misc field directly so they can be used in weapon states
//
//==========================================================================

void A_PlaySoundParms(mobj_t * self)
{
	int index=CheckIndex(self, 1);
	if (index<0) return;

	int soundid = StateParameters[index];
	S_StartSound(self, soundid);
}

//==========================================================================
//
// Generic seeker missile function
//
//==========================================================================
//----------------------------------------------------------------------------
//
// FUNC P_FaceMobj
//
// Returns 1 if 'source' needs to turn clockwise, or 0 if 'source' needs
// to turn counter clockwise.  'delta' is set to the amount 'source'
// needs to turn.
//
//----------------------------------------------------------------------------

int P_FaceMobj(mobj_t *source, mobj_t *target, angle_t *delta)
{
	angle_t diff;
	angle_t angle1;
	angle_t angle2;

	angle1 = source->angle;
	angle2 = R_PointToAngle2(source->x, source->y, target->x, target->y);
	if (angle2 > angle1)
	{
		diff = angle2 - angle1;
		if (diff > ANG180)
		{
			*delta = ANGLE_MAX - diff;
			return 0;
		}
		else
		{
			*delta = diff;
			return 1;
		}
	}
	else
	{
		diff = angle1 - angle2;
		if (diff > ANG180)
		{
			*delta = ANGLE_MAX - diff;
			return 1;
		}
		else
		{
			*delta = diff;
			return 0;
		}
	}
}

//----------------------------------------------------------------------------
//
// FUNC P_SeekerMissile
//
// The missile's tracer field must be the target.  Returns true if
// target was tracked, false if not.
//
//----------------------------------------------------------------------------

dboolean P_SeekerMissile(mobj_t *actor, angle_t thresh, angle_t turnMax)
{
	int dir;
	int dist;
	angle_t delta;
	angle_t angle;
	mobj_t *target;

	target = actor->tracer;
	if (target == NULL)
	{
		return false;
	}
	if (!(target->flags & MF_SHOOTABLE))
	{ // Target died
		actor->tracer = NULL;
		return false;
	}
	dir = P_FaceMobj(actor, target, &delta);
	if (delta > thresh)
	{
		delta >>= 1;
		if (delta > turnMax)
		{
			delta = turnMax;
		}
	}
	if (dir)
	{ // Turn clockwise
		actor->angle += delta;
	}
	else
	{ // Turn counter clockwise
		actor->angle -= delta;
	}
	angle = actor->angle >> ANGLETOFINESHIFT;
	actor->momx = FixedMul(actor->info->speed, finecosine[angle]);
	actor->momy = FixedMul(actor->info->speed, finesine[angle]);
	if (actor->z + actor->height < target->z ||
		target->z + target->height < actor->z)
	{ // Need to seek vertically
		dist = P_AproxDistance(target->x - actor->x, target->y - actor->y);
		dist = dist / actor->info->speed;
		if (dist < 1)
		{
			dist = 1;
		}
		actor->momz = ((target->z + target->height / 2) - (actor->z + actor->height / 2)) / dist;
	}
	return true;
}

void A_SeekerMissile(mobj_t * self)
{
	int index=CheckIndex(self, 2);
	if (index < 0) return;
	P_SeekerMissile(self, BETWEEN(StateParameters[index], 0, ANG90), BETWEEN(StateParameters[index + 1], 0, ANG90));
}

//==========================================================================
//
// Hitscan attack with a customizable amount of bullets (specified in damage)
//
//==========================================================================
void A_BulletAttack (mobj_t *self)
{
	int i;
	int bangle;
	int slope;
		
	if (!self->target) return;

	A_FaceTarget (self);
	bangle = self->angle;

	slope = P_AimLineAttack (self, bangle, MISSILERANGE, 0);

	S_StartSound (self, self->info->attacksound);
	for (i=0 ; i<self->info->damage ; i++)
    {
		int r1 = P_Random(pr_cabullet) & 255;
		int r2 = P_Random(pr_cabullet) & 255;
		int angle = bangle + ((r1 - r2) << 20);
		int damage = ((P_Random(pr_cabullet) % 5) + 1) * 3;
		P_LineAttack(self, angle, MISSILERANGE, slope, damage);
    }
}

//==========================================================================
//
// State jump function
//
//==========================================================================
void A_Jump(mobj_t * self)
{
	int index=CheckIndex(self, 2);

	if (index>=0 && P_Random(pr_cajump) < BETWEEN(StateParameters[index], 0, 255))
		P_SetMobjState(self, (statenum_t)StateParameters[index+1]);
}

//==========================================================================
//
// State jump function
//
//==========================================================================
void A_JumpIfHealthLower(mobj_t * self)
{
	int index=CheckIndex(self, 2);

	if (index>=0 && self->health < StateParameters[index])
		P_SetMobjState(self, (statenum_t)StateParameters[index + 1]);
}

//==========================================================================
//
// State jump function
//
//==========================================================================
void A_JumpIfCloser(mobj_t * self)
{
	int index=CheckIndex(self, 2);
	mobj_t * target = self->target;

	// No target - no jump
	if (target==NULL) return;

	fixed_t dist = StateParameters[index];
	if (index>0 && P_AproxDistance(self->x-self->target->x, self->y-self->target->y) < dist)
		P_SetMobjState(self, (statenum_t)StateParameters[index + 1]);
}

//==========================================================================
//
// Parameterized version of A_Explode
//
//==========================================================================

void A_ExplodeParms (mobj_t *self)
{
	int damage = 128;

	int index=CheckIndex(self, 1);
	if (index>=0) 
	{
		if (StateParameters[index] != 0)
		{
			damage = StateParameters[index];
		}
	}
	P_RadiusAttack (self, self->target, damage);
}


//==========================================================================
//
// The ultimate code pointer: Fully customizable missiles!
//
//==========================================================================
void A_CustomMissile(mobj_t * self)
{
	int index=CheckIndex(self, 5);
	if (index<0) return;

	mobjtype_t MissileName=(mobjtype_t)StateParameters[index];
	fixed_t SpawnHeight=StateParameters[index+1];
	fixed_t Spawnofs_XY=StateParameters[index+2];
	angle_t Angle=StateParameters[index+3];
	int aimmode = StateParameters[index + 4];

	mobj_t * targ;
	mobj_t * missile;

	if (self->target != NULL || aimmode==2)
	{
		if (MissileName > 0 /*&& MissileName < NumTypes*/ ) 
		{
			angle_t ang = (self->angle - ANG90) >> ANGLETOFINESHIFT;
			fixed_t x = FixedMul(Spawnofs_XY, finecosine[ang]);
			fixed_t y = FixedMul(Spawnofs_XY,finesine[ang]);
			fixed_t z = SpawnHeight-32*FRACUNIT;

			switch (aimmode)
			{
			case 0:
				// This aims directly at the target
				self->x+=x;
				self->y+=y;
				self->z+=z;
				missile = P_SpawnMissile(self, self->target, MissileName);
				self->x-=x;
				self->y-=y;
				self->z-=z;
				break;

			case 1:
				// This aims parallel to the main missile
				missile = P_SpawnMissile(self, self->target, MissileName);
				missile->x += x;
				missile->y += y;
				missile->z += SpawnHeight;
				break;

			default:
				return;
			}

			if (missile)
			{
				// Use the actual momentum instead of the missile's Speed property
				// so that this can handle missiles with a high vertical velocity 
				// component properly.
				double velocity[] = { (double)missile->momx, (double)missile->momy };

				fixed_t missilespeed = (fixed_t)sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);

				missile->angle += Angle;
				ang = missile->angle >> ANGLETOFINESHIFT;
				missile->momx = FixedMul (missilespeed, finecosine[ang]);
				missile->momy = FixedMul (missilespeed, finesine[ang]);
	
				// handle projectile shooting projectiles - track the
				// links back to a real owner
                if (self->flags&MF_MISSILE)
                {
                	mobj_t * owner=self ;//->target;
                	while (owner->flags&MF_MISSILE && owner->target) owner=owner->target;
                	targ=owner;
                	missile->target=owner;
					// automatic handling of seeker missiles
					if (self->flags & missile->flags & MF_SEEKERMISSILE)
					{
						missile->tracer=self->tracer;
					}
                }
				else if (missile->flags & MF_SEEKERMISSILE)
				{
					// automatic handling of seeker missiles
					missile->tracer=self->target;
				}
			}
		}
	}
}

//==========================================================================
//
// An even more customizable hitscan attack
//
//==========================================================================
fixed_t PitchToSlope(fixed_t pitch)
{
	// Taken from ZDoom's A_FireGoldWandPL2.
	return finetangent[FINEANGLES / 4 - ((signed)pitch >> ANGLETOFINESHIFT)];
}

fixed_t SlopeToPitch(fixed_t slope)
{
	// extrapolated from how ZDoom calculates pitch where PrBoom calculates slope.
	return -(int)R_PointToAngle2(0, 0, FRACUNIT, slope);
}

void A_CustomBulletAttack (mobj_t *self)
{
	int index=CheckIndex(self, 4);
	if (index<0) return;

	angle_t Spread_XY=StateParameters[index];
	angle_t Spread_Z=StateParameters[index+1];
	int NumBullets=StateParameters[index+2];
	int DamagePerBullet=StateParameters[index+3];


	int i;
	int bangle;
	int bslope;

	if (self->target)
	{
		A_FaceTarget(self);
		bangle = self->angle;

		bslope = P_AimLineAttack(self, bangle, MISSILERANGE, 0);

		S_StartSound(self, self->info->attacksound);
		for (i = 0; i < NumBullets; i++)
		{
			int r1 = P_Random(pr_cabullet) & 255;
			int r2 = P_Random(pr_cabullet) & 255;
			int angle = bangle + (r1 - r2) * (Spread_XY / 255);

			r1 = P_Random(pr_cabullet) & 255;
			r2 = P_Random(pr_cabullet) & 255;
			int slope = PitchToSlope(SlopeToPitch(bslope) + (r1 - r2) * (Spread_Z / 255));

			int damage = ((P_Random(pr_cabullet) % 3) + 1) * DamagePerBullet;
			P_LineAttack(self, angle, MISSILERANGE, slope, damage);
		}
	}
}

//===========================================================================
//
// A_ChangeFlag
//
//===========================================================================
void A_ChangeFlag(mobj_t * self)
{
	int index = CheckIndex(self, 2);
	uint_64_t flagbit = uint_64_t(1) << StateParameters[index];
	int set = StateParameters[index + 1];

	if (flagbit & (MF_NOBLOCKMAP | MF_NOSECTOR)) P_UnsetThingPosition(self);

	if (set) self->flags |= flagbit;
	else self->flags &= flagbit;

	if (flagbit & (MF_NOBLOCKMAP | MF_NOSECTOR)) P_SetThingPosition(self);
}


//----------------------------------------------------------------------------
//
// PROC A_Fall
//
// Drop item spawning is done here, like in ZDoom, to avoid different
// semantics as for the builtin zombie actors.
//
//----------------------------------------------------------------------------

extern "C" void A_Fall(mobj_t *actor)
{
	actor->flags &= ~MF_SOLID;

	size_t index = actor->info->dropindex;

	// If the actor has attached data for items to drop, drop those.
	if (index >= 0 && index < DropItemList.Size())
	{
		FDropItem *di = DropItemList[index];

		while (di != NULL)
		{
			mobj_t *mo = P_SpawnMobj(actor->x, actor->y, ONFLOORZ, di->mobjtype);
			mo->flags |= MF_DROPPED;    // special versions of items
			di = di->Next;
		}
	}
}

