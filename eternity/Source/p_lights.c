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
//  Action routines for lighting thinkers
//  Spawn sector based lighting effects.
//  Handle lighting linedef types
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_lights.c,v 1.11 1998/05/18 09:04:41 jim Exp $";

#include "doomstat.h" //jff 5/18/98
#include "doomdef.h"
#include "m_random.h"
#include "r_main.h"
#include "p_spec.h"
#include "p_tick.h"

//////////////////////////////////////////////////////////
//
// Lighting action routines, called once per tick
//
//////////////////////////////////////////////////////////

//
// T_FireFlicker()
//
// Firelight flicker action routine, called once per tick
//
// Passed a fireflicker_t structure containing light levels and timing
// Returns nothing
//
void T_FireFlicker (fireflicker_t* flick)
{
   int amount;
   
   if(--flick->count)
      return;
   
   amount = (P_Random(pr_lights)&3)*16;
   
   if(flick->sector->lightlevel - amount < flick->minlight)
      flick->sector->lightlevel = flick->minlight;
   else
      flick->sector->lightlevel = flick->maxlight - amount;
   
   flick->count = 4;
}

//
// T_LightFlash()
//
// Broken light flashing action routine, called once per tick
//
// Passed a lightflash_t structure containing light levels and timing
// Returns nothing
//
void T_LightFlash (lightflash_t* flash)
{
   if(--flash->count)
      return;
   
   if(flash->sector->lightlevel == flash->maxlight)
   {
      flash-> sector->lightlevel = flash->minlight;
      flash->count = (P_Random(pr_lights)&flash->mintime)+1;
   }
   else
   {
      flash-> sector->lightlevel = flash->maxlight;
      flash->count = (P_Random(pr_lights)&flash->maxtime)+1;
   }
}

//
// T_StrobeFlash()
//
// Strobe light flashing action routine, called once per tick
//
// Passed a strobe_t structure containing light levels and timing
// Returns nothing
//
void T_StrobeFlash (strobe_t*   flash)
{
   if(--flash->count)
      return;
   
   if(flash->sector->lightlevel == flash->minlight)
   {
      flash-> sector->lightlevel = flash->maxlight;
      flash->count = flash->brighttime;
   }
   else
   {
      flash-> sector->lightlevel = flash->minlight;
      flash->count =flash->darktime;
   }
}

//
// T_Glow()
//
// Glowing light action routine, called once per tick
//
// Passed a glow_t structure containing light levels and timing
// Returns nothing
//

void T_Glow(glow_t* g)
{
   switch(g->direction)
   {
   case -1:
      // light dims
      g->sector->lightlevel -= GLOWSPEED;
      if(g->sector->lightlevel <= g->minlight)
      {
         g->sector->lightlevel += GLOWSPEED;
         g->direction = 1;
      }
      break;
      
   case 1:
      // light brightens
      g->sector->lightlevel += GLOWSPEED;
      if(g->sector->lightlevel >= g->maxlight)
      {
         g->sector->lightlevel -= GLOWSPEED;
         g->direction = -1;
      }
      break;
   }
}

// sf 13/10/99:
//
// T_LightFade()
//
// Just fade the light level in a sector to a new level
//

void T_LightFade(lightlevel_t *ll)
{
   if(ll->sector->lightlevel < ll->destlevel)
   {
      // increase the lightlevel
      if(ll->sector->lightlevel + ll->speed >= ll->destlevel)
      {
         // stop changing light level
         ll->sector->lightlevel = ll->destlevel; // set to dest lightlevel
         
         ll->sector->lightingdata = NULL;        // clear lightingdata
         P_RemoveThinker(&ll->thinker);          // remove thinker
      }
      else
      {
         ll->sector->lightlevel += ll->speed; // move lightlevel
      }
   }
   else
   {
      // decrease lightlevel
      if(ll->sector->lightlevel - ll->speed <= ll->destlevel)
      {
         // stop changing light level
         ll->sector->lightlevel = ll->destlevel;  // set to dest lightlevel
         
         ll->sector->lightingdata = NULL;         // clear lightingdata
         P_RemoveThinker(&ll->thinker);           // remove thinker       
      }
      else
      {
         ll->sector->lightlevel -= ll->speed;     // move lightlevel
      }
   }
}


//////////////////////////////////////////////////////////
//
// Sector lighting type spawners
//
// After the map has been loaded, each sector is scanned
// for specials that spawn thinkers
//
//////////////////////////////////////////////////////////

//
// P_SpawnFireFlicker()
//
// Spawns a fire flicker lighting thinker
//
// Passed the sector that spawned the thinker
// Returns nothing
//
void P_SpawnFireFlicker (sector_t *sector)
{
   fireflicker_t *flick;
   
   // Note that we are resetting sector attributes.
   // Nothing special about it during gameplay.
   sector->special &= ~31; //jff 3/14/98 clear non-generalized sector type
   
   flick = Z_Malloc(sizeof(*flick), PU_LEVSPEC, 0);
   
   P_AddThinker(&flick->thinker);
   
   flick->thinker.function = T_FireFlicker;
   flick->sector = sector;
   flick->maxlight = sector->lightlevel;
   flick->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel)+16;
   flick->count = 4;
}

//
// P_SpawnLightFlash()
//
// Spawns a broken light flash lighting thinker
//
// Passed the sector that spawned the thinker
// Returns nothing
//
void P_SpawnLightFlash (sector_t *sector)
{
   lightflash_t *flash;
   
   // nothing special about it during gameplay
   sector->special &= ~31; //jff 3/14/98 clear non-generalized sector type
   
   flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);
   
   P_AddThinker(&flash->thinker);
   
   flash->thinker.function = T_LightFlash;
   flash->sector = sector;
   flash->maxlight = sector->lightlevel;
   
   flash->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
   flash->maxtime = 64;
   flash->mintime = 7;
   flash->count = (P_Random(pr_lights)&flash->maxtime)+1;
}

//
// P_SpawnStrobeFlash
//
// Spawns a blinking light thinker
//
// Passed the sector that spawned the thinker, speed of blinking
// and whether blinking is to by syncrhonous with other sectors
//
// Returns nothing
//
void P_SpawnStrobeFlash
( sector_t *sector,
  int   fastOrSlow,
  int   inSync )
{
   strobe_t *flash;
   
   flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);
   
   P_AddThinker(&flash->thinker);
   
   flash->sector = sector;
   flash->darktime = fastOrSlow;
   flash->brighttime = STROBEBRIGHT;
   flash->thinker.function = T_StrobeFlash;
   flash->maxlight = sector->lightlevel;
   flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);
   
   if(flash->minlight == flash->maxlight)
      flash->minlight = 0;
   
   // nothing special about it during gameplay
   sector->special &= ~31; //jff 3/14/98 clear non-generalized sector type
   
   if(!inSync)
      flash->count = (P_Random(pr_lights)&7)+1;
   else
      flash->count = 1;
}

//
// P_SpawnGlowingLight()
//
// Spawns a glowing light (smooth oscillation from min to max) thinker
//
// Passed the sector that spawned the thinker
// Returns nothing
//
void P_SpawnGlowingLight(sector_t *sector)
{
   glow_t *g;
   
   g = Z_Malloc(sizeof(*g), PU_LEVSPEC, 0);
   
   P_AddThinker(&g->thinker);
   
   g->sector = sector;
   g->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
   g->maxlight = sector->lightlevel;
   g->thinker.function = T_Glow;
   g->direction = -1;
   
   sector->special &= ~31; //jff 3/14/98 clear non-generalized sector type
}

// sf 13/10/99:
//
// P_FadeLight()
//
// Fade all the lights in sectors with a particular tag to a new value
//

void P_FadeLight(int tag, int destvalue, int speed)
{
   int i;
   lightlevel_t *ll;
   
   // search all sectors for ones with tag
   for(i = -1; (i = P_FindSectorFromTag(tag,i)) >= 0;)
   {
      sector_t *sector = &sectors[i];
      sector->lightingdata = sector;    // just set it to something
      
      ll = Z_Malloc(sizeof(*ll), PU_LEVSPEC, 0);
      ll->thinker.function = T_LightFade;
      
      P_AddThinker(&ll->thinker);       // add thinker
      
      ll->sector = sector;
      ll->destlevel = destvalue;
      ll->speed = speed;
   }
}

//////////////////////////////////////////////////////////
//
// Linedef lighting function handlers
//
//////////////////////////////////////////////////////////

//
// EV_StartLightStrobing()
//
// Start strobing lights (usually from a trigger)
//
// Passed the line that activated the strobing
// Returns true
//
// jff 2/12/98 added int return value, fixed return
//
int EV_StartLightStrobing(line_t* line)
{
   int   secnum;
   sector_t* sec;
   
   secnum = -1;
   // start lights strobing in all sectors tagged same as line
   while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
   {
      sec = &sectors[secnum];
      // if already doing a lighting function, don't start a second
      if(P_SectorActive(lighting_special,sec)) //jff 2/22/98
         continue;
      
      P_SpawnStrobeFlash (sec,SLOWDARK, 0);
   }
   return 1;
}

//
// EV_TurnTagLightsOff()
//
// Turn line's tagged sector's lights to min adjacent neighbor level
//
// Passed the line that activated the lights being turned off
// Returns true
//
// jff 2/12/98 added int return value, fixed return
//
int EV_TurnTagLightsOff(line_t* line)
{
   int j;
   
   // search sectors for those with same tag as activating line
   
   // killough 10/98: replaced inefficient search with fast search
   for(j = -1; (j = P_FindSectorFromLineTag(line,j)) >= 0;)
   {
      sector_t *sector = sectors + j, *tsec;
      int i, min = sector->lightlevel;
      // find min neighbor light level
      for(i = 0;i < sector->linecount; i++)
      {
         if ((tsec = getNextSector(sector->lines[i], sector)) &&
            tsec->lightlevel < min)
            min = tsec->lightlevel;
      }
      sector->lightlevel = min;
   }
   return 1;
}

//
// EV_LightTurnOn()
//
// Turn sectors tagged to line lights on to specified or max neighbor level
//
// Passed the activating line, and a level to set the light to
// If level passed is 0, the maximum neighbor lighting is used
// Returns true
//
// jff 2/12/98 added int return value, fixed return
//
int EV_LightTurnOn(line_t *line, int bright)
{
   int i;
   
   // search all sectors for ones with same tag as activating line
   
   // killough 10/98: replace inefficient search with fast search
   for(i = -1; (i = P_FindSectorFromLineTag(line,i)) >= 0;)
   {
      sector_t *temp, *sector = sectors+i;
      int j, tbright = bright; //jff 5/17/98 search for maximum PER sector
      
      // bright = 0 means to search for highest light level surrounding sector
      
      if(!bright)
      {
         for(j = 0;j < sector->linecount; j++)
         {
            if((temp = getNextSector(sector->lines[j],sector)) &&
               temp->lightlevel > tbright)
               tbright = temp->lightlevel;
         }
      }

      sector->lightlevel = tbright;
      
      //jff 5/17/98 unless compatibility optioned 
      //then maximum near ANY tagged sector
      
      if(comp[comp_model])
         bright = tbright;
   }
   return 1;
}

// killough 10/98:
//
// EV_LightTurnOnPartway()
//
// Turn sectors tagged to line lights on to specified or max neighbor level
//
// Passed the activating line's tag, and a light level fraction between 
// 0 and 1. Sets the light to min on 0, max on 1, and interpolates in-
// between. Used for doors with gradual lighting effects.
//
// haleyjd 02/28/05: changed to take a tag instead of a line.
//
// Returns true
//
int EV_LightTurnOnPartway(int tag, fixed_t level)
{
   int i;
   
   if(level < 0)          // clip at extremes 
      level = 0;
   if(level > FRACUNIT)
      level = FRACUNIT;
   
   // search all sectors for ones with same tag as activating line
   for(i = -1; (i = P_FindSectorFromTag(tag,i)) >= 0;)
   {
      sector_t *temp, *sector = sectors + i;
      int j, bright = 0, min = sector->lightlevel;

      for(j = 0; j < sector->linecount; ++j)
      {
         if((temp = getNextSector(sector->lines[j], sector)))
         {
            if(temp->lightlevel > bright)
               bright = temp->lightlevel;
            if(temp->lightlevel < min)
               min = temp->lightlevel;
         }
      }

      sector->lightlevel =   // Set level in-between extremes
         (level * bright + (FRACUNIT - level) * min) >> FRACBITS;
   }
   return 1;
}

//----------------------------------------------------------------------------
//
// $Log: p_lights.c,v $
// Revision 1.11  1998/05/18  09:04:41  jim
// fix compatibility decl
//
// Revision 1.10  1998/05/17  11:31:36  jim
// fixed bug in lights to max neighbor
//
// Revision 1.9  1998/05/09  18:57:50  jim
// formatted/documented p_lights
//
// Revision 1.8  1998/05/03  23:17:23  killough
// Fix #includes at the top, nothing else
//
// Revision 1.7  1998/03/15  14:40:10  jim
// added pure texture change linedefs & generalized sector types
//
// Revision 1.6  1998/02/23  23:46:56  jim
// Compatibility flagged multiple thinker support
//
// Revision 1.5  1998/02/23  00:41:51  jim
// Implemented elevators
//
// Revision 1.4  1998/02/17  06:07:11  killough
// Change RNG calling sequence
//
// Revision 1.3  1998/02/13  03:28:42  jim
// Fixed W1,G1 linedefs clearing untriggered special, cosmetic changes
//
// Revision 1.2  1998/01/26  19:24:07  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:59  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
