/******************************* Chasecam code *****************************/
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Chasecam
//
// Follows the displayplayer. It does exactly what it says on the
// cover!
//
//--------------------------------------------------------------------------

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
#include "a_small.h"

boolean PTR_chasetraverse(intercept_t *in);

camera_t chasecam;
long chaseviewz;
int chasecam_active = 0;
long targetx, targety, targetz;
int chasecam_turnoff = 0;

void P_ChaseSetupFrame(void)
{
   viewx = chasecam.x;
   viewy = chasecam.y;
   viewz = chaseviewz;
   viewangle = chasecam.angle;
}

                // for simplicity
#define playermobj players[displayplayer].mo
#define playerangle (playermobj->angle)

int chasecam_height;
int chasecam_dist;

void P_GetChasecamTarget(void)
{
   int aimfor;
   int sin, cos;
   subsector_t *ss;
   int ceilingheight, floorheight;

   // aimfor is the preferred height of the chasecam above
   // the player
   // haleyjd: 1 unit for each degree of pitch works surprisingly well
   aimfor = players[displayplayer].viewheight + chasecam_height*FRACUNIT 
               + FixedDiv(players[displayplayer].pitch, ANGLE_1);
      
   sin = finesine[playerangle>>ANGLETOFINESHIFT];
   cos = finecosine[playerangle>>ANGLETOFINESHIFT];
   
   targetx = playermobj->x - chasecam_dist * cos;
   targety = playermobj->y - chasecam_dist * sin;
   targetz = playermobj->z + aimfor;

   // the intersections test mucks up the first time, but
   // aiming at something seems to cure it
   P_AimLineAttack(players[consoleplayer].mo, 0, MELEERANGE, 0);

   // check for intersections
   P_PathTraverse(playermobj->x, playermobj->y, targetx, targety,
                  PT_ADDLINES, PTR_chasetraverse);

   ss = R_PointInSubsector(targetx, targety);
   
   floorheight = ss->sector->floorheight;
   ceilingheight = ss->sector->ceilingheight;

   // don't aim above the ceiling or below the floor
   if(targetz > ceilingheight - 10*FRACUNIT)
      targetz = ceilingheight - 10*FRACUNIT;
   if(targetz < floorheight + 10*FRACUNIT)
      targetz = floorheight + 10*FRACUNIT;
}

        // the 'speed' of the chasecam: the percentage closer we
        // get to the target each tic
int chasecam_speed;

void P_ChaseTicker(void)
{
   int xdist, ydist, zdist;
   subsector_t *subsec; // haleyjd

   // find the target
   P_GetChasecamTarget();
   
   // find distance to target..
   xdist = targetx-chasecam.x;
   ydist = targety-chasecam.y;
   zdist = targetz-chasecam.z;
   
   // haleyjd: patched these lines with cph's fix
   //          for overflow occuring in the multiplication
   // now move chasecam
   chasecam.x += FixedMul(xdist, chasecam_speed*(FRACUNIT/100));
   chasecam.y += FixedMul(ydist, chasecam_speed*(FRACUNIT/100));
   chasecam.z += FixedMul(zdist, chasecam_speed*(FRACUNIT/100));
   
   chasecam.pitch = players[displayplayer].pitch;
   chasecam.angle = playerangle;

   // haleyjd: fix for deep water HOM bug -- 
   // although this is called from P_GetChasecamTarget, we
   // can't do it there because the target point may have
   // different deep water qualities than the interim locale
   subsec = R_PointInSubsector(chasecam.x, chasecam.y);
   chasecam.heightsec = subsec->sector->heightsec;
}

// console commands

VARIABLE_BOOLEAN(chasecam_active, NULL, onoff);

CONSOLE_VARIABLE(chasecam, chasecam_active, 0)
{
   if(atoi(c_argv[0]))
      P_ChaseStart();
   else
      P_ChaseEnd();
}

VARIABLE_INT(chasecam_height, NULL, -31, 100, NULL);
CONSOLE_VARIABLE(chasecam_height, chasecam_height, 0) {}

VARIABLE_INT(chasecam_dist, NULL, 10, 1024, NULL);
CONSOLE_VARIABLE(chasecam_dist, chasecam_dist, 0) {}

VARIABLE_INT(chasecam_speed, NULL, 1, 100, NULL);
CONSOLE_VARIABLE(chasecam_speed, chasecam_speed, 0) {}

void P_ChaseStart(void)
{
   // if(chasecam_active) return;     // already active
   DEBUGMSG("activate chasecam\n");
   chasecam_active = true;
   camera = &chasecam;
   P_ResetChasecam();
}

void P_ChaseEnd(void)
{
   // if(!chasecam_active) return;
   DEBUGMSG("deactivate chasecam\n");
   chasecam_active = false;
   camera = NULL;
}

// Z of the line at the point of intersection.
// this function is really just to cast all the
// variables to 64-bit integers

long zi(Long64 dist, Long64 totaldist, Long64 ztarget, Long64 playerz)
{
   Long64 thezi;
   
   thezi = (dist * (ztarget - playerz)) / totaldist;
   thezi += playerz;
   
   return (long)thezi;
}

// go til you hit a wall
// set the chasecam target x and ys if you hit one
// originally based on the shooting traverse function in p_maputl.c

extern fixed_t attackrange;
extern fixed_t shootz;
extern fixed_t aimslope;

boolean PTR_chasetraverse(intercept_t *in)
{
   fixed_t dist, frac;
   subsector_t *ss;
   long x, y;
   long z;
   sector_t *hitsector, *othersector;

   if(in->isaline)
   {
      line_t *li = in->d.line;
      
      dist = FixedMul(attackrange, in->frac);
      frac = in->frac - FixedDiv(12*FRACUNIT,attackrange);
      
      // hit line
      // position a bit closer
      
      x = trace.x + FixedMul(trace.dx, frac);
      y = trace.y + FixedMul(trace.dy, frac);

      if(li->flags & ML_TWOSIDED)
      {  // crosses a two sided line

         // sf: find which side it hit
         
         ss = R_PointInSubsector (x, y);
         
         hitsector = li->frontsector; othersector=li->backsector;
         
         if(ss->sector==li->backsector)      // other side
         {
            hitsector = li->backsector;
            othersector = li->frontsector;
         }

         // interpolate, find z at the point of intersection
         
         z = zi(dist, attackrange, targetz, playermobj->z+28*FRACUNIT);
         
         // found which side, check for intersections
         if( (li->flags & ML_BLOCKING) ||
            (othersector->floorheight>z) || (othersector->ceilingheight<z)
            || (othersector->ceilingheight-othersector->floorheight
                < 40*FRACUNIT));          // hit
         else
         {
            return true;    // continue
         }
      }

      targetx = x;        // point the new chasecam target at the intersection
      targety = y;
      targetz = zi(dist, attackrange, targetz, playermobj->z+28*FRACUNIT);
      
      // don't go any farther
      
      return false;
   }
   
   return true;
}

// reset chasecam eg after teleporting etc

void P_ResetChasecam(void)
{
   if(!chasecam_active) return;
   if(gamestate != GS_LEVEL) return;       // only in level

   DEBUGMSG("reset chasecam\n");
   
   // find the chasecam target
   P_GetChasecamTarget();
   
   chasecam.x = targetx;
   chasecam.y = targety;
   chasecam.z = targetz;
   
   // haleyjd
   chasecam.heightsec = 
      R_PointInSubsector(chasecam.x, chasecam.y)->sector->heightsec;
}


/*******************
 WALK AROUND CAMERA
 *******************/

// walk around inside demos without
// upsetting demo sync

camera_t walkcamera;
int walkcam_active = 0;

void P_WalkTicker(void)
{
   ticcmd_t *walktic = &netcmds[consoleplayer][(gametic/ticdup)%BACKUPTICS];

   walkcamera.angle += walktic->angleturn << 16;
   
   // moving forward
   walkcamera.x += FixedMul((ORIG_FRICTION/4) * walktic->forwardmove,
      finecosine[walkcamera.angle >> ANGLETOFINESHIFT]);
   walkcamera.y += FixedMul((ORIG_FRICTION/4) * walktic->forwardmove,
      finesine[walkcamera.angle >> ANGLETOFINESHIFT]);

   // strafing
   walkcamera.x += FixedMul((ORIG_FRICTION/6) * walktic->sidemove,
      finecosine[(walkcamera.angle-ANG90) >> ANGLETOFINESHIFT]);
   walkcamera.y += FixedMul((ORIG_FRICTION/6) * walktic->sidemove,
      finesine[(walkcamera.angle-ANG90) >> ANGLETOFINESHIFT]);

   {
      // haleyjd: FIXME -- this could be optimized by only
      // doing a traversal when the camera actually moves, rather
      // than every frame, naively
      subsector_t *subsec = 
         R_PointInSubsector(walkcamera.x, walkcamera.y);
      
      // keep on the ground
      walkcamera.z = subsec->sector->floorheight + 41*FRACUNIT;
      
      // haleyjd: handle deep water appropriately
      walkcamera.heightsec = subsec->sector->heightsec;
   }

   // looking up/down 
   // haleyjd: this is the same as new code in p_user.c, but for walkcam
   {
      int look = walktic->look;

      if(look)
      {
         // test for special centerview value
         if(look == -32768)
            walkcamera.pitch = 0;
         else
         {
            walkcamera.pitch -= look << 16;
            if(walkcamera.pitch < -ANGLE_1*32)
               walkcamera.pitch = -ANGLE_1*32;
            else if(walkcamera.pitch > ANGLE_1*32)
               walkcamera.pitch = ANGLE_1*32;
         }
      }
   } // end local block
}

VARIABLE_BOOLEAN(walkcam_active, NULL,              onoff);
CONSOLE_VARIABLE(walkcam, walkcam_active, cf_notnet)
{
   if(!c_argc)
      walkcam_active = !walkcam_active;
   else
      walkcam_active = atoi(c_argv[0]);

   if(walkcam_active)
   {
      camera = &walkcamera;
      P_ResetWalkcam();
   }
   else
      camera = NULL;
}

void P_ResetWalkcam(void)
{
   walkcamera.x = playerstarts[0].x << FRACBITS;
   walkcamera.y = playerstarts[0].y << FRACBITS;
   walkcamera.angle = R_WadToAngle(playerstarts[0].angle);
   // haleyjd
   walkcamera.heightsec =
      R_PointInSubsector(walkcamera.x, walkcamera.y)->sector->heightsec;
}

void P_Chase_AddCommands(void)
{
   C_AddCommand(chasecam);
   C_AddCommand(chasecam_height);
   C_AddCommand(chasecam_dist);
   C_AddCommand(chasecam_speed);
   C_AddCommand(walkcam);
}

static cell AMX_NATIVE_CALL sm_chasecam(AMX *amx, cell *params)
{
   int onoff = (int)params[1];

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   if(onoff)
      P_ChaseStart();
   else
      P_ChaseEnd();

   return 0;
}

static cell AMX_NATIVE_CALL sm_ischaseon(AMX *amx, cell *params)
{
   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   return chasecam_active;
}

AMX_NATIVE_INFO chase_Natives[] =
{
   { "_ToggleChasecam", sm_chasecam  },
   { "_IsChasecamOn",   sm_ischaseon },
   { NULL, NULL }
};

// EOF
