/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_mobj.h,v 1.1 2000/05/04 08:12:52 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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
 *      Map Objects, MObj, definition and handling.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __P_MOBJ__
#define __P_MOBJ__

// Basics.
#include "tables.h"
#include "m_fixed.h"

// We need the thinker_t stuff.
#include "d_think.h"

// We need the WAD data structure for Map things,
// from the THINGS lump.
#include "doomdata.h"

// States are tied to finite states are
//  tied to animation frames.
// Needs precompiled tables/data structures.
#include "info.h"

#ifdef __GNUG__
#pragma interface
#endif

//
// NOTES: mobj_t
//
// mobj_ts are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are allmost allways set
// from state_t structures.
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the mobj_t.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when mobj_ts are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The mobj_t->flags element has various bit flags
// used by the simulation.
//
// Every mobj_t is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with R_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any mobj_t that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable mobj_t that has its origin contained.  
//
// A valid mobj_t is a mobj_t that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO? flags while a thing is valid.
//
// Any questions?
//

//
// Misc. mobj flags
//
typedef enum
{
    // Call P_SpecialThing when touched.
    MF_SPECIAL          = 1,
    // Blocks.
    MF_SOLID            = 2,
    // Can be hit.
    MF_SHOOTABLE        = 4,
    // Don't use the sector links (invisible but touchable).
    MF_NOSECTOR         = 8,
    // Don't use the blocklinks (inert but displayable)
    MF_NOBLOCKMAP       = 16,                    

    // Not to be activated by sound, deaf monster.
    MF_AMBUSH           = 32,
    // Will try to attack right back.
    MF_JUSTHIT          = 64,
    // Will take at least one step before attacking.
    MF_JUSTATTACKED     = 128,
    // On level spawning (initial position),
    //  hang from ceiling instead of stand on floor.
    MF_SPAWNCEILING     = 256,
    // Don't apply gravity (every tic),
    //  that is, object will float, keeping current height
    //  or changing it actively.
    MF_NOGRAVITY        = 512,

    // Movement flags.
    // This allows jumps from high places.
    MF_DROPOFF          = 0x400,
    // For players, will pick up items.
    MF_PICKUP           = 0x800,
    // Player cheat. ???
    MF_NOCLIP           = 0x1000,
    // Player: keep info about sliding along walls.
    MF_SLIDE            = 0x2000,
    // Allow moves to any height, no gravity.
    // For active floaters, e.g. cacodemons, pain elementals.
    MF_FLOAT            = 0x4000,
    // Don't cross lines
    //   ??? or look at heights on teleport.
    MF_TELEPORT         = 0x8000,
    // Don't hit same species, explode on block.
    // Player missiles as well as fireballs of various kinds.
    MF_MISSILE          = 0x10000,      
    // Dropped by a demon, not level spawned.
    // E.g. ammo clips dropped by dying former humans.
    MF_DROPPED          = 0x20000,
    // Use fuzzy draw (shadow demons or spectres),
    //  temporary player invisibility powerup.
    MF_SHADOW           = 0x40000,
    // Flag: don't bleed when shot (use puff),
    //  barrels and shootable furniture shall not bleed.
    MF_NOBLOOD          = 0x80000,
    // Don't stop moving halfway off a step,
    //  that is, have dead bodies slide down all the way.
    MF_CORPSE           = 0x100000,
    // Floating to a height for a move, ???
    //  don't auto float to target's height.
    MF_INFLOAT          = 0x200000,

    // On kill, count this enemy object
    //  towards intermission kill total.
    // Happy gathering.
    MF_COUNTKILL        = 0x400000,
    
    // On picking up, count this item object
    //  towards intermission item total.
    MF_COUNTITEM        = 0x800000,

    // Special handling: skull in flight.
    // Neither a cacodemon nor a missile.
    MF_SKULLFLY         = 0x1000000,

    // Don't spawn this object
    //  in death match mode (e.g. key cards).
    MF_NOTDMATCH        = 0x2000000,

    // Player sprites in multiplayer modes are modified
    //  using an internal color lookup table for re-indexing.
    // If 0x4 0x8 or 0xc,
    //  use a translation table for player colormaps
    MF_TRANSLATION      = 0xc000000,
    // Hmm ???.
    MF_TRANSSHIFT       = 26,

    // Translucent sprite?                                          // phares
    MF_TRANSLUCENT      = 0x80000000                                // phares

} mobjflag_t;


// Map Object definition.
//
// killough 2/20/98:
//
// WARNING: Special steps must be taken in p_saveg.c if C pointers are added to
// this mobj_s struct, or else savegames will crash when loaded. See p_saveg.c.
// Do not add "struct mobj_s *fooptr" without adding code to p_saveg.c to
// convert the pointers to ordinals and back for savegames. This was the whole
// reason behind monsters going to sleep when loading savegames (the "target"
// pointer was simply nullified after loading, to prevent Doom from crashing),
// and the whole reason behind loadgames crashing on savegames of AV attacks.
// 

typedef struct mobj_s
{
    // List: thinker links.
    thinker_t           thinker;

    // Info for drawing: position.
    fixed_t             x;
    fixed_t             y;
    fixed_t             z;

    // More list: links in sector (if needed)
    struct mobj_s*      snext;
    struct mobj_s*      sprev;

    //More drawing info: to determine current sprite.
    angle_t             angle;  // orientation
    spritenum_t         sprite; // used to find patch_t and flip value
    int                 frame;  // might be ORed with FF_FULLBRIGHT

    // Interaction info, by BLOCKMAP.
    // Links in blocks (if needed).
    struct mobj_s*      bnext;
    struct mobj_s*      bprev;
    
    struct subsector_s* subsector;

    // The closest interval over all contacted Sectors.
    fixed_t             floorz;
    fixed_t             ceilingz;

    // For movement checking.
    fixed_t             radius;
    fixed_t             height; 

    // Momentums, used to update position.
    fixed_t             momx;
    fixed_t             momy;
    fixed_t             momz;

    // If == validcount, already checked.
    int                 validcount;

    mobjtype_t          type;
    mobjinfo_t*         info;   // &mobjinfo[mobj->type]
    
    int                 tics;   // state tic counter
    state_t*            state;
    int                 flags;
    int                 health;

    // Movement direction, movement generation (zig-zagging).
    int                 movedir;        // 0-7
    int                 movecount;      // when 0, select a new dir

    // Thing being chased/attacked (or NULL),
    // also the originator for missiles.
    struct mobj_s*      target;

    // Reaction time: if non 0, don't attack yet.
    // Used by player to freeze a bit after teleporting.
    int                 reactiontime;   

    // If >0, the target will be chased
    // no matter what (even if shot)
    int                 threshold;

    // Additional info record for player avatars only.
    // Only valid if type == MT_PLAYER
    struct player_s*    player;

    // Player number last looked for.
    int                 lastlook;       

    // For nightmare respawn.
    mapthing_t          spawnpoint;     

    // Thing being chased/attacked for tracers.
    struct mobj_s*      tracer; 

    // new field: last known enemy -- killough 2/15/98
    struct mobj_s*      lastenemy;

    // Are we above a Thing? above_thing points to the Thing        // phares
    // if so, otherwise it's zero.                                  //   |
                                                                    //   V
    struct mobj_s* above_thing;

    // Are we below a Thing? below_thing points to the Thing
    // if so, otherwise it's zero.
                                                                    //   ^
    struct mobj_s* below_thing;                                     //   |
                                                                    // phares
    // Friction values for the sector the object is in
    int friction;                                           // phares 3/17/98
    int movefactor;

    // a linked list of sectors where this object appears
    struct msecnode_s* touching_sectorlist;                 // phares 3/14/98

    // SEE WARNING ABOVE ABOUT POINTER FIELDS!!!

    int references;

} mobj_t;

// External declarations (fomerly in p_local.h) -- killough 5/2/98

#define VIEWHEIGHT      (41*FRACUNIT)
#define PLAYERRADIUS    (16*FRACUNIT)

#define GRAVITY         FRACUNIT
#define MAXMOVE         (30*FRACUNIT)

#define ONFLOORZ        INT_MIN
#define ONCEILINGZ      INT_MAX

// Time interval for item respawning.
#define ITEMQUESIZE     128

#define FLOATSPEED      (FRACUNIT*4)
#define STOPSPEED       (FRACUNIT/16)

extern mapthing_t itemrespawnque[];
extern int itemrespawntime[];
extern int iquehead;
extern int iquetail;

void    P_RespawnSpecials(void);
mobj_t  *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);
void    P_RemoveMobj(mobj_t *th);
boolean P_SetMobjState(mobj_t *mobj, statenum_t state);
void    P_MobjThinker(mobj_t *mobj);
void    P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z);
void    P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage);
mobj_t  *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type);
void    P_SpawnPlayerMissile(mobj_t *source, mobjtype_t type);
void    P_SpawnMapThing (mapthing_t*  mthing);

#endif

//----------------------------------------------------------------------------
//
// $Log: p_mobj.h,v $
// Revision 1.1  2000/05/04 08:12:52  proff_fs
// Initial revision
//
// Revision 1.4  1999/10/12 13:01:15  cphipps
// Changed header to GPL
//
// Revision 1.3  1999/02/04 15:32:14  cphipps
// Add field for counting pointer references to mobj_t's
//
// Revision 1.2  1999/01/25 15:53:20  cphipps
// Use newer limit macros from limits.h instead of depreciated values.h macros
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.10  1998/05/03  23:45:09  killough
// beautification, fix headers, declarations
//
// Revision 1.9  1998/03/23  15:24:33  phares
// Changed pushers to linedef control
//
// Revision 1.8  1998/03/20  00:30:09  phares
// Changed friction to linedef control
//
// Revision 1.7  1998/03/09  18:27:13  phares
// Fixed bug in neighboring variable friction sectors
//
// Revision 1.6  1998/02/24  08:46:24  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.5  1998/02/20  21:56:34  phares
// Preliminarey sprite translucency
//
// Revision 1.4  1998/02/20  09:51:14  killough
// Add savegame warning
//
// Revision 1.3  1998/02/17  05:48:16  killough
// Add new last enemy field to prevent monster sleepiness
//
// Revision 1.2  1998/01/26  19:27:23  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
