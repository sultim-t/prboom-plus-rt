// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: M_misc.c,v 1.2 2000/04/26 20:00:03 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//  Main loop menu stuff.
//  Default Config File.
//  PCX Screenshots.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: M_misc.c,v 1.2 2000/04/26 20:00:03 proff_fs Exp $";

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
#include "d_main.h"
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

#ifdef _MSC_VER // proff
#include <io.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else //_MSC_VER
#include <unistd.h>
#endif //_MSC_VER
//#include <errno.h>

//
// DEFAULTS
//

static int config_help;         //jff 3/3/98
int usemouse;
int usejoystick;
int screenshot_pcx; //jff 3/30/98 // option to output screenshot as pcx or bmp
extern int mousebfire;
extern int mousebstrafe;
extern int mousebforward;
extern int mousebbackward; // proff 08/17/98: Added backward to mousebuttons
extern int joybfire;
extern int joybstrafe;
extern int joybuse;
extern int joybspeed;
extern int viewwidth;
extern int viewheight;
extern int mouseSensitivity_horiz,mouseSensitivity_vert;  // killough
extern int realtic_clock_rate;         // killough 4/13/98: adjustable timer
extern int leds_always_off;            // killough 3/6/98
extern int tran_filter_pct;            // killough 2/21/98
// proff 11/19/98: Added for 2 new filters
extern int tran_filter_pct25;
extern int tran_filter_pct75;
// proff 11/27/98: Added translucency to exploding missiles
extern int missile_explode_trans;
// proff 11/27/98: Added translucency for ressurected enemys
extern int vil_ressurected_trans;

extern int screenblocks;
extern int showMessages;

// proff 08/17/98: Added the following to allow change without recompile
#ifdef _WIN32
extern int snd_freq;
extern int snd_bits;
extern int snd_stereo;
extern int snd_mididevice;
#endif // _WIN32

extern char *chat_macros[], *wad_files[], *deh_files[];  // killough 10/98

//jff 3/3/98 added min, max, and help string to all entries
//jff 4/10/98 added isstr field to specify whether value is string or int
//
// killough 11/98: entirely restructured to allow options to be modified
// from wads, and to consolidate with menu code

default_t defaults[] = {
  { //jff 3/3/98
    "config_help",
    &config_help, NULL,
    1, {0,1}, number, ss_none, wad_no,
    "1 to show help strings about each variable in config file"
  },

  { // jff 3/24/98 allow default skill setting
    "default_skill",
    &defaultskill, NULL,
    3, {1,5}, number, ss_none, wad_no,
    "selects default skill 1=TYTD 2=NTR 3=HMP 4=UV 5=NM"
  },

#ifndef _WIN32 // proff

  { // jff 1/18/98 allow Allegro drivers to be set,  -1 = autodetect
    "sound_card",
    &default_snd_card, NULL,
    -1, {-1,7}, number, ss_gen, wad_no,
    "code used by Allegro to select sounds driver, -1 is autodetect"
  },

  {
    "music_card",
    &default_mus_card, NULL,
    -1, {-1,9}, number, ss_gen, wad_no,
    "code used by Allegro to select music driver, -1 is autodetect"
  },

  { // jff 3/4/98 detect # voices
    "detect_voices",
    &detect_voices, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "1 enables voice detection prior to calling install sound"
  },

  { // killough 11/98: hires
    "hires", &hires, NULL,
    0, {0,1}, number, ss_gen, wad_no,
    "1 to enable 640x400 resolution for rendering scenes"
  },

#else // _WIN32

  {
    "vid_fullscreen", &vidFullScreen, NULL,
    0, {0,1}, number, ss_gen, wad_no,
    "1 to start fullscreen"
  },

  {
    "vid_width", &SCREENWIDTH, NULL,
    320, {320,MAX_SCREENWIDTH}, number, ss_gen, wad_no,
    "Screen width"
  },

  {
    "vid_height", &SCREENHEIGHT, NULL,
    200, {200,MAX_SCREENHEIGHT}, number, ss_gen, wad_no,
    "Screen height"
  },

  {
    "sound_card", &snd_card, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "0=No Sound, 1=DirectSound"
  },

  {
    "music_card", &mus_card, NULL,
    1, {0,2}, number, ss_gen, wad_no,
    "used to select MIDI transfer type. 0=No Music, 1=MCI-MIDI, 2=Stream-MIDI"
  },

  {
    "snd_mididevice", &snd_mididevice, NULL,
    0, {0,UL}, number, ss_gen, wad_no,
    "0=Midi Mapper"
  },

  {
    "snd_frequency", &snd_freq, NULL,
    11025, {11025,44100}, number, ss_gen, wad_no,
    "sample frequency"
  },

  {
    "snd_bits", &snd_bits, NULL,
    8, {8,16}, number, ss_gen, wad_no,
    "bits per sample"
  },

  {
    "snd_stereo", &snd_stereo, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "use stereo"
  },

#endif // _WIN32

  {
    "use_vsync",
    &use_vsync, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "1 to enable wait for vsync to avoid display tearing"
  },

  { // killough 8/15/98: page flipping option
    "page_flip",
    &page_flip, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "1 to enable page flipping to avoid display tearing"
  },

  {
    "realtic_clock_rate",
    &realtic_clock_rate, NULL,
    100, {10,1000}, number, ss_gen, wad_no,
    "Percentage of normal speed (35 fps) realtic clock runs at"
  },

#if 0 // MBF
  { // killough 10/98
    "disk_icon",
    &disk_icon, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "1 to enable flashing icon during disk IO"
  },
#endif

  { // killough 2/21/98
    "pitched_sounds",
    &pitched_sounds, NULL,
    0, {0,1}, number, ss_gen, wad_yes,
    "1 to enable variable pitch in sound effects (from id's original code)"
  },

  { // phares
    "translucency",
    &general_translucency, NULL,
    1, {0,1}, number, ss_gen, wad_yes,
    "1 to enable translucency for some things"
  },

  { // killough 2/21/98
    "tran_filter_pct25",
    &tran_filter_pct25, NULL,
    25, {0,100}, number, ss_gen, wad_yes,
    "set percentage of foreground/background translucency mix"
  },

  { // killough 2/21/98
    "tran_filter_pct",
    &tran_filter_pct, NULL,
    66, {0,100}, number, ss_gen, wad_yes,
    "set percentage of foreground/background translucency mix"
  },

  { // killough 2/21/98
    "tran_filter_pct75",
    &tran_filter_pct75, NULL,
    75, {0,100}, number, ss_gen, wad_yes,
    "set percentage of foreground/background translucency mix"
  },

  { // killough 2/8/98
    "max_player_corpse",
    &default_bodyquesize, NULL,
    32, {UL,UL},number, ss_gen, wad_no,
    "number of dead bodies in view supported (negative value = no limit)"
  },

  { // killough 10/98
    "flashing_hom",
    &flashing_hom, NULL,
    1, {0,1}, number, ss_gen, wad_yes,
    "1 to enable flashing HOM indicator"
  },

  { // killough 3/31/98
    "demo_insurance",
    &default_demo_insurance, NULL,
    2, {0,2},number, ss_none, wad_no,
    "1=take special steps ensuring demo sync, 2=only during recordings"
  },

  { // phares
    "weapon_recoil",
    &default_weapon_recoil, &weapon_recoil,
    0, {0,1}, number, ss_weap, wad_yes,
    "1 to enable recoil from weapon fire"
  },

  { // killough 10/98
    "doom_weapon_toggles",
    &doom_weapon_toggles, NULL,
    1, {0,1}, number, ss_weap, wad_no,
    "1 to toggle between SG/SSG and Fist/Chainsaw"
  },

  { // phares 2/25/98
    "player_bobbing",
    &default_player_bobbing, &player_bobbing,
    1, {0,1}, number, ss_weap, wad_no,
    "1 to enable player bobbing (view moving up/down slightly)"
  },

  { // killough 3/1/98
    "monsters_remember",
    &default_monsters_remember, &monsters_remember,
    1, {0,1}, number, ss_enem, wad_yes,
    "1 to enable monsters remembering enemies after killing others"
  },

  { // killough 7/19/98
    "monster_infighting",
    &default_monster_infighting, &monster_infighting,
    1, {0,1}, number, ss_enem, wad_yes,
    "1 to enable monsters fighting against each other when provoked"
  },

  { // killough 9/8/98
    "monster_backing",
    &default_monster_backing, &monster_backing,
    0, {0,1}, number, ss_enem, wad_yes,
    "1 to enable monsters backing away from targets"
  },

  { //killough 9/9/98:
    "monster_avoid_hazards",
    &default_monster_avoid_hazards, &monster_avoid_hazards,
    1, {0,1}, number, ss_enem, wad_yes,
    "1 to enable monsters to intelligently avoid hazards"
  },

  {
    "monkeys",
    &default_monkeys, &monkeys,
    0, {0,1}, number, ss_enem, wad_yes,
    "1 to enable monsters to move up/down steep stairs"
  },

  { //killough 9/9/98:
    "monster_friction",
    &default_monster_friction, &monster_friction,
    1, {0,1}, number, ss_enem, wad_yes,
    "1 to enable monsters to be affected by friction"
  },

  { //killough 9/9/98:
    "help_friends",
    &default_help_friends, &help_friends,
    1, {0,1}, number, ss_enem, wad_yes,
    "1 to enable monsters to help dying friends"
  },

#ifdef DOGS

  { // killough 7/19/98
    "player_helpers",
    &default_dogs, &dogs,
    0, {0,3}, number, ss_enem, wad_yes,
    "number of single-player helpers"
  },

  { // killough 8/8/98
    "friend_distance",
    &default_distfriend, &distfriend,
    128, {0,999}, number, ss_enem, wad_yes,
    "distance friends stay away"
  },

  { // killough 10/98
    "dog_jumping",
    &default_dog_jumping, &dog_jumping,
    1, {0,1}, number, ss_enem, wad_yes,
    "1 to enable dogs to jump"
  },
#endif

  { // no color changes on status bar
    "sts_always_red",
    &sts_always_red, NULL,
    0, {0,1}, number, ss_stat, wad_yes,
    "1 to disable use of color on status bar"
  },

  {
    "sts_pct_always_gray",
    &sts_pct_always_gray, NULL,
    1, {0,1}, number, ss_stat, wad_yes,
    "1 to make percent signs on status bar always gray"
  },

  { // killough 2/28/98
    "sts_traditional_keys",
    &sts_traditional_keys, NULL,
    1, {0,1}, number, ss_stat, wad_yes,
    "1 to disable doubled card and skull key display on status bar"
  },

  { // killough 4/17/98
    "traditional_menu",
    &traditional_menu, NULL,
    0, {0,1}, number, ss_none, wad_yes,
    "1 to use Doom's main menu ordering"
  },

  { // killough 3/6/98
    "leds_always_off",
    &leds_always_off, NULL,
    0, {0,1}, number, ss_gen, wad_no,
    "1 to keep keyboard LEDs turned off"
  },

  { //jff 4/3/98 allow unlimited sensitivity
    "mouse_sensitivity_horiz",
    &mouseSensitivity_horiz, NULL,
    5, {0,UL}, number, ss_none, wad_no,
    "adjust horizontal (x) mouse sensitivity"
  },

  { //jff 4/3/98 allow unlimited sensitivity
    "mouse_sensitivity_vert",
    &mouseSensitivity_vert, NULL,
    5, {0,UL}, number, ss_none, wad_no,
    "adjust vertical (y) mouse sensitivity"
  },

  {
    "sfx_volume",
    &snd_SfxVolume, NULL,
    8, {0,15}, number, ss_none, wad_no,
    "adjust sound effects volume"
  },

  {
    "music_volume",
    &snd_MusicVolume, NULL,
    8, {0,15}, number, ss_none, wad_no,
    "adjust music volume"
  },

  {
    "show_messages",
    &showMessages, NULL,
    1, {0,1}, number, ss_none, wad_no,
    "1 to enable message display"
  },

  { // killough 3/6/98: preserve autorun across games
    "autorun",
    &autorun, NULL,
    0, {0,1}, number, ss_none, wad_no,
    "1 to enable autorun"
  },

  { // killough 2/21/98: default to 10
    "screenblocks",
    &screenblocks, NULL,
    10, {3,11}, number, ss_none, wad_no,
    "initial play screen size"
  },

  { //jff 3/6/98 fix erroneous upper limit in range
    "usegamma",
    &usegamma, NULL,
    0, {0,4}, number, ss_none, wad_no,
    "screen brightness (gamma correction)"
  },

  { // killough 10/98: preloaded files
    "wadfile_1",
    (int *) &wad_files[0], NULL,
    (int) "", {0}, string, ss_none, wad_no,
    "WAD file preloaded at program startup"
  },

  {
    "wadfile_2",
    (int *) &wad_files[1], NULL,
    (int) "", {0}, string, ss_none, wad_no,
    "WAD file preloaded at program startup"
  },

  {
    "dehfile_1",
    (int *) &deh_files[0], NULL,
    (int) "", {0}, string, ss_none, wad_no,
    "DEH/BEX file preloaded at program startup"
  },

  {
    "dehfile_2",
    (int *) &deh_files[1], NULL,
    (int) "", {0}, string, ss_none, wad_no,
    "DEH/BEX file preloaded at program startup"
  },

  // killough 10/98: compatibility vector:

  {
    "comp_zombie",
    &default_comp[comp_zombie], &comp[comp_zombie],
    0, {0,1}, number, ss_comp, wad_yes,
    "Zombie players can exit levels"
  },

  {
    "comp_infcheat",
    &default_comp[comp_infcheat], &comp[comp_infcheat],
    0, {0,1}, number, ss_comp, wad_yes,
    "Powerup cheats are not infinite duration"
  },

  {
    "comp_stairs",
    &default_comp[comp_stairs], &comp[comp_stairs],
    0, {0,1}, number, ss_comp, wad_yes,
    "Build stairs exactly the same way that Doom does"
  },

  {
    "comp_telefrag",
    &default_comp[comp_telefrag], &comp[comp_telefrag],
    0, {0,1}, number, ss_comp, wad_yes,
    "Monsters can telefrag on MAP30"
  },

  {
    "comp_dropoff",
    &default_comp[comp_dropoff], &comp[comp_dropoff],
    0, {0,1}, number, ss_comp, wad_yes,
    "Some objects never move over tall ledges"
  },

  {
    "comp_falloff",
    &default_comp[comp_falloff], &comp[comp_falloff],
    0, {0,1}, number, ss_comp, wad_yes,
    "Objects don't fall off ledges under their own weight"
  },

  {
    "comp_staylift",
    &default_comp[comp_staylift], &comp[comp_staylift],
    0, {0,1}, number, ss_comp, wad_yes,
    "Monsters randomly walk off of moving lifts"
  },

  {
    "comp_doorstuck",
    &default_comp[comp_doorstuck], &comp[comp_doorstuck],
    0, {0,1}, number, ss_comp, wad_yes,
    "Monsters get stuck on doortracks"
  },

  {
    "comp_pursuit",
    &default_comp[comp_pursuit], &comp[comp_pursuit],
    0, {0,1}, number, ss_comp, wad_yes,
    "Monsters don't give up pursuit of targets"
  },

  {
    "comp_vile",
    &default_comp[comp_vile], &comp[comp_vile],
    0, {0,1}, number, ss_comp, wad_yes,
    "Arch-Vile resurrects invincible ghosts"
  },

  {
    "comp_pain",
    &default_comp[comp_pain], &comp[comp_pain],
    0, {0,1}, number, ss_comp, wad_yes,
    "Pain Elemental limited to 20 lost souls"
  },

  {
    "comp_skull",
    &default_comp[comp_skull], &comp[comp_skull],
    0, {0,1}, number, ss_comp, wad_yes,
    "Lost souls get stuck behind walls"
  },

  {
    "comp_blazing",
    &default_comp[comp_blazing], &comp[comp_blazing],
    0, {0,1}, number, ss_comp, wad_yes,
    "Blazing doors make double closing sounds"
  },

  {
    "comp_doorlight",
    &default_comp[comp_doorlight], &comp[comp_doorlight],
    0, {0,1}, number, ss_comp, wad_yes,
    "Tagged doors don't trigger special lighting"
  },

  {
    "comp_god",
    &default_comp[comp_god], &comp[comp_god],
    0, {0,1}, number, ss_comp, wad_yes,
    "God mode isn't absolute"
  },

  {
    "comp_skymap",
    &default_comp[comp_skymap], &comp[comp_skymap],
    0, {0,1}, number, ss_comp, wad_yes,
    "Sky is unaffected by invulnerability"
  },

  {
    "comp_floors",
    &default_comp[comp_floors], &comp[comp_floors],
    0, {0,1}, number, ss_comp, wad_yes,
    "Use exactly Doom's floor motion behavior"
  },

  {
    "comp_model",
    &default_comp[comp_model], &comp[comp_model],
    0, {0,1}, number, ss_comp, wad_yes,
    "Use exactly Doom's linedef trigger model"
  },

  {
    "comp_zerotags",
    &default_comp[comp_zerotags], &comp[comp_zerotags],
    0, {0,1}, number, ss_comp, wad_yes,
    "Linedef effects work with sector tag = 0"
  },

  // For key bindings, the values stored in the key_* variables       // phares
  // are the internal Doom Codes. The values stored in the default.cfg
  // file are the keyboard codes. I_ScanCode2DoomCode converts from
  // keyboard codes to Doom Codes. I_DoomCode2ScanCode converts from
  // Doom Codes to keyboard codes, and is only used when writing back
  // to default.cfg. For the printable keys (i.e. alphas, numbers)
  // the Doom Code is the ascii code.

  {
    "key_right",
    &key_right, NULL,
    KEYD_RIGHTARROW, {0,255}, number, ss_keys, wad_no,
    "key to turn right"
  },

  {
    "key_left",
    &key_left, NULL,
    KEYD_LEFTARROW, {0,255}, number, ss_keys, wad_no,
    "key to turn left"
  },

  {
    "key_up",
    &key_up, NULL,
    KEYD_UPARROW, {0,255}, number, ss_keys, wad_no,
    "key to move forward"
  },

  {
    "key_down",
    &key_down, NULL,
    KEYD_DOWNARROW, {0,255}, number, ss_keys, wad_no,
    "key to move backward"
  },

  { // phares 3/7/98
    "key_menu_right",
    &key_menu_right, NULL,
    KEYD_RIGHTARROW, {0,255}, number, ss_keys, wad_no,
    "key to move right in a menu"
  },
  {
    "key_menu_left",
    &key_menu_left, NULL,
    KEYD_LEFTARROW, {0,255}, number, ss_keys, wad_no,
    "key to move left in a menu"
  },

  {
    "key_menu_up",
    &key_menu_up, NULL,
    KEYD_UPARROW, {0,255}, number, ss_keys, wad_no,
    "key to move up in a menu"
  },

  {
    "key_menu_down",
    &key_menu_down, NULL,
    KEYD_DOWNARROW, {0,255}, number, ss_keys, wad_no,
    "key to move down in a menu"
  },

  {
    "key_menu_backspace",
    &key_menu_backspace, NULL,
    KEYD_BACKSPACE, {0,255}, number, ss_keys, wad_no,
    "key to erase last character typed in a menu"
  },

  {
    "key_menu_escape",
    &key_menu_escape, NULL,
    KEYD_ESCAPE, {0,255}, number, ss_keys, wad_no,
    "key to leave a menu"
  }, // phares 3/7/98

  {
    "key_menu_enter",
    &key_menu_enter, NULL,
    KEYD_ENTER, {0,255}, number, ss_keys, wad_no,
    "key to select from menu or review past messages"
  },

  {
    "key_strafeleft",
    &key_strafeleft, NULL,
    ',', {0,255}, number, ss_keys, wad_no,
    "key to strafe left (sideways left)"
  },

  {
    "key_straferight",
    &key_straferight, NULL,
    '.', {0,255}, number, ss_keys, wad_no,
    "key to strafe right (sideways right)"
  },

  {
    "key_fire",
    &key_fire, NULL,
    KEYD_RCTRL, {0,255}, number, ss_keys, wad_no,
    "key to fire current weapon"
  },

  {
    "key_use",
    &key_use, NULL,
    ' ', {0,255}, number, ss_keys, wad_no,
    "key to open a door, use a switch"
  },

  {
    "key_strafe",
    &key_strafe, NULL,
    KEYD_RALT, {0,255}, number, ss_keys, wad_no,
    "key to use with arrows to strafe"
  },

  {
    "key_speed",
    &key_speed, NULL,
    KEYD_RSHIFT, {0,255}, number, ss_keys, wad_no,
    "key to run (move fast)"
  },

  {
    "key_savegame",
    &key_savegame, NULL,
    KEYD_F2, {0,255}, number, ss_keys, wad_no,
    "key to save current game"
  },

  {
    "key_loadgame",
    &key_loadgame, NULL,
    KEYD_F3, {0,255}, number, ss_keys, wad_no,
    "key to restore from saved games"
  },

  {
    "key_soundvolume",
    &key_soundvolume, NULL,
    KEYD_F4, {0,255}, number, ss_keys, wad_no,
    "key to bring up sound control panel"
  },

  {
    "key_hud",
    &key_hud, NULL,
    KEYD_F5, {0,255}, number, ss_keys, wad_no,
    "key to adjust heads up display mode"
  },

  {
    "key_quicksave",
    &key_quicksave, NULL,
    KEYD_F6, {0,255}, number, ss_keys, wad_no,
    "key to to save to last slot saved"
  },

  {
    "key_endgame",
    &key_endgame, NULL,
    KEYD_F7, {0,255}, number, ss_keys, wad_no,
    "key to end the game"
  },

  {
    "key_messages",
    &key_messages, NULL,
    KEYD_F8, {0,255}, number, ss_keys, wad_no,
    "key to toggle message enable"
  },

  {
    "key_quickload",
    &key_quickload, NULL,
    KEYD_F9, {0,255}, number, ss_keys, wad_no,
    "key to load from quick saved game"
  },

  {
    "key_quit",
    &key_quit, NULL,
    KEYD_F10, {0,255}, number, ss_keys, wad_no,
    "key to quit game to DOS"
  },

  {
    "key_gamma",
    &key_gamma, NULL,
    KEYD_F11, {0,255}, number, ss_keys, wad_no,
    "key to adjust screen brightness (gamma correction)"
  },

  {
    "key_spy",
    &key_spy, NULL,
    KEYD_F12, {0,255}, number, ss_keys, wad_no,
    "key to view from another player's vantage"
  },

  {
    "key_pause",
    &key_pause, NULL,
    KEYD_PAUSE, {0,255}, number, ss_keys, wad_no,
    "key to pause the game"
  },

  {
    "key_autorun",
    &key_autorun, NULL,
    KEYD_CAPSLOCK, {0,255}, number, ss_keys, wad_no,
    "key to toggle always run mode"
  },

  {
    "key_chat",
    &key_chat, NULL,
    't', {0,255}, number, ss_keys, wad_no,
    "key to enter a chat message"
  },

  {
    "key_backspace",
    &key_backspace, NULL,
    KEYD_BACKSPACE, {0,255}, number, ss_keys, wad_no,
    "key to erase last character typed"
  },

  {
    "key_enter",
    &key_enter, NULL,
    KEYD_ENTER, {0,255}, number, ss_keys, wad_no,
    "key to select from menu or review past messages"
  },

  {
    "key_map",
    &key_map, NULL,
    KEYD_TAB, {0,255}, number, ss_keys, wad_no,
    "key to toggle automap display"
  },

  { // phares 3/7/98
    "key_map_right",
    &key_map_right, NULL,
    KEYD_RIGHTARROW, {0,255}, number, ss_keys, wad_no,
    "key to shift automap right"
  },

  {
    "key_map_left",
    &key_map_left, NULL,
    KEYD_LEFTARROW, {0,255}, number, ss_keys, wad_no,
    "key to shift automap left"
  },

  {
    "key_map_up",
    &key_map_up, NULL,
    KEYD_UPARROW, {0,255}, number, ss_keys, wad_no,
    "key to shift automap up"
  },

  {
    "key_map_down",
    &key_map_down, NULL,
    KEYD_DOWNARROW, {0,255}, number, ss_keys, wad_no,
    "key to shift automap down"
  },

  {
    "key_map_zoomin",
    &key_map_zoomin, NULL,
    '=', {0,255}, number, ss_keys, wad_no,
    "key to enlarge automap"
  },

  {
    "key_map_zoomout",
    &key_map_zoomout, NULL,
    '-', {0,255}, number, ss_keys, wad_no,
    "key to reduce automap"
  },

  {
    "key_map_gobig",
    &key_map_gobig, NULL,
    '0', {0,255}, number, ss_keys, wad_no,
    "key to get max zoom for automap"
  },

  {
    "key_map_follow",
    &key_map_follow, NULL,
    'f', {0,255}, number, ss_keys, wad_no,
    "key to toggle scrolling/moving with automap"
  },

  {
    "key_map_mark",
    &key_map_mark, NULL,
    'm', {0,255}, number, ss_keys, wad_no,
    "key to drop a marker on automap"
  },

  {
    "key_map_clear",
    &key_map_clear, NULL,
    'c', {0,255}, number, ss_keys, wad_no,
    "key to clear all markers on automap"
  },

  {
    "key_map_grid",
    &key_map_grid, NULL,
    'g', {0,255}, number, ss_keys, wad_no,
    "key to toggle grid display over automap"
  },

  {
    "key_reverse",
    &key_reverse, NULL,
    '/', {0,255}, number, ss_keys, wad_no,
    "key to spin 180 instantly"
  },

  {
    "key_zoomin",
    &key_zoomin, NULL,
    '=', {0,255}, number, ss_keys, wad_no,
    "key to enlarge display"
  },

  {
    "key_zoomout",
    &key_zoomout, NULL,
    '-', {0,255}, number, ss_keys, wad_no,
    "key to reduce display"
  },

  {
    "key_chatplayer1",
    &destination_keys[0], NULL,
    'g', {0,255}, number, ss_keys, wad_no,
    "key to chat with player 1"
  },

  { // killough 11/98: fix 'i'/'b' reversal
    "key_chatplayer2",
    &destination_keys[1], NULL,
    'i', {0,255}, number, ss_keys, wad_no,
    "key to chat with player 2"
  },

  {  // killough 11/98: fix 'i'/'b' reversal
    "key_chatplayer3",
    &destination_keys[2], NULL,
    'b', {0,255}, number, ss_keys, wad_no,
    "key to chat with player 3"
  },

  {
    "key_chatplayer4",
    &destination_keys[3], NULL,
    'r', {0,255}, number, ss_keys, wad_no,
    "key to chat with player 4"
  },

  {
    "key_weapontoggle",
    &key_weapontoggle, NULL,
    '0', {0,255}, number, ss_keys, wad_no,
    "key to toggle between two most preferred weapons with ammo"
  },

  {
    "key_weapon1",
    &key_weapon1, NULL,
    '1', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 1 (fist/chainsaw)"
  },

  {
    "key_weapon2",
    &key_weapon2, NULL,
    '2', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 2 (pistol)"
  },

  {
    "key_weapon3",
    &key_weapon3, NULL,
    '3', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 3 (supershotgun/shotgun)"
  },

  {
    "key_weapon4",
    &key_weapon4, NULL,
    '4', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 4 (chaingun)"
  },

  {
    "key_weapon5",
    &key_weapon5, NULL,
    '5', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 5 (rocket launcher)"
  },

  {
    "key_weapon6",
    &key_weapon6, NULL,
    '6', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 6 (plasma rifle)"
  },

  {
    "key_weapon7",
    &key_weapon7, NULL,
    '7', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 7 (bfg9000)"
  },

  {
    "key_weapon8",
    &key_weapon8, NULL,
    '8', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 8 (chainsaw)"
  },

  {
    "key_weapon9",
    &key_weapon9, NULL,
    '9', {0,255}, number, ss_keys, wad_no,
    "key to switch to weapon 9 (supershotgun)"
  }, // phares

  { // killough 2/22/98: screenshot key
    "key_screenshot",
    &key_screenshot, NULL,
    '*', {0,255}, number, ss_keys, wad_no,
    "key to take a screenshot (devparm independent)"
  },

  { // HOME key  // killough 10/98: shortcut to setup menu
    "key_setup",
    &key_setup, NULL,
    199, {0,255}, number, ss_keys, wad_no,
    "shortcut key to enter setup menu"
  },

  { // jff 3/30/98 add ability to take screenshots in BMP format
    "screenshot_pcx",
    &screenshot_pcx, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "1 to take a screenshot in PCX format, 0 for BMP"
  },

  {
    "use_mouse",
    &usemouse, NULL,
    1, {0,1}, number, ss_gen, wad_no,
    "1 to enable use of mouse"
  },

  { //jff 3/8/98 allow -1 in mouse bindings to disable mouse function
    "mouseb_fire",
    &mousebfire, NULL,
    0, {-1,2}, number, ss_keys, wad_no,
    "mouse button number to use for fire (-1 = disable)"
  },

  {
    "mouseb_strafe",
    &mousebstrafe, NULL,
    1, {-1,2}, number, ss_keys, wad_no,
    "mouse button number to use for strafing (-1 = disable)"
  },

  {
    "mouseb_forward",
    &mousebforward, NULL,
    2, {-1,2}, number, ss_keys, wad_no,
    "mouse button number to use for forward motion (-1 = disable)"
  }, //jff 3/8/98 end of lower range change for -1 allowed in mouse binding

  {
    "mouseb_backward",
    &mousebbackward, NULL,
    -1, {-1,2}, number, ss_keys, wad_no,
    "mouse button number to use for backward motion (-1 = disable)"
  }, //jff 3/8/98 end of lower range change for -1 allowed in mouse binding

  {
    "use_joystick",
    &usejoystick, NULL,
    0, {0,1}, number, ss_gen, wad_no,
    "1 to enable use of joystick"
  },

  {
    "joyb_fire",
    &joybfire, NULL,
    0, {0,UL}, number, ss_keys, wad_no,
    "joystick button number to use for fire"
  },

  {
    "joyb_strafe",
    &joybstrafe, NULL,
    1, {0,UL}, 0, ss_keys, wad_no,
    "joystick button number to use for strafing"
  },

  {
    "joyb_speed",
    &joybspeed, NULL,
    2, {0,UL}, 0, ss_keys, wad_no,
    "joystick button number to use for running"
  },

  {
    "joyb_use",
    &joybuse, NULL,
    3, {0,UL}, 0, ss_keys, wad_no,
    "joystick button number to use for use/open"
  },

  { // killough
    "snd_channels",
    &default_numChannels, NULL,
    32, {1, 128}, 0, ss_gen, wad_no,
    "number of sound effects handled simultaneously"
  },

  {
    "chatmacro0",
    (int *) &chat_macros[0], NULL,
    (int) HUSTR_CHATMACRO0, {0}, string, ss_chat, wad_yes,
    "chat string associated with 0 key"
  },

  {
    "chatmacro1",
    (int *) &chat_macros[1], NULL,
    (int) HUSTR_CHATMACRO1, {0}, string, ss_chat, wad_yes,
    "chat string associated with 1 key"
  },

  {
    "chatmacro2",
    (int *) &chat_macros[2], NULL,
    (int) HUSTR_CHATMACRO2, {0}, string, ss_chat, wad_yes,
    "chat string associated with 2 key"
  },

  {
    "chatmacro3",
    (int *) &chat_macros[3], NULL,
    (int) HUSTR_CHATMACRO3, {0}, string, ss_chat, wad_yes,
    "chat string associated with 3 key"
  },

  {
    "chatmacro4",
    (int *) &chat_macros[4], NULL,
    (int) HUSTR_CHATMACRO4, {0}, string, ss_chat, wad_yes,
    "chat string associated with 4 key"
  },

  {
    "chatmacro5",
    (int *) &chat_macros[5], NULL,
    (int) HUSTR_CHATMACRO5, {0}, string, ss_chat, wad_yes,
    "chat string associated with 5 key"
  },

  {
    "chatmacro6",
    (int *) &chat_macros[6], NULL,
    (int) HUSTR_CHATMACRO6, {0}, string, ss_chat, wad_yes,
    "chat string associated with 6 key"
  },

  {
    "chatmacro7",
    (int *) &chat_macros[7], NULL,
    (int) HUSTR_CHATMACRO7, {0}, string, ss_chat, wad_yes,
    "chat string associated with 7 key"
  },

  {
    "chatmacro8",
    (int *) &chat_macros[8], NULL,
    (int) HUSTR_CHATMACRO8, {0}, string, ss_chat, wad_yes,
    "chat string associated with 8 key"
  },

  {
    "chatmacro9",
    (int *) &chat_macros[9], NULL,
    (int) HUSTR_CHATMACRO9, {0}, string, ss_chat, wad_yes,
    "chat string associated with 9 key"
  },

  //jff 1/7/98 defaults for automap colors
  //jff 4/3/98 remove -1 in lower range, 0 now disables new map features
  { // black //jff 4/6/98 new black
    "mapcolor_back",
    &mapcolor_back, NULL,
    247, {0,255}, number, ss_auto, wad_yes,
    "color used as background for automap"
  },

  {  // dk gray
    "mapcolor_grid",
    &mapcolor_grid, NULL,
    104, {0,255}, number, ss_auto, wad_yes,
    "color used for automap grid lines"
  },

  { // red-brown
    "mapcolor_wall",
    &mapcolor_wall, NULL,
    23, {0,255}, number, ss_auto, wad_yes,
    "color used for one side walls on automap"
  },

  { // lt brown
    "mapcolor_fchg",
    &mapcolor_fchg, NULL,
    55, {0,255}, number, ss_auto, wad_yes,
    "color used for lines floor height changes across"
  },

  { // orange
    "mapcolor_cchg",
    &mapcolor_cchg, NULL,
    215, {0,255}, number, ss_auto, wad_yes,
    "color used for lines ceiling height changes across"
  },

  { // white
    "mapcolor_clsd",
    &mapcolor_clsd, NULL,
    208, {0,255}, number, ss_auto, wad_yes,
    "color used for lines denoting closed doors, objects"
  },

  { // red
    "mapcolor_rkey",
    &mapcolor_rkey, NULL,
    175, {0,255}, number, ss_auto, wad_yes,
    "color used for red key sprites"
  },

  { // blue
    "mapcolor_bkey",
    &mapcolor_bkey, NULL,
    204, {0,255}, number, ss_auto, wad_yes,
    "color used for blue key sprites"
  },

  { // yellow
    "mapcolor_ykey",
    &mapcolor_ykey, NULL,
    231, {0,255}, number, ss_auto, wad_yes,
    "color used for yellow key sprites"
  },

  { // red
    "mapcolor_rdor",
    &mapcolor_rdor, NULL,
    175, {0,255}, number, ss_auto, wad_yes,
    "color used for closed red doors"
  },

  { // blue
    "mapcolor_bdor",
    &mapcolor_bdor, NULL,
    204, {0,255}, number, ss_auto, wad_yes,
    "color used for closed blue doors"
  },

  { // yellow
    "mapcolor_ydor",
    &mapcolor_ydor, NULL,
    231, {0,255}, number, ss_auto, wad_yes,
    "color used for closed yellow doors"
  },

  { // dk green
    "mapcolor_tele",
    &mapcolor_tele, NULL,
    119, {0,255}, number, ss_auto, wad_yes,
    "color used for teleporter lines"
  },

  { // purple
    "mapcolor_secr",
    &mapcolor_secr, NULL,
    252, {0,255}, number, ss_auto, wad_yes,
    "color used for lines around secret sectors"
  },

  { // none
    "mapcolor_exit",
    &mapcolor_exit, NULL,
    0, {0,255}, number, ss_auto, wad_yes,
    "color used for exit lines"
  },

  { // dk gray
    "mapcolor_unsn",
    &mapcolor_unsn, NULL,
    104, {0,255}, number, ss_auto, wad_yes,
    "color used for lines not seen without computer map"
  },

  { // lt gray
    "mapcolor_flat",
    &mapcolor_flat, NULL,
    88, {0,255}, number, ss_auto, wad_yes,
    "color used for lines with no height changes"
  },

  { // green
    "mapcolor_sprt",
    &mapcolor_sprt, NULL,
    112, {0,255}, number, ss_auto, wad_yes,
    "color used as things"
  },

  { // white
    "mapcolor_hair",
    &mapcolor_hair, NULL,
    208, {0,255}, number, ss_auto, wad_yes,
    "color used for dot crosshair denoting center of map"
  },

  { // white
    "mapcolor_sngl",
    &mapcolor_sngl, NULL,
    208, {0,255}, number, ss_auto, wad_yes,
    "color used for the single player arrow"
  },

  { // green
    "mapcolor_ply1",
    &mapcolor_plyr[0], NULL,
    112, {0,255}, number, ss_auto, wad_yes,
    "color used for the green player arrow"
  },

  { // lt gray
    "mapcolor_ply2",
    &mapcolor_plyr[1], NULL,
    88, {0,255}, number, ss_auto, wad_yes,
    "color used for the gray player arrow"
  },

  { // brown
    "mapcolor_ply3",
    &mapcolor_plyr[2], NULL,
    64, {0,255}, number, ss_auto, wad_yes,
    "color used for the brown player arrow"
  },

  { // red
    "mapcolor_ply4",
    &mapcolor_plyr[3], NULL,
    176, {0,255}, number, ss_auto, wad_yes,
    "color used for the red player arrow"
  },

  {  // purple                     // killough 8/8/98
    "mapcolor_frnd",
    &mapcolor_frnd, NULL,
    252, {0,255}, number, ss_auto, wad_yes,
    "color used for friends"
  },

  {
    "map_point_coord",
    &map_point_coordinates, NULL,
    1, {0,1}, number, ss_auto, wad_yes,
    "1 to show automap pointer coordinates in non-follow mode"
  },

  //jff 3/9/98 add option to not show secrets til after found
  // killough change default, to avoid spoilers and preserve Doom mystery
  { // show secret after gotten
    "map_secret_after",
    &map_secret_after, NULL,
    1, {0,1}, number, ss_auto, wad_yes,
    "1 to not show secret sectors till after entered"
  },

  //jff 1/7/98 end additions for automap

  //jff 2/16/98 defaults for color ranges in hud and status

  { // gold range
    "hudcolor_titl",
    &hudcolor_titl, NULL,
    5, {0,9}, number, ss_auto, wad_yes,
    "color range used for automap level title"
  },

  { // green range
    "hudcolor_xyco",
    &hudcolor_xyco, NULL,
    3, {0,9}, number, ss_auto, wad_yes,
    "color range used for automap coordinates"
  },

  { // red range
    "hudcolor_mesg",
    &hudcolor_mesg, NULL,
    6, {0,9}, number, ss_mess, wad_yes,
    "color range used for messages during play"
  },

  { // gold range
    "hudcolor_chat",
    &hudcolor_chat, NULL,
    5, {0,9}, number, ss_mess, wad_yes,
    "color range used for chat messages and entry"
  },

  { // killough 11/98
    "chat_msg_timer",
    &chat_msg_timer, NULL,
    4000, {0,UL}, 0, ss_mess, wad_yes,
    "Duration of chat messages (ms)"
  },

  { // gold range  //jff 2/26/98
    "hudcolor_list",
    &hudcolor_list, NULL,
    5, {0,9}, number, ss_mess, wad_yes,
    "color range used for message review"
  },

  { // 1 line scrolling window
    "hud_msg_lines",
    &hud_msg_lines, NULL,
    1, {1,16}, number, ss_mess, wad_yes,
    "number of lines in review display"
  },

  { // killough 11/98
    "hud_msg_scrollup",
    &hud_msg_scrollup, NULL,
    1, {0,1}, number, ss_mess, wad_yes,
    "1 enables message review list scrolling upward"
  },

  { // killough 11/98
    "hud_msg_timed",
    &hud_msg_timed, NULL,
    1, {0,1}, 0, ss_mess, wad_yes,
    "1 enables temporary message review list"
  },

  { // killough 11/98
    "hud_msg_timer",
    &hud_msg_timer, NULL,
    4000, {0,UL}, 0, ss_mess, wad_yes,
    "Duration of temporary message review list (ms)"
  },

  { // killough 11/98
    "message_list",
    &message_list, NULL,
    0, {0,1}, number, ss_mess, wad_yes,
    "1 means multiline message list is active"
  },

  { // killough 11/98
    "message_timer",
    &message_timer, NULL,
    4000, {0,UL}, 0, ss_mess, wad_yes,
    "Duration of normal Doom messages (ms)"
  },

  { // solid window bg ena //jff 2/26/98
    "hud_list_bgon",
    &hud_list_bgon, NULL,
    0, {0,1}, number, ss_mess, wad_yes,
    "1 enables background window behind message review"
  },

  { // hud broken up into 3 displays //jff 3/4/98
    "hud_distributed",
    &hud_distributed, NULL,
    0, {0,1}, number, ss_none, wad_yes,
    "1 splits HUD into three 2 line displays"
  },

  { // below is red
    "health_red",
    &health_red, NULL,
    25, {0,200}, number, ss_stat, wad_yes,
    "amount of health for red to yellow transition"
  },

  { // below is yellow
    "health_yellow",
    &health_yellow, NULL,
    50, {0,200}, number, ss_stat, wad_yes,
    "amount of health for yellow to green transition"
  },

  { // below is green, above blue
    "health_green",
    &health_green, NULL,
    100, {0,200}, number, ss_stat, wad_yes,
    "amount of health for green to blue transition"
  },

  { // below is red
    "armor_red",
    &armor_red, NULL,
    25, {0,200}, number, ss_stat, wad_yes,
    "amount of armor for red to yellow transition"
  },

  { // below is yellow
    "armor_yellow",
    &armor_yellow, NULL,
    50, {0,200}, number, ss_stat, wad_yes,
    "amount of armor for yellow to green transition"
  },

  { // below is green, above blue
    "armor_green",
    &armor_green, NULL,
    100, {0,200}, number, ss_stat, wad_yes,
    "amount of armor for green to blue transition"
  },

  { // below 25% is red
    "ammo_red",
    &ammo_red, NULL,
    25, {0,100}, number, ss_stat, wad_yes,
    "percent of ammo for red to yellow transition"
  },

  { // below 50% is yellow, above green
    "ammo_yellow",
    &ammo_yellow, NULL,
    50, {0,100}, number, ss_stat, wad_yes,
    "percent of ammo for yellow to green transition"
  },

  { // 0=off, 1=small, 2=full //jff 2/16/98 HUD and status feature controls
    "hud_active",
    &hud_active, NULL,
    2, {0,2}, number, ss_none, wad_yes,
    "0 for HUD off, 1 for HUD small, 2 for full HUD"
  },

  {  // whether hud is displayed //jff 2/23/98
    "hud_displayed",
    &hud_displayed, NULL,
    0, {0,1}, number, ss_none, wad_yes,
    "1 to enable display of HUD"
  },

  { // no secrets/items/kills HUD line
    "hud_nosecrets",
    &hud_nosecrets, NULL,
    1, {0,1}, number, ss_stat, wad_yes,
    "1 to disable display of kills/items/secrets on HUD"
  },

  {  // killough 2/8/98: weapon preferences set by user:
    "weapon_choice_1",
    &weapon_preferences[0][0], NULL,
    6, {1,9}, number, ss_weap, wad_yes,
    "first choice for weapon (best)"
  },

  {
    "weapon_choice_2",
    &weapon_preferences[0][1], NULL,
    9, {1,9}, number, ss_weap, wad_yes,
    "second choice for weapon"
  },

  {
    "weapon_choice_3",
    &weapon_preferences[0][2], NULL,
    4, {1,9}, number, ss_weap, wad_yes,
    "third choice for weapon"
  },

  {
    "weapon_choice_4",
    &weapon_preferences[0][3], NULL,
    3, {1,9}, number, ss_weap, wad_yes,
    "fourth choice for weapon"
  },

  {
    "weapon_choice_5",
    &weapon_preferences[0][4], NULL,
    2, {1,9}, number, ss_weap, wad_yes,
    "fifth choice for weapon"
  },

  {
    "weapon_choice_6",
    &weapon_preferences[0][5], NULL,
    8, {1,9}, number, ss_weap, wad_yes,
    "sixth choice for weapon"
  },

  {
    "weapon_choice_7",
    &weapon_preferences[0][6], NULL,
    5, {1,9}, number, ss_weap, wad_yes,
    "seventh choice for weapon "
  },

  {
    "weapon_choice_8",
    &weapon_preferences[0][7], NULL,
    7, {1,9}, number, ss_weap, wad_yes,
    "eighth choice for weapon"
  },

  {
    "weapon_choice_9",
    &weapon_preferences[0][8], NULL,
    1, {1,9}, number, ss_weap, wad_yes,
    "ninth choice for weapon (worst)"
  },

  {NULL}         // last entry
};

static char *defaultfile;
static H_boolean defaults_loaded = false;      // killough 10/98

// killough 10/98: keep track of comments in .cfg files
static struct { char *text; int line; } *comments;
static size_t comment, comment_alloc;
static int config_help_header;  // killough 10/98

#define NUMDEFAULTS ((unsigned)(sizeof defaults / sizeof *defaults - 1))

// killough 11/98: hash function for name lookup
static unsigned default_hash(const char *name)
{
  unsigned hash = 0;
  while (*name)
    hash = hash*2 + toupper(*name++);
  return hash % NUMDEFAULTS;
}

default_t *M_LookupDefault(const char *name)
{
  static int hash_init;
  register default_t *dp;

  // Initialize hash table if not initialized already
  if (!hash_init)
    for (hash_init = 1, dp = defaults; dp->name; dp++)
      {
        unsigned h = default_hash(dp->name);
        dp->next = defaults[h].first;
        defaults[h].first = dp;
      }

  // Look up name in hash table
  for (dp = defaults[default_hash(name)].first;
       dp && strcasecmp(name, dp->name); dp = dp->next);

  return dp;
}

//
// M_SaveDefaults
//

void M_SaveDefaults (void)
{
  char tmpfile[PATH_MAX+1];
  register default_t *dp;
  int line, blanks;
  FILE *f;

  // killough 10/98: for when exiting early
  if (!defaults_loaded || !defaultfile)
    return;

  sprintf(tmpfile, "%s/tmp%.5s.cfg", D_DoomExeDir(), D_DoomExeName());
  NormalizeSlashes(tmpfile);

  errno = 0;
  if (!(f = fopen(tmpfile, "w")))  // killough 9/21/98
    goto error;

  // 3/3/98 explain format of file
  // killough 10/98: use executable's name

  if (config_help && !config_help_header &&
      fprintf(f,";%s.cfg format:\n"
	      ";[min-max(default)] description of variable\n"
	      ";* at end indicates variable is settable in wads\n"
	      ";variable   value\n\n", D_DoomExeName()) == EOF)
    goto error;

  // killough 10/98: output comment lines which were read in during input

  for (blanks = 1, line = 0, dp = defaults; ; dp++, blanks = 0)
    {
      int brackets = 0, value;

      for (;line < comment && comments[line].line <= dp-defaults; line++)
        if (*comments[line].text != '[' || (brackets = 1, config_help))

	    // If we haven't seen any blank lines
	    // yet, and this one isn't blank,
	    // output a blank line for separation

            if ((!blanks && (blanks = 1, 
			     *comments[line].text != '\n' &&
			     putc('\n',f) == EOF)) ||
		fputs(comments[line].text, f) == EOF)
	      goto error;

      // If we still haven't seen any blanks,
      // Output a blank line for separation

      if (!blanks && putc('\n',f) == EOF)
	goto error;

      if (!dp->name)      // If we're at end of defaults table, exit loop
        break;

      //jff 3/3/98 output help string
      //
      // killough 10/98:
      // Don't output config help if any [ lines appeared before this one.
      // Make default values, and numeric range output, automatic.

      if (config_help && !brackets)
	if ((dp->isstr ? 
	     fprintf(f,"[(\"%s\")]", (char *) dp->defaultvalue) :
	     dp->limit.min == UL ?
	     dp->limit.max == UL ?
	     fprintf(f, "[?-?(%d)]", dp->defaultvalue) :
	     fprintf(f, "[?-%d(%d)]", dp->limit.max, dp->defaultvalue) :
	     dp->limit.max == UL ?
	     fprintf(f, "[%d-?(%d)]", dp->limit.min, dp->defaultvalue) :
	     fprintf(f, "[%d-%d(%d)]", dp->limit.min, dp->limit.max,
		     dp->defaultvalue)) == EOF ||
	    fprintf(f," %s %s\n", dp->help, dp->wad_allowed ? "*" :"") == EOF)
	  goto error;

      // killough 11/98:
      // Write out original default if .wad file has modified the default
      
      value = dp->modified ? dp->orig_default : (int) *dp->location;

      //jff 4/10/98 kill super-hack on pointer value
      // killough 3/6/98:
      // use spaces instead of tabs for uniform justification

      if (!dp->isstr ? fprintf(f, "%-25s %5i\n", dp->name, 
			       strncmp(dp->name, "key_", 4) ? value :
			       I_DoomCode2ScanCode(value)) == EOF :
	  fprintf(f,"%-25s \"%s\"\n", dp->name, (char *) value) == EOF)
	goto error;
    }

  if (fclose(f) == EOF)
    {
    error:
      I_Error("Could not write defaults to %s: %s\n%s left unchanged\n",
	      tmpfile, errno ? strerror(errno): "(Unknown Error)",defaultfile);
      return;
    }

  remove(defaultfile);

  if (rename(tmpfile, defaultfile))
    I_Error("Could not write defaults to %s: %s\n", defaultfile,
	    errno ? strerror(errno): "(Unknown Error)");
}

//
// M_ParseOption()
//
// killough 11/98:
//
// This function parses .cfg file lines, or lines in OPTIONS lumps
//

H_boolean M_ParseOption(const char *p, H_boolean wad)
{
  char name[80], strparm[100];
  default_t *dp;
  int parm;

  while (isspace(*p))  // killough 10/98: skip leading whitespace
    p++;

  //jff 3/3/98 skip lines not starting with an alphanum
  // killough 10/98: move to be made part of main test, add comment-handling

  if (sscanf(p, "%79s %99[^\n]", name, strparm) != 2 || !isalnum(*name) ||
      !(dp = M_LookupDefault(name)) || (*strparm == '"') == !dp->isstr ||
      (wad && !dp->wad_allowed))
    return 1;

  if (dp->isstr)     // get a string default
    {
      int len = strlen(strparm)-1;

      while (isspace(strparm[len]))
        len--;

      if (strparm[len] == '"')
        len--;

      strparm[len+1] = 0;

      if (wad && !dp->modified)                       // Modified by wad
	{                                             // First time modified
	  dp->modified = 1;                           // Mark it as modified
	  dp->orig_default = *dp->location;           // Save original default
	}
      else
	free(*(char **) dp->location);                // Free old value

      *(char **) dp->location = strdup(strparm+1);    // Change default value

      if (dp->current)                                // Current value
	{
	  free(*(char **) dp->current);               // Free old value
	  *(char **) dp->current = strdup(strparm+1); // Change current value
	}
    }
  else
    {
      if (sscanf(strparm, "%i", &parm) != 1)
	return 1;                       // Not A Number

      if (!strncmp(name, "key_", 4))    // killough
	parm = I_ScanCode2DoomCode(parm);

      //jff 3/4/98 range check numeric parameters
      if ((dp->limit.min == UL || dp->limit.min <= parm) &&
	  (dp->limit.max == UL || dp->limit.max >= parm))
	{
	  if (wad)
	    {
	      if (!dp->modified)         // First time it's modified by wad
		{
		  dp->modified = 1;      // Mark it as modified
		  dp->orig_default = *dp->location;  // Save original default
		}
	      if (dp->current)           // Change current value
		*dp->current = parm;
	    }
	  *dp->location = parm;          // Change default
	}
    }

  if (wad && dp->setup_menu)
    dp->setup_menu->m_flags |= S_SKIP;

  return 0;                          // Success
}

//
// M_LoadOptions()
//
// killough 11/98:
// This function is used to load the OPTIONS lump.
// It allows wads to change game options.
//

void M_LoadOptions(void)
{
  int lump;

  if ((lump = W_CheckNumForName("OPTIONS")) != -1)
    {
      int size = W_LumpLength(lump), buflen = 0;
      char *buf = NULL, *p, *options = p = W_CacheLumpNum(lump, PU_STATIC);
      while (size > 0)
	{
	  int len = 0;
	  while (len < size && p[len++] && p[len-1] != '\n');
	  if (len >= buflen)
	    buf = realloc(buf, buflen = len+1);
	  strncpy(buf, p, len)[len] = 0;
	  p += len;
	  size -= len;
	  M_ParseOption(buf, true);
	}
      free(buf);
      Z_ChangeTag(options, PU_CACHE);
    }

  M_Trans();           // reset translucency in case of change
  M_ResetMenu();       // reset menu in case of change
}

//
// M_LoadDefaults
//

void M_LoadDefaults (void)
{
  register default_t *dp;
  int i;
  FILE *f;

  // set everything to base values
  //
  // phares 4/13/98:
  // provide default strings with their own malloced memory so that when
  // we leave this routine, that's what we're dealing with whether there
  // was a config file or not, and whether there were chat definitions
  // in it or not. This provides consistency later on when/if we need to
  // edit these strings (i.e. chat macros in the Chat Strings Setup screen).

  for (dp = defaults; dp->name; dp++)
    *dp->location =
      dp->isstr ? (int) strdup((char *) dp->defaultvalue) : dp->defaultvalue;

  // check for a custom default file

  if (!defaultfile)
    if ((i = M_CheckParm("-config")) && i < myargc-1)
      lprintf (LO_CONFIRM," default file: %s\n", defaultfile = strdup(myargv[i+1]));
    else
      defaultfile = strdup(basedefault);

  NormalizeSlashes(defaultfile);

  // read the file in, overriding any set defaults
  //
  // killough 9/21/98: Print warning if file missing, and use fgets for reading

  if (!(f = fopen(defaultfile, "r")))
    lprintf (LO_CONFIRM,"Warning: Cannot read %s -- using built-in defaults\n",defaultfile);
  else
    {
      int skipblanks = 1, line = comment = config_help_header = 0;
      char s[256];

      while (fgets(s, sizeof s, f))
        if (!M_ParseOption(s, false))
          line++;       // Line numbers
        else
          {             // Remember comment lines
            const char *p = s;

            while (isspace(*p))  // killough 10/98: skip leading whitespace
              p++;

            if (*p)                // If this is not a blank line,
              {
                skipblanks = 0;    // stop skipping blanks.
                if (strstr(p, ".cfg format:"))
                  config_help_header = 1;
              }
            else
              if (skipblanks)      // If we are skipping blanks, skip line
                continue;
              else            // Skip multiple blanks, but remember this one
                skipblanks = 1, p = "\n";

            if (comment >= comment_alloc)
              comments = realloc(comments, sizeof *comments *
                                 (comment_alloc = comment_alloc ?
                                  comment_alloc * 2 : 10));
            comments[comment].line = line;
            comments[comment++].text = strdup(p);
          }
      fclose (f);
    }

  defaults_loaded = true;            // killough 10/98

  //jff 3/4/98 redundant range checks for hud deleted here
}

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//

extern patchnum_t hu_font[HU_FONTSIZE];

int M_DrawText(int x,int y,H_boolean direct,char* string)
{
  int c;
  int w;

  while (*string)
    {
      c = toupper(*string) - HU_FONTSTART;
      string++;
      if (c < 0 || c> HU_FONTSIZE)
        {
          x += 4;
          continue;
        }

      w = SHORT (hu_font[c].width);
      if (x+w > SCREENWIDTH)
        break;

//  killough 11/98: makes no difference anymore:
//      if (direct)
//        V_DrawPatchDirect(x, y, 0, hu_font[c]);
//      else

        V_DrawPatchStretchedFromNum(x, y, 0, hu_font[c].lumpnum);
      x+=w;
    }

  return x;
}


//
// M_WriteFile
//
// killough 9/98: rewritten to use stdio and to flash disk icon

H_boolean M_WriteFile(char const *name, void *source, int length)
{
  FILE *fp;

  errno = 0;
  
  if (!(fp = fopen(name, "wb")))       // Try opening file
    return 0;                          // Could not open file for writing

  I_BeginRead();                       // Disk icon on
  length = fwrite(source, 1, length, fp) == length;   // Write data
  fclose(fp);
  I_EndRead();                         // Disk icon off

  if (!length)                         // Remove partially written file
    remove(name);

  return length;
}

//
// M_ReadFile
//
// killough 9/98: rewritten to use stdio and to flash disk icon

int M_ReadFile(char const *name, byte **buffer)
{
  FILE *fp;

  errno = 0;

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

  I_Error("Couldn't read file %s: %s", name, 
	  errno ? strerror(errno) : "(Unknown Error)");

  return 0;
}

//
// SCREEN SHOTS
//

typedef struct
{
  char     manufacturer;
  char     version;
  char     encoding;
  char     bits_per_pixel;

  unsigned short  xmin;
  unsigned short  ymin;
  unsigned short  xmax;
  unsigned short  ymax;

  unsigned short  hres;
  unsigned short  vres;

  unsigned char palette[48];

  char     reserved;
  char     color_planes;
  unsigned short  bytes_per_line;
  unsigned short  palette_type;

  char     filler[58];
  unsigned char data;   // unbounded
} pcx_t;


//
// WritePCXfile
//

static H_boolean WritePCXfile(char *filename, byte *data, int width,
                     int height, byte *palette)
{
  int    i;
  int    length;
  pcx_t* pcx;
  byte*  pack;
  H_boolean success;      // killough 10/98

  pcx = Z_Malloc(width*height*2+1000, PU_STATIC, NULL);

  pcx->manufacturer = 0x0a; // PCX id
  pcx->version = 5;         // 256 color
  pcx->encoding = 1;        // uncompressed
  pcx->bits_per_pixel = 8;  // 256 color
  pcx->xmin = 0;
  pcx->ymin = 0;
  pcx->xmax = SHORT(width-1);
  pcx->ymax = SHORT(height-1);
  pcx->hres = SHORT(width);
  pcx->vres = SHORT(height);
  memset (pcx->palette,0,sizeof(pcx->palette));
  pcx->color_planes = 1;        // chunky image
  pcx->bytes_per_line = SHORT(width);
  pcx->palette_type = SHORT(2); // not a grey scale
  memset (pcx->filler,0,sizeof(pcx->filler));

  // pack the image

  pack = &pcx->data;

  for (i = 0 ; i < width*height ; i++)
    if ( (*data & 0xc0) != 0xc0)
      *pack++ = *data++;
    else
      {
        *pack++ = 0xc1;
        *pack++ = *data++;
      }

  // write the palette

  *pack++ = 0x0c; // palette ID byte
  for (i = 0 ; i < 768 ; i++)
    *pack++ = gammatable[usegamma][*palette++];   // killough

  // write output file

  length = pack - (byte *)pcx;
  success = M_WriteFile (filename, pcx, length);  // killough 10/98

  Z_Free (pcx);

  return success;    // killough 10/98
}


// jff 3/30/98 types and data structures for BMP output of screenshots
//
// killough 5/2/98:
// Changed type names to avoid conflicts with endianess functions

typedef unsigned short uint_t;
typedef unsigned long dword_t;
typedef long     long_t;
typedef unsigned char ubyte_t;

#define BI_RGB 0L

typedef struct tagBITMAPFILEHEADER
{
  uint_t  bfType;
  dword_t bfSize;
  uint_t  bfReserved1;
  uint_t  bfReserved2;
  dword_t bfOffBits;
} __attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  dword_t biSize;
  long_t  biWidth;
  long_t  biHeight;
  uint_t  biPlanes;
  uint_t  biBitCount;
  dword_t biCompression;
  dword_t biSizeImage;
  long_t  biXPelsPerMeter;
  long_t  biYPelsPerMeter;
  dword_t biClrUsed;
  dword_t biClrImportant;
} __attribute__ ((packed)) BITMAPINFOHEADER;

// jff 3/30/98 binary file write with error detection
// killough 10/98: changed into macro to return failure instead of aborting

#define SafeWrite(data,size,number,st) do {   \
    if (fwrite(data,size,number,st) < (number)) \
   return fclose(st), I_EndRead(), false; } while(0)

//
// WriteBMPfile
// jff 3/30/98 Add capability to write a .BMP file (256 color uncompressed)
//

static H_boolean WriteBMPfile(char *filename, byte *data, int width,
                     int height, byte *palette)
{
  int i,wid;
  BITMAPFILEHEADER bmfh;
  BITMAPINFOHEADER bmih;
  int fhsiz,ihsiz;
  FILE *st;
  char zero=0;
  ubyte_t c;

  I_BeginRead();              // killough 10/98

  fhsiz = sizeof(BITMAPFILEHEADER);
  ihsiz = sizeof(BITMAPINFOHEADER);
  wid = 4*((width+3)/4);
  //jff 4/22/98 add endian macros
  bmfh.bfType = SHORT(19778);
  bmfh.bfSize = LONG(fhsiz+ihsiz+256L*4+width*height);
  bmfh.bfReserved1 = SHORT(0);
  bmfh.bfReserved2 = SHORT(0);
  bmfh.bfOffBits = LONG(fhsiz+ihsiz+256L*4);

  bmih.biSize = LONG(ihsiz);
  bmih.biWidth = LONG(width);
  bmih.biHeight = LONG(height);
  bmih.biPlanes = SHORT(1);
  bmih.biBitCount = SHORT(8);
  bmih.biCompression = LONG(BI_RGB);
  bmih.biSizeImage = LONG(wid*height);
  bmih.biXPelsPerMeter = LONG(0);
  bmih.biYPelsPerMeter = LONG(0);
  bmih.biClrUsed = LONG(256);
  bmih.biClrImportant = LONG(256);

  st = fopen(filename,"wb");
  if (st!=NULL)
    {
      // write the header
      SafeWrite(&bmfh.bfType,sizeof(bmfh.bfType),1,st);
      SafeWrite(&bmfh.bfSize,sizeof(bmfh.bfSize),1,st);
      SafeWrite(&bmfh.bfReserved1,sizeof(bmfh.bfReserved1),1,st);
      SafeWrite(&bmfh.bfReserved2,sizeof(bmfh.bfReserved2),1,st);
      SafeWrite(&bmfh.bfOffBits,sizeof(bmfh.bfOffBits),1,st);

      SafeWrite(&bmih.biSize,sizeof(bmih.biSize),1,st);
      SafeWrite(&bmih.biWidth,sizeof(bmih.biWidth),1,st);
      SafeWrite(&bmih.biHeight,sizeof(bmih.biHeight),1,st);
      SafeWrite(&bmih.biPlanes,sizeof(bmih.biPlanes),1,st);
      SafeWrite(&bmih.biBitCount,sizeof(bmih.biBitCount),1,st);
      SafeWrite(&bmih.biCompression,sizeof(bmih.biCompression),1,st);
      SafeWrite(&bmih.biSizeImage,sizeof(bmih.biSizeImage),1,st);
      SafeWrite(&bmih.biXPelsPerMeter,sizeof(bmih.biXPelsPerMeter),1,st);
      SafeWrite(&bmih.biYPelsPerMeter,sizeof(bmih.biYPelsPerMeter),1,st);
      SafeWrite(&bmih.biClrUsed,sizeof(bmih.biClrUsed),1,st);
      SafeWrite(&bmih.biClrImportant,sizeof(bmih.biClrImportant),1,st);

      // write the palette, in blue-green-red order, gamma corrected
      for (i=0;i<768;i+=3)
        {
          c=gammatable[usegamma][palette[i+2]];
          SafeWrite(&c,sizeof(char),1,st);
          c=gammatable[usegamma][palette[i+1]];
          SafeWrite(&c,sizeof(char),1,st);
          c=gammatable[usegamma][palette[i+0]];
          SafeWrite(&c,sizeof(char),1,st);
          SafeWrite(&zero,sizeof(char),1,st);
        }

      for (i = 0 ; i < height ; i++)
        SafeWrite(data+(height-1-i)*width,sizeof(byte),wid,st);

      fclose(st);
    }
  return I_EndRead(), true;       // killough 10/98
}

//
// M_ScreenShot
//
// Modified by Lee Killough so that any number of shots can be taken,
// the code is faster, and no annoying "screenshot" message appears.
//
// killough 10/98: improved error-handling

void M_ScreenShot (void)
{
#ifndef GL_DOOM
  H_boolean success = false;

  errno = 0;

  if (!access(".",2))
    {
      static int shot;
      char lbmname[PATH_MAX+1];
      int tries = 10000;

      do
        sprintf(lbmname,                         //jff 3/30/98 pcx or bmp?
                screenshot_pcx ? "doom%02d.pcx" : "doom%02d.bmp", shot++);
      while (!access(lbmname,0) && --tries);

      if (tries)
        {
          // killough 4/18/98: make palette stay around
          // (PU_CACHE could cause crash)

          byte *pal = W_CacheLumpName ("PLAYPAL", PU_STATIC);
          byte *linear = screens[2];

          I_ReadScreen(linear);

          // save the pcx file
          //jff 3/30/98 write pcx or bmp depending on mode

          // killough 10/98: detect failure and remove file if error
	  // killough 11/98: add hires support
//          if (!(success = (screenshot_pcx ? WritePCXfile : WriteBMPfile)
//                (lbmname,linear, SCREENWIDTH<<hires, SCREENHEIGHT<<hires,pal)))
          if (!(success = (screenshot_pcx ? WritePCXfile : WriteBMPfile)
                (lbmname,linear, SCREENWIDTH, SCREENHEIGHT,pal)))
	    {
	      int t = errno;
	      remove(lbmname);
	      errno = t;
	    }

          // killough 4/18/98: now you can mark it PU_CACHE
          Z_ChangeTag(pal, PU_CACHE);
        }
    }

  // 1/18/98 killough: replace "SCREEN SHOT" acknowledgement with sfx
  // players[consoleplayer].message = "screen shot"

  // killough 10/98: print error message and change sound effect if error
  S_StartSound(NULL, !success ? dprintf(errno ? strerror(errno) :
					"Could not take screenshot"), sfx_oof :
               gamemode==commercial ? sfx_radio : sfx_tink);


#endif // GL_DOOM
}

//----------------------------------------------------------------------------
//
// $Log: M_misc.c,v $
// Revision 1.2  2000/04/26 20:00:03  proff_fs
// now using SDL for video and sound output.
// sound output is currently mono only.
// Get SDL from:
// http://www.devolution.com/~slouken/SDL/
//
// Revision 1.1.1.1  2000/04/09 17:59:57  proff_fs
// Initial login
//
// Revision 1.60  1998/06/03  20:32:12  jim
// Fixed mispelling of key_chat string
//
// Revision 1.59  1998/05/21  12:12:28  jim
// Removed conditional from net code
//
// Revision 1.58  1998/05/16  09:41:15  jim
// formatted net files, installed temp switch for testing Stan/Lee's version
//
// Revision 1.57  1998/05/12  12:47:04  phares
// Removed OVER_UNDER code
//
// Revision 1.56  1998/05/05  19:56:01  phares
// Formatting and Doc changes
//
// Revision 1.55  1998/05/05  16:29:12  phares
// Removed RECOIL and OPT_BOBBING defines
//
// Revision 1.54  1998/05/03  23:05:19  killough
// Fix #includes, remove external decls duplicated elsewhere, fix LONG() conflict
//
// Revision 1.53  1998/04/23  13:07:27  jim
// Add exit line to automap
//
// Revision 1.51  1998/04/22  13:46:12  phares
// Added Setup screen Reset to Defaults
//
// Revision 1.50  1998/04/19  01:13:50  killough
// Fix freeing memory before use in savegame code
//
// Revision 1.49  1998/04/17  10:35:50  killough
// Add traditional_menu option for main menu
//
// Revision 1.48  1998/04/14  08:18:11  killough
// replace obsolete adaptive_gametic with realtic_clock_rate
//
// Revision 1.47  1998/04/13  21:36:33  phares
// Cemented ESC and F1 in place
//
// Revision 1.46  1998/04/13  12:30:02  phares
// Resolved Z_Free error msg when no boom.cfg file
//
// Revision 1.45  1998/04/12  22:55:33  phares
// Remaining 3 Setup screens
//
// Revision 1.44  1998/04/10  23:21:41  jim
// fixed string/int differentiation by value
//
// Revision 1.43  1998/04/10  06:37:54  killough
// Add adaptive gametic timer option
//
// Revision 1.42  1998/04/06  11:05:00  jim
// Remove LEESFIXES, AMAP bdg->247
//
// Revision 1.41  1998/04/06  04:50:00  killough
// Support demo_insurance=2
//
// Revision 1.40  1998/04/05  00:51:13  phares
// Joystick support, Main Menu re-ordering
//
// Revision 1.39  1998/04/03  14:45:49  jim
// Fixed automap disables at 0, mouse sens unbounded
//
// Revision 1.38  1998/03/31  10:44:31  killough
// Add demo insurance option
//
// Revision 1.37  1998/03/31  00:39:44  jim
// Screenshots in BMP format added
//
// Revision 1.36  1998/03/25  16:31:23  jim
// Fixed bad default value for defaultskill
//
// Revision 1.34  1998/03/23  15:24:17  phares
// Changed pushers to linedef control
//
// Revision 1.33  1998/03/20  00:29:47  phares
// Changed friction to linedef control
//
// Revision 1.32  1998/03/11  17:48:16  phares
// New cheats, clean help code, friction fix
//
// Revision 1.31  1998/03/10  07:06:30  jim
// Added secrets on automap after found only option
//
// Revision 1.30  1998/03/09  18:29:12  phares
// Created separately bound automap and menu keys
//
// Revision 1.29  1998/03/09  11:00:20  jim
// allowed -1 in mouse bindings and map functions
//
// Revision 1.28  1998/03/09  07:35:18  killough
// Rearrange order of cfg options, add capslock options
//
// Revision 1.27  1998/03/06  21:41:04  jim
// fixed erroneous range for gamma in config
//
// Revision 1.26  1998/03/05  00:57:47  jim
// Scattered HUD
//
// Revision 1.25  1998/03/04  11:55:42  jim
// Add range checking, help strings to BOOM.CFG
//
// Revision 1.24  1998/03/02  15:34:15  jim
// Added Rand's HELP screen as lump and loaded and displayed it
//
// Revision 1.23  1998/03/02  11:36:44  killough
// clone defaults, add sts_traditional_keys
//
// Revision 1.22  1998/02/27  19:22:05  jim
// Range checked hud/sound card variables
//
// Revision 1.21  1998/02/27  08:10:02  phares
// Added optional player bobbing
//
// Revision 1.20  1998/02/26  22:58:39  jim
// Added message review display to HUD
//
// Revision 1.19  1998/02/24  22:00:57  killough
// turn translucency back on by default
//
// Revision 1.18  1998/02/24  08:46:05  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.17  1998/02/23  14:21:14  jim
// Merged HUD stuff, fixed p_plats.c to support elevators again
//
// Revision 1.16  1998/02/23  04:40:48  killough
// Lots of new options
//
// Revision 1.14  1998/02/20  21:57:00  phares
// Preliminarey sprite translucency
//
// Revision 1.13  1998/02/20  18:46:58  jim
// cleanup of HUD control
//
// Revision 1.12  1998/02/19  16:54:33  jim
// Optimized HUD and made more configurable
//
// Revision 1.11  1998/02/18  11:56:11  jim
// Fixed issues with HUD and reduced screen size
//
// Revision 1.9  1998/02/15  03:21:20  phares
// Jim's comment: Fixed bug in automap from mistaking framebuffer index for mark color
//
// Revision 1.8  1998/02/15  03:17:56  phares
// User-defined keys
//
// Revision 1.6  1998/02/09  03:04:12  killough
// Add weapon preferences, player corpse, vsync options
//
// Revision 1.5  1998/02/02  13:37:26  killough
// Clone compatibility flag, for TNTCOMP to work
//
// Revision 1.4  1998/01/26  19:23:49  phares
// First rev with no ^Ms
//
// Revision 1.3  1998/01/26  04:59:07  killough
// Fix DOOM 1 screenshot acknowledgement
//
// Revision 1.2  1998/01/21  16:56:16  jim
// Music fixed, defaults for cards added
//
// Revision 1.1.1.1  1998/01/19  14:02:57  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------

