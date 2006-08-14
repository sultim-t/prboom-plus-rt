/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze, Andrey Budko
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Uncapped framerate stuff
 *
 *---------------------------------------------------------------------
 */

#include "doomstat.h"
#include "SDL.h"
#include "r_defs.h"
#include "r_state.h"
#include "p_spec.h"
#include "r_demo.h"
#include "r_fps.h"
#include "e6y.h"

int movement_smooth;

typedef enum
{
  INTERP_SectorFloor,
  INTERP_SectorCeiling,
  INTERP_Vertex,
  INTERP_WallPanning,
  INTERP_FloorPanning,
  INTERP_CeilingPanning
} interpolation_type_e;

typedef struct
{
  interpolation_type_e type;
  void *address;
} interpolation_t;

int numinterpolations = 0;
boolean NewThinkerPresent = false;

unsigned int TicStart;
unsigned int TicNext;
unsigned int TicStep;
float TicksInMSec;

fixed_t	r_TicFrac;
int otic;

fixed_t PrevX;
fixed_t PrevY;
fixed_t PrevZ;

fixed_t oviewx;
fixed_t oviewy;
fixed_t oviewz;
angle_t oviewangle;
angle_t oviewpitch;

extern int_64_t I_GetTime_Scale;
extern int realtic_clock_rate;
void D_Display(void);

void e6y_I_Init(void)
{
  TicksInMSec = realtic_clock_rate * TICRATE / 100000.0f;
}

void Extra_D_Display(void)
{
#ifdef GL_DOOM
  if (movement_smooth)
#else
    if (movement_smooth && gamestate==wipegamestate)
#endif
  {
    isExtraDDisplay = true;
    D_Display();
    isExtraDDisplay = false;
  }
}

fixed_t I_GetTimeFrac (void)
{
  unsigned long now;
  fixed_t frac;

  now = SDL_GetTicks();

  if (TicStep == 0)
    return FRACUNIT;
  else
  {
    frac = (fixed_t)((now - TicStart + DDisplayTime) * FRACUNIT / TicStep);
    if (frac < 0)
      frac = 0;
    if (frac > FRACUNIT)
      frac = FRACUNIT;
    return frac;
  }
}

void I_GetTime_SaveMS(void)
{
  if (!movement_smooth)
    return;

  TicStart = SDL_GetTicks();
  TicNext = (unsigned int) ((TicStart * TicksInMSec + 1.0f) / TicksInMSec);
  TicStep = TicNext - TicStart;
}

typedef fixed_t fixed2_t[2];
fixed2_t *oldipos;
fixed2_t *bakipos;
interpolation_t *curipos;

boolean NoInterpolateView;
boolean r_NoInterpolate;
static boolean didInterp;

void R_InterpolateView (player_t *player, fixed_t frac)
{
  if (movement_smooth)
  {
    if (NoInterpolateView)
    {
      NoInterpolateView = false;
      oviewx = player->mo->x;
      oviewy = player->mo->y;
      oviewz = player->viewz;

      oviewangle = player->mo->angle + viewangleoffset;
      oviewpitch = player->mo->pitch;

      if(walkcamera.type)
      {
        walkcamera.PrevX = walkcamera.x;
        walkcamera.PrevY = walkcamera.y;
        walkcamera.PrevZ = walkcamera.z;
        walkcamera.PrevAngle = walkcamera.angle;
        walkcamera.PrevPitch = walkcamera.pitch;
      }
    }

    if (walkcamera.type != 2)
    {
      viewx = oviewx + FixedMul (frac, player->mo->x - oviewx);
      viewy = oviewy + FixedMul (frac, player->mo->y - oviewy);
      viewz = oviewz + FixedMul (frac, player->viewz - oviewz);
    }
    else
    {
      viewx = walkcamera.PrevX + FixedMul (frac, walkcamera.x - walkcamera.PrevX);
      viewy = walkcamera.PrevY + FixedMul (frac, walkcamera.y - walkcamera.PrevY);
      viewz = walkcamera.PrevZ + FixedMul (frac, walkcamera.z - walkcamera.PrevZ);
    }

    if (walkcamera.type)
    {
      viewangle = walkcamera.PrevAngle + FixedMul (frac, walkcamera.angle - walkcamera.PrevAngle);
      viewpitch = walkcamera.PrevPitch + FixedMul (frac, walkcamera.pitch - walkcamera.PrevPitch);
    }
    else
    {
      viewangle = oviewangle + FixedMul (frac, R_SmoothPlaying_Get(player->mo->angle) + viewangleoffset - oviewangle);
      viewpitch = oviewpitch + FixedMul (frac, player->mo->pitch /*+ viewangleoffset*/ - oviewpitch);
    }
  }
  else
  {
    if (walkcamera.type != 2)
    {
      viewx = player->mo->x;
      viewy = player->mo->y;
      viewz = player->viewz;
    }
    else
    {
      viewx = walkcamera.x;
      viewy = walkcamera.y;
      viewz = walkcamera.z;
    }
    if (walkcamera.type)
    {
      viewangle = walkcamera.angle;
      viewpitch = walkcamera.pitch;
    }
    else
    {
      viewangle = R_SmoothPlaying_Get(player->mo->angle);
      //viewangle = player->mo->angle + viewangleoffset;
      viewpitch = player->mo->pitch;// + viewangleoffset;
    }
  }
}

void R_ResetViewInterpolation ()
{
  NoInterpolateView = true;
}

void CopyInterpToOld (int i)
{
  switch (curipos[i].type)
  {
  case INTERP_SectorFloor:
    oldipos[i][0] = ((sector_t*)curipos[i].address)->floorheight;
    break;
  case INTERP_SectorCeiling:
    oldipos[i][0] = ((sector_t*)curipos[i].address)->ceilingheight;
    break;
  case INTERP_Vertex:
    oldipos[i][0] = ((vertex_t*)curipos[i].address)->x;
    oldipos[i][1] = ((vertex_t*)curipos[i].address)->y;
    break;
  case INTERP_WallPanning:
    oldipos[i][0] = ((side_t*)curipos[i].address)->rowoffset;
    oldipos[i][1] = ((side_t*)curipos[i].address)->textureoffset;
    break;
  case INTERP_FloorPanning:
    oldipos[i][0] = ((sector_t*)curipos[i].address)->floor_xoffs;
    oldipos[i][1] = ((sector_t*)curipos[i].address)->floor_yoffs;
    break;
  case INTERP_CeilingPanning:
    oldipos[i][0] = ((sector_t*)curipos[i].address)->ceiling_xoffs;
    oldipos[i][1] = ((sector_t*)curipos[i].address)->ceiling_yoffs;
    break;
  }
}

void CopyBakToInterp (int i)
{
  switch (curipos[i].type)
  {
  case INTERP_SectorFloor:
    ((sector_t*)curipos[i].address)->floorheight = bakipos[i][0];
    break;
  case INTERP_SectorCeiling:
    ((sector_t*)curipos[i].address)->ceilingheight = bakipos[i][0];
    break;
  case INTERP_Vertex:
    ((vertex_t*)curipos[i].address)->x = bakipos[i][0];
    ((vertex_t*)curipos[i].address)->y = bakipos[i][1];
    break;
  case INTERP_WallPanning:
    ((side_t*)curipos[i].address)->rowoffset = bakipos[i][0];
    ((side_t*)curipos[i].address)->textureoffset = bakipos[i][1];
    break;
  case INTERP_FloorPanning:
    ((sector_t*)curipos[i].address)->floor_xoffs = bakipos[i][0];
    ((sector_t*)curipos[i].address)->floor_yoffs = bakipos[i][1];
    break;
  case INTERP_CeilingPanning:
    ((sector_t*)curipos[i].address)->ceiling_xoffs = bakipos[i][0];
    ((sector_t*)curipos[i].address)->ceiling_yoffs = bakipos[i][1];
    break;
  }
}

void DoAnInterpolation (int i, fixed_t smoothratio)
{
  fixed_t pos;
  fixed_t *adr1 = NULL;
  fixed_t *adr2 = NULL;

  switch (curipos[i].type)
  {
  case INTERP_SectorFloor:
    adr1 = &((sector_t*)curipos[i].address)->floorheight;
    break;
  case INTERP_SectorCeiling:
    adr1 = &((sector_t*)curipos[i].address)->ceilingheight;
    break;
  case INTERP_Vertex:
    adr1 = &((vertex_t*)curipos[i].address)->x;
////    adr2 = &((vertex_t*)curipos[i].Address)->y;
    break;
  case INTERP_WallPanning:
    adr1 = &((side_t*)curipos[i].address)->rowoffset;
    adr2 = &((side_t*)curipos[i].address)->textureoffset;
    break;
  case INTERP_FloorPanning:
    adr1 = &((sector_t*)curipos[i].address)->floor_xoffs;
    adr2 = &((sector_t*)curipos[i].address)->floor_yoffs;
    break;
  case INTERP_CeilingPanning:
    adr1 = &((sector_t*)curipos[i].address)->ceiling_xoffs;
    adr2 = &((sector_t*)curipos[i].address)->ceiling_yoffs;
    break;

 default:
    return;
  }

  if (adr1)
  {
    pos = bakipos[i][0] = *adr1;
    *adr1 = oldipos[i][0] + FixedMul (pos - oldipos[i][0], smoothratio);
  }

  if (adr2)
  {
    pos = bakipos[i][1] = *adr2;
    *adr2 = oldipos[i][1] + FixedMul (pos - oldipos[i][1], smoothratio);
  }
}

void updateinterpolations()
{
  int i;
  if (!movement_smooth)
    return;
  for (i = numinterpolations-1; i >= 0; --i)
    CopyInterpToOld (i);
}

int interpolations_max = 0;

void setinterpolation(interpolation_type_e type, void *posptr)
{
  int i;
  if (!movement_smooth)
    return;
  
  if (numinterpolations >= interpolations_max) {
    interpolations_max = interpolations_max ? interpolations_max * 2 : 256;
    
    oldipos = (fixed2_t*)realloc(oldipos, sizeof(*oldipos) * interpolations_max);
    bakipos = (fixed2_t*)realloc(bakipos, sizeof(*bakipos) * interpolations_max);
    curipos = (interpolation_t*)realloc(curipos, sizeof(*curipos) * interpolations_max);
  }
  
  for(i = numinterpolations-1; i >= 0; i--)
    if (curipos[i].address == posptr && curipos[i].type == type)
      return;

  curipos[numinterpolations].address = posptr;
  curipos[numinterpolations].type = type;
  CopyInterpToOld (numinterpolations);
  numinterpolations++;
} 

void stopinterpolation(interpolation_type_e type, void *posptr)
{
  int i;

  if (!movement_smooth)
    return;

  for(i=numinterpolations-1; i>= 0; --i)
  {
    if (curipos[i].address == posptr && curipos[i].type == type)
    {
      numinterpolations--;
      oldipos[i][0] = oldipos[numinterpolations][0];
      oldipos[i][1] = oldipos[numinterpolations][1];
      bakipos[i][0] = bakipos[numinterpolations][0];
      bakipos[i][1] = bakipos[numinterpolations][1];
      curipos[i] = curipos[numinterpolations];
      break;
    }
  }
}

void stopallinterpolation(void)
{
  int i;
  
  if (!movement_smooth)
    return;

  for(i=numinterpolations-1; i>= 0; --i)
  {
    numinterpolations--;
    oldipos[i][0] = oldipos[numinterpolations][0];
    oldipos[i][1] = oldipos[numinterpolations][1];
    bakipos[i][0] = bakipos[numinterpolations][0];
    bakipos[i][1] = bakipos[numinterpolations][1];
    curipos[i] = curipos[numinterpolations];
  }
}

void dointerpolations(fixed_t smoothratio)
{
  int i;
  if (!movement_smooth)
    return;

  if (smoothratio == FRACUNIT)
  {
    didInterp = false;
    return;
  }

  didInterp = true;

  for (i = numinterpolations-1; i >= 0; --i)
  {
    DoAnInterpolation (i, smoothratio);
  }
}

void restoreinterpolations()
{
  int i;
  
  if (!movement_smooth)
    return;

  if (didInterp)
  {
    didInterp = false;
    for (i = numinterpolations-1; i >= 0; --i)
    {
      CopyBakToInterp (i);
    }
  }
}

void P_ActivateAllInterpolations()
{
  int i;
  sector_t     *sec;

  if (!movement_smooth)
    return;

  for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
  {
    if (sec->floordata)
      setinterpolation (INTERP_SectorFloor, sec);
    if (sec->ceilingdata)
      setinterpolation (INTERP_SectorCeiling, sec);
  }
}

void InterpolationGetData(thinker_t *th,
  interpolation_type_e *type1, interpolation_type_e *type2,
  void **posptr1, void **posptr2)
{
  *posptr1 = NULL;
  *posptr2 = NULL;

  if (th->function == T_MoveFloor)
  {
    *type1 = INTERP_SectorFloor;
    *posptr1 = ((floormove_t *)th)->sector;
  }
  else
  if (th->function == T_PlatRaise)
  {
    *type1 = INTERP_SectorFloor;
    *posptr1 = ((plat_t *)th)->sector;
  }
  else
  if (th->function == T_MoveCeiling)
  {
    *type1 = INTERP_SectorCeiling;
    *posptr1 = ((ceiling_t *)th)->sector;
  }
  else
  if (th->function == T_VerticalDoor)
  {
    *type1 = INTERP_SectorCeiling;
    *posptr1 = ((vldoor_t *)th)->sector;
  }
  else
  if (th->function == T_MoveElevator)
  {
    *type1 = INTERP_SectorFloor;
    *posptr1 = ((elevator_t *)th)->sector;
    *type2 = INTERP_SectorCeiling;
    *posptr2 = ((elevator_t *)th)->sector;
  }
  else
  if (th->function == T_Scroll)
  {
    switch (((scroll_t *)th)->type)
    {
      case sc_side:
        *type1 = INTERP_WallPanning;
        *posptr1 = sides + ((scroll_t *)th)->affectee;
        break;
      case sc_floor:
        *type1 = INTERP_FloorPanning;
        *posptr1 = sectors + ((scroll_t *)th)->affectee;
        break;
      case sc_ceiling:
        *type1 = INTERP_CeilingPanning;
        *posptr1 = sectors + ((scroll_t *)th)->affectee;
        break;
      default: ;
    }
  }
}

void SetInterpolationIfNew(thinker_t *th)
{
  void *posptr1;
  void *posptr2;
  interpolation_type_e type1, type2;

  if (!movement_smooth)
    return;

  InterpolationGetData(th, &type1, &type2, &posptr1, &posptr2);

  if(posptr1)
  {
    setinterpolation (type1, posptr1);
    
    if(posptr2)
      setinterpolation (type2, posptr2);
  }
}

void StopInterpolationIfNeeded(thinker_t *th)
{
  void *posptr1;
  void *posptr2;
  interpolation_type_e type1, type2;

  if (!movement_smooth)
    return;

  InterpolationGetData(th, &type1, &type2, &posptr1, &posptr2);

  if(posptr1)
  {
    stopinterpolation (type1, posptr1);
    if(posptr2)
      stopinterpolation (type2, posptr2);
  }
}

