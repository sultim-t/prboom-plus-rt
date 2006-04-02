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

//////////////////////////////////////////////////////////
//
// Generalized Linedef Type handlers
//
//////////////////////////////////////////////////////////

int EV_DoParamFloor(line_t *line, int tag, int Crsh, int ChgT, 
                    int Targ, int Dirn, int ChgM, int Sped, int Trig,
                    extfloordata_t *paramData)
{
   int         secnum;
   int         rtn = 0;
   boolean     manual = false;
   sector_t    *sec;
   floormove_t *floor;

   // check if a manual trigger, if so do just the sector on the backside
   // haleyjd 05/07/04: only line actions can be manual
   if(line && (Trig == PushOnce || Trig == PushMany))
   {
      if(!(sec = line->backsector))
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
      floor->crush = Crsh;
      floor->direction = Dirn ? plat_up : plat_down;
      floor->sector = sec;
      floor->texture = sec->floorpic;
      floor->newspecial = sec->special;
      //jff 3/14/98 transfer old special field too
      floor->oldspecial = sec->oldspecial;
      floor->type = genFloor;

      // set the speed of motion
      switch(Sped)
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
         floor->speed = paramData ? paramData->speed : FLOORSPEED*2;
         break;
      default:
         break;
      }

      // set the destination height
      switch(Targ)
      {
      case FtoHnF:
         floor->floordestheight = P_FindHighestFloorSurrounding(sec);
         break;
      case FtoLnF:
         floor->floordestheight = P_FindLowestFloorSurrounding(sec);
         break;
      case FtoNnF:
         floor->floordestheight = Dirn?
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
      case FbyParam: // haleyjd 05/07/04: parameterized extension
         floor->floordestheight = paramData ? paramData->targetheight : 0;
         break;
      default:
         break;
      }

      // set texture/type change properties
      if(ChgT)   // if a texture change is indicated
      {
         if(ChgM) // if a numeric model change
         {
            sector_t *sec;

            //jff 5/23/98 find model with ceiling at target height
            //if target is a ceiling type
            sec = (Targ==FtoLnC || Targ==FtoC)?
               P_FindModelCeilingSector(floor->floordestheight,secnum) :
               P_FindModelFloorSector(floor->floordestheight,secnum);
            
            if(sec)
            {
               floor->texture = sec->floorpic;
               switch(ChgT)
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
               switch(ChgT)
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
   unsigned value = (unsigned)line->special - GenFloorBase;

   // parse the bit fields in the line's special type
   
   int Crsh = (value & FloorCrush) >> FloorCrushShift;
   int ChgT = (value & FloorChange) >> FloorChangeShift;
   int Targ = (value & FloorTarget) >> FloorTargetShift;
   int Dirn = (value & FloorDirection) >> FloorDirectionShift;
   int ChgM = (value & FloorModel) >> FloorModelShift;
   int Sped = (value & FloorSpeed) >> FloorSpeedShift;
   int Trig = (value & TriggerType) >> TriggerTypeShift;

   return EV_DoParamFloor(line, line->tag, Crsh, ChgT, Targ, Dirn,
                          ChgM, Sped, Trig, NULL);
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
      ceiling->crush = Crsh;
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
      plat->crush = false;
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
      floor->crush = false;
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
            floor->crush = false;
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
      ceiling->crush = true;
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
// 4. This routine is only called for manual push doors.
//
// ** genDoorThing must be set before the calling routine is
//    executed!
//
static int GenDoorRetrigger(vldoor_t *door, int trig)
{
   if(door->thinker.function == T_VerticalDoor &&
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
// the ability to pass in fully customized data.
//
// Parameters:
// line -- pointer to originating line; may be NULL
// tag  -- tag of sectors to affect (may come from line or elsewhere)
// Dely, Kind, Sped, Trig -- basic info as extracted for generalized types
// paramData -- pointer to extra info for parameterized doors; may be NULL
//
int EV_DoParamDoor(line_t *line, int tag,
                   int Dely, int Kind, int Sped, int Trig,
                   extdoordata_t *paramData)
{
   int secnum, rtn = 0;
   sector_t *sec;
   vldoor_t *door;
   boolean manual = false;
   char *sndname;

   // check if a manual trigger, if so do just the sector on the backside
   // haleyjd 05/04/04: door actions with no line can't be manual
   if(line && (Trig == PushOnce || Trig == PushMany))
   {
      if(!(sec = line->backsector))
         return rtn;
      secnum = sec-sectors;
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
      if(P_SectorActive(ceiling_special,sec)) //jff 2/22/98
      {
         if(manual)
         {
            // haleyjd 02/23/04: allow repushing of certain generalized
            // doors
            if(demo_version >= 331)
            {
               rtn = GenDoorRetrigger(sec->ceilingdata, Trig);
            }

            return rtn;
         }
         continue;
      }

      // new door thinker
      rtn = 1;
      door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
      P_AddThinker (&door->thinker);
      sec->ceilingdata = door; //jff 2/22/98
      
      door->thinker.function = T_VerticalDoor;
      door->sector = sec;
      // setup delay for door remaining open/closed
      switch(Dely)
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
      case doorWaitParam: // haleyjd 05/04/04: parameterized
         door->topwait = paramData ? paramData->topwait : VDOORWAIT;
         break;
      }

      // setup speed of door motion
      switch(Sped)
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
      case SpeedParam: // haleyjd 05/04/04: parameterized
         door->speed = paramData ? paramData->speed : VDOORSPEED;
         break;
      }
      door->line = line; // jff 1/31/98 remember line that triggered us

      // killough 10/98: implement gradual lighting
      // haleyjd 05/04/04: make sure line is valid
      door->lighttag = !comp[comp_doorlight] && line && 
         (line->special&6) == 6 && 
         line->special > GenLockedBase ? line->tag : 0;
      
      // set kind of door, whether it opens then close, opens, closes etc.
      // assign target heights accordingly
      // haleyjd 05/04/04: fixed sound playing; was totally messed up!
      switch(Kind)
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
int EV_DoGenLockedDoor(line_t* line)
{
   unsigned value = (unsigned)line->special - GenLockedBase;

   // parse the bit fields in the line's special type
   
   int Kind = (value & LockedKind) >> LockedKindShift;
   int Sped = (value & LockedSpeed) >> LockedSpeedShift;
   int Trig = (value & TriggerType) >> TriggerTypeShift;
   
   return EV_DoParamDoor(line, line->tag, doorWaitStd, Kind, Sped, Trig, 
                         NULL);
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
   unsigned value = (unsigned)line->special - GenDoorBase;

   // parse the bit fields in the line's special type
   
   int Dely = (value & DoorDelay) >> DoorDelayShift;
   int Kind = (value & DoorKind) >> DoorKindShift;
   int Sped = (value & DoorSpeed) >> DoorSpeedShift;
   int Trig = (value & TriggerType) >> TriggerTypeShift;

   return EV_DoParamDoor(line, line->tag, Dely, Kind, Sped, Trig, NULL);
}

//
// Small Natives
//

//
// sm_flooraction
// * Implements FloorAction(crush, direction, speed, changetex,
//                          id, idtype, desttype, destheight)
//
static cell AMX_NATIVE_CALL sm_flooraction(AMX *amx, cell *params)
{
   extfloordata_t fd;
   int id, crush, direction, desttype, changetex;

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   crush     = (int)params[1];
   direction = (int)params[2];
   fd.speed  = (int)params[3];
   changetex = (int)params[4];
   id        = (int)params[5];
   // TODO: SID support (params[6] == idtype)
   desttype  = (int)params[7];
   fd.targetheight = (int)params[8];

   return EV_DoParamFloor(NULL, id, crush, changetex, desttype,
                          direction, FNumericModel, SpeedParam, 
                          WalkMany, &fd);
}

//
// sm_dooraction
// * Implements DoorAction(kind, speed, delay, id, idtype = 0)
//
// This is the first function to use the new parameterized specials
// which expand the BOOM generalized line system. It allows door
// actions of any type to be started. Returns 1 if the action was
// successful, 0 otherwise.
//
static cell AMX_NATIVE_CALL sm_dooraction(AMX *amx, cell *params)
{
   extdoordata_t dd;
   int kind, id;

   if(gamestate != GS_LEVEL)
   {
      amx_RaiseError(amx, SC_ERR_GAMEMODE | SC_ERR_MASK);
      return -1;
   }

   kind       = (int)params[1];
   dd.speed   = (int)params[2];
   dd.topwait = (int)params[3];
   id         = (int)params[4];
   // TODO: SID support (params[5] == idtype)

   return EV_DoParamDoor(NULL, id, 
                         doorWaitParam, kind, SpeedParam, WalkMany, 
                         &dd);
}

AMX_NATIVE_INFO genlin_Natives[] =
{
   { "S_FloorAction", sm_flooraction },
   { "S_DoorAction",  sm_dooraction },
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
          
