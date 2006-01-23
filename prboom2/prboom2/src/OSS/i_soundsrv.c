/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  Sound server for LxDoom, based on the sound server released with the 
 *   original linuxdoom sources.
 *  Copyright (C) 1993-1996 by id Software
 *  Copyright (C) 1999 by Colin Phipps
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
 *  Interface to the main program. Initially accepts the sound 
 *  data on stdin. Then accepts sound commands on stdin, adding 
 *  sounds to the playing sound list. Calls the low level sound
 *  output functions at regular intervals.
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#define SNDSERV
#include "sounds.h"
#include "i_soundgen.h"
#include "i_soundsrv.h"
#include "i_system.h"

#define USE_SELECT

static int snd_verbose;

static void I_GetData(void) 
{
    { 
  /* This has been generalised to accept sound data not matching that 
   * expected.
   * The only restriction is that there are fewer than NUMSFX sounds.
   */
      int i;
      unsigned long numsounds;
      
      fread(&numsounds, sizeof(numsounds), 1, stdin);

      for (i=0; i<numsounds; i++) {
	snd_pass_t sfxpass;
	void* databuf = NULL;
	void* crapbuf = NULL;
	
	fread(&sfxpass, sizeof(sfxpass), 1, stdin);
	
	if (sfxpass.sfxid != i) {
	  fprintf(stderr, "I_GetData: Sound ID mismatch\n");
	  exit(-1);
	}
	
	// If we are within the bounds of our array...
	if (i < NUMSFX)
	  if (sfxpass.link == -1) {
	    if (sfxpass.datalen) {
	      databuf = malloc(lengths[i] = sfxpass.datalen);
	      fread(databuf, 1, sfxpass.datalen, stdin);
	      
	      S_sfx[i].data = (unsigned char*)I_PadSfx(databuf, &lengths[i]);
	    } else {
	      // No data available. Make it safe
	      S_sfx[i].data = S_sfx[0].data;
	      lengths[i] = 0;
	    }
	  } else {
	    unsigned int link_num = (unsigned int)(sfxpass.link);
	    S_sfx[i].data = S_sfx[link_num].data;
	    lengths[i]=lengths[link_num];
	  }
	else if (sfxpass.datalen) {
	  // We cannot hold this data, but must clear it from the pipe
	  crapbuf = malloc(sfxpass.datalen);
	  fread(crapbuf, 1, sfxpass.datalen, stdin);
	  free(crapbuf);
	}
      }
    }
}

static fd_set		fdset;
static fd_set		scratchset;

int main(int argc, const char** argv)
{
    struct timeval	zerowait = { 0, 0 };
    int		done = 0;
    int		rc, nrc;
    int		sndnum;
    int		handle = 0;
    int         badcmd = 0;
    
    unsigned char	commandbuf[10];
    
    int 	pitch, vol, sep;
    int         thistime, lasttime = I_GetTime_RealTime();

    // Debugging output?
    snd_verbose = 0;
    if (argc >= 3)
      if (!stricmp(argv[2], "-devparm"))
	snd_verbose = 1;
    if (argc >= 4)
      if (!stricmp(argv[3], "-devparm"))
	snd_verbose = 1;

    I_InitSoundGen((argv[1] != NULL) ? argv[1] : "/dev/dsp");

    usleep(200000);
    // get sound data
    I_GetData();

    if (snd_verbose)
      fprintf(stderr, "ready\n");
    
    // parse commands and play sounds until done
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);

    while (!done) 
      {
        // Sync at 35Hz
        while (lasttime == (thistime = I_GetTime_RealTime())) {
	  usleep(1000);
        }
      
	lasttime = thistime;

	do {
	  scratchset = fdset;
	  rc = select(FD_SETSIZE, &scratchset, 0, 0, &zerowait);
	  
	  if (rc > 0)
	  {
	    //	fprintf(stderr, "select is true\n");
	    // got a command
	    rc = 1;
	    nrc = read(STDIN_FILENO, commandbuf, 1);

	    if (nrc <= 0) {
	      done = 1;
	      if (snd_verbose) 
		fprintf(stderr, "select true but no data: exiting\n");
	      rc = 0;
	    }
	    else {
	      if (snd_verbose)
		fprintf(stderr, "cmd: %c", commandbuf[0]);
	      
	      switch (commandbuf[0]) {
	      case 'p':
		// play a new sound effect
		read(STDIN_FILENO, commandbuf, 9);
		
		if (snd_verbose) {
		  commandbuf[9]=0;
		  fprintf(stderr, "%s\n", commandbuf);
		}
		
		commandbuf[0] -=
		  commandbuf[0]>='a' ? 'a'-10 : '0';
		commandbuf[1] -=
		  commandbuf[1]>='a' ? 'a'-10 : '0';
		commandbuf[2] -=
		  commandbuf[2]>='a' ? 'a'-10 : '0';
		commandbuf[3] -=
		  commandbuf[3]>='a' ? 'a'-10 : '0';
		commandbuf[4] -=
		  commandbuf[4]>='a' ? 'a'-10 : '0';
		commandbuf[5] -=
		  commandbuf[5]>='a' ? 'a'-10 : '0';
		commandbuf[6] -=
		  commandbuf[6]>='a' ? 'a'-10 : '0';
		commandbuf[7] -=
		  commandbuf[7]>='a' ? 'a'-10 : '0';
		
		//	p<snd#><pitch><vol><sep>
		sndnum = (commandbuf[0]<<4) + commandbuf[1];
		pitch = (commandbuf[2]<<4) + commandbuf[3];
		vol = (commandbuf[4]<<4) + commandbuf[5];
		sep = (commandbuf[6]<<4) + commandbuf[7];
		
		if (sndnum < NUMSFX)
		  handle = I_AddSfx(sndnum, vol, pitch, sep);
		// returns the handle
		//	outputushort(handle);
		break;
		
	      case 'q':
		read(STDIN_FILENO, commandbuf, 1);
		done = 1; rc = 0;
		break;
		
	      case 's':
		{
		  int fd;
		  read(STDIN_FILENO, commandbuf, 3);
		  commandbuf[2] = 0;
		  fd = open((char*)commandbuf, O_CREAT|O_WRONLY, 0644);
		  commandbuf[0] -= commandbuf[0]>='a' ? 'a'-10 : '0';
		  commandbuf[1] -= commandbuf[1]>='a' ? 'a'-10 : '0';
		  sndnum = (commandbuf[0]<<4) + commandbuf[1];
		  write(fd, S_sfx[sndnum].data, lengths[sndnum]);
		  close(fd);
		}
	      break;
	      
	      default:
		if (!badcmd) // cph - first char of bad command sequence
		  fprintf(stderr, "sndserver: Bad command:");
		badcmd += 2;
		fprintf(stderr, "`%c' %d",commandbuf[0], commandbuf[0]);
		break;
	      }
	      // cph - carriage return after bad command line
	      if (badcmd)
		if (!(badcmd>>=1)) putc('\n',stderr);
	    }
	  }
	  else if (rc < 0) {
	    exit(0);
	  }
	} while (rc > 0);
	
	
	I_UpdateSound();
	I_SubmitSound();
	
      }
    
    I_EndSoundGen();
    return 0;
}
