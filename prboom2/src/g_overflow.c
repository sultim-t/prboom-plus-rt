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
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>

#include "g_overflow.h"

#include "doomstat.h"
#include "lprintf.h"
#include "m_argv.h"
#include "m_misc.h"
#include "e6y.h"

int overflows_enabled = true;

overrun_param_t overflows[OVERFLOW_MAX];
const char *overflow_cfgname[OVERFLOW_MAX] =
{
  "overrun_spechit_emulate",
  "overrun_reject_emulate",
  "overrun_intercept_emulate",
  "overrun_playeringame_emulate",
  "overrun_donut_emulate",
  "overrun_missedbackside_emulate"
};

static void ShowOverflowWarning(overrun_list_t overflow, int fatal, const char *params, ...)
{
  overflows[overflow].shit_happens = true;

  if (overflows[overflow].warn && !overflows[overflow].promted)
  {
    va_list argptr;
    char buffer[1024];

    static const char *name[OVERFLOW_MAX] = {
      "SPECHIT", "REJECT", "INTERCEPT", "PLYERINGAME", "DONUT", "MISSEDBACKSIDE"};

    static const char str1[] =
      "Too big or not supported %s overflow has been detected. "
      "Desync or crash can occur soon "
      "or during playback with the vanilla engine in case you're recording demo.%s%s";
    
    static const char str2[] = 
      "%s overflow has been detected.%s%s";

    static const char str3[] = 
      "%s overflow has been detected. "
      "The option responsible for emulation of this overflow is switched off "
      "hence desync or crash can occur soon "
      "or during playback with the vanilla engine in case you're recording demo.%s%s";

    overflows[overflow].promted = true;

    sprintf(buffer,
      (fatal ? str1 : (EMULATE(overflow) ? str2 : str3)), 
      name[overflow],
      "\nYou can change PrBoom behaviour for this overflow through in-game menu.",
      params);
    
    va_start(argptr, params);
    I_vWarning(buffer, argptr);
    va_end(argptr);
  }
}

// e6y
//
// Intercepts Overrun emulation
// See more information on:
// doomworld.com/vb/doom-speed-demos/35214-spechits-reject-and-intercepts-overflow-lists
//
// Thanks to Simon Howard (fraggle) for refactor the intercepts
// overrun code so that it should work properly on big endian machines
// as well as little endian machines.

// Overwrite a specific memory location with a value.
static void InterceptsMemoryOverrun(int location, int value)
{
  int i, offset;
  int index;
  void *addr;

  i = 0;
  offset = 0;

  // Search down the array until we find the right entry

  while (intercepts_overrun[i].len != 0)
  {
    if (offset + intercepts_overrun[i].len > location)
    {
      addr = intercepts_overrun[i].addr;

      // Write the value to the memory location.
      // 16-bit and 32-bit values are written differently.

      if (addr != NULL)
      {
        if (intercepts_overrun[i].int16_array)
        {
          index = (location - offset) / 2;
          ((short *) addr)[index] = value & 0xffff;
          ((short *) addr)[index + 1] = (value >> 16) & 0xffff;
        }
        else
        {
          index = (location - offset) / 4;
          ((int *) addr)[index] = value;
        }
      }

      break;
    }

    offset += intercepts_overrun[i].len;
    ++i;
  }
}

void InterceptsOverrun(int num_intercepts, intercept_t *intercept)
{
  if (num_intercepts > MAXINTERCEPTS_ORIGINAL && demo_compatibility && PROCESS(OVERFLOW_INTERCEPT))
  {
    ShowOverflowWarning(OVERFLOW_INTERCEPT, false, "");

    if (EMULATE(OVERFLOW_INTERCEPT))
    {
      int location = (num_intercepts - MAXINTERCEPTS_ORIGINAL - 1) * 12;

      // Overwrite memory that is overwritten in Vanilla Doom, using
      // the values from the intercept structure.
      //
      // Note: the ->d.{thing,line} member should really have its
      // address translated into the correct address value for 
      // Vanilla Doom.

      InterceptsMemoryOverrun(location, intercept->frac);
      InterceptsMemoryOverrun(location + 4, intercept->isaline);
      InterceptsMemoryOverrun(location + 8, (int) intercept->d.thing);
    }
  }
}

// e6y
// playeringame overrun emulation
// it detects and emulates overflows on vex6d.wad\bug_wald(toke).lmp, etc.
// http://www.doom2.net/doom2/research/runningbody.zip

int PlayeringameOverrun(const mapthing_t* mthing)
{
  if (mthing->type == 0 && PROCESS(OVERFLOW_PLYERINGAME))
  {
    // playeringame[-1] == players[3].didsecret
    ShowOverflowWarning(OVERFLOW_PLYERINGAME, (players + 3)->didsecret, "");

    if (EMULATE(OVERFLOW_PLYERINGAME))
    {
      return true;
    }
  }
  return false;
}

//
// spechit overrun emulation
//

unsigned int spechit_baseaddr = 0;

// e6y
// Code to emulate the behavior of Vanilla Doom when encountering an overrun
// of the spechit array.
// No more desyncs on compet-n\hr.wad\hr18*.lmp, all strain.wad\map07 demos etc.
// http://www.doomworld.com/vb/showthread.php?s=&threadid=35214
// See more information on:
// doomworld.com/vb/doom-speed-demos/35214-spechits-reject-and-intercepts-overflow-lists
void SpechitOverrun(spechit_overrun_param_t *params)
{
  int numspechit = *(params->numspechit);

  if (demo_compatibility && numspechit > 8)
  {
    line_t **spechit = *(params->spechit);

    ShowOverflowWarning(OVERFLOW_SPECHIT,
      numspechit > 
        (compatibility_level == dosdoom_compatibility || 
        compatibility_level == tasdoom_compatibility ? 10 : 14), 
      "\n\nThe list of LineID leading to overrun:\n%d, %d, %d, %d, %d, %d, %d, %d, %d.",
      spechit[0]->iLineID, spechit[1]->iLineID, spechit[2]->iLineID,
      spechit[3]->iLineID, spechit[4]->iLineID, spechit[5]->iLineID,
      spechit[6]->iLineID, spechit[7]->iLineID, spechit[8]->iLineID);

    if (EMULATE(OVERFLOW_SPECHIT))
    {
      unsigned int addr;

      if (spechit_baseaddr == 0)
      {
        int p;

        // This is the first time we have had an overrun.  Work out
        // what base address we are going to use.
        // Allow a spechit value to be specified on the command line.

        //
        // Use the specified magic value when emulating spechit overruns.
        //

        p = M_CheckParm("-spechit");
        
        if (p > 0)
        {
          //baseaddr = atoi(myargv[p+1]);
          M_StrToInt(myargv[p+1], (int*)&spechit_baseaddr);
        }
        else
        {
          spechit_baseaddr = DEFAULT_SPECHIT_MAGIC;
        }
      }

      // Calculate address used in doom2.exe

      addr = spechit_baseaddr + (params->line - lines) * 0x3E;

      if (compatibility_level == dosdoom_compatibility || compatibility_level == tasdoom_compatibility)
      {
        // There are no more desyncs in the following dosdoom demos: 
        // flsofdth.wad\fod3uv.lmp - http://www.doomworld.com/sda/flsofdth.htm
        // hr.wad\hf181430.lmp - http://www.doomworld.com/tas/hf181430.zip
        // hr.wad\hr181329.lmp - http://www.doomworld.com/tas/hr181329.zip
        // icarus.wad\ic09uv.lmp - http://competn.doom2.net/pub/sda/i-o/icuvlmps.zip

        switch(numspechit)
        {
        case 9: 
          *(params->tmfloorz) = addr;
          break;
        case 10:
          *(params->tmceilingz) = addr;
          break;
          
        default:
          fprintf(stderr, "SpechitOverrun: Warning: unable to emulate"
                          "an overrun where numspechit=%i\n",
                           numspechit);
          break;
        }
      }
      else
      {
        switch(numspechit)
        {
        case 9: 
        case 10:
        case 11:
        case 12:
          params->tmbbox[numspechit-9] = addr;
          break;
        case 13:
          *(params->nofit) = addr;
          break;
        case 14:
          *(params->crushchange) = addr;
          break;

        default:
          lprintf(LO_ERROR, "SpechitOverrun: Warning: unable to emulate"
                            " an overrun where numspechit=%i\n",
                            numspechit);
          break;
        }
      }
    }
  }
}

//
// reject overrun emulation
//

// padding the reject table if it is too short
// totallines must be the number returned by P_GroupLines()
// an underflow will be padded with zeroes, or a doom.exe z_zone header
// 
// e6y
// reject overrun emulation code
// It's emulated successfully if the size of overflow no more than 16 bytes.
// No more desync on teeth-32.wad\teeth-32.lmp.
// http://www.doomworld.com/vb/showthread.php?s=&threadid=35214

void RejectOverrun(int rejectlump, const byte **rejectmatrix, int totallines)
{
  unsigned int length, required;
  byte *newreject;
  unsigned char pad;

  required = (numsectors * numsectors + 7) / 8;
  length = W_LumpLength(rejectlump);

  if (length < required)
  {
    // allocate a new block and copy the reject table into it; zero the rest
    // PU_LEVEL => will be freed on level exit
    newreject = Z_Malloc(required, PU_LEVEL, NULL);
    *rejectmatrix = memmove(newreject, *rejectmatrix, length);

    // e6y
    // PrBoom 2.2.5 and 2.2.6 padded a short REJECT with 0xff
    // This command line switch is needed for all potential demos 
    // recorded with these versions of PrBoom on maps with too short REJECT
    // I don't think there are any demos that will need it but yes that seems sensible
    pad = prboom_comp[PC_REJECT_PAD_WITH_FF].state ? 0xff : 0;

    memset(newreject + length, pad, required - length);
    // unlock the original lump, it is no longer needed
    W_UnlockLumpNum(rejectlump);
    rejectlump = -1;

    if (demo_compatibility && PROCESS(OVERFLOW_REJECT))
    {
      ShowOverflowWarning(OVERFLOW_REJECT, (required - length > 16) || (length%4 != 0), "");

      if (EMULATE(OVERFLOW_REJECT))
      {
        // merged in RejectOverrunAddInt(), and the 4 calls to it, here
        unsigned int rejectpad[4] = {
          0,        // size, will be filled in using totallines
          0,        // part of the header of a doom.exe z_zone block
          50,       // DOOM_CONST_PU_LEVEL
          0x1d4a11  // DOOM_CONST_ZONEID
        };
        unsigned int i, pad = 0, *src = rejectpad;
        byte *dest = newreject + length;

        rejectpad[0] = ((totallines*4+3)&~3)+24; // doom.exe zone header size

        // copy at most 16 bytes from rejectpad
        // emulating a 32-bit, little-endian architecture (can't memmove)
        for (i = 0; i < (unsigned int)(required - length) && i < 16; i++) { // 16 hard-coded
          if (!(i&3)) // get the next 4 bytes to copy when i=0,4,8,12
            pad = *src++;
          *dest++ = pad & 0xff; // store lowest-significant byte
          pad >>= 8; // rotate the next byte down
        }
      }
    }

    lprintf(LO_WARN, "P_LoadReject: REJECT too short (%u<%u) - padded\n", length, required);
  }
}

//
// Read Access Violation emulation.
//

// C:\>debug
// -d 0:0
//
// DOS 6.22:
// 0000:0000  (57 92 19 00) F4 06 70 00-(16 00)
// DOS 7.1:
// 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
// Win98:
// 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
// DOSBox under XP:
// 0000:0000  (00 00 00 F1) ?? ?? ?? 00-(07 00)

#define DOS_MEM_DUMP_SIZE 10

unsigned char mem_dump_dos622[DOS_MEM_DUMP_SIZE] = {
  0x57, 0x92, 0x19, 0x00, 0xF4, 0x06, 0x70, 0x00, 0x16, 0x00};
unsigned char mem_dump_win98[DOS_MEM_DUMP_SIZE] = {
  0x9E, 0x0F, 0xC9, 0x00, 0x65, 0x04, 0x70, 0x00, 0x16, 0x00};
unsigned char mem_dump_dosbox[DOS_MEM_DUMP_SIZE] = {
  0x00, 0x00, 0x00, 0xF1, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00};

unsigned char *dos_mem_dump = mem_dump_dos622;

static int GetMemoryValue(unsigned int offset, void *value, int size)
{
  static int firsttime = true;

  if (firsttime)
  {
    int p, i, val;

    firsttime = false;
    i = 0;

    if ((p = M_CheckParm("-setmem")) && (p < myargc-1))
    {
      if (!strcasecmp(myargv[p + 1], "dos622"))
        dos_mem_dump = mem_dump_dos622;
      if (!strcasecmp(myargv[p + 1], "dos71"))
        dos_mem_dump = mem_dump_win98;
      else if (!strcasecmp(myargv[p + 1], "dosbox"))
        dos_mem_dump = mem_dump_dosbox;
      else
      {
        while (++p != myargc && *myargv[p] != '-' && i < DOS_MEM_DUMP_SIZE)
        {
          M_StrToInt(myargv[p], &val);
          dos_mem_dump[i++] = (unsigned char)val;
        }
      }
    }
  }

  if (value)
  {
    switch (size)
    {
    case 1:
      *((unsigned char*)value) = *((unsigned char*)(&dos_mem_dump[offset]));
      return true;
    case 2:
      *((unsigned short*)value) = *((unsigned short*)(&dos_mem_dump[offset]));
      return true;
    case 4:
      *((unsigned int*)value) = *((unsigned int*)(&dos_mem_dump[offset]));
      return true;
    }
  }

  return false;
}

//
// donut overrun emulation (linedef action #9)
//

#define DONUT_FLOORPIC_DEFAULT 0x16
int DonutOverrun(fixed_t *pfloorheight, short *pfloorpic)
{
  if (demo_compatibility && PROCESS(OVERFLOW_DONUT))
  {
    ShowOverflowWarning(OVERFLOW_DONUT, 0, "");

    if (EMULATE(OVERFLOW_DONUT))
    {
      if (pfloorheight && pfloorpic)
      {
        GetMemoryValue(0, pfloorheight, 4);
        GetMemoryValue(8, pfloorpic, 2);
        
        // bounds-check floorpic
        if ((*pfloorpic) <= 0 || (*pfloorpic) >= numflats)
        {
          *pfloorpic = MIN(numflats - 1, DONUT_FLOORPIC_DEFAULT);
        }

        return true;
      }
    }
  }

  return false;
}


int MissedBackSideOverrun(line_t *line)
{
  if (demo_compatibility)
  {
    if (line)
    {
      ShowOverflowWarning(OVERFLOW_MISSEDBACKSIDE, 0,
        "\n\nLinedef %d has two-sided flag set, but no second sidedef",
        line->iLineID);
    }
    else
    {
      ShowOverflowWarning(OVERFLOW_MISSEDBACKSIDE, 0, "");
    }
  }

  return false;
}

//
// GetSectorAtNullAddress
//
sector_t* GetSectorAtNullAddress(void)
{
  static int null_sector_is_initialized = false;
  static sector_t null_sector;

  if (demo_compatibility && EMULATE(OVERFLOW_MISSEDBACKSIDE))
  {
    if (!null_sector_is_initialized)
    {
      memset(&null_sector, 0, sizeof(null_sector));
      null_sector.flags = NULL_SECTOR;
      GetMemoryValue(0, &null_sector.floorheight, 4);
      GetMemoryValue(4, &null_sector.ceilingheight, 4);
      null_sector_is_initialized = true;
    }

    return &null_sector;
  }

  return 0;
}
