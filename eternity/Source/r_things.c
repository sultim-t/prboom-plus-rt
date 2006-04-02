// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2001 James Haley
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
//  Refresh of things represented by sprites --
//  i.e. map objects and particles.
//
//  Particle code largely from zdoom, thanks to Randy Heit.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: r_things.c,v 1.22 1998/05/03 22:46:41 killough Exp $";

#include "c_io.h"
#include "doomstat.h"
#include "w_wad.h"
#include "g_game.h"
#include "d_main.h"
#include "p_skin.h"
#include "r_main.h"
#include "r_bsp.h"
#include "r_segs.h"
#include "r_draw.h"
#include "r_things.h"
#include "m_argv.h"
#include "p_user.h"
#include "e_edf.h"
#include "p_info.h"

#define MINZ        (FRACUNIT*4)
#define BASEYCENTER 100

#define HTIC_GHOST_TRANS 26624

extern int columnofs[];

typedef struct {
  int x1;
  int x2;
  int column;
  int topclip;
  int bottomclip;
} maskdraw_t;

#ifdef R_PORTALS
// top and bottom of portal silhouette
static short portaltop[MAX_SCREENWIDTH];
static short portalbottom[MAX_SCREENWIDTH];

static short *ptop, *pbottom;

//
// R_SetMaskedSilhouette
//
void R_SetMaskedSilhouette(short *top, short *bottom)
{
   if(!top || !bottom)
   {
      register short *topp = portaltop, *bottomp = portalbottom, *stopp = portaltop + MAX_SCREENWIDTH;
      register short *tp = top, *bp = bottom;

      while(topp < stopp)
      {
         *topp++ = -1;
         *bottomp++ = viewheight;
      }
   }
   else
   {
      memcpy(portaltop, top, sizeof(short) * MAX_SCREENWIDTH);
      memcpy(portalbottom, bottom, sizeof(short) * MAX_SCREENWIDTH);
   }
}
#endif

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//

extern int global_cmap_index; // haleyjd: NGCS

short pscreenheightarray[MAX_SCREENWIDTH]; // for psprites

fixed_t pspritescale;
fixed_t pspriteiscale;
fixed_t pspriteyscale; // ANYRES
fixed_t pspriteiyscale;

static lighttable_t **spritelights;        // killough 1/25/98 made static

// constant arrays
//  used for psprite clipping and initializing clipping

short negonearray[MAX_SCREENWIDTH];        // killough 2/8/98:
short screenheightarray[MAX_SCREENWIDTH];  // change to MAX_*
int lefthanded=0;

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up and range check thing_t sprites patches

spritedef_t *sprites;
int numsprites;

#define MAX_SPRITE_FRAMES 29          /* Macroized -- killough 1/25/98 */

static spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
static int maxframe;

// haleyjd: global particle system state

int        numParticles;
int        activeParticles;
int        inactiveParticles;
particle_t *Particles;
int        particle_trans;

//
// R_InstallSpriteLump
//
// Local function for R_InitSprites.
//
static void R_InstallSpriteLump(int lump, unsigned frame,
                                unsigned rotation, boolean flipped)
{
   if(frame >= MAX_SPRITE_FRAMES || rotation > 8)
      I_Error("R_InstallSpriteLump: Bad frame characters in lump %i", lump);

   if((int)frame > maxframe)
      maxframe = frame;

   if(rotation == 0)
   {    // the lump should be used for all rotations
      int r;
      for(r = 0; r < 8 ; ++r)
         if(sprtemp[frame].lump[r]==-1)
         {
            sprtemp[frame].lump[r] = lump - firstspritelump;
            sprtemp[frame].flip[r] = (byte) flipped;
            sprtemp[frame].rotate = false; //jff 4/24/98 if any subbed, rotless
         }
      return;
   }

   // the lump is only used for one rotation
   
   if(sprtemp[frame].lump[--rotation] == -1)
   {
      sprtemp[frame].lump[rotation] = lump - firstspritelump;
      sprtemp[frame].flip[rotation] = (byte) flipped;
      sprtemp[frame].rotate = true; //jff 4/24/98 only change if rot used
   }
}

//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
// (4 chars exactly) to be used.
//
// Builds the sprite rotation matrixes to account
// for horizontally flipped sprites.
//
// Will report an error if the lumps are inconsistent.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
//
// A sprite that is flippable will have an additional
//  letter/number appended.
//
// The rotation character can be 0 to signify no rotations.
//
// 1/25/98, 1/31/98 killough : Rewritten for performance
//
// Empirically verified to have excellent hash
// properties across standard Doom sprites:

#define R_SpriteNameHash(s) ((unsigned)((s)[0]-((s)[1]*3-(s)[3]*2-(s)[2])*2))

void R_InitSpriteDefs(char **namelist)
{
   size_t numentries = lastspritelump-firstspritelump+1;
   struct { int index, next; } *hash;
   unsigned int i;
      
   if(!numentries || !*namelist)
      return;
   
   // count the number of sprite names
   for(i = 0; namelist[i]; ++i)
      ; // do nothing
   
   numsprites = (signed)i;

   sprites = Z_Malloc(numsprites *sizeof(*sprites), PU_STATIC, NULL);
   
   // Create hash table based on just the first four letters of each sprite
   // killough 1/31/98
   
   hash = malloc(sizeof(*hash)*numentries); // allocate hash table

   for(i = 0; i < numentries; ++i) // initialize hash table as empty
      hash[i].index = -1;

   for(i = 0; i < numentries; ++i)  // Prepend each sprite to hash chain
   {                                // prepend so that later ones win
      int j = R_SpriteNameHash(lumpinfo[i+firstspritelump]->name) % numentries;
      hash[i].next = hash[j].index;
      hash[j].index = i;
   }

   // scan all the lump names for each of the names,
   //  noting the highest frame letter.

   for(i = 0; i < (unsigned)numsprites; ++i)
   {
      const char *spritename = namelist[i];
      int j = hash[R_SpriteNameHash(spritename) % numentries].index;
      
      if(j >= 0)
      {
         memset(sprtemp, -1, sizeof(sprtemp));
         maxframe = -1;
         do
         {
            register lumpinfo_t *lump = lumpinfo[j + firstspritelump];

            // Fast portable comparison -- killough
            // (using int pointer cast is nonportable):

            if(!((lump->name[0] ^ spritename[0]) |
                 (lump->name[1] ^ spritename[1]) |
                 (lump->name[2] ^ spritename[2]) |
                 (lump->name[3] ^ spritename[3])))
            {
               R_InstallSpriteLump(j+firstspritelump,
                                   lump->name[4] - 'A',
                                   lump->name[5] - '0',
                                   false);
               if(lump->name[6])
                  R_InstallSpriteLump(j+firstspritelump,
                                      lump->name[6] - 'A',
                                      lump->name[7] - '0',
                                      true);
            }
         }
         while((j = hash[j].next) >= 0);

         // check the frames that were found for completeness
         if((sprites[i].numframes = ++maxframe))  // killough 1/31/98
         {
            int frame;
            for (frame = 0; frame < maxframe; frame++)
            {
               switch((int)sprtemp[frame].rotate)
               {
               case -1:
                  // no rotations were found for that frame at all
                  I_Error ("R_InitSprites: No patches found "
                     "for %.8s frame %c", namelist[i], frame+'A');
                  break;
                  
               case 0:
                  // only the first rotation is needed
                  break;
                  
               case 1:
                  // must have all 8 frames
                  {
                     int rotation;
                     for(rotation=0 ; rotation<8 ; rotation++)
                     {
                        if(sprtemp[frame].lump[rotation] == -1)
                           I_Error ("R_InitSprites: Sprite %.8s frame %c "
                                    "is missing rotations",
                                    namelist[i], frame+'A');
                     }
                     break;
                  }
               }
            }
            // allocate space for the frames present and copy sprtemp to it
            sprites[i].spriteframes =
               Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
            memcpy(sprites[i].spriteframes, sprtemp,
                   maxframe*sizeof(spriteframe_t));
         }
      }
      else
      {
         // haleyjd 08/15/02: problem here.
         // If j was -1 above, meaning that there are no lumps for
         // the sprite present, the sprite is left uninitialized.
         // This creates major problems in R_PrecacheLevel if a 
         // thing tries to subsequently use that sprite.
         
         sprites[i].numframes = 0;
         sprites[i].spriteframes = NULL;
      }
   }
   free(hash);             // free hash table
}

//
// GAME FUNCTIONS
//

static vissprite_t *vissprites, **vissprite_ptrs;  // killough
static size_t num_vissprite, num_vissprite_alloc, num_vissprite_ptrs;

//
// R_InitSprites
// Called at program start.
//

void R_InitSprites(char **namelist)
{
   int i;
   for(i = 0; i < MAX_SCREENWIDTH; ++i)    // killough 2/8/98
      negonearray[i] = -1;
   R_InitSpriteDefs(namelist);
}

//
// R_ClearSprites
//
// Called at frame start.
//
void R_ClearSprites(void)
{
   num_vissprite = 0; // killough
}

#ifdef R_PORTALS
// SoM 12/13/03: the masked stack
static maskedstack_t *mstack = NULL;
static int stacksize = 0, stackmax = 0;

static int firstds, lastds;
static int firstsprite, lastsprite;

void R_PushMasked(void)
{
   if(stacksize == stackmax)
   {
      stackmax += 10;
      mstack = realloc(mstack, sizeof(maskedstack_t) * stackmax);
   }

   if(!stacksize)
   {
      mstack[0].firstds = mstack[0].firstsprite = 0;
   }
   else
   {
      mstack[stacksize].firstds = mstack[stacksize-1].lastds;
      mstack[stacksize].firstsprite = mstack[stacksize-1].lastsprite;
   }

   // SoM: project the particles to be included with the sprites
   /*
   if(drawparticles)
   {
      int i = activeParticles;
      while(i != -1)
      {
         R_ProjectParticle(Particles + i);
         i = Particles[i].next;
      }
   }
   */

   mstack[stacksize].lastds = ds_p - drawsegs;
   mstack[stacksize].lastsprite = num_vissprite;

   memcpy(mstack[stacksize].ceilingclip, portaltop, MAX_SCREENWIDTH * sizeof(short));
   memcpy(mstack[stacksize].floorclip, portalbottom, MAX_SCREENWIDTH * sizeof(short));
   stacksize ++;
}
#endif

//
// R_NewVisSprite
//
// Creates a new vissprite if needed, or recycles an unused one.
//
vissprite_t *R_NewVisSprite(void)
{
   if(num_vissprite >= num_vissprite_alloc)             // killough
   {
      num_vissprite_alloc = num_vissprite_alloc ? num_vissprite_alloc*2 : 128;
      vissprites = realloc(vissprites,num_vissprite_alloc*sizeof(*vissprites));
   }
   return vissprites + num_vissprite++;
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//

short   *mfloorclip;
short   *mceilingclip;
fixed_t spryscale;
fixed_t sprtopscreen;

void R_DrawMaskedColumn(column_t *column)
{
   int topscreen, bottomscreen;
   fixed_t basetexturemid = dc_texturemid;
   
   dc_texheight = 0; // killough 11/98

   while(column->topdelta != 0xff)
   {
      // calculate unclipped screen coordinates for post
      topscreen = sprtopscreen + spryscale*column->topdelta;
      bottomscreen = topscreen + spryscale*column->length;

      // Here's where "sparkles" come in -- killough:
      dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
      dc_yh = (bottomscreen-1)>>FRACBITS;
      
      if(dc_yh >= mfloorclip[dc_x])
         dc_yh = mfloorclip[dc_x]-1;
      
      if(dc_yl <= mceilingclip[dc_x])
         dc_yl = mceilingclip[dc_x]+1;

      // killough 3/2/98, 3/27/98: Failsafe against overflow/crash:
      if(dc_yl <= dc_yh && dc_yh < viewheight)
      {
         dc_source = (byte *) column + 3;
         dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);

         // Drawn by either R_DrawColumn
         //  or (SHADOW) R_DrawFuzzColumn.
         colfunc();
      }
      column = (column_t *)((byte *)column + column->length + 4);
   }
   dc_texturemid = basetexturemid;
}

//
// R_DrawVisSprite
//
//  mfloorclip and mceilingclip should also be set.
//
void R_DrawVisSprite(vissprite_t *vis, int x1, int x2)
{
   column_t *column;
   int      texturecolumn;
   fixed_t  frac;
   patch_t  *patch;
   boolean  footclipon = false;
   short    baseclip = 0;
   
   if(vis->patch == -1)
   {
      // this vissprite belongs to a particle
      R_DrawParticle(vis);
      return;
   }
  
   patch = W_CacheLumpNum(vis->patch+firstspritelump, PU_CACHE);
   
   dc_colormap = vis->colormap;
   
   // killough 4/11/98: rearrange and handle translucent sprites
   // mixed with translucent/non-translucent 2s normals
   
   // sf: shadow draw now done by mobj flags, not a null colormap
   
   if(vis->mobjflags & MF_SHADOW)   // shadow draw
   {
      colfunc = R_DrawFuzzColumn;    // killough 3/14/98
   }
   else if(vis->mobjflags3 & MF3_TLSTYLEADD)
   {
      // haleyjd 02/08/05: additive translucency support
      if(vis->colour)
      {
         colfunc = R_DrawAddTlatedColumn;
         dc_translation = translationtables[vis->colour - 1];
      }
      else
         colfunc = R_DrawAddColumn;

      dc_translevel = vis->translucency;
   }
   else if(vis->translucency < FRACUNIT && general_translucency)
   {
      // haleyjd: zdoom-style translucency
      // 01/12/04: changed translation handling
      if(vis->colour)
      {
         colfunc = R_DrawFlexTlatedColumn;
         dc_translation = translationtables[vis->colour - 1];
      }
      else
         colfunc = R_DrawFlexTLColumn;

      dc_translevel = vis->translucency;
   }
   else if(vis->mobjflags & MF_TRANSLUCENT && general_translucency) // phares
   {
      // haleyjd 02/08/05: allow translated BOOM tl columns too
      if(vis->colour)
      {
         colfunc = R_DrawTLTlatedColumn;
         dc_translation = translationtables[vis->colour - 1];
      }
      else
         colfunc = R_DrawTLColumn;
      
      tranmap = main_tranmap; // killough 4/11/98
   }
   else if(vis->colour)
   {
      // haleyjd 01/12/04: changed translation handling
      colfunc = R_DrawTranslatedColumn;
      dc_translation = translationtables[vis->colour - 1];
   }
   else
      colfunc = R_DrawColumn;         // killough 3/14/98, 4/11/98

   dc_iscale = FixedDiv(FRACUNIT, vis->scale);
   dc_texturemid = vis->texturemid;
   frac = vis->startfrac;
   spryscale = vis->scale;
   sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);
   
   // haleyjd 10/10/02: foot clipping
   if(vis->footclip)
   {
      fixed_t sprbotscreen = 
         sprtopscreen + FixedMul(SHORT(patch->height)<<FRACBITS, spryscale);

      footclipon = true;
      
      baseclip = (sprbotscreen - FixedMul(vis->footclip, spryscale))>>FRACBITS;
   }

   // haleyjd: use a separate loop for footclip things, to minimize
   // overhead for regular sprites and to require no separate loop
   // just to update mfloorclip
   if(footclipon)
   {
      for(dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
      {
         // haleyjd: if baseclip is higher than mfloorclip for this
         // column, swap it in for that column
         if(baseclip < mfloorclip[dc_x] - 1)
            mfloorclip[dc_x] = baseclip + 1;

         texturecolumn = frac>>FRACBITS;
         
#ifdef RANGECHECK
         if(texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
            I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
         
         column = (column_t *)((byte *) patch +
            LONG(patch->columnofs[texturecolumn]));
         R_DrawMaskedColumn (column);
      }
   }
   else
   {
      for(dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
      {
         texturecolumn = frac>>FRACBITS;
         
#ifdef RANGECHECK
         if(texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
            I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
         
         column = (column_t *)((byte *) patch +
            LONG(patch->columnofs[texturecolumn]));
         R_DrawMaskedColumn (column);
      }
   }
   colfunc = R_DrawColumn;         // killough 3/14/98
}

//
// R_ProjectSprite
//
// Generates a vissprite for a thing if it might be visible.
//
void R_ProjectSprite (mobj_t* thing)
{
   fixed_t   tr_x, tr_y;
   fixed_t   gxt, gyt;
   fixed_t   gzt;            // killough 3/27/98
   fixed_t   tx, tz;
   fixed_t   xscale, yscale; // ANYRES
   int       x1, x2;
   spritedef_t   *sprdef;
   spriteframe_t *sprframe;
   int       lump;
   boolean   flip;
   vissprite_t *vis;
   fixed_t   iscale;
   int heightsec;            // killough 3/27/98

   // haleyjd 04/18/99: MF2_DONTDRAW
   //         09/01/02: zdoom-style translucency
   if((thing->flags2 & MF2_DONTDRAW) || !thing->translucency)
      return; // don't generate vissprite

   // transform the origin point
   tr_x = thing->x - viewx;
   tr_y = thing->y - viewy;

   gxt =  FixedMul(tr_x, viewcos);
   gyt = -FixedMul(tr_y, viewsin);

   tz = gxt - gyt;
   
   // thing is behind view plane?
   if(tz < MINZ)
      return;

   xscale = FixedDiv(projection, tz);
   yscale = FixedMul(yaspectmul, xscale);
   
   gxt = -FixedMul(tr_x,viewsin);
   gyt = FixedMul(tr_y,viewcos);

   tx = -(gyt+gxt);
   
   // too far off the side?
   if(D_abs(tx)>(tz<<2))
      return;

   // decide which patch to use for sprite relative to player
   if((unsigned)thing->sprite >= (unsigned)numsprites)
   {
      // haleyjd 08/12/02: modified error handling
      doom_printf(FC_ERROR"R_ProjectSprite: invalid sprite number %i\n",
                  thing->sprite);
      thing->info->translucency = 0;
      thing->translucency = 0;
      return;
   }

   sprdef = &sprites[thing->sprite];
   
   if(((thing->frame&FF_FRAMEMASK) >= sprdef->numframes) ||
      !(sprdef->spriteframes))
   {
      // haleyjd 08/12/02: modified error handling
      doom_printf(FC_ERROR"R_ProjectSprite: invalid frame %i for sprite %s",
                  thing->frame & FF_FRAMEMASK, 
                  spritelist[thing->sprite]);
      thing->info->translucency = 0;
      thing->translucency = 0;
      return;
   }

   sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];
   
   if(sprframe->rotate)
   {
      // choose a different rotation based on player view
      angle_t ang = R_PointToAngle(thing->x, thing->y);
      unsigned rot = (ang - thing->angle + (unsigned)(ANG45/2)*9) >> 29;
      lump = sprframe->lump[rot];
      flip = (boolean)sprframe->flip[rot];
   }
   else
   {
      // use single rotation for all views
      lump = sprframe->lump[0];
      flip = (boolean) sprframe->flip[0];
   }

   // calculate edges of the shape  
   // haleyjd 11/17/03: flip x offset when the sprite is flipped;
   // this problem was originally identified by fraggle
   // tx -= spriteoffset[lump];
   tx -= flip ? spritewidth[lump] - spriteoffset[lump] : spriteoffset[lump];
   x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;

   // off the right side?
   if(x1 > viewwidth)
      return;
   
   tx += spritewidth[lump];
   x2 = ((centerxfrac + FixedMul(tx, xscale)) >> FRACBITS) - 1;

   // off the left side
   if(x2 < 0)
      return;

   gzt = thing->z + spritetopoffset[lump];
   
   // killough 4/9/98: clip things which are out of view due to height
   // sf : fix for look up/down
   //        centeryfrac=(viewheight<<(FRACBITS-1));
   if(thing->z > viewz + FixedDiv((viewheight<<FRACBITS), xscale) ||
      gzt      < viewz - FixedDiv((viewheight<<FRACBITS)-viewheight, xscale))
      return;

   // killough 3/27/98: exclude things totally separated
   // from the viewer, by either water or fake ceilings
   // killough 4/11/98: improve sprite clipping for underwater/fake ceilings

   heightsec = thing->subsector->sector->heightsec;
   
   if(heightsec != -1)   // only clip things which are in special sectors
   {
      // haleyjd: and yet ANOTHER assumption!
      int phs = viewcamera ? viewcamera->heightsec :
                   viewplayer->mo->subsector->sector->heightsec;
      if(phs != -1 && viewz < sectors[phs].floorheight ?
           thing->z >= sectors[heightsec].floorheight :
           gzt < sectors[heightsec].floorheight)
         return;
      if(phs != -1 && viewz > sectors[phs].ceilingheight ?
           gzt < sectors[heightsec].ceilingheight &&
           viewz >= sectors[heightsec].ceilingheight :
           thing->z >= sectors[heightsec].ceilingheight)
         return;
   }

   // store information in a vissprite
   vis = R_NewVisSprite();
   
   // killough 3/27/98: save sector for special clipping later   
   vis->heightsec = heightsec;
   
   vis->mobjflags  = thing->flags;
   vis->mobjflags3 = thing->flags3; // haleyjd
   vis->colour = thing->colour;
   vis->scale = yscale; // SoM: ANYRES //xscale;
   vis->gx = thing->x;
   vis->gy = thing->y;
   vis->gz = thing->z;
   vis->gzt = gzt;                          // killough 3/27/98
   vis->x1 = x1 < 0 ? 0 : x1;
   vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
   iscale = FixedDiv(FRACUNIT, xscale);
   vis->translucency = thing->translucency; // haleyjd 09/01/02

   // haleyjd 11/14/02: ghost flag
   if(thing->flags3 & MF3_GHOST)
      vis->translucency = HTIC_GHOST_TRANS;

   // haleyjd 10/12/02: foot clipping
   vis->footclip = thing->floorclip;

   // haleyjd: moved this down, added footclip term
   vis->texturemid = gzt - viewz - vis->footclip;   

   if(flip)
   {
      vis->startfrac = spritewidth[lump]-1;
      vis->xiscale = -iscale;
   }
   else
   {
      vis->startfrac = 0;
      vis->xiscale = iscale;
   }

   if(vis->x1 > x1)
      vis->startfrac += vis->xiscale*(vis->x1-x1);
   vis->patch = lump;

   // get light level
   if(thing->flags & MF_SHADOW)     // sf
      vis->colormap = colormaps[global_cmap_index]; // haleyjd: NGCS -- was 0
   else if(fixedcolormap)
      vis->colormap = fixedcolormap;      // fixed map
   else if(LevelInfo.useFullBright && (thing->frame & FF_FULLBRIGHT)) // haleyjd
      vis->colormap = fullcolormap;       // full bright  // killough 3/20/98
   else
   {      // diminished light
      // SoM: ANYRES
      int index = xscale >> (LIGHTSCALESHIFT + addscaleshift);
      if(index >= MAXLIGHTSCALE)
         index = MAXLIGHTSCALE-1;
      vis->colormap = spritelights[index];
   }
}

//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
// killough 9/18/98: add lightlevel as parameter, fixing underwater lighting
//
void R_AddSprites(sector_t* sec, int lightlevel)
{
   mobj_t *thing;
   particle_t *ptcl;
   int    lightnum;
   
   // BSP is traversed by subsector.
   // A sector might have been split into several
   //  subsectors during BSP building.
   // Thus we check whether its already added.

   if(sec->validcount == validcount)
      return;
   
   // Well, now it will be done.
   sec->validcount = validcount;
   
   lightnum = (lightlevel >> LIGHTSEGSHIFT)+extralight;
   
   if(lightnum < 0)
      spritelights = scalelight[0];
   else if(lightnum >= LIGHTLEVELS)
      spritelights = scalelight[LIGHTLEVELS-1];
   else
      spritelights = scalelight[lightnum];
   
   // Handle all things in sector.
   
   for(thing = sec->thinglist; thing; thing = thing->snext)
      R_ProjectSprite(thing);

   // haleyjd 02/20/04: Handle all particles in sector.

   if(drawparticles)
   {
      for(ptcl = sec->ptcllist; ptcl; ptcl = (particle_t *)(ptcl->seclinks.next))
         R_ProjectParticle(ptcl);
   }
}

//
// R_DrawPSprite
//
// Draws player gun sprites.
//
void R_DrawPSprite(pspdef_t *psp)
{
  fixed_t       tx;
  int           x1, x2;
  spritedef_t   *sprdef;
  spriteframe_t *sprframe;
  int           lump;
  boolean       flip;
  vissprite_t   *vis;
  vissprite_t   avis;
  int           oldcentery;
  fixed_t       oldcenteryfrac;

  // haleyjd: total invis. psprite disable

  if(viewplayer->mo->flags2 & MF2_DONTDRAW)
    return;

  // decide which patch to use
  // haleyjd 08/14/02: should not be rangecheck, modified error
  // handling

  if((unsigned)psp->state->sprite >= (unsigned)numsprites)
  {
    doom_printf(FC_ERROR"R_DrawPSprite: invalid sprite number %i", 
                psp->state->sprite);
    psp->state->sprite = blankSpriteNum;
    psp->state->frame = 0;
  }

  sprdef = &sprites[psp->state->sprite];

  if(((psp->state->frame&FF_FRAMEMASK) >= sprdef->numframes) ||
     !(sprdef->spriteframes))
  {
    doom_printf(FC_ERROR"R_DrawPSprite: invalid frame %i for sprite %s",
                (int)(psp->state->frame & FF_FRAMEMASK),
                spritelist[psp->state->sprite]);
    psp->state->sprite = blankSpriteNum;
    psp->state->frame = 0;
    // reset sprdef
    sprdef = &sprites[psp->state->sprite];
  }

  sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];

  lump = sprframe->lump[0];
  flip = ( (boolean) sprframe->flip[0] ) ^ lefthanded;

  // calculate edges of the shape
  tx = psp->sx-160*FRACUNIT;
  tx -= spriteoffset[lump];

  x1 = (centerxfrac + FixedMul (tx,pspritescale))>>FRACBITS;

  // off the right side
  if (x1 > viewwidth)
    return;

  tx +=  spritewidth[lump];
  x2 = ((centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

  // off the left side
  if (x2 < 0)
    return;
 
  if(lefthanded)
  {
        int tmpx=x1;
        x1=viewwidth-x2;
        x2=viewwidth-tmpx;    // viewwidth-x1
  }

  // store information in a vissprite
  vis = &avis;
  vis->mobjflags = 0;
  vis->mobjflags3 = 0; // haleyjd

  // killough 12/98: fix psprite positioning problem
  vis->texturemid = (BASEYCENTER<<FRACBITS) /* + FRACUNIT/2 */ -
                    (psp->sy-spritetopoffset[lump]);

  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  vis->scale = pspriteyscale; // ANYRES
  vis->colour = 0;      // sf: default colourmap
  vis->translucency = FRACUNIT; // haleyjd: default zdoom trans.
  vis->footclip = 0; // haleyjd

  if (flip)
    {
      vis->xiscale = -pspriteiscale;
      vis->startfrac = spritewidth[lump]-1;
    }
  else
    {
      vis->xiscale = pspriteiscale;
      vis->startfrac = 0;
    }

  if (vis->x1 > x1)
    vis->startfrac += vis->xiscale*(vis->x1-x1);

  vis->patch = lump;

  if (viewplayer->powers[pw_invisibility] > 4*32
   || viewplayer->powers[pw_invisibility] & 8)
  {
           // sf: shadow draw now detected by flags
     vis->mobjflags |= MF_SHADOW;                    // shadow draw
     vis->colormap = colormaps[global_cmap_index]; // haleyjd: NGCS -- was 0
  }
  else if(viewplayer->powers[pw_ghost] > 4*32 || // haleyjd: ghost
          viewplayer->powers[pw_ghost] & 8)
  {
     vis->translucency = HTIC_GHOST_TRANS;
     vis->colormap = spritelights[MAXLIGHTSCALE-1];
  }
  else if (fixedcolormap)
    vis->colormap = fixedcolormap;           // fixed color
  else if (psp->state->frame & FF_FULLBRIGHT)
    vis->colormap = fullcolormap;            // full bright // killough 3/20/98
  else
    vis->colormap = spritelights[MAXLIGHTSCALE-1];  // local light

  if(psp->trans) // translucent gunflash
    vis->mobjflags |= MF_TRANSLUCENT;

  oldcentery = centery;
  centery = viewheight / 2;
  oldcenteryfrac = centeryfrac;
  centeryfrac = centery << FRACBITS;

  R_DrawVisSprite (vis, vis->x1, vis->x2);

  centery = oldcentery;
  centeryfrac = oldcenteryfrac;
}

//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites(void)
{
  int i, lightnum;
  pspdef_t *psp;
  sector_t tmpsec;
  int floorlightlevel, ceilinglightlevel;

        // sf: psprite switch
  if(!showpsprites || viewcamera) return;

  R_SectorColormap(viewplayer->mo->subsector->sector);

  // get light level
  // killough 9/18/98: compute lightlevel from floor and ceiling lightlevels
  // (see r_bsp.c for similar calculations for non-player sprites)

  R_FakeFlat(viewplayer->mo->subsector->sector, &tmpsec,
             &floorlightlevel, &ceilinglightlevel, 0);
  lightnum = ((floorlightlevel+ceilinglightlevel) >> (LIGHTSEGSHIFT+1))
    + extralight;

  if (lightnum < 0)
    spritelights = scalelight[0];
  else if (lightnum >= LIGHTLEVELS)
    spritelights = scalelight[LIGHTLEVELS-1];
  else
    spritelights = scalelight[lightnum];

  for(i=0;i<viewwidth;i++)
  {
    pscreenheightarray[i] = viewheight;
  }

  // clip to screen bounds
  mfloorclip = pscreenheightarray;
  mceilingclip = negonearray;

  // add all active psprites
  for (i=0, psp=viewplayer->psprites; i<NUMPSPRITES; i++,psp++)
    if (psp->state)
      R_DrawPSprite (psp);
}

//
// R_SortVisSprites
//
// Rewritten by Lee Killough to avoid using unnecessary
// linked lists, and to use faster sorting algorithm.
//

// killough 9/22/98: inlined memcpy of pointer arrays

/* Julian 6/7/2001

        1) cleansed macro layout
        2) remove esi,edi,ecx from cloberred regs since used in constraints
           (useless on old gcc, error maker on modern versions)
*/

#ifdef DJGPP

#define bcopyp(d, s, n) \
asm(\
" cld\n"\
"rep\n" \
"movsl" :\
: "D"(d), "S"(s), "c"(n) : "%cc")

#else

#define bcopyp(d, s, n) memcpy(d, s, (n) * sizeof(void *))

#endif

//
// killough 9/2/98: merge sort
//
static void msort(vissprite_t **s, vissprite_t **t, int n)
{
   if(n >= 16)
   {
      int n1 = n/2, n2 = n - n1;
      vissprite_t **s1 = s, **s2 = s + n1, **d = t;

      msort(s1, t, n1);
      msort(s2, t, n2);
      
      while((*s1)->scale > (*s2)->scale ?
            (*d++ = *s1++, --n1) : (*d++ = *s2++, --n2));

      if(n2)
         bcopyp(d, s2, n2);
      else
         bcopyp(d, s1, n1);
      
      bcopyp(s, t, n);
   }
   else
   {
      int i;
      for(i = 1; i < n; ++i)
      {
         vissprite_t *temp = s[i];
         if(s[i-1]->scale < temp->scale)
         {
            int j = i;
            while((s[j] = s[j-1])->scale < temp->scale && --j);
            s[j] = temp;
         }
      }
   }
}

void R_SortVisSprites(void)
{
   if(num_vissprite)
   {
      int i = num_vissprite;
      
      // If we need to allocate more pointers for the vissprites,
      // allocate as many as were allocated for sprites -- killough
      // killough 9/22/98: allocate twice as many
      
      if(num_vissprite_ptrs < num_vissprite*2)
      {
         free(vissprite_ptrs);  // better than realloc -- no preserving needed
         vissprite_ptrs = malloc((num_vissprite_ptrs = num_vissprite_alloc*2)
                                  * sizeof *vissprite_ptrs);
      }

      while(--i >= 0)
         vissprite_ptrs[i] = vissprites+i;

      // killough 9/22/98: replace qsort with merge sort, since the keys
      // are roughly in order to begin with, due to BSP rendering.
      
      msort(vissprite_ptrs, vissprite_ptrs + num_vissprite, num_vissprite);
   }
}


#ifdef R_PORTALS
//
// R_SortVisSpriteRange
//
// Sorts only a subset of the vissprites, for portal rendering.
//
void R_SortVisSpriteRange(int first, int last)
{
   unsigned int numsprites = last - first;
   
   if(numsprites > 0)
   {
      int i = numsprites;
      
      // If we need to allocate more pointers for the vissprites,
      // allocate as many as were allocated for sprites -- killough
      // killough 9/22/98: allocate twice as many
      
      if(num_vissprite_ptrs < numsprites*2)
      {
         free(vissprite_ptrs);  // better than realloc -- no preserving needed
         vissprite_ptrs = malloc((num_vissprite_ptrs = num_vissprite_alloc*2)
                                  * sizeof *vissprite_ptrs);
      }

      while(--i >= 0)
         vissprite_ptrs[i] = vissprites+i+first;

      // killough 9/22/98: replace qsort with merge sort, since the keys
      // are roughly in order to begin with, due to BSP rendering.
      
      msort(vissprite_ptrs, vissprite_ptrs + numsprites, numsprites);
   }
}
#endif

//
// R_DrawSprite
//
void R_DrawSprite (vissprite_t* spr)
{
   drawseg_t *ds;
   short   clipbot[MAX_SCREENWIDTH];       // killough 2/8/98:
   short   cliptop[MAX_SCREENWIDTH];       // change to MAX_*
   int     x;
   int     r1;
   int     r2;
   fixed_t scale;
   fixed_t lowscale;
   
   for(x = spr->x1; x <= spr->x2; ++x)
      clipbot[x] = cliptop[x] = -2;

   // Scan drawsegs from end to start for obscuring segs.
   // The first drawseg that has a greater scale is the clip seg.
   
   // Modified by Lee Killough:
   // (pointer check was originally nonportable
   // and buggy, by going past LEFT end of array):
   
   //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code

   for(ds = ds_p; ds-- > drawsegs; )  // new -- killough
   {
      // determine if the drawseg obscures the sprite
      if(ds->x1 > spr->x2 || ds->x2 < spr->x1 ||
         (!ds->silhouette && !ds->maskedtexturecol))
         continue; // does not cover sprite

      r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
      r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;
      
      if(ds->scale1 > ds->scale2)
      {
         lowscale = ds->scale2;
         scale = ds->scale1;
      }
      else
      {
         lowscale = ds->scale1;
         scale = ds->scale2;
      }

      if(scale < spr->scale || (lowscale < spr->scale &&
         !R_PointOnSegSide (spr->gx, spr->gy, ds->curline)))
      {
         if(ds->maskedtexturecol) // masked mid texture?
            R_RenderMaskedSegRange(ds, r1, r2);
         continue;                // seg is behind sprite
      }

      // clip this piece of the sprite
      // killough 3/27/98: optimized and made much shorter
      
       // bottom sil
      if(ds->silhouette & SIL_BOTTOM && spr->gz < ds->bsilheight)
         for(x = r1; x <= r2; ++x)
            if(clipbot[x] == -2)
               clipbot[x] = ds->sprbottomclip[x];

      // top sil
      if(ds->silhouette & SIL_TOP && spr->gzt > ds->tsilheight)
         for(x = r1; x <= r2; ++x)
            if(cliptop[x] == -2)
               cliptop[x] = ds->sprtopclip[x];
   }

   // killough 3/27/98:
   // Clip the sprite against deep water and/or fake ceilings.
   // killough 4/9/98: optimize by adding mh
   // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
   // killough 11/98: fix disappearing sprites
   
   if(spr->heightsec != -1)  // only things in specially marked sectors
   {
      fixed_t h,mh;
      
      // haleyjd: yet another instance of assumption that only players
      // can be involved in determining the z partition...
      int phs = viewcamera ? viewcamera->heightsec :
                   viewplayer->mo->subsector->sector->heightsec;

      if((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
         (h = centeryfrac - FixedMul(mh-=viewz, spr->scale)) >= 0 &&
         (h >>= FRACBITS) < viewheight)
      {
         if(mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
         {
            // clip bottom
            for(x = spr->x1; x <= spr->x2; ++x)
               if(clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
         }
         else  // clip top
            if(phs != -1 && viewz <= sectors[phs].floorheight) // killough 11/98
               for(x = spr->x1; x <= spr->x2; ++x)
                  if(cliptop[x] == -2 || h > cliptop[x])
                     cliptop[x] = h;
      }

      if((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
         (h = centeryfrac - FixedMul(mh-viewz, spr->scale)) >= 0 &&
         (h >>= FRACBITS) < viewheight)
      {
         if(phs != -1 && viewz >= sectors[phs].ceilingheight)
         {
            // clip bottom
            for(x = spr->x1; x <= spr->x2; ++x)
               if(clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
         }
         else  // clip top
            for(x = spr->x1; x <= spr->x2; ++x)
               if(cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = h;
      }
   }
   // killough 3/27/98: end special clipping for deep water / fake ceilings

   // all clipping has been performed, so draw the sprite
   // check for unclipped columns
   
   for(x = spr->x1; x <= spr->x2; ++x)
   {
      if(clipbot[x] == -2)
         clipbot[x] = viewheight;
      
      if(cliptop[x] == -2)
         cliptop[x] = -1;
   }

   mfloorclip = clipbot;
   mceilingclip = cliptop;
   R_DrawVisSprite (spr, spr->x1, spr->x2);
}


#ifdef R_PORTALS
//
// R_DrawSpriteInDSRange
//
// Draws a sprite within a given drawseg range, for portals.
//
void R_DrawSpriteInDSRange(vissprite_t* spr, int firstds, int lastds)
{
   drawseg_t *ds;
   short   clipbot[MAX_SCREENWIDTH];       // killough 2/8/98:
   short   cliptop[MAX_SCREENWIDTH];       // change to MAX_*
   int     x;
   int     r1;
   int     r2;
   fixed_t scale;
   fixed_t lowscale;

   for(x = spr->x1; x <= spr->x2; ++x)
      clipbot[x] = cliptop[x] = -2;

   // Scan drawsegs from end to start for obscuring segs.
   // The first drawseg that has a greater scale is the clip seg.
   
   for(ds = drawsegs + lastds; ds-- > drawsegs + firstds; )
   {      
      // determine if the drawseg obscures the sprite
      if(ds->x1 > spr->x2 || ds->x2 < spr->x1 ||
         (!ds->silhouette && !ds->maskedtexturecol))
         continue; // does not cover sprite

      r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
      r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

      if (ds->scale1 > ds->scale2)
      {
         lowscale = ds->scale2;
         scale = ds->scale1;
      }
      else
      {
         lowscale = ds->scale1;
         scale = ds->scale2;
      }

      if(scale < spr->scale || (lowscale < spr->scale &&
         !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
      {
         if(ds->maskedtexturecol) // masked mid texture?
            R_RenderMaskedSegRange(ds, r1, r2);
         continue;                // seg is behind sprite
      }
      
      // clip this piece of the sprite
      // killough 3/27/98: optimized and made much shorter

      // bottom sil
      if(ds->silhouette & SIL_BOTTOM && spr->gz < ds->bsilheight)
         for(x = r1; x <= r2; ++x)
            if(clipbot[x] == -2)
               clipbot[x] = ds->sprbottomclip[x];

      // top sil
      if(ds->silhouette & SIL_TOP && spr->gzt > ds->tsilheight)
         for(x = r1; x <= r2; ++x)
            if(cliptop[x] == -2)
               cliptop[x] = ds->sprtopclip[x];
   }

   // Clip the sprite against deep water and/or fake ceilings.

   if(spr->heightsec != -1) // only things in specially marked sectors
   {
      fixed_t h,mh;
      
      int phs = viewcamera ? viewcamera->heightsec :
                   viewplayer->mo->subsector->sector->heightsec;

      if((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
         (h = centeryfrac - FixedMul(mh-=viewz, spr->scale)) >= 0 &&
         (h >>= FRACBITS) < viewheight)
      {
         if(mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
         {
            // clip bottom
            for(x = spr->x1; x <= spr->x2; ++x)
               if(clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
         }
         else  // clip top
            if(phs != -1 && viewz <= sectors[phs].floorheight) // killough 11/98
               for(x = spr->x1; x <= spr->x2; ++x)
                  if(cliptop[x] == -2 || h > cliptop[x])
                     cliptop[x] = h;
      }

      if((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
         (h = centeryfrac - FixedMul(mh-viewz, spr->scale)) >= 0 &&
         (h >>= FRACBITS) < viewheight)
      {
         if(phs != -1 && viewz >= sectors[phs].ceilingheight)
         {
            // clip bottom
            for(x = spr->x1; x <= spr->x2; ++x)
               if(clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
         }
         else  // clip top
            for(x = spr->x1; x <= spr->x2; ++x)
               if(cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = h;
      }
   }
   // killough 3/27/98: end special clipping for deep water / fake ceilings

   // all clipping has been performed, so draw the sprite
   // check for unclipped columns
   
   for(x = spr->x1; x <= spr->x2; ++x)
   {
      if(clipbot[x] == -2 || clipbot[x] > pbottom[x])
         clipbot[x] = pbottom[x];
      
      if(cliptop[x] == -2 || cliptop[x] < ptop[x])
         cliptop[x] = ptop[x];
    }

   mfloorclip = clipbot;
   mceilingclip = cliptop;
   R_DrawVisSprite(spr, spr->x1, spr->x2);
}
#endif


//
// R_DrawMasked
//

#ifdef R_PORTALS
void R_DrawMasked(void)
{
   int firstds, lastds, firstsprite, lastsprite;
   int i;
   drawseg_t *ds;
 
   while(stacksize > 0)
   {
      --stacksize;

      firstds = mstack[stacksize].firstds;
      lastds  = mstack[stacksize].lastds;
      firstsprite = mstack[stacksize].firstsprite;
      lastsprite  = mstack[stacksize].lastsprite;

      R_SortVisSpriteRange(firstsprite, lastsprite);

      ptop    = mstack[stacksize].ceilingclip;
      pbottom = mstack[stacksize].floorclip;

      for(i = lastsprite - firstsprite; --i >= 0; )
         R_DrawSpriteInDSRange(vissprite_ptrs[i], firstds, lastds);         // killough

      // render any remaining masked mid textures

      // Modified by Lee Killough:
      // (pointer check was originally nonportable
      // and buggy, by going past LEFT end of array):

      for(ds=drawsegs + lastds ; ds-- > drawsegs + firstds; )  // new -- killough
         if(ds->maskedtexturecol)
            R_RenderMaskedSegRange(ds, ds->x1, ds->x2);
   }

   // draw the psprites on top of everything
   //  but does not draw on side views
   if(!viewangleoffset)
      R_DrawPlayerSprites();
}
#else
void R_DrawMasked(void)
{
  int i;
  drawseg_t *ds;

  if(drawparticles)
  {
     int i = activeParticles;
     while(i != -1)
     {
	R_ProjectParticle(Particles + i);
	i = Particles[i].next;
     }
  }

  R_SortVisSprites();

  // draw all vissprites back to front

  for(i = num_vissprite; --i >= 0; )
    R_DrawSprite(vissprite_ptrs[i]);         // killough

  // render any remaining masked mid textures

  // Modified by Lee Killough:
  // (pointer check was originally nonportable
  // and buggy, by going past LEFT end of array):

  //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code

  for(ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
    if(ds->maskedtexturecol)
      R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

  // draw the psprites on top of everything
  //  but does not draw on side views
  if(!viewangleoffset)
    R_DrawPlayerSprites();
}

#endif

//=====================================================================
//
// haleyjd 09/30/01
//
// Particle Rendering
// This incorporates itself mostly seamlessly within the
// vissprite system, incurring only minor changes to the functions
// above.

//
// newParticle
//
// Tries to find an inactive particle in the Particles list
// Returns NULL on failure
//
particle_t *newParticle(void)
{
   particle_t *result = NULL;
   if(inactiveParticles != -1)
   {
      result = Particles + inactiveParticles;
      inactiveParticles = result->next;
      result->next = activeParticles;
      activeParticles = result - Particles;
   }
   return result;
}

//
// R_InitParticles
//
// Allocate the particle list and initialize it
//
void R_InitParticles(void)
{
   int i;

   numParticles = 0;

   if((i = M_CheckParm("-numparticles")) && i < myargc - 1)
      numParticles = atoi(myargv[i+1]);
   
   if(numParticles == 0) // assume default
   {
      //numParticles = 4000; haleyjd 09/07/05: experiment
      numParticles = 2000;
   }
   else if(numParticles < 100)
   {
      numParticles = 100;
   }
   
   Particles = Z_Malloc(numParticles*sizeof(particle_t), PU_STATIC, NULL);
   R_ClearParticles();
}

//
// R_ClearParticles
//
// set up the particle list
//
void R_ClearParticles(void)
{
   int i;
   
   memset(Particles, 0, numParticles*sizeof(particle_t));
   activeParticles = -1;
   inactiveParticles = 0;
   for(i = 0; i < numParticles - 1; i++)
      Particles[i].next = i + 1;
   Particles[i].next = -1;
}

//
// R_ProjectParticle
//
void R_ProjectParticle(particle_t *particle)
{
   fixed_t tr_x, tr_y;
   fixed_t gxt, gyt, gzt;
   fixed_t tx, tz;
   fixed_t xscale, yscale;
   int x1, x2;
   vissprite_t*	vis;
   sector_t* sector = NULL;
   fixed_t iscale;
   int heightsec = -1;
   
   // transform the origin point
   tr_x = particle->x - viewx;
   tr_y = particle->y - viewy;
   
   gxt =  FixedMul(tr_x, viewcos); 
   gyt = -FixedMul(tr_y, viewsin);
   
   tz = gxt - gyt; 
   
   // particle is behind view plane?
   if(tz < MINZ)
      return;
   
   xscale = FixedDiv(projection, tz);
   yscale = FixedMul(yaspectmul, xscale); 
   
   gxt = -FixedMul(tr_x, viewsin); 
   gyt =  FixedMul(tr_y, viewcos); 
   tx  = -(gyt + gxt); 
   
   // too far off the side?
   if(D_abs(tx) > (tz << 2))
      return;
   
   // calculate edges of the shape
   x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;
   
   // off the right side?
   if(x1 >= viewwidth)
      return;
   
   x2 = ((centerxfrac + 
          FixedMul(tx+particle->size*(FRACUNIT/4), xscale)) >> FRACBITS);
   
   // off the left side?
   if(x2 < 0)
      return;
   
   gzt = particle->z + 1;
   
   // killough 3/27/98: exclude things totally separated
   // from the viewer, by either water or fake ceilings
   // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
   
   {
      // haleyjd 02/20/04: use subsector now stored in particle
      subsector_t *subsector = particle->subsector;
      sector = subsector->sector;
      heightsec = sector->heightsec;

      if(particle->z < sector->floorheight || 
	 particle->z > sector->ceilingheight)
	 return;
   }
   
   // only clip particles which are in special sectors
   if(heightsec != -1)
   {
      int phs = viewcamera ? viewcamera->heightsec :
                viewplayer->mo->subsector->sector->heightsec;
      
      if(phs != -1 && 
	 viewz < sectors[phs].floorheight ?
	         particle->z >= sectors[heightsec].floorheight :
                 gzt < sectors[heightsec].floorheight)
	 return;

      if(phs != -1 && 
	 viewz > sectors[phs].ceilingheight ?
	         gzt < sectors[heightsec].ceilingheight &&
	           viewz >= sectors[heightsec].ceilingheight :
                 particle->z >= sectors[heightsec].ceilingheight)
	 return;
   }
   
   // store information in a vissprite
   vis = R_NewVisSprite();
   vis->heightsec = heightsec;
   vis->scale = yscale; // SoM: ANYRES aspect ratio
   vis->gx = particle->x;
   vis->gy = particle->y;
   vis->gz = particle->z;
   vis->gzt = gzt;
   vis->texturemid = vis->gzt - viewz;
   vis->x1 = x1 < 0 ? 0 : x1;
   vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
   //vis->translation = NULL;
   iscale = FixedDiv(FRACUNIT, xscale);
   vis->startfrac = particle->color;
   vis->xiscale = iscale;
   vis->patch = -1;
   vis->mobjflags = particle->trans;
   vis->mobjflags3 = 0; // haleyjd
   
   if(fixedcolormap ==
      fullcolormap + INVERSECOLORMAP*256*sizeof(lighttable_t))
   {
      vis->colormap = fixedcolormap;
   } 
   else
   {
      // haleyjd 01/12/02: wow is this code wrong! :)
      //int index = xscale>>(LIGHTSCALESHIFT + hires);
      //if(index >= MAXLIGHTSCALE) 
      //   index = MAXLIGHTSCALE-1;      
      //vis->colormap = spritelights[index];

      R_SectorColormap(sector);

      if(LevelInfo.useFullBright && (particle->styleflags & PS_FULLBRIGHT))
      {
         vis->colormap = fullcolormap;
      }
      else
      {
         lighttable_t **ltable;
         sector_t tmpsec;
         int floorlightlevel, ceilinglightlevel, lightnum, index;

         R_FakeFlat(sector, &tmpsec, &floorlightlevel, 
                    &ceilinglightlevel, false);

         lightnum = (floorlightlevel + ceilinglightlevel) / 2;
         lightnum = (lightnum >> LIGHTSEGSHIFT) + extralight;
         
         if(lightnum >= LIGHTLEVELS || fixedcolormap)
            ltable = scalelight[LIGHTLEVELS - 1];      
         else if(lightnum < 0)
            ltable = scalelight[0];
         else
            ltable = scalelight[lightnum];
         
         // SoM: ANYRES
         index = xscale >> (LIGHTSCALESHIFT + addscaleshift);
         if(index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE - 1;
         
         vis->colormap = ltable[index];
      }
   }
}

//
// R_DrawParticle
//
// haleyjd: this function had to be mostly rewritten
//
void R_DrawParticle(vissprite_t *vis)
{
   int x1, x2;
   int yl, yh;
   byte color;

   x1 = vis->x1;
   x2 = vis->x2;
   if(x1 < 0)
      x1 = 0;
   if(x2 < x1)
      x2 = x1;
   if(x2 >= viewwidth)
      x2 = viewwidth - 1;

   yl = (centeryfrac - FixedMul(vis->texturemid, vis->scale) + 
         FRACUNIT - 1) >> FRACBITS;
   yh = yl + (x2 - x1);

   // due to square shape, it is unnecessary to clip the entire
   // particle
   if(yh >= mfloorclip[x1])
      yh = mfloorclip[x1]-1;
   if(yl <= mceilingclip[x1])
      yl = mceilingclip[x1]+1;
   if(yh >= mfloorclip[x2])
      yh = mfloorclip[x2]-1;
   if(yl <= mceilingclip[x2])
      yl = mceilingclip[x2]+1;

   color = vis->colormap[vis->startfrac];

   {
      int xcount, ycount, spacing;
      byte *dest;

      xcount = x2 - x1 + 1;
      
      ycount = yh - yl;      
      if(ycount < 0)
	 return;
      ++ycount;

      spacing = v_width - xcount;
      dest = ylookup[yl] + columnofs[x1];

      // haleyjd 02/08/05: rewritten to remove inner loop invariants
      if(general_translucency && particle_trans)
      {
         if(particle_trans == 1) // smooth (DosDOOM-style)
         {
            unsigned int bg, fg;
            unsigned int *fg2rgb, *bg2rgb;
            fixed_t fglevel, bglevel;

            // look up translucency information
            fglevel = ((vis->mobjflags + 1) << 8) & ~0x3ff;
            bglevel = FRACUNIT - fglevel;
            fg2rgb  = Col2RGB[fglevel >> 10];
            bg2rgb  = Col2RGB[bglevel >> 10];
            fg      = fg2rgb[color]; // foreground color is invariant

            do // step in y
            {
               int count = xcount;
               
               do // step in x
               {
                  bg = bg2rgb[*dest];
                  bg = (fg + bg) | 0xf07c3e1f;
                  *dest++ = RGB8k[0][0][(bg >> 5) & (bg >> 19)];
               } while(--count);
               dest += spacing;  // go to next row
            } while(--ycount);
         }
         else // general (BOOM)
         {
            do // step in y
            {
               int count = xcount;
               
               do // step in x
               {
                  *dest++ = main_tranmap[(*dest << 8) + color];
               } while(--count);
               dest += spacing;  // go to next row
            } while(--ycount);
         } // end else [particle_trans == 2]
      }
      else // opaque (fast, and looks terrible)
      {
         do // step in y
         {
            int count = xcount;
            
            do // step in x
            {
               *dest++ = color;
            } while(--count);
            dest += spacing;  // go to next row
         } while(--ycount);
      } // end else [!general_translucency]
   } // end local block
}

//----------------------------------------------------------------------------
//
// $Log: r_things.c,v $
// Revision 1.22  1998/05/03  22:46:41  killough
// beautification
//
// Revision 1.21  1998/05/01  15:26:50  killough
// beautification
//
// Revision 1.20  1998/04/27  02:04:43  killough
// Fix incorrect I_Error format string
//
// Revision 1.19  1998/04/24  11:03:26  jim
// Fixed bug in sprites in PWAD
//
// Revision 1.18  1998/04/13  09:45:30  killough
// Fix sprite clipping under fake ceilings
//
// Revision 1.17  1998/04/12  02:02:19  killough
// Fix underwater sprite clipping, add wall translucency
//
// Revision 1.16  1998/04/09  13:18:48  killough
// minor optimization, plus fix ghost sprites due to huge z-height diffs
//
// Revision 1.15  1998/03/31  19:15:27  killough
// Fix underwater sprite clipping bug
//
// Revision 1.14  1998/03/28  18:15:29  killough
// Add true deep water / fake ceiling sprite clipping
//
// Revision 1.13  1998/03/23  03:41:43  killough
// Use 'fullcolormap' for fully-bright colormap
//
// Revision 1.12  1998/03/16  12:42:37  killough
// Optimize away some function pointers
//
// Revision 1.11  1998/03/09  07:28:16  killough
// Add primitive underwater support
//
// Revision 1.10  1998/03/02  11:48:59  killough
// Add failsafe against texture mapping overflow crashes
//
// Revision 1.9  1998/02/23  04:55:52  killough
// Remove some comments
//
// Revision 1.8  1998/02/20  22:53:22  phares
// Moved TRANMAP initialization to w_wad.c
//
// Revision 1.7  1998/02/20  21:56:37  phares
// Preliminarey sprite translucency
//
// Revision 1.6  1998/02/09  03:23:01  killough
// Change array decl to use MAX screen width/height
//
// Revision 1.5  1998/02/02  13:32:49  killough
// Performance tuning, program beautification
//
// Revision 1.4  1998/01/26  19:24:50  phares
// First rev with no ^Ms
//
// Revision 1.3  1998/01/26  06:13:58  killough
// Performance tuning
//
// Revision 1.2  1998/01/23  20:28:14  jim
// Basic sprite/flat functionality in PWAD added
//
// Revision 1.1.1.1  1998/01/19  14:03:06  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
