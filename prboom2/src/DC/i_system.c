/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_system.c,v 1.5 2002/02/10 20:59:44 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *  Misc system stuff needed by Doom, implemented for Dreamcast.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: i_system.c,v 1.5 2002/02/10 20:59:44 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <kos.h>
#include <GL/gl.h>
#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "v_video.h"
#include "lprintf.h"

#define TEX_SIZE 512

GLuint video_tex;
uint16 *tex_mem;
uint32 txr;
uint16 tex_pal[256];

void setbuf(FILE *f, char *fn)
{
}

#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */

int access(const char *path, int mode)
{
	uint32 handle=0;

	if ((mode==F_OK) || (mode==R_OK))
		handle=fs_open(path,O_RDONLY);
	if (mode==W_OK)
		handle=fs_open(path,O_WRONLY);
	if (handle)
		fs_close(handle);
	return (handle==0)?(-1):(0);
}

int sscanf(const char *buf, const char *fmt, ...)
{
	return 0;
}

long strtol(const char *nptr, char **endptr, int def)
{
	return def;
}

uint32 open(const char *fn, int mode)
{
	uint32 handle;

	handle = fs_open(fn,mode);

	if (handle==0)
		return -1;

	return handle;
}

void *signal(int sig, void *func)
{
	return NULL;
}

void I_InitSound(void)
{
}

void I_SetChannels(void)
{
}

int I_StartSound(int id, int channel, int vol, int sep, int pitch, int priority)
{
}

void I_StopSound(int handle)
{
}

int I_GetSfxLumpNum (void *sfxinfo)
{
}

boolean I_SoundIsPlaying(int handle)
{
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
}

void I_InitMusic(void)
{
}

void I_SetMusicVolume(int volume)
{
}

void I_PauseSong(int handle)
{
}

void I_ResumeSong(int handle)
{
}

int I_RegisterSong(const void *data, size_t len)
{
}

void I_PlaySong(int handle, int looping)
{
}

void I_StopSong(int handle)
{
}

void I_UnRegisterSong(int handle)
{
}

/* cphipps - I_GetVersionString
 * Returns a version string in the given buffer 
 */
const char* I_GetVersionString(char* buf, size_t sz)
{
  sprintf(buf,"%s v%s (dreamcast experimental) (http://prboom.sourceforge.net/)",PACKAGE,VERSION);
  return buf;
}

int I_GetTime_RealTime (void)
{
	static time=0;
	
	return time++;
}

/* cphipps - I_SigString
 * Returns a string describing a signal number
 */
const char* I_SigString(char* buf, size_t sz, int signum)
{
  sprintf(buf,"signal %d",signum);
  return buf;
}

/*
 * I_GetRandomTimeSeed
 *
 * CPhipps - extracted from G_ReloadDefaults because it is O/S based
 */
unsigned long I_GetRandomTimeSeed(void)
{                            
/* This isnt very random */
  return(0);
}

void I_uSleep(unsigned long usecs)
{
}

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame

inline static boolean I_SkipFrame(void)
{
  static int frameno;

  frameno++;
  switch (gamestate) {
  case GS_LEVEL:
    if (!paused)
      return false;
  default:
    // Skip odd frames
    return (frameno & 1) ? true : false;
  }
}

void I_FinishUpdate (void)
{
	int x,y;
	uint8 *s;
	uint16 *d;

  if (I_SkipFrame()) return;

	s=screens[0];
	for (y=0; y<SCREENHEIGHT; y++)
	{
		d=&tex_mem[y*TEX_SIZE];
		for (x=0; x<SCREENWIDTH; x++)
		{
			*d++=tex_pal[*s++];
		}
	}

	//printf("I_FinishUpdate %i %i %p %p\n",SCREENWIDTH,SCREENHEIGHT,txr,tex_mem);

	glKosBeginFrame();

	ta_txr_load(txr, tex_mem, TEX_SIZE * TEX_SIZE * 2);

	glBindTexture(GL_TEXTURE_2D, video_tex);

	glBegin(GL_QUADS);
	glVertex3f(-1.0f, -1.0f, 0.5f);
	glVertex3f(10.0f, -1.0f, 0.5f);
	glVertex3f(10.0f, 10.0f, 0.5f);
	glVertex3f(-1.0f, 10.0f, 0.5f);
	glEnd();

	glKosFinishFrame();
#if 0
    poly_hdr_t poly;
	int x,y;
	uint8 *s;
	uint16 *d;
    vertex_ot_t vert;
	
	s=screens[0];
	for (y=0; y<SCREENHEIGHT; y++)
	{
		d=&tex_mem[y*TEX_SIZE*2];
		for (x=0; x<SCREENWIDTH; x++)
		{
			*d++=tex_pal[*s++];
//			*d++=*s++;
		}
	}
    /* Begin opaque polygons */
    ta_begin_render();
    
    /* Send polygon header to the TA using store queues */
    ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_RGB565, TEX_SIZE, TEX_SIZE, video_tex, TA_BILINEAR_FILTER);
    ta_commit_poly_hdr(&poly);

    /* Draw frame */
    
    vert.r = vert.g = vert.b = vert.a = 1.0f;
    vert.oa = vert.or = vert.ob = 0.0f;
    vert.flags = TA_VERTEX_NORMAL;
    vert.z = 2.0f;

    vert.x = 0.0f;
    vert.y = 479.0f;
    vert.u = 0.0;
    vert.v = (float)SCREENHEIGHT/(float)TEX_SIZE;
    ta_commit_vertex(&vert, sizeof(vert));
    
    vert.x = 0.0f;
    vert.y = 0.0f;
    vert.u = 0.0;
    vert.v = 0.0;
    ta_commit_vertex(&vert, sizeof(vert));
    
    vert.x = 639.0f;
    vert.y = 479.0f;
    vert.u = (float)SCREENWIDTH/(float)TEX_SIZE;
    vert.v = (float)SCREENHEIGHT/(float)TEX_SIZE;
    ta_commit_vertex(&vert, sizeof(vert));
    
    vert.x = 639.0f;
    vert.y = 0.0f;
    vert.u = (float)SCREENWIDTH/(float)TEX_SIZE;
    vert.v = 0.0;
    vert.flags = TA_VERTEX_EOL;
    ta_commit_vertex(&vert, sizeof(vert));
    
    /* Begin translucent polygons */
    ta_commit_eol();
    
    /* Finish up */
    ta_commit_eol();
    ta_finish_frame();
#endif
}

void I_UpdateNoBlit (void)
{
}

void I_StartFrame (void)
{
}

void I_StartTic (void)
{
	cont_cond_t cond;
	uint8	c;
	
	c = maple_first_controller();
	/* Check key status */
	if (cont_get_cond(c, &cond) < 0) {
		printf("Error reading controller\n");
		exit(0);
	}
	if (!(cond.buttons & CONT_START))
		exit(0);
}

void I_PreInitGraphics(void)
{
    kos_init_all(IRQ_ENABLE | TA_ENABLE, ROMDISK_NONE);
	/* Get basic stuff initialized */
	glKosInit();
}

void I_SetRes(unsigned int width, unsigned int height)
{
  SCREENWIDTH = (width+3) & ~3;
  SCREENHEIGHT = (height+3) & ~3;

  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics (void)
{
    int            lump = W_GetNumForName("PLAYPAL");
    const byte *palette = W_CacheLumpNum(lump);
    const byte *spal = &palette[768];
	int n;

	for (n=0; n<256; n++)
		tex_pal[n] = (uint16)((((uint8)spal[n*3]>>3)<<11) || (((uint8)spal[n*3+1]>>2)<<5) || ((uint8)spal[n*3+2]>>3));
    W_UnlockLumpNum(lump);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 320.0, 200.0, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_KOS_AUTO_UV);
	glDisable(GL_CULL_FACE);
	glGenTextures(1, &video_tex);
	tex_mem=malloc(TEX_SIZE * TEX_SIZE * 2);
	txr=ta_txr_allocate(TEX_SIZE * TEX_SIZE * 2);
	glBindTexture(GL_TEXTURE_2D, video_tex);
	glKosTex2D(GL_RGB565, TEX_SIZE, TEX_SIZE, txr);
}

void I_UpdateVideoMode(void)
{
  screens[0]=malloc(SCREENWIDTH*SCREENHEIGHT);
  R_InitBuffer(SCREENWIDTH,SCREENHEIGHT);
}

void I_ShutdownGraphics(void)
{
}

void I_ReadScreen (byte* scr)
{
  memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette(int pal)
{
    int            lump = W_GetNumForName("PLAYPAL");
    const byte *palette = W_CacheLumpNum(lump);
    const byte *spal = &palette[768*pal];
	int n;

	for (n=0; n<256; n++)
		tex_pal[n] = (((spal[n*3]>>3)<<11) | ((spal[n*3+1]>>2)<<5) | (spal[n*3+2]>>3));
    W_UnlockLumpNum(lump);
}

/*
 * I_Filelength
 *
 * Return length of an open file.
 */

/* 
 * I_Read
 *
 * cph 2001/11/18 - wrapper for read(2) which handles partial reads and aborts
 * on error.
 */
void I_Read(int fd, void* buf, size_t sz)
{
  while (sz) {
    int rc = fs_read(fd,buf,sz);
    if (rc <= 0) {
      I_Error("I_Read: read failed: %s", /*rc ? strerror(errno) :*/ "EOF");
    }
    sz -= rc; (unsigned char *)buf += rc;
  }
}

int I_Filelength(int handle)
{
	return fs_total(handle);
}

int use_doublebuffer = 0;
int use_fullscreen = 0;
int snd_card = -1;
int mus_card = -1;
int snd_samplerate = 11025;
