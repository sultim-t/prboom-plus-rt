/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_system.c,v 1.3 2001/07/07 15:00:29 proff_fs Exp $
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
rcsid[] = "$Id: i_system.c,v 1.3 2001/07/07 15:00:29 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include <kallisti/libk.h>
#include <kallisti/abi/ta.h>
#include "config.h"
#endif
#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "v_video.h"

#define TEX_SIZE 512
abi_ta_t	*ta = NULL;

unsigned int video_tex;
uint8 *tex_mem;
unsigned texture_pool;
uint16 tex_pal[256];

void exit(int i)
{
	(*(void(**)(int))0x8c0000e0)(-3);
}

#define	_U	01
#define	_L	02
#define	_N	04
#define	_S	010
#define _P	020
#define _C	040
#define _X	0100
#define	_B	0200

const char _ctype_[1 + 256] = {
	0,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_S|_B,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
	_N,	_N,	_N,	_N,	_N,	_N,	_N,	_N,
	_N,	_N,	_P,	_P,	_P,	_P,	_P,	_P,
	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_P,	_P,	_P,	_P,	_P,
	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_P,	_P,	_P,	_P,	_C
};

int isalpha(int c){ return ((_ctype_+1)[(unsigned)(c)]&(_U|_L));}
int isupper(int c){ return ((_ctype_+1)[(unsigned)(c)]&_U);}
int islower(int c){ return ((_ctype_+1)[(unsigned)(c)]&_L);}
int isdigit(int c){ return ((_ctype_+1)[(unsigned)(c)]&_N);}
int isxdigit(int c){ return ((_ctype_+1)[(unsigned)(c)]&(_X|_N));}
int isspace(int c){ return ((_ctype_+1)[(unsigned)(c)]&_S);}
int ispunct(int c){ return ((_ctype_+1)[(unsigned)(c)]&_P);}
int isalnum(int c){ return ((_ctype_+1)[(unsigned)(c)]&(_U|_L|_N));}
//int isprint(int c){ return ((_ctype_+1)[(unsigned)(c)]&(_P|_U|_L|_N|_B));}
int isgraph(int c){ return ((_ctype_+1)[(unsigned)(c)]&(_P|_U|_L|_N));}
int iscntrl(int c){ return ((_ctype_+1)[(unsigned)(c)]&_C);}

int toupper(int c)
{ 
	return islower(c) ? (c - 'a' + 'A') : c;
}

/*
int tolower(int c)
{ 
	return isupper(c) ? (c - 'A' + 'a') : c;
}
*/

int atoi(const char *s)
{
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;
	int base;
	
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	base = 10;
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
	} else if (neg)
		acc = -acc;
	return (acc);
}

int puts(const char * s)
{
    return printf("%s",s);
}

int putchar(const char c)
{
    return printf("%c",c);
}

void *signal(int sig, void *func)
{
}

void ProcessDehFile(const char *filename, const char *outfilename, int lumpnum)
{
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
  sprintf(buf,"%s v%s (http://prboom.sourceforge.net/)",PACKAGE,VERSION);
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

unsigned int texture_alloc(uint32 size)
{
    unsigned int rv;
    
    rv = texture_pool;
    
    /* Align to 8 bytes */  
    size = (size + 7) & (~7);
    texture_pool += size;
    
    printf("Allocating texture at %08x\r\n", rv);
    return rv;
}

void I_FinishUpdate (void)
{
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
    ta->begin_render();
    
    /* Send polygon header to the TA using store queues */
    ta->poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_RGB565, TEX_SIZE, TEX_SIZE, video_tex, TA_BILINEAR_FILTER);
    ta->commit_poly_hdr(&poly);

    /* Draw frame */
    
    vert.r = vert.g = vert.b = vert.a = 1.0f;
    vert.oa = vert.or = vert.ob = 0.0f;
    vert.flags = TA_VERTEX_NORMAL;
    vert.z = 2.0f;

    vert.x = 0.0f;
    vert.y = 479.0f;
    vert.u = 0.0;
    vert.v = (float)SCREENHEIGHT/(float)TEX_SIZE;
    ta->commit_vertex(&vert, sizeof(vert));
    
    vert.x = 0.0f;
    vert.y = 0.0f;
    vert.u = 0.0;
    vert.v = 0.0;
    ta->commit_vertex(&vert, sizeof(vert));
    
    vert.x = 639.0f;
    vert.y = 479.0f;
    vert.u = (float)SCREENWIDTH/(float)TEX_SIZE;
    vert.v = (float)SCREENHEIGHT/(float)TEX_SIZE;
    ta->commit_vertex(&vert, sizeof(vert));
    
    vert.x = 639.0f;
    vert.y = 0.0f;
    vert.u = (float)SCREENWIDTH/(float)TEX_SIZE;
    vert.v = 0.0;
    vert.flags = TA_VERTEX_EOL;
    ta->commit_vertex(&vert, sizeof(vert));
    
    /* Begin translucent polygons */
    ta->commit_eol();
    
    /* Finish up */
    ta->commit_eol();
    ta->finish_frame();
}

void I_UpdateNoBlit (void)
{
}

void I_StartFrame (void)
{
}

void I_StartTic (void)
{
}

void I_PreInitGraphics(void)
{
	printf("Opening TA lib\r\n");
	if (ta == NULL) ta = lib_open("ta");
    if (ta == NULL) { printf("Can't open TA lib\r\n"); exit(1); }

    texture_pool = 0;

	printf("Allocating texture\r\n");
    video_tex = texture_alloc(TEX_SIZE*TEX_SIZE*2);
    tex_mem = ta->txr_map(video_tex);
	*((vuint32*)(0xa05f8108)) = 1;
}

void I_SetRes(unsigned int width, unsigned int height)
{
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
}

void I_UpdateVideoMode(void)
{
}

void I_ShutdownGraphics(void)
{
}

void I_ReadScreen (byte* scr)
{
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


int use_doublebuffer;
int use_fullscreen;
int snd_card, mus_card;

int test(int argc, const char *argv[])
{
  D_DoomMain ();
  return 0;
}
