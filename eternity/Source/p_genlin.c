// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
// DESCRIPTION:
//  Generalized linedef type handlers
//  Floors, Ceilings, Doors, Locked Doors, Lifts, Stairs, Crushers
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_genlin.c,v 1.18 1998/05/23 10:23:23 jim Exp $";

#include "doomstat.h"
#include "r_main.h"
#include "p_info.h"
#include "p_spec.h"
#include "p_tick.h"
#include "m_random.h"
#include "s_sound.h"
#include "sounds.h"
#include "a_small.h"
#include "e_exdata.h"

//////////////////////////////////////////////////////////
//
// Generalized Linedef Type handlers
//
//////////////////////////////////////////////////////////

int EV_DoParamFloor(line_t *line, int tag, floordata_t *fd)
{
   int         secnum;
   int         rtn = 0;
   boolean     manual = false;
   sector_t    *sec;
   floormove_t *floor;

   // check if a manual trigger, if so do just the sector on the backside
   // haleyjd 05/07/04: only line actions can be manual
   if(fd->trigger_type == PushOnce || fd->trigger_type == PushMany)
   {
      if(!line || !(sec = line->backsector))
         return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_floor;
   }

   secnum = -1;
   // if not manual do all sectors tagged the same as the line
   while((secnum = P_FindSectorFromTag(tag, secnum)) >= 0)
   {
      sec = &sectors[secnum];
      
manual_floor:                
      // Do not start another function if floor already moving
      if(P_SectorActive(floor_special,sec))
      {
         if(manual)
            return rtn;
         continue;
      }

      // new floor thinker
      rtn = 1;
      floor = Z_Malloc(sizeof(*floor), PU_LEVSPEC, 0);
      P_AddThinker(&floor->thinker);
      sec->floordata = floor;
      
      floor->thinker.function = T_MoveFloor;
      floor->crush = fd->crush;
      floor->direction = fd->direction ? plat_up : plat_down;
      floor->sector = sec;
      floor->texture = sec->floorpic;
      floor->newspecial = sec->special;
      //jff 3/14/98 transfer old special field too
      floor->oldspecial = sec->oldspecial;
      floor->type = genFloor;

      // set the speed of motion
      switch(fd->speed_type)
      {
      case SpeedSlow:
         floor->speed = FLOORSPEED;
         break;
      case SpeedNormal:
         floor->speed = FLOORSPEED*2;
         break;
      case SpeedFast:
         floor->speed = FLOORSPEED*4;
         break;
      case SpeedTurbo:
         floor->speed = FLOORSPEED*8;
         break;
      case SpeedParam: // haleyjd 05/07/04: parameterized extension
         floor->speed = fd->speed_value;
         break;
      default:
         break;
      }

      // set the destination height
      switch(fd->target_type)
      {
      case FtoHnF:
         floor->floordestheight = P_FindHighestFloorSurrounding(sec);
         break;
      case FtoLnF:
         floor->floordestheight = P_FindLowestFloorSurrounding(sec);
         break;
      case FtoNnF:
         floor->floordestheight = fd->direction ?
            P_FindNextHighestFloor(sec,sec->floorheight) :
            P_FindNextLowestFloor(sec,sec->floorheight);
         break;
      case FtoLnC:
         floor->floordestheight = P_FindLowestCeilingSurrounding(sec);
         break;
      case FtoC:
         floor->floordestheight = sec->ceilingheight;
         break;
      case FbyST:
         floor->floordestheight = 
            (floor->sector->floorheight>>FRACBITS) + floor->direction * 
            (P_FindShortestTextureAround(secnum)>>FRACBITS);
         if(floor->floordestheight>32000)  //jff 3/13/98 prevent overflow
            floor->floordestheight=32000;    // wraparound in floor height
         if(floor->floordestheight<-32000)
            floor->floordestheight=-32000;
         floor->floordestheight<<=FRACBITS;
         break;
      case Fby24:
         floor->floordestheight = floor->sector->floorheight +
            floor->direction * 24*FRACUNIT;
         break;
      case Fby32:
         floor->floordestheight = floor->sector->floorheight +
            floor->direction * 32*FRACUNIT;
         break;
      
         // haleyjd 05/07/04: parameterized extensions
         //         05/20/05: added FtoAbs, FInst
      case FbyParam: 
         floor->floordestheight = floor->sector->floorheight +
            floor->direction * fd->height_value;
         break;
      case FtoAbs:
         floor->floordestheight = fd->height_value;
         // adjust direction appropriately (instant movement not possible)
         if(floor->floordestheight > floor->sector->floorheight)
            floor->direction = plat_up;
         else
            floor->direction = plat_down;
         break;
      case FInst:
         floor->floordestheight = floor->sector->floorheight +
            floor->direction * fd->height_value;
         // adjust direction appropriately (always instant)
         if(floor->floordestheight > floor->sector->floorheight)
            floor->direction = plat_down;
         else
            floor->direction = plat_up;
         break;
      default:
         break;
      }

      // set texture/type change properties
      if(fd->change_type)   // if a texture change is indicated
      {
         if(fd->change_model) // if a numeric model change
         {
            sector_t *sec;

            //jff 5/23/98 find model with ceiling at target height
            //if target is a ceiling type
            sec = (fd->target_type == FtoLnC || fd->target_type == FtoC)?
               P_FindModelCeilingSector(floor->floordestheight,secnum) :
               P_FindModelFloorSector(floor->floordestheight,secnum);
            
            if(sec)
            {
               floor->texture = sec->floorpic;
               switch(fd->change_type)
               {
               case FChgZero:  // zero type
                  floor->newspecial = 0;
                  //jff 3/14/98 change old field too
                  floor->oldspecial = 0;
                  floor->type = genFloorChg0;
                  break;
               case FChgTyp:   // copy type
                  floor->newspecial = sec->special;
                  //jff 3/14/98 change old field too
                  floor->oldspecial = sec->oldspecial;
                  floor->type = genFloorChgT;
                  break;
               case FChgTxt:   // leave type be
                  floor->type = genFloorChg;
                  break;
               default:
                  break;
               }
            }
         }
         else     // else if a trigger model change
         {
            if(line) // haleyjd 05/07/04: only line actions can use this
            {
               floor->texture = line->frontsector->floorpic;
               switch(fd->change_type)
               {
               case FChgZero:    // zero type
                  floor->newspecial = 0;
                  //jff 3/14/98 change old field too
                  floor->oldspecial = 0;
                  floor->type = genFloorChg0;
                  break;
               case FChgTyp:     // copy type
                  floor->newspecial = line->frontsector->special;
                  //jff 3/14/98 change old field too
                  floor->oldspecial = line->frontsector->oldspecial;
                  floor->type = genFloorChgT;
                  break;
               case FChgTxt:     // leave type be
                  floor->type = genFloorChg;
               default:
                  break;
               }
            } // end if(line)
         }
      }
      if(manual)
         return rtn;
   }
   return rtn;
}

//
// EV_DoGenFloor()
//
// Handle generalized floor types
//
// Passed the line activating the generalized floor function
// Returns true if a thinker is created
//
// jff 02/04/98 Added this routine (and file) to handle generalized
// floor movers using bit fields in the line special type.
//
// haleyjd 05/07/04: rewritten to use EV_DoParamFloor
//
int EV_DoGenFloor(line_t *line)
{
   floordata_t fd;
   unsigned value = (unsigned)line->special - GenFloorBase;

   // parse the bit fields in the line's special type
   
   fd.crush        = ((value & FloorCrush) >> FloorCrushShift) ? 10 : -1;
   fd.change_type  = (value & FloorChange) >> FloorChangeShift;
   fd.target_type  = (value & FloorTarget) >> FloorTargetShift;
   fd.direction    = (value & FloorDirection) >> FloorDirectionShift;
   fd.change_model = (value & FloorModel) >> FloorModelShift;
   fd.speed_type   = (value & FloorSpeed) >> FloorSpeedShift;
   fd.trigger_type = (value & TriggerType) >> TriggerTypeShift;

   return EV_DoParamFloor(line, line->tag, &fd);
}


//
// EV_DoGenCeiling()
//
// Handle generalized ceiling types
//
// Passed the linedef activating the ceiling function
// Returns true if a thinker created
//
// jff 02/04/98 Added this routine (and file) to handle generalized
// floor movers using bit fields in the line special type.
//
int EV_DoGenCeiling(line_t *line)
{
   int       secnum;
   int       rtn;
   boolean   manual;
   fixed_t   targheight;
   sector_t  *sec;
   ceiling_t *ceiling;
   unsigned  value = (unsigned)line->special - GenCeilingBase;

   // parse the bit fields in the line's special type
   
   int Crsh = (value & CeilingCrush) >> CeilingCrushShift;
   int ChgT = (value & CeilingChange) >> CeilingChangeShift;
   int Targ = (value & CeilingTarget) >> CeilingTargetShift;
   int Dirn = (value & CeilingDirection) >> CeilingDirectionShift;
   int ChgM = (value & CeilingModel) >> CeilingModelShift;
   int Sped = (value & CeilingSpeed) >> CeilingSpeedShift;
   int Trig = (value & TriggerType) >> TriggerTypeShift;

   rtn = 0;

   // check if a manual trigger, if so do just the sector on the backside
   manual = false;
   if(Trig==PushOnce || Trig==PushMany)
   {
      if(!(sec = line->backsector))
         return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_ceiling;
   }

   secnum = -1;
   // if not manual do all sectors tagged the same as the line
   while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
   {
      sec = &sectors[secnum];

manual_ceiling:                
      // Do not start another function if ceiling already moving
      if(P_SectorActive(ceiling_special,sec)) //jff 2/22/98
      {
         if(!manual)
            continue;
         else
            return rtn;
      }

      // new ceiling thinker
      rtn = 1;
      ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, 0);
      P_AddThinker (&ceiling->thinker);
      sec->ceilingdata = ceiling; //jff 2/22/98
      ceiling->thinker.function = T_MoveCeiling;
      ceiling->crush = (Crsh ? 10 : -1);
      ceiling->direction = Dirn? plat_up : plat_down;
      ceiling->sector = sec;
      ceiling->texture = sec->ceilingpic;
      ceiling->newspecial = sec->special;
      //jff 3/14/98 change old field too
      ceiling->oldspecial = sec->oldspecial;
      ceiling->tag = sec->tag;
      ceiling->type = genCeiling;

      // set speed of motion
      switch (Sped)
      {
      case SpeedSlow:
         ceiling->speed = CEILSPEED;
         break;
      case SpeedNormal:
         ceiling->speed = CEILSPEED*2;
         break;
      case SpeedFast:
         ceiling->speed = CEILSPEED*4;
         break;
      case SpeedTurbo:
         ceiling->speed = CEILSPEED*8;
         break;
      default:
         break;
      }

      // set destination target height
      targheight = sec->ceilingheight;
      switch(Targ)
      {
      case CtoHnC:
         targheight = P_FindHighestCeilingSurrounding(sec);
         break;
      case CtoLnC:
         targheight = P_FindLowestCeilingSurrounding(sec);
         break;
      case CtoNnC:
         targheight = Dirn?
            P_FindNextHighestCeiling(sec,sec->ceilingheight) :
            P_FindNextLowestCeiling(sec,sec->ceilingheight);
         break;
      case CtoHnF:
         targheight = P_FindHighestFloorSurrounding(sec);
         break;
      case CtoF:
         targheight = sec->floorheight;
         break;
      case CbyST:
         targheight = (ceiling->sector->ceilingheight>>FRACBITS) +
            ceiling->direction * (P_FindShortestUpperAround(secnum)>>FRACBITS);
         if(targheight>32000)  //jff 3/13/98 prevent overflow
            targheight=32000;    // wraparound in ceiling height
         if(targheight<-32000)
            targheight=-32000;
         targheight<<=FRACBITS;
         break;
      case Cby24:
         targheight = ceiling->sector->ceilingheight +
            ceiling->direction * 24*FRACUNIT;
         break;
      case Cby32:
         targheight = ceiling->sector->ceilingheight +
            ceiling->direction * 32*FRACUNIT;
         break;
      default:
         break;
      }
    
      //Dirn? ceiling->topheight : ceiling->bottomheight = targheight;
      // SoM 3/12/2002: VC threw errors at the old statement... weird
      if(Dirn)
      {
         ceiling->topheight = targheight;
      }
      else
      {
         ceiling->bottomheight = targheight;
      }

      // set texture/type change properties
      if(ChgT)     // if a texture change is indicated
      {
         if(ChgM)   // if a numeric model change
         {
            sector_t *sec;

            //jff 5/23/98 find model with floor at target height if target
            //is a floor type
            sec = (Targ==CtoHnF || Targ==CtoF)?         
               P_FindModelFloorSector(targheight,secnum) :
               P_FindModelCeilingSector(targheight,secnum);
            if(sec)
            {
               ceiling->texture = sec->ceilingpic;
               switch(ChgT)
               {
               case CChgZero:  // type is zeroed
                  ceiling->newspecial = 0;
                  //jff 3/14/98 change old field too
                  ceiling->oldspecial = 0;
                  ceiling->type = genCeilingChg0;
                  break;
               case CChgTyp:   // type is copied
                  ceiling->newspecial = sec->special;
                  //jff 3/14/98 change old field too
                  ceiling->oldspecial = sec->oldspecial;
                  ceiling->type = genCeilingChgT;
                  break;
               case CChgTxt:   // type is left alone
                  ceiling->type = genCeilingChg;
                  break;
               default:
                  break;
               }
            }
         }
         else        // else if a trigger model change
         {
            ceiling->texture = line->frontsector->ceilingpic;
            switch(ChgT)
            {
            case CChgZero:    // type is zeroed
               ceiling->newspecial = 0;
               //jff 3/14/98 change old field too
               ceiling->oldspecial = 0;
               ceiling->type = genCeilingChg0;
               break;
            case CChgTyp:     // type is copied
               ceiling->newspecial = line->frontsector->special;
               //jff 3/14/98 change old field too
               ceiling->oldspecial = line->frontsector->oldspecial;
               ceiling->type = genCeilingChgT;
               break;
            case CChgTxt:     // type is left alone
               ceiling->type = genCeilingChg;
               break;
            default:
               break;
            }
         }
      }
      P_AddActiveCeiling(ceiling);  // add this ceiling to the active list
      if(manual)
         return rtn;
   }
   return rtn;
}

//
// EV_DoGenLift()
//
// Handle generalized lift types
//
// Passed the linedef activating the lift
// Returns true if a thinker is created
//
int EV_DoGenLift(line_t*       line)
{
   plat_t   *plat;
   int      secnum;
   int      rtn;
   boolean  manual;
   sector_t *sec;
   unsigned value = (unsigned)line->special - GenLiftBase;

   // parse the bit fields in the line's special type
   
   int Targ = (value & LiftTarget) >> LiftTargetShift;
   int Dely = (value & LiftDelay) >> LiftDelayShift;
   int Sped = (value & LiftSpeed) >> LiftSpeedShift;
   int Trig = (value & TriggerType) >> TriggerTypeShift;

   secnum = -1;
   rtn = 0;
   
   // Activate all <type> plats that are in_stasis
   
   if(Targ==LnF2HnF)
      P_ActivateInStasis(line->tag);
        
   // check if a manual trigger, if so do just the sector on the backside
   manual = false;
   if(Trig==PushOnce || Trig==PushMany)
   {
      if (!(sec = line->backsector))
         return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_lift;
   }

   // if not manual do all sectors tagged the same as the line
   while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
   {
      sec = &sectors[secnum];
      
manual_lift:
      // Do not start another function if floor already moving
      if(P_SectorActive(floor_special,sec))
      {
         if(!manual)
            continue;
         else
            return rtn;
      }
      
      // Setup the plat thinker
      rtn = 1;
      plat = Z_Malloc(sizeof(*plat), PU_LEVSPEC, 0);
      P_AddThinker(&plat->thinker);
      
      plat->sector = sec;
      plat->sector->floordata = plat;
      plat->thinker.function = T_PlatRaise;
      plat->crush = -1;
      plat->tag = line->tag;
      
      plat->type = genLift;
      plat->high = sec->floorheight;
      plat->status = down;

      // setup the target destination height
      switch(Targ)
      {
      case F2LnF:
         plat->low = P_FindLowestFloorSurrounding(sec);
         if(plat->low > sec->floorheight)
            plat->low = sec->floorheight;
         break;
      case F2NnF:
         plat->low = P_FindNextLowestFloor(sec,sec->floorheight);
         break;
      case F2LnC:
         plat->low = P_FindLowestCeilingSurrounding(sec);
         if(plat->low > sec->floorheight)
            plat->low = sec->floorheight;
         break;
      case LnF2HnF:
         plat->type = genPerpetual;
         plat->low = P_FindLowestFloorSurrounding(sec);
         if(plat->low > sec->floorheight)
            plat->low = sec->floorheight;
         plat->high = P_FindHighestFloorSurrounding(sec);
         if(plat->high < sec->floorheight)
            plat->high = sec->floorheight;
         plat->status = P_Random(pr_genlift)&1;
         break;
      default:
         break;
      }

      // setup the speed of motion
      switch(Sped)
      {
      case SpeedSlow:
         plat->speed = PLATSPEED * 2;
         break;
      case SpeedNormal:
         plat->speed = PLATSPEED * 4;
         break;
      case SpeedFast:
         plat->speed = PLATSPEED * 8;
         break;
      case SpeedTurbo:
         plat->speed = PLATSPEED * 16;
         break;
      default:
         break;
      }

      // setup the delay time before the floor returns
      switch(Dely)
      {
      case 0:
         plat->wait = 1*35;
         break;
      case 1:
         plat->wait = PLATWAIT*35;
         break;
      case 2:
         plat->wait = 5*35;
         break;
      case 3:
         plat->wait = 10*35;
         break;
      }

      if(!silentmove(sec))        //sf: silentmove
         S_StartSoundName((mobj_t *)&sec->soundorg,LevelInfo.sound_pstart);
      P_AddActivePlat(plat); // add this plat to the list of active plats
      
      if(manual)
         return rtn;
   }
   return rtn;
}

//
// EV_DoGenStairs()
//
// Handle generalized stair building
//
// Passed the linedef activating the stairs
// Returns true if a thinker is created
//
int EV_DoGenStairs(line_t *line)
{
   int      secnum;
   int      osecnum; //jff 3/4/98 preserve loop index
   int      height;
   int      i;
   int      newsecnum;
   int      texture;
   int      ok;
   int      rtn;
   boolean  manual;
    
   sector_t *sec;
   sector_t *tsec;
   
   floormove_t *floor;
   
   fixed_t  stairsize;
   fixed_t  speed;

   unsigned value = (unsigned)line->special - GenStairsBase;

   // parse the bit fields in the line's special type
   
   int Igno = (value & StairIgnore) >> StairIgnoreShift;
   int Dirn = (value & StairDirection) >> StairDirectionShift;
   int Step = (value & StairStep) >> StairStepShift;
   int Sped = (value & StairSpeed) >> StairSpeedShift;
   int Trig = (value & TriggerType) >> TriggerTypeShift;

   rtn = 0;
   
   // check if a manual trigger, if so do just the sector on the backside
   manual = false;
   if(Trig==PushOnce || Trig==PushMany)
   {
      if(!(sec = line->backsector))
         return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_stair;
   }

   secnum = -1;
   // if not manual do all sectors tagged the same as the line
   while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
   {
      sec = &sectors[secnum];
      
manual_stair:          
      //Do not start another function if floor already moving
      //jff 2/26/98 add special lockout condition to wait for entire
      //staircase to build before retriggering
      if(P_SectorActive(floor_special,sec) || sec->stairlock)
      {
         if(!manual)
            continue;
         else
            return rtn;
      }
      
      // new floor thinker
      rtn = 1;
      floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
      P_AddThinker (&floor->thinker);
      sec->floordata = floor;
      floor->thinker.function = T_MoveFloor;
      floor->direction = Dirn? plat_up : plat_down;
      floor->sector = sec;

      // setup speed of stair building
      switch(Sped)
      {
      default:
      case SpeedSlow:
         floor->speed = FLOORSPEED/4;
         break;
      case SpeedNormal:
         floor->speed = FLOORSPEED/2;
         break;
      case SpeedFast:
	 floor->speed = FLOORSPEED*2;
         break;
      case SpeedTurbo:
         floor->speed = FLOORSPEED*4;
         break;
      }

      // setup stepsize for stairs
      switch(Step)
      {
      default:
      case 0:
         stairsize = 4*FRACUNIT;
         break;
      case 1:
         stairsize = 8*FRACUNIT;
         break;
      case 2:
         stairsize = 16*FRACUNIT;
         break;
      case 3:
         stairsize = 24*FRACUNIT;
         break;
      }

      speed = floor->speed;
      height = sec->floorheight + floor->direction * stairsize;
      floor->floordestheight = height;
      texture = sec->floorpic;
      floor->crush = -1;
      floor->type = genBuildStair; // jff 3/31/98 do not leave uninited
      
      sec->stairlock = -2;         // jff 2/26/98 set up lock on current sector
      sec->nextsec = -1;
      sec->prevsec = -1;
      
      osecnum = secnum;            //jff 3/4/98 preserve loop index  
      // Find next sector to raise
      // 1.     Find 2-sided line with same sector side[0]
      // 2.     Other side is the next sector to raise
      do
      {
         ok = 0;
         for (i = 0;i < sec->linecount;i++)
         {
            if(!((sec->lines[i])->backsector) )
               continue;
            
            tsec = (sec->lines[i])->frontsector;
            newsecnum = tsec-sectors;
            
            if(secnum != newsecnum)
               continue;
            
            tsec = (sec->lines[i])->backsector;
            newsecnum = tsec - sectors;
            
            if(!Igno && tsec->floorpic != texture)
               continue;

            // jff 6/19/98 prevent double stepsize
            // killough 10/98: corrected use of demo compatibility flag
            if(demo_version < 202)
               height += floor->direction * stairsize;

            //jff 2/26/98 special lockout condition for retriggering
            if(P_SectorActive(floor_special,tsec) || tsec->stairlock)
               continue;

            // jff 6/19/98 increase height AFTER continue        
            // killough 10/98: corrected use of demo compatibility flag
            if(demo_version >= 202)
               height += floor->direction * stairsize;

            // jff 2/26/98
            // link the stair chain in both directions
            // lock the stair sector until building complete
            sec->nextsec = newsecnum; // link step to next
            tsec->prevsec = secnum;   // link next back
            tsec->nextsec = -1;       // set next forward link as end
            tsec->stairlock = -2;     // lock the step
            
            sec = tsec;
            secnum = newsecnum;
            floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
            
            P_AddThinker (&floor->thinker);
            
            sec->floordata = floor;
            floor->thinker.function = T_MoveFloor;
            floor->direction = Dirn? plat_up : plat_down;
            floor->sector = sec;
            floor->speed = speed;
            floor->floordestheight = height;
            floor->crush = -1;
            floor->type = genBuildStair; // jff 3/31/98 do not leave uninited
            
            ok = 1;
            break;
         }
      } while(ok);
      if(manual)
         return rtn;
      secnum = osecnum; //jff 3/4/98 restore old loop index
   }
   // retriggerable generalized stairs build up or down alternately
   if(rtn)
      line->special ^= StairDirection; // alternate dir on succ activations
   return rtn;
}

//
// EV_DoGenCrusher()
//
// Handle generalized crusher types
//
// Passed the linedef activating the crusher
// Returns true if a thinker created
//
int EV_DoGenCrusher(line_t *line)
{
   int       secnum;
   int       rtn;
   boolean   manual;
   sector_t  *sec;
   ceiling_t *ceiling;
   unsigned  value = (unsigned)line->special - GenCrusherBase;
   
   // parse the bit fields in the line's special type
   
   int Slnt = (value & CrusherSilent) >> CrusherSilentShift;
   int Sped = (value & CrusherSpeed) >> CrusherSpeedShift;
   int Trig = (value & TriggerType) >> TriggerTypeShift;

   //jff 2/22/98  Reactivate in-stasis ceilings...for certain types.
   //jff 4/5/98 return if activated
   rtn = P_ActivateInStasisCeiling(line);

   // check if a manual trigger, if so do just the sector on the backside
   manual = false;
   if(Trig==PushOnce || Trig==PushMany)
   {
      if(!(sec = line->backsector))
         return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_crusher;
   }
   
   secnum = -1;
   // if not manual do all sectors tagged the same as the line
   while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
   {
      sec = &sectors[secnum];
      
manual_crusher:                
      // Do not start another function if ceiling already moving
      if(P_SectorActive(ceiling_special,sec)) //jff 2/22/98
      {
         if(!manual)
            continue;
         else
            return rtn;
      }

      // new ceiling thinker
      rtn = 1;
      ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, 0);
      P_AddThinker (&ceiling->thinker);
      sec->ceilingdata = ceiling; //jff 2/22/98
      ceiling->thinker.function = T_MoveCeiling;
      ceiling->crush = 10;
      ceiling->direction = plat_down;
      ceiling->sector = sec;
      ceiling->texture = sec->ceilingpic;
      ceiling->newspecial = sec->special;
      ceiling->tag = sec->tag;
      ceiling->type = Slnt? genSilentCrusher : genCrusher;
      ceiling->topheight = sec->ceilingheight;
      ceiling->bottomheight = sec->floorheight + (8*FRACUNIT);

      // setup ceiling motion speed
      switch (Sped)
      {
      case SpeedSlow:
         ceiling->speed = CEILSPEED;
         break;
      case SpeedNormal:
         ceiling->speed = CEILSPEED*2;
         break;
      case SpeedFast:
         ceiling->speed = CEILSPEED*4;
         break;
      case SpeedTurbo:
         ceiling->speed = CEILSPEED*8;
         break;
      default:
         break;
      }
      ceiling->oldspeed=ceiling->speed;

      P_AddActiveCeiling(ceiling); // add to list of active ceilings
      if (manual) return rtn;
   }
   return rtn;
}

// haleyjd 02/23/04: yuck, a global -- this is necessary because
// I can't change the prototype of EV_DoGenDoor
mobj_t *genDoorThing;

//
// GenDoorRetrigger
//
// haleyjd 02/23/04: This function handles retriggering of certain
// active generalized door types, a functionality which was neglected 
// in BOOM. To be retriggerable, the door must fit these criteria:
// 1. The thinker on the sector must be a T_VerticalDoor
// 2. The door type must be raise, not open or close
// 3. The activation trigger must be PushMany
//
// ** genDoorThing must be set before the calling routine is
//    executed! If it is NULL, no retrigger can occur.
//
static int GenDoorRetrigger(vldoor_t *door, int trig)
{
   if(genDoorThing && door->thinker.function == T_VerticalDoor &&
      (door->type == genRaise || door->type == genBlazeRaise) &&
      trig == PushMany)
   {
      if(door->direction == plat_down) // door is closing
         door->direction = plat_up;
      else
      {
         // monsters will not close doors
         if(!genDoorThing->player)
            return 0;
         door->direction = plat_down;
      }
      
      return 1;
   }

   return 0;
}

//
// EV_DoParamDoor
//
// haleyjd 05/04/04: Parameterized extension of the generalized
// door types. Absorbs code from the below two functions and adds
// the ability to pass in fully customized data. Values for the
// speed and delay types that are outside the range representable
// in BOOM generalized lines are used to indicate that full values 
// are contained in the doordata structure and should be used 
// instead of the hardcoded generalized options.
//
// Parameters:
// line -- pointer to originating line; may be NULL
// tag  -- tag of sectors to affect (may come from line or elsewhere)
// dd   -- pointer to full parameter info for door
//
int EV_DoParamDoor(line_t *line, int tag, doordata_t *dd)
{
   int secnum, rtn = 0;
   sector_t *sec;
   vldoor_t *door;
   boolean manual = false;
   char *sndname;

   // check if a manual trigger, if so do just the sector on the backside
   // haleyjd 05/04/04: door actions with no line can't be manual
   if(dd->trigger_type == PushOnce || dd->trigger_type == PushMany)
   {
      if(!line || !(sec = line->backsector))
         return rtn;
      secnum = sec - sectors;
      manual = true;
      goto manual_door;
   }

   secnum = -1;
   rtn = 0;

   // if not manual do all sectors tagged the same as the line
   while((secnum = P_FindSectorFromTag(tag, secnum)) >= 0)
   {
      sec = &sectors[secnum];
manual_door:
      // Do not start another function if ceiling already moving
      if(P_SectorActive(ceiling_special, sec)) //jff 2/22/98
      {
         if(manual)
         {
            // haleyjd 02/23/04: allow repushing of certain generalized
            // doors
            if(demo_version >= 331)
               rtn = GenDoorRetrigger(sec->ceilingdata, dd->trigger_type);

            return rtn;
         }
         continue;
      }

      // new door thinker
      rtn = 1;
      door = Z_Malloc(sizeof(*door), PU_LEVSPEC, 0);
      P_AddThinker(&door->thinker);
      sec->ceilingdata = door; //jff 2/22/98
      
      door->thinker.function = T_VerticalDoor;
      door->sector = sec;
      
      // setup delay for door remaining open/closed
      switch(dd->delay_type)
      {
      default:
      case doorWaitOneSec:
         door->topwait = 35;
         break;
      case doorWaitStd:
         door->topwait = VDOORWAIT;
         break;
      case doorWaitStd2x:
         door->topwait = 2*VDOORWAIT;
         break;
      case doorWaitStd7x:
         door->topwait = 7*VDOORWAIT;
         break;
      case doorWaitParam: // haleyjd 05/04/04: parameterized wait
         door->topwait = dd->delay_value;
         break;
      }

      // setup speed of door motion
      switch(dd->speed_type)
      {
      default:
      case SpeedSlow:
         door->speed = VDOORSPEED;
         break;
      case SpeedNormal:
         door->speed = VDOORSPEED*2;
         break;
      case SpeedFast:
         door->speed = VDOORSPEED*4;
         break;
      case SpeedTurbo:
         door->speed = VDOORSPEED*8;
         break;
      case SpeedParam: // haleyjd 05/04/04: parameterized speed
         door->speed = dd->speed_value;
         break;
      }
      door->line = line; // jff 1/31/98 remember line that triggered us

      // killough 10/98: implement gradual lighting
      // haleyjd 02/28/05: support light changes from alternate tag
      if(dd->usealtlighttag)
         door->lighttag = dd->altlighttag;
      else
         door->lighttag = !comp[comp_doorlight] && line && 
            (line->special&6) == 6 && 
            line->special > GenLockedBase ? line->tag : 0;
      
      // set kind of door, whether it opens then close, opens, closes etc.
      // assign target heights accordingly
      // haleyjd 05/04/04: fixed sound playing; was totally messed up!
      switch(dd->kind)
      {
      case OdCDoor:
         door->direction = plat_up;
         door->topheight = P_FindLowestCeilingSurrounding(sec);
         door->topheight -= 4*FRACUNIT;
         if(door->speed >= VDOORSPEED*4)
         {
            door->type = genBlazeRaise;
            sndname = LevelInfo.sound_bdopn;
         }
         else
         {
            door->type = genRaise;
            sndname = LevelInfo.sound_doropn;
         }
         if(door->topheight != sec->ceilingheight)
            S_StartSoundName((mobj_t *)&door->sector->soundorg, sndname);
         break;
      case ODoor:
         door->direction = plat_up;
         door->topheight = P_FindLowestCeilingSurrounding(sec);
         door->topheight -= 4*FRACUNIT;
         if(door->speed >= VDOORSPEED*4)
         {
            door->type = genBlazeOpen;
            sndname = LevelInfo.sound_bdopn;
         }
         else
         {
            door->type = genOpen;
            sndname = LevelInfo.sound_doropn;
         }
         if(door->topheight != sec->ceilingheight)
            S_StartSoundName((mobj_t *)&door->sector->soundorg, sndname);
         break;
      case CdODoor:
         door->topheight = sec->ceilingheight;
         door->direction = plat_down;
         if(door->speed >= VDOORSPEED*4)
         {
            door->type = genBlazeCdO;
            sndname = LevelInfo.sound_bdcls;
         }
         else
         {
            door->type = genCdO;
            sndname = LevelInfo.sound_dorcls;
         }
         S_StartSoundName((mobj_t *)&door->sector->soundorg, sndname);
         break;
      case CDoor:
         door->topheight = P_FindLowestCeilingSurrounding(sec);
         door->topheight -= 4*FRACUNIT;
         door->direction = plat_down;
         if(door->speed >= VDOORSPEED*4)
         {
            door->type = genBlazeClose;
            sndname = LevelInfo.sound_bdcls;
         }
         else
         {
            door->type = genClose;
            sndname = LevelInfo.sound_dorcls;
         }
         S_StartSoundName((mobj_t *)&door->sector->soundorg, sndname);
         break;
      // haleyjd: The following door types are parameterized only
      case pDOdCDoor:
         // parameterized "raise in" type
         door->direction = plat_special; // door starts in stasis
         door->topheight = P_FindLowestCeilingSurrounding(sec);
         door->topheight -= 4*FRACUNIT;
         door->topcountdown = dd->topcountdown; // wait to start
         if(door->speed >= VDOORSPEED*4)
            door->type = paramBlazeRaiseIn;
         else
            door->type = paramRaiseIn;
         break;
      case pDCDoor:
         // parameterized "close in" type
         door->direction    = plat_stop;        // door starts in wait
         door->topcountdown = dd->topcountdown; // wait to start
         if(door->speed >= VDOORSPEED*4)
            door->type = paramBlazeCloseIn;
         else
            door->type = paramCloseIn;
         break;
         break;
      default:
         break;
      }
      if(manual)
         return rtn;
   }
   return rtn;
}

//
// EV_DoGenLockedDoor()
//
// Handle generalized locked door types
//
// Passed the linedef activating the generalized locked door
// Returns true if a thinker created
//
// haleyjd 05/04/04: rewritten to use EV_DoParamDoor
//
int EV_DoGenLockedDoor(line_t *line)
{
   doordata_t dd;
   unsigned value = (unsigned)line->special - GenLockedBase;

   // parse the bit fields in the line's special type
   
   dd.delay_type   = doorWaitStd;
   dd.kind         = (value & LockedKind ) >> LockedKindShift;
   dd.speed_type   = (value & LockedSpeed) >> LockedSpeedShift;
   dd.trigger_type = (value & TriggerType) >> TriggerTypeShift;
   dd.usealtlighttag = false;
   
   return EV_DoParamDoor(line, line->tag, &dd);
}

//
// EV_DoGenDoor()
//
// Handle generalized door types
//
// Passed the linedef activating the generalized door
// Returns true if a thinker created
//
// haleyjd 05/04/04: rewritten to use EV_DoParamDoor
//
int EV_DoGenDoor(line_t* line)
{
   doordata_t dd;
   unsigned value = (unsigned)line->special - GenDoorBase;

   // parse the bit fields in the line's special type
   
   dd.delay_type   = (value & DoorDelay  ) >> DoorDelayShift;
   dd.kind         = (value & DoorKind   ) >> DoorKindShift;
   dd.speed_type   = (value & DoorSpeed  ) >> DoorSpeedShift;
   dd.trigger_type = (value & TriggerType) >> TriggerTypeShift;
   dd.usealtlighttag = false;

   return EV_DoParamDoor(line, line->tag, &dd);
}

//
// haleyjd 02/28/05: Parameterized Line Special System
//
// This is the code that dispatches requests to execute parameterized
// line specials, which work very similar to Hexen's line specials.
// Parameterized specials avoid code explosion by absorbing the
// generalized line code as special cases and then allowing fully
// customized data to be passed into the EV_ functions above inside
// new structs that hold all the parameters. This is a lot easier and
// more compatible than converting generalized lines into parameterized
// specials at run-time.
//

//
// pspec_TriggerType
//
// Routine to get a generalized line trigger type for a given
// parameterized special activation.
//
static int pspec_TriggerType(int spac, long tag, boolean reuse)
{
   int trig = 0;

   // zero tags must always be treated as push types
   if(!tag)
      return (reuse ? PushMany : PushOnce);

   switch(spac)
   {
   case SPAC_USE:
      trig = (reuse ? SwitchMany : SwitchOnce);
      break;
   case SPAC_IMPACT:
      trig = (reuse ? GunMany : GunOnce);
      break;
   case SPAC_CROSS:
      trig = (reuse ? WalkMany : WalkOnce);
      break;
   case SPAC_PUSH:
      // TODO
      break;
   }

   return trig;
}

// parameterized door trigger type lookup table

static int param_door_kinds[6] =
{
   OdCDoor, ODoor, CDoor, CdODoor, pDOdCDoor, pDCDoor
};

//
// pspec_Door
//
// Implements Door_Raise, Door_Open, Door_Close, Door_CloseWaitOpen,
// Door_WaitRaise, and Door_WaitClose specials.
//
static boolean pspec_Door(line_t *line, mobj_t *thing, long *args, 
                          short special, int trigger_type)
{
   int kind;
   doordata_t dd;

#ifdef RANGECHECK
   if(special < 300 || special > 305)
      I_Error("pspec_Door: parameterized door special out of range\n");
#endif

   kind = param_door_kinds[special - 300];

   // speed is always second parameter
   // value is eighths of a unit per tic
   dd.speed_type  = SpeedParam;
   dd.speed_value = args[1] * FRACUNIT / 8;
  
   // all param doors support alternate light tagging
   dd.usealtlighttag = true;

   // OdC and CdO doors support wait as third param.
   // pDCDoor has topcountdown as third param
   // pDOdCDoor has delay and countdown as third and fourth
   // Other door types have light tag as third param.
   switch(kind)
   {
   case OdCDoor:
   case CdODoor:
      dd.delay_type  = doorWaitParam;
      dd.delay_value = args[2];
      dd.altlighttag = args[3];
      break;
   case pDCDoor:
      dd.delay_type   = doorWaitStd; // not used by this door type
      dd.topcountdown = args[2];
      dd.altlighttag  = args[3];
      break;
   case pDOdCDoor:
      dd.delay_type   = doorWaitParam;
      dd.delay_value  = args[2];
      dd.topcountdown = args[3];
      dd.altlighttag  = args[4];
      break;
   default:
      dd.delay_type  = doorWaitStd; // not used by this door type
      dd.altlighttag = args[2];
      break;
   }

   dd.kind = kind;
   dd.trigger_type = trigger_type;

   // set genDoorThing in case of manual retrigger
   genDoorThing = thing;

   return EV_DoParamDoor(line, args[0], &dd);
}

//
// Tablified data for parameterized floor types
//

static int param_floor_data[16][2] =
{
//  dir trigger
   { 1, FtoHnF   }, // 306: Floor_RaiseToHighest
   { 0, FtoHnF   }, // 307: Floor_LowerToHighest
   { 1, FtoLnF   }, // 308: Floor_RaiseToLowest
   { 0, FtoLnF   }, // 309: Floor_LowerToLowest
   { 1, FtoNnF   }, // 310: Floor_RaiseToNearest
   { 0, FtoNnF   }, // 311: Floor_LowerToNearest
   { 1, FtoLnC   }, // 312: Floor_RaiseToLowestCeiling
   { 0, FtoLnC   }, // 313: Floor_LowerToLowestCeiling
   { 1, FtoC     }, // 314: Floor_RaiseToCeiling
   { 1, FbyST    }, // 315: Floor_RaiseByTexture
   { 0, FbyST    }, // 316: Floor_LowerByTexture
   { 1, FbyParam }, // 317: Floor_RaiseByValue
   { 0, FbyParam }, // 318: Floor_LowerByValue
   { 1, FtoAbs   }, // 319: Floor_MoveToValue (note: this dir not used)
   { 1, FInst    }, // 320: Floor_RaiseInstant
   { 0, FInst    }, // 321: Floor_LowerInstant
};

static int fchgdata[7][2] =
{
//   model          change type
   { FTriggerModel, FNoChg },
   { FTriggerModel, FChgZero },
   { FNumericModel, FChgZero },
   { FTriggerModel, FChgTxt  },
   { FNumericModel, FChgTxt  },
   { FTriggerModel, FChgTyp  },
   { FNumericModel, FChgTyp  },
};

//
// pspec_Floor
//
// Implements parameterized floor specials.
//
static boolean pspec_Floor(line_t *line, long *args, short special, 
                           int trigger_type)
{
   floordata_t fd;
   int normspec;

#ifdef RANGECHECK
   if(special < 306 || special > 321)
      I_Error("pspec_Floor: parameterized floor trigger out of range\n");
#endif

   normspec = special - 306;

   fd.direction   = param_floor_data[normspec][0];
   fd.target_type = param_floor_data[normspec][1];
   fd.trigger_type = trigger_type;
   fd.crush = -1;

   switch(special)
   {
   case 306: // Floor_RaiseToHighest
   case 310: // Floor_RaiseToNearest
   case 312: // Floor_RaiseToLowestCeiling
   case 314: // Floor_RaiseToCeiling
   case 315: // Floor_RaiseByTexture
      fd.crush = args[3];
      // fall through:
   case 307: // Floor_LowerToHighest
   case 309: // Floor_LowerToLowest
   case 311: // Floor_LowerToNearest
   case 313: // Floor_LowerToLowestCeiling
   case 316: // Floor_LowerByTexture
      fd.speed_type   = SpeedParam;
      fd.speed_value  = args[1] * FRACUNIT / 8;
      if(args[2] >= 0 && args[2] < 7)
      {
         fd.change_model = fchgdata[args[2]][0];
         fd.change_type  = fchgdata[args[2]][1];
      }
      else
      {
         fd.change_model = 0;
         fd.change_type  = 0;
      }
      break;
   case 308: // Floor_RaiseToLowest -- special case, always instant
      fd.speed_type   = SpeedNormal; // not used
      if(args[1] >= 0 && args[1] < 7)
      {
         fd.change_model = fchgdata[args[1]][0];
         fd.change_type  = fchgdata[args[1]][1];
      }
      else
      {
         fd.change_model = 0;
         fd.change_type  = 0;
      }
      fd.crush = args[2];
      break;
   case 317: // Floor_RaiseByValue
   case 319: // Floor_MoveToValue
      fd.crush = args[4];
      // fall through:
   case 318: // Floor_LowerByValue
      fd.speed_type   = SpeedParam;
      fd.speed_value  = args[1] * FRACUNIT / 8;
      fd.height_value = args[2] * FRACUNIT;
      if(args[3] >= 0 && args[3] < 7)
      {
         fd.change_model = fchgdata[args[3]][0];
         fd.change_type  = fchgdata[args[3]][1];
      }
      else
      {
         fd.change_model = 0;
         fd.change_type  = 0;
      }
      break;
   case 320: // Floor_RaiseInstant
      fd.crush = args[3];
      // fall through:
   case 321: // Floor_LowerInstant
      fd.speed_type   = SpeedNormal; // not really used
      fd.height_value = args[1] * FRACUNIT;
      if(args[2] >= 0 && args[2] < 7)
      {
         fd.change_model = fchgdata[args[2]][0];
         fd.change_type  = fchgdata[args[2]][1];
      }
      else
      {
         fd.change_model = 0;
         fd.change_type  = 0;
      }
      break;
   }

   return EV_DoParamFloor(line, args[0], &fd);
}

//
// P_ExecParamLineSpec
//
// Executes a parameterized line special.
//
// line:    Pointer to line being activated. May be NULL in this context.
// thing:   Pointer to thing doing activation. May be NULL in this context.
// special: Special to execute.
// args:    Arguments to special.
// side:    Side of line activated. May be ignored.
// reuse:   if action is repeatable
//
boolean P_ExecParamLineSpec(line_t *line, mobj_t *thing, short special, 
                            long *args, int side, int spac, boolean reuse)
{
   boolean success = false;

   int trigger_type = pspec_TriggerType(spac, args[0], reuse);

   switch(special)
   {
   case 300: // Door_Raise
   case 301: // Door_Open
   case 302: // Door_Close
   case 303: // Door_CloseWaitOpen
   case 304: // Door_WaitRaise
   case 305: // Door_WaitClose
      success = pspec_Door(line, thing, args, special, trigger_type);
      break;
   case 306: // Floor_RaiseToHighest
   case 307: // Floor_LowerToHighest
   case 308: // Floor_RaiseToLowest
   case 309: // Floor_LowerToLowest
   case 310: // Floor_RaiseToNearest
   case 311: // Floor_LowerToNearest
   case 312: // Floor_RaiseToLowestCeiling
   case 313: // Floor_LowerToLowestCeiling
   case 314: // Floor_RaiseToCeiling
   case 315: // Floor_RaiseByTexture
   case 316: // Floor_LowerByTexture
   case 317: // Floor_RaiseByValue
   case 318: // Floor_LowerByValue
   case 319: // Floor_MoveToValue
   case 320: // Floor_RaiseInstant
   case 321: // Floor_LowerInstant
      success = pspec_Floor(line, args, special, trigger_type);
      break;
   default:
      break;
   }

   return success;
}

//
// P_ActivateParamLine
//
// Handles a line activation and dispatches it to the appropriate
// parameterized line special.
//
// line:  The line being activated. Never NULL in this context.
// thing: The thing that wants to activate this line. Never NULL in this context.
// side:  Side of line activated, 0 or 1.
// spac:  Type of activation. This is de-wed from the special with
//        parameterized lines using the ExtraData extflags line field.
//
boolean P_ActivateParamLine(line_t *line, mobj_t *thing, int side, int spac)
{
   boolean success = false, reuse = false;
   long flags = 0;

   // check player / monster / missile enable flags
   if(thing->player)                   // treat as player?
      flags |= EX_ML_PLAYER;
   if(thing->flags3 & MF3_SPACMISSILE) // treat as missile?
      flags |= EX_ML_MISSILE;
   if(thing->flags3 & MF3_SPACMONSTER) // treat as monster?
      flags |= EX_ML_MONSTER;

   if(!(line->extflags & flags))
      return false;

   // check activation flags -- can we activate this line this way?
   switch(spac)
   {
   case SPAC_CROSS:
      flags = EX_ML_CROSS;
      break;
   case SPAC_USE:
      flags = EX_ML_USE;
      break;
   case SPAC_IMPACT:
      flags = EX_ML_IMPACT;
      break;
   case SPAC_PUSH:
      flags = EX_ML_PUSH;
      break;
   }

   if(!(line->extflags & flags))
      return false;

   // check 1S only flag -- if set, must be activated from first side
   if(line->extflags & EX_ML_1SONLY && side != 0)
      return false;

   // is action reusable?
   if(line->extflags & EX_ML_REPEAT)
      reuse = true;

   // execute the special
   success = P_ExecParamLineSpec(line, thing, line->special, line->args,
                                 side, spac, reuse);

   // actions to take if line activation was successful:
   if(success)
   {
      // clear special if line is not repeatable
      if(!reuse)
         line->special = 0;
      
      // change switch textures where appropriate
      if(spac == SPAC_USE || spac == SPAC_IMPACT)
         P_ChangeSwitchTexture(line, reuse, side);
   }

   return success;
}

//
// Small Natives
//

enum
{
   SM_SPEC_NULL,
   SM_SPEC_PASS
};

//
// sm_specialmode
//
// Allows the semantics for specials called from line scripts to be
// changed so that the special is executed as if it belongs to the line
// that started the script. This allows use of zero tags and trigger
// model change types.
//
static cell AMX_NATIVE_CALL sm_specialmode(AMX *amx, cell *params)
{
   SmallContext_t *ctx = A_GetContextForAMX(amx);

   ctx->invocationData.spec_mode = params[1];

   return 0;
}

//
// P_ScriptSpec
//
// haleyjd 05/20/05
//
// Thunks from Small script params to line special args and executes
// the indicated special. All functions using this must take the
// same arguments in the same order as the line special.
//
static boolean P_ScriptSpec(short spec, AMX *amx, cell *params)
{
   long args[5] = { 0, 0, 0, 0, 0 };
   int i, numparams = params[0] / sizeof(cell);
   SmallContext_t *ctx;
   line_t *line  = NULL;
   mobj_t *thing = NULL;

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   ctx = A_GetContextForAMX(amx);

   // if special mode is "pass", pass on the line and thing involved
   // in a line script execution so that the special executes as if it
   // came from the line that started the script.

   if(ctx->invocationData.invokeType == SC_INVOKE_LINE &&
      ctx->invocationData.spec_mode == SM_SPEC_PASS)
   {
      line  = ctx->invocationData.line;
      thing = ctx->invocationData.trigger;
   }

   for(i = 0; i < numparams; ++i)
      args[i] = params[i + 1];

   return P_ExecParamLineSpec(line, thing, spec, args, 0, SPAC_CROSS, true);
}

//
// Small Param Line Special Wrappers
//

static cell AMX_NATIVE_CALL sm_door_raise(AMX *amx, cell *params)
{
   return P_ScriptSpec(300, amx, params);
}

static cell AMX_NATIVE_CALL sm_door_open(AMX *amx, cell *params)
{
   return P_ScriptSpec(301, amx, params);
}

static cell AMX_NATIVE_CALL sm_door_close(AMX *amx, cell *params)
{
   return P_ScriptSpec(302, amx, params);
}

static cell AMX_NATIVE_CALL sm_door_closewaitopen(AMX *amx, cell *params)
{
   return P_ScriptSpec(303, amx, params);
}

static cell AMX_NATIVE_CALL sm_door_waitraise(AMX *amx, cell *params)
{
   return P_ScriptSpec(304, amx, params);
}

static cell AMX_NATIVE_CALL sm_door_waitclose(AMX *amx, cell *params)
{
   return P_ScriptSpec(305, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raisetohighest(AMX *amx, cell *params)
{
   return P_ScriptSpec(306, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_lowertohighest(AMX *amx, cell *params)
{
   return P_ScriptSpec(307, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raisetolowest(AMX *amx, cell *params)
{
   return P_ScriptSpec(308, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_lowertolowest(AMX *amx, cell *params)
{
   return P_ScriptSpec(309, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raisetonearest(AMX *amx, cell *params)
{
   return P_ScriptSpec(310, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_lowertonearest(AMX *amx, cell *params)
{
   return P_ScriptSpec(311, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raisetolowestceiling(AMX *amx, cell *params)
{
   return P_ScriptSpec(312, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_lowertolowestceiling(AMX *amx, cell *params)
{
   return P_ScriptSpec(313, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raisetoceiling(AMX *amx, cell *params)
{
   return P_ScriptSpec(314, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raisebytexture(AMX *amx, cell *params)
{
   return P_ScriptSpec(315, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_lowerbytexture(AMX *amx, cell *params)
{
   return P_ScriptSpec(316, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raisebyvalue(AMX *amx, cell *params)
{
   return P_ScriptSpec(317, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_lowerbyvalue(AMX *amx, cell *params)
{
   return P_ScriptSpec(318, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_movetovalue(AMX *amx, cell *params)
{
   return P_ScriptSpec(319, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_raiseinstant(AMX *amx, cell *params)
{
   return P_ScriptSpec(320, amx, params);
}

static cell AMX_NATIVE_CALL sm_floor_lowerinstant(AMX *amx, cell *params)
{
   return P_ScriptSpec(321, amx, params);
}

AMX_NATIVE_INFO genlin_Natives[] =
{
   { "_SpecialMode",          sm_specialmode          },
   { "_Door_Raise",           sm_door_raise           },
   { "_Door_Open",            sm_door_open            },
   { "_Door_Close",           sm_door_close           },
   { "_Door_CloseWaitOpen",   sm_door_closewaitopen   },
   { "_Door_WaitRaise",       sm_door_waitraise       },
   { "_Door_WaitClose",       sm_door_waitclose       },
   { "_Floor_RaiseToHighest", sm_floor_raisetohighest },
   { "_Floor_LowerToHighest", sm_floor_lowertohighest },
   { "_Floor_RaiseToLowest",  sm_floor_raisetolowest  },
   { "_Floor_LowerToLowest",  sm_floor_lowertolowest  },
   { "_Floor_RaiseToNearest", sm_floor_raisetonearest },
   { "_Floor_LowerToNearest", sm_floor_lowertonearest },
   { "_Floor_RaiseToLowestCeiling", sm_floor_raisetolowestceiling },
   { "_Floor_LowerToLowestCeiling", sm_floor_lowertolowestceiling },
   { "_Floor_RaiseToCeiling", sm_floor_raisetoceiling },
   { "_Floor_RaiseByTexture", sm_floor_raisebytexture },
   { "_Floor_LowerByTexture", sm_floor_lowerbytexture },
   { "_Floor_RaiseByValue",   sm_floor_raisebyvalue   },
   { "_Floor_LowerByValue",   sm_floor_lowerbyvalue   },
   { "_Floor_MoveToValue",    sm_floor_movetovalue    },
   { "_Floor_RaiseInstant",   sm_floor_raiseinstant   },
   { "_Floor_LowerInstant",   sm_floor_lowerinstant   },
   { NULL,          NULL }
};

//----------------------------------------------------------------------------
//
// $Log: p_genlin.c,v $
// Revision 1.18  1998/05/23  10:23:23  jim
// Fix numeric changer loop corruption
//
// Revision 1.17  1998/05/08  03:34:56  jim
// formatted/documented p_genlin
//
// Revision 1.16  1998/05/03  23:05:56  killough
// Fix #includes at the top, nothing else
//
// Revision 1.15  1998/04/16  06:25:23  killough
// Fix generalized doors' opening sounds
//
// Revision 1.14  1998/04/05  13:54:10  jim
// fixed switch change on second activation
//
// Revision 1.13  1998/03/31  16:52:15  jim
// Fixed uninited type field in stair builders
//
// Revision 1.12  1998/03/20  14:24:28  jim
// Gen ceiling target now shortest UPPER texture
//
// Revision 1.11  1998/03/15  14:40:14  jim
// added pure texture change linedefs & generalized sector types
//
// Revision 1.10  1998/03/13  14:05:56  jim
// Fixed arith overflow in some linedef types
//
// Revision 1.9  1998/03/04  11:56:30  jim
// Fix multiple sector stair raise
//
// Revision 1.8  1998/02/27  11:50:59  jim
// Fixes for stairs
//
// Revision 1.7  1998/02/23  23:46:50  jim
// Compatibility flagged multiple thinker support
//
// Revision 1.6  1998/02/23  00:41:46  jim
// Implemented elevators
//
// Revision 1.4  1998/02/17  06:07:56  killough
// Change RNG calling sequence
//
// Revision 1.3  1998/02/13  03:28:36  jim
// Fixed W1,G1 linedefs clearing untriggered special, cosmetic changes
//
//
// Revision 1.1.1.1  1998/02/04  09:19:00  jim
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
          
