/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_system.c,v 1.8 2002/02/11 19:21:52 proff_fs Exp $
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
rcsid[] = "$Id: i_system.c,v 1.8 2002/02/11 19:21:52 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <kos.h>
#include <GL/gl.h>
#include "doomdef.h"
#include "doomstat.h"
#include "m_argv.h"
#include "d_main.h"
#include "v_video.h"
#include "lprintf.h"

#define TEX_SIZE 512

GLuint video_tex;
uint8 *tex_mem;
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
  return (jiffies*(1000/HZ)*TICRATE)/1000;
/*
	static time=0;
	
	return time++;
*/
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
  return((unsigned long)rtc_unix_secs());
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
	//uint8 *d;

  if (I_SkipFrame()) return;

	s=screens[0];
	for (y=0; y<SCREENHEIGHT; y++)
	{
		d=(uint16 *)&tex_mem[y*TEX_SIZE*2];
		for (x=0; x<SCREENWIDTH; x++)
		{
			*d++=tex_pal[*s++];
			//*d++=*s++;
		}
	}

	//printf("I_FinishUpdate %i %i %p %p\n",SCREENWIDTH,SCREENHEIGHT,txr,tex_mem);

	glKosBeginFrame();

	ta_txr_load(txr, tex_mem, TEX_SIZE * TEX_SIZE * 2);
	//txr_twiddle_copy_general(tex_mem, txr, TEX_SIZE, TEX_SIZE, 8);

	glBindTexture(GL_TEXTURE_2D, video_tex);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0,0.0);
	glVertex3f(0.0f, 0.0f, 0.5f);
	glTexCoord2f(1.0/(float)TEX_SIZE*320.0,0.0);
	glVertex3f(320.0f, 0.0f, 0.5f);
	glTexCoord2f(1.0/(float)TEX_SIZE*320.0,1.0/(float)TEX_SIZE*200.0);
	glVertex3f(320.0f, 200.0f, 0.5f);
	glTexCoord2f(0.0,1.0/(float)TEX_SIZE*200.0);
	glVertex3f(0.0f, 200.0f, 0.5f);
	glEnd();

	glKosFinishFrame();
}

void I_UpdateNoBlit (void)
{
}

void I_StartFrame (void)
{
}

static int I_TranslateKey(uint8 key)
{
  int rc = 0;

  switch (key)
  {
  case KBD_KEY_LEFT:		rc = KEYD_LEFTARROW;	break;
  case KBD_KEY_RIGHT:		rc = KEYD_RIGHTARROW;	break;
  case KBD_KEY_DOWN:		rc = KEYD_DOWNARROW;	break;
  case KBD_KEY_UP:		rc = KEYD_UPARROW;	break;
  case KBD_KEY_ESCAPE:		rc = KEYD_ESCAPE;	break;
  case KBD_KEY_ENTER:		rc = KEYD_ENTER;	break;
  case KBD_KEY_TAB:		rc = KEYD_TAB;		break;
  case KBD_KEY_F1:		rc = KEYD_F1;		break;
  case KBD_KEY_F2:		rc = KEYD_F2;		break;
  case KBD_KEY_F3:		rc = KEYD_F3;		break;
  case KBD_KEY_F4:		rc = KEYD_F4;		break;
  case KBD_KEY_F5:		rc = KEYD_F5;		break;
  case KBD_KEY_F6:		rc = KEYD_F6;		break;
  case KBD_KEY_F7:		rc = KEYD_F7;		break;
  case KBD_KEY_F8:		rc = KEYD_F8;		break;
  case KBD_KEY_F9:		rc = KEYD_F9;		break;
  case KBD_KEY_F10:		rc = KEYD_F10;		break;
  case KBD_KEY_F11:		rc = KEYD_F11;		break;
  case KBD_KEY_F12:		rc = KEYD_F12;		break;
  case KBD_KEY_BACKSPACE:	rc = KEYD_BACKSPACE;	break;
  case KBD_KEY_DEL:		rc = KEYD_DEL;		break;
  case KBD_KEY_INSERT:		rc = KEYD_INSERT;	break;
  case KBD_KEY_PGUP:		rc = KEYD_PAGEUP;	break;
  case KBD_KEY_PGDOWN:		rc = KEYD_PAGEDOWN;	break;
  case KBD_KEY_HOME:		rc = KEYD_HOME;		break;
  case KBD_KEY_END:		rc = KEYD_END;		break;
  case KBD_KEY_PAUSE:		rc = KEYD_PAUSE;	break;
  case KBD_KEY_PLUS:		rc = KEYD_EQUALS;	break;
  case KBD_KEY_MINUS:		rc = KEYD_MINUS;	break;
  case KBD_KEY_PAD_0:		rc = KEYD_KEYPAD0;	break;
  case KBD_KEY_PAD_1:		rc = KEYD_KEYPAD1;	break;
  case KBD_KEY_PAD_2:		rc = KEYD_KEYPAD2;	break;
  case KBD_KEY_PAD_3:		rc = KEYD_KEYPAD3;	break;
  case KBD_KEY_PAD_4:		rc = KEYD_KEYPAD4;	break;
  case KBD_KEY_PAD_5:		rc = KEYD_KEYPAD5;	break;
  case KBD_KEY_PAD_6:		rc = KEYD_KEYPAD6;	break;
  case KBD_KEY_PAD_7:		rc = KEYD_KEYPAD7;	break;
  case KBD_KEY_PAD_8:		rc = KEYD_KEYPAD8;	break;
  case KBD_KEY_PAD_9:		rc = KEYD_KEYPAD9;	break;
  case KBD_KEY_PAD_PLUS:	rc = KEYD_KEYPADPLUS;	break;
  case KBD_KEY_PAD_MINUS:	rc = KEYD_KEYPADMINUS;	break;
  case KBD_KEY_PAD_DIVIDE:	rc = KEYD_KEYPADDIVIDE;	break;
  case KBD_KEY_PAD_MULTIPLY: 	rc = KEYD_KEYPADMULTIPLY; break;
  case KBD_KEY_PAD_ENTER:	rc = KEYD_KEYPADENTER;	break;
  case KBD_KEY_PAD_PERIOD:	rc = KEYD_KEYPADPERIOD;	break;
  case KBD_KEY_CAPSLOCK: 	rc = KEYD_CAPSLOCK; 	break;
  case KBD_KEY_0: 		rc = '0'; 		break;
  case KBD_KEY_1: 		rc = '1'; 		break;
  case KBD_KEY_2: 		rc = '2'; 		break;
  case KBD_KEY_3: 		rc = '3'; 		break;
  case KBD_KEY_4: 		rc = '4'; 		break;
  case KBD_KEY_5: 		rc = '5'; 		break;
  case KBD_KEY_6: 		rc = '6'; 		break;
  case KBD_KEY_7: 		rc = '7'; 		break;
  case KBD_KEY_8: 		rc = '8'; 		break;
  case KBD_KEY_9: 		rc = '9'; 		break;
  case KBD_KEY_A: 		rc = 'A'; 		break;
  case KBD_KEY_B: 		rc = 'B'; 		break;
  case KBD_KEY_C: 		rc = 'C'; 		break;
  case KBD_KEY_D: 		rc = 'D'; 		break;
  case KBD_KEY_E: 		rc = 'E'; 		break;
  case KBD_KEY_F: 		rc = 'F'; 		break;
  case KBD_KEY_G: 		rc = 'G'; 		break;
  case KBD_KEY_H: 		rc = 'H'; 		break;
  case KBD_KEY_I: 		rc = 'I'; 		break;
  case KBD_KEY_J: 		rc = 'J'; 		break;
  case KBD_KEY_K: 		rc = 'K'; 		break;
  case KBD_KEY_L: 		rc = 'L'; 		break;
  case KBD_KEY_M: 		rc = 'M'; 		break;
  case KBD_KEY_N: 		rc = 'N'; 		break;
  case KBD_KEY_O: 		rc = 'O'; 		break;
  case KBD_KEY_P: 		rc = 'P'; 		break;
  case KBD_KEY_Q: 		rc = 'Q'; 		break;
  case KBD_KEY_R: 		rc = 'R'; 		break;
  case KBD_KEY_S: 		rc = 'S'; 		break;
  case KBD_KEY_T: 		rc = 'T'; 		break;
  case KBD_KEY_U: 		rc = 'U'; 		break;
  case KBD_KEY_V: 		rc = 'V'; 		break;
  case KBD_KEY_W: 		rc = 'W'; 		break;
  case KBD_KEY_X: 		rc = 'X'; 		break;
  case KBD_KEY_Y: 		rc = 'Y'; 		break;
  case KBD_KEY_Z: 		rc = 'Z'; 		break;
  case KBD_KEY_SPACE: 		rc = ' '; 		break;
  case KBD_KEY_LBRACKET: 	rc = '['; 		break;
  case KBD_KEY_RBRACKET: 	rc = ']'; 		break;
  case KBD_KEY_BACKSLASH: 	rc = '\\'; 		break;
  case KBD_KEY_SEMICOLON: 	rc = ';'; 		break;
  case KBD_KEY_QUOTE: 		rc = '\''; 		break;
  case KBD_KEY_COMMA: 		rc = ','; 		break;
  case KBD_KEY_PERIOD: 		rc = '.'; 		break;
  case KBD_KEY_SLASH: 		rc = '/'; 		break;
  case KBD_KEY_TILDE: 		rc = '`'; 		break;
  default:		  	rc = 0;  		break;
  }

  return rc;
}

static int I_TranslateModKey(uint8 key)
{
  int rc = 0;

  switch (key)
  {
  case 0:	rc = KEYD_RCTRL;	break;
  case 1:	rc = KEYD_RSHIFT;	break;
  case 2:	rc = KEYD_RALT;		break;
  case 3:	rc = 0;			break;
  case 4:	rc = KEYD_RCTRL;	break;
  case 5:	rc = KEYD_RSHIFT;	break;
  case 6:	rc = KEYD_RALT;		break;
  case 7:	rc = 0;			break;
  default:	rc = 0;  		break;
  }

  return rc;
}

void I_StartTic (void)
{
	cont_cond_t cont_cond;
	kbd_cond_t kb_cond;
	
	/* Check key status */
	if (cont_get_cond(maple_first_controller(), &cont_cond) >= 0)
		if (!(cont_cond.buttons & CONT_START))
	    		C_RunTextCmd("quit");
	
	if (kbd_get_cond(maple_first_kb(), &kb_cond) >= 0)
	{
		static uint8 kbd_matrix[256] = {0};
		static uint8 kbd_modifiers[8] = {0};
		int i;
		event_t event;

		/* Process all pressed modifier keys */
    		for (i=0; i<8; i++)
    		{
			if ((kb_cond.modifiers & (1<<i)) != 0)
			{
				int p = kbd_modifiers[i];
				kbd_modifiers[i] = 2;	/* 2 == currently pressed */
				if (!p)
				{
			    		event.type = ev_keydown;
	    				event.data1 = I_TranslateModKey(i);
    					D_PostEvent(&event);
				}
			}
		}

		/* Now normalize the modifier key matrix */
		for (i=0; i<8; i++)
		{
			if (kbd_modifiers[i] == 1)
			{
				kbd_modifiers[i] = 0;
		    		event.type = ev_keyup;
    				event.data1 = I_TranslateModKey(i);
   				D_PostEvent(&event);
			}
			else if (kbd_modifiers[i] == 2)
				kbd_modifiers[i] = 1;
		}

		/* Process all pressed keys */
		for (i=0; i<6; i++)
		{
			if (kb_cond.keys[i] != 0)
			{
				int p = kbd_matrix[kb_cond.keys[i]];
				kbd_matrix[kb_cond.keys[i]] = 2;	/* 2 == currently pressed */
				if (!p)
				{
			    		event.type = ev_keydown;
	    				event.data1 = I_TranslateKey(kb_cond.keys[i]);
    					D_PostEvent(&event);
				}
			}
		}
	
		/* Now normalize the key matrix */
		for (i=0; i<256; i++)
		{
			if (kbd_matrix[i] == 1)
			{
				kbd_matrix[i] = 0;
		    		event.type = ev_keyup;
    				event.data1 = I_TranslateKey(i);
   				D_PostEvent(&event);
			}
			else if (kbd_matrix[i] == 2)
				kbd_matrix[i] = 1;
		}
	}
}

void I_PreInitGraphics(void)
{
    kos_init_all(IRQ_ENABLE | TA_ENABLE | THD_ENABLE, ROMDISK_NONE);
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
	glOrtho(0.0, 320.0, 0.0, 200.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	//glEnable(GL_KOS_AUTO_UV);
	glDisable(GL_CULL_FACE);
	glGenTextures(1, &video_tex);
	tex_mem=malloc(TEX_SIZE * TEX_SIZE * 2);
	txr=ta_txr_allocate(TEX_SIZE * TEX_SIZE * 2);
	glBindTexture(GL_TEXTURE_2D, video_tex);
	glKosTex2D(GL_RGB565, TEX_SIZE, TEX_SIZE, txr);
	//glKosTex2D(TA_PAL8BPP, TEX_SIZE, TEX_SIZE, txr);
	//pvr_set_pal_format(PVR_PAL_RGB565);
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
	{
		tex_pal[n] = (((spal[n*3]>>3)<<11) | ((spal[n*3+1]>>2)<<5) | (spal[n*3+2]>>3));
		//pvr_set_pal_entry(n,(((spal[n*3]>>3)<<11) | ((spal[n*3+1]>>2)<<5) | (spal[n*3+2]>>3)));
	}
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
    ssize_t rc;
    rc = fs_read(fd,buf,sz);
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
