/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: l_sound.c,v 1.1 2000/05/04 08:07:27 proff_fs Exp $
 *
 *  Sound interface from the original linuxdoom, extensively modified
 *  for LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Colin Phipps
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
 *	System interface for sound.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: l_sound.c,v 1.1 2000/05/04 08:07:27 proff_fs Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include <math.h>

#include <sys/time.h>

#include "z_zone.h"

#include "i_main.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "d_main.h"

#include "sounds.h"

#include "lprintf.h"

/* cph - catch broken pipes */
#define PIPE_CHECK(fh) if (broken_pipe) { fclose(fh); fh = NULL; broken_pipe = 0; }

/* Variables used by Boom from Allegro
 * created here to avoid changes to core Boom files
 */
int snd_card = 1;
int mus_card = 1;

#include "l_soundsrv.h"

// Separate sound server process.
static FILE*	sndserver=0;

//
// MUSIC API.
//

static FILE* musserver;
static const unsigned char* musdata;
static size_t muslen;
static int playing_handle, loaded_handle;
static boolean muspaused, looping_current;

void I_ShutdownMusic(void) 
{
  musdata = NULL;

  if (musserver) {
    fputc('Q', musserver);
    fclose(musserver);
  }
}

void I_InitMusic(void)
{
  paused = false;
  if (!access(musserver_filename, X_OK)) {
    char buf[1024]; // WARNING - potential buffer overrun

    strcpy(buf, musserver_filename);
    if (devparm) strcat(buf, " -v");

    { // Parse other parameters gives
      int i;

      for (i=1; i<myargc; i++) {
	const char musparmstart[] = { "-mus-" };
	if (!strncmp(myargv[i], musparmstart, strlen(musparmstart))) { 
	  strcat(buf, " ");
	  strcat(buf, myargv[i] + strlen(musparmstart));
	}
      }

      if (devparm) fprintf(stderr, "Musserver command line: %s\n", buf);
    }
 
    musserver = popen(buf, "w");
 
    { // Pass GENMIDI
      int lump = W_GetNumForName("GENMIDI");
      size_t len = W_LumpLength(lump);
      //      fwrite(&len, 1, sizeof(size_t), musserver); 
#ifndef DOSDOOM
      fwrite(W_CacheLumpNum(lump), len, 1, musserver);
      W_UnlockLumpNum(lump);
#else
      fwrite(W_CacheLumpNum(lump, PU_CACHE), len, 1, musserver);
#endif
      fflush(musserver);
      PIPE_CHECK(musserver);
    }
  } else 
    fprintf(stderr, "No access to music server: %s\n", musserver_filename);
}

void I_PlaySong(int handle, int looping)
{
  if (musserver == NULL) return;
#ifdef RANGECHECK
  if (musdata == NULL) 
    fprintf(stderr, "I_PlaySong: no MUS data available\n");
  else if (loaded_handle != handle)
    fprintf(stderr, "I_PlaySong: non-current handle used\n");
  else 
#endif
  {
    /* cph - must use the length as specified by the MUS data, not the 
     * lump length (midi2mus must get this wrong sometimes),
     * as lxmusserv expects the former.
     * So take a copy fo the header, and correct it if necessary.
     */
    struct {
      char sig[4];
      unsigned short scorelen;
      unsigned short scorestart;
      unsigned short channels;
      unsigned short sec_channels;
      unsigned short instrumentcount;
      unsigned short pad;
    } __attribute__ ((packed)) musheader;
    size_t MUSlen;

    memcpy(&musheader, musdata, sizeof(musheader));
    MUSlen = sizeof(musheader) 
      + sizeof(unsigned short) * musheader.instrumentcount
      + musheader.scorelen;

    if (MUSlen < muslen) {
      lprintf(LO_WARN, "I_PlaySong: excess data in MUS lump (%d > %d), correcting header\n", muslen, MUSlen);
      // Correct the MUS header, by Gady Kozma <gady@math.tau.ac.il>
      musheader.scorelen += muslen - MUSlen; MUSlen = muslen;
    } else if (MUSlen > muslen) {
      lprintf(LO_ERROR, "I_PlaySong: Incomplete MUS lump (%d < %d)\n", muslen, MUSlen);
      return;
    }
    fprintf(musserver, "N%d\n", looping_current = looping);
    /* cph - write the modified header, then the rest of the data */
    fwrite(&musheader, sizeof(musheader), 1, musserver);
    fwrite(musdata + sizeof(musheader), 
	   MUSlen - sizeof(musheader), 1, musserver);
    fflush(musserver);
    PIPE_CHECK(musserver);
    playing_handle = loaded_handle;
  }
}

extern int mus_pause_opt; // From m_misc.c

void I_PauseSong (int handle)
{
  switch(mus_pause_opt) {
  case 0:
    I_StopSong(handle);
    break;
  case 1:
    fputc('P', musserver);
    fflush(musserver);
    muspaused = true;
    break;
  }
  // Default - let music continue
}

void I_ResumeSong (int handle)
{
  if (playing_handle == 0)
    I_PlaySong(handle, looping_current);
  if (muspaused) {
    fputc('R', musserver);
    fflush(musserver);
    muspaused = false;
  }
}

void I_StopSong(int handle)
{
  if ((musserver == NULL) || (playing_handle == 0))
    return;

  fputc('S', musserver);
  fflush(musserver);
  playing_handle = 0;
}

void I_UnRegisterSong(int handle)
{
  if (handle != loaded_handle)
    fprintf(stderr, "I_UnRegisterSong: song not registered\n");
  else {
    if (musdata != NULL) {
      musdata = NULL;
    }

    if (playing_handle == loaded_handle)
      I_StopSong(playing_handle);

    loaded_handle = 0;
  }
}

int I_RegisterSong(const void* data, size_t len)
{
  {
    const char* sig = data;

    if (sig[0] != 'M') return 0;
    if (sig[1] != 'U') return 0;
    if (sig[2] != 'S') return 0;
  }

  muslen = len;
  musdata = data;

  return (++loaded_handle);
}

void I_SetMusicVolume(int volume)
{
  if (musserver == NULL) return;
  fprintf(musserver, "V%d\n", snd_MusicVolume = volume);
  fflush(musserver);
}

//
// SFX API
//

void I_SetChannels(void)
{
  // CPhipps - moved to I_InitSoundGen
  // Redundant
}	
 
//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
  char namebuf[9];
  int lump;

  sprintf(namebuf, "ds%s", sfx->name);

  if ((lump = W_CheckNumForName(namebuf)) == -1) {
    // Clumsy hack - IPC does not support missing sounds
    return (-1);
  } else
    return lump;
}

//
// Starting a sound means sending a message to
// the sound server to start the numbered sound
//
int I_StartSound(int id, int vol, int sep, int pitch, int priority)
{
  // UNUSED
  priority = 0; 
  vol<<=2;
  
  if (sndserver) {
    fprintf(sndserver, "p%2.2x%2.2x%2.2x%2.2x\n", id, pitch, vol, sep);
    fflush(sndserver);
    PIPE_CHECK(sndserver);
  }

  return id;
}

void I_StopSound (int handle)
{
  // Stop a playing sound. Not worth it

  handle = 0;
}

boolean I_SoundIsPlaying(int handle)
{
  // Not easy to do without keeping an extra redundant copy of 
  // the playing sound tables here, or more pipe traffic

  return gametic < handle;
}

void I_UpdateSoundParams( int handle, int vol, int sep, int pitch)
{
  // Should change the attributes on a playing sound, but 
  // to do this requires a lot of traffic on our pipe 
  // which would introduce too much latency IMO
  handle = vol = sep = pitch = 0;
}

void I_ShutdownSound(void)
{    
  if (sndserver) {
    // Send a "quit" command.
    fprintf(sndserver, "q\n");
    fflush(sndserver);
  }

  I_ShutdownMusic();

  return;
}

void I_InitSound(void)
{ 
  // start sound process
  if ( !access(sndserver_filename, X_OK) ) {
    char buf[1024];

    snprintf(buf, sizeof(buf), "%s %s %s", sndserver_filename, snd_device,
	    devparm ? "-devparm" : "");
    sndserver = popen(buf, "w");
    atexit(I_ShutdownSound);

    fprintf(stderr, "I_InitSound: Passing sound data to %s via ", 
	    sndserver_filename);

    { /* Write data into pipe. */
      snd_pass_t sfxpass;
      const void* s_data = NULL;
      int i;

      fprintf(stderr, "pipe: ");

      {
	// Write the number of sound effects to be passed
	unsigned long i = NUMSFX;
	
	fwrite(&i, sizeof(i), 1, sndserver);
      }

      for (i=0; sndserver && (i<NUMSFX); i++) {
	int lump = -1;
	sfxpass.sfxid = i;
	if ((sfxpass.link = I_GetLinkNum(i)) == -1) {
	  lump = I_GetSfxLumpNum(&S_sfx[i]);
	  
	  if (lump == -1) {
	    sfxpass.datalen = 0;
	  } else {
	    sfxpass.datalen = W_LumpLength(lump);
#ifndef DOSDOOM
	    s_data = W_CacheLumpNum(lump);
#else
	    s_data = W_CacheLumpNum(lump, PU_CACHE);
#endif
	  }
	} else {
	  sfxpass.datalen = 0;;
	}

	fwrite(&sfxpass, sizeof(sfxpass), 1, sndserver);
	if (sfxpass.datalen) {
	  fwrite(s_data, 1, sfxpass.datalen, sndserver);
#ifndef DOSDOOM
	  W_UnlockLumpNum(lump);
#endif
	}
	PIPE_CHECK(sndserver);
      }
      fprintf(stderr, "sent OK\n");
    }
  } else
    fprintf(stderr, "Could not start sound server [%s]\n",sndserver_filename);

  if (!nomusicparm)
    I_InitMusic();
  
#ifdef DOSDOOM
  atexit(I_ShutdownSound);
  return true;
#endif
}

#ifdef DOSDOOM

int snd_CDMusicVolume;
cdType_t cdaudio;

void I_SetSfxVolume(int vol) { };
void I_UpdateSound(void) { };
void I_SetCDMusicVolume(int vol) { };

#endif
/*
 * $Log: l_sound.c,v $
 * Revision 1.1  2000/05/04 08:07:27  proff_fs
 * Initial revision
 *
 * Revision 1.41  2000/04/09 13:23:24  cph
 * Remove PCX screenshot support
 * Remove DOS-specific config file options
 * Add level_precache option
 *
 * Revision 1.40  2000/04/03 22:10:12  cph
 * Const parameter to I_RegisterSong
 * Reduce duplication of music data
 *
 * Revision 1.39  2000/03/19 20:14:32  cph
 * Sound code cleaning: DosDoom, IPC and such unused code removed
 *
 * Revision 1.38  1999/12/18 22:23:35  cphipps
 * Fix bug where music only played once after a resume from pause when
 *  mus_pause_opt was 0
 *
 * Revision 1.37  1999/10/31 16:27:59  cphipps
 * Change i_system.h include to i_main.h to get system stuff
 * Minor comment editing
 *
 * Revision 1.36  1999/10/12 13:01:11  cphipps
 * Changed header to GPL
 *
 * Revision 1.35  1999/09/05 21:26:00  cphipps
 * Imported a fix from Gady Kozma, which edits the MUS lump header to reflect the true length of
 * the data; this means no data is lost in passing to the music server with buggy MUS lumps
 * (e.g. Requiem, Memento Mori, ...)
 *
 * Revision 1.34  1999/07/03 13:14:49  cphipps
 * Added code to check for broken pipes and close them
 *
 * Revision 1.33  1999/03/23 13:05:24  cphipps
 * Remove more DosDoom stuff
 * Use the length specified in the MUS lump rather than the lump length from the WAD, to
 * be consistent with lxmusserver; report mismatches
 *
 * Revision 1.32  1999/03/13 10:06:51  cphipps
 * Interface changes for new musserver, from Rafael Reilova
 *
 * Revision 1.31  1999/03/02 13:33:09  cphipps
 * Music pausing/unpausing enabled
 *
 * Revision 1.30  1999/01/10 13:05:36  cphipps
 * More DosDoom sound stuff
 *
 * Revision 1.29  1999/01/03 12:02:20  cphipps
 * Wad lump handling updated
 *
 * Revision 1.28  1998/12/25 17:15:22  cphipps
 * Scale volumes from Doom internal scale to soundserver scale
 *
 * Revision 1.27  1998/12/07 17:14:41  cphipps
 * Sound now works for dosdoom
 *
 * Revision 1.26  1998/12/07 09:58:39  cphipps
 * Use option variable from m_misc to determine what to do to music
 * when the game is paused
 *
 * Revision 1.25  1998/12/02 18:52:50  cphipps
 * Add spaces between params to musserver :)
 * Add I_ShutdownSound call for DosDoom
 *
 * Revision 1.24  1998/12/02 08:48:55  cphipps
 * Allow parameter passing to the musserver
 * Verified new musserver is working
 *
 * Revision 1.23  1998/12/01 23:06:27  cphipps
 * Fix parameters for new musserver
 * Pass NUMSFX to sound server
 *
 * Revision 1.22  1998/11/25 17:27:11  cphipps
 * Mask non-mus musics
 * Disable music pausing for now, always stop
 *
 * Revision 1.21  1998/11/17 16:38:16  cphipps
 * Sound updates so DosDoom compiles
 *
 * Revision 1.20  1998/10/31 08:58:17  cphipps
 * Pass volume messages
 *
 * Revision 1.19  1998/10/26 19:45:08  cphipps
 * The music server will need GENMIDI to set up the instruments,
 * so now pass that immediately on starting it.
 *
 * Revision 1.18  1998/10/22 17:19:12  cphipps
 * Fix music passing struct
 *
 * Revision 1.17  1998/10/22 17:06:42  cphipps
 * Remove l_musserver.h
 *
 * Revision 1.16  1998/10/20 19:39:23  cphipps
 * Minor music server fix
 *
 * Revision 1.15  1998/10/20 19:35:36  cphipps
 * Pass -devparm to music server
 * Move music server filename to config file
 *
 * Revision 1.14  1998/10/20 14:44:45  cphipps
 * Add length parameter to I_RegisterSong
 *
 * Revision 1.13  1998/10/18 19:17:23  cphipps
 * Removed all redundant code: functions from the Sfx and Mus API's that Boom
 * didn't use, and all the timer/internal sound code.
 * Removed most of the #ifdef's, so everything is more readable now :-)
 *
 * Revision 1.12  1998/10/18 19:01:27  cphipps
 * Removed sound-generation related headers, only l_soundgen.c needs them
 * Remarked out some redundant functions
 * Specified a couple of prototypes better
 * Minor fixes to allow non-SNDSERV code to compile
 *
 * Revision 1.11  1998/10/15 07:08:50  cphipps
 * Pass devparm parameter on to sound server
 *
 * Revision 1.10  1998/10/13 17:54:32  cphipps
 * Both IPC and pipe data passing code is compiled, and choice made at run time by
 * user
 *
 * Revision 1.9  1998/10/11 08:18:57  cphipps
 * Pass sound data via pipe instead of IPC
 *
 * Revision 1.8  1998/10/10 20:34:47  cphipps
 * Pass sound device to sound server
 *
 * Revision 1.7  1998/10/06 17:10:13  cphipps
 * Musserver load and communication written in, yet to be used/tested.
 * Added some headers that xdoom uses, may help people compiling for other platforms.
 *
 * Revision 1.6  1998/10/01 08:21:22  cphipps
 * Fixed warning by gcc due to the stupid type of pointer needed by shmdt
 *
 * Revision 1.5  1998/09/27 13:13:35  cphipps
 * Added timeout to soundserver IPC
 *
 * Revision 1.4  1998/09/20 16:09:05  cphipps
 * Implemented IPC shared memory method of passing sound data to sndserv. This
 * enables sounds-in-a-PWAD (which were not supported under the original
 * soundserver), and removed a lot of redundancy in the code for the soundserver.
 *
 * Revision 1.3  1998/09/20 11:02:03  cphipps
 * Extracted linux sound output code to l_soundgen.c.
 *
 * This file now does only this:
 * - interfaces to w_wad.c for getting sound data
 * - provides the sound & music APIs, mostly dummies
 * - calls sndserv for separate soundserver if that is the selected method of output
 * - provides the timer for asynchronous sound output if that is the selected mathod of sound output (this bit needs working on)
 *
 * Revision 1.2  1998/09/18 15:44:45  cphipps
 * Got internal and timer based sound generation working. Quality is still
 * very poor though.
 *
 * Allocate internal sound mixing structures dynamically, to save memory when
 * using -nosound.
 *
 * Do not include any internal mixing structures or subroutines when using
 * sound server.
 *
 * Made many functions static, added I_ to some internal functions
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 */
