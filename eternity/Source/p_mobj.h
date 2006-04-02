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
//      Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------

#ifndef __P_MOBJ__
#define __P_MOBJ__

typedef struct mobj_s mobj_t;   //sf: move up here

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

// sf: skins
#include "p_skin.h"

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
    // Hmm ???.         -- well, what? sf  -- ty halderman again, probably -- haleyjd
    MF_TRANSSHIFT       = 26,

    MF_TOUCHY = 0x10000000,        // killough 11/98: dies when solids touch it
    MF_BOUNCES = 0x20000000,       // killough 7/11/98: for beta BFG fireballs
    MF_FRIEND = 0x40000000,        // killough 7/18/98: friendly monsters

    // Translucent sprite?                                          // phares
    MF_TRANSLUCENT      = 0x80000000,                               // phares
} mobjflag_t;

typedef enum
{
  // haleyjd 04/09/99: extended mobj flags
  // More of these will be filled in as I add support.
  MF2_LOGRAV        = 0x00000001,  // subject to low gravity
  MF2_NOSPLASH      = 0x00000002,  // does not splash
  MF2_NOSTRAFE      = 0x00000004,  // never strafes
  MF2_NORESPAWN     = 0x00000008,  // never respawns
  MF2_ALWAYSRESPAWN = 0x00000010,  // respawns in ALL difficulties
  MF2_REMOVEDEAD    = 0x00000020,  // removed shortly after death
  MF2_NOTHRUST      = 0x00000040,  // not affected by push/pull/wind/current
  MF2_NOCROSS       = 0x00000080,  // cannot trigger special lines
  MF2_JUMPDOWN      = 0x00000100,  // if friend, can jump down
  MF2_PUSHABLE 	    = 0x00000200,  // can be pushed by moving things
  MF2_MAP07BOSS1    = 0x00000400,  // is a MAP07 boss type 1
  MF2_MAP07BOSS2    = 0x00000800,  // is a MAP07 boss type 2
  MF2_E1M8BOSS      = 0x00001000,  // is an E1M8 boss 
  MF2_E2M8BOSS      = 0x00002000,  // is an E2M8 boss
  MF2_E3M8BOSS      = 0x00004000,  // is an E3M8 boss
  MF2_BOSS 	    = 0x00008000,  // is a boss
  MF2_E4M6BOSS      = 0x00010000,  // is an E4M6 boss
  MF2_E4M8BOSS      = 0x00020000,  // is an E4M8 boss
  MF2_FOOTCLIP      = 0x00040000,  // feet are clipped by liquids
  MF2_FLOATBOB 	    = 0x00080000,  // uses floatbob z movement
  MF2_DONTDRAW 	    = 0x00100000,  // doesn't generate vissprite
  MF2_SHORTMRANGE   = 0x00200000,  // has short missile range
  MF2_LONGMELEE     = 0x00400000,  // has long melee range
  MF2_RANGEHALF     = 0x00800000,  // uses half actual distance
  MF2_HIGHERMPROB   = 0x01000000,  // min prob. of miss. att. = 37.5% vs 22%
  MF2_CANTLEAVEFLOORPIC = 0x02000000,  // restricted to current floorpic
  MF2_SPAWNFLOAT    = 0x04000000,  // random initial z coordinate
  MF2_INVULNERABLE  = 0x08000000,  // invulnerable to damage
  MF2_DORMANT       = 0x10000000,  // dormant (internal)
  MF2_SEEKERMISSILE = 0x20000000,  // might use tracer effects
  MF2_DEFLECTIVE    = 0x40000000,  // deflects projectiles
  MF2_REFLECTIVE    = 0x80000000,  // reflects projectiles
} mobjflag2_t;

// haleyjd 11/03/02: flags3 -- even more stuff!
typedef enum
{
   MF3_GHOST        = 0x00000001,  // heretic ghost effect
   MF3_THRUGHOST    = 0x00000002,  // object passes through ghosts
   MF3_NODMGTHRUST  = 0x00000004,  // don't thrust target on damage
   MF3_ACTSEESOUND  = 0x00000008,  // use seesound as activesound 50% of time
   MF3_LOUDACTIVE   = 0x00000010,  // play activesound full-volume
   MF3_E5M8BOSS     = 0x00000020,  // thing is heretic E5M8 boss
   MF3_DMGIGNORED   = 0x00000040,  // other things ignore its attacks
   MF3_BOSSIGNORE   = 0x00000080,  // attacks ignored if both have flag
   MF3_SLIDE        = 0x00000100,  // slides against walls
   MF3_TELESTOMP    = 0x00000200,  // thing can telefrag other things
   MF3_WINDTHRUST   = 0x00000400,  // affected by heretic wind sectors
   MF3_FIREDAMAGE   = 0x00000800,  // does fire damage
   MF3_KILLABLE     = 0x00001000,  // mobj is killable, but doesn't count
   MF3_DEADFLOAT    = 0x00002000,  // NOGRAVITY isn't removed on death
   MF3_NOTHRESHOLD  = 0x00004000,  // has no target threshold
   MF3_FLOORMISSILE = 0x00008000,  // is a floor missile
   MF3_SUPERITEM    = 0x00010000,  // is a super powerup item
   MF3_NOITEMRESP   = 0x00020000,  // item doesn't respawn
   MF3_SUPERFRIEND  = 0x00040000,  // monster won't attack friends
   MF3_INVULNCHARGE = 0x00080000,  // invincible when skull flying
   MF3_EXPLOCOUNT   = 0x00100000,  // doesn't explode until this expires
   MF3_CANNOTPUSH   = 0x00200000,  // thing can't push pushable things
} mobjflag3_t;

// killough 9/15/98: Same, but internal flags, not intended for .deh
// (some degree of opaqueness is good, to avoid compatibility woes)

enum
{
   MIF_FALLING     = 0x00000001, // Object is falling
   MIF_ARMED       = 0x00000002, // Object is armed (for MF_TOUCHY objects)
   MIF_LINEDONE    = 0x00000004, // Object has activated W1 or S1 linedef via DEH frame
   MIF_DIEDFALLING = 0x00000008, // haleyjd: object died by falling
   MIF_ONFLOOR     = 0x00000010, // SoM: object stands on floor
   MIF_ONSECFLOOR  = 0x00000020, // SoM: Object stands on sector floor *specific*
   MIF_SCREAMED    = 0x00000040, // haleyjd: player has screamed
   MIF_NOFACE      = 0x00000080, // haleyjd: thing won't face its target
   MIF_CRASHED     = 0x00000100, // haleyjd: thing has entered crashstate
};

// ammo + weapon in a dropped backpack 

typedef struct
{
        short ammo[NUMAMMO];
        char weapon;
} backpack_t;
  
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

// killough 9/8/98: changed some fields to shorts,
// for better memory usage (if only for cache).

struct mobj_s
{
  // List: thinker links.
  thinker_t           thinker;
  
  // Info for drawing: position.
  fixed_t             x;
  fixed_t             y;
  fixed_t             z;
  
  // More list: links in sector (if needed)
  struct mobj_s*      snext;
  struct mobj_s**     sprev; // killough 8/10/98: change to ptr-to-ptr

  //More drawing info: to determine current sprite.
  angle_t             angle;  // orientation
  spritenum_t         sprite; // used to find patch_t and flip value
  int                 frame;  // might be ORed with FF_FULLBRIGHT

  // Interaction info, by BLOCKMAP.
  // Links in blocks (if needed).
  struct mobj_s*      bnext;
  struct mobj_s**     bprev; // killough 8/11/98: change to ptr-to-ptr
  
  struct subsector_s* subsector;
  
  // The closest interval over all contacted Sectors.
  fixed_t             floorz;
  fixed_t             ceilingz;

  // killough 11/98: the lowest floor over all contacted Sectors.
  fixed_t             dropoffz;

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

  int colour; // sf: the sprite colour

  union
  {
    long           bfgcount;
    backpack_t*    backpack;       // for if its a backpack
  } extradata;

  int                 tics;   // state tic counter
  state_t*            state;
  unsigned long       flags;
  unsigned long       flags2;    // haleyjd 04/09/99: I know, kill me now
  unsigned long       flags3;    // haleyjd 11/03/02
  int                 intflags;  // killough 9/15/98: internal flags
  int                 health;

  // Movement direction, movement generation (zig-zagging).
  short               movedir;        // 0-7
  short               movecount;      // when 0, select a new dir
  short               strafecount;    // killough 9/8/98: monster strafing
  
  // Thing being chased/attacked (or NULL),
  // also the originator for missiles.
  struct mobj_s*      target;

  // Reaction time: if non 0, don't attack yet.
  // Used by player to freeze a bit after teleporting.
  short               reactiontime;   

  // If >0, the current target will be chased no
  // matter what (even if shot by another object)
  short               threshold;

  // killough 9/9/98: How long a monster pursues a target.
  short               pursuecount;
  
  short               gear; // killough 11/98: used in torque simulation
  
  // Additional info record for player avatars only.
  // Only valid if thing is a player
  struct player_s*    player;
  skin_t *           skin;   //sf: skin
  
  // Player number last looked for.
  short               lastlook;       
  
  // For nightmare respawn.
  mapthing_t          spawnpoint;     
  
  // Thing being chased/attacked for tracers.
  struct mobj_s*      tracer; 
  
  // new field: last known enemy -- killough 2/15/98
  struct mobj_s*      lastenemy;
  
  // killough 8/2/98: friction properties part of sectors,
  // not objects -- removed friction properties from here
  
  // a linked list of sectors where this object appears
  struct msecnode_s* touching_sectorlist;                 // phares 3/14/98
  
  // SEE WARNING ABOVE ABOUT POINTER FIELDS!!!

  // New Fields for Eternity -- haleyjd
  
  // specials -- these are used by some codepointers to maintain state
  // Note: these are now known as "counters" on the user-side of things.
  short special1;
  short special2;
  short special3;

  int effects;      // particle effect flag field
  int translucency; // zdoom-style translucency level
  int floatbob;     // floatbob offset
  int floorsec;     // # of sector responsible for floorz
  int damage;       // haleyjd 08/02/04: copy damage to mobj now
  fixed_t floorclip;    // haleyjd 08/07/04: floor clip amount

  #ifdef OVER_UNDER
  fixed_t secfloorz;
  fixed_t secceilz;

  // SoM 11/6/02: Yet again! Two more z values that must be stored
  // in the mobj struct 9_9
  // These are the floor and ceiling heights given by the first
  // clipping pass (map architecture + 3d sides).
  fixed_t passfloorz;
  fixed_t passceilz;
  #endif

  // scripting fields
  long args[5];       // arguments
  unsigned short tid; // thing id used by scripts
  mobj_t *tid_next;   // ptr to next thing in tid chain
  mobj_t **tid_prevn; // ptr to last thing's next pointer
};

        // put it here, it works, ok?
#include "d_player.h"

// External declarations (formerly in p_local.h) -- killough 5/2/98

#define VIEWHEIGHT      (41*FRACUNIT)
#define PLAYERRADIUS    (16*FRACUNIT)

// sf: gravity >>> defaultgravity
#define DEFAULTGRAVITY  FRACUNIT
#define MAXMOVE         (30*FRACUNIT)

#define ONFLOORZ        D_MININT
#define ONCEILINGZ      D_MAXINT
// haleyjd 10/13/02: floatrand 
#define FLOATRANDZ     (D_MAXINT - 1)
#define MINFLTRNDZ     (40*FRACUNIT)

// Time interval for item respawning.
#define ITEMQUESIZE     128

#define FLOATSPEED      (FRACUNIT*4)
#define STOPSPEED       (FRACUNIT/16)

                // convert the looking up/down angle to a slope for firing
                        // projectiles (slope=angle*LOOKSLOPE)
// #define LOOKSLOPE 680
#define LOOKSLOPE 800

// killough 11/98:
// For torque simulation:

#define OVERDRIVE 6
#define MAXGEAR (OVERDRIVE+16)

// killough 11/98:
// Whether an object is "sentient" or not. Used for environmental influences.
#define sentient(mobj) ((mobj)->health > 0 && (mobj)->info->seestate)

// haleyjd 11/28/02: default z coord addend for missile spawn
#define DEFAULTMISSILEZ (4*8*FRACUNIT)

extern mapthing_t itemrespawnque[];
extern int itemrespawntime[];
extern int iquehead;
extern int iquetail;

void    P_RespawnSpecials(void);
mobj_t  *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);
void    P_RemoveMobj(mobj_t *th);
boolean P_SetMobjState(mobj_t *mobj, statenum_t state);
void    P_MobjThinker(mobj_t *mobj);
void    P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z, angle_t dir, int updown, boolean ptcl);
void    P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, angle_t dir, int damage, mobj_t *target);
mobj_t  *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type, fixed_t z);
mobj_t  *P_SpawnPlayerMissile(mobj_t *source, mobjtype_t type);
mobj_t  *P_SpawnMapThing (mapthing_t*  mthing);
void    P_CheckMissileSpawn(mobj_t*);  // killough 8/2/98
void    P_ExplodeMissile(mobj_t*);    // killough

// particles and lines: sf
/*
void P_SpawnParticle(fixed_t x, fixed_t y, fixed_t z);
void P_ParticleLine(mobj_t *source, mobj_t *dest);
*/

// new Eternity mobj function prototypes  haleyjd
void    P_Massacre(int friends); // haleyjd 1/22/99:  kills everything
boolean P_SetMobjStateNF(mobj_t *mobj, statenum_t state); // sets state without calling action function
mobj_t  *P_SpawnMissileAngle(mobj_t *source, mobjtype_t type, angle_t angle, fixed_t momz, fixed_t z);  // cleaner angled firing
void    P_ThrustMobj(mobj_t *mo, angle_t angle, fixed_t move);
fixed_t P_MissileMomz(fixed_t, fixed_t, fixed_t, int);

void P_InitTIDHash(void);
void P_AddThingTID(mobj_t *mo, int tid);
void P_RemoveThingTID(mobj_t *mo);

void P_AdjustFloorClip(mobj_t *thing);

// Thing Collections

typedef struct MobjCollection_s
{
   int type;
   int num;
   int numalloc;
   mobj_t **ptrarray;

   int wrapiterator;
} MobjCollection_t;

void P_InitMobjCollection(MobjCollection_t *, int);
void P_ReInitMobjCollection(MobjCollection_t *, int);
void P_ClearMobjCollection(MobjCollection_t *);
void P_CollectThings(MobjCollection_t *);
boolean P_CollectionIsEmpty(MobjCollection_t *);
mobj_t *P_CollectionWrapIterator(MobjCollection_t *);
mobj_t *P_CollectionGetRandom(MobjCollection_t *, int);

// end new Eternity mobj functions

#endif

//----------------------------------------------------------------------------
//
// $Log: p_mobj.h,v $
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
