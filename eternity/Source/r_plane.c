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
//      Here is a core component: drawing the floors and ceilings,
//       while maintaining a per column clipping list only.
//      Moreover, the sky areas have to be determined.
//
// MAXVISPLANES is no longer a limit on the number of visplanes,
// but a limit on the number of hash slots; larger numbers mean
// better performance usually but after a point they are wasted,
// and memory and time overheads creep in.
//
// For more information on visplanes, see:
//
// http://classicgaming.com/doom/editing/
//
// Lee Killough
//
//-----------------------------------------------------------------------------

#include "z_zone.h"  /* memory allocation wrappers -- killough */

static const char
rcsid[] = "$Id: r_plane.c,v 1.8 1998/05/03 23:09:53 killough Exp $";

#include "doomstat.h"

#include "c_io.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_things.h"
#include "r_sky.h"
#include "r_ripple.h"
#include "r_plane.h"
#include "v_video.h"

#include "p_info.h"
#include "p_anim.h"
#include "p_user.h"

#define MAXVISPLANES 128    /* must be a power of 2 */

static visplane_t *visplanes[MAXVISPLANES];   // killough
static visplane_t *freetail;                  // killough
static visplane_t **freehead = &freetail;     // killough
visplane_t *floorplane, *ceilingplane;

int num_visplanes;      // sf: count visplanes

// killough -- hash function for visplanes
// Empirically verified to be fairly uniform:

#define visplane_hash(picnum,lightlevel,height) \
  (((unsigned)(picnum)*3+(unsigned)(lightlevel)+(unsigned)(height)*7) & (MAXVISPLANES-1))

// killough 8/1/98: set static number of openings to be large enough
// (a static limit is okay in this case and avoids difficulties in r_segs.c)
#define MAXOPENINGS (MAX_SCREENWIDTH*MAX_SCREENHEIGHT)
short openings[MAXOPENINGS],*lastopening;

// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1

#ifdef R_PORTALS
// SoM 12/8/03: floorclip and ceilingclip changed to pointers so they can be set
// to the clipping arrays of portals.
short floorcliparray[MAX_SCREENWIDTH], ceilingcliparray[MAX_SCREENWIDTH];
short *floorclip = floorcliparray, *ceilingclip = ceilingcliparray;
#else
short floorclip[MAX_SCREENWIDTH], ceilingclip[MAX_SCREENWIDTH];
#endif

// spanstart holds the start of a plane span; initialized to 0 at start

static int spanstart[MAX_SCREENHEIGHT];                // killough 2/8/98

//
// texture mapping
//

static lighttable_t **planezlight;
static fixed_t planeheight;

// killough 2/8/98: make variables static

static fixed_t basexscale, baseyscale;
static fixed_t cachedheight[MAX_SCREENHEIGHT];
static fixed_t cacheddistance[MAX_SCREENHEIGHT];
static fixed_t cachedxstep[MAX_SCREENHEIGHT];
static fixed_t cachedystep[MAX_SCREENHEIGHT];
static fixed_t xoffs,yoffs;    // killough 2/28/98: flat offsets
#ifdef R_PORTALS
static fixed_t pviewx, pviewy, pviewz;
#endif

fixed_t *yslope;
fixed_t origyslope[MAX_SCREENHEIGHT*2];
fixed_t distscale[MAX_SCREENWIDTH];
int     visplane_view=0;

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes(void)
{
}

//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//  xoffs
//  yoffs
//
// BASIC PRIMITIVE
//
static void R_MapPlane(int y, int x1, int x2)
{
   angle_t angle;
   fixed_t distance, length;
   unsigned index;
   
#ifdef RANGECHECK
   if(x2 < x1 || x1 < 0 || x2 >= viewwidth || y < 0 || y >= viewheight)
      I_Error ("R_MapPlane: %i, %i at %i", x1, x2, y);
#endif

   if (planeheight != cachedheight[y])
   {
      cachedheight[y] = planeheight;
      distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
      ds_xstep = cachedxstep[y] = FixedMul (distance,basexscale);
      ds_ystep = cachedystep[y] = FixedMul (distance,baseyscale);
   }
   else
   {
      distance = cacheddistance[y];
      ds_xstep = cachedxstep[y];
      ds_ystep = cachedystep[y];
   }
   
   length = FixedMul(distance,distscale[x1]);
   angle = (viewangle + xtoviewangle[x1]) >> ANGLETOFINESHIFT;

   // killough 2/28/98: Add offsets
#ifdef R_PORTALS
   ds_xfrac =  pviewx + FixedMul(finecosine[angle], length) + xoffs;
   ds_yfrac = -pviewy - FixedMul(finesine[angle],   length) + yoffs;
#else
   ds_xfrac =  viewx + FixedMul(finecosine[angle], length) + xoffs;
   ds_yfrac = -viewy - FixedMul(finesine[angle],   length) + yoffs;
#endif

   if(!(ds_colormap = fixedcolormap))
   {
      index = distance >> LIGHTZSHIFT;
      if(index >= MAXLIGHTZ )
         index = MAXLIGHTZ-1;
      ds_colormap = planezlight[index];
   }
   else
   {
      // haleyjd 10/31/02: invuln fix
      if(fixedcolormap != 
         fullcolormap + INVERSECOLORMAP*256*sizeof(lighttable_t))
      {
         // SoM 10/19/02: deep water colormap fix
         ds_colormap = planezlight[MAXLIGHTZ-1];
      }
   }

   ds_y = y;
   ds_x1 = x1;
   ds_x2 = x2;
   
   R_DrawSpan();

  // visplane viewing -- sf
  if(visplane_view)
  {
     if(ds_y >=0 && ds_y < viewheight)
     {
        // haleyjd FIXME: should use gamemode black, not 0
        // SoM: ANYRES
        if(ds_x1 >= 0 && ds_x1<=viewwidth)
           *(screens[0]+y*v_width+x1) = 0;
        if(ds_x2 >= 0 && ds_x2<=viewwidth)
           *(screens[0]+y*v_width+x2) = 0;
     }
  }
}

//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes(void)
{
   int i, a;
   angle_t angle;
   
   // haleyjd FIXME: wtf is this?
   a = consoleactive ?
      (current_height-viewwindowy) < 0 ? -1 : current_height-viewwindowy
           : -1;

   a = -1;

#ifdef R_PORTALS  
   floorclip = floorcliparray;
   ceilingclip = ceilingcliparray;
#endif

   // opening / clipping determination
   for(i = 0; i < viewwidth; ++i)
      floorclip[i] = viewheight, ceilingclip[i] = a;

   for(i = 0; i < MAXVISPLANES; ++i)    // new code -- killough
      for(*freehead = visplanes[i], visplanes[i] = NULL; *freehead; )
         freehead = &(*freehead)->next;

   lastopening = openings;

   // texture calculation
   memset (cachedheight, 0, sizeof(cachedheight));

   // left to right mapping
   angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;
   // scale will be unit scale at SCREENWIDTH/2 distance
   basexscale = FixedDiv (finecosine[angle],centerxfrac*zoom);
   baseyscale = -FixedDiv (finesine[angle],centerxfrac*zoom);
   
   num_visplanes = 0;    // reset
}

// New function, by Lee Killough

static visplane_t *new_visplane(unsigned hash)
{
   visplane_t *check = freetail;

   if(!check)
   {
      check = calloc(1, sizeof *check);
      check->max_width = 0;
      check->pad1 = check->pad2 = check->pad3 = check->pad4 = check->top = check->bottom = NULL;
   }
   else 
      if(!(freetail = freetail->next))
         freehead = &freetail;
   
   check->next = visplanes[hash];
   visplanes[hash] = check;

   if(check->max_width < (unsigned)v_width)
   {
      if(check->pad1)
         free(check->pad1);
      if(check->pad3)
         free(check->pad3);

      check->max_width = v_width;
      check->pad1 = (unsigned short *)calloc(1, (v_width + 2) * sizeof(unsigned short));
      check->top = check->pad1 + 1;
      check->pad2 = check->pad1 + v_width + 1;

      check->pad3 = (unsigned short *)calloc(1, (v_width + 2) * sizeof(unsigned short));
      check->bottom = check->pad3 + 1;
      check->pad4 = check->pad3 + v_width + 1;
   }
   
   num_visplanes++;      // keep track of how many for counter
   
   return check;
}

//
// R_FindPlane
//
// killough 2/28/98: Add offsets

visplane_t *R_FindPlane(fixed_t height, int picnum, int lightlevel,
                fixed_t xoffs, fixed_t yoffs)
{
   visplane_t *check;
   unsigned hash;                      // killough
   
   if(picnum == skyflatnum || picnum == sky2flatnum || picnum & PL_SKYFLAT)  // killough 10/98
      lightlevel = height = 0;   // killough 7/19/98: most skies map together

   // New visplane algorithm uses hash table -- killough
   hash = visplane_hash(picnum,lightlevel,height);

   for(check=visplanes[hash]; check; check=check->next)  // killough
   {
      if(height == check->height &&
         picnum == check->picnum &&
         lightlevel == check->lightlevel &&
         xoffs == check->xoffs &&      // killough 2/28/98: Add offset checks
         yoffs == check->yoffs &&
         zlight == check->colormap
#ifdef R_PORTALS
         && viewx == check->viewx
         && viewy == check->viewy
         && viewz == check->viewz
#endif
        )
        return check;
   }

   check = new_visplane(hash);         // killough

   check->height = height;
   check->picnum = picnum;
   check->lightlevel = lightlevel;
   check->minx = viewwidth;            // Was SCREENWIDTH -- killough 11/98
   check->maxx = -1;
   check->xoffs = xoffs;               // killough 2/28/98: Save offsets
   check->yoffs = yoffs;
   check->colormap = zlight;
#ifdef R_PORTALS
   check->viewx = viewx;
   check->viewy = viewy;
   check->viewz = viewz;
#endif 
   
   // SoM: memset should use the check->max_width
   memset (check->top, 0xff, sizeof(unsigned short) * check->max_width);
   
   return check;
}

//
// R_CheckPlane
//
visplane_t *R_CheckPlane(visplane_t *pl, int start, int stop)
{
   int intrl, intrh, unionl, unionh, x;
   
   if(start < pl->minx)
      intrl   = pl->minx, unionl = start;
   else
      unionl  = pl->minx,  intrl = start;
   
   if(stop  > pl->maxx)
      intrh   = pl->maxx, unionh = stop;
   else
      unionh  = pl->maxx, intrh  = stop;

   for(x = intrl; x <= intrh && pl->top[x] == 0xffff; ++x)
      ;

   if(x > intrh)
      pl->minx = unionl, pl->maxx = unionh;
   else
   {
      unsigned hash = visplane_hash(pl->picnum, pl->lightlevel, pl->height);
      visplane_t *new_pl = new_visplane(hash);
      
      new_pl->height = pl->height;
      new_pl->picnum = pl->picnum;
      new_pl->lightlevel = pl->lightlevel;
      new_pl->colormap = pl->colormap;
      new_pl->xoffs = pl->xoffs;           // killough 2/28/98
      new_pl->yoffs = pl->yoffs;
#ifdef R_PORTALS
      new_pl->viewx = pl->viewx;
      new_pl->viewy = pl->viewy;
      new_pl->viewz = pl->viewz;
#endif 
      pl = new_pl;
      pl->minx = start;
      pl->maxx = stop;
      // SoM: ANYRES
      memset(pl->top, 0xff, sizeof(unsigned short) * pl->max_width);
   }
   
   return pl;
}

//
// R_MakeSpans
//

static void R_MakeSpans(int x, int t1, int b1, int t2, int b2)
{
   for(; t1 < t2 && t1 <= b1; ++t1)
      R_MapPlane(t1, spanstart[t1], x-1);
   for(; b1 > b2 && b1 >= t1; --b1)
      R_MapPlane(b1, spanstart[b1], x-1);
   while(t2 < t1 && t2 <= b2)
      spanstart[t2++] = x;
   while(b2 > b1 && b2 >= t2)
      spanstart[b2--] = x;
}

extern void R_DrawNewSkyColumn(void);

// haleyjd: moved here from r_newsky.c
void do_draw_newsky(visplane_t *pl)
{
   int x, offset, skyTexture, offset2, skyTexture2;
   angle_t an;
   skytexture_t *sky1, *sky2;
   
   an = viewangle;
   
   if(LevelInfo.doubleSky) // render two layers
   {
      // get scrolling offsets and textures
      offset = Sky1ColumnOffset>>16;
      skyTexture = texturetranslation[skytexture];
      offset2 = Sky2ColumnOffset>>16;
      skyTexture2 = texturetranslation[sky2texture];

      sky1 = R_GetSkyTexture(skyTexture);
      sky2 = R_GetSkyTexture(skyTexture2);
      
      if(comp[comp_skymap] || !(dc_colormap = fixedcolormap))
         dc_colormap = fullcolormap;
      
      // first draw sky 2 with R_DrawColumn (unmasked)
      dc_texturemid = sky2->texturemid;      
      dc_texheight = sky2->height;

      // haleyjd: don't stretch textures over 200 tall
      if(dc_texheight < 200)
         dc_iscale = pspriteiyscale >> stretchsky;
      else
         dc_iscale = pspriteiyscale;
      
      for(x = pl->minx; (dc_x = x) <= pl->maxx; x++)
      {
         if ((dc_yl = pl->top[x]) <= (dc_yh = pl->bottom[x]))
         {
            dc_source =
               R_GetColumn(skyTexture2,
               (((an + xtoviewangle[x])) >> (ANGLETOSKYSHIFT))+offset2);
            colfunc();
         }
      }
      
      // now draw sky 1 with R_DrawNewSkyColumn (masked)
      dc_texturemid = sky1->texturemid;
      dc_texheight = sky1->height;

      // haleyjd: don't stretch textures over 200 tall
      if(dc_texheight < 200)
         dc_iscale = pspriteiyscale >> stretchsky;
      else
         dc_iscale = pspriteiyscale;
      
      for(x = pl->minx; (dc_x = x) <= pl->maxx; x++)
      {
         if ((dc_yl = pl->top[x]) <= (dc_yh = pl->bottom[x]))
         {
            dc_source =
               R_GetColumn(skyTexture,
               (((an + xtoviewangle[x])) >> (ANGLETOSKYSHIFT))+offset);
            R_DrawNewSkyColumn();
         }
      }
   }
   else // one layer only -- use pl->picnum
   {
      if(pl->picnum == skyflatnum)
      {
         offset = Sky1ColumnOffset>>16;
         skyTexture = texturetranslation[skytexture];
      }
      else
      {
         offset = Sky2ColumnOffset>>16;
         skyTexture = texturetranslation[sky2texture];
      }

      sky1 = R_GetSkyTexture(skyTexture);
      
      dc_texturemid = sky1->texturemid;    // Default y-offset
      
      if(comp[comp_skymap] || !(dc_colormap = fixedcolormap))
         dc_colormap = fullcolormap;
      
      dc_texheight = sky1->height;

      // haleyjd: don't stretch textures over 200 tall
      if(dc_texheight < 200)
         dc_iscale = pspriteiyscale >> stretchsky;
      else
         dc_iscale = pspriteiyscale;
      
      for(x = pl->minx; (dc_x = x) <= pl->maxx; x++)
      {
         if ((dc_yl = pl->top[x]) <= (dc_yh = pl->bottom[x]))
         {
            dc_source =
               R_GetColumn(skyTexture,
               (((an + xtoviewangle[x])) >> (ANGLETOSKYSHIFT))+offset);
            colfunc();
         }
      }
   }
}

// New function, by Lee Killough
// haleyjd 08/30/02: slight restructuring to use hashed
// sky texture info cache

static void do_draw_plane(visplane_t *pl)
{
   register int x;

   if(!(pl->minx <= pl->maxx))
      return;

   // haleyjd: hexen-style skies:
   // * Always for sky2
   // * Use for sky1 IF double skies or sky delta set
   if(pl->picnum == sky2flatnum ||
      (pl->picnum == skyflatnum && 
       (LevelInfo.doubleSky || LevelInfo.skyDelta)))
   {
      do_draw_newsky(pl);
      return;
   }

   if(pl->picnum == skyflatnum || pl->picnum & PL_SKYFLAT)  // sky flat
   {
      int texture;
      angle_t an, flip;
      skytexture_t *sky;
      
      // killough 10/98: allow skies to come from sidedefs.
      // Allows scrolling and/or animated skies, as well as
      // arbitrary multiple skies per level without having
      // to use info lumps.

      an = viewangle;
      
      if(pl->picnum & PL_SKYFLAT)
      { 
         // Sky Linedef
         const line_t *l = &lines[pl->picnum & ~PL_SKYFLAT];
         
         // Sky transferred from first sidedef
         const side_t *s = *l->sidenum + sides;
         
         // Texture comes from upper texture of reference sidedef
         texture = texturetranslation[s->toptexture];

         // haleyjd 08/30/02: set skytexture info pointer
         sky = R_GetSkyTexture(texture);

         // Horizontal offset is turned into an angle offset,
         // to allow sky rotation as well as careful positioning.
         // However, the offset is scaled very small, so that it
         // allows a long-period of sky rotation.
         
         an += s->textureoffset;
         
         // Vertical offset allows careful sky positioning.        
         
         dc_texturemid = s->rowoffset - 28*FRACUNIT;
         
         // We sometimes flip the picture horizontally.
         //
         // Doom always flipped the picture, so we make it optional,
         // to make it easier to use the new feature, while to still
         // allow old sky textures to be used.
         
         flip = l->special==272 ? 0u : ~0u;
      }
      else 	 // Normal Doom sky, only one allowed per level
      {
         texture = skytexture;             // Default texture
         sky = R_GetSkyTexture(texture);   // haleyjd 08/30/02         
         dc_texturemid = sky->texturemid;  // Default y-offset
         flip = 0;                         // Doom flips it
      }

      // Sky is always drawn full bright, i.e. colormaps[0] is used.
      // Because of this hack, sky is not affected by INVUL inverse mapping.
      //
      // killough 7/19/98: fix hack to be more realistic:

      if(comp[comp_skymap] || !(dc_colormap = fixedcolormap))
         dc_colormap = fullcolormap;          // killough 3/20/98

      //dc_texheight = (textureheight[texture])>>FRACBITS; // killough
      // haleyjd: use height determined from patches in texture
      dc_texheight = sky->height;
      
      // haleyjd: don't stretch textures over 200 tall
      if(dc_texheight < 200)
         dc_iscale = pspriteiyscale >> stretchsky;
      else
         dc_iscale = pspriteiyscale;

      // killough 10/98: Use sky scrolling offset, and possibly flip picture
      for (x = pl->minx; (dc_x = x) <= pl->maxx; x++)
      {
         if ((dc_yl = pl->top[x]) <= (dc_yh = pl->bottom[x]))
         {
            dc_source = R_GetColumn(texture,
               ((an + xtoviewangle[x])^flip) >> (ANGLETOSKYSHIFT));
            colfunc();
         }
      }
   }
   else      // regular flat
   {
      int stop, light;
      int swirling = 0;

      swirling = flattranslation[pl->picnum] == -1;
      
      if(swirling)
      {
         ds_source = R_DistortedFlat(pl->picnum);
      }
      else
      {
         ds_source = 
            W_CacheLumpNum(firstflat + flattranslation[pl->picnum],
                           PU_STATIC);
      }
        
      // haleyjd: ?? - inconsequential
      // trans = swirling;

      xoffs = pl->xoffs;  // killough 2/28/98: Add offsets
      yoffs = pl->yoffs;
#ifdef R_PORTALS
      pviewx = pl->viewx;
      pviewy = pl->viewy;
      pviewz = pl->viewz;
      planeheight = D_abs(pl->height-pviewz);
#else
      planeheight = D_abs(pl->height-viewz);
#endif
      
      //light = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;

      // SoM 10/19/02: deep water colormap fix
      if(fixedcolormap)
         light = (255  >> LIGHTSEGSHIFT);
      else
         light = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;

      if(light >= LIGHTLEVELS)
         light = LIGHTLEVELS-1;

      if(light < 0)
         light = 0;

      stop = pl->maxx + 1;
      pl->top[pl->minx-1] = pl->top[stop] = 0xffff;
      planezlight = pl->colormap[light];//zlight[light];

      for(x = pl->minx ; x <= stop ; x++)
         R_MakeSpans(x,pl->top[x-1],pl->bottom[x-1],pl->top[x],pl->bottom[x]);

      if(!swirling)
         Z_ChangeTag(ds_source, PU_CACHE);
   }
}

//
// R_DrawPlanes
// At the end of each frame.
//

void R_DrawPlanes(void)
{
   visplane_t *pl;
   int i;
   
   for(i = 0; i < MAXVISPLANES; ++i)
      for(pl = visplanes[i]; pl; pl = pl->next)
      {
         do_draw_plane(pl);
      }
}

//----------------------------------------------------------------------------
//
// $Log: r_plane.c,v $
// Revision 1.8  1998/05/03  23:09:53  killough
// Fix #includes at the top
//
// Revision 1.7  1998/04/27  01:48:31  killough
// Program beautification
//
// Revision 1.6  1998/03/23  03:38:26  killough
// Use 'fullcolormap' for fully-bright F_SKY1
//
// Revision 1.5  1998/03/02  11:47:13  killough
// Add support for general flats xy offsets
//
// Revision 1.4  1998/02/09  03:16:03  killough
// Change arrays to use MAX height/width
//
// Revision 1.3  1998/02/02  13:28:40  killough
// performance tuning
//
// Revision 1.2  1998/01/26  19:24:45  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
