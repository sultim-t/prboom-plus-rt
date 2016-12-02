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
 *  Main loop menu stuff.
 *  Default Config File.
 *  PCX Screenshots.
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _MSC_VER
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>

#include "doomstat.h"
#include "m_argv.h"
#include "g_game.h"
#include "m_menu.h"
#include "am_map.h"
#include "w_wad.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "dstrings.h"
#include "m_misc.h"
#include "s_sound.h"
#include "sounds.h"
#include "i_joy.h"
#include "lprintf.h"
#include "d_main.h"
#include "d_deh.h"
#include "r_draw.h"
#include "r_demo.h"
#include "r_fps.h"
#include "r_main.h"
#include "r_things.h"
#include "r_sky.h"

//e6y
#ifdef GL_DOOM
#include "gl_struct.h"
#endif
#include "g_overflow.h"
#include "e6y.h"
#ifdef USE_WINDOWS_LAUNCHER
#include "e6y_launcher.h"
#endif

// NSM
#include "i_capture.h"

/* cph - disk icon not implemented */
static inline void I_BeginRead(void) {}
static inline void I_EndRead(void) {}

/*
 * M_WriteFile
 *
 * killough 9/98: rewritten to use stdio and to flash disk icon
 */

dboolean M_WriteFile(char const *name, const void *source, size_t length)
{
  FILE *fp;

  errno = 0;

  if (!(fp = fopen(name, "wb")))       // Try opening file
    return 0;                          // Could not open file for writing

  I_BeginRead();                       // Disk icon on
  length = fwrite(source, 1, length, fp) == (size_t)length;   // Write data
  fclose(fp);
  I_EndRead();                         // Disk icon off

  if (!length)                         // Remove partially written file
    remove(name);

  return length;
}

/*
 * M_ReadFile
 *
 * killough 9/98: rewritten to use stdio and to flash disk icon
 */

int M_ReadFile(char const *name, byte **buffer)
{
  FILE *fp;

  if ((fp = fopen(name, "rb")))
    {
      size_t length;

      I_BeginRead();
      fseek(fp, 0, SEEK_END);
      length = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      *buffer = Z_Malloc(length, PU_STATIC, 0);
      if (fread(*buffer, 1, length, fp) == length)
        {
          fclose(fp);
          I_EndRead();
          return length;
        }
      fclose(fp);
    }

  /* cph 2002/08/10 - this used to return 0 on error, but that's ambiguous,
   * because we could have a legit 0-length file. So make it -1. */
  return -1;
}

//
// DEFAULTS
//

int usemouse;
dboolean    precache = true; /* if true, load all graphics at start */

// The available anisotropic
typedef enum {
  gl_anisotropic_off = 0,
  gl_anisotropic_2x  = 1,
  gl_anisotropic_4x  = 2,
  gl_anisotropic_8x  = 3,
  gl_anisotropic_16x = 4,
} gl_anisotropic_mode_t;

extern int viewwidth;
extern int viewheight;
#ifdef GL_DOOM
extern int gl_nearclip;
extern int gl_colorbuffer_bits;
extern int gl_depthbuffer_bits;
extern int gl_texture_filter;
extern int gl_sprite_filter;
extern int gl_patch_filter;
extern int gl_texture_filter_anisotropic;
extern const char *gl_tex_format_string;
extern int gl_sky_detail;
extern int gl_use_paletted_texture;
extern int gl_use_shared_texture_palette;

//e6y: all OpenGL extentions will be disabled with TRUE
extern int gl_compatibility;

//cfg values
extern int gl_ext_texture_filter_anisotropic_default;
extern int gl_arb_texture_non_power_of_two_default;
extern int gl_arb_multitexture_default;
extern int gl_arb_texture_compression_default;
extern int gl_ext_framebuffer_object_default;
extern int gl_ext_packed_depth_stencil_default;
extern int gl_ext_blend_color_default;
extern int gl_use_stencil_default;
extern int gl_ext_arb_vertex_buffer_object_default;
extern int gl_arb_pixel_buffer_object_default;
extern int gl_arb_shader_objects_default;

//e6y: motion bloor
extern int gl_motionblur;

//e6y: fog
extern int gl_fog;
extern int gl_fog_color;

extern int gl_finish;
extern int gl_clear;
extern int gl_ztrick;

#endif

extern int realtic_clock_rate;         // killough 4/13/98: adjustable timer
extern int tran_filter_pct;            // killough 2/21/98

extern int screenblocks;
extern int showMessages;

#ifndef DJGPP
int         mus_pause_opt; // 0 = kill music, 1 = pause, 2 = continue
#endif

extern const char* chat_macros[];

extern int endoom_mode;

extern const char* S_music_files[]; // cournia

/* cph - Some MBF stuff parked here for now
 * killough 10/98
 */
int map_point_coordinates;
int map_level_stat;

default_t defaults[] =
{
  //e6y
  {"System settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"process_priority", {&process_priority},{0},0,2,def_int,ss_none},
  
  {"Misc settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"default_compatibility_level",{(int*)&default_compatibility_level},
   {-1},-1,MAX_COMPATIBILITY_LEVEL-1,
   def_int,ss_none}, // compatibility level" - CPhipps
  {"realtic_clock_rate",{&realtic_clock_rate},{100},0,UL,
   def_int,ss_none}, // percentage of normal speed (35 fps) realtic clock runs at
  {"menu_background", {(int*)&menu_background}, {1}, 0, 1,
   def_bool,ss_none}, // do Boom fullscreen menus have backgrounds?
  {"max_player_corpse", {&bodyquesize}, {32},-1,UL,   // killough 2/8/98
   def_int,ss_none}, // number of dead bodies in view supported (-1 = no limit)
  {"flashing_hom",{&flashing_hom},{0},0,1,
   def_bool,ss_none}, // killough 10/98 - enable flashing HOM indicator
  {"demo_insurance",{&default_demo_insurance},{2},0,2,  // killough 3/31/98
   def_int,ss_none}, // 1=take special steps ensuring demo sync, 2=only during recordings
  {"endoom_mode", {&endoom_mode},{5},0,7, // CPhipps - endoom flags
   def_hex, ss_none}, // 0, +1 for colours, +2 for non-ascii chars, +4 for skip-last-line
  {"level_precache",{(int*)&precache},{1},0,1,
   def_bool,ss_none}, // precache level data?
  {"demo_smoothturns", {&demo_smoothturns},  {0},0,1,
   def_bool,ss_stat},
  {"demo_smoothturnsfactor", {&demo_smoothturnsfactor},  {6},1,SMOOTH_PLAYING_MAXFACTOR,
   def_int,ss_stat},
   
  {"Files",{NULL},{0},UL,UL,def_none,ss_none},
  /* cph - MBF-like wad/deh/bex autoload code */
  {"wadfile_1",{NULL,&wad_files[1]},{0,""},UL,UL,def_str,ss_none},
  {"wadfile_2",{NULL,&wad_files[2]},{0,""},UL,UL,def_str,ss_none},
  {"dehfile_1",{NULL,&deh_files[0]},{0,""},UL,UL,def_str,ss_none},
  {"dehfile_2",{NULL,&deh_files[1]},{0,""},UL,UL,def_str,ss_none},

  {"Game settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"default_skill",{&defaultskill},{3},1,5, // jff 3/24/98 allow default skill setting
   def_int,ss_none}, // selects default skill 1=TYTD 2=NTR 3=HMP 4=UV 5=NM
  {"weapon_recoil",{&default_weapon_recoil},{0},0,1,
   def_bool,ss_weap, &weapon_recoil},
  /* killough 10/98 - toggle between SG/SSG and Fist/Chainsaw */
  {"doom_weapon_toggles",{&doom_weapon_toggles}, {1}, 0, 1,
   def_bool, ss_weap },
  {"player_bobbing",{&default_player_bobbing},{1},0,1,         // phares 2/25/98
   def_bool,ss_weap, &player_bobbing},
  {"monsters_remember",{&default_monsters_remember},{1},0,1,   // killough 3/1/98
   def_bool,ss_enem, &monsters_remember},
   /* MBF AI enhancement options */
  {"monster_infighting",{&default_monster_infighting}, {1}, 0, 1,
   def_bool, ss_enem, &monster_infighting},
  {"monster_backing",{&default_monster_backing}, {0}, 0, 1,
   def_bool, ss_enem, &monster_backing},
  {"monster_avoid_hazards",{&default_monster_avoid_hazards}, {1}, 0, 1,
   def_bool, ss_enem, &monster_avoid_hazards},
  {"monkeys",{&default_monkeys}, {0}, 0, 1,
   def_bool, ss_enem, &monkeys},
  {"monster_friction",{&default_monster_friction}, {1}, 0, 1,
   def_bool, ss_enem, &monster_friction},
  {"help_friends",{&default_help_friends}, {0}, 0, 1,
   def_bool, ss_enem, &help_friends},
  {"allow_pushers",{&default_allow_pushers},{1},0,1,
   def_bool,ss_weap, &allow_pushers},
  {"variable_friction",{&default_variable_friction},{1},0,1,
   def_bool,ss_weap, &variable_friction},
#ifdef DOGS
  {"player_helpers",{&default_dogs}, {0}, 0, 3,
   def_bool, ss_enem },
  {"friend_distance",{&default_distfriend}, {128}, 0, 999,
   def_int, ss_enem, &distfriend},
  {"dog_jumping",{&default_dog_jumping}, {1}, 0, 1,
   def_bool, ss_enem, &dog_jumping},
#endif
   /* End of MBF AI extras */

  {"sts_always_red",{&sts_always_red},{1},0,1, // no color changes on status bar
   def_bool,ss_stat},
  {"sts_pct_always_gray",{&sts_pct_always_gray},{0},0,1, // 2/23/98 chg default
   def_bool,ss_stat}, // makes percent signs on status bar always gray
  {"sts_traditional_keys",{&sts_traditional_keys},{0},0,1,  // killough 2/28/98
   def_bool,ss_stat}, // disables doubled card and skull key display on status bar
  {"show_messages",{&showMessages},{1},0,1,
   def_bool,ss_none}, // enables message display
  {"autorun",{&autorun},{1},0,1,  // killough 3/6/98: preserve autorun across games
   def_bool,ss_none},

  {"Dehacked settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"deh_apply_cheats",{&deh_apply_cheats},{1},0,1,
   def_bool,ss_stat}, // if 0, dehacked cheat replacements are ignored.

  {"Compatibility settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"comp_zombie",{&default_comp[comp_zombie]},{1},0,1,def_bool,ss_comp,&comp[comp_zombie]},
  {"comp_infcheat",{&default_comp[comp_infcheat]},{0},0,1,def_bool,ss_comp,&comp[comp_infcheat]},
  {"comp_stairs",{&default_comp[comp_stairs]},{0},0,1,def_bool,ss_comp,&comp[comp_stairs]},
  {"comp_telefrag",{&default_comp[comp_telefrag]},{0},0,1,def_bool,ss_comp,&comp[comp_telefrag]},
  {"comp_dropoff",{&default_comp[comp_dropoff]},{0},0,1,def_bool,ss_comp,&comp[comp_dropoff]},
  {"comp_falloff",{&default_comp[comp_falloff]},{0},0,1,def_bool,ss_comp,&comp[comp_falloff]},
  {"comp_staylift",{&default_comp[comp_staylift]},{0},0,1,def_bool,ss_comp,&comp[comp_staylift]},
  {"comp_doorstuck",{&default_comp[comp_doorstuck]},{0},0,1,def_bool,ss_comp,&comp[comp_doorstuck]},
  {"comp_pursuit",{&default_comp[comp_pursuit]},{0},0,1,def_bool,ss_comp,&comp[comp_pursuit]},
  {"comp_vile",{&default_comp[comp_vile]},{0},0,1,def_bool,ss_comp,&comp[comp_vile]},
  {"comp_pain",{&default_comp[comp_pain]},{0},0,1,def_bool,ss_comp,&comp[comp_pain]},
  {"comp_skull",{&default_comp[comp_skull]},{0},0,1,def_bool,ss_comp,&comp[comp_skull]},
  {"comp_blazing",{&default_comp[comp_blazing]},{0},0,1,def_bool,ss_comp,&comp[comp_blazing]},
  {"comp_doorlight",{&default_comp[comp_doorlight]},{0},0,1,def_bool,ss_comp,&comp[comp_doorlight]},
  {"comp_god",{&default_comp[comp_god]},{0},0,1,def_bool,ss_comp,&comp[comp_god]},
  {"comp_skymap",{&default_comp[comp_skymap]},{0},0,1,def_bool,ss_comp,&comp[comp_skymap]},
  {"comp_floors",{&default_comp[comp_floors]},{0},0,1,def_bool,ss_comp,&comp[comp_floors]},
  {"comp_model",{&default_comp[comp_model]},{0},0,1,def_bool,ss_comp,&comp[comp_model]},
  {"comp_zerotags",{&default_comp[comp_zerotags]},{0},0,1,def_bool,ss_comp,&comp[comp_zerotags]},
  {"comp_moveblock",{&default_comp[comp_moveblock]},{0},0,1,def_bool,ss_comp,&comp[comp_moveblock]},
  {"comp_sound",{&default_comp[comp_sound]},{0},0,1,def_bool,ss_comp,&comp[comp_sound]},
  {"comp_666",{&default_comp[comp_666]},{0},0,1,def_bool,ss_comp,&comp[comp_666]},
  {"comp_soul",{&default_comp[comp_soul]},{0},0,1,def_bool,ss_comp,&comp[comp_soul]},
  {"comp_maskedanim",{&default_comp[comp_maskedanim]},{0},0,1,def_bool,ss_comp,&comp[comp_maskedanim]},
  //e6y
  {"PrBoom-plus compatibility settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"comp_ouchface",{&default_comp[comp_ouchface]},{0},0,1,def_bool,ss_comp,&comp[comp_ouchface]},
  {"comp_maxhealth",{&default_comp[comp_maxhealth]},{0},0,1,def_bool,ss_comp,&comp[comp_maxhealth]},
  {"comp_translucency",{&default_comp[comp_translucency]},{0},0,1,def_bool,ss_comp,&comp[comp_translucency]},

  {"Sound settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"snd_pcspeaker",{&snd_pcspeaker},{0}, 0, 1, def_bool,ss_none},
  {"sound_card",{&snd_card},{-1},-1,7,       // jff 1/18/98 allow Allegro drivers
   def_int,ss_none}, // select sounds driver (DOS), -1 is autodetect, 0 is none; in Linux, non-zero enables sound
  {"music_card",{&mus_card},{-1},-1,9,       //  to be set,  -1 = autodetect
   def_int,ss_none}, // select music driver (DOS), -1 is autodetect, 0 is none"; in Linux, non-zero enables music
  {"pitched_sounds",{&pitched_sounds},{0},0,1, // killough 2/21/98
   def_bool,ss_none}, // enables variable pitch in sound effects (from id's original code)
  {"samplerate",{&snd_samplerate},{22050},11025,48000, def_int,ss_none},
  {"sfx_volume",{&snd_SfxVolume},{8},0,15, def_int,ss_none},
  {"music_volume",{&snd_MusicVolume},{8},0,15, def_int,ss_none},
  {"mus_pause_opt",{&mus_pause_opt},{1},0,2, // CPhipps - music pausing
   def_int, ss_none}, // 0 = kill music when paused, 1 = pause music, 2 = let music continue
  {"snd_channels",{&default_numChannels},{32},1,32,
   def_int,ss_none}, // number of audio events simultaneously // killough
#ifdef _WIN32
  {"snd_midiplayer",{NULL, &snd_midiplayer},{0,"fluidsynth"},UL,UL,def_str,ss_none},
#else
  {"snd_midiplayer",{NULL, &snd_midiplayer},{0,"sdl"},UL,UL,def_str,ss_none},
#endif
  {"snd_soundfont",{NULL, &snd_soundfont},{0,"TimGM6mb.sf2"},UL,UL,def_str,ss_none}, // soundfont name for synths that support it
  {"snd_mididev",{NULL, &snd_mididev},{0,""},UL,UL,def_str,ss_none}, // midi device to use for portmidiplayer

#ifdef _WIN32
  {"mus_extend_volume",{&mus_extend_volume},{0},0,1,
   def_bool,ss_none}, // e6y: apply midi volume to all midi devices
#endif
  {"mus_fluidsynth_chorus",{&mus_fluidsynth_chorus},{1},0,1,def_bool,ss_none},
  {"mus_fluidsynth_reverb",{&mus_fluidsynth_reverb},{1},0,1,def_bool,ss_none},
  {"mus_fluidsynth_gain",{&mus_fluidsynth_gain},{50},0,1000,def_int,ss_none}, // NSM  fine tune fluidsynth output level
  {"mus_opl_gain",{&mus_opl_gain},{50},0,1000,def_int,ss_none}, // NSM  fine tune opl output level

  {"Video settings",{NULL},{0},UL,UL,def_none,ss_none},
#ifdef GL_DOOM
  #ifdef _MSC_VER
    {"videomode",{NULL, &default_videomode},{0,"OpenGL"},UL,UL,def_str,ss_none},
  #else
    {"videomode",{NULL, &default_videomode},{0,"8"},UL,UL,def_str,ss_none},
  #endif
#else
  {"videomode",{NULL, &default_videomode},{0,"8"},UL,UL,def_str,ss_none},
#endif
  /* 640x480 default resolution */
  {"screen_resolution",{NULL, &screen_resolution},{0,"640x480"},UL,UL,def_str,ss_none},
  {"use_fullscreen",{&use_fullscreen},{0},0,1, /* proff 21/05/2000 */
   def_bool,ss_none},
  {"render_vsync",{&render_vsync},{1},0,1,
   def_bool,ss_none},
  {"translucency",{&default_translucency},{1},0,1,   // phares
   def_bool,ss_none}, // enables translucency
  {"tran_filter_pct",{&tran_filter_pct},{66},0,100,         // killough 2/21/98
   def_int,ss_none}, // set percentage of foreground/background translucency mix
  {"screenblocks",{&screenblocks},{10},3,11,  // killough 2/21/98: default to 10
   def_int,ss_none},
  {"usegamma",{&usegamma},{3},0,4, //jff 3/6/98 fix erroneous upper limit in range
   def_int,ss_none}, // gamma correction level // killough 1/18/98
  {"uncapped_framerate", {&movement_smooth_default},  {1},0,1,
   def_bool,ss_stat},
  {"test_interpolation_method", {&interpolation_method},  {0},0,1,
   def_int,ss_stat},
  {"filter_wall",{(int*)&drawvars.filterwall},{RDRAW_FILTER_POINT},
   RDRAW_FILTER_POINT, RDRAW_FILTER_ROUNDED, def_int,ss_none},
  {"filter_floor",{(int*)&drawvars.filterfloor},{RDRAW_FILTER_POINT},
   RDRAW_FILTER_POINT, RDRAW_FILTER_ROUNDED, def_int,ss_none},
  {"filter_sprite",{(int*)&drawvars.filtersprite},{RDRAW_FILTER_POINT},
   RDRAW_FILTER_POINT, RDRAW_FILTER_ROUNDED, def_int,ss_none},
  {"filter_z",{(int*)&drawvars.filterz},{RDRAW_FILTER_POINT},
   RDRAW_FILTER_POINT, RDRAW_FILTER_LINEAR, def_int,ss_none},
  {"filter_patch",{(int*)&drawvars.filterpatch},{RDRAW_FILTER_POINT},
   RDRAW_FILTER_POINT, RDRAW_FILTER_ROUNDED, def_int,ss_none},
  {"filter_threshold",{(int*)&drawvars.mag_threshold},{49152},
   0, UL, def_int,ss_none},
  {"sprite_edges",{(int*)&drawvars.sprite_edges},{RDRAW_MASKEDCOLUMNEDGE_SQUARE},
   RDRAW_MASKEDCOLUMNEDGE_SQUARE, RDRAW_MASKEDCOLUMNEDGE_SLOPED, def_int,ss_none},
  {"patch_edges",{(int*)&drawvars.patch_edges},{RDRAW_MASKEDCOLUMNEDGE_SQUARE},
   RDRAW_MASKEDCOLUMNEDGE_SQUARE, RDRAW_MASKEDCOLUMNEDGE_SLOPED, def_int,ss_none},

#ifdef GL_DOOM
  {"OpenGL settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"gl_compatibility", {&gl_compatibility},  {0},0,1,
   def_bool,ss_stat},

  {"gl_arb_multitexture", {&gl_arb_multitexture_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_arb_texture_compression", {&gl_arb_texture_compression_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_arb_texture_non_power_of_two", {&gl_arb_texture_non_power_of_two_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_ext_arb_vertex_buffer_object", {&gl_ext_arb_vertex_buffer_object_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_arb_pixel_buffer_object", {&gl_arb_pixel_buffer_object_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_arb_shader_objects", {&gl_arb_shader_objects_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_ext_blend_color", {&gl_ext_blend_color_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_ext_framebuffer_object", {&gl_ext_framebuffer_object_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_ext_packed_depth_stencil", {&gl_ext_packed_depth_stencil_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_ext_texture_filter_anisotropic", {&gl_ext_texture_filter_anisotropic_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_use_stencil", {&gl_use_stencil_default}, {1},0,1,
   def_bool,ss_stat},
  {"gl_use_display_lists",{&gl_use_display_lists},{0},0,1,
   def_bool,ss_none},

  {"gl_finish",{&gl_finish},{1},0,1,
   def_bool,ss_none},
  {"gl_clear",{&gl_clear},{0},0,1,
   def_bool,ss_none},
  {"gl_ztrick",{&gl_ztrick},{0},0,1,
   def_bool,ss_none},
  {"gl_nearclip",{&gl_nearclip},{5},0,UL,
   def_int,ss_none}, /* near clipping plane pos */
  {"gl_colorbuffer_bits",{&gl_colorbuffer_bits},{32},16,32,
   def_int,ss_none},
  {"gl_depthbuffer_bits",{&gl_depthbuffer_bits},{24},16,32,
   def_int,ss_none},
  {"gl_texture_filter",{(int*)&gl_texture_filter},
   {filter_nearest_mipmap_linear}, filter_nearest, filter_count - 1, def_int,ss_none},
  {"gl_sprite_filter",{(int*)&gl_sprite_filter},
   {filter_nearest}, filter_nearest, filter_linear_mipmap_nearest, def_int,ss_none},
  {"gl_patch_filter",{(int*)&gl_patch_filter},
   {filter_nearest}, filter_nearest, filter_linear, def_int,ss_none},
  {"gl_texture_filter_anisotropic",{(int*)&gl_texture_filter_anisotropic},
   {gl_anisotropic_8x}, gl_anisotropic_off, gl_anisotropic_16x, def_int,ss_none},
  {"gl_tex_format_string", {NULL,&gl_tex_format_string}, {0,"GL_RGBA"},UL,UL,
   def_str,ss_none},
  {"gl_sprite_offset",{&gl_sprite_offset_default},{0}, 0, 5,
   def_int,ss_none}, // amount to bring items out of floor (GL) Mead 8/13/03
  {"gl_sprite_blend",{&gl_sprite_blend},{0},0,1,
   def_bool,ss_none},
  {"gl_mask_sprite_threshold",{&gl_mask_sprite_threshold},{50},0,100,
   def_int,ss_none},
  {"gl_skymode",{(int*)&gl_skymode},
  {skytype_auto}, skytype_auto, skytype_count - 1, def_int,ss_none},
  {"gl_sky_detail",{&gl_sky_detail},{16},1,32,
   def_int,ss_none},
  {"gl_use_paletted_texture",{&gl_use_paletted_texture},{0},0,1,
   def_bool,ss_none},
  {"gl_use_shared_texture_palette",{&gl_use_shared_texture_palette},{0},0,1,
   def_bool,ss_none},
#endif

  {"Mouse settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"use_mouse",{&usemouse},{1},0,1,
   def_bool,ss_none}, // enables use of mouse with DOOM
  //jff 4/3/98 allow unlimited sensitivity
  {"mouse_sensitivity_horiz",{&mouseSensitivity_horiz},{10},0,UL,
   def_int,ss_none}, /* adjust horizontal (x) mouse sensitivity killough/mead */
  //jff 4/3/98 allow unlimited sensitivity
  {"mouse_sensitivity_vert",{&mouseSensitivity_vert},{10},0,UL,
   def_int,ss_none}, /* adjust vertical (y) mouse sensitivity killough/mead */
  //jff 3/8/98 allow -1 in mouse bindings to disable mouse function
  {"mouseb_fire",{&mousebfire},{0},-1,MAX_MOUSEB,
   def_int,ss_keys}, // mouse button number to use for fire
  {"mouseb_strafe",{&mousebstrafe},{1},-1,MAX_MOUSEB,
   def_int,ss_keys}, // mouse button number to use for strafing
  {"mouseb_forward",{&mousebforward},{2},-1,MAX_MOUSEB,
   def_int,ss_keys}, // mouse button number to use for forward motion
  {"mouseb_backward",{&mousebbackward},{-1},-1,MAX_MOUSEB,
   def_int,ss_keys}, // mouse button number to use for backward motion
  {"mouseb_use", {&mousebuse},{-1},-1,MAX_MOUSEB,
   def_int,ss_keys}, // mouse button number to use for using doors/switches
  //jff 3/8/98 end of lower range change for -1 allowed in mouse binding

// For key bindings, the values stored in the key_* variables       // phares
// are the internal Doom Codes. The values stored in the default.cfg
// file are the keyboard codes.
// CPhipps - now they're the doom codes, so default.cfg can be portable

  {"Key bindings",{NULL},{0},UL,UL,def_none,ss_none},
  {"key_right",       {&key_right},          {KEYD_RIGHTARROW},
   0,MAX_KEY,def_key,ss_keys}, // key to turn right
  {"key_left",        {&key_left},           {KEYD_LEFTARROW} ,
   0,MAX_KEY,def_key,ss_keys}, // key to turn left
  {"key_up",          {&key_up},             {'w'}   ,
   0,MAX_KEY,def_key,ss_keys}, // key to move forward
  {"key_down",        {&key_down},           {'s'},
   0,MAX_KEY,def_key,ss_keys}, // key to move backward
  {"key_mlook",       {&key_mlook},           {'\\'},
   0,MAX_KEY,def_key,ss_keys}, // key to move backward
  {"key_menu_right",  {&key_menu_right},     {KEYD_RIGHTARROW},// phares 3/7/98
   0,MAX_KEY,def_key,ss_keys}, // key to move right in a menu  //     |
  {"key_menu_left",   {&key_menu_left},      {KEYD_LEFTARROW} ,//     V
   0,MAX_KEY,def_key,ss_keys}, // key to move left in a menu
  {"key_menu_up",     {&key_menu_up},        {KEYD_UPARROW}   ,
   0,MAX_KEY,def_key,ss_keys}, // key to move up in a menu
  {"key_menu_down",   {&key_menu_down},      {KEYD_DOWNARROW} ,
   0,MAX_KEY,def_key,ss_keys}, // key to move down in a menu
  {"key_menu_backspace",{&key_menu_backspace},{KEYD_BACKSPACE} ,
   0,MAX_KEY,def_key,ss_keys}, // delete key in a menu
  {"key_menu_escape", {&key_menu_escape},    {KEYD_ESCAPE}    ,
   0,MAX_KEY,def_key,ss_keys}, // key to leave a menu      ,   // phares 3/7/98
  {"key_menu_enter",  {&key_menu_enter},     {KEYD_ENTER}     ,
   0,MAX_KEY,def_key,ss_keys}, // key to select from menu
  {"key_menu_clear",  {&key_menu_clear},     {KEYD_DEL}     ,
   0,MAX_KEY,def_key,ss_keys}, // key to clear a key binding
  {"key_setup",       {&key_setup},          {0},
   0,MAX_KEY,def_key,ss_keys}, //e6y: key for entering setup menu
  {"key_strafeleft",  {&key_strafeleft},     {'a'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to strafe left
  {"key_straferight", {&key_straferight},    {'d'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to fly up
  {"key_flyup",  {&key_flyup}, {'.'},
   0,MAX_KEY,def_key,ss_keys}, // key to fly down
  {"key_flydown", {&key_flydown}, {','},
   0,MAX_KEY,def_key,ss_keys}, // key to strafe right

  {"key_fire",        {&key_fire},           {KEYD_RCTRL}     ,
   0,MAX_KEY,def_key,ss_keys}, // duh
  {"key_use",         {&key_use},            {' '}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to open a door, use a switch
  {"key_strafe",      {&key_strafe},         {KEYD_RALT}      ,
   0,MAX_KEY,def_key,ss_keys}, // key to use with arrows to strafe
  {"key_speed",       {&key_speed},          {KEYD_RSHIFT}    ,
   0,MAX_KEY,def_key,ss_keys}, // key to run

  {"key_savegame",    {&key_savegame},       {KEYD_F2}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to save current game
  {"key_loadgame",    {&key_loadgame},       {KEYD_F3}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to restore from saved games
  {"key_soundvolume", {&key_soundvolume},    {KEYD_F4}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to bring up sound controls
  {"key_hud",         {&key_hud},            {KEYD_F5}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to adjust HUD
  {"key_quicksave",   {&key_quicksave},      {KEYD_F6}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to to quicksave
  {"key_endgame",     {&key_endgame},        {KEYD_F7}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to end the game
  {"key_messages",    {&key_messages},       {KEYD_F8}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle message enable
  {"key_quickload",   {&key_quickload},      {KEYD_F9}        ,
   0,MAX_KEY,def_key,ss_keys}, // key to load from quicksave
  {"key_quit",        {&key_quit},           {KEYD_F10}       ,
   0,MAX_KEY,def_key,ss_keys}, // key to quit game
  {"key_gamma",       {&key_gamma},          {KEYD_F11}       ,
   0,MAX_KEY,def_key,ss_keys}, // key to adjust gamma correction
  {"key_spy",         {&key_spy},            {KEYD_F12}       ,
   0,MAX_KEY,def_key,ss_keys}, // key to view from another coop player's view
  {"key_pause",       {&key_pause},          {KEYD_PAUSE}     ,
   0,MAX_KEY,def_key,ss_keys}, // key to pause the game
  {"key_autorun",     {&key_autorun},        {KEYD_CAPSLOCK}  ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle always run mode
  {"key_chat",        {&key_chat},           {'t'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to enter a chat message
  {"key_backspace",   {&key_backspace},      {KEYD_BACKSPACE} ,
   0,MAX_KEY,def_key,ss_keys}, // backspace key
  {"key_enter",       {&key_enter},          {KEYD_ENTER}     ,
   0,MAX_KEY,def_key,ss_keys}, // key to select from menu or see last message
  {"key_map",         {&key_map},            {KEYD_TAB}       ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle automap display
  {"key_map_right",   {&key_map_right},      {KEYD_RIGHTARROW},// phares 3/7/98
   0,MAX_KEY,def_key,ss_keys}, // key to shift automap right   //     |
  {"key_map_left",    {&key_map_left},       {KEYD_LEFTARROW} ,//     V
   0,MAX_KEY,def_key,ss_keys}, // key to shift automap left
  {"key_map_up",      {&key_map_up},         {KEYD_UPARROW}   ,
   0,MAX_KEY,def_key,ss_keys}, // key to shift automap up
  {"key_map_down",    {&key_map_down},       {KEYD_DOWNARROW} ,
   0,MAX_KEY,def_key,ss_keys}, // key to shift automap down
  {"key_map_zoomin",  {&key_map_zoomin},      {'='}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to enlarge automap
  {"key_map_zoomout", {&key_map_zoomout},     {'-'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to reduce automap
  {"key_map_gobig",   {&key_map_gobig},       {'0'}           ,
   0,MAX_KEY,def_key,ss_keys},  // key to get max zoom for automap
  {"key_map_follow",  {&key_map_follow},      {'f'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle follow mode
  {"key_map_mark",    {&key_map_mark},        {'m'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to drop a marker on automap
  {"key_map_clear",   {&key_map_clear},       {'c'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to clear all markers on automap
  {"key_map_grid",    {&key_map_grid},        {'g'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle grid display over automap
  {"key_map_rotate",  {&key_map_rotate},      {'r'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle rotating the automap to match the player's orientation
  {"key_map_overlay", {&key_map_overlay},     {'o'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle overlaying the automap on the rendered display
#ifdef GL_DOOM
  {"key_map_textured", {&key_map_textured},   {0}             ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle textured automap
#endif
  {"key_reverse",     {&key_reverse},         {'/'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to spin 180 instantly
  {"key_zoomin",      {&key_zoomin},          {'='}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to enlarge display
  {"key_zoomout",     {&key_zoomout},         {'-'}           ,
   0,MAX_KEY,def_key,ss_keys}, // key to reduce display
  {"key_chatplayer1", {&destination_keys[0]}, {'g'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to chat with player 1
  // killough 11/98: fix 'i'/'b' reversal
  {"key_chatplayer2", {&destination_keys[1]}, {'i'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to chat with player 2
  {"key_chatplayer3", {&destination_keys[2]}, {'b'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to chat with player 3
  {"key_chatplayer4", {&destination_keys[3]}, {'r'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to chat with player 4
  {"key_weapontoggle",{&key_weapontoggle},    {'0'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to toggle between two most preferred weapons with ammo
  {"key_weapon1",     {&key_weapon1},         {'1'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 1 (fist/chainsaw)
  {"key_weapon2",     {&key_weapon2},         {'2'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 2 (pistol)
  {"key_weapon3",     {&key_weapon3},         {'3'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 3 (supershotgun/shotgun)
  {"key_weapon4",     {&key_weapon4},         {'4'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 4 (chaingun)
  {"key_weapon5",     {&key_weapon5},         {'5'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 5 (rocket launcher)
  {"key_weapon6",     {&key_weapon6},         {'6'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 6 (plasma rifle)
  {"key_weapon7",     {&key_weapon7},         {'7'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 7 (bfg9000)         //    ^
  {"key_weapon8",     {&key_weapon8},         {'8'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 8 (chainsaw)        //    |
  {"key_weapon9",     {&key_weapon9},         {'9'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to switch to weapon 9 (supershotgun)    // phares
  {"key_nextweapon",  {&key_nextweapon},      {KEYD_MWHEELUP}  ,
   0,MAX_KEY,def_key,ss_keys}, // key to cycle to the next weapon
  {"key_prevweapon",  {&key_prevweapon},      {KEYD_MWHEELDOWN},
   0,MAX_KEY,def_key,ss_keys}, // key to cycle to the previous weapon

  // killough 2/22/98: screenshot key
  {"key_screenshot",  {&key_screenshot},      {'*'}            ,
   0,MAX_KEY,def_key,ss_keys}, // key to take a screenshot

  {"Joystick settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"use_joystick",{&usejoystick},{0},0,2,
   def_int,ss_none}, // number of joystick to use (0 for none)
  {"joy_left",{&joyleft},{0},  UL,UL,def_int,ss_none},
  {"joy_right",{&joyright},{0},UL,UL,def_int,ss_none},
  {"joy_up",  {&joyup},  {0},  UL,UL,def_int,ss_none},
  {"joy_down",{&joydown},{0},  UL,UL,def_int,ss_none},
  {"joyb_fire",{&joybfire},{0},0,UL,
   def_int,ss_keys}, // joystick button number to use for fire
  {"joyb_strafe",{&joybstrafe},{1},0,UL,
   def_int,ss_keys}, // joystick button number to use for strafing
  {"joyb_strafeleft",{&joybstrafeleft},{4},0,UL,
   def_int,ss_keys}, // joystick button number to use for strafe left
  {"joyb_straferight",{&joybstraferight},{5},0,UL,
   def_int,ss_keys}, // joystick button number to use for strafe right
  {"joyb_speed",{&joybspeed},{2},0,UL,
   def_int,ss_keys}, // joystick button number to use for running
  {"joyb_use",{&joybuse},{3},0,UL,
   def_int,ss_keys}, // joystick button number to use for use/open

  {"Chat macros",{NULL},{0},UL,UL,def_none,ss_none},
  {"chatmacro0", {0,&chat_macros[0]}, {0,HUSTR_CHATMACRO0},UL,UL,
   def_str,ss_chat}, // chat string associated with 0 key
  {"chatmacro1", {0,&chat_macros[1]}, {0,HUSTR_CHATMACRO1},UL,UL,
   def_str,ss_chat}, // chat string associated with 1 key
  {"chatmacro2", {0,&chat_macros[2]}, {0,HUSTR_CHATMACRO2},UL,UL,
   def_str,ss_chat}, // chat string associated with 2 key
  {"chatmacro3", {0,&chat_macros[3]}, {0,HUSTR_CHATMACRO3},UL,UL,
   def_str,ss_chat}, // chat string associated with 3 key
  {"chatmacro4", {0,&chat_macros[4]}, {0,HUSTR_CHATMACRO4},UL,UL,
   def_str,ss_chat}, // chat string associated with 4 key
  {"chatmacro5", {0,&chat_macros[5]}, {0,HUSTR_CHATMACRO5},UL,UL,
   def_str,ss_chat}, // chat string associated with 5 key
  {"chatmacro6", {0,&chat_macros[6]}, {0,HUSTR_CHATMACRO6},UL,UL,
   def_str,ss_chat}, // chat string associated with 6 key
  {"chatmacro7", {0,&chat_macros[7]}, {0,HUSTR_CHATMACRO7},UL,UL,
   def_str,ss_chat}, // chat string associated with 7 key
  {"chatmacro8", {0,&chat_macros[8]}, {0,HUSTR_CHATMACRO8},UL,UL,
   def_str,ss_chat}, // chat string associated with 8 key
  {"chatmacro9", {0,&chat_macros[9]}, {0,HUSTR_CHATMACRO9},UL,UL,
   def_str,ss_chat}, // chat string associated with 9 key

  {"Automap settings",{NULL},{0},UL,UL,def_none,ss_none},
  //jff 1/7/98 defaults for automap colors
  //jff 4/3/98 remove -1 in lower range, 0 now disables new map features
  {"mapcolor_back", {&mapcolor_back}, {247},0,255,  // black //jff 4/6/98 new black
   def_colour,ss_auto}, // color used as background for automap
  {"mapcolor_grid", {&mapcolor_grid}, {104},0,255,  // dk gray
   def_colour,ss_auto}, // color used for automap grid lines
  {"mapcolor_wall", {&mapcolor_wall}, {23},0,255,   // red-brown
   def_colour,ss_auto}, // color used for one side walls on automap
  {"mapcolor_fchg", {&mapcolor_fchg}, {55},0,255,   // lt brown
   def_colour,ss_auto}, // color used for lines floor height changes across
  {"mapcolor_cchg", {&mapcolor_cchg}, {215},0,255,  // orange
   def_colour,ss_auto}, // color used for lines ceiling height changes across
  {"mapcolor_clsd", {&mapcolor_clsd}, {208},0,255,  // white
   def_colour,ss_auto}, // color used for lines denoting closed doors, objects
  {"mapcolor_rkey", {&mapcolor_rkey}, {175},0,255,  // red
   def_colour,ss_auto}, // color used for red key sprites
  {"mapcolor_bkey", {&mapcolor_bkey}, {204},0,255,  // blue
   def_colour,ss_auto}, // color used for blue key sprites
  {"mapcolor_ykey", {&mapcolor_ykey}, {231},0,255,  // yellow
   def_colour,ss_auto}, // color used for yellow key sprites
  {"mapcolor_rdor", {&mapcolor_rdor}, {175},0,255,  // red
   def_colour,ss_auto}, // color used for closed red doors
  {"mapcolor_bdor", {&mapcolor_bdor}, {204},0,255,  // blue
   def_colour,ss_auto}, // color used for closed blue doors
  {"mapcolor_ydor", {&mapcolor_ydor}, {231},0,255,  // yellow
   def_colour,ss_auto}, // color used for closed yellow doors
  {"mapcolor_tele", {&mapcolor_tele}, {119},0,255,  // dk green
   def_colour,ss_auto}, // color used for teleporter lines
  {"mapcolor_secr", {&mapcolor_secr}, {252},0,255,  // purple
   def_colour,ss_auto}, // color used for lines around secret sectors
  {"mapcolor_exit", {&mapcolor_exit}, {0},0,255,    // none
   def_colour,ss_auto}, // color used for exit lines
  {"mapcolor_unsn", {&mapcolor_unsn}, {104},0,255,  // dk gray
   def_colour,ss_auto}, // color used for lines not seen without computer map
  {"mapcolor_flat", {&mapcolor_flat}, {88},0,255,   // lt gray
   def_colour,ss_auto}, // color used for lines with no height changes
  {"mapcolor_sprt", {&mapcolor_sprt}, {112},0,255,  // green
   def_colour,ss_auto}, // color used as things
  {"mapcolor_item", {&mapcolor_item}, {231},0,255,  // yellow
   def_colour,ss_auto}, // color used for counted items
  {"mapcolor_hair", {&mapcolor_hair}, {208},0,255,  // white
   def_colour,ss_auto}, // color used for dot crosshair denoting center of map
  {"mapcolor_sngl", {&mapcolor_sngl}, {208},0,255,  // white
   def_colour,ss_auto}, // color used for the single player arrow
  {"mapcolor_me",   {&mapcolor_me}, {112},0,255, // green
   def_colour,ss_auto}, // your (player) colour
  {"mapcolor_enemy",   {&mapcolor_enemy}, {177},0,255,
   def_colour,ss_auto},
  {"mapcolor_frnd",   {&mapcolor_frnd}, {112},0,255,
   def_colour,ss_auto},
  //jff 3/9/98 add option to not show secrets til after found
  {"map_secret_after", {&map_secret_after}, {0},0,1, // show secret after gotten
   def_bool,ss_auto}, // prevents showing secret sectors till after entered
  {"map_point_coord", {&map_point_coordinates}, {0},0,1,
   def_bool,ss_auto},
  {"map_level_stat", {&map_level_stat}, {1},0,1,
   def_bool,ss_auto},
  //jff 1/7/98 end additions for automap
  {"automapmode", {(int*)&automapmode}, {0}, 0, 31, // CPhipps - remember automap mode
   def_hex,ss_none}, // automap mode
  {"map_always_updates", {&map_always_updates}, {1},0,1,
   def_bool,ss_auto},
  {"map_grid_size", {&map_grid_size}, {128},8,256,
   def_int,ss_auto},
  {"map_scroll_speed", {&map_scroll_speed}, {8},1,32,
   def_int,ss_auto},
  {"map_wheel_zoom", {&map_wheel_zoom}, {1},0,1,
   def_bool,ss_auto},
#ifdef GL_DOOM
  {"map_use_multisamling", {&map_use_multisamling}, {1},0,1,
   def_bool,ss_auto},
#else
  {"map_use_multisamling", {&map_use_multisamling}, {0},0,1,
   def_bool,ss_auto},
#endif
#ifdef GL_DOOM
  {"map_textured", {&map_textured}, {1},0,1,
   def_bool,ss_auto},
  {"map_textured_trans", {&map_textured_trans}, {100},0,100,
   def_int,ss_auto},
  {"map_textured_overlay_trans", {&map_textured_overlay_trans}, {66},0,100,
   def_int,ss_auto},
  {"map_lines_overlay_trans", {&map_lines_overlay_trans}, {100},0,100,
   def_int,ss_auto},
#endif
  {"map_overlay_pos_x", {&map_overlay_pos_x}, {0},0,319,
   def_int,ss_auto},
  {"map_overlay_pos_y", {&map_overlay_pos_y}, {0},0,199,
   def_int,ss_auto},
  {"map_overlay_pos_width", {&map_overlay_pos_width}, {320},0,320,
   def_int,ss_auto},
  {"map_overlay_pos_height", {&map_overlay_pos_height}, {200},0,200,
   def_int,ss_auto},
  {"map_things_appearance", {(int*)&map_things_appearance}, {map_things_appearance_max-1},0,map_things_appearance_max-1,
   def_int,ss_auto},

  {"Heads-up display settings",{NULL},{0},UL,UL,def_none,ss_none},
  //jff 2/16/98 defaults for color ranges in hud and status
  {"hudcolor_titl", {&hudcolor_titl}, {5},0,9,  // gold range
   def_int,ss_auto}, // color range used for automap level title
  {"hudcolor_xyco", {&hudcolor_xyco}, {3},0,9,  // green range
   def_int,ss_auto}, // color range used for automap coordinates
   {"hudcolor_mapstat_title", {&hudcolor_mapstat_title}, {6},0,9, // red range
   def_int,ss_auto}, // color range used for automap statistics for titles
  {"hudcolor_mapstat_value", {&hudcolor_mapstat_value}, {2},0,9,    // gray range
   def_int,ss_auto}, // color range used for automap statistics for data
  {"hudcolor_mapstat_time", {&hudcolor_mapstat_time}, {2},0,9,    // gray range
   def_int,ss_auto}, // color range used for automap statistics for level time and total time
  {"hudcolor_mesg", {&hudcolor_mesg}, {6},0,9,  // red range
   def_int,ss_mess}, // color range used for messages during play
  {"hudcolor_chat", {&hudcolor_chat}, {5},0,9,  // gold range
   def_int,ss_mess}, // color range used for chat messages and entry
  {"hudcolor_list", {&hudcolor_list}, {5},0,9,  // gold range  //jff 2/26/98
   def_int,ss_mess}, // color range used for message review
  {"hud_msg_lines", {&hud_msg_lines}, {1},1,16,  // 1 line scrolling window
   def_int,ss_mess}, // number of messages in review display (1=disable)
  {"hud_list_bgon", {&hud_list_bgon}, {0},0,1,  // solid window bg ena //jff 2/26/98
   def_bool,ss_mess}, // enables background window behind message review

  {"health_red",    {&health_red}   , {25},0,200, // below is red
   def_int,ss_stat}, // amount of health for red to yellow transition
  {"health_yellow", {&health_yellow}, {50},0,200, // below is yellow
   def_int,ss_stat}, // amount of health for yellow to green transition
  {"health_green",  {&health_green} , {100},0,200,// below is green, above blue
   def_int,ss_stat}, // amount of health for green to blue transition
  {"armor_red",     {&armor_red}    , {25},0,200, // below is red
   def_int,ss_stat}, // amount of armor for red to yellow transition
  {"armor_yellow",  {&armor_yellow} , {50},0,200, // below is yellow
   def_int,ss_stat}, // amount of armor for yellow to green transition
  {"armor_green",   {&armor_green}  , {100},0,200,// below is green, above blue
   def_int,ss_stat}, // amount of armor for green to blue transition
  {"ammo_red",      {&ammo_red}     , {25},0,100, // below 25% is red
   def_int,ss_stat}, // percent of ammo for red to yellow transition
  {"ammo_yellow",   {&ammo_yellow}  , {50},0,100, // below 50% is yellow, above green
   def_int,ss_stat}, // percent of ammo for yellow to green transition
  {"ammo_colour_behaviour",{(int*)&ammo_colour_behaviour},
   {ammo_colour_behaviour_max-1}, // whether backpack changes thresholds above
   0,ammo_colour_behaviour_max-1,def_int,ss_stat},

  //jff 2/16/98 HUD and status feature controls
  {"hud_num",    {&hud_num}, {6},0,100,
   def_int,ss_none},
  //jff 2/23/98
  {"hud_displayed", {&hud_displayed},  {0},0,1, // whether hud is displayed
   def_bool,ss_none}, // enables display of HUD

//e6y
  {"Prboom-plus key bindings",{NULL},{0},UL,UL,def_none,ss_none},
  {"key_speedup", {&key_speed_up}, {KEYD_KEYPADPLUS},
   0,MAX_KEY,def_key,ss_keys},
  {"key_speeddown", {&key_speed_down}, {KEYD_KEYPADMINUS},
   0,MAX_KEY,def_key,ss_keys},
  {"key_speeddefault", {&key_speed_default}, {KEYD_KEYPADMULTIPLY},
   0,MAX_KEY,def_key,ss_keys},
  {"speed_step",{&speed_step},{0},0,1000,
   def_int,ss_none},
  {"key_demo_skip", {&key_demo_skip}, {KEYD_INSERT},
   0,MAX_KEY,def_key,ss_keys},
  {"key_level_restart", {&key_level_restart}, {KEYD_HOME},
   0,MAX_KEY,def_key,ss_keys},
  {"key_nextlevel", {&key_nextlevel}, {KEYD_PAGEDOWN},
   0,MAX_KEY,def_key,ss_keys},
  {"key_demo_jointogame", {&key_demo_jointogame}, {'q'},
   0,MAX_KEY,def_key,ss_keys},
  {"key_demo_endlevel", {&key_demo_endlevel}, {KEYD_END},
   0,MAX_KEY,def_key,ss_keys},
  {"key_walkcamera", {&key_walkcamera}, {KEYD_KEYPAD0},
   0,MAX_KEY,def_key,ss_keys},
  {"key_showalive", {&key_showalive}, {KEYD_KEYPADDIVIDE},
   0,MAX_KEY,def_key,ss_keys},

  {"Prboom-plus heads-up display settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"hudadd_gamespeed", {&hudadd_gamespeed},  {0},0,1,
   def_bool,ss_stat},
  {"hudadd_leveltime", {&hudadd_leveltime},  {0},0,1,
   def_bool,ss_stat},
  {"hudadd_demotime", {&hudadd_demotime},  {0},0,1,
   def_bool,ss_stat},
  {"hudadd_secretarea", {&hudadd_secretarea},  {0},0,1,
   def_bool,ss_stat},
  {"hudadd_smarttotals", {&hudadd_smarttotals},  {0},0,1,
   def_bool,ss_stat},
  {"hudadd_demoprogressbar", {&hudadd_demoprogressbar},  {1},0,1,
   def_bool,ss_stat},
  {"hudadd_crosshair", {&hudadd_crosshair},  {0},0,HU_CROSSHAIRS-1,
   def_bool,ss_stat},
  {"hudadd_crosshair_scale", {&hudadd_crosshair_scale},  {0},0,1,
   def_bool,ss_stat},
  {"hudadd_crosshair_color", {&hudadd_crosshair_color},  {3},0,9,
   def_int,ss_stat},
  {"hudadd_crosshair_health", {&hudadd_crosshair_health},  {0},0,1,
   def_bool,ss_stat},
  {"hudadd_crosshair_target", {&hudadd_crosshair_target},  {0},0,1,
   def_bool,ss_stat},
   {"hudadd_crosshair_target_color", {&hudadd_crosshair_target_color}, {9},0,9,
   def_int,ss_stat},
  {"hudadd_crosshair_lock_target", {&hudadd_crosshair_lock_target},  {0},0,1,
   def_bool,ss_stat},

  //e6y
  {"Prboom-plus mouse settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"mouse_acceleration",{&mouse_acceleration},{0},0,UL,
   def_int,ss_none},
  {"mouse_sensitivity_mlook",{&mouseSensitivity_mlook},{10},0,UL,
   def_int,ss_none},
  {"mouse_doubleclick_as_use", {&mouse_doubleclick_as_use},  {1},0,1,
   def_bool,ss_stat},

  {"Prboom-plus demos settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"demo_extendedformat", {&demo_extendedformat_default},  {1},0,1,
   def_bool,ss_stat},
  {"demo_demoex_filename", {NULL,&demo_demoex_filename}, {0,""},UL,UL,
   def_str,ss_none},
  {"getwad_cmdline", {NULL, &getwad_cmdline}, {0,""},UL,UL,
   def_str,ss_none},
  {"demo_overwriteexisting", {&demo_overwriteexisting},  {1},0,1,
   def_bool,ss_stat},

  {"Prboom-plus game settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"movement_strafe50", {&movement_strafe50},  {0},0,1,
   def_bool,ss_stat},
  {"movement_strafe50onturns", {&movement_strafe50onturns},  {0},0,1,
   def_bool,ss_stat},
  {"movement_shorttics", {&movement_shorttics},  {0},0,1,
   def_bool,ss_stat},
  {"interpolation_maxobjects", {&interpolation_maxobjects},  {0},0,UL,
   def_int,ss_stat},

  {"Prboom-plus misc settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"showendoom", {&showendoom},  {0},0,1,
   def_bool,ss_stat},
  {"screenshot_dir", {NULL,&screenshot_dir}, {0,""},UL,UL,
   def_str,ss_none},
#ifdef GL_DOOM
  {"health_bar", {&health_bar}, {0},0,1,
   def_bool,ss_stat},
  {"health_bar_full_length", {&health_bar_full_length}, {1},0,1,
   def_bool,ss_stat},
  {"health_bar_red", {&health_bar_red}, {50},0,100,
   def_int,ss_stat},
  {"health_bar_yellow", {&health_bar_yellow}, {99},0,100,
   def_int,ss_stat},
  {"health_bar_green", {&health_bar_green}, {0},0,100,
   def_int,ss_stat},
#endif

  // NSM
  {"Video capture encoding settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"cap_soundcommand",{NULL, &cap_soundcommand},{0,"oggenc2 -r -R %s -q 5 - -o output.ogg"},UL,UL,def_str,ss_none},
  {"cap_videocommand",{NULL, &cap_videocommand},{0,"x264 -o output.mp4 --crf 18 --muxer mp4 --demuxer raw --input-csp rgb --input-depth 8 --input-res %wx%h --fps %r -"},UL,UL,def_str,ss_none},
  {"cap_muxcommand",{NULL, &cap_muxcommand},{0,"mkvmerge -o %f output.mp4 output.ogg"},UL,UL,def_str,ss_none},
  {"cap_tempfile1",{NULL, &cap_tempfile1},{0,"output.ogg"},UL,UL,def_str,ss_none},
  {"cap_tempfile2",{NULL, &cap_tempfile2},{0,"output.mp4"},UL,UL,def_str,ss_none},
  {"cap_remove_tempfiles", {&cap_remove_tempfiles},{1},0,1,def_bool,ss_none},
  {"cap_fps", {&cap_fps},{60},16,300,def_int,ss_none},

  {"Prboom-plus video settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"sdl_video_window_pos", {NULL,&sdl_video_window_pos}, {0,"center"},UL,UL,
   def_str,ss_none},
  {"palette_ondamage", {&palette_ondamage},  {1},0,1,
   def_bool,ss_stat},
  {"palette_onbonus", {&palette_onbonus},  {1},0,1,
   def_bool,ss_stat},
  {"palette_onpowers", {&palette_onpowers},  {1},0,1,
   def_bool,ss_stat},
  {"render_wipescreen", {&render_wipescreen},  {1},0,1,
   def_bool,ss_stat},
  {"render_screen_multiply", {&render_screen_multiply},  {1},1,4,
   def_int,ss_stat},
  {"render_aspect", {&render_aspect},  {0},0,4,
   def_int,ss_stat},
  {"render_doom_lightmaps", {&render_doom_lightmaps},  {0},0,1,
   def_bool,ss_stat},
  {"fake_contrast", {&fake_contrast},  {1},0,1,
   def_bool,ss_stat}, /* cph - allow crappy fake contrast to be disabled */
  {"render_stretch_hud", {&render_stretch_hud_default},{patch_stretch_16x10},0,patch_stretch_max - 1,
  def_int,ss_stat},
  {"render_patches_scalex", {&render_patches_scalex},{0},0,16,
  def_int,ss_stat},
  {"render_patches_scaley", {&render_patches_scaley},{0},0,16,
  def_int,ss_stat},
  {"render_stretchsky",{&r_stretchsky},{1},0,1,
   def_bool,ss_none},
  {"sprites_doom_order", {&sprites_doom_order}, {DOOM_ORDER_STATIC},0,DOOM_ORDER_LAST - 1,
   def_int,ss_stat},

  {"movement_mouselook", {&movement_mouselook},  {0},0,1,
   def_bool,ss_stat},
  {"movement_maxviewpitch", {&movement_maxviewpitch},  {90},0,90,
   def_int,ss_stat},
  {"movement_mouseinvert", {&movement_mouseinvert},  {0},0,1,
   def_bool,ss_stat},

#ifdef GL_DOOM
  {"Prboom-plus OpenGL settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"gl_allow_detail_textures", {&gl_allow_detail_textures},  {1},0,1,
   def_bool,ss_stat},
  {"gl_detail_maxdist", {&gl_detail_maxdist},  {0},0,65535,
   def_int,ss_stat},
  {"render_multisampling", {&render_multisampling},  {0},0,8,
   def_int,ss_stat},
  {"render_fov", {&render_fov},  {90},20,160,
   def_int,ss_stat},
  {"gl_spriteclip",{(int*)&gl_spriteclip},{spriteclip_smart}, spriteclip_const, spriteclip_smart, def_int,ss_none},
   {"gl_spriteclip_threshold", {&gl_spriteclip_threshold},  {10},0,100,
   def_int,ss_stat},
   {"gl_sprites_frustum_culling", {&gl_sprites_frustum_culling},  {1},0,1,
   def_bool,ss_stat},
  {"render_paperitems", {&render_paperitems},  {0},0,1,
   def_bool,ss_stat},
  {"gl_boom_colormaps", {&gl_boom_colormaps_default},  {1},0,1,
   def_bool,ss_stat},
  {"gl_hires_24bit_colormap", {&gl_hires_24bit_colormap},  {0},0,1,
   def_bool,ss_stat},
  {"gl_texture_internal_hires", {&gl_texture_internal_hires},  {1},0,1,
   def_bool,ss_stat},
  {"gl_texture_external_hires", {&gl_texture_external_hires},  {0},0,1,
   def_bool,ss_stat},
  {"gl_hires_override_pwads", {&gl_hires_override_pwads},  {0},0,1,
   def_bool,ss_stat},
  {"gl_texture_hires_dir", {NULL,&gl_texture_hires_dir}, {0,""},UL,UL,
   def_str,ss_none},
  {"gl_texture_hqresize", {&gl_texture_hqresize},  {0},0,1,
   def_bool,ss_stat},
  {"gl_texture_hqresize_textures", {&gl_texture_hqresize_textures},
   {hq_scale_2x},hq_scale_none,hq_scale_max-1, def_int,ss_stat},
  {"gl_texture_hqresize_sprites", {&gl_texture_hqresize_sprites},
   {hq_scale_none},hq_scale_none,hq_scale_max-1, def_int,ss_stat},
  {"gl_texture_hqresize_patches", {&gl_texture_hqresize_patches},
   {hq_scale_2x},hq_scale_none,hq_scale_max-1,def_int,ss_stat},
  {"gl_motionblur", {&gl_motionblur},  {0},0,1,
   def_bool,ss_stat},
  {"gl_motionblur_min_speed", {NULL,&motion_blur.str_min_speed}, {0,"21.36"},UL,UL,
   def_str,ss_none},
  {"gl_motionblur_min_angle", {NULL,&motion_blur.str_min_angle}, {0,"20.0"},UL,UL,
   def_str,ss_none},
  {"gl_motionblur_att_a", {NULL,&motion_blur.str_att_a}, {0,"55.0"},UL,UL,
   def_str,ss_none},
  {"gl_motionblur_att_b", {NULL,&motion_blur.str_att_b}, {0,"1.8"},UL,UL,
   def_str,ss_none},
  {"gl_motionblur_att_c", {NULL,&motion_blur.str_att_c}, {0,"0.9"},UL,UL,
   def_str,ss_none},
  {"gl_lightmode",{(int*)&gl_lightmode_default},{gl_lightmode_glboom},
   gl_lightmode_glboom, gl_lightmode_last-1, def_int,ss_none},
  {"gl_light_ambient", {&gl_light_ambient},  {20},1,255,
   def_int,ss_stat},
  {"gl_fog", {&gl_fog},  {1},0,1,
   def_bool,ss_stat},
  {"gl_fog_color", {&gl_fog_color},  {0},0,0xffffff,
   def_hex,ss_stat},
  {"useglgamma",{&useglgamma},{6},0,MAX_GLGAMMA,
   def_int,ss_none},
  {"gl_color_mip_levels", {&gl_color_mip_levels},  {0},0,1,
   def_bool,ss_stat},
  {"gl_shadows", {&simple_shadows.enable},  {0},0,1,
   def_bool,ss_stat},
  {"gl_shadows_maxdist",{&gl_shadows_maxdist},{1000},0,32767,
   def_int,ss_none},
  {"gl_shadows_factor",{&gl_shadows_factor},{128},0,255,
   def_int,ss_none},
  {"gl_blend_animations",{&gl_blend_animations},{0},0,1,
   def_bool,ss_none},
#endif
  {"Prboom-plus emulation settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"overrun_spechit_warn", {&overflows[OVERFLOW_SPECHIT].warn},  {0},0,1,
   def_bool,ss_stat},
  {"overrun_spechit_emulate", {&overflows[OVERFLOW_SPECHIT].emulate},  {1},0,1,
   def_bool,ss_stat},
  {"overrun_reject_warn", {&overflows[OVERFLOW_REJECT].warn},  {0},0,1,
   def_bool,ss_stat},
  {"overrun_reject_emulate", {&overflows[OVERFLOW_REJECT].emulate},  {1},0,1,
   def_bool,ss_stat},
  {"overrun_intercept_warn", {&overflows[OVERFLOW_INTERCEPT].warn},  {0},0,1,
   def_bool,ss_stat},
  {"overrun_intercept_emulate", {&overflows[OVERFLOW_INTERCEPT].emulate},  {1},0,1,
   def_bool,ss_stat},
  {"overrun_playeringame_warn", {&overflows[OVERFLOW_PLYERINGAME].warn},  {0},0,1,
   def_bool,ss_stat},
  {"overrun_playeringame_emulate", {&overflows[OVERFLOW_PLYERINGAME].emulate},  {1},0,1,
   def_bool,ss_stat},
  {"overrun_donut_warn", {&overflows[OVERFLOW_DONUT].warn},  {0},0,1,
   def_bool,ss_stat},
  {"overrun_donut_emulate", {&overflows[OVERFLOW_DONUT].emulate},  {0},0,1,
   def_bool,ss_stat},
  {"overrun_missedbackside_warn", {&overflows[OVERFLOW_MISSEDBACKSIDE].warn},  {0},0,1,
   def_bool,ss_stat},
  {"overrun_missedbackside_emulate", {&overflows[OVERFLOW_MISSEDBACKSIDE].emulate},  {0},0,1,
   def_bool,ss_stat},

  {"Prboom-plus 'bad' compatibility settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"comperr_zerotag", {&default_comperr[comperr_zerotag]},  {0},0,1,
   def_bool,ss_stat},
  {"comperr_passuse", {&default_comperr[comperr_passuse]},  {0},0,1,
   def_bool,ss_stat},
  {"comperr_hangsolid", {&default_comperr[comperr_hangsolid]},  {0},0,1,
   def_bool,ss_stat},
  {"comperr_blockmap", {&default_comperr[comperr_blockmap]},  {0},0,1,
   def_bool,ss_stat},
  {"comperr_allowjump", {&default_comperr[comperr_allowjump]},  {0},0,2,
   def_int,ss_stat},
  {"comperr_freeaim", {&default_comperr[comperr_freeaim]},  {0},0,1,
   def_bool,ss_stat},

#ifdef USE_WINDOWS_LAUNCHER
  {"Prboom-plus launcher settings",{NULL},{0},UL,UL,def_none,ss_none},
  {"launcher_enable",{(int*)&launcher_enable},{launcher_enable_never},
   launcher_enable_never, launcher_enable_count - 1, def_int,ss_none},
  {"launcher_history0", {NULL,&launcher_history[0]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history1", {NULL,&launcher_history[1]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history2", {NULL,&launcher_history[2]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history3", {NULL,&launcher_history[3]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history4", {NULL,&launcher_history[4]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history5", {NULL,&launcher_history[5]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history6", {NULL,&launcher_history[6]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history7", {NULL,&launcher_history[7]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history8", {NULL,&launcher_history[8]}, {0,""},UL,UL,def_str,ss_none},
  {"launcher_history9", {NULL,&launcher_history[9]}, {0,""},UL,UL,def_str,ss_none},
#endif
  {"Prboom-plus demo patterns list. Put your patterns here",{NULL},{0},UL,UL,def_none,ss_none},
  {"demo_patterns_mask", {NULL, &demo_patterns_mask, &demo_patterns_count, &demo_patterns_list}, {0,"demo_pattern",9, &demo_patterns_list_def[0]},UL,UL,def_arr,ss_none},
  {"demo_pattern0", {NULL,&demo_patterns_list_def[0]}, 
   {0,"DOOM 2: Hell on Earth/((lv)|(nm)|(pa)|(ty))\\d\\d.\\d\\d\\d\\.lmp/doom2.wad"},UL,UL,def_str,ss_none},
  {"demo_pattern1", {NULL,&demo_patterns_list_def[1]}, 
   {0,"DOOM 2: Plutonia Experiment/p(c|f|l|n|p|r|s|t)\\d\\d.\\d\\d\\d\\.lmp/doom2.wad|plutonia.wad"},UL,UL,def_str,ss_none},
  {"demo_pattern2", {NULL,&demo_patterns_list_def[2]}, 
   {0,"DOOM 2: TNT - Evilution/((e(c|f|v|p|r|s|t))|(tn))\\d\\d.\\d\\d\\d\\.lmp/doom2.wad|tnt.wad"},UL,UL,def_str,ss_none},
  {"demo_pattern3", {NULL,&demo_patterns_list_def[3]}, 
   {0,"The Ultimate DOOM/(((e|f|n|p|r|t|u)\\dm\\d)|(n\\ds\\d)).\\d\\d\\d\\.lmp/doom.wad"},UL,UL,def_str,ss_none},
  {"demo_pattern4", {NULL,&demo_patterns_list_def[4]}, 
   {0,"Alien Vendetta/a(c|f|n|p|r|s|t|v)\\d\\d.\\d\\d\\d\\.lmp/doom2.wad|av.wad|av.deh"},UL,UL,def_str,ss_none},
  {"demo_pattern5", {NULL,&demo_patterns_list_def[5]}, 
   {0,"Requiem/r(c|f|n|p|q|r|s|t)\\d\\d.\\d\\d\\d\\.lmp/doom2.wad|requiem.wad|req21fix.wad|reqmus.wad"},UL,UL,def_str,ss_none},
  {"demo_pattern6", {NULL,&demo_patterns_list_def[6]}, 
   {0,"Hell Revealed/h(c|e|f|n|p|r|s|t)\\d\\d.\\d\\d\\d\\.lmp/doom2.wad|hr.wad|hrmus.wad"},UL,UL,def_str,ss_none},
  {"demo_pattern7", {NULL,&demo_patterns_list_def[7]}, 
   {0,"Memento Mori/mm\\d\\d.\\d\\d\\d\\.lmp/doom2.wad|mm.wad|mmmus.wad"},UL,UL,def_str,ss_none},
  {"demo_pattern8", {NULL,&demo_patterns_list_def[8]}, 
   {0,"Memento Mori 2/m2\\d\\d.\\d\\d\\d\\.lmp/doom2.wad|mm2.wad|mm2mus.wad"},UL,UL,def_str,ss_none},

  {"Weapon preferences",{NULL},{0},UL,UL,def_none,ss_none},
  // killough 2/8/98: weapon preferences set by user:
  {"weapon_choice_1", {&weapon_preferences[0][0]}, {6}, 0,9,
   def_int,ss_weap}, // first choice for weapon (best)
  {"weapon_choice_2", {&weapon_preferences[0][1]}, {9}, 0,9,
   def_int,ss_weap}, // second choice for weapon
  {"weapon_choice_3", {&weapon_preferences[0][2]}, {4}, 0,9,
   def_int,ss_weap}, // third choice for weapon
  {"weapon_choice_4", {&weapon_preferences[0][3]}, {3}, 0,9,
   def_int,ss_weap}, // fourth choice for weapon
  {"weapon_choice_5", {&weapon_preferences[0][4]}, {2}, 0,9,
   def_int,ss_weap}, // fifth choice for weapon
  {"weapon_choice_6", {&weapon_preferences[0][5]}, {8}, 0,9,
   def_int,ss_weap}, // sixth choice for weapon
  {"weapon_choice_7", {&weapon_preferences[0][6]}, {5}, 0,9,
   def_int,ss_weap}, // seventh choice for weapon
  {"weapon_choice_8", {&weapon_preferences[0][7]}, {7}, 0,9,
   def_int,ss_weap}, // eighth choice for weapon
  {"weapon_choice_9", {&weapon_preferences[0][8]}, {1}, 0,9,
   def_int,ss_weap}, // ninth choice for weapon (worst)

  // cournia - support for arbitrary music file (defaults are mp3)
  {"Music", {NULL},{0},UL,UL,def_none,ss_none},
  {"mus_e1m1", {0,&S_music_files[mus_e1m1]}, {0,"e1m1.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m2", {0,&S_music_files[mus_e1m2]}, {0,"e1m2.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m3", {0,&S_music_files[mus_e1m3]}, {0,"e1m3.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m4", {0,&S_music_files[mus_e1m4]}, {0,"e1m4.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m5", {0,&S_music_files[mus_e1m5]}, {0,"e1m5.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m6", {0,&S_music_files[mus_e1m6]}, {0,"e1m6.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m7", {0,&S_music_files[mus_e1m7]}, {0,"e1m7.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m8", {0,&S_music_files[mus_e1m8]}, {0,"e1m8.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e1m9", {0,&S_music_files[mus_e1m9]}, {0,"e1m9.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m1", {0,&S_music_files[mus_e2m1]}, {0,"e2m1.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m2", {0,&S_music_files[mus_e2m2]}, {0,"e2m2.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m3", {0,&S_music_files[mus_e2m3]}, {0,"e2m3.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m4", {0,&S_music_files[mus_e2m4]}, {0,"e2m4.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m5", {0,&S_music_files[mus_e2m5]}, {0,"e1m7.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m6", {0,&S_music_files[mus_e2m6]}, {0,"e2m6.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m7", {0,&S_music_files[mus_e2m7]}, {0,"e2m7.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m8", {0,&S_music_files[mus_e2m8]}, {0,"e2m8.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e2m9", {0,&S_music_files[mus_e2m9]}, {0,"e3m1.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m1", {0,&S_music_files[mus_e3m1]}, {0,"e3m1.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m2", {0,&S_music_files[mus_e3m2]}, {0,"e3m2.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m3", {0,&S_music_files[mus_e3m3]}, {0,"e3m3.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m4", {0,&S_music_files[mus_e3m4]}, {0,"e1m8.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m5", {0,&S_music_files[mus_e3m5]}, {0,"e1m7.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m6", {0,&S_music_files[mus_e3m6]}, {0,"e1m6.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m7", {0,&S_music_files[mus_e3m7]}, {0,"e2m7.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m8", {0,&S_music_files[mus_e3m8]}, {0,"e3m8.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_e3m9", {0,&S_music_files[mus_e3m9]}, {0,"e1m9.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_inter", {0,&S_music_files[mus_inter]}, {0,"e2m3.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_intro", {0,&S_music_files[mus_intro]}, {0,"intro.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_bunny", {0,&S_music_files[mus_bunny]}, {0,"bunny.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_victor", {0,&S_music_files[mus_victor]}, {0,"victor.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_introa", {0,&S_music_files[mus_introa]}, {0,"intro.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_runnin", {0,&S_music_files[mus_runnin]}, {0,"runnin.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_stalks", {0,&S_music_files[mus_stalks]}, {0,"stalks.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_countd", {0,&S_music_files[mus_countd]}, {0,"countd.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_betwee", {0,&S_music_files[mus_betwee]}, {0,"betwee.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_doom", {0,&S_music_files[mus_doom]}, {0,"doom.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_the_da", {0,&S_music_files[mus_the_da]}, {0,"the_da.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_shawn", {0,&S_music_files[mus_shawn]}, {0,"shawn.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_ddtblu", {0,&S_music_files[mus_ddtblu]}, {0,"ddtblu.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_in_cit", {0,&S_music_files[mus_in_cit]}, {0,"in_cit.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_dead", {0,&S_music_files[mus_dead]}, {0,"dead.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_stlks2", {0,&S_music_files[mus_stlks2]}, {0,"stalks.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_theda2", {0,&S_music_files[mus_theda2]}, {0,"the_da.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_doom2", {0,&S_music_files[mus_doom2]}, {0,"doom.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_ddtbl2", {0,&S_music_files[mus_ddtbl2]}, {0,"ddtblu.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_runni2", {0,&S_music_files[mus_runni2]}, {0,"runnin.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_dead2", {0,&S_music_files[mus_dead2]}, {0,"dead.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_stlks3", {0,&S_music_files[mus_stlks3]}, {0,"stalks.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_romero", {0,&S_music_files[mus_romero]}, {0,"romero.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_shawn2", {0,&S_music_files[mus_shawn2]}, {0,"shawn.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_messag", {0,&S_music_files[mus_messag]}, {0,"messag.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_count2", {0,&S_music_files[mus_count2]}, {0,"countd.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_ddtbl3", {0,&S_music_files[mus_ddtbl3]}, {0,"ddtblu.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_ampie", {0,&S_music_files[mus_ampie]}, {0,"ampie.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_theda3", {0,&S_music_files[mus_theda3]}, {0,"the_da.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_adrian", {0,&S_music_files[mus_adrian]}, {0,"adrian.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_messg2", {0,&S_music_files[mus_messg2]}, {0,"messag.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_romer2", {0,&S_music_files[mus_romer2]}, {0,"romero.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_tense", {0,&S_music_files[mus_tense]}, {0,"tense.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_shawn3", {0,&S_music_files[mus_shawn3]}, {0,"shawn.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_openin", {0,&S_music_files[mus_openin]}, {0,"openin.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_evil", {0,&S_music_files[mus_evil]}, {0,"evil.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_ultima", {0,&S_music_files[mus_ultima]}, {0,"ultima.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_read_m", {0,&S_music_files[mus_read_m]}, {0,"read_m.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_dm2ttl", {0,&S_music_files[mus_dm2ttl]}, {0,"dm2ttl.mp3"},UL,UL,
   def_str,ss_none},
  {"mus_dm2int", {0,&S_music_files[mus_dm2int]}, {0,"dm2int.mp3"},UL,UL,
   def_str,ss_none},
};

int numdefaults;
static char* defaultfile; // CPhipps - static, const

//
// M_SaveDefaults
//

void M_SaveDefaults (void)
{
  int   i;
  FILE* f;

  f = fopen (defaultfile, "w");
  if (!f)
    return; // can't write the file, but don't complain

  // 3/3/98 explain format of file

  fprintf(f,"# Doom config file\n");
  fprintf(f,"# Format:\n");
  fprintf(f,"# variable   value\n");

  for (i = 0 ; i < numdefaults ; i++) {
    if (defaults[i].type == def_none) {
      // CPhipps - pure headers
      fprintf(f, "\n# %s\n", defaults[i].name);
    } else
      // e6y: arrays
      if (defaults[i].type == def_arr)
      {
        int k;
        fprintf (f,"%-25s \"%s\"\n",defaults[i].name,*(defaults[i].location.ppsz));
        for (k = 0; k < *(defaults[i].location.array_size); k++)
        {
          char ***arr = defaults[i].location.array_data;
          if ((*arr)[k])
          {
            char def[80];
            sprintf(def, "%s%d", *(defaults[i].location.ppsz), k);
            fprintf (f,"%-25s \"%s\"\n",def, (*arr)[k]);
          }
        }
        i += defaults[i].defaultvalue.array_size;
      }
      else

    // CPhipps - modified for new default_t form
    if (!IS_STRING(defaults[i])) //jff 4/10/98 kill super-hack on pointer value
      {
      // CPhipps - remove keycode hack
      // killough 3/6/98: use spaces instead of tabs for uniform justification
      if (defaults[i].type == def_hex)
  fprintf (f,"%-25s 0x%x\n",defaults[i].name,*(defaults[i].location.pi));
      else
  fprintf (f,"%-25s %5i\n",defaults[i].name,*(defaults[i].location.pi));
      }
    else
      {
      fprintf (f,"%-25s \"%s\"\n",defaults[i].name,*(defaults[i].location.ppsz));
      }
    }

  fclose (f);
}

/*
 * M_LookupDefault
 *
 * cph - mimic MBF function for now. Yes it's crap.
 */

struct default_s *M_LookupDefault(const char *name)
{
  int i;
  for (i = 0 ; i < numdefaults ; i++)
  {
    if ((defaults[i].type != def_none) && !strcmp(name, defaults[i].name))
      return &defaults[i];
  }

  I_Error("M_LookupDefault: %s not found",name);
  return NULL;
}

//
// M_LoadDefaults
//

#define NUMCHATSTRINGS 10 // phares 4/13/98

#define CFG_BUFFERMAX 32000

void M_LoadDefaults (void)
{
  int   i;
  int   len;
  FILE* f;
  char  def[80];
  char* strparm = malloc(CFG_BUFFERMAX);
  char* cfgline = malloc(CFG_BUFFERMAX);
  char* newstring = NULL;   // killough
  int   parm;
  dboolean isstring;
  // e6y: arrays
  default_t *item = NULL;

  // set everything to base values

  numdefaults = sizeof(defaults)/sizeof(defaults[0]);
  for (i = 0 ; i < numdefaults ; i++) {
    if (defaults[i].location.ppsz)
      *defaults[i].location.ppsz = strdup(defaults[i].defaultvalue.psz);
    if (defaults[i].location.pi)
      *defaults[i].location.pi = defaults[i].defaultvalue.i;
  }

  //e6y: arrays
  for (i = 0 ; i < numdefaults ; i++) {
    if (defaults[i].type == def_arr)
    {
      int k;
      default_t *item = &defaults[i];
      char ***arr = (char***)(item->location.array_data);
      // free memory
      for (k = 0; k < *(item->location.array_size); k++)
      {
        if ((*arr)[k])
        {
          free((*arr)[k]);
          (*arr)[k] = NULL;
        }
      }
      free(*arr);
      *arr = NULL;
      *(item->location.array_size) = 0;
      // load predefined data
      *arr = realloc(*arr, sizeof(char*) * item->defaultvalue.array_size);
      *(item->location.array_size) = item->defaultvalue.array_size;
      item->location.array_index = 0;
      for (k = 0; k < item->defaultvalue.array_size; k++)
      {
        if (item->defaultvalue.array_data[k])
          (*arr)[k] = strdup(item->defaultvalue.array_data[k]);
        else
          (*arr)[k] = strdup("");
      }
    }
  }

  // check for a custom default file

#if ((defined GL_DOOM) && (defined _MSC_VER))
#define BOOM_CFG "glboom-plus.cfg"
#else
#define BOOM_CFG "prboom-plus.cfg"
#endif

  i = M_CheckParm ("-config");
  if (i && i < myargc-1)
  {
    defaultfile = strdup(myargv[i+1]);
  }
  else
  {
    const char* exedir = I_DoomExeDir();
    /* get config file from same directory as executable */
    int len = doom_snprintf(NULL, 0, "%s/" BOOM_CFG, exedir);
    defaultfile = malloc(len+1);
    doom_snprintf(defaultfile, len+1, "%s/" BOOM_CFG, exedir);
  }

  lprintf (LO_CONFIRM, " default file: %s\n",defaultfile);

  // read the file in, overriding any set defaults

  f = fopen (defaultfile, "r");
  if (f)
    {
    while (!feof(f))
      {
      isstring = false;
      parm = 0;
      fgets(cfgline, CFG_BUFFERMAX, f);
      if (sscanf (cfgline, "%79s %[^\n]\n", def, strparm) == 2)
        {

        //jff 3/3/98 skip lines not starting with an alphanum

        if (!isalnum(def[0]))
          continue;

        if (strparm[0] == '"') {
          // get a string default

          isstring = true;
          len = strlen(strparm);
          newstring = malloc(len);
          strparm[len-1] = 0; // clears trailing double-quote mark
          strcpy(newstring, strparm+1); // clears leading double-quote mark
  } else if ((strparm[0] == '0') && (strparm[1] == 'x')) {
    // CPhipps - allow ints to be specified in hex
    sscanf(strparm+2, "%x", &parm);
  } else {
          sscanf(strparm, "%i", &parm);
    // Keycode hack removed
  }

        // e6y: arrays
        if (item)
        {
          int *pcount = item->location.array_size;
          int *index = &item->location.array_index;
          char ***arr = (char***)(item->location.array_data);
          if (!strncmp(def, *(item->location.ppsz), strlen(*(item->location.ppsz))) 
              && ((item->maxvalue == UL) || *(item->location.array_size) < item->maxvalue) )
          {
            if ((*index) + 1 > *pcount)
            {
              *arr = realloc(*arr, sizeof(char*) * ((*index) + 1));
              (*pcount)++;
            }
            else
            {
              if ((*arr)[(*index)])
              {
                free((*arr)[(*index)]);
                (*arr)[(*index)] = NULL;
              }
            }
            (*arr)[(*index)] = newstring;
            (*index)++;
            continue;
          }
          else
          {
            item = NULL;
          }
        }

        for (i = 0 ; i < numdefaults ; i++)
          if ((defaults[i].type != def_none) && !strcmp(def, defaults[i].name))
            {
              // e6y: arrays
              if (defaults[i].type == def_arr)
              {
                union { const char **c; char **s; } u; // type punning via unions

                u.c = defaults[i].location.ppsz;
                free(*(u.s));
                *(u.s) = newstring;

                item = &defaults[i];
                continue;
              }

      // CPhipps - safety check
            if (isstring != IS_STRING(defaults[i])) {
        lprintf(LO_WARN, "M_LoadDefaults: Type mismatch reading %s\n", defaults[i].name);
        continue;
      }
            if (!isstring)
              {

              //jff 3/4/98 range check numeric parameters

              if ((defaults[i].minvalue==UL || defaults[i].minvalue<=parm) &&
                  (defaults[i].maxvalue==UL || defaults[i].maxvalue>=parm))
                *(defaults[i].location.pi) = parm;
              }
            else
              {
                union { const char **c; char **s; } u; // type punning via unions

                u.c = defaults[i].location.ppsz;
                free(*(u.s));
                *(u.s) = newstring;
              }
            break;
            }
        }
      }

    fclose (f);
    }

  free(strparm);
  free(cfgline);

  //jff 3/4/98 redundant range checks for hud deleted here
  /* proff 2001/7/1 - added prboom.wad as last entry so it's always loaded and
     doesn't overlap with the cfg settings */
  //e6y: Check on existence of prboom.wad
  if (!(wad_files[0] = I_FindFile(PACKAGE_TARNAME ".wad", "")))
    I_Error("PrBoom-Plus.wad not found. Can't continue.");
}


//
// SCREEN SHOTS
//

//
// M_ScreenShot
//
// Modified by Lee Killough so that any number of shots can be taken,
// the code is faster, and no annoying "screenshot" message appears.

// CPhipps - modified to use its own buffer for the image
//         - checks for the case where no file can be created (doesn't occur on POSIX systems, would on DOS)
//         - track errors better
//         - split into 2 functions

//
// M_DoScreenShot
// Takes a screenshot into the names file

const char *screenshot_dir;

void M_DoScreenShot (const char* fname)
{
  if (I_ScreenShot(fname) != 0)
    doom_printf("M_ScreenShot: Error writing screenshot\n");
}

#ifndef SCREENSHOT_DIR
#define SCREENSHOT_DIR "."
#endif

#ifdef HAVE_LIBSDL2_IMAGE
#define SCREENSHOT_EXT ".png"
#else
#define SCREENSHOT_EXT ".bmp"
#endif

const char* M_CheckWritableDir(const char *dir)
{
  static char *base = NULL;
  static int base_len = 0;

  const char *result = NULL;
  int len;

  if (!dir || !(len = strlen(dir)))
  {
    return NULL;
  }

  if (len + 1 > base_len)
  {
    base_len = len + 1;
    base = malloc(len + 1);
  }

  if (base)
  {
    strcpy(base, dir);

    if (base[len - 1] != '\\' && base[len - 1] != '/')
      strcat(base, "/");
    if (!access(base, O_RDWR))
    {
      base[strlen(base) - 1] = 0;
      result = base;
    }
  }

  return result;
}

void M_ScreenShot(void)
{
  static int shot;
  char       *lbmname = NULL;
  int        startshot;
  const char *shot_dir = NULL;
  int p;
  int        success = 0;

  if ((p = M_CheckParm("-shotdir")) && (p < myargc - 1))
    shot_dir = M_CheckWritableDir(myargv[p + 1]);
  if (!shot_dir)
    shot_dir = M_CheckWritableDir(screenshot_dir);
  if (!shot_dir)
#ifdef _WIN32
    shot_dir = M_CheckWritableDir(I_DoomExeDir());
#else
    shot_dir = (!access(SCREENSHOT_DIR, 2) ? SCREENSHOT_DIR : NULL);
#endif

  if (shot_dir)
  {
    startshot = shot; // CPhipps - prevent infinite loop

    do {
      int size = doom_snprintf(NULL, 0, "%s/doom%02d" SCREENSHOT_EXT, shot_dir, shot);
      lbmname = realloc(lbmname, size+1);
      doom_snprintf(lbmname, size+1, "%s/doom%02d" SCREENSHOT_EXT, shot_dir, shot);
      shot++;
    } while (!access(lbmname,0) && (shot != startshot) && (shot < 10000));

    if (access(lbmname,0))
    {
      S_StartSound(NULL,gamemode==commercial ? sfx_radio : sfx_tink);
      M_DoScreenShot(lbmname); // cph
      success = 1;
    }
    free(lbmname);
    if (success) return;
  }

  doom_printf ("M_ScreenShot: Couldn't create screenshot");
  return;
}

int M_StrToInt(const char *s, int *l)
{      
  return (
    (sscanf(s, " 0x%x", l) == 1) ||
    (sscanf(s, " 0X%x", l) == 1) ||
    (sscanf(s, " 0%o", l) == 1) ||
    (sscanf(s, " %d", l) == 1)
  );
}

int M_StrToFloat(const char *s, float *f)
{      
  return (
    (sscanf(s, " %f", f) == 1)
  );
}

int M_DoubleToInt(double x)
{
#ifdef __GNUC__
 double tmp = x;
 return (int)tmp;
#else
 return (int)x;
#endif
}

char* M_Strlwr(char* str)
{
  char* p;
  for (p=str; *p; p++) *p = tolower(*p);
  return str;
}

char* M_Strupr(char* str)
{
  char* p;
  for (p=str; *p; p++) *p = toupper(*p);
  return str;
}

char *M_StrRTrim(char *str)
{
  char *end;

  if (str)
  {
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace(*end))
    {
      end--;
    }

    // Write new null terminator
    *(end + 1) = 0;
  }

  return str;
}

void M_ArrayClear(array_t *data)
{
  data->count = 0;
}

void M_ArrayFree(array_t *data)
{
  if (data->data)
  {
    free(data->data);
    data->data = NULL;
  }

  data->capacity = 0;
  data->count = 0;
}

void M_ArrayAddItem(array_t *data, void *item, int itemsize)
{
  if (data->count + 1 >= data->capacity)
  {
    data->capacity = (data->capacity ? data->capacity * 2 : 128);
    data->data = realloc(data->data, data->capacity * itemsize);
  }

  memcpy((unsigned char*)data->data + data->count * itemsize, item, itemsize);
  data->count++;
}

void* M_ArrayGetNewItem(array_t *data, int itemsize)
{
  if (data->count + 1 >= data->capacity)
  {
    data->capacity = (data->capacity ? data->capacity * 2 : 128);
    data->data = realloc(data->data, data->capacity * itemsize);
  }

  data->count++;

  return (unsigned char*)data->data + (data->count - 1) * itemsize;
}
