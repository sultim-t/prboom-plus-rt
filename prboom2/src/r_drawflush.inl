/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *-----------------------------------------------------------------------------*/


//
// R_FlushWholeOpaque
//
// Flushes the entire columns in the buffer, one at a time.
// This is used when a quad flush isn't possible.
// Opaque version -- no remapping whatsoever.
//
static void R_FLUSHWHOLE_FUNCNAME(void)
{
   register byte *source;
   register byte *dest;
   register int  count, yl;

   while(--temp_x >= 0)
   {
      yl     = tempyl[temp_x];
      source = &tempbuf[temp_x + (yl << 2)];
      dest   = drawvars.topleft + yl*drawvars.pitch + startx + temp_x;
      count  = tempyh[temp_x] - yl + 1;
      
      while(--count >= 0)
      {
#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
         *dest = temptranmap[(*dest<<8) + *source];
#elif (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
         // SoM 7-28-04: Fix the fuzz problem.
         *dest = tempfuzzmap[6*256+dest[fuzzoffset[fuzzpos]]];
         
         // Clamp table lookup index.
         if(++fuzzpos == FUZZTABLE) 
            fuzzpos = 0;
#else
         *dest = *source;
#endif

         source += 4;
         dest += drawvars.pitch;
      }
   }
}

//
// R_FlushHTOpaque
//
// Flushes the head and tail of columns in the buffer in
// preparation for a quad flush.
// Opaque version -- no remapping whatsoever.
//
static void R_FLUSHHEADTAIL_FUNCNAME(void)
{
   register byte *source;
   register byte *dest;
   register int count, colnum = 0;
   int yl, yh;

   while(colnum < 4)
   {
      yl = tempyl[colnum];
      yh = tempyh[colnum];
      
      // flush column head
      if(yl < commontop)
      {
         source = &tempbuf[colnum + (yl << 2)];
         dest   = drawvars.topleft + yl*drawvars.pitch + startx + colnum;
         count  = commontop - yl;
         
         while(--count >= 0)
         {
#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
            // haleyjd 09/11/04: use temptranmap here
            *dest = temptranmap[(*dest<<8) + *source];
#elif (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
            // SoM 7-28-04: Fix the fuzz problem.
            *dest = tempfuzzmap[6*256+dest[fuzzoffset[fuzzpos]]];
            
            // Clamp table lookup index.
            if(++fuzzpos == FUZZTABLE) 
               fuzzpos = 0;
#else
            *dest = *source;
#endif

            source += 4;
            dest += drawvars.pitch;
         }
      }
      
      // flush column tail
      if(yh > commonbot)
      {
         source = &tempbuf[colnum + ((commonbot + 1) << 2)];
         dest   = drawvars.topleft + (commonbot + 1)*drawvars.pitch + startx + colnum;
         count  = yh - commonbot;
         
         while(--count >= 0)
         {
#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
            // haleyjd 09/11/04: use temptranmap here
            *dest = temptranmap[(*dest<<8) + *source];
#elif (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
            // SoM 7-28-04: Fix the fuzz problem.
            *dest = tempfuzzmap[6*256+dest[fuzzoffset[fuzzpos]]];
            
            // Clamp table lookup index.
            if(++fuzzpos == FUZZTABLE) 
               fuzzpos = 0;
#else
            *dest = *source;
#endif

            source += 4;
            dest += drawvars.pitch;
         }
      }         
      ++colnum;
   }
}

static void R_FLUSHQUAD_FUNCNAME(void)
{
   byte *source = &tempbuf[commontop << 2];
   byte *dest = drawvars.topleft + commontop*drawvars.pitch + startx;
   int count;
#if (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
   int fuzz1, fuzz2, fuzz3, fuzz4;

   fuzz1 = fuzzpos;
   fuzz2 = (fuzz1 + tempyl[1]) % FUZZTABLE;
   fuzz3 = (fuzz2 + tempyl[2]) % FUZZTABLE;
   fuzz4 = (fuzz3 + tempyl[3]) % FUZZTABLE;
#endif

   count = commonbot - commontop + 1;

#if (R_DRAWCOLUMN_PIPELINE & RDC_TRANSLUCENT)
   while(--count >= 0)
   {
      dest[0] = temptranmap[(dest[0]<<8) + source[0]];
      dest[1] = temptranmap[(dest[1]<<8) + source[1]];
      dest[2] = temptranmap[(dest[2]<<8) + source[2]];
      dest[3] = temptranmap[(dest[3]<<8) + source[3]];
      source += 4 * sizeof(byte);
      dest += drawvars.pitch * sizeof(byte);
   }
#elif (R_DRAWCOLUMN_PIPELINE & RDC_FUZZ)
   while(--count >= 0)
   {
      dest[0] = tempfuzzmap[6*256+dest[0 + fuzzoffset[fuzz1]]];
      dest[1] = tempfuzzmap[6*256+dest[1 + fuzzoffset[fuzz2]]];
      dest[2] = tempfuzzmap[6*256+dest[2 + fuzzoffset[fuzz3]]];
      dest[3] = tempfuzzmap[6*256+dest[3 + fuzzoffset[fuzz4]]];
      fuzz1 = (fuzz1 + 1) % FUZZTABLE;
      fuzz2 = (fuzz2 + 1) % FUZZTABLE;
      fuzz3 = (fuzz3 + 1) % FUZZTABLE;
      fuzz4 = (fuzz4 + 1) % FUZZTABLE;
      source += 4 * sizeof(byte);
      dest += drawvars.pitch * sizeof(byte);
   }
#else
   if ((sizeof(int) == 4) && (((int)source % 4) == 0) && (((int)dest % 4) == 0)) {
      while(--count >= 0)
      {
         *(int *)dest = *(int *)source;
         source += 4 * sizeof(byte);
         dest += drawvars.pitch * sizeof(byte);
      }
   } else {
      while(--count >= 0)
      {
         dest[0] = source[0];
         dest[1] = source[1];
         dest[2] = source[2];
         dest[3] = source[3];
         source += 4 * sizeof(byte);
         dest += drawvars.pitch * sizeof(byte);
      }
   }
#endif
}

#undef R_DRAWCOLUMN_PIPELINE
#undef R_FLUSHWHOLE_FUNCNAME
#undef R_FLUSHHEADTAIL_FUNCNAME
#undef R_FLUSHQUAD_FUNCNAME
