/*--------------------------------------------------------------------------
 *
 * Copyright(C) 2000 James Haley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *--------------------------------------------------------------------------
 *
 * Chasecam
 *
 * Follows the displayplayer. It does exactly what it says on the
 * cover!
 *
 *--------------------------------------------------------------------------
 */

#include "c_io.h"
#include "c_runcmd.h"
#include "doomdef.h"
#include "doomstat.h"
#include "info.h"
#include "d_main.h"
#include "p_chase.h"
#include "p_map.h"
#include "p_maputl.h"
#include "p_mobj.h"
#include "r_main.h"
#include "g_game.h"

static boolean PTR_chasetraverse (intercept_t * in);

camera_t chasecam;
static long chaseviewz;
int chasecam_active = 0;
static long targetx, targety, targetz;
int chasecam_turnoff = 0;

void P_ChaseSetupFrame()
{
  viewx = chasecam.x;
  viewy = chasecam.y;
  viewz = chaseviewz;
  viewangle = chasecam.angle;
}

		// for simplicity
#define playermobj players[displayplayer].mo
#define playerangle (playermobj->angle)

void P_GetChasecamTarget()
{
  int aimfor;
  int sin, cos;
  subsector_t *ss;
  int ceilingheight, floorheight;

  // aimfor is the preferred height of the chasecam above
  // the player

  aimfor = 68 * FRACUNIT; // - players[displayplayer].updownangle * FRACUNIT;

  sin = finesine[playerangle >> ANGLETOFINESHIFT];
  cos = finecosine[playerangle >> ANGLETOFINESHIFT];

  targetx = playermobj->x - (cos >> 9) * FRACUNIT;
  targety = playermobj->y - (sin >> 9) * FRACUNIT;
  targetz = playermobj->z + aimfor;

  // the intersections test mucks up the first time, but
  // aiming at something seems to cure it
  P_AimLineAttack (players[consoleplayer].mo, 0, MELEERANGE, 0);

  // check for intersections
  P_PathTraverse (playermobj->x, playermobj->y, targetx, targety,
		  PT_ADDLINES, PTR_chasetraverse);

  ss = R_PointInSubsector (targetx, targety);

  floorheight = ss->sector->floorheight;
  ceilingheight = ss->sector->ceilingheight;
  if (aimfor > ceilingheight - floorheight - 20 * FRACUNIT)
    aimfor = ceilingheight - floorheight - 20 * FRACUNIT;


  // don't aim above the ceiling or below the floor
  if (targetz > ceilingheight - 3 * FRACUNIT)
    targetz = ceilingheight - 3 * FRACUNIT;
  if (targetz < floorheight + 3 * FRACUNIT)
    targetz = floorheight + 3 * FRACUNIT;
}

	// the 'speed' of the chasecam: the percentage closer we
	// get to the target each tic
int chasespeed = 33;

void P_ChaseTicker()
{
  int xdist, ydist, zdist;
  //subsector_t *subsec;		// haleyjd

  // find the target
  P_GetChasecamTarget ();

  // find distance to target..
  xdist = targetx - chasecam.x;
  ydist = targety - chasecam.y;
  zdist = targetz - chasecam.z;

  if (xdist + ydist + zdist > 256*FRACUNIT) {
    chasecam.x = targetx;
    chasecam.y = targety;
    chasecam.z = targetz;
  } else {
    // now move chasecam
    chasecam.x += FixedMul(xdist, chasespeed * (FRACUNIT / 100));
    chasecam.y += FixedMul(ydist, chasespeed * (FRACUNIT / 100));
    chasecam.z += FixedMul(zdist, chasespeed * (FRACUNIT / 100));
  }

  // chasecam.updownangle = players[displayplayer].updownangle;
  chasecam.angle =
/*

            // point to the player if in a demo
                demoplayback ?
        R_PointToAngle2(chasecam.x, chasecam.y, playermobj->x,
                        playermobj->y)
                :

*/
    // use player angle otherwise because its hard to control (icy)
    playerangle;

#if 0
  /* haleyjd: fix for deep water HOM bug -- 
   * although this is called from P_GetChasecamTarget, we
   * can't do it there because the target point may have
   * different deep water qualities than the interim locale
   */
  subsec = R_PointInSubsector (chasecam.x, chasecam.y);
  chasecam.heightsec = subsec->sector->heightsec;
#endif
}

/* console command */

CONSOLE_BOOLEAN(chasecam, chasecam_active, NULL, onoff, cf_nosave)
{
  if (atoi (c_argv[0]))
    P_ChaseStart ();
  else
    P_ChaseEnd ();
}

void P_ChaseStart()
{
  chasecam_active = true;
  camera = &chasecam;
  P_ResetChasecam ();
}

void P_ChaseEnd()
{
  chasecam_active = false;
  camera = NULL;
}

/* Z of the line at the point of intersection.
 * this function is really just to cast all the
 * variables to long longs
 */

static long
zi (int_64_t dist, int_64_t totaldist, int_64_t ztarget, int_64_t playerz)
{
  int_64_t thezi;

  thezi = (dist * (ztarget - playerz)) / totaldist;
  thezi += playerz;

  return (long)thezi;
}

/* go til you hit a wall
 * set the chasecam target x and ys if you hit one
 * originally based on the shooting traverse function in p_maputl.c
 */

extern fixed_t attackrange;
extern fixed_t shootz;
extern fixed_t aimslope;

static boolean
PTR_chasetraverse (intercept_t * in)
{
  fixed_t dist, frac;
  subsector_t *ss;
  long x, y;
  long z;
  sector_t *hitsector, *othersector;

  if (in->isaline)
    {
      line_t *li = in->d.line;

      dist = FixedMul (attackrange, in->frac);
      frac = in->frac - FixedDiv (12 * FRACUNIT, attackrange);

      // hit line
      // position a bit closer

      x = trace.x + FixedMul (trace.dx, frac);
      y = trace.y + FixedMul (trace.dy, frac);

      if (li->flags & ML_TWOSIDED)
	{			// crosses a two sided line

	  // sf: find which side it hit

	  ss = R_PointInSubsector (x, y);

	  hitsector = li->frontsector;
	  othersector = li->backsector;

	  if (ss->sector == li->backsector)	// other side
	    {
	      hitsector = li->backsector;
	      othersector = li->frontsector;
	    }

	  // interpolate, find z at the point of intersection

	  z = zi (dist, attackrange, targetz, playermobj->z + 28 * FRACUNIT);

	  // found which side, check for intersections
	  if ((li->flags & ML_BLOCKING) || (othersector->floorheight > z) || (othersector->ceilingheight < z) || (othersector->ceilingheight - othersector->floorheight < 40 * FRACUNIT));	// hit
	  else
	    {
	      return true;	// continue
	    }
	}

      targetx = x;		// point the new chasecam target at the intersection
      targety = y;
      targetz = zi(dist, attackrange, targetz, playermobj->z+28*FRACUNIT);

      // don't go any farther

      return false;
    }

  return true;
}

// reset chasecam eg after teleporting etc

void P_ResetChasecam()
{
  if (!chasecam_active)
    return;
  if (gamestate != GS_LEVEL)
    return;			// only in level

  // find the chasecam target
  P_GetChasecamTarget ();

  chasecam.x = targetx;
  chasecam.y = targety;
  chasecam.z = targetz;

#if 0
  // haleyjd
  chasecam.heightsec =
    R_PointInSubsector (chasecam.x, chasecam.y)->sector->heightsec;
#endif
}


//==========================================================================
//
// Walkcam
//
// walk around inside playing demos without upsetting demo sync
//
//==========================================================================

// walk around inside demos without
// upsetting demo sync

camera_t walkcamera;
int walkcam_active = 0;

void P_WalkTicker()
{
  ticcmd_t *walktic =
    &netcmds[consoleplayer][(gametic / ticdup) % BACKUPTICS];

  walkcamera.angle += walktic->angleturn << 16;

  // moving forward
  walkcamera.x += FixedMul ((ORIG_FRICTION / 4) * walktic->forwardmove,
			    finecosine[walkcamera.angle >> ANGLETOFINESHIFT]);
  walkcamera.y += FixedMul ((ORIG_FRICTION / 4) * walktic->forwardmove,
			    finesine[walkcamera.angle >> ANGLETOFINESHIFT]);

  // strafing
  walkcamera.x += FixedMul ((ORIG_FRICTION / 6) * walktic->sidemove,
			    finecosine[(walkcamera.angle -
					ANG90) >> ANGLETOFINESHIFT]);
  walkcamera.y +=
    FixedMul ((ORIG_FRICTION / 6) * walktic->sidemove,
	      finesine[(walkcamera.angle - ANG90) >> ANGLETOFINESHIFT]);

  {
    // haleyjd: FIXME -- this could be optimized by only
    // doing a traversal when the camera actually moves, rather
    // than every frame, naively
    subsector_t *subsec = R_PointInSubsector (walkcamera.x, walkcamera.y);

    // keep on the ground
    walkcamera.z = subsec->sector->floorheight + 41 * FRACUNIT;

#if 0
    // haleyjd: handle deep water appropriately
    walkcamera.heightsec = subsec->sector->heightsec;
#endif
  }

#if 0
  // looking up/down
  walkcamera.updownangle += walktic->updownangle;
  walkcamera.updownangle = walkcamera.updownangle < -50 ? -50 :
    walkcamera.updownangle > 50 ? 50 : walkcamera.updownangle;
#endif
}

CONSOLE_BOOLEAN(walkcam, walkcam_active, NULL, onoff, cf_notnet|cf_nosave)
{
  if (!c_argc)
    walkcam_active = !walkcam_active;
  else
    walkcam_active = atoi (c_argv[0]);

  if (walkcam_active)
    {
      camera = &walkcamera;
      P_ResetWalkcam ();
    }
  else
    camera = NULL;
}

void P_ResetWalkcam()
{
  walkcamera.x = playerstarts[0].x << FRACBITS;
  walkcamera.y = playerstarts[0].y << FRACBITS;
  walkcamera.angle = ANG45 * (playerstarts[0].angle/45);
#if 0
  // haleyjd
  walkcamera.heightsec =
    R_PointInSubsector (walkcamera.x, walkcamera.y)->sector->heightsec;
#endif
}

void P_Chase_AddCommands()
{
  C_AddCommand (chasecam);
  C_AddCommand (walkcam);
}
