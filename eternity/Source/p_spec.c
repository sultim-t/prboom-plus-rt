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
//   -Loads and initializes texture and flat animation sequences
//   -Implements utility functions for all linedef/sector special handlers
//   -Dispatches walkover and gun line triggers
//   -Initializes and implements special sector types
//   -Implements donut linedef triggers
//   -Initializes and implements BOOM linedef triggers for
//     Scrollers/Conveyors
//     Friction
//     Wind/Current
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_spec.c,v 1.56 1998/05/25 10:40:30 killough Exp $";

#include "doomstat.h"
#include "p_spec.h"
#include "p_tick.h"
#include "p_setup.h"
#include "m_random.h"
#include "d_englsh.h"
#include "m_argv.h"
#include "w_wad.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_map.h"
#include "g_game.h"
#include "p_inter.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_bbox.h"                                         // phares 3/20/98
#include "d_deh.h"
#include "r_plane.h"  // killough 10/98
#include "p_info.h"
#include "c_runcmd.h"
#include "hu_stuff.h"
#include "r_ripple.h"
#include "d_gi.h"
#include "p_user.h"
#include "e_edf.h"

//
// Animating textures and planes
// There is another anim_t used in wi_stuff, unrelated.
//
typedef struct
{
  boolean     istexture;
  int         picnum;
  int         basepic;
  int         numpics;
  int         speed;
} anim_t;

//
//      source animation definition
//
#ifdef _MSC_VER
#pragma pack(1)
#endif

typedef struct
{
   char istexture;            //jff 3/23/98 make char for comparison
   char endname[9];           //  if false, it is a flat
   char startname[9];
   int  speed;
} __attribute__ ((packed)) animdef_t; //jff 3/23/98 pack to read from memory

#ifdef _MSC_VER
#pragma pack()
#endif

#define MAXANIMS 32                   // no longer a strict limit -- killough
static anim_t *lastanim, *anims;      // new structure w/o limits -- killough
static size_t maxanims;

// killough 3/7/98: Initialize generalized scrolling
static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);    // phares 3/16/98
static void P_SpawnPushers(void);     // phares 3/20/98
static void P_SpawnHereticWind(line_t *line); // haleyjd 03/12/03

extern int allow_pushers;
extern int variable_friction;         // phares 3/20/98

// haleyjd 01/24/04: portals
#ifdef R_PORTALS
typedef enum
{
   portal_plane,
   portal_horizon,
   portal_skybox,
   portal_anchored
} portal_type;

typedef enum
{
   portal_ceiling,
   portal_floor,
   portal_both
} portal_effect;

static void P_SpawnPortal(line_t *, portal_type, portal_effect);
#endif

//
// P_InitPicAnims
//
// Load the table of animation definitions, checking for existence of
// the start and end of each frame. If the start doesn't exist the sequence
// is skipped, if the last doesn't exist, BOOM exits.
//
// Wall/Flat animation sequences, defined by name of first and last frame,
// The full animation sequence is given using all lumps between the start
// and end entry, in the order found in the WAD file.
//
// This routine modified to read its data from a predefined lump or
// PWAD lump called ANIMATED rather than a static table in this module to
// allow wad designers to insert or modify animation sequences.
//
// Lump format is an array of byte packed animdef_t structures, terminated
// by a structure with istexture == -1. The lump can be generated from a
// text source file using SWANTBLS.EXE, distributed with the BOOM utils.
// The standard list of switches and animations is contained in the example
// source text file DEFSWANI.DAT also in the BOOM util distribution.
//
//
void P_InitPicAnims (void)
{
   int         i;
   animdef_t   *animdefs; //jff 3/23/98 pointer to animation lump
   
   //  Init animation
   //jff 3/23/98 read from predefined or wad lump instead of table
   animdefs = W_CacheLumpName("ANIMATED",PU_STATIC);

   lastanim = anims;
   for(i=0 ; animdefs[i].istexture != -1 ; i++)
   {
      // 1/11/98 killough -- removed limit by array-doubling
      if(lastanim >= anims + maxanims)
      {
         size_t newmax = maxanims ? maxanims*2 : MAXANIMS;
         anims = realloc(anims, newmax*sizeof(*anims));   // killough
         lastanim = anims + maxanims;
         maxanims = newmax;
      }

      if (animdefs[i].istexture)
      {
         // different episode ?
         if(R_CheckTextureNumForName(animdefs[i].startname) == -1)
            continue;
         
         lastanim->picnum = R_TextureNumForName(animdefs[i].endname);
         lastanim->basepic = R_TextureNumForName(animdefs[i].startname);
      }
      else
      {
         if((W_CheckNumForName)(animdefs[i].startname, ns_flats) == -1)  // killough 4/17/98
            continue;
         
         lastanim->picnum = R_FlatNumForName (animdefs[i].endname);
         lastanim->basepic = R_FlatNumForName (animdefs[i].startname);
      }

      lastanim->istexture = animdefs[i].istexture;
      lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;
      lastanim->speed = LONG(animdefs[i].speed); // killough 5/5/98: add LONG()

      // sf: include support for swirly water hack
      if(lastanim->speed < 65536 && lastanim->numpics != 1)
      {
         if(lastanim->numpics < 2)
            I_Error ("P_InitPicAnims: bad cycle from %s to %s",
                     animdefs[i].startname,
                     animdefs[i].endname);
      }

      lastanim++;
   }

   Z_ChangeTag (animdefs,PU_CACHE); //jff 3/23/98 allow table to be freed
}

///////////////////////////////////////////////////////////////
//
// Linedef and Sector Special Implementation Utility Functions
//
///////////////////////////////////////////////////////////////

//
// getSide()
//
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
// Note: if side=1 is specified, it must exist or results undefined
//

side_t *getSide(int currentSector, int line, int side)
{
   return &sides[sectors[currentSector].lines[line]->sidenum[side]];
}

//
// getSector()
//
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
// Note: if side=1 is specified, it must exist or results undefined
//

sector_t *getSector(int currentSector, int line, int side)
{
   return 
     sides[sectors[currentSector].lines[line]->sidenum[side]].sector;
}

//
// twoSided()
//
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
// modified to return actual two-sidedness rather than presence
// of 2S flag unless compatibility optioned
//
// killough 11/98: reformatted

int twoSided(int sector, int line)
{
   //jff 1/26/98 return what is actually needed, whether the line
   //has two sidedefs, rather than whether the 2S flag is set
   
   return 
      comp[comp_model] ? 
         sectors[sector].lines[line]->flags & ML_TWOSIDED :
         sectors[sector].lines[line]->sidenum[1] != -1;
}

//
// getNextSector()
//
// Return sector_t * of sector next to current across line.
//
// Note: returns NULL if not two-sided line, or both sides refer to sector
//
// killough 11/98: reformatted

sector_t *getNextSector(line_t *line, sector_t *sec)
{
   //jff 1/26/98 check unneeded since line->backsector already
   //returns NULL if the line is not two sided, and does so from
   //the actual two-sidedness of the line, rather than its 2S flag
   //
   //jff 5/3/98 don't retn sec unless compatibility
   // fixes an intra-sector line breaking functions
   // like floor->highest floor

   return comp[comp_model] && !(line->flags & ML_TWOSIDED) ? NULL :
     line->frontsector == sec ? comp[comp_model] || line->backsector != sec ?
     line->backsector : NULL : line->frontsector;
}

//
// P_FindLowestFloorSurrounding()
//
// Returns the fixed point value of the lowest floor height
// in the sector passed or its surrounding sectors.
//
// killough 11/98: reformatted

fixed_t P_FindLowestFloorSurrounding(sector_t* sec)
{
   fixed_t floor = sec->floorheight;
   const sector_t *other;
   int i;
   
   for(i = 0; i < sec->linecount; i++)
   {
      if((other = getNextSector(sec->lines[i], sec)) &&
         other->floorheight < floor)
         floor = other->floorheight;
   }
   
   return floor;
}

//
// P_FindHighestFloorSurrounding()
//
// Passed a sector, returns the fixed point value of the largest
// floor height in the surrounding sectors, not including that passed
//
// NOTE: if no surrounding sector exists -32000*FRACUINT is returned
//       if compatibility then -500*FRACUNIT is the smallest return possible
//
// killough 11/98: reformatted

fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
   fixed_t floor = -500*FRACUNIT;
   const sector_t *other;
   int i;

   //jff 1/26/98 Fix initial value for floor to not act differently
   //in sections of wad that are below -500 units
   
   if(!comp[comp_model])          //jff 3/12/98 avoid ovf
      floor = -32000*FRACUNIT;      // in height calculations

   for(i=0 ;i < sec->linecount ; i++)
   {
      if((other = getNextSector(sec->lines[i],sec)) &&
         other->floorheight > floor)
         floor = other->floorheight;
   }
   
   return floor;
}

//
// P_FindNextHighestFloor()
//
// Passed a sector and a floor height, returns the fixed point value
// of the smallest floor height in a surrounding sector larger than
// the floor height passed. If no such height exists the floorheight
// passed is returned.
//
// Rewritten by Lee Killough to avoid fixed array and to be faster
//

fixed_t P_FindNextHighestFloor(sector_t *sec, int currentheight)
{
   sector_t *other;
   int i;
   
   for(i=0; i < sec->linecount; i++)
   {
      if((other = getNextSector(sec->lines[i],sec)) &&
         other->floorheight > currentheight)
      {
         int height = other->floorheight;
         while (++i < sec->linecount)
         {
            if((other = getNextSector(sec->lines[i],sec)) &&
               other->floorheight < height &&
               other->floorheight > currentheight)
               height = other->floorheight;
         }
         return height;
      }
   }
   return currentheight;
}

//
// P_FindNextLowestFloor()
//
// Passed a sector and a floor height, returns the fixed point value
// of the largest floor height in a surrounding sector smaller than
// the floor height passed. If no such height exists the floorheight
// passed is returned.
//
// jff 02/03/98 Twiddled Lee's P_FindNextHighestFloor to make this

fixed_t P_FindNextLowestFloor(sector_t *sec, int currentheight)
{
   sector_t *other;
   int i;
   
   for(i=0; i < sec->linecount; i++)
   {
      if((other = getNextSector(sec->lines[i],sec)) &&
         other->floorheight < currentheight)
      {
         int height = other->floorheight;
         while (++i < sec->linecount)
         {
            if((other = getNextSector(sec->lines[i],sec)) &&
               other->floorheight > height &&
               other->floorheight < currentheight)
               height = other->floorheight;
         }
         return height;
      }
   }
   return currentheight;
}

//
// P_FindNextLowestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the largest ceiling height in a surrounding sector smaller than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
// jff 02/03/98 Twiddled Lee's P_FindNextHighestFloor to make this

fixed_t P_FindNextLowestCeiling(sector_t *sec, int currentheight)
{
   sector_t *other;
   int i;
   
   for(i=0 ;i < sec->linecount ; i++)
   {
      if((other = getNextSector(sec->lines[i],sec)) &&
         other->ceilingheight < currentheight)
      {
         int height = other->ceilingheight;
         while (++i < sec->linecount)
         {
            if((other = getNextSector(sec->lines[i],sec)) &&
               other->ceilingheight > height &&
               other->ceilingheight < currentheight)
               height = other->ceilingheight;
         }
        return height;
      }
   }
   return currentheight;
}

//
// P_FindNextHighestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the smallest ceiling height in a surrounding sector larger than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
// jff 02/03/98 Twiddled Lee's P_FindNextHighestFloor to make this

fixed_t P_FindNextHighestCeiling(sector_t *sec, int currentheight)
{
   sector_t *other;
   int i;
   
   for(i=0; i < sec->linecount; i++)
   {
      if((other = getNextSector(sec->lines[i],sec)) &&
         other->ceilingheight > currentheight)
      {
         int height = other->ceilingheight;
         while (++i < sec->linecount)
         {
            if((other = getNextSector(sec->lines[i],sec)) &&
               other->ceilingheight < height &&
               other->ceilingheight > currentheight)
               height = other->ceilingheight;
         }
         return height;
      }
   }
   return currentheight;
}

//
// P_FindLowestCeilingSurrounding()
//
// Passed a sector, returns the fixed point value of the smallest
// ceiling height in the surrounding sectors, not including that passed
//
// NOTE: if no surrounding sector exists 32000*FRACUINT is returned
//       but if compatibility then MAXINT is the return
//
// killough 11/98: reformatted

fixed_t P_FindLowestCeilingSurrounding(sector_t* sec)
{
   const sector_t *other;
   fixed_t height = D_MAXINT;
   int i;

   if(!comp[comp_model])
      height = 32000*FRACUNIT; //jff 3/12/98 avoid ovf in

   // height calculations
   for(i=0; i < sec->linecount; i++)
      if((other = getNextSector(sec->lines[i],sec)) &&
         other->ceilingheight < height)
         height = other->ceilingheight;

   return height;
}

//
// P_FindHighestCeilingSurrounding()
//
// Passed a sector, returns the fixed point value of the largest
// ceiling height in the surrounding sectors, not including that passed
//
// NOTE: if no surrounding sector exists -32000*FRACUINT is returned
//       but if compatibility then 0 is the smallest return possible
//
// killough 11/98: reformatted

fixed_t P_FindHighestCeilingSurrounding(sector_t* sec)
{
   const sector_t *other;
   fixed_t height = 0;
   int i;

   //jff 1/26/98 Fix initial value for floor to not act differently
   //in sections of wad that are below 0 units

   if(!comp[comp_model])
      height = -32000*FRACUNIT; //jff 3/12/98 avoid ovf in
   
   // height calculations
   for(i=0; i < sec->linecount; i++)
      if((other = getNextSector(sec->lines[i],sec)) &&
         other->ceilingheight > height)
         height = other->ceilingheight;
      
   return height;
}

//
// P_FindShortestTextureAround()
//
// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
// Note: If no lower texture exists 32000*FRACUNIT is returned.
//       but if compatibility then MAXINT is returned
//
// jff 02/03/98 Add routine to find shortest lower texture
//
// killough 11/98: reformatted

fixed_t P_FindShortestTextureAround(int secnum)
{
   const sector_t *sec = &sectors[secnum];
   int i, minsize = D_MAXINT;
   
   if(!comp[comp_model])
      minsize = 32000<<FRACBITS; //jff 3/13/98 prevent overflow in height calcs
   
   for(i = 0; i < sec->linecount; i++)
   {
      if(twoSided(secnum, i))
      {
         const side_t *side;
         if((side = getSide(secnum,i,0))->bottomtexture >= 0 &&
            textureheight[side->bottomtexture] < minsize)
            minsize = textureheight[side->bottomtexture];
         if((side = getSide(secnum,i,1))->bottomtexture >= 0 &&
            textureheight[side->bottomtexture] < minsize)
            minsize = textureheight[side->bottomtexture];
      }
   }
   
   return minsize;
}

//
// P_FindShortestUpperAround()
//
// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.
//
// Note: If no upper texture exists 32000*FRACUNIT is returned.
//       but if compatibility then MAXINT is returned
//
// jff 03/20/98 Add routine to find shortest upper texture
//
// killough 11/98: reformatted

fixed_t P_FindShortestUpperAround(int secnum)
{
   const sector_t *sec = &sectors[secnum];
   int i, minsize = D_MAXINT;
   
   if (!comp[comp_model])
      minsize = 32000<<FRACBITS; //jff 3/13/98 prevent overflow

   // in height calcs
   for(i = 0; i < sec->linecount; i++)
   {
      if(twoSided(secnum, i))
      {
         const side_t *side;
         if((side = getSide(secnum,i,0))->toptexture >= 0)
            if(textureheight[side->toptexture] < minsize)
               minsize = textureheight[side->toptexture];
         if((side = getSide(secnum,i,1))->toptexture >= 0)
            if(textureheight[side->toptexture] < minsize)
               minsize = textureheight[side->toptexture];
      }
   }

   return minsize;
}

//
// P_FindModelFloorSector()
//
// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
// jff 02/03/98 Add routine to find numeric model floor
//  around a sector specified by sector number
// jff 3/14/98 change first parameter to plain height to allow call
//  from routine not using floormove_t
//
// killough 11/98: reformatted
// 

sector_t *P_FindModelFloorSector(fixed_t floordestheight, int secnum)
{
   sector_t *sec = &sectors[secnum]; //jff 3/2/98 woops! better do this

   //jff 5/23/98 don't disturb sec->linecount while searching
   // but allow early exit in old demos

   int i, linecount = sec->linecount;
   
   for(i = 0; i < (demo_compatibility && sec->linecount < linecount ?
      sec->linecount : linecount); i++)
      if (twoSided(secnum, i) &&
         (sec = getSector(secnum, i,
         getSide(secnum,i,0)->sector-sectors == secnum))->
         floorheight == floordestheight)
         return sec;
      
   return NULL;
}

//
// P_FindModelCeilingSector()
//
// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
// jff 02/03/98 Add routine to find numeric model ceiling
//  around a sector specified by sector number
//  used only from generalized ceiling types
// jff 3/14/98 change first parameter to plain height to allow call
//  from routine not using ceiling_t
//
// killough 11/98: reformatted
// haleyjd 09/23/02: reformatted again

sector_t *P_FindModelCeilingSector(fixed_t ceildestheight, int secnum)
{
   sector_t *sec = &sectors[secnum]; //jff 3/2/98 woops! better do this

   //jff 5/23/98 don't disturb sec->linecount while searching
   // but allow early exit in old demos

   int i, linecount = sec->linecount;

   for(i = 0; i < (demo_compatibility && sec->linecount<linecount?
                   sec->linecount : linecount); i++)
      if(twoSided(secnum, i) &&
         (sec = getSector(secnum, i,
                          getSide(secnum,i,0)->sector-sectors == secnum))->
          ceilingheight == ceildestheight)
         return sec;

   return NULL;
}

//
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//

// Find the next sector with the same tag as a linedef.
// Rewritten by Lee Killough to use chained hashing to improve speed

int P_FindSectorFromLineTag(const line_t *line, int start)
{
   start = 
      (start >= 0 ? sectors[start].nexttag :
       sectors[(unsigned)line->tag % (unsigned)numsectors].firsttag);
  
   while(start >= 0 && sectors[start].tag != line->tag)
      start = sectors[start].nexttag;
   
   return start;
}

// killough 4/16/98: Same thing, only for linedefs

int P_FindLineFromLineTag(const line_t *line, int start)
{
   start = 
      (start >= 0 ? lines[start].nexttag :
       lines[(unsigned)line->tag % (unsigned)numlines].firsttag);
  
   while(start >= 0 && lines[start].tag != line->tag)
      start = lines[start].nexttag;
   
   return start;
}

// sf: same thing but from just a number

int P_FindSectorFromTag(const int tag, int start)
{
   start = 
      (start >= 0 ? sectors[start].nexttag :
       sectors[(unsigned)tag % (unsigned)numsectors].firsttag);
  
   while(start >= 0 && sectors[start].tag != tag)
      start = sectors[start].nexttag;
  
   return start;
}


// Hash the sector tags across the sectors and linedefs.
static void P_InitTagLists(void)
{
   register int i;

   for(i=numsectors; --i>=0; )      // Initially make all slots empty.
      sectors[i].firsttag = -1;

   for(i=numsectors; --i>=0; )       // Proceed from last to first sector
   {                                 // so that lower sectors appear first
      int j = (unsigned)sectors[i].tag % (unsigned)numsectors; // Hash func
      sectors[i].nexttag = sectors[j].firsttag;   // Prepend sector to chain
      sectors[j].firsttag = i;
   }

  // killough 4/17/98: same thing, only for linedefs

   for(i=numlines; --i>=0; )        // Initially make all slots empty.
      lines[i].firsttag = -1;

   for(i=numlines; --i>=0; )       // Proceed from last to first linedef
   {                               // so that lower linedefs appear first
      int j = (unsigned)lines[i].tag % (unsigned)numlines; // Hash func
      lines[i].nexttag = lines[j].firsttag;   // Prepend linedef to chain
      lines[j].firsttag = i;
   }
}

//
// P_FindMinSurroundingLight()
//
// Passed a sector and a light level, returns the smallest light level
// in a surrounding sector less than that passed. If no smaller light
// level exists, the light level passed is returned.
//
// killough 11/98: reformatted

int P_FindMinSurroundingLight(sector_t *sector, int min)
{
   const sector_t *check;
   int i;

   for(i=0; i < sector->linecount; i++)
   {
      if((check = getNextSector(sector->lines[i], sector)) &&
         check->lightlevel < min)
         min = check->lightlevel;
   }

   return min;
}

//
// P_CanUnlockGenDoor()
//
// Passed a generalized locked door linedef and a player, returns whether
// the player has the keys necessary to unlock that door.
//
// Note: The linedef passed MUST be a generalized locked door type
//       or results are undefined.
//
// jff 02/05/98 routine added to test for unlockability of
//  generalized locked doors
//
// killough 11/98: reformatted
//
// haleyjd 08/22/00: fixed bug found by fraggle
//

boolean P_CanUnlockGenDoor(line_t *line, player_t *player)
{
   // does this line special distinguish between skulls and keys?
   int skulliscard = (line->special & LockedNKeys)>>LockedNKeysShift;

   // determine for each case of lock type if player's keys are 
   // adequate
   switch((line->special & LockedKey)>>LockedKeyShift)
   {
   case AnyKey:
      if(!player->cards[it_redcard] &&
         !player->cards[it_redskull] &&
         !player->cards[it_bluecard] &&
         !player->cards[it_blueskull] &&
         !player->cards[it_yellowcard] &&
         !player->cards[it_yellowskull])
      {
         player_printf(player, s_PD_ANY);
         S_StartSound(player->mo,sfx_oof);  // killough 3/20/98
         return false;
      }
      break;
   case RCard:
      if(!player->cards[it_redcard] &&
         (!skulliscard || !player->cards[it_redskull]))
      {
         if(gameModeInfo->flags & GIF_HERETIC)
            player_printf(player, s_HPD_GREENK);
         else
            player_printf(player, skulliscard? s_PD_REDK : s_PD_REDC);
         
         S_StartSound(player->mo,sfx_oof);  // killough 3/20/98
         return false;
      }
      break;
   case BCard:
      if(!player->cards[it_bluecard] &&
         (!skulliscard || !player->cards[it_blueskull]))
      {
         player_printf(player, skulliscard ? s_PD_BLUEK : s_PD_BLUEC);
         S_StartSound(player->mo,sfx_oof);  // killough 3/20/98
         return false;
      }
      break;
   case YCard:
      if(!player->cards[it_yellowcard] &&
         (!skulliscard || !player->cards[it_yellowskull]))
      {
         player_printf(player, skulliscard? s_PD_YELLOWK : s_PD_YELLOWC);
         S_StartSound(player->mo,sfx_oof);  // killough 3/20/98
         return false;
      }
      break;
   case RSkull:
      if(!player->cards[it_redskull] &&
         (!skulliscard || !player->cards[it_redcard]))
      {
         if(gameModeInfo->flags & GIF_HERETIC)
            player_printf(player, s_HPD_GREENK);
         else
            player_printf(player, skulliscard? s_PD_REDK : s_PD_REDS);
         S_StartSound(player->mo,sfx_oof);             // killough 3/20/98
         return false;
      }
      break;
   case BSkull:
      if(!player->cards[it_blueskull] &&
         (!skulliscard || !player->cards[it_bluecard]))
      {
         player_printf(player, skulliscard? s_PD_BLUEK : s_PD_BLUES);
         S_StartSound(player->mo,sfx_oof);             // killough 3/20/98
         return false;
      }
      break;
   case YSkull:
      if(!player->cards[it_yellowskull] &&
         (!skulliscard || !player->cards[it_yellowcard]))
      {
         player_printf(player, skulliscard? s_PD_YELLOWK : s_PD_YELLOWS);
         S_StartSound(player->mo,sfx_oof);             // killough 3/20/98
         return false;
      }
      break;
   case AllKeys:
      if(!skulliscard &&
         (!player->cards[it_redcard] ||
          !player->cards[it_redskull] ||
          !player->cards[it_bluecard] ||
          !player->cards[it_blueskull] ||
          !player->cards[it_yellowcard] ||
          !player->cards[it_yellowskull]))
      {
         player_printf(player, s_PD_ALL6);
         S_StartSound(player->mo,sfx_oof);             // killough 3/20/98
         return false;
      }
      // haleyjd: removed extra ! from player->cards[it_yellowskull]
      //          allowed door to be opened without yellow keys
      //          06/30/01: optioned on MBF demo comp. (v2.03)
      if(skulliscard &&
         (!(player->cards[it_redcard] | player->cards[it_redskull]) ||
          !(player->cards[it_bluecard] | player->cards[it_blueskull]) ||
          !(player->cards[it_yellowcard] | 
          ((demo_version == 203) ? !player->cards[it_yellowskull] : 
                                    player->cards[it_yellowskull]))))
      {
         player_printf(player, s_PD_ALL3);
         S_StartSound(player->mo,sfx_oof);             // killough 3/20/98
         return false;
      }
      break;
   }
   return true;
}

//
// P_SectorActive()
//
// Passed a linedef special class (floor, ceiling, lighting) and a sector
// returns whether the sector is already busy with a linedef special of the
// same class. If old demo compatibility true, all linedef special classes
// are the same.
//
// jff 2/23/98 added to prevent old demos from
//  succeeding in starting multiple specials on one sector
//
// killough 11/98: reformatted

int P_SectorActive(special_e t,sector_t *sec)
{
   return demo_compatibility ?  // return whether any thinker is active
     sec->floordata || sec->ceilingdata || sec->lightingdata :
     t == floor_special ? !!sec->floordata :        // return whether
     t == ceiling_special ? !!sec->ceilingdata :    // thinker of same
     t == lighting_special ? !!sec->lightingdata :  // type is active
     1; // don't know which special, must be active, shouldn't be here
}

//
// P_CheckTag()
//
// Passed a line, returns true if the tag is non-zero or the line special
// allows no tag without harm. If compatibility, all linedef specials are
// allowed to have zero tag.
//
// Note: Only line specials activated by walkover, pushing, or shooting are
//       checked by this routine.
//
// jff 2/27/98 Added to check for zero tag allowed for regular special types

int P_CheckTag(line_t *line)
{
   // killough 11/98: compatibility option:
   
   if(comp[comp_zerotags] || line->tag)
      return 1;

   switch (line->special)
   {
   case 1:   // Manual door specials
   case 26:
   case 27:
   case 28:
   case 31:
   case 32:
   case 33:
   case 34:
   case 117:
   case 118:
   case 139:  // Lighting specials
   case 170:
   case 79:
   case 35:
   case 138:
   case 171:
   case 81:
   case 13:
   case 192:
   case 169:
   case 80:
   case 12:
   case 194:
   case 173:
   case 157:
   case 104:
   case 193:
   case 172:
   case 156:
   case 17:
   case 195:  // Thing teleporters
   case 174:
   case 97:
   case 39:
   case 126:
   case 125:
   case 210:
   case 209:
   case 208:
   case 207:
   case 11:  // Exits
   case 52:
   case 197:
   case 51:
   case 124:
   case 198:
   case 48:  // Scrolling walls
   case 85:
   case 273:
   case 274:   // W1
   case 275:
   case 276:   // SR
   case 277:   // S1
   case 278:   // GR
   case 279:   // G1
   case 280:   // WR -- haleyjd
      return 1;
   }

  return 0;
}

//
// P_IsSecret()
//
// Passed a sector, returns if the sector secret type is still active, i.e.
// secret type is set and the secret has not yet been obtained.
//
// jff 3/14/98 added to simplify checks for whether sector is secret
//  in automap and other places
//

boolean P_IsSecret(sector_t *sec)
{
   return (sec->special == 9 || sec->special & SECRET_MASK);
}

//
// P_WasSecret()
//
// Passed a sector, returns if the sector secret type is was active, i.e.
// secret type was set and the secret has been obtained already.
//
// jff 3/14/98 added to simplify checks for whether sector is secret
//  in automap and other places
//

boolean P_WasSecret(sector_t *sec)
{
   return (sec->oldspecial == 9 || sec->oldspecial & SECRET_MASK);
}

//////////////////////////////////////////////////////////////////////////
//
// Events
//
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//
/////////////////////////////////////////////////////////////////////////

//
// P_CrossSpecialLine - Walkover Trigger Dispatcher
//
// Called every time a thing origin is about
//  to cross a line with a non 0 special, whether a walkover type or not.
//
// jff 02/12/98 all W1 lines were fixed to check the result from the EV_
//  function before clearing the special. This avoids losing the function
//  of the line, should the sector already be active when the line is
//  crossed. Change is qualified by demo_compatibility.
//
// killough 11/98: change linenum parameter to a line_t pointer

void P_CrossSpecialLine(line_t *line, int side, mobj_t *thing)
{
   int ok;
   
   //  Things that should never trigger lines
   if(!thing->player)
   { 
      // haleyjd: changed to check against MF2_NOCROSS flag instead 
      // of switching on type
      if(thing->flags2 & MF2_NOCROSS)
         return;
   }
    
   //jff 02/04/98 add check here for generalized lindef types
   if(!demo_compatibility) // generalized types not recognized if old demo
   {
      // pointer to line function is NULL by default, set non-null if
      // line special is walkover generalized linedef type
      int (*linefunc)(line_t *)=NULL;

      // check each range of generalized linedefs
      if((unsigned)line->special >= GenFloorBase)
      {
         if(!thing->player)
         {
            if((line->special & FloorChange) || 
               !(line->special & FloorModel))
               return;     // FloorModel is "Allow Monsters" if FloorChange is 0
         }
         if(!line->tag) //jff 2/27/98 all walk generalized types require tag
            return;
         linefunc = EV_DoGenFloor;
      }
      else if((unsigned)line->special >= GenCeilingBase)
      {
         if(!thing->player)
         {
            if((line->special & CeilingChange) || !(line->special & CeilingModel))
               return;     // CeilingModel is "Allow Monsters" if CeilingChange is 0
         }
         if(!line->tag) //jff 2/27/98 all walk generalized types require tag
            return;
         linefunc = EV_DoGenCeiling;
      }
      else if((unsigned)line->special >= GenDoorBase)
      {
         if (!thing->player)
         {
            if(!(line->special & DoorMonster))
               return;                    // monsters disallowed from this door
            if(line->flags & ML_SECRET) // they can't open secret doors either
               return;
         }
         if(!line->tag) //3/2/98 move outside the monster check
            return;
         genDoorThing = thing;
         linefunc = EV_DoGenDoor;
      }
      else if((unsigned)line->special >= GenLockedBase)
      {
         if(!thing->player)
            return;                     // monsters disallowed from unlocking doors
         if(((line->special&TriggerType)==WalkOnce) || 
            ((line->special&TriggerType)==WalkMany))
         { //jff 4/1/98 check for being a walk type before reporting door type
            if(!P_CanUnlockGenDoor(line,thing->player))
               return;
         }
         else
            return;
         genDoorThing = thing;
         linefunc = EV_DoGenLockedDoor;
      }
      else if((unsigned)line->special >= GenLiftBase)
      {
         if(!thing->player)
         {
            if(!(line->special & LiftMonster))
               return; // monsters disallowed
         }
         if(!line->tag) //jff 2/27/98 all walk generalized types require tag
            return;
         linefunc = EV_DoGenLift;
      }
      else if((unsigned)line->special >= GenStairsBase)
      {
         if(!thing->player)
         {
            if(!(line->special & StairMonster))
               return; // monsters disallowed
         }
         if(!line->tag) //jff 2/27/98 all walk generalized types require tag
            return;
         linefunc = EV_DoGenStairs;
      }

      if(linefunc) // if it was a valid generalized type
      {
         switch((line->special & TriggerType) >> TriggerTypeShift)
         {
         case WalkOnce:
            if(linefunc(line))
               line->special = 0;  // clear special if a walk once type
            return;
         case WalkMany:
            linefunc(line);
            return;
         default:              // if not a walk type, do nothing here
            return;
         }
      }
   }

   if(!thing->player)
   {
      ok = 0;
      switch(line->special)
      {
      case 39:      // teleport trigger
      case 97:      // teleport retrigger
      case 125:     // teleport monsteronly trigger
      case 126:     // teleport monsteronly retrigger
      case 4:       // raise door
      case 10:      // plat down-wait-up-stay trigger
      case 88:      // plat down-wait-up-stay retrigger
         //jff 3/5/98 add ability of monsters etc. to use teleporters
      case 208:     //silent thing teleporters
      case 207:
      case 243:     //silent line-line teleporter
      case 244:     //jff 3/6/98 make fit within DCK's 256 linedef types
      case 262:     //jff 4/14/98 add monster only
      case 263:     //jff 4/14/98 silent thing,line,line rev types
      case 264:     //jff 4/14/98 plus player/monster silent line
      case 265:     //            reversed types
      case 266:
      case 267:
      case 268:
      case 269:
         ok = 1;
         break;
      }
      if(!ok)
         return;
   }

   if(!P_CheckTag(line))  //jff 2/27/98 disallow zero tag on some types
      return;

   // Dispatch on the line special value to the line's action routine
   // If a once only function, and successful, clear the line special

   switch(line->special)
   {
      // Regular walk once triggers

   case 2:
      // Open Door
      if(EV_DoDoor(line,doorOpen) || demo_compatibility)
         line->special = 0;
      break;

   case 3:
      // Close Door
      if(EV_DoDoor(line,doorClose) || demo_compatibility)
         line->special = 0;
      break;

   case 4:
      // Raise Door
      if(EV_DoDoor(line,doorNormal) || demo_compatibility)
         line->special = 0;
      break;

   case 5:
      // Raise Floor
      if(EV_DoFloor(line,raiseFloor) || demo_compatibility)
         line->special = 0;
      break;

   case 6:
      // Fast Ceiling Crush & Raise
      if(EV_DoCeiling(line,fastCrushAndRaise) || demo_compatibility)
         line->special = 0;
      break;

   case 8:
      // Build Stairs
      if(EV_BuildStairs(line,build8) || demo_compatibility)
         line->special = 0;
      break;

   case 10:
      // PlatDownWaitUp
      if(EV_DoPlat(line,downWaitUpStay,0) || demo_compatibility)
         line->special = 0;
      break;

   case 12:
      // Light Turn On - brightest near
      if(EV_LightTurnOn(line,0) || demo_compatibility)
         line->special = 0;
      break;

   case 13:
      // Light Turn On 255
      if(EV_LightTurnOn(line,255) || demo_compatibility)
         line->special = 0;
      break;

   case 16:
      // Close Door 30
      if(EV_DoDoor(line,close30ThenOpen) || demo_compatibility)
         line->special = 0;
      break;

   case 17:
      // Start Light Strobing
      if(EV_StartLightStrobing(line) || demo_compatibility)
         line->special = 0;
      break;

   case 19:
      // Lower Floor
      if(EV_DoFloor(line,lowerFloor) || demo_compatibility)
         line->special = 0;
      break;

   case 22:
      // Raise floor to nearest height and change texture
      if(EV_DoPlat(line,raiseToNearestAndChange,0) || demo_compatibility)
         line->special = 0;
      break;

   case 25:
      // Ceiling Crush and Raise
      if(EV_DoCeiling(line,crushAndRaise) || demo_compatibility)
         line->special = 0;
      break;

   case 30:
      // Raise floor to shortest texture height
      //  on either side of lines.
      if(EV_DoFloor(line,raiseToTexture) || demo_compatibility)
         line->special = 0;
      break;

   case 35:
      // Lights Very Dark
      if(EV_LightTurnOn(line,35) || demo_compatibility)
         line->special = 0;
      break;

   case 36:
      // Lower Floor (TURBO)
      if(EV_DoFloor(line,turboLower) || demo_compatibility)
         line->special = 0;
      break;

   case 37:
      // LowerAndChange
      if(EV_DoFloor(line,lowerAndChange) || demo_compatibility)
         line->special = 0;
      break;

   case 38:
      // Lower Floor To Lowest
      if(EV_DoFloor(line, lowerFloorToLowest) || demo_compatibility)
         line->special = 0;
      break;

   case 39:
      // TELEPORT! //jff 02/09/98 fix using up with wrong side crossing
      if(EV_Teleport(line, side, thing) || demo_compatibility)
         line->special = 0;
      break;

   case 40:
      // RaiseCeilingLowerFloor
      if(demo_compatibility)
      {
         EV_DoCeiling( line, raiseToHighest );
         EV_DoFloor( line, lowerFloorToLowest ); //jff 02/12/98 doesn't work
         line->special = 0;
      }
      else if(EV_DoCeiling(line, raiseToHighest))
         line->special = 0;
      break;

   case 44:
      // Ceiling Crush
      if(EV_DoCeiling(line, lowerAndCrush) || demo_compatibility)
         line->special = 0;
      break;

   case 52:
      // EXIT!
      // killough 10/98: prevent zombies from exiting levels
      if(!(thing->player && thing->player->health <= 0 && !comp[comp_zombie]))
         G_ExitLevel();
      break;

   case 53:
      // Perpetual Platform Raise
      if(EV_DoPlat(line,perpetualRaise,0) || demo_compatibility)
         line->special = 0;
      break;

   case 54:
      // Platform Stop
      if(EV_StopPlat(line) || demo_compatibility)
         line->special = 0;
      break;

   case 56:
      // Raise Floor Crush
      if(EV_DoFloor(line,raiseFloorCrush) || demo_compatibility)
         line->special = 0;
      break;

   case 57:
      // Ceiling Crush Stop
      if(EV_CeilingCrushStop(line) || demo_compatibility)
         line->special = 0;
      break;

   case 58:
      // Raise Floor 24
      if(EV_DoFloor(line,raiseFloor24) || demo_compatibility)
         line->special = 0;
      break;
      
   case 59:
      // Raise Floor 24 And Change
      if(EV_DoFloor(line,raiseFloor24AndChange) || demo_compatibility)
         line->special = 0;
      break;
      
   case 100:
      // Build Stairs Turbo 16
      if(EV_BuildStairs(line,turbo16) || demo_compatibility)
         line->special = 0;
      break;

   case 104:
      // Turn lights off in sector(tag)
      if(EV_TurnTagLightsOff(line) || demo_compatibility)
         line->special = 0;
      break;

   case 108:
      // Blazing Door Raise (faster than TURBO!)
      if(EV_DoDoor(line,blazeRaise) || demo_compatibility)
         line->special = 0;
      break;
      
   case 109:
      // Blazing Door Open (faster than TURBO!)
      if(EV_DoDoor (line,blazeOpen) || demo_compatibility)
         line->special = 0;
      break;
      
   case 110:
      // Blazing Door Close (faster than TURBO!)
      if(EV_DoDoor (line,blazeClose) || demo_compatibility)
         line->special = 0;
      break;
      
   case 119:
      // Raise floor to nearest surr. floor
      if(EV_DoFloor(line,raiseFloorToNearest) || demo_compatibility)
         line->special = 0;
      break;

   case 121:
      // Blazing PlatDownWaitUpStay
      if(EV_DoPlat(line,blazeDWUS,0) || demo_compatibility)
         line->special = 0;
      break;
      
   case 124:
      // Secret EXIT
      // killough 10/98: prevent zombies from exiting levels
      if(!(thing->player && thing->player->health <= 0 && !comp[comp_zombie]))
         G_SecretExitLevel();
      break;

   case 125:
      // TELEPORT MonsterONLY
      if(!thing->player &&
         (EV_Teleport(line, side, thing) || demo_compatibility))
         line->special = 0;
      break;
      
   case 130:
      // Raise Floor Turbo
      if(EV_DoFloor(line,raiseFloorTurbo) || demo_compatibility)
         line->special = 0;
      break;
      
   case 141:
      // Silent Ceiling Crush & Raise
      if(EV_DoCeiling(line,silentCrushAndRaise) || demo_compatibility)
         line->special = 0;
      break;

      // Regular walk many retriggerable

   case 72:
      // Ceiling Crush
      EV_DoCeiling( line, lowerAndCrush );
      break;
      
   case 73:
      // Ceiling Crush and Raise
      EV_DoCeiling(line,crushAndRaise);
      break;
      
   case 74:
      // Ceiling Crush Stop
      EV_CeilingCrushStop(line);
      break;
      
   case 75:
      // Close Door
      EV_DoDoor(line,doorClose);
      break;
      
   case 76:
      // Close Door 30
      EV_DoDoor(line,close30ThenOpen);
      break;
      
   case 77:
      // Fast Ceiling Crush & Raise
      EV_DoCeiling(line,fastCrushAndRaise);
      break;
      
   case 79:
      // Lights Very Dark
      EV_LightTurnOn(line,35);
      break;
      
   case 80:
      // Light Turn On - brightest near
      EV_LightTurnOn(line,0);
      break;
      
   case 81:
      // Light Turn On 255
      EV_LightTurnOn(line,255);
      break;
      
   case 82:
      // Lower Floor To Lowest
      EV_DoFloor( line, lowerFloorToLowest );
      break;
      
   case 83:
      // Lower Floor
      EV_DoFloor(line,lowerFloor);
      break;
      
   case 84:
      // LowerAndChange
      EV_DoFloor(line,lowerAndChange);
      break;
      
   case 86:
      // Open Door
      EV_DoDoor(line,doorOpen);
      break;
      
   case 87:
      // Perpetual Platform Raise
      EV_DoPlat(line,perpetualRaise,0);
      break;
      
   case 88:
      // PlatDownWaitUp
      EV_DoPlat(line,downWaitUpStay,0);
      break;
      
   case 89:
      // Platform Stop
      EV_StopPlat(line);
      break;
      
   case 90:
      // Raise Door
      EV_DoDoor(line,doorNormal);
      break;
      
   case 91:
      // Raise Floor
      EV_DoFloor(line,raiseFloor);
      break;
      
   case 92:
      // Raise Floor 24
      EV_DoFloor(line,raiseFloor24);
      break;
      
   case 93:
      // Raise Floor 24 And Change
      EV_DoFloor(line,raiseFloor24AndChange);
      break;
      
   case 94:
      // Raise Floor Crush
      EV_DoFloor(line,raiseFloorCrush);
      break;
      
   case 95:
      // Raise floor to nearest height
      // and change texture.
      EV_DoPlat(line,raiseToNearestAndChange,0);
      break;
      
   case 96:
      // Raise floor to shortest texture height
      // on either side of lines.
      EV_DoFloor(line,raiseToTexture);
      break;
      
   case 97:
      // TELEPORT!
      EV_Teleport( line, side, thing );
      break;
      
   case 98:
      // Lower Floor (TURBO)
      EV_DoFloor(line,turboLower);
      break;
      
   case 105:
      // Blazing Door Raise (faster than TURBO!)
      EV_DoDoor(line,blazeRaise);
      break;
      
   case 106:
      // Blazing Door Open (faster than TURBO!)
      EV_DoDoor(line,blazeOpen);
      break;
      
   case 107:
      // Blazing Door Close (faster than TURBO!)
      EV_DoDoor(line,blazeClose);
      break;
      
   case 120:
      // Blazing PlatDownWaitUpStay.
      EV_DoPlat(line,blazeDWUS,0);
      break;
      
   case 126:
      // TELEPORT MonsterONLY.
      if(!thing->player)
         EV_Teleport(line, side, thing);
      break;
      
   case 128:
      // Raise To Nearest Floor
      EV_DoFloor(line,raiseFloorToNearest);
      break;
      
   case 129:
      // Raise Floor Turbo
      EV_DoFloor(line,raiseFloorTurbo);
      break;

      // Extended walk triggers
      
      // jff 1/29/98 added new linedef types to fill all functions out so that
      // all have varieties SR, S1, WR, W1
      
      // killough 1/31/98: "factor out" compatibility test, by
      // adding inner switch qualified by compatibility flag.
      // relax test to demo_compatibility
      
      // killough 2/16/98: Fix problems with W1 types being cleared too early

   default:
      if(!demo_compatibility)
      {
         switch (line->special)
         {
            // Extended walk once triggers

         case 142:
            // Raise Floor 512
            // 142 W1  EV_DoFloor(raiseFloor512)
            if(EV_DoFloor(line,raiseFloor512))
               line->special = 0;
            break;

         case 143:
            // Raise Floor 24 and change
            // 143 W1  EV_DoPlat(raiseAndChange,24)
            if(EV_DoPlat(line,raiseAndChange,24))
               line->special = 0;
            break;

         case 144:
            // Raise Floor 32 and change
            // 144 W1  EV_DoPlat(raiseAndChange,32)
            if(EV_DoPlat(line,raiseAndChange,32))
               line->special = 0;
            break;

         case 145:
            // Lower Ceiling to Floor
            // 145 W1  EV_DoCeiling(lowerToFloor)
            if(EV_DoCeiling( line, lowerToFloor ))
               line->special = 0;
            break;

         case 146:
            // Lower Pillar, Raise Donut
            // 146 W1  EV_DoDonut()
            if(EV_DoDonut(line))
               line->special = 0;
            break;

         case 199:
            // Lower ceiling to lowest surrounding ceiling
            // 199 W1 EV_DoCeiling(lowerToLowest)
            if(EV_DoCeiling(line,lowerToLowest))
               line->special = 0;
            break;
            
         case 200:
            // Lower ceiling to highest surrounding floor
            // 200 W1 EV_DoCeiling(lowerToMaxFloor)
            if(EV_DoCeiling(line,lowerToMaxFloor))
               line->special = 0;
            break;
            
         case 207:
            // killough 2/16/98: W1 silent teleporter (normal kind)
            if(EV_SilentTeleport(line, side, thing))
               line->special = 0;
            break;
            
            //jff 3/16/98 renumber 215->153
         case 153: //jff 3/15/98 create texture change no motion type
            // Texture/Type Change Only (Trig)
            // 153 W1 Change Texture/Type Only
            if(EV_DoChange(line,trigChangeOnly))
               line->special = 0;
            break;
            
         case 239: //jff 3/15/98 create texture change no motion type
            // Texture/Type Change Only (Numeric)
            // 239 W1 Change Texture/Type Only
            if(EV_DoChange(line,numChangeOnly))
               line->special = 0;
            break;
            
         case 219:
            // Lower floor to next lower neighbor
            // 219 W1 Lower Floor Next Lower Neighbor
            if(EV_DoFloor(line,lowerFloorToNearest))
               line->special = 0;
            break;
            
         case 227:
            // Raise elevator next floor
            // 227 W1 Raise Elevator next floor
            if(EV_DoElevator(line,elevateUp))
               line->special = 0;
            break;
            
         case 231:
            // Lower elevator next floor
            // 231 W1 Lower Elevator next floor
            if(EV_DoElevator(line,elevateDown))
               line->special = 0;
            break;
            
         case 235:
            // Elevator to current floor
            // 235 W1 Elevator to current floor
            if(EV_DoElevator(line,elevateCurrent))
               line->special = 0;
            break;
            
         case 243: //jff 3/6/98 make fit within DCK's 256 linedef types
            // killough 2/16/98: W1 silent teleporter (linedef-linedef kind)
            if(EV_SilentLineTeleport(line, side, thing, false))
               line->special = 0;
            break;
            
         case 262: //jff 4/14/98 add silent line-line reversed
            if(EV_SilentLineTeleport(line, side, thing, true))
               line->special = 0;
            break;
            
         case 264: //jff 4/14/98 add monster-only silent line-line reversed
            if(!thing->player &&
               EV_SilentLineTeleport(line, side, thing, true))
               line->special = 0;
            break;
            
         case 266: //jff 4/14/98 add monster-only silent line-line
            if(!thing->player &&
               EV_SilentLineTeleport(line, side, thing, false))
               line->special = 0;
            break;
            
         case 268: //jff 4/14/98 add monster-only silent
            if(!thing->player && EV_SilentTeleport(line, side, thing))
               line->special = 0;
            break;

            //jff 1/29/98 end of added W1 linedef types
            
            // Extended walk many retriggerable
            
            //jff 1/29/98 added new linedef types to fill all functions
            //out so that all have varieties SR, S1, WR, W1

         case 147:
            // Raise Floor 512
            // 147 WR  EV_DoFloor(raiseFloor512)
            EV_DoFloor(line,raiseFloor512);
            break;

         case 148:
            // Raise Floor 24 and Change
            // 148 WR  EV_DoPlat(raiseAndChange,24)
            EV_DoPlat(line,raiseAndChange,24);
            break;
            
         case 149:
            // Raise Floor 32 and Change
            // 149 WR  EV_DoPlat(raiseAndChange,32)
            EV_DoPlat(line,raiseAndChange,32);
            break;
            
         case 150:
            // Start slow silent crusher
            // 150 WR  EV_DoCeiling(silentCrushAndRaise)
            EV_DoCeiling(line,silentCrushAndRaise);
            break;
            
         case 151:
            // RaiseCeilingLowerFloor
            // 151 WR  EV_DoCeiling(raiseToHighest),
            //         EV_DoFloor(lowerFloortoLowest)
            EV_DoCeiling(line, raiseToHighest);
            EV_DoFloor(line, lowerFloorToLowest);
            break;
            
         case 152:
            // Lower Ceiling to Floor
            // 152 WR  EV_DoCeiling(lowerToFloor)
            EV_DoCeiling( line, lowerToFloor );
            break;
            
            //jff 3/16/98 renumber 153->256
         case 256:
            // Build stairs, step 8
            // 256 WR EV_BuildStairs(build8)
            EV_BuildStairs(line,build8);
            break;
            
            //jff 3/16/98 renumber 154->257
         case 257:
            // Build stairs, step 16
            // 257 WR EV_BuildStairs(turbo16)
            EV_BuildStairs(line,turbo16);
            break;
            
         case 155:
            // Lower Pillar, Raise Donut
            // 155 WR  EV_DoDonut()
            EV_DoDonut(line);
            break;
            
         case 156:
            // Start lights strobing
            // 156 WR Lights EV_StartLightStrobing()
            EV_StartLightStrobing(line);
            break;
            
         case 157:
            // Lights to dimmest near
            // 157 WR Lights EV_TurnTagLightsOff()
            EV_TurnTagLightsOff(line);
            break;
            
         case 201:
            // Lower ceiling to lowest surrounding ceiling
            // 201 WR EV_DoCeiling(lowerToLowest)
            EV_DoCeiling(line,lowerToLowest);
            break;
            
         case 202:
            // Lower ceiling to highest surrounding floor
            // 202 WR EV_DoCeiling(lowerToMaxFloor)
            EV_DoCeiling(line,lowerToMaxFloor);
            break;
            
         case 208:
            // killough 2/16/98: WR silent teleporter (normal kind)
            EV_SilentTeleport(line, side, thing);
            break;
            
         case 212: //jff 3/14/98 create instant toggle floor type
            // Toggle floor between C and F instantly
            // 212 WR Instant Toggle Floor
            EV_DoPlat(line,toggleUpDn,0);
            break;
            
            //jff 3/16/98 renumber 216->154
         case 154: //jff 3/15/98 create texture change no motion type
            // Texture/Type Change Only (Trigger)
            // 154 WR Change Texture/Type Only
            EV_DoChange(line,trigChangeOnly);
            break;
            
         case 240: //jff 3/15/98 create texture change no motion type
            // Texture/Type Change Only (Numeric)
            // 240 WR Change Texture/Type Only
            EV_DoChange(line,numChangeOnly);
            break;
            
         case 220:
            // Lower floor to next lower neighbor
            // 220 WR Lower Floor Next Lower Neighbor
            EV_DoFloor(line,lowerFloorToNearest);
            break;
            
         case 228:
            // Raise elevator next floor
            // 228 WR Raise Elevator next floor
            EV_DoElevator(line,elevateUp);
            break;
            
         case 232:
            // Lower elevator next floor
            // 232 WR Lower Elevator next floor
            EV_DoElevator(line,elevateDown);
            break;
            
         case 236:
            // Elevator to current floor
            // 236 WR Elevator to current floor
            EV_DoElevator(line,elevateCurrent);
            break;
            
         case 244: //jff 3/6/98 make fit within DCK's 256 linedef types
            // killough 2/16/98: WR silent teleporter (linedef-linedef kind)
            EV_SilentLineTeleport(line, side, thing, false);
            break;
            
         case 263: //jff 4/14/98 add silent line-line reversed
            EV_SilentLineTeleport(line, side, thing, true);
            break;
            
         case 265: //jff 4/14/98 add monster-only silent line-line reversed
            if(!thing->player)
               EV_SilentLineTeleport(line, side, thing, true);
            break;
            
         case 267: //jff 4/14/98 add monster-only silent line-line
            if(!thing->player)
               EV_SilentLineTeleport(line, side, thing, false);
            break;
            
         case 269: //jff 4/14/98 add monster-only silent
            if(!thing->player)
               EV_SilentTeleport(line, side, thing);
            break;
            //jff 1/29/98 end of added WR linedef types
            
            // scripting ld types
            
            // repeatable            
         case 273:  // WR start script 1-way
            if(side)
               break;            
         case 280:  // WR start script
            break;
                        
            // once-only triggers            
         case 275:  // W1 start script 1-way
            if(side)
               break;            
         case 274:  // W1 start script
            line->special = 0;        // clear trigger
            break;
         }
      }
      break;
   }
}

//
// P_ShootSpecialLine - Gun trigger special dispatcher
//
// Called when a thing shoots a special line with bullet, shell, saw, or fist.
//
// jff 02/12/98 all G1 lines were fixed to check the result from the EV_
// function before clearing the special. This avoids losing the function
// of the line, should the sector already be in motion when the line is
// impacted. Change is qualified by demo_compatibility.
//
void P_ShootSpecialLine(mobj_t *thing, line_t *line)
{
   //jff 02/04/98 add check here for generalized linedef
   if(!demo_compatibility)
   {
      // pointer to line function is NULL by default, set non-null if
      // line special is gun triggered generalized linedef type
      int (*linefunc)(line_t *line)=NULL;

      // check each range of generalized linedefs
      if((unsigned)line->special >= GenFloorBase)
      {
         if(!thing->player)
         {
            if((line->special & FloorChange) ||
               !(line->special & FloorModel))
               return;   // FloorModel is "Allow Monsters" if FloorChange is 0
         }
         if(!line->tag) //jff 2/27/98 all gun generalized types require tag
            return;
         
         linefunc = EV_DoGenFloor;
      }
      else if((unsigned)line->special >= GenCeilingBase)
      {
         if(!thing->player)
         {
            if((line->special & CeilingChange) || !(line->special & CeilingModel))
               return;   // CeilingModel is "Allow Monsters" if CeilingChange is 0
         }
         if(!line->tag) //jff 2/27/98 all gun generalized types require tag
            return;
         linefunc = EV_DoGenCeiling;
      }
      else if((unsigned)line->special >= GenDoorBase)
      {
         if(!thing->player)
         {
            if(!(line->special & DoorMonster))
               return;   // monsters disallowed from this door
            if(line->flags & ML_SECRET) // they can't open secret doors either
               return;
         }
         if(!line->tag) //jff 3/2/98 all gun generalized types require tag
            return;
         genDoorThing = thing;
         linefunc = EV_DoGenDoor;
      }
      else if((unsigned)line->special >= GenLockedBase)
      {
         if(!thing->player)
            return;   // monsters disallowed from unlocking doors
         if(((line->special&TriggerType)==GunOnce) ||
            ((line->special&TriggerType)==GunMany))
         { //jff 4/1/98 check for being a gun type before reporting door type
            if(!P_CanUnlockGenDoor(line,thing->player))
               return;
         }
         else
            return;
         if(!line->tag) //jff 2/27/98 all gun generalized types require tag
            return;
         
         genDoorThing = thing;
         linefunc = EV_DoGenLockedDoor;
      }
      else if((unsigned)line->special >= GenLiftBase)
      {
         if(!thing->player)
         {
            if(!(line->special & LiftMonster))
               return; // monsters disallowed
         }
         linefunc = EV_DoGenLift;
      }
      else if((unsigned)line->special >= GenStairsBase)
      {
         if(!thing->player)
         {
            if(!(line->special & StairMonster))
               return; // monsters disallowed
         }
         if(!line->tag) //jff 2/27/98 all gun generalized types require tag
            return;
         linefunc = EV_DoGenStairs;
      }
      else if((unsigned)line->special >= GenCrusherBase)
      {
         if(!thing->player)
         {
            if(!(line->special & StairMonster))
               return; // monsters disallowed
         }
         if(!line->tag) //jff 2/27/98 all gun generalized types require tag
            return;
         linefunc = EV_DoGenCrusher;
      }

      if(linefunc)
      {
         switch((line->special & TriggerType) >> TriggerTypeShift)
         {
         case GunOnce:
            if (linefunc(line))
               P_ChangeSwitchTexture(line,0);
            return;
         case GunMany:
            if (linefunc(line))
               P_ChangeSwitchTexture(line,1);
            return;
         default:  // if not a gun type, do nothing here
            return;
         }
      }
   }

   // Impacts that other things can activate.
   if(!thing->player)
   {
      int ok = 0;
      switch(line->special)
      {
      case 46:
         // 46 GR Open door on impact weapon is monster activatable
         ok = 1;
         break;
      }
      if(!ok)
         return;
   }

   if(!P_CheckTag(line))  //jff 2/27/98 disallow zero tag on some types
      return;

   switch(line->special)
   {
   case 24:
      // 24 G1 raise floor to highest adjacent
      if(EV_DoFloor(line,raiseFloor) || demo_compatibility)
         P_ChangeSwitchTexture(line,0);
      break;

   case 46:
      // 46 GR open door, stay open
      EV_DoDoor(line,doorOpen);
      P_ChangeSwitchTexture(line,1);
      break;
      
   case 47:
      // 47 G1 raise floor to nearest and change texture and type
      if(EV_DoPlat(line,raiseToNearestAndChange,0) || demo_compatibility)
         P_ChangeSwitchTexture(line,0);
      break;
      
      //jff 1/30/98 added new gun linedefs here
      // killough 1/31/98: added demo_compatibility check, added inner switch

   default:
      if(!demo_compatibility)
      {
         switch (line->special)
         {
         case 197:
            // Exit to next level
            // killough 10/98: prevent zombies from exiting levels
            if(thing->player && thing->player->health<=0 && !comp[comp_zombie])
               break;
            P_ChangeSwitchTexture(line,0);
            G_ExitLevel();
            break;
            
         case 198:
            // Exit to secret level
            // killough 10/98: prevent zombies from exiting levels
            if(thing->player && thing->player->health<=0 && !comp[comp_zombie])
               break;
            P_ChangeSwitchTexture(line,0);
            G_SecretExitLevel();
            break;
            //jff end addition of new gun linedefs

            // sf: scripting
         case 279: // G1 start script
            line->special = 0;
         case 278: // GR start script
            break;
         }
      }
      break;
   }
}

        // sf: changed to enable_nuke for console
int enable_nuke = 1;  // killough 12/98: nukage disabling cheat

// haleyjd 10/08/02: better support for heretic nukage types

#define DOOMNUKAGE 0
#define HTICNUKAGE 1

typedef struct nukage_s
{
   int dmgAmount;  // how much damage to inflict
   int dmgTics;    // how often to inflict damage
} nukage_t;

nukage_t nukageValues[4][2] =
{
   { {  0, 0x00 }, { 0, 0x00 } }, // extended type 0
   { {  5, 0x1f }, { 4, 0x1f } }, // extended type 1
   { { 10, 0x1f }, { 5, 0x0f } }, // extended type 2
   { { 20, 0x1f }, { 8, 0x0f } }, // extended type 3
};

//
// P_PlayerInSpecialSector()
//
// Called every tick frame
//  that the player origin is in a special sector
//
// Changed to ignore sector types the engine does not recognize
//

void P_PlayerInSpecialSector (player_t *player)
{
   int dmgAmt, dmgTics, dmgType;
   sector_t *sector = player->mo->subsector->sector;

   // Falling, not all the way down yet?
   // Sector specials don't apply in mid-air
   if(player->mo->z != sector->floorheight)
      return;

   // Has hit ground.
   //jff add if to handle old vs generalized types
   if(sector->special<32) // regular sector specials
   {
      if(sector->special == 9)     // killough 12/98
      {
         // Tally player in secret sector, clear secret special
         player->secretcount++;
         sector->special = 0;
      }
      else if(enable_nuke)  // killough 12/98: nukage disabling cheat
      {
         switch(sector->special)
         {
         case 5:
            // 5/10 unit damage per 31 ticks
            if(!player->powers[pw_ironfeet])
            {
               if(!(leveltime&0x1f))
               {
                  P_DamageMobj(player->mo, NULL, NULL, 10, MOD_SLIME);
               }
            }
            break;

         case 7:
            // 2/5 unit damage per 31 ticks
            if(!player->powers[pw_ironfeet])
            {
               if(!(leveltime&0x1f))
                  P_DamageMobj(player->mo, NULL, NULL, 5, MOD_SLIME);
            }
            break;

         case 16:
            // 10/20 unit damage per 31 ticks
         case 4:
            // 10/20 unit damage plus blinking light (light already spawned)
            if(!player->powers[pw_ironfeet] || 
               (P_Random(pr_slimehurt)<5) ) // even with suit, take damage
            {
               if(!(leveltime&0x1f))
                  P_DamageMobj(player->mo, NULL, NULL, 20, MOD_SLIME);
            }
            break;

         case 11:
            // Exit on health < 11, take 10/20 damage per 31 ticks
            if(comp[comp_god])   // killough 2/21/98: add compatibility switch
               player->cheats &= ~CF_GODMODE; // on godmode cheat clearing
            // does not affect invulnerability
            if(!(leveltime&0x1f))
               P_DamageMobj (player->mo, NULL, NULL, 20, MOD_SLIME);
            if(player->health <= 10)
               G_ExitLevel();
            break;

         default:
            //jff 1/24/98 Don't exit as DOOM2 did, just ignore
            break;
         }
      }
   }
   else //jff 3/14/98 handle extended sector types for secrets and damage
   {
      if(enable_nuke)  // killough 12/98: nukage disabling cheat
      {
         // haleyjd 03/12/03: heretic damage selection
         if(demo_version >= 331 && (sector->special & HTIC_DMG_MASK))
            dmgType = HTICNUKAGE;
         else
            dmgType = DOOMNUKAGE;

         switch((sector->special&DAMAGE_MASK)>>DAMAGE_SHIFT)
         {
         case 0: // no damage
            break;
         case 1: // 2/5 damage per 31 ticks
            dmgAmt  = nukageValues[dmgType][1].dmgAmount;
            dmgTics = nukageValues[dmgType][1].dmgTics;
            if(!player->powers[pw_ironfeet])
            {
               if(!(leveltime&dmgTics))
                  P_DamageMobj(player->mo, NULL, NULL, dmgAmt, MOD_SLIME);
            }
            break;
         case 2: // 5/10 damage per 31 ticks
            dmgAmt  = nukageValues[dmgType][2].dmgAmount;
            dmgTics = nukageValues[dmgType][2].dmgTics;
            if(!player->powers[pw_ironfeet])
            {
               if(!(leveltime&dmgTics))
               {
                  P_DamageMobj(player->mo, NULL, NULL, dmgAmt, MOD_SLIME);
                  
                  if(dmgType == HTICNUKAGE)
                     P_HitFloor(player->mo);
               }
            }
            break;
         case 3: // 10/20 damage per 31 ticks
            dmgAmt  = nukageValues[dmgType][3].dmgAmount;
            dmgTics = nukageValues[dmgType][3].dmgTics;
            if(!player->powers[pw_ironfeet]
               || (P_Random(pr_slimehurt)<5))  // take damage even with suit
            {
               if(!(leveltime&dmgTics))
               {
                  P_DamageMobj(player->mo, NULL, NULL, dmgAmt, MOD_SLIME);
                  
                  if(dmgType == HTICNUKAGE)
                     P_HitFloor(player->mo);
               }
            }
            break;
         }
      }

      if(sector->special&SECRET_MASK)
      {
         player->secretcount++;
         sector->special &= ~SECRET_MASK;
         if(sector->special<32) // if all extended bits clear,
            sector->special=0;    // sector is not special anymore
      }

      // phares 3/19/98:
      //
      // If FRICTION_MASK or PUSH_MASK is set, we don't care at this
      // point, since the code to deal with those situations is
      // handled by Thinkers.
   }
}

//
// P_UpdateSpecials()
//
// Check level timer, frag counter,
// animate flats, scroll walls,
// change button textures
//
// Reads and modifies globals:
//  levelTimer, levelTimeCount,
//  levelFragLimit, levelFragLimitCount
//

// sf: rearranged variables

int             levelTime;              // sf
int             levelTimeLimit;
int             levelFragLimit; // Ty 03/18/98 Added -frags support

void P_UpdateSpecials (void)
{
   anim_t *anim;
   int    pic;
   int    i;

   levelTime++;

   // Downcount level timer, exit level if elapsed
   if(levelTimeLimit && leveltime >= levelTimeLimit*35*60 )
      G_ExitLevel();

   // Check frag counters, if frag limit reached, exit level // Ty 03/18/98
   //  Seems like the total frags should be kept in a simple
   //  array somewhere, but until they are...
   if(levelFragLimit)  // we used -frags so compare count
   {
      int i;
      for(i=0; i<MAXPLAYERS; i++)
      {
         if(!playeringame[i])
            continue;
          // sf: use hu_frags.c frag counter
         if(players[i].totalfrags >= levelFragLimit)
            break;
      }
      if(i < MAXPLAYERS)       // sf: removed exitflag (ugh)
         G_ExitLevel();
   }

   // Animate flats and textures globally
   for (anim = anims ; anim < lastanim ; anim++)
   {
      for(i=anim->basepic ; i<anim->basepic+anim->numpics ; i++)
      {
         pic = anim->basepic + 
               ((leveltime/anim->speed + i)%anim->numpics);
         if(anim->istexture)
            texturetranslation[i] = pic;
         else                    // sf: swirly water hack
            flattranslation[i] = r_swirl ? -1 : pic;
         // sf: > 65535 : swirly hack 
         if(anim->speed > 65535 || anim->numpics == 1)
            flattranslation[i] = -1;
      }
   }

   // Check buttons (retriggerable switches) and change texture on timeout
   for(i = 0; i < MAXBUTTONS; i++)
   {
      if(buttonlist[i].btimer)
      {
         buttonlist[i].btimer--;
         if(!buttonlist[i].btimer)
         {
            switch(buttonlist[i].where)
            {
            case top:
               sides[buttonlist[i].line->sidenum[0]].toptexture =
                  buttonlist[i].btexture;
               break;

            case middle:
               sides[buttonlist[i].line->sidenum[0]].midtexture =
                  buttonlist[i].btexture;
               break;

            case bottom:
               sides[buttonlist[i].line->sidenum[0]].bottomtexture =
                  buttonlist[i].btexture;
               break;
            }
            S_StartSound((mobj_t *)&buttonlist[i].soundorg,sfx_swtchn);
            memset(&buttonlist[i],0,sizeof(button_t));
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////
//
// Sector and Line special thinker spawning at level startup
//
//////////////////////////////////////////////////////////////////////

//
// P_SpawnSpecials
// After the map has been loaded,
//  scan for specials that spawn thinkers
//

// Parses command line parameters.
void P_SpawnSpecials (void)
{
   sector_t *sector;
   int      i;
   int      episode;
   
   episode = 1;
   if(W_CheckNumForName("texture2") >= 0)
      episode = 2;
   
   // sf: -timer moved to d_main.c
   //     -avg also
   
   // sf: changed -frags: not loaded at start of every level
   //     to allow changing by console

   // reset levelTime.
   levelTime = 0;

   //  Init special sectors.
   sector = sectors;
   for (i=0 ; i<numsectors ; i++, sector++)
   {
      if(!sector->special)
         continue;

      if(sector->special&SECRET_MASK) //jff 3/15/98 count extended
         totalsecret++;                 // secret sectors too

      switch (sector->special&31)
      {
      case 1:
         // random off
         P_SpawnLightFlash (sector);
         break;

      case 2:
         // strobe fast
         P_SpawnStrobeFlash(sector,FASTDARK,0);
         break;

      case 3:
         // strobe slow
         P_SpawnStrobeFlash(sector,SLOWDARK,0);
         break;

      case 4:
         // strobe fast/death slime
         P_SpawnStrobeFlash(sector,FASTDARK,0);
         sector->special |= 3<<DAMAGE_SHIFT; //jff 3/14/98 put damage bits in
         break;

      case 8:
         // glowing light
         P_SpawnGlowingLight(sector);
         break;
      case 9:
         // secret sector
         if (sector->special<32) //jff 3/14/98 bits don't count unless not
            totalsecret++;        // a generalized sector type
         break;

      case 10:
         // door close in 30 seconds
         P_SpawnDoorCloseIn30 (sector);
         break;
         
      case 12:
         // sync strobe slow
         P_SpawnStrobeFlash (sector, SLOWDARK, 1);
         break;
         
      case 13:
         // sync strobe fast
         P_SpawnStrobeFlash (sector, FASTDARK, 1);
         break;
         
      case 14:
         // door raise in 5 minutes
         P_SpawnDoorRaiseIn5Mins (sector, i);
         break;
         
      case 17:
         // fire flickering
         P_SpawnFireFlicker(sector);
         break;
      }
   }

   P_RemoveAllActiveCeilings();  // jff 2/22/98 use killough's scheme
   
   P_RemoveAllActivePlats();     // killough

   for(i = 0; i < MAXBUTTONS; i++)
      memset(&buttonlist[i], 0, sizeof(button_t));

   // P_InitTagLists() must be called before P_FindSectorFromLineTag()
   // or P_FindLineFromLineTag() can be called.

   P_InitTagLists();   // killough 1/30/98: Create xref tables for tags
   
   P_SpawnScrollers(); // killough 3/7/98: Add generalized scrollers
   
   P_SpawnFriction();  // phares 3/12/98: New friction model using linedefs
   
   P_SpawnPushers();   // phares 3/20/98: New pusher model using linedefs

   for(i = 0; i < numlines; i++)
   {
      switch(lines[i].special)
      {
         int s, sec;

         // killough 3/7/98:
         // support for drawn heights coming from different sector
      case 242:
         sec = sides[*lines[i].sidenum].sector-sectors;
         for(s = -1; (s = P_FindSectorFromLineTag(lines+i,s)) >= 0;)
            sectors[s].heightsec = sec;
         break;

         // killough 3/16/98: Add support for setting
         // floor lighting independently (e.g. lava)
      case 213:
         sec = sides[*lines[i].sidenum].sector-sectors;
         for(s = -1; (s = P_FindSectorFromLineTag(lines+i,s)) >= 0;)
            sectors[s].floorlightsec = sec;
         break;

         // killough 4/11/98: Add support for setting
         // ceiling lighting independently
      case 261:
         sec = sides[*lines[i].sidenum].sector-sectors;
         for(s = -1; (s = P_FindSectorFromLineTag(lines+i,s)) >= 0;)
            sectors[s].ceilinglightsec = sec;
         break;

         // killough 10/98:
         //
         // Support for sky textures being transferred from sidedefs.
         // Allows scrolling and other effects (but if scrolling is
         // used, then the same sector tag needs to be used for the
         // sky sector, the sky-transfer linedef, and the scroll-effect
         // linedef). Still requires user to use F_SKY1 for the floor
         // or ceiling texture, to distinguish floor and ceiling sky.

      case 271:   // Regular sky
      case 272:   // Same, only flipped
         for(s = -1; (s = P_FindSectorFromLineTag(lines+i,s)) >= 0;)
            sectors[s].sky = i | PL_SKYFLAT;
         break;


      // SoM 9/19/02
      // Support for attaching sectors to each other. When a sector
      // is attached to another sector, the master sector's floor
      // and/or ceiling will move all 3d sides of the attached
      // sectors. The 3d sides, will then be tested in P_MoveFlat
      // and will affect weather or not the sector will keep moving,
      // thus keeping compatibility for all thinker types.
      case 281:
         P_AttachSectors(&lines[i], false);
         break;
      case 282:
         P_AttachSectors(&lines[i], true);
         break;

      // haleyjd 03/12/03: Heretic wind transfer specials
      case 293:
      case 294:
         P_SpawnHereticWind(&lines[i]);
         break;

#ifdef R_PORTALS
      // SoM 12/10/03: added skybox/portal specials
      // haleyjd 01/24/04: functionalized code to reduce footprint
      case 283:
      case 284:
      case 285:
         P_SpawnPortal(&lines[i], portal_plane, lines[i].special - 283);
         break;
      case 286:
      case 287:
      case 288:
         P_SpawnPortal(&lines[i], portal_horizon, lines[i].special - 286);
         break;
      case 290:
      case 291:
      case 292:
         P_SpawnPortal(&lines[i], portal_skybox, lines[i].special - 290);
         break;
      case 295:
      case 296:
      case 297:
         P_SpawnPortal(&lines[i], portal_anchored, lines[i].special - 295);
         break;
#endif 
      }
   }
}

// killough 2/28/98:
//
// This function, with the help of r_plane.c and r_bsp.c, supports generalized
// scrolling floors and walls, with optional mobj-carrying properties, e.g.
// conveyor belts, rivers, etc. A linedef with a special type affects all
// tagged sectors the same way, by creating scrolling and/or object-carrying
// properties. Multiple linedefs may be used on the same sector and are
// cumulative, although the special case of scrolling a floor and carrying
// things on it, requires only one linedef. The linedef's direction determines
// the scrolling direction, and the linedef's length determines the scrolling
// speed. This was designed so that an edge around the sector could be used to
// control the direction of the sector's scrolling, which is usually what is
// desired.
//
// Process the active scrollers.
//
// This is the main scrolling code
// killough 3/7/98

void T_Scroll(scroll_t *s)
{
   fixed_t dx = s->dx, dy = s->dy;
   
   if(s->control != -1)
   {   // compute scroll amounts based on a sector's height changes
      fixed_t height = sectors[s->control].floorheight +
         sectors[s->control].ceilingheight;
      fixed_t delta = height - s->last_height;
      s->last_height = height;
      dx = FixedMul(dx, delta);
      dy = FixedMul(dy, delta);
   }

   // killough 3/14/98: Add acceleration
   if(s->accel)
   {
      s->vdx = dx += s->vdx;
      s->vdy = dy += s->vdy;
   }

   if(!(dx | dy))                   // no-op if both (x,y) offsets 0
      return;

   switch (s->type)
   {
      side_t *side;
      sector_t *sec;
      fixed_t height, waterheight;  // killough 4/4/98: add waterheight
      msecnode_t *node;
      mobj_t *thing;

   case sc_side:                   // killough 3/7/98: Scroll wall texture
      side = sides + s->affectee;
      side->textureoffset += dx;
      side->rowoffset += dy;
      break;

   case sc_floor:                  // killough 3/7/98: Scroll floor texture
      sec = sectors + s->affectee;
      sec->floor_xoffs += dx;
      sec->floor_yoffs += dy;
      break;

   case sc_ceiling:               // killough 3/7/98: Scroll ceiling texture
      sec = sectors + s->affectee;
      sec->ceiling_xoffs += dx;
      sec->ceiling_yoffs += dy;
      break;

   case sc_carry:
      
      // killough 3/7/98: Carry things on floor
      // killough 3/20/98: use new sector list which reflects true members
      // killough 3/27/98: fix carrier bug
      // killough 4/4/98: Underwater, carry things even w/o gravity

      sec = sectors + s->affectee;
      height = sec->floorheight;
      waterheight = sec->heightsec != -1 &&
         sectors[sec->heightsec].floorheight > height ?
         sectors[sec->heightsec].floorheight : D_MININT;

      // Move objects only if on floor or underwater,
      // non-floating, and clipped.
      
      // haleyjd: added much-needed MF2_NOTHRUST flag to make some
      //   objects unmoveable by sector effects

      for(node = sec->touching_thinglist; node; node = node->m_snext)
      {
         if(!((thing = node->m_thing)->flags & MF_NOCLIP) &&
            !(thing->flags2 & MF2_NOTHRUST) &&
            (!(thing->flags & MF_NOGRAVITY || thing->z > height) ||
             thing->z < waterheight))
         {
            thing->momx += dx, thing->momy += dy;
         }
      }
      break;

   case sc_carry_ceiling:       // to be added later
      break;
   }
}

//
// Add_Scroller()
//
// Add a generalized scroller to the thinker list.
//
// type: the enumerated type of scrolling: floor, ceiling, floor carrier,
//   wall, floor carrier & scroller
//
// (dx,dy): the direction and speed of the scrolling or its acceleration
//
// control: the sector whose heights control this scroller's effect
//   remotely, or -1 if no control sector
//
// affectee: the index of the affected object (sector or sidedef)
//
// accel: non-zero if this is an accelerative effect
//

static void Add_Scroller(int type, fixed_t dx, fixed_t dy,
                         int control, int affectee, int accel)
{
   scroll_t *s = Z_Malloc(sizeof *s, PU_LEVSPEC, 0);
   s->thinker.function = T_Scroll;
   s->type = type;
   s->dx = dx;
   s->dy = dy;
   s->accel = accel;
   s->vdx = s->vdy = 0;
   if((s->control = control) != -1)
      s->last_height =
       sectors[control].floorheight + sectors[control].ceilingheight;
   s->affectee = affectee;
   P_AddThinker(&s->thinker);
}

// Adds wall scroller. Scroll amount is rotated with respect to wall's
// linedef first, so that scrolling towards the wall in a perpendicular
// direction is translated into vertical motion, while scrolling along
// the wall in a parallel direction is translated into horizontal motion.
//
// killough 5/25/98: cleaned up arithmetic to avoid drift due to roundoff
//
// killough 10/98:
// fix scrolling aliasing problems, caused by long linedefs causing overflowing

static void Add_WallScroller(Long64 dx, Long64 dy, const line_t *l,
                             int control, int accel)
{
   fixed_t x = D_abs(l->dx), y = D_abs(l->dy), d;
   if(y > x)
      d = x, x = y, y = d;
   d = FixedDiv(x,
      finesine[(tantoangle[FixedDiv(y,x)>>DBITS]+ANG90) >> ANGLETOFINESHIFT]);

   x = (int)((dy * -l->dy - dx * l->dx) / d);  // killough 10/98:
   y = (int)((dy * l->dx - dx * l->dy) / d);   // Use 64-bit arithmetic
   Add_Scroller(sc_side, x, y, control, *l->sidenum, accel);
}

// Amount (dx,dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR ((fixed_t)(FRACUNIT*.09375))

// Initialize the scrollers
static void P_SpawnScrollers(void)
{
   int i;
   line_t *l = lines;

   for(i=0;i<numlines;i++,l++)
   {
      fixed_t dx = l->dx >> SCROLL_SHIFT;  // direction and speed of scrolling
      fixed_t dy = l->dy >> SCROLL_SHIFT;
      int control = -1, accel = 0;         // no control sector or acceleration
      int special = l->special;

      // killough 3/7/98: Types 245-249 are same as 250-254 except that the
      // first side's sector's heights cause scrolling when they change, and
      // this linedef controls the direction and speed of the scrolling. The
      // most complicated linedef since donuts, but powerful :)
      //
      // killough 3/15/98: Add acceleration. Types 214-218 are the same but
      // are accelerative.

      if(special >= 245 && special <= 249)         // displacement scrollers
      {
         special += 250-245;
         control = sides[*l->sidenum].sector - sectors;
      }
      else if(special >= 214 && special <= 218)       // accelerative scrollers
      {
         accel = 1;
         special += 250-214;
         control = sides[*l->sidenum].sector - sectors;
      }

      switch(special)
      {
         register int s;

      case 250:   // scroll effect ceiling
         for(s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
            Add_Scroller(sc_ceiling, -dx, dy, control, s, accel);
         break;

      case 251:   // scroll effect floor
      case 253:   // scroll and carry objects on floor
         for(s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
            Add_Scroller(sc_floor, -dx, dy, control, s, accel);
         if(special != 253)
            break;

      case 252: // carry objects on floor
         dx = FixedMul(dx,CARRYFACTOR);
         dy = FixedMul(dy,CARRYFACTOR);
         for(s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
            Add_Scroller(sc_carry, dx, dy, control, s, accel);
         break;

         // killough 3/1/98: scroll wall according to linedef
         // (same direction and speed as scrolling floors)
      case 254:
         for(s=-1; (s = P_FindLineFromLineTag(l,s)) >= 0;)
         {
            if(s != i)
               Add_WallScroller(dx, dy, lines+s, control, accel);
         }
         break;

      case 255:    // killough 3/2/98: scroll according to sidedef offsets
         s = lines[i].sidenum[0];
         Add_Scroller(sc_side, -sides[s].textureoffset,
                      sides[s].rowoffset, -1, s, accel);
         break;

      case 48:                  // scroll first side
         Add_Scroller(sc_side,  FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
         break;
         
      case 85:                  // jff 1/30/98 2-way scroll
         Add_Scroller(sc_side, -FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
         break;
      }
   }
}

// killough 3/7/98 -- end generalized scroll effects

////////////////////////////////////////////////////////////////////////////
//
// FRICTION EFFECTS
//
// phares 3/12/98: Start of friction effects
//
// As the player moves, friction is applied by decreasing the x and y
// momentum values on each tic. By varying the percentage of decrease,
// we can simulate muddy or icy conditions. In mud, the player slows
// down faster. In ice, the player slows down more slowly.
//
// The amount of friction change is controlled by the length of a linedef
// with type 223. A length < 100 gives you mud. A length > 100 gives you ice.
//
// Also, each sector where these effects are to take place is given a
// new special type _______. Changing the type value at runtime allows
// these effects to be turned on or off.
//
// Sector boundaries present problems. The player should experience these
// friction changes only when his feet are touching the sector floor. At
// sector boundaries where floor height changes, the player can find
// himself still 'in' one sector, but with his feet at the floor level
// of the next sector (steps up or down). To handle this, Thinkers are used
// in icy/muddy sectors. These thinkers examine each object that is touching
// their sectors, looking for players whose feet are at the same level as
// their floors. Players satisfying this condition are given new friction
// values that are applied by the player movement code later.
//
// killough 8/28/98:
//
// Completely redid code, which did not need thinkers, and which put a heavy
// drag on CPU. Friction is now a property of sectors, NOT objects inside
// them. All objects, not just players, are affected by it, if they touch
// the sector's floor. Code simpler and faster, only calling on friction
// calculations when an object needs friction considered, instead of doing
// friction calculations on every sector during every tic.
//
// Although this -might- ruin Boom demo sync involving friction, it's the only
// way, short of code explosion, to fix the original design bug. Fixing the
// design bug in Boom's original friction code, while maintaining demo sync
// under every conceivable circumstance, would double or triple code size, and
// would require maintenance of buggy legacy code which is only useful for old
// demos. Doom demos, which are more important IMO, are not affected by this
// change.
//
/////////////////////////////
//
// Initialize the sectors where friction is increased or decreased

static void P_SpawnFriction(void)
{
   int i;
   line_t *l = lines;
   
   // killough 8/28/98: initialize all sectors to normal friction first
   for(i = 0; i < numsectors; i++)
   {
      // haleyjd: special hacks may have already set the friction, so
      // skip any value that's not zero (now zeroed in P_LoadSectors)
      if(!sectors[i].friction)
      {
         sectors[i].friction = ORIG_FRICTION;
         sectors[i].movefactor = ORIG_FRICTION_FACTOR;
      }
   }

   for(i = 0 ; i < numlines ; i++,l++)
   {
      if(l->special == 223)
      {
         int length = P_AproxDistance(l->dx,l->dy)>>FRACBITS;
         int friction = (0x1EB8*length)/0x80 + 0xD000;
         int movefactor, s;

         // The following check might seem odd. At the time of movement,
         // the move distance is multiplied by 'friction/0x10000', so a
         // higher friction value actually means 'less friction'.

         if(friction > ORIG_FRICTION)       // ice
            movefactor = ((0x10092 - friction)*(0x70))/0x158;
         else
            movefactor = ((friction - 0xDB34)*(0xA))/0x80;

         if(demo_version >= 203)
         { // killough 8/28/98: prevent odd situations
            if(friction > FRACUNIT)
               friction = FRACUNIT;
            if(friction < 0)
               friction = 0;
            if(movefactor < 32)
               movefactor = 32;
         }

         for(s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ;)
         {
            // killough 8/28/98:
            //
            // Instead of spawning thinkers, which are slow and expensive,
            // modify the sector's own friction values. Friction should be
            // a property of sectors, not objects which reside inside them.
            // Original code scanned every object in every friction sector
            // on every tic, adjusting its friction, putting unnecessary
            // drag on CPU. New code adjusts friction of sector only once
            // at level startup, and then uses this friction value.
            
            sectors[s].friction = friction;
            sectors[s].movefactor = movefactor;
         }
      }
   }
}

//
// phares 3/12/98: End of friction effects
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// PUSH/PULL EFFECT
//
// phares 3/20/98: Start of push/pull effects
//
// This is where push/pull effects are applied to objects in the sectors.
//
// There are four kinds of push effects
//
// 1) Pushing Away
//
//    Pushes you away from a point source defined by the location of an
//    PUSH Thing. The force decreases linearly with distance from the
//    source. This force crosses sector boundaries and is felt w/in a circle
//    whose center is at the PUSH. The force is felt only if the point
//    PUSH can see the target object.
//
// 2) Pulling toward
//
//    Same as Pushing Away except you're pulled toward an PULL point
//    source. This force crosses sector boundaries and is felt w/in a circle
//    whose center is at the PULL. The force is felt only if the point
//    PULL can see the target object.
//
// 3) Wind
//
//    Pushes you in a constant direction. Full force above ground, half
//    force on the ground, nothing if you're below it (water).
//
// 4) Current
//
//    Pushes you in a constant direction. No force above ground, full
//    force if on the ground or below it (water).
//
// The magnitude of the force is controlled by the length of a controlling
// linedef. The force vector for types 3 & 4 is determined by the angle
// of the linedef, and is constant.
//
// For each sector where these effects occur, the sector special type has
// to have the PUSH_MASK bit set. If this bit is turned off by a switch
// at run-time, the effect will not occur. The controlling sector for
// types 1 & 2 is the sector containing the PUSH/PULL Thing.

#define PUSH_FACTOR 7

/////////////////////////////
//
// Add a push thinker to the thinker list

static void Add_Pusher(int type, int x_mag, int y_mag,
                       mobj_t *source, int affectee)
{
   pusher_t *p = Z_Malloc(sizeof *p, PU_LEVSPEC, 0);
   
   p->thinker.function = T_Pusher;
   p->source = source;
   p->type = type;
   p->x_mag = x_mag>>FRACBITS;
   p->y_mag = y_mag>>FRACBITS;
   p->magnitude = P_AproxDistance(p->x_mag,p->y_mag);
   if(source) // point source exist?
   {
      p->radius = (p->magnitude)<<(FRACBITS+1); // where force goes to zero
      p->x = p->source->x;
      p->y = p->source->y;
   }
   p->affectee = affectee;
   P_AddThinker(&p->thinker);
}

/////////////////////////////
//
// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.
//
// tmpusher belongs to the point source (PUSH/PULL).
//
// killough 10/98: allow to affect things besides players

pusher_t* tmpusher; // pusher structure for blockmap searches

boolean PIT_PushThing(mobj_t* thing)
{
   if(demo_version < 203  ?     // killough 10/98: made more general
      thing->player && !(thing->flags & (MF_NOCLIP | MF_NOGRAVITY)) :
      (sentient(thing) || thing->flags & MF_SHOOTABLE) &&
      !(thing->flags & MF_NOCLIP) &&
      !(thing->flags2 & MF2_NOTHRUST)) // haleyjd
   {
      angle_t pushangle;
      fixed_t speed;
      fixed_t sx = tmpusher->x;
      fixed_t sy = tmpusher->y;

      speed = (tmpusher->magnitude -
               ((P_AproxDistance(thing->x - sx,thing->y - sy)
                 >>FRACBITS)>>1))<<(FRACBITS-PUSH_FACTOR-1);

      // killough 10/98: make magnitude decrease with square
      // of distance, making it more in line with real nature,
      // so long as it's still in range with original formula.
      //
      // Removes angular distortion, and makes effort required
      // to stay close to source, grow increasingly hard as you
      // get closer, as expected. Still, it doesn't consider z :(

      if(speed > 0 && demo_version >= 203)
      {
         int x = (thing->x-sx) >> FRACBITS;
         int y = (thing->y-sy) >> FRACBITS;
         speed = 
            (int)(((Long64)tmpusher->magnitude << 23) / (x*x+y*y+1));
      }

      // If speed <= 0, you're outside the effective radius. You also have
      // to be able to see the push/pull source point.

      if(speed > 0 && P_CheckSight(thing,tmpusher->source))
      {
         pushangle = R_PointToAngle2(thing->x,thing->y,sx,sy);
         if(tmpusher->source->type == E_ThingNumForDEHNum(MT_PUSH))
            pushangle += ANG180;    // away
         pushangle >>= ANGLETOFINESHIFT;
         thing->momx += FixedMul(speed,finecosine[pushangle]);
         thing->momy += FixedMul(speed,finesine[pushangle]);
      }
   }
   return true;
}

/////////////////////////////
//
// T_Pusher looks for all objects that are inside the radius of
// the effect.
//

void T_Pusher(pusher_t *p)
{
   sector_t *sec;
   mobj_t   *thing;
   msecnode_t* node;
   int xspeed,yspeed;
   int xl,xh,yl,yh,bx,by;
   int radius;
   int ht = 0;
   
   if(!allow_pushers)
      return;

   sec = sectors + p->affectee;
   
   // Be sure the special sector type is still turned on. If so, proceed.
   // Else, bail out; the sector type has been changed on us.
   
   if(!(sec->special & PUSH_MASK))
      return;

   // For constant pushers (wind/current) there are 3 situations:
   //
   // 1) Affected Thing is above the floor.
   //
   //    Apply the full force if wind, no force if current.
   //
   // 2) Affected Thing is on the ground.
   //
   //    Apply half force if wind, full force if current.
   //
   // 3) Affected Thing is below the ground (underwater effect).
   //
   //    Apply no force if wind, full force if current.
   //
   // haleyjd:
   // 4) Affected thing bears MF2_NOTHRUST flag
   //
   //    Apply nothing at any time!

   if(p->type == p_push)
   {
      // Seek out all pushable things within the force radius of this
      // point pusher. Crosses sectors, so use blockmap.

      tmpusher = p; // PUSH/PULL point source
      radius = p->radius; // where force goes to zero
      tmbbox[BOXTOP]    = p->y + radius;
      tmbbox[BOXBOTTOM] = p->y - radius;
      tmbbox[BOXRIGHT]  = p->x + radius;
      tmbbox[BOXLEFT]   = p->x - radius;
      
      xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
      xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
      yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
      yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
      for (bx = xl; bx <= xh; bx++)
      {
         for(by = yl; by <= yh; by++)
            P_BlockThingsIterator(bx,by,PIT_PushThing);
      }
      return;
   }

   // constant pushers p_wind and p_current
   
   if(sec->heightsec != -1) // special water sector?
      ht = sectors[sec->heightsec].floorheight;
   node = sec->touching_thinglist; // things touching this sector
   for( ; node; node = node->m_snext)
    {
      thing = node->m_thing;
      if(!thing->player || 
         (thing->flags2 & MF2_NOTHRUST) ||                // haleyjd
         (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
         continue;
      if(p->type == p_wind)
      {
         if(sec->heightsec == -1) // NOT special water sector
         {
            if(thing->z > thing->floorz) // above ground
            {
               xspeed = p->x_mag; // full force
               yspeed = p->y_mag;
            }
            else // on ground
            {
               xspeed = (p->x_mag)>>1; // half force
               yspeed = (p->y_mag)>>1;
            }
         }
         else // special water sector
         {
            if(thing->z > ht) // above ground
            {
               xspeed = p->x_mag; // full force
               yspeed = p->y_mag;
            }
            else if(thing->player->viewz < ht) // underwater
               xspeed = yspeed = 0; // no force
            else // wading in water
            {
               xspeed = (p->x_mag)>>1; // half force
               yspeed = (p->y_mag)>>1;
            }
         }
      }
      else // p_current
      {
         if(sec->heightsec == -1) // NOT special water sector
         {
            if(thing->z > sec->floorheight) // above ground
               xspeed = yspeed = 0; // no force
            else // on ground
            {
               xspeed = p->x_mag; // full force
               yspeed = p->y_mag;
            }
         }
         else // special water sector
         {
            if(thing->z > ht) // above ground
               xspeed = yspeed = 0; // no force
            else // underwater
            {
               xspeed = p->x_mag; // full force
               yspeed = p->y_mag;
            }
         }
      }
      thing->momx += xspeed<<(FRACBITS-PUSH_FACTOR);
      thing->momy += yspeed<<(FRACBITS-PUSH_FACTOR);
   }
}

/////////////////////////////
//
// P_GetPushThing() returns a pointer to an PUSH or PULL thing,
// NULL otherwise.

mobj_t* P_GetPushThing(int s)
{
   mobj_t* thing;
   sector_t* sec;
   static int PushType = -1;
   static int PullType = -1;

   if(PushType == -1)
   {
      PushType = E_ThingNumForDEHNum(MT_PUSH);
      PullType = E_ThingNumForDEHNum(MT_PULL);
   }

   sec = sectors + s;
   thing = sec->thinglist;
   while(thing)
   {
      if(thing->type == PushType || thing->type == PullType)
         return thing;

      thing = thing->snext;
   }
   return NULL;
}

/////////////////////////////
//
// Initialize the sectors where pushers are present
//

static void P_SpawnPushers(void)
{
   int i;
   line_t *l = lines;
   register int s;
   mobj_t* thing;

   for(i = 0; i < numlines; i++,l++)
   {
      switch(l->special)
      {
      case 224: // wind
         for(s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ; )
            Add_Pusher(p_wind,l->dx,l->dy,NULL,s);
         break;
      case 225: // current
         for(s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ; )
            Add_Pusher(p_current,l->dx,l->dy,NULL,s);
         break;
      case 226: // push/pull
         for(s = -1; (s = P_FindSectorFromLineTag(l,s)) >= 0 ; )
         {
            thing = P_GetPushThing(s);
            if(thing) // No P* means no effect
               Add_Pusher(p_push,l->dx,l->dy,thing,s);
         }
         break;
      }
   }
}

//
// P_SpawnHereticWind
//
// haleyjd 03/12/03: Heretic Wind/Current Transfer specials
//
static void P_SpawnHereticWind(line_t *line)
{
   int s;
   angle_t lineangle;
   fixed_t magnitude;

   lineangle = R_PointToAngle2(0, 0, line->dx, line->dy);
   magnitude = (P_AproxDistance(line->dx, line->dy)>>FRACBITS) * 512;

   for(s = -1; (s = P_FindSectorFromLineTag(line,s)) >= 0; )
   {
      // types 20-39 affect the player in P_PlayerThink
      // types 40-51 affect MF3_WINDTHRUST things in P_XYMovement
      // this is selected by use of lines 294 or 293, respectively

      sectors[s].hticPushType  = (line->special == 294) ? 20 : 40;
      sectors[s].hticPushAngle = lineangle;
      sectors[s].hticPushForce = magnitude;
   }
}

//
// phares 3/20/98: End of Pusher effects
//
////////////////////////////////////////////////////////////////////////////


//================================
//
// haleyjd 3/17/99: TerrainTypes
//
//================================

int numterraindefs;
int *TerrainTypes = NULL; // return to array model; optimization

// haleyjd 11/20/00: restructuring for purposes of adding a
// customizable data lump

typedef struct terraintype_s
{
   char  name[9];
   short type;
} terraintype_t; 

terraintype_t *TerrainTypeDefs = NULL;

/*
   haleyjd 11/20/00: added a really sweet feature, made
   TerrainTypes editable via the TERTYPES lump. Binary format, needs
   an external editor utility (which I have written). This code
   expects to find a lump with the number of terrain definitions,
   then for each, a 9-character NULL-extended string for the flat
   name, and a short describing what type of effect to trigger
   (see p_spec.h, the values are all the same).
*/

void P_LoadTerrainTypeDefs(void)
{
   short *shrtptr, temp;
   int lumpnum, i, j;
   char *chrptr, name[9];

   if((lumpnum = W_CheckNumForName("TERTYPES")) == -1)
   {
      numterraindefs = 0;
      return;
   }
   else
   {
      shrtptr = W_CacheLumpNum(lumpnum, PU_CACHE);
      numterraindefs = SHORT(*shrtptr);
      shrtptr++;
      if(TerrainTypeDefs)
         Z_Free(TerrainTypeDefs);
      TerrainTypeDefs = 
         Z_Malloc((numterraindefs+1)*sizeof(terraintype_t), 
                  PU_STATIC, NULL);
      chrptr = (char *)shrtptr;
      
      for(i = 0; i < numterraindefs; i++)
      {
         // get null-terminated flat name
         for(j=0; j<9; j++)
            name[j] = *chrptr++;
         shrtptr = (short *)chrptr;
         temp = SHORT(*shrtptr);
         shrtptr++;
         strcpy(TerrainTypeDefs[i].name, name);
         
         if(temp < 0 || temp > MAXTERRAINDEF)
            temp = 0;
         TerrainTypeDefs[i].type = temp;
         chrptr = (char *)shrtptr;
      }

      strcpy(TerrainTypeDefs[numterraindefs].name, "END");
      TerrainTypeDefs[numterraindefs].type = -1;
   }
}

// FIXME: this wastes space by allocating an array for all flats,
// when a hash table could be used
void P_InitTerrainTypes(void)
{
   int i;
   int lump;
   int size;

   size = (numflats + 1) * sizeof(int);
   if(TerrainTypes)
      Z_Free(TerrainTypes);
   TerrainTypes = Z_Malloc(size, PU_STATIC, NULL);
   memset(TerrainTypes, 0, size);
   
   if(numterraindefs == 0) // no terrain types defined, leave zero
     return;

   for(i = 0; TerrainTypeDefs[i].type != -1; i++)
   {
      // haleyjd 07/01/99
      // Change this from R_FlatNumForName to W_CheckNumForName.
      // Restores Ultimate DOOM to functionality without an added 
      // flat wad.

      lump = (W_CheckNumForName)(TerrainTypeDefs[i].name, ns_flats);
      if(lump != -1)
      {
         TerrainTypes[lump-firstflat] = TerrainTypeDefs[i].type;
      }
   }
}

int P_GetThingFloorType(mobj_t *thing)
{
   // 07/03/99: function simplified by restoring initialized model
   // FIXME: TerrainTypes currently disabled for deep water, totally
   // broken -- needs some crazy fix
   if(!thing || 
      (demo_version >= 331 && 
       thing->subsector->sector->heightsec != -1))
      return FLOOR_SOLID;
   else
      return (TerrainTypes[thing->subsector->sector->floorpic]);
}

//
// haleyjd 06/21/02: function to get TerrainType from a point
//
int P_GetTerrainTypeForPt(fixed_t x, fixed_t y, int position)
{
   subsector_t *subsec = R_PointInSubsector(x, y);

   // can retrieve a TerrainType for either the floor or the
   // ceiling
   switch(position)
   {
   case 0:
      return (TerrainTypes[subsec->sector->floorpic]);
   case 1:
      return (TerrainTypes[subsec->sector->ceilingpic]);
   default:
      return FLOOR_SOLID;
   }
}

int P_GetSecTerrainType(sector_t *sector, int position)
{
   switch(position)
   {
   case 0:
      return (TerrainTypes[sector->floorpic]);
   case 1:
      return (TerrainTypes[sector->ceilingpic]);
   default:
      return FLOOR_SOLID;
   }
}

//
// P_TerrainFloorClip
//
// Returns whether a TerrainType should clip feet or not
//
boolean P_TerrainFloorClip(int tt)
{
   switch(tt)
   {
   case FLOOR_WATER:
   case FLOOR_LAVA:
   case FLOOR_SLUDGE:
      return true;
   default:
      return false;
   }
}

static void FloorWater(mobj_t *, fixed_t);
static void FloorSludge(mobj_t *, fixed_t);
static void FloorLava(mobj_t *, fixed_t);

int P_HitFloor(mobj_t *thing)
{
   int currentsec;   // fixes for deep water sectors
   fixed_t currentz;
   subsector_t *subsec;

   if(!thing) return FLOOR_SOLID; // just makes me feel better
   
   // 07/01/99 -- Aurikan suggested this fix for the bug involving splashing
   // being triggered on floors not contacted, as in when the player was
   // hanging over a ledge and was actually "in" the lower sector.

   subsec = R_PointInSubsector(thing->x, thing->y);
   currentsec = subsec->sector->heightsec;
   
   if(thing->floorz != subsec->sector->floorheight)
     return FLOOR_SOLID;

   // add low-mass things here -- haleyjd: add demo compatibility
   if((thing->flags2 & MF2_NOSPLASH) || demo_version < 329)
     return FLOOR_SOLID;

   // haleyjd 04/29/99: fix deep water sectors
   if(currentsec != -1)
   {
      currentz = sectors[currentsec].floorheight;
   }
   else
   {
      currentz = ONFLOORZ;
   }

   switch(P_GetThingFloorType(thing))
   {
   case FLOOR_WATER:
      FloorWater(thing, currentz);
      return FLOOR_WATER;
   case FLOOR_LAVA:
      FloorLava(thing, currentz);
      return FLOOR_LAVA;
   case FLOOR_SLUDGE:
      FloorSludge(thing, currentz);
      return FLOOR_SLUDGE;
   default:
      break;
   }
   return FLOOR_SOLID;
}

// TerrainType implementor functions

//
// FLOOR_WATER
//
static void FloorWater(mobj_t *thing, fixed_t currentz)
{
   mobj_t *mo;
   int temp;

   mo = P_SpawnMobj(thing->x, thing->y, currentz, 
                    E_SafeThingType(MT_SPLASHBASE));
   S_StartSound(mo, sfx_gloop);
   
   mo = P_SpawnMobj(thing->x, thing->y, currentz, 
                    E_SafeThingType(MT_SPLASH));
   P_SetTarget(&mo->target, thing);
   
   // haleyjd 11/20/00: remove dependence on order of evaluation
   temp = P_Random(pr_splash);
   mo->momx = (temp - P_Random(pr_splash))<<8;
   temp = P_Random(pr_splash);
   mo->momy = (temp - P_Random(pr_splash))<<8;
   mo->momz = 2*FRACUNIT+(P_Random(pr_splash)<<8);
}

//
// FLOOR_LAVA
//
static void FloorLava(mobj_t *thing, fixed_t currentz)
{
   mobj_t *mo;

   mo = P_SpawnMobj(thing->x, thing->y, currentz, 
                    E_SafeThingType(MT_LAVASPLASH));
   S_StartSound(mo, sfx_burn);

   mo = P_SpawnMobj(thing->x, thing->y, currentz, 
                    E_SafeThingType(MT_LAVASMOKE));
   mo->momz = FRACUNIT + (P_Random(pr_splash)<<7);
}

//
// FLOOR_SLUDGE
//
static void FloorSludge(mobj_t *thing, fixed_t currentz)
{
   mobj_t *mo;
   int temp;

   mo = P_SpawnMobj(thing->x, thing->y, currentz, 
                    E_SafeThingType(MT_SLUDGEBASE));
   S_StartSound(mo, sfx_muck);
   
   mo = P_SpawnMobj(thing->x, thing->y, currentz, 
                    E_SafeThingType(MT_SLUDGECHUNK));
   P_SetTarget(&mo->target, thing);
   
   // haleyjd 11/20/00: remove dependence on order of evaluation
   temp = P_Random(pr_splash);
   mo->momx = (temp - P_Random(pr_splash))<<8;
   temp = P_Random(pr_splash);
   mo->momy = (temp - P_Random(pr_splash))<<8;
   mo->momz = FRACUNIT+(P_Random(pr_splash)<<8);
}


//==========================
//
// haleyjd: Misc New Stuff
//
//==========================

/*
   P_FindLine:  a much nicer line finding function.
     Probably slower, but probably easier to use and 
     understand :P
*/

line_t *P_FindLine(int tag, int *searchPosition)
{
   int i;
   
   for(i = *searchPosition+1; i < numlines; i++)
   {
      if(lines[i].tag == tag)
      {
         *searchPosition = i;
         return &lines[i];
      }
   }
   *searchPosition = -1;
   return NULL;
}

//===============================================================
//
// 3D Sides
//
// SoM: New functions to facilitate scrolling of 3d sides to make
// use as doors/lifts
//
//===============================================================

//
// SoM 9/19/2002
// P_Scroll3DSides
//
// Runs through the given attached sector list and scrolls both
// sides of any linedef it finds with same tag.
//
boolean P_Scroll3DSides(int tag, int numattach, int *attached, fixed_t delta, boolean crush)
{
   boolean  ok = true;
   sector_t *sec;
   int      i, p;
   line_t   *line;

   // Go through the sectors list one sector at a time.
   // Move any qualifying linedef's side offsets up/down based
   // on delta. 
   for(i = 0; i < numattach; i++)
   {
      if(attached[i] < 0 || attached[i] >= numsectors)
         I_Error("P_Scroll3DSides: attached[i] is not a valid sector index.\n");

      sec = sectors + attached[i];
      for(p = 0; p < sec->linecount; p++)
      {
         line = sec->lines[p];
         if(line->tag != tag ||
            !(line->flags & (ML_TWOSIDED|ML_3DMIDTEX)) ||
            line->sidenum[0] == -1 ||
            line->sidenum[1] == -1)
            continue;

         sides[line->sidenum[0]].rowoffset += delta;
         sides[line->sidenum[1]].rowoffset += delta;
      }

      if(P_CheckSector(sec, crush))
         ok = false;
   }

   return ok;
}

//
// SoM 9/19/2002
// P_AttachSectors
//
// Attaches all sectors that have lines with same tag as cline to
// cline's front sector.
//
void P_AttachSectors(line_t *cline, boolean ceiling)
{
   static int maxattach = 0;
   static int numattach = 0;
   static int *attached = NULL;

   int start = 0, i;
   line_t *line;

   if(!cline->frontsector)
      return;

   numattach = 0;

   // Check to insure that this sector doesn't already 
   // have attachments.
   if(!ceiling)
   {
      if(cline->frontsector->f_numattached)
         return;
   }
   else
   {
      if(cline->frontsector->c_numattached)
         return;
   }

   // Search the lines list. Check for every tagged line that
   // has the 3dmidtex lineflag, then add the front and back sectors
   // to the attached list.
   for (start=-1; (start = P_FindLineFromLineTag(cline,start)) >= 0;)
      if (start != cline-lines)
      {
         line = lines+start;

         if(!line->frontsector || !line->backsector ||
            !(line->flags & ML_3DMIDTEX))
            continue;

         for(i = 0; i < numattach;i++)
         {
            if(line->frontsector - sectors == attached[i])
            break;
         }

         if(i == numattach)
         {
            if(numattach == maxattach)
            {
              maxattach += 5;

              attached = (int *)realloc(attached, sizeof(int) * maxattach);
            }

            attached[numattach++] = line->frontsector-sectors;
         }
         
         // SoM 12/8/02: Don't attach the backsector.
      }

   // Copy the list to the c_attached or f_attached list.
   if(ceiling)
   {
      cline->frontsector->c_numattached = numattach;
      cline->frontsector->c_attached = Z_Malloc(sizeof(int) * numattach, PU_LEVEL, 0);
      memcpy(cline->frontsector->c_attached, attached, sizeof(int) * numattach);
      cline->frontsector->c_tag = cline->tag;
   }
   else
   {
      cline->frontsector->f_numattached = numattach;
      cline->frontsector->f_attached = Z_Malloc(sizeof(int) * numattach, PU_LEVEL, 0);
      memcpy(cline->frontsector->f_attached, attached, sizeof(int) * numattach);
      cline->frontsector->f_tag = cline->tag;
   }
}

//
// P_ConvertHereticSpecials
//
// haleyjd 08/14/02
// This function converts old Heretic levels to a BOOM-compatible
// format. Conveniently, BOOM already implements all the new
// Heretic line types, although some as generalized lines.
//
void P_ConvertHereticSpecials(void)
{
   int i;
   line_t *line;
   sector_t *sector;

   fixed_t pushForces[5] = 
   { 
      2048*5,  2048*10, 2048*25, 2048*30, 2048*35
   };

   // FIXME: try to verify that these are as close as possible --
   // if they are not, new line types might be appropriate
   // convert heretic line specials
   for(i = 0; i < numlines; i++)
   {
      line = &(lines[i]);

      switch(line->special)
      {
      case 99:  // Texture scroll right
         line->special = 85;
         break;
      case 100: // WR raise door turbo -- FIXME: needs spd VDOORSPEED*3
         line->special = 0x3d91; // gen type 15761
         break;
      case 105: // W1 secret exit
         line->special = 124;
         break;
      case 106: // W1 build stairs 16 (normal speed) -- FIXME: needs FLOORSPEED
         line->special = 0x3188; // gen type 12680
         break;
      case 107: // S1 build stairs 16 (normal) -- FIXME: needs FLOORSPEED
         line->special = 0x318a; // gen type 12682
         break;
      default:
         break;
      }
   }

   // sector types
   for(i = 0; i < numsectors; i++)
   {
      sector = &(sectors[i]);

      switch(sector->special)
      {
      case 4: // scroll east lava damage
         // blinks 0.5 seconds, 5 per 15 damage, heretic-style
         sector->special = 2 | (2 << DAMAGE_SHIFT) | HTIC_DMG_MASK;
         // heretic current pusher type
         sector->hticPushType  = 20;
         sector->hticPushAngle = 0;
         sector->hticPushForce = 2048*28;
         // scrolls to the east
         Add_Scroller(sc_floor, (-FRACUNIT/2)<<3, 0, -1,
                      sector - sectors, 0);
         continue;
      case 5: // handle heretic nukage types
         sector->special = (2 << DAMAGE_SHIFT) | HTIC_DMG_MASK;
         continue;
      case 7:
         sector->special = (1 << DAMAGE_SHIFT) | HTIC_DMG_MASK;
         continue;
      case 16:
         sector->special = (3 << DAMAGE_SHIFT) | HTIC_DMG_MASK;
         continue;
      case 15: // low friction -- ice
         sector->friction = 0xf900;       // from heretic source
         sector->movefactor = 0x276;      // precomputed by me
         sector->special = FRICTION_MASK; // clear, set friction bit
         continue;
      default:
         break;
      }

      // 03/12/03: Heretic current and wind specials

      if(sector->special >= 20 && sector->special <= 24)
      {
         // Scroll_East
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = 0;
         sector->hticPushForce = pushForces[sector->special - 20];         
         Add_Scroller(sc_floor, (-FRACUNIT/2)<<(sector->special - 20),
                      0, -1, sector-sectors, 0);
         sector->special = 0;
      }
      else if(sector->special >= 25 && sector->special <= 29)
      {
         // Scroll_North
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = ANG90;
         sector->hticPushForce = pushForces[sector->special - 25];
         sector->special = 0;
      }
      else if(sector->special >= 30 && sector->special <= 34)
      {
         // Scroll_South
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = ANG270;
         sector->hticPushForce = pushForces[sector->special - 30];
         sector->special = 0;
      }
      else if(sector->special >= 35 && sector->special <= 39)
      {
         // Scroll_West
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = ANG180;
         sector->hticPushForce = pushForces[sector->special - 35];
         sector->special = 0;
      }
      else if(sector->special >= 40 && sector->special <= 42)
      {
         // Wind_East
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = 0;
         sector->hticPushForce = pushForces[sector->special - 40];
         sector->special = 0;
      }
      else if(sector->special >= 43 && sector->special <= 45)
      {
         // Wind_North
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = ANG90;
         sector->hticPushForce = pushForces[sector->special - 43];
         sector->special = 0;
      }
      else if(sector->special >= 46 && sector->special <= 48)
      {
         // Wind_South
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = ANG270;
         sector->hticPushForce = pushForces[sector->special - 46];
         sector->special = 0;
      }
      else if(sector->special >= 49 && sector->special <= 51)
      {
         // Wind_West
         sector->hticPushType  = sector->special;
         sector->hticPushAngle = ANG180;
         sector->hticPushForce = pushForces[sector->special - 49];
         sector->special = 0;
      }
   }
}

//
// Portals
//

#ifdef R_PORTALS

//
// P_SpawnPortal
//
// Code by SoM, functionalized by Quasar
// Spawns a portal and attaches it to floors and/or ceilings
// of appropriate sectors, and to lines with special 289.
//
static void P_SpawnPortal(line_t *line, 
                          portal_type type, 
                          portal_effect effects)
{
   sector_t  *sector;
   rportal_t *portal;
   mobj_t    *skycam;
   static int CamType = -1;
   int s;
   fixed_t deltax, deltay, deltaz;
   int anchortype; // SoM 3-10-04: new plan.

   if(!(sector = line->frontsector))
      return;

   // create the appropriate type of portal
   switch(type)
   {
   case portal_plane:
      portal = R_GetPlanePortal(&sector->ceilingpic, 
                                &sector->ceilingheight, 
                                &sector->lightlevel, 
                                &sector->ceiling_xoffs, 
                                &sector->ceiling_yoffs);
      break;
   case portal_horizon:
      portal = R_GetHorizonPortal(&sector->floorpic, &sector->ceilingpic, 
                                  &sector->floorheight, &sector->ceilingheight,
                                  &sector->lightlevel, &sector->lightlevel,
                                  &sector->floor_xoffs, &sector->floor_yoffs,
                                  &sector->ceiling_xoffs, &sector->ceiling_yoffs);
      break;
   case portal_skybox:
      // find the skybox camera object
      if(CamType == -1)
         CamType = E_ThingNumForName("EESkyboxCam");

      skycam = sector->thinglist;
      while(skycam)
      {
         if(skycam->type == CamType)
            break;
         skycam = skycam->snext;
      }
      if(!skycam)
      {
         C_Printf(FC_ERROR"Skybox found with no skybox camera\n");
         return;
      }
      
      portal = R_GetSkyBoxPortal(skycam);
      break;
   case portal_anchored:
      // determine proper anchor type (see below)
      if(line->special == 295 || line->special == 297)
         anchortype = 298;
      else
         anchortype = 299;

      // find anchor line
      for(s = -1; (s = P_FindLineFromLineTag(line, s)) >= 0; )
      {
         // SoM 3-10-04: Two different anchor linedef codes so I can tag 
         // two anchored portals to the same sector.
         if(line == &lines[s] || lines[s].special != anchortype)          
            continue;

         deltax = ((lines[s].v1->x + lines[s].v2->x) / 2) - ((line->v1->x + line->v2->x) / 2);
         deltay = ((lines[s].v1->y + lines[s].v2->y) / 2) - ((line->v1->y + line->v2->y) / 2);
         deltaz = 0; /// ???
         break;
      }
      if(s < 0)
      {
         C_Printf(FC_ERROR"No anchor line for portal.\n");
         return;
      }

      portal = R_GetAnchoredPortal(deltax, deltay, deltaz);
      break;
   default:
      I_Error("P_SpawnPortal: unknown portal type\n");
   }

   // attach portal to tagged sector floors/ceilings
   for(s = -1; (s = P_FindSectorFromLineTag(line, s)) >= 0; )
   {
      switch(effects)
      {
      case portal_ceiling:
         sectors[s].c_portal = portal;
         break;
      case portal_floor:
         sectors[s].f_portal = portal;
         break;
      case portal_both:
         sectors[s].c_portal = sectors[s].f_portal = portal;
         break;
      default:
         I_Error("P_SpawnPortal: unknown portal effect\n");
      }
   }

   // attach portal to like-tagged 289 lines
   for(s = -1; (s = P_FindLineFromLineTag(line, s)) >= 0; )
   {
      if(line == &lines[s] || lines[s].special != 289 ||
         (lines[s].backsector && lines[s].frontsector)) // ignore 2S lines
         continue;

      lines[s].portal = portal;
   }
}

#endif

//----------------------------------------------------------------------------
//
// $Log: p_spec.c,v $
// Revision 1.56  1998/05/25  10:40:30  killough
// Fix wall scrolling bug
//
// Revision 1.55  1998/05/23  10:23:32  jim
// Fix numeric changer loop corruption
//
// Revision 1.54  1998/05/11  06:52:56  phares
// Documentation
//
// Revision 1.53  1998/05/07  00:51:34  killough
// beautification
//
// Revision 1.52  1998/05/04  11:47:23  killough
// Add #include d_deh.h
//
// Revision 1.51  1998/05/04  02:22:06  jim
// formatted p_specs, moved a coupla routines to p_floor
//
// Revision 1.50  1998/05/03  22:06:30  killough
// Provide minimal required headers at top (no other changes)
//
// Revision 1.49  1998/04/17  18:57:51  killough
// fix comment
//
// Revision 1.48  1998/04/17  18:49:02  killough
// Fix lack of animation in flats
//
// Revision 1.47  1998/04/17  10:24:47  killough
// Add P_FindLineFromLineTag(), add CARRY_CEILING macro
//
// Revision 1.46  1998/04/14  18:49:36  jim
// Added monster only and reverse teleports
//
// Revision 1.45  1998/04/12  02:05:25  killough
// Add ceiling light setting, start ceiling carriers
//
// Revision 1.44  1998/04/06  11:05:23  jim
// Remove LEESFIXES, AMAP bdg->247
//
// Revision 1.43  1998/04/06  04:39:04  killough
// Make scroll carriers carry all things underwater
//
// Revision 1.42  1998/04/01  16:39:11  jim
// Fix keyed door message on gunfire
//
// Revision 1.41  1998/03/29  20:13:35  jim
// Fixed use of 2S flag in Donut linedef
//
// Revision 1.40  1998/03/28  18:13:24  killough
// Fix conveyor bug (carry objects not touching but overhanging)
//
// Revision 1.39  1998/03/28  05:32:48  jim
// Text enabling changes for DEH
//
// Revision 1.38  1998/03/23  18:38:48  jim
// Switch and animation tables now lumps
//
// Revision 1.37  1998/03/23  15:24:41  phares
// Changed pushers to linedef control
//
// Revision 1.36  1998/03/23  03:32:36  killough
// Make "oof" sounds have true mobj origins (for spy mode hearing)
// Make carrying floors carry objects hanging over edges of sectors
//
// Revision 1.35  1998/03/20  14:24:36  jim
// Gen ceiling target now shortest UPPER texture
//
// Revision 1.34  1998/03/20  00:30:21  phares
// Changed friction to linedef control
//
// Revision 1.33  1998/03/18  23:14:02  jim
// Deh text additions
//
// Revision 1.32  1998/03/16  15:43:33  killough
// Add accelerative scrollers, merge Jim's changes
//
// Revision 1.29  1998/03/13  14:05:44  jim
// Fixed arith overflow in some linedef types
//
// Revision 1.28  1998/03/12  21:54:12  jim
// Freed up 12 linedefs for use as vectors
//
// Revision 1.26  1998/03/09  10:57:55  jim
// Allowed Lee's change to 0 tag trigger compatibility
//
// Revision 1.25  1998/03/09  07:23:43  killough
// Add generalized scrollers, renumber some linedefs
//
// Revision 1.24  1998/03/06  12:34:39  jim
// Renumbered 300+ linetypes under 256 for DCK
//
// Revision 1.23  1998/03/05  16:59:10  jim
// Fixed inability of monsters/barrels to use new teleports
//
// Revision 1.22  1998/03/04  07:33:04  killough
// Fix infinite loop caused by multiple carrier references
//
// Revision 1.21  1998/03/02  15:32:57  jim
// fixed errors in numeric model sector search and 0 tag trigger defeats
//
// Revision 1.20  1998/03/02  12:13:57  killough
// Add generalized scrolling flats & walls, carrying floors
//
// Revision 1.19  1998/02/28  01:24:53  jim
// Fixed error in 0 tag trigger fix
//
// Revision 1.17  1998/02/24  08:46:36  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.16  1998/02/23  23:47:05  jim
// Compatibility flagged multiple thinker support
//
// Revision 1.15  1998/02/23  04:52:33  killough
// Allow god mode cheat to work on E1M8 unless compatibility
//
// Revision 1.14  1998/02/23  00:42:02  jim
// Implemented elevators
//
// Revision 1.12  1998/02/17  05:55:06  killough
// Add silent teleporters
// Change RNG calling sequence
// Cosmetic changes
//
// Revision 1.11  1998/02/13  03:28:06  jim
// Fixed W1,G1 linedefs clearing untriggered special, cosmetic changes
//
// Revision 1.10  1998/02/08  05:35:39  jim
// Added generalized linedef types
//
// Revision 1.8  1998/02/02  13:34:26  killough
// Performance tuning, program beautification
//
// Revision 1.7  1998/01/30  14:43:54  jim
// Added gun exits, right scrolling walls and ceiling mover specials
//
// Revision 1.4  1998/01/27  16:19:29  jim
// Fixed subroutines used by linedef triggers and a NULL ref in Donut
//
// Revision 1.3  1998/01/26  19:24:26  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/25  20:24:45  jim
// Fixed crusher floor, lowerandChange floor types, and unknown sector special error
//
// Revision 1.1.1.1  1998/01/19  14:03:01  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
