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
 *      System specific interface stuff.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __I_VIDEO__
#define __I_VIDEO__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef GL_DOOM
#include <SDL_opengl.h>
#endif

#include "doomtype.h"
#include "v_video.h"
#include "SDL.h"

#ifdef __GNUG__
#pragma interface
#endif

#include "RT/rt_common.h"

extern int render_vsync;
extern int render_screen_multiply;
extern int integer_scaling;

typedef struct
{
  // NOTE: dlss, fsr, renderscale - are mutually exclusive
  int dlss, fsr;              // 0 - off, [1,4] - quality from highest to lowest
  int renderscale;            // index in rt_settings_renderscale_e
  int bloom_intensity;        // index in [0,Reduced,Default,Exaggerated]
  int muzzleflash_intensity;  // index in [0,Reduced,Default]
  int statusbar_scale;        // to calculate procents: 10*(statusbar_scale+1)
#if RT_SEPARATE_HUD_SCALE
  int hud_scale;              // to calculate procents: 10*(hud_scale+1)
#endif
  int hud_style;
  int refl_refr_max_depth;
  int classic_flashlight;
  int bounce_quality;         // 0 - one bounce, 1 - two bounces for poly lights and flashlight, 2 - two bounces 
} rt_settings_t;
typedef enum
{
  RT_SETTINGS_RENDERSCALE_DEFAULT, // 100%
  RT_SETTINGS_RENDERSCALE_320x200,
  RT_SETTINGS_RENDERSCALE_480,
  RT_SETTINGS_RENDERSCALE_600,
  RT_SETTINGS_RENDERSCALE_720,
  RT_SETTINGS_RENDERSCALE_900,
  RT_SETTINGS_RENDERSCALE_1080,
  RT_SETTINGS_RENDERSCALE_1200,
  RT_SETTINGS_RENDERSCALE_1440,
  RT_SETTINGS_RENDERSCALE_1600,
  RT_SETTINGS_RENDERSCALE_1920,
  RT_SETTINGS_RENDERSCALE_2160,
  RT_SETTINGS_RENDERSCALE_NUM,
} rt_settings_renderscale_e;
extern rt_settings_t rt_settings;

extern SDL_Window *sdl_window;
extern SDL_Renderer *sdl_renderer;

extern const char *screen_resolutions_list[];
extern const char *screen_resolution;

extern const char *sdl_video_window_pos;

void I_PreInitGraphics(void); /* CPhipps - do stuff immediately on start */
void I_InitScreenResolution(void); /* init resolution */
void I_SetWindowCaption(void); /* Set the window caption */
void I_SetWindowIcon(void); /* Set the application icon */
void I_InitGraphics (void);
void I_UpdateVideoMode(void);
void I_ShutdownGraphics(void);

/* Takes full 8 bit values. */
void I_SetPalette(int pal); /* CPhipps - pass down palette number */

void I_UpdateNoBlit (void);
void I_FinishUpdate (void);

int I_ScreenShot (const char *fname);
// NSM expose lower level screen data grab for vidcap
unsigned char *I_GrabScreen (void);

/* I_StartTic
 * Called by D_DoomLoop,
 * called before processing each tic in a frame.
 * Quick syncronous operations are performed here.
 * Can call D_PostEvent.
 */
void I_StartTic (void);

/* I_StartFrame
 * Called by D_DoomLoop,
 * called before processing any tics in a frame
 * (just after displaying a frame).
 * Time consuming syncronous operations
 * are performed here (joystick reading).
 * Can call D_PostEvent.
 */

void I_StartFrame (void);

extern int use_fullscreen;  /* proff 21/05/2000 */
extern int desired_fullscreen; //e6y
extern int exclusive_fullscreen;

void I_UpdateRenderSize(void);	// Handle potential
extern int renderW;		// resolution scaling
extern int renderH;		// - DTIED

// Set the process affinity mask so that all threads
extern int process_affinity_mask;
// Priority class for the prboom-plus process
extern int process_priority;

extern dboolean window_focused;
void UpdateGrab(void);

#endif
