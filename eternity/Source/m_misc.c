// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//  Main loop menu stuff.
//  Default Config File.
//  PCX Screenshots.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: m_misc.c,v 1.60 1998/06/03 20:32:12 jim Exp $";

#include "doomstat.h"
#include "m_argv.h"
#include "g_game.h"
#include "mn_engin.h"
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
#include "r_sky.h" // haleyjd
#include "r_draw.h"
#include "c_io.h"
#include "c_net.h"
#include "d_io.h"  // SoM 3/12/2002: moved unistd stuff into d_io.h
#include "p_partcl.h"
#include "r_things.h"
#include "d_gi.h"

#include <errno.h>

//
// DEFAULTS
//

static int config_help;         //jff 3/3/98
int usemouse;
int usejoystick;
int screenshot_pcx; //jff 3/30/98 // option to output screenshot as pcx or bmp
// [obsolete] extern int mousebfire; 
extern int mousebstrafe;
extern int mousebforward;
// [obsolete] extern int joybfire;
// [obsolete] extern int joybstrafe;
// [obsolete] extern int joybuse;
// [obsolete] extern int joybspeed;
extern int viewwidth;
extern int viewheight;
extern int mouseSensitivity_horiz,mouseSensitivity_vert;  // killough
extern int realtic_clock_rate;         // killough 4/13/98: adjustable timer
extern int leds_always_off;            // killough 3/6/98
extern int tran_filter_pct;            // killough 2/21/98
extern int showMessages;
extern int screenSize;

extern char *chat_macros[], *wad_files[], *deh_files[];  // killough 10/98
extern char *csc_files[];   // haleyjd: auto-loaded console scripts

extern int hud_msg_timer;   // killough 11/98: timer used for review messages
extern int hud_msg_lines;   // number of message lines in window up to 16
extern int hud_msg_scrollup;// killough 11/98: whether message list scrolls up
extern int hud_overlaystyle;  // sf: overlay style
extern int hud_enabled;       // sf: fullscreen hud on/off
extern int hud_hidestatus;      // sf
extern int message_timer;   // killough 11/98: timer used for normal messages
extern int show_scores;
extern int show_vpo;

extern int textmode_startup;

//jff 3/3/98 added min, max, and help string to all entries
//jff 4/10/98 added isstr field to specify whether value is string or int
//
// killough 11/98: entirely restructured to allow options to be modified
// from wads, and to consolidate with menu code

default_t defaults[] = {
  { //jff 3/3/98
    "config_help",
    &config_help, NULL,
    1, {0,1}, dt_number, ss_none, wad_no,
    "1 to show help strings about each variable in config file"
  },

  {
    "colour",
    &default_colour, NULL,
    0, {0,TRANSLATIONCOLOURS}, dt_number, ss_none, wad_no,
    "the default player colour (green, indigo,brown, red)"
  },

  {
    "name",
    (int*)&default_name, NULL,
    (int) "player", {0}, dt_string, ss_none, wad_no,
    "the default player name"
  },

  { // jff 3/24/98 allow default skill setting
    "default_skill",
    &defaultskill, NULL,
    3, {1,5}, dt_number, ss_none, wad_no,
    "selects default skill 1=TYTD 2=NTR 3=HMP 4=UV 5=NM"
  },

  // haleyjd: fixed for cross-platform support -- see m_misc.h
  { // jff 1/18/98 allow Allegro drivers to be set,  -1 = autodetect
    "sound_card",
    &snd_card, NULL,
    SND_DEFAULT, {SND_MIN,SND_MAX}, dt_number, ss_gen, wad_no,
    SND_DESCR
  },

  {
    "music_card",
    &mus_card, NULL,
    MUS_DEFAULT, {MUS_MIN,MUS_MAX}, dt_number, ss_gen, wad_no,
    MUS_DESCR
  },

#ifdef _SDL_VER
    // haleyjd 04/15/02: SDL joystick device number
    {
       "joystick_num",
       &i_SDLJoystickNum, NULL,
       -1, {-1,UL}, dt_number, ss_none, wad_no,
       "SDL joystick device number, -1 to disable"
    },
    
    // joystick sensitivities
    {
       "joystickSens_x",
       &joystickSens_x, NULL,
       0, {-32768, 32767}, dt_number, ss_none, wad_no,
       "SDL joystick horizontal sensitivity"
    },

    {
       "joystickSens_y",
       &joystickSens_y, NULL,
       0, {-32768, 32767}, dt_number, ss_none, wad_no,
       "SDL joystick vertical sensitivity"
    },
#endif

  {
    "s_precache",
    &s_precache, NULL,
    0, {0,1}, dt_number, ss_gen, wad_no,
    "precache sounds at startup"
  },
#ifdef DJGPP
  { // jff 3/4/98 detect # voices
    "detect_voices",
    &detect_voices, NULL,
    1, {0,1}, dt_number, ss_gen, wad_no,
    "1 enables voice detection prior to calling install sound"
  },
#endif
  {
    "v_mode",
    &v_mode, NULL,
    0, {0,32}, dt_number, ss_gen, wad_no,
    "graphics mode"
  },

  {
    "textmode_startup",
    &textmode_startup, NULL,
    0, {0,1}, dt_number, ss_gen, wad_no,
    "start up ETERNITY in text mode"
  },

  {
    "use_vsync",
    &use_vsync, NULL,
    1, {0,1}, dt_number, ss_gen, wad_no,
    "1 to enable wait for vsync to avoid display tearing"
  },

  {
    "realtic_clock_rate",
    &realtic_clock_rate, NULL,
    100, {10,1000}, dt_number, ss_gen, wad_no,
    "Percentage of normal speed (35 fps) realtic clock runs at"
  },
#ifdef DJGPP
  { // killough 10/98
    "disk_icon",
    &disk_icon, NULL,
    1, {0,1}, dt_number, ss_gen, wad_no,
    "1 to enable flashing icon during disk IO"
  },
#endif
  { // killough 2/21/98
    "pitched_sounds",
    &pitched_sounds, NULL,
    0, {0,1}, dt_number, ss_gen, wad_yes,
    "1 to enable variable pitch in sound effects (from id's original code)"
  },

  { // phares
    "translucency",
    &general_translucency, NULL,
    1, {0,1}, dt_number, ss_gen, wad_yes,
    "1 to enable translucency for some things"
  },

  { // killough 2/21/98
    "tran_filter_pct",
    &tran_filter_pct, NULL,
    66, {0,100}, dt_number, ss_gen, wad_yes,
    "set percentage of foreground/background translucency mix"
  },

  { // killough 2/8/98
    "max_player_corpse",
    &default_bodyquesize, NULL,
    32, {UL,UL},dt_number, ss_gen, wad_no,
    "number of dead bodies in view supported (negative value = no limit)"
  },

  {  // haleyjd: changed default to 0
    "show_vpo",
    &show_vpo, NULL,
    0, {0,1}, dt_number, ss_gen, wad_no,
    "1 to enable VPO warning indicator"
  },

  { // killough 10/98
    "flashing_hom",
    &flashing_hom, NULL,
    1, {0,1}, dt_number, ss_gen, wad_no,
    "1 to enable flashing HOM indicator"
  },

  { // killough 3/31/98
    "demo_insurance",
    &default_demo_insurance, NULL,
    2, {0,2},dt_number, ss_none, wad_no,
    "1=take special steps ensuring demo sync, 2=only during recordings"
  },

  { // phares
    "weapon_recoil",
    &default_weapon_recoil, &weapon_recoil,
    0, {0,1}, dt_number, ss_weap, wad_yes,
    "1 to enable recoil from weapon fire"
  },

  { // killough 7/19/98         // sf:changed to bfgtype
    "bfgtype",
    (int*)&default_bfgtype, (int*)&bfgtype,
    0, {0,4}, dt_number, ss_weap, wad_yes,
    "0 - normal, 1 - classic, 2 - 11k, 3 - bouncing!, 4 - burst"
  },

  {             //sf
    "crosshair",
    &crosshairnum, NULL,
    0, {0,CROSSHAIRS}, dt_number, ss_gen, wad_yes,
    "0 - none, 1 - cross, 2 - angle"
  },

  {             // sf
    "show_scores",
    &show_scores, NULL,
    0, {0,1}, dt_number, ss_gen, wad_yes,
    "show scores in deathmatch"
  },

  {
     "lefthanded",
     &lefthanded, NULL,
     0, {0,1}, dt_number, ss_gen, wad_yes,
     "0 - right handed, 1 - left handed"
  },

  { // killough 10/98
    "doom_weapon_toggles",
    &doom_weapon_toggles, NULL,
    1, {0,1}, dt_number, ss_weap, wad_no,
    "1 to toggle between SG/SSG and Fist/Chainsaw"
  },

  { // phares 2/25/98
    "player_bobbing",
    &default_player_bobbing, &player_bobbing,
    1, {0,1}, dt_number, ss_weap, wad_yes,
    "1 to enable player bobbing (view moving up/down slightly)"
  },

  { // killough 3/1/98
    "monsters_remember",
    &default_monsters_remember, &monsters_remember,
    1, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable monsters remembering enemies after killing others"
  },

  { // killough 7/19/98
    "monster_infighting",
    &default_monster_infighting, &monster_infighting,
    1, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable monsters fighting against each other when provoked"
  },

  { // killough 9/8/98
    "monster_backing",
    &default_monster_backing, &monster_backing,
    0, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable monsters backing away from targets"
  },

  { //killough 9/9/98:
    "monster_avoid_hazards",
    &default_monster_avoid_hazards, &monster_avoid_hazards,
    1, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable monsters to intelligently avoid hazards"
  },

  {
    "monkeys",
    &default_monkeys, &monkeys,
    0, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable monsters to move up/down steep stairs"
  },

  { //killough 9/9/98:
    "monster_friction",
    &default_monster_friction, &monster_friction,
    1, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable monsters to be affected by friction"
  },

  { //killough 9/9/98:
    "help_friends",
    &default_help_friends, &help_friends,
    1, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable monsters to help dying friends"
  },


  { // killough 7/19/98
    "player_helpers",
    &default_dogs, &dogs,
    0, {0,3}, dt_number, ss_enem, wad_yes,
    "number of single-player helpers"
  },

  { // killough 8/8/98
    "friend_distance",
    &default_distfriend, &distfriend,
    128, {0,999}, dt_number, ss_enem, wad_yes,
    "distance friends stay away"
  },

  { // killough 10/98
    "dog_jumping",
    &default_dog_jumping, &dog_jumping,
    1, {0,1}, dt_number, ss_enem, wad_yes,
    "1 to enable dogs to jump"
  },

  { // no color changes on status bar
    "sts_always_red",
    &sts_always_red, NULL,
    1, {0,1}, dt_number, ss_stat, wad_yes,
    "1 to disable use of color on status bar"
  },

  {
    "sts_pct_always_gray",
    &sts_pct_always_gray, NULL,
    0, {0,1}, dt_number, ss_stat, wad_yes,
    "1 to make percent signs on status bar always gray"
  },

  { // killough 2/28/98
    "sts_traditional_keys",
    &sts_traditional_keys, NULL,
    1, {0,1}, dt_number, ss_stat, wad_yes,
    "1 to disable doubled card and skull key display on status bar"
  },
/*
  { // killough 4/17/98
    "traditional_menu",
    &traditional_menu, NULL,
    1, {0,1}, dt_number, ss_none, wad_yes,
    "1 to use Doom's main menu ordering"
  },
*/
  { // killough 3/6/98
    "leds_always_off",
    &leds_always_off, NULL,
    0, {0,1}, dt_number, ss_gen, wad_no,
    "1 to keep keyboard LEDs turned off"
  },

  { //jff 4/3/98 allow unlimited sensitivity
    "mouse_sensitivity_horiz",
    &mouseSensitivity_horiz, NULL,
    5, {0,UL}, dt_number, ss_none, wad_no,
    "adjust horizontal (x) mouse sensitivity"
  },

  { //jff 4/3/98 allow unlimited sensitivity
    "mouse_sensitivity_vert",
    &mouseSensitivity_vert, NULL,
    5, {0,UL}, dt_number, ss_none, wad_no,
    "adjust vertical (y) mouse sensitivity"
  },

  {
    "sfx_volume",
    &snd_SfxVolume, NULL,
    8, {0,15}, dt_number, ss_none, wad_no,
    "adjust sound effects volume"
  },

  {
    "music_volume",
    &snd_MusicVolume, NULL,
    8, {0,15}, dt_number, ss_none, wad_no,
    "adjust music volume"
  },

  { // haleyjd 12/08/01
    "force_flip_pan",
    &forceFlipPan, NULL,
    0, {0, 1}, dt_number, ss_none, wad_no,
    "forces reversal of audio channels: 0 = normal, 1 = reverse"
  },

  {
    "show_messages",
    &showMessages, NULL,
    1, {0,1}, dt_number, ss_none, wad_no,
    "1 to enable message display"
  },

  {
    "mess_colour",
    &mess_colour, NULL,
    CR_RED, {0,CR_LIMIT-1}, dt_number, ss_none, wad_no,
    "messages colour"
  },

  { // killough 3/6/98: preserve autorun across games
    "autorun",
    &autorun, NULL,
    0, {0,1}, dt_number, ss_none, wad_no,
    "1 to enable autorun"
  },

  { // killough 2/21/98: default to 10
    // sf: removed screenblocks, screensize only now
    // changed values down 3
    "screensize",
    &screenSize, NULL,
    7, {0,8}, dt_number, ss_none, wad_no,
    "initial play screen size"
  },

  { //jff 3/6/98 fix erroneous upper limit in range
    "usegamma",
    &usegamma, NULL,
    0, {0,4}, dt_number, ss_none, wad_no,
    "screen brightness (gamma correction)"
  },

  { // killough 10/98: preloaded files
    "wadfile_1",
    (int *) &wad_files[0], NULL,
    (int) "", {0}, dt_string, ss_none, wad_no,
    "WAD file preloaded at program startup"
  },

  {
    "wadfile_2",
    (int *) &wad_files[1], NULL,
    (int) "", {0}, dt_string, ss_none, wad_no,
    "WAD file preloaded at program startup"
  },

  {
    "dehfile_1",
    (int *) &deh_files[0], NULL,
    (int) "", {0}, dt_string, ss_none, wad_no,
    "DEH/BEX file preloaded at program startup"
  },

  {
    "dehfile_2",
    (int *) &deh_files[1], NULL,
    (int) "", {0}, dt_string, ss_none, wad_no,
    "DEH/BEX file preloaded at program startup"
  },

  // haleyjd: auto-loaded console scripts
  {
    "cscript_1",
    (int *) &csc_files[0], NULL,
    (int) "", {0}, dt_string, ss_none, wad_no,
    "Console script executed at program startup"
  },
  
  {
    "cscript_2",
    (int *) &csc_files[1], NULL,
    (int) "", {0}, dt_string, ss_none, wad_no,
    "Console script executed at program startup"
  },
  
  {
    "use_startmap",
    &use_startmap, NULL,
    -1, {-1, 1}, dt_number, ss_comp, wad_yes,
    "use start map instead of menu"
  },

  // killough 10/98: compatibility vector:

  {
    "comp_zombie",
    &default_comp[comp_zombie], &comp[comp_zombie],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Zombie players can exit levels"
  },

  {
    "comp_infcheat",
    &default_comp[comp_infcheat], &comp[comp_infcheat],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Powerup cheats are not infinite duration"
  },

  {
    "comp_stairs",
    &default_comp[comp_stairs], &comp[comp_stairs],
    1, {0,1}, dt_number, ss_comp, wad_yes,
    "Build stairs exactly the same way that Doom does"
  },

  {
    "comp_telefrag",
    &default_comp[comp_telefrag], &comp[comp_telefrag],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Monsters can telefrag on MAP30"
  },

  {
    "comp_dropoff",
    &default_comp[comp_dropoff], &comp[comp_dropoff],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Some objects never move over tall ledges"
  },

  {
    "comp_falloff",
    &default_comp[comp_falloff], &comp[comp_falloff],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Objects don't fall off ledges under their own weight"
  },

  {
    "comp_staylift",
    &default_comp[comp_staylift], &comp[comp_staylift],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Monsters randomly walk off of moving lifts"
  },

  {
    "comp_doorstuck",
    &default_comp[comp_doorstuck], &comp[comp_doorstuck],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Monsters get stuck on doortracks"
  },

  {
    "comp_pursuit",
    &default_comp[comp_pursuit], &comp[comp_pursuit],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Monsters don't give up pursuit of targets"
  },

  {
    "comp_vile",
    &default_comp[comp_vile], &comp[comp_vile],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Arch-Vile resurrects invincible ghosts"
  },

  {
    "comp_pain",
    &default_comp[comp_pain], &comp[comp_pain],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Pain Elemental limited to 20 lost souls"
  },

  {
    "comp_skull",
    &default_comp[comp_skull], &comp[comp_skull],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Lost souls get stuck behind walls"
  },

  {
    "comp_blazing",
    &default_comp[comp_blazing], &comp[comp_blazing],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Blazing doors make double closing sounds"
  },

  {
    "comp_doorlight",
    &default_comp[comp_doorlight], &comp[comp_doorlight],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Tagged doors don't trigger special lighting"
  },

  {
    "comp_god",
    &default_comp[comp_god], &comp[comp_god],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "God mode isn't absolute"
  },

  {
    "comp_skymap",
    &default_comp[comp_skymap], &comp[comp_skymap],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Sky is unaffected by invulnerability"
  },

  {
    "comp_floors",
    &default_comp[comp_floors], &comp[comp_floors],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Use exactly Doom's floor motion behavior"
  },

  {
    "comp_model",
    &default_comp[comp_model], &comp[comp_model],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Use exactly Doom's linedef trigger model"
  },

  {
    "comp_zerotags",
    &default_comp[comp_zerotags], &comp[comp_zerotags],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Linedef effects work with sector tag = 0"
  },

  {
    "comp_terrain", // haleyjd
    &default_comp[comp_terrain], &comp[comp_terrain],
    1, {0,1}, dt_number, ss_comp, wad_yes,
    "Terrain effects not activated on floor contact"
  },
  
  {
    "comp_respawnfix", // haleyjd
    &default_comp[comp_respawnfix], &comp[comp_respawnfix],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Creatures with no spawnpoint respawn at (0,0)"
  },
  
  {
    "comp_fallingdmg", // haleyjd
    &default_comp[comp_fallingdmg], &comp[comp_fallingdmg],
    1, {0,1}, dt_number, ss_comp, wad_yes,
    "Players do not take falling damage"
  },

  {
    "comp_soul", // haleyjd
    &default_comp[comp_soul], &comp[comp_soul],
    0, {0,1}, dt_number, ss_comp, wad_yes,
    "Lost souls do not bounce on floors"
  },

  // haleyjd 02/15/02: z checks (includes,supercedes comp_scratch)
  {
    "comp_overunder",
    &default_comp[comp_overunder], &comp[comp_overunder],
    1, {0,1}, dt_number, ss_comp, wad_yes,
    "Things not fully clipped with respect to z coord"
  },

  // For key bindings, the values stored in the key_* variables       // phares
  // are the internal Doom Codes. The values stored in the default.cfg
  // file are the keyboard codes. I_ScanCode2DoomCode converts from
  // keyboard codes to Doom Codes. I_DoomCode2ScanCode converts from
  // Doom Codes to keyboard codes, and is only used when writing back
  // to default.cfg. For the printable keys (i.e. alphas, numbers)
  // the Doom Code is the ascii code.

  // haleyjd: This now only sets keys which are not dynamically
  // rebindable for various reasons. To not allow rebinding at all
  // would be bad, but to do it in real time would be hard. This is
  // a compromise.

  { // phares 3/7/98
    "key_menu_right",
    &key_menu_right, NULL,
    KEYD_RIGHTARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to move right in a menu"
  },
  {
    "key_menu_left",
    &key_menu_left, NULL,
    KEYD_LEFTARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to move left in a menu"
  },

  {
    "key_menu_up",
    &key_menu_up, NULL,
    KEYD_UPARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to move up in a menu"
  },

  {
    "key_menu_down",
    &key_menu_down, NULL,
    KEYD_DOWNARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to move down in a menu"
  },

  {
    "key_menu_backspace",
    &key_menu_backspace, NULL,
    KEYD_BACKSPACE, {0,255}, dt_number, ss_keys, wad_no,
    "key to erase last character typed in a menu"
  },

  {
    "key_menu_escape",
    &key_menu_escape, NULL,
    KEYD_ESCAPE, {0,255}, dt_number, ss_keys, wad_no,
    "key to leave a menu"
  }, // phares 3/7/98

  {
    "key_menu_enter",
    &key_menu_enter, NULL,
    KEYD_ENTER, {0,255}, dt_number, ss_keys, wad_no,
    "key to select from menu or review past messages"
  },

  {
    "key_spy",
    &key_spy, NULL,
    KEYD_F12, {0,255}, dt_number, ss_keys, wad_no,
    "key to view from another player's vantage"
  },

  {
    "key_pause",
    &key_pause, NULL,
    KEYD_PAUSE, {0,255}, dt_number, ss_keys, wad_no,
    "key to pause the game"
  },

  {
    "key_autorun",
    &key_autorun, NULL,
    KEYD_CAPSLOCK, {0,255}, dt_number, ss_keys, wad_no,
    "key to toggle always run mode"
  },

  {
    "key_chat",
    &key_chat, NULL,
    't', {0,255}, dt_number, ss_keys, wad_no,
    "key to enter a chat message"
  },

  {
    "key_backspace",
    &key_backspace, NULL,
    KEYD_BACKSPACE, {0,255}, dt_number, ss_keys, wad_no,
    "key to erase last character typed"
  },

  {
    "key_enter",
    &key_enter, NULL,
    KEYD_ENTER, {0,255}, dt_number, ss_keys, wad_no,
    "key to select from menu or review past messages"
  },

  {
    "key_map",
    &key_map, NULL,
    KEYD_TAB, {0,255}, dt_number, ss_keys, wad_no,
    "key to toggle automap display"
  },

  { // phares 3/7/98
    "key_map_right",
    &key_map_right, NULL,
    KEYD_RIGHTARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to shift automap right"
  },

  {
    "key_map_left",
    &key_map_left, NULL,
    KEYD_LEFTARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to shift automap left"
  },

  {
    "key_map_up",
    &key_map_up, NULL,
    KEYD_UPARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to shift automap up"
  },

  {
    "key_map_down",
    &key_map_down, NULL,
    KEYD_DOWNARROW, {0,255}, dt_number, ss_keys, wad_no,
    "key to shift automap down"
  },

  {
    "key_map_zoomin",
    &key_map_zoomin, NULL,
    '=', {0,255}, dt_number, ss_keys, wad_no,
    "key to enlarge automap"
  },

  {
    "key_map_zoomout",
    &key_map_zoomout, NULL,
    '-', {0,255}, dt_number, ss_keys, wad_no,
    "key to reduce automap"
  },

  {
    "key_map_gobig",
    &key_map_gobig, NULL,
    '0', {0,255}, dt_number, ss_keys, wad_no,
    "key to get max zoom for automap"
  },

  {
    "key_map_follow",
    &key_map_follow, NULL,
    'f', {0,255}, dt_number, ss_keys, wad_no,
    "key to toggle scrolling/moving with automap"
  },

  {
    "key_map_mark",
    &key_map_mark, NULL,
    'm', {0,255}, dt_number, ss_keys, wad_no,
    "key to drop a marker on automap"
  },

  {
    "key_map_clear",
    &key_map_clear, NULL,
    'c', {0,255}, dt_number, ss_keys, wad_no,
    "key to clear all markers on automap"
  },

  {
    "key_map_grid",
    &key_map_grid, NULL,
    'g', {0,255}, dt_number, ss_keys, wad_no,
    "key to toggle grid display over automap"
  },

  {
    "key_chatplayer1",
    &destination_keys[0], NULL,
    'g', {0,255}, dt_number, ss_keys, wad_no,
    "key to chat with player 1"
  },

  { // killough 11/98: fix 'i'/'b' reversal
    "key_chatplayer2",
    &destination_keys[1], NULL,
    'i', {0,255}, dt_number, ss_keys, wad_no,
    "key to chat with player 2"
  },

  {  // killough 11/98: fix 'i'/'b' reversal
    "key_chatplayer3",
    &destination_keys[2], NULL,
    'b', {0,255}, dt_number, ss_keys, wad_no,
    "key to chat with player 3"
  },

  {
    "key_chatplayer4",
    &destination_keys[3], NULL,
    'r', {0,255}, dt_number, ss_keys, wad_no,
    "key to chat with player 4"
  },

  { // killough 2/22/98: screenshot key
    "key_screenshot",
    &key_screenshot, NULL,
    '*', {0,255}, dt_number, ss_keys, wad_no,
    "key to take a screenshot (devparm independent)"
  },

  { // HOME key  // killough 10/98: shortcut to setup menu
    "key_setup",
    &key_setup, NULL,
    199, {0,255}, dt_number, ss_keys, wad_no,
    "shortcut key to enter setup menu"
  },

  {
    "automlook",
    &automlook, NULL,
    0, {0,1}, dt_number, ss_gen, wad_no,
    "set to 1 to always mouselook"
  },

  {
    "invert_mouse",
    &invert_mouse, NULL,
    1, {0,1}, dt_number, ss_gen, wad_no,
    "set to 1 to invert mouse during mouselooking"
  },

  { // jff 3/30/98 add ability to take screenshots in BMP format
    "screenshot_pcx",
    &screenshot_pcx, NULL,
    1, {0,1}, dt_number, ss_gen, wad_no,
    "1 to take a screenshot in PCX format, 0 for BMP"
  },

  {
    "use_mouse",
    &usemouse, NULL,
    1, {0,1}, dt_number, ss_gen, wad_no,
    "1 to enable use of mouse"
  },
/*
  haleyjd: obsolete
  { //jff 3/8/98 allow -1 in mouse bindings to disable mouse function
    "mouseb_fire",
    &mousebfire, NULL,
    0, {-1,2}, dt_number, ss_keys, wad_no,
    "mouse button dt_number to use for fire (-1 = disable)"
  },
*/
  // haleyjd: rename these buttons on the user-side to prevent
  // confusion
  {
    "mouseb_dblc1",
    &mousebstrafe, NULL,
    1, {-1,2}, dt_number, ss_keys, wad_no,
    "1st mouse button to enable for double-click use action (-1 = disable)"
  },

  {
    "mouseb_dblc2",
    &mousebforward, NULL,
    2, {-1,2}, dt_number, ss_keys, wad_no,
    "2nd mouse button to enable for double-click use action (-1 = disable)"
  }, //jff 3/8/98 end of lower range change for -1 allowed in mouse binding

  {
    "use_joystick",
    &usejoystick, NULL,
    0, {0,1}, dt_number, ss_gen, wad_no,
    "1 to enable use of joystick"
  },
/*
  haleyjd: joyb* variables are obsolete
  {
    "joyb_fire",
    &joybfire, NULL,
    0, {0,UL}, dt_number, ss_keys, wad_no,
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
*/
  { // killough
    "snd_channels",
    &default_numChannels, NULL,
    32, {1, 128}, 0, ss_gen, wad_no,
    "number of sound effects handled simultaneously"
  },

  {
    "chatmacro0",
    (int *) &chat_macros[0], NULL,
    (int) HUSTR_CHATMACRO0, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 0 key"
  },

  {
    "chatmacro1",
    (int *) &chat_macros[1], NULL,
    (int) HUSTR_CHATMACRO1, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 1 key"
  },

  {
    "chatmacro2",
    (int *) &chat_macros[2], NULL,
    (int) HUSTR_CHATMACRO2, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 2 key"
  },

  {
    "chatmacro3",
    (int *) &chat_macros[3], NULL,
    (int) HUSTR_CHATMACRO3, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 3 key"
  },

  {
    "chatmacro4",
    (int *) &chat_macros[4], NULL,
    (int) HUSTR_CHATMACRO4, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 4 key"
  },

  {
    "chatmacro5",
    (int *) &chat_macros[5], NULL,
    (int) HUSTR_CHATMACRO5, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 5 key"
  },

  {
    "chatmacro6",
    (int *) &chat_macros[6], NULL,
    (int) HUSTR_CHATMACRO6, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 6 key"
  },

  {
    "chatmacro7",
    (int *) &chat_macros[7], NULL,
    (int) HUSTR_CHATMACRO7, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 7 key"
  },

  {
    "chatmacro8",
    (int *) &chat_macros[8], NULL,
    (int) HUSTR_CHATMACRO8, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 8 key"
  },

  {
    "chatmacro9",
    (int *) &chat_macros[9], NULL,
    (int) HUSTR_CHATMACRO9, {0}, dt_string, ss_chat, wad_yes,
    "chat string associated with 9 key"
  },

  //jff 1/7/98 defaults for automap colors
  //jff 4/3/98 remove -1 in lower range, 0 now disables new map features
  { // black //jff 4/6/98 new black
    "mapcolor_back",
    &mapcolor_back, NULL,
    247, {0,255}, dt_number, ss_auto, wad_yes,
    "color used as background for automap"
  },

  {  // dk gray
    "mapcolor_grid",
    &mapcolor_grid, NULL,
    104, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for automap grid lines"
  },

  { // red-brown
    "mapcolor_wall",
    &mapcolor_wall, NULL,
    181, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for one side walls on automap"
  },

  { // lt brown
    "mapcolor_fchg",
    &mapcolor_fchg, NULL,
    166, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for lines floor height changes across"
  },

  { // orange
    "mapcolor_cchg",
    &mapcolor_cchg, NULL,
    231, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for lines ceiling height changes across"
  },

  { // white
    "mapcolor_clsd",
    &mapcolor_clsd, NULL,
    231, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for lines denoting closed doors, objects"
  },

  { // red
    "mapcolor_rkey",
    &mapcolor_rkey, NULL,
    175, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for red key sprites"
  },

  { // blue
    "mapcolor_bkey",
    &mapcolor_bkey, NULL,
    204, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for blue key sprites"
  },

  { // yellow
    "mapcolor_ykey",
    &mapcolor_ykey, NULL,
    231, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for yellow key sprites"
  },

  { // red
    "mapcolor_rdor",
    &mapcolor_rdor, NULL,
    175, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for closed red doors"
  },

  { // blue
    "mapcolor_bdor",
    &mapcolor_bdor, NULL,
    204, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for closed blue doors"
  },

  { // yellow
    "mapcolor_ydor",
    &mapcolor_ydor, NULL,
    231, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for closed yellow doors"
  },

  { // dk green
    "mapcolor_tele",
    &mapcolor_tele, NULL,
    119, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for teleporter lines"
  },

  { // purple
    "mapcolor_secr",
    &mapcolor_secr, NULL,
    176, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for lines around secret sectors"
  },

  { // none
    "mapcolor_exit",
    &mapcolor_exit, NULL,
    0, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for exit lines"
  },

  { // dk gray
    "mapcolor_unsn",
    &mapcolor_unsn, NULL,
    96, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for lines not seen without computer map"
  },

  { // lt gray
    "mapcolor_flat",
    &mapcolor_flat, NULL,
    88, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for lines with no height changes"
  },

  { // green
    "mapcolor_sprt",
    &mapcolor_sprt, NULL,
    112, {0,255}, dt_number, ss_auto, wad_yes,
    "color used as things"
  },

  { // white
    "mapcolor_hair",
    &mapcolor_hair, NULL,
    208, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for dot crosshair denoting center of map"
  },

  { // white
    "mapcolor_sngl",
    &mapcolor_sngl, NULL,
    208, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for the single player arrow"
  },

  { // green
    "mapcolor_ply1",
    &mapcolor_plyr[0], NULL,
    112, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for the green player arrow"
  },

  { // lt gray
    "mapcolor_ply2",
    &mapcolor_plyr[1], NULL,
    88, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for the gray player arrow"
  },

  { // brown
    "mapcolor_ply3",
    &mapcolor_plyr[2], NULL,
    64, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for the brown player arrow"
  },

  { // red
    "mapcolor_ply4",
    &mapcolor_plyr[3], NULL,
    176, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for the red player arrow"
  },

  {  // purple                     // killough 8/8/98
    "mapcolor_frnd",
    &mapcolor_frnd, NULL,
    252, {0,255}, dt_number, ss_auto, wad_yes,
    "color used for friends"
  },

  {
    "map_point_coord",
    &map_point_coordinates, NULL,
    1, {0,1}, dt_number, ss_auto, wad_yes,
    "1 to show automap pointer coordinates in non-follow mode"
  },

  //jff 3/9/98 add option to not show secrets til after found
  // killough change default, to avoid spoilers and preserve Doom mystery
  { // show secret after gotten
    "map_secret_after",
    &map_secret_after, NULL,
    1, {0,1}, dt_number, ss_auto, wad_yes,
    "1 to not show secret sectors till after entered"
  },

  //jff 1/7/98 end additions for automap

  //jff 2/16/98 defaults for color ranges in hud and status

  { // 1 line scrolling window
    "hud_msg_lines",
    &hud_msg_lines, NULL,
    1, {1,16}, dt_number, ss_mess, wad_yes,
    "number of lines in review display"
  },

  { // killough 11/98
    "hud_msg_scrollup",
    &hud_msg_scrollup, NULL,
    1, {0,1}, dt_number, ss_mess, wad_yes,
    "1 enables message review list scrolling upward"
  },

  { // killough 11/98
    "message_timer",
    &message_timer, NULL,
    4000, {0,UL}, 0, ss_mess, wad_no,
    "Duration of normal Doom messages (ms)"
  },

  {     //sf : fullscreen hud style
    "hud_overlaystyle",
    &hud_overlaystyle, NULL,
    1, {0,3}, 0, ss_mess, wad_yes,
    "fullscreen hud style"
  },

  {
    "hud_enabled",
    &hud_enabled, NULL,
    1, {0,1}, 0, ss_mess, wad_yes,
    "fullscreen hud enabled"
  },

  {
    "hud_hidestatus",
    &hud_hidestatus, NULL,
    0, {0,1}, 0, ss_mess, wad_yes,
    "hide kills/items/secrets info on fullscreen hud"
  },

  { // below is red
    "health_red",
    &health_red, NULL,
    25, {0,200}, dt_number, ss_stat, wad_yes,
    "amount of health for red to yellow transition"
  },

  { // below is yellow
    "health_yellow",
    &health_yellow, NULL,
    50, {0,200}, dt_number, ss_stat, wad_yes,
    "amount of health for yellow to green transition"
  },

  { // below is green, above blue
    "health_green",
    &health_green, NULL,
    100, {0,200}, dt_number, ss_stat, wad_yes,
    "amount of health for green to blue transition"
  },

  { // below is red
    "armor_red",
    &armor_red, NULL,
    25, {0,200}, dt_number, ss_stat, wad_yes,
    "amount of armor for red to yellow transition"
  },

  { // below is yellow
    "armor_yellow",
    &armor_yellow, NULL,
    50, {0,200}, dt_number, ss_stat, wad_yes,
    "amount of armor for yellow to green transition"
  },

  { // below is green, above blue
    "armor_green",
    &armor_green, NULL,
    100, {0,200}, dt_number, ss_stat, wad_yes,
    "amount of armor for green to blue transition"
  },

  { // below 25% is red
    "ammo_red",
    &ammo_red, NULL,
    25, {0,100}, dt_number, ss_stat, wad_yes,
    "percent of ammo for red to yellow transition"
  },

  { // below 50% is yellow, above green
    "ammo_yellow",
    &ammo_yellow, NULL,
    50, {0,100}, dt_number, ss_stat, wad_yes,
    "percent of ammo for yellow to green transition"
  },

  {  // killough 2/8/98: weapon preferences set by user:
    "weapon_choice_1",
    &weapon_preferences[0][0], NULL,
    6, {1,9}, dt_number, ss_weap, wad_yes,
    "first choice for weapon (best)"
  },

  {
    "weapon_choice_2",
    &weapon_preferences[0][1], NULL,
    9, {1,9}, dt_number, ss_weap, wad_yes,
    "second choice for weapon"
  },

  {
    "weapon_choice_3",
    &weapon_preferences[0][2], NULL,
    4, {1,9}, dt_number, ss_weap, wad_yes,
    "third choice for weapon"
  },

  {
    "weapon_choice_4",
    &weapon_preferences[0][3], NULL,
    3, {1,9}, dt_number, ss_weap, wad_yes,
    "fourth choice for weapon"
  },

  {
    "weapon_choice_5",
    &weapon_preferences[0][4], NULL,
    2, {1,9}, dt_number, ss_weap, wad_yes,
    "fifth choice for weapon"
  },

  {
    "weapon_choice_6",
    &weapon_preferences[0][5], NULL,
    8, {1,9}, dt_number, ss_weap, wad_yes,
    "sixth choice for weapon"
  },

  {
    "weapon_choice_7",
    &weapon_preferences[0][6], NULL,
    5, {1,9}, dt_number, ss_weap, wad_yes,
    "seventh choice for weapon "
  },

  {
    "weapon_choice_8",
    &weapon_preferences[0][7], NULL,
    7, {1,9}, dt_number, ss_weap, wad_yes,
    "eighth choice for weapon"
  },

  {
    "weapon_choice_9",
    &weapon_preferences[0][8], NULL,
    1, {1,9}, dt_number, ss_weap, wad_yes,
    "ninth choice for weapon (worst)"
  },

  {
    "c_speed",
    &c_speed, NULL,
    10, {1,200}, dt_number, ss_none, wad_no,
    "console speed, pixels/tic"
  },

  {
    "c_height",
    &c_height, NULL,
    100, {0,200}, dt_number, ss_none, wad_no,
    "console height, pixels"
  },

  {
    "obituaries",
    &obituaries, NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "obituaries on/off"
  },

  {
    "obcolour",
    &obcolour, NULL,
    0, {0,CR_LIMIT-1}, dt_number, ss_none, wad_no,
    "obituaries colour"
  },

  {
    "draw_particles",
    &drawparticles, NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "toggle particle effects on or off"
  },

  {
    "particle_trans",
    &particle_trans, NULL,
    1, {0,2}, dt_number, ss_none, wad_yes,
    "particle translucency (0 = none, 1 = smooth, 2 = general)"
  },

  {
    "blood_particles",
    &bloodsplat_particle, NULL,
    0, {0,2}, dt_number, ss_none, wad_yes,
    "use sprites, particles, or both for blood (sprites = 0)"
  },

  {
    "bullet_particles",
    &bulletpuff_particle, NULL,
    0, {0,2}, dt_number, ss_none, wad_yes,
    "use sprites, particles, or both for bullet puffs (sprites = 0)"
  },

  {
    "rocket_trails",
    &drawrockettrails, NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "draw particle rocket trails"
  },

  {
    "grenade_trails",
    &drawgrenadetrails, NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "draw particle grenade trails"
  },

  {
    "bfg_cloud",
    &drawbfgcloud, NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "draw particle bfg cloud"
  },

  {
    "pevent_rexpl",
    &(particleEvents[P_EVENT_ROCKET_EXPLODE].enabled), NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "draw particle rocket explosions"
  },

  {
    "pevent_bfgexpl",
    &(particleEvents[P_EVENT_BFG_EXPLODE].enabled), NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "draw particle bfg explosions"
  },

  {
    "stretchsky",
    &stretchsky, NULL,
    1, {0,1}, dt_number, ss_none, wad_yes,
    "stretch short sky textures for mlook"
  },

  {
    "startnewmap",
    &startOnNewMap, NULL,
    0, {0,1}, dt_number, ss_none, wad_yes,
    "start game on first new map"
  },

  {NULL}         // last entry
};

static char *defaultfile;
static boolean defaults_loaded = false;      // killough 10/98

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
   if(!hash_init)
   {
      for(hash_init = 1, dp = defaults; dp->name; dp++)
      {
         unsigned h = default_hash(dp->name);
         dp->next = defaults[h].first;
         defaults[h].first = dp;
      }
   }

   // Look up name in hash table
   for(dp = defaults[default_hash(name)].first;
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
   if(!defaults_loaded || !defaultfile)
      return;

   psnprintf(tmpfile, sizeof(tmpfile), "%s/tmp%.5s.cfg", 
             D_DoomExeDir(), D_DoomExeName());
   NormalizeSlashes(tmpfile);

   errno = 0;
   if(!(f = fopen(tmpfile, "w")))  // killough 9/21/98
      goto error;

   // 3/3/98 explain format of file
   // killough 10/98: use executable's name

   if(config_help && !config_help_header &&
      fprintf(f,";%s.cfg format:\n"
              ";[min-max(default)] description of variable\n"
              ";* at end indicates variable is settable in wads\n"
              ";variable   value\n\n", D_DoomExeName()) == EOF)
      goto error;

   // killough 10/98: output comment lines which were read in during input

   for(blanks = 1, line = 0, dp = defaults; ; dp++, blanks = 0)
   {
      int brackets = 0, value;

      for(;line < comment && comments[line].line <= dp-defaults; line++)
      {
         if(*comments[line].text != '[' || (brackets = 1, config_help))
         {
            // If we haven't seen any blank lines
            // yet, and this one isn't blank,
            // output a blank line for separation

            if((!blanks && (blanks = 1, 
                            *comments[line].text != '\n' &&
                            putc('\n',f) == EOF)) ||
               fputs(comments[line].text, f) == EOF)
               goto error;
         }
      }

      // If we still haven't seen any blanks,
      // Output a blank line for separation

      if(!blanks && putc('\n',f) == EOF)
         goto error;

      if(!dp->name)      // If we're at end of defaults table, exit loop
         break;

      //jff 3/3/98 output help string
      //
      // killough 10/98:
      // Don't output config help if any [ lines appeared before this one.
      // Make default values, and numeric range output, automatic.

      if(config_help && !brackets)
      {
         if((dp->isstr ? 
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
      }

      // killough 11/98:
      // Write out original default if .wad file has modified the default
      
      value = dp->modified ? dp->orig_default : (int) *dp->location;

      //jff 4/10/98 kill super-hack on pointer value
      // killough 3/6/98:
      // use spaces instead of tabs for uniform justification

      if(!dp->isstr ? fprintf(f, "%-25s %5i\n", dp->name, 
                              strncmp(dp->name, "key_", 4) ? value :
                              I_DoomCode2ScanCode(value)) == EOF :
         fprintf(f,"%-25s \"%s\"\n", dp->name, (char *) value) == EOF)
        goto error;
   }

   if(fclose(f) == EOF)
   {
   error:
      I_Error("Could not write defaults to %s: %s\n%s left unchanged\n",
              tmpfile, errno ? strerror(errno): "(Unknown Error)",defaultfile);
      return;
   }

   remove(defaultfile);

   if(rename(tmpfile, defaultfile))
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

boolean M_ParseOption(const char *p, boolean wad)
{
   char name[80], strparm[100];
   default_t *dp;
   int parm;
   
   while(isspace(*p))  // killough 10/98: skip leading whitespace
      p++;

   //jff 3/3/98 skip lines not starting with an alphanum
   // killough 10/98: move to be made part of main test, add comment-handling

   if(sscanf(p, "%79s %99[^\n]", name, strparm) != 2 || !isalnum(*name) ||
      !(dp = M_LookupDefault(name)) || (*strparm == '"') == !dp->isstr ||
      (wad && !dp->wad_allowed))
   {
      return 1;
   }

   if(dp->isstr)     // get a string default
   {
      int len = strlen(strparm)-1;

      while(isspace(strparm[len]))
         len--;

      if(strparm[len] == '"')
         len--;

      strparm[len+1] = 0;

      if (wad && !dp->modified)                 // Modified by wad
      {                                         // First time modified
         dp->modified = 1;                      // Mark it as modified
         dp->orig_default = *dp->location;      // Save original default
      }
      else
         free(*(char **) dp->location);              // Free old value

      *(char **) dp->location = strdup(strparm+1);   // Change default value

      if(dp->current)                                // Current value
      {
         free(*(char **) dp->current);               // Free old value
         *(char **) dp->current = strdup(strparm+1); // Change current value
      }
   }
   else
   {
      if(sscanf(strparm, "%i", &parm) != 1)
         return 1;                       // Not A Number

      if(!strncmp(name, "key_", 4))    // killough
         parm = I_ScanCode2DoomCode(parm);

      //jff 3/4/98 range check numeric parameters
      if((dp->limit.min == UL || dp->limit.min <= parm) &&
         (dp->limit.max == UL || dp->limit.max >= parm))
      {
         if(wad)
         {
            if(!dp->modified)         // First time it's modified by wad
            {
               dp->modified = 1;      // Mark it as modified
               dp->orig_default = *dp->location;  // Save original default
            }
            if(dp->current)           // Change current value
               *dp->current = parm;
         }
         *dp->location = parm;          // Change default
      }
   }

//  if (wad && dp->setup_menu)
//    dp->setup_menu->m_flags |= S_SKIP;

   return 0;                          // Success
}

//
// haleyjd 02/28/03: function to resolve comp flag conflicts
//
static void CheckCompConflicts(void)
{
   // if comp_floors is on, comp_overunder must be on
   if(comp[comp_floors] && demo_version >= 331)
   {
      default_comp[comp_overunder] = comp[comp_overunder] = 1;
   }
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
   
   if((lump = W_CheckNumForName("OPTIONS")) != -1)
   {
      int size = W_LumpLength(lump), buflen = 0;
      char *buf = NULL, *p, *options = p = W_CacheLumpNum(lump, PU_STATIC);
      while (size > 0)
      {
         int len = 0;
         while(len < size && p[len++] && p[len-1] != '\n');
         if(len >= buflen)
            buf = realloc(buf, buflen = len+1);
         strncpy(buf, p, len)[len] = 0;
         p += len;
         size -= len;
         M_ParseOption(buf, true);
      }
      free(buf);
      Z_ChangeTag(options, PU_CACHE);
   }

   CheckCompConflicts(); // haleyjd

   //  M_Trans();           // reset translucency in case of change
   //  MN_ResetMenu();       // reset menu in case of change
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

   for(dp = defaults; dp->name; dp++)
   {
      *dp->location =
         dp->isstr ? (int) strdup((char *) dp->defaultvalue) : dp->defaultvalue;
   }

   // check for a custom default file
   
   if (!defaultfile)
   {
      if((i = M_CheckParm("-config")) && i < myargc-1)
         printf(" default file: %s\n", defaultfile = strdup(myargv[i+1]));
      else
         defaultfile = strdup(basedefault);
   }

   NormalizeSlashes(defaultfile);
   
   // read the file in, overriding any set defaults
   //
   // killough 9/21/98: Print warning if file missing, and use fgets for reading

   if(!(f = fopen(defaultfile, "r")))
      printf("Warning: Cannot read %s -- using built-in defaults\n",defaultfile);
   else
   {
      int skipblanks = 1, line = comment = config_help_header = 0;
      char s[256];

      while(fgets(s, sizeof s, f))
      {
         if(!M_ParseOption(s, false))
            line++;       // Line numbers
         else
         {             // Remember comment lines
            const char *p = s;
            
            while(isspace(*p))  // killough 10/98: skip leading whitespace
               p++;

            if(*p)                // If this is not a blank line,
            {
               skipblanks = 0;    // stop skipping blanks.
               if(strstr(p, ".cfg format:"))
                  config_help_header = 1;
            }
            else
            {
               if(skipblanks)      // If we are skipping blanks, skip line
                  continue;
               else            // Skip multiple blanks, but remember this one
                  skipblanks = 1, p = "\n";
            }

            if(comment >= comment_alloc)
               comments = realloc(comments, sizeof *comments *
                                  (comment_alloc = comment_alloc ?
                                   comment_alloc * 2 : 10));
            comments[comment].line = line;
            comments[comment++].text = strdup(p);
         }
      }
      fclose (f);
   }

   defaults_loaded = true;            // killough 10/98

   CheckCompConflicts(); // haleyjd
   
   //jff 3/4/98 redundant range checks for hud deleted here
}

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//
// sf: removed as it wasnt being used

//
// M_WriteFile
//
// killough 9/98: rewritten to use stdio and to flash disk icon

boolean M_WriteFile(char const *name, void *source, int length)
{
   FILE *fp;
   
   errno = 0;
   
   if(!(fp = fopen(name, "wb")))         // Try opening file
      return 0;                          // Could not open file for writing
   
   I_BeginRead();                       // Disk icon on
   length = fwrite(source, 1, length, fp) == length;   // Write data
   fclose(fp);
   I_EndRead();                         // Disk icon off
   
   if(!length)                          // Remove partially written file
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
   
   if((fp = fopen(name, "rb")))
   {
      size_t length;

      I_BeginRead();
      fseek(fp, 0, SEEK_END);
      length = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      *buffer = Z_Malloc(length, PU_STATIC, 0);
      
      if(fread(*buffer, 1, length, fp) == length)
      {
         fclose(fp);
         I_EndRead();
         return length;
      }
      fclose(fp);
   }

   // sf: do not quit on file not found
   //  I_Error("Couldn't read file %s: %s", name, 
   //	  errno ? strerror(errno) : "(Unknown Error)");
   
   return -1;
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

boolean WritePCXfile(char *filename, byte *data, int width,
                     int height, byte *palette)
{
  int    i;
  int    length;
  pcx_t* pcx;
  byte*  pack;
  boolean success;      // killough 10/98

  pcx = Z_Malloc(width*height*2+1000, PU_STATIC, NULL);

  pcx->manufacturer = 0x0a; // PCX id
  pcx->version = 5;         // 256 color
  pcx->encoding = 1;        // uncompressed
  pcx->bits_per_pixel = 8;  // 256 color
  pcx->xmin = 0;
  pcx->ymin = 0;
  pcx->xmax = SHORT((short)(width-1));
  pcx->ymax = SHORT((short)(height-1));
  pcx->hres = SHORT((short)width);
  pcx->vres = SHORT((short)height);
  memset(pcx->palette,0,sizeof(pcx->palette));
  pcx->color_planes = 1;        // chunky image
  pcx->bytes_per_line = SHORT((short)width);
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

// SoM 6/5/02: Chu-Chu-Chu-Chu-Chu-Changes... heh
#ifdef _MSC_VER
#pragma pack(1)
#endif

#define BI_RGB 0L

typedef unsigned short uint_t;
typedef unsigned long dword_t;
typedef long     long_t;
typedef unsigned char ubyte_t;

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

#ifdef _MSC_VER
#pragma pack()
#endif

// jff 3/30/98 binary file write with error detection
// killough 10/98: changed into macro to return failure instead of aborting

#define SafeWrite(data,size,number,st) do {   \
    if (fwrite(data,size,number,st) < (number)) \
   return fclose(st), I_EndRead(), false; } while(0)

//
// WriteBMPfile
// jff 3/30/98 Add capability to write a .BMP file (256 color uncompressed)
//

boolean WriteBMPfile(char *filename, byte *data, int width,
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
  boolean success = false;

  errno = 0;

  // haleyjd 05/23/02: corrected uses of access to use defined
  // constants rather than integers, some of which were not even
  // correct under DJGPP to begin with (its a wonder it worked...)
  if (!access(".", W_OK))
  {
      static int shot;
      char lbmname[PATH_MAX+1];
      int tries = 10000;

      // sf: changed to smmu from doom
      // haleyjd: changed to etrn
      do
        sprintf(lbmname,                         //jff 3/30/98 pcx or bmp?
                screenshot_pcx ? "etrn%02d.pcx" : "etrn%02d.bmp", shot++);
      while (!access(lbmname, F_OK) && --tries);

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
         // SoM: ANYRES
         if(!(success = (screenshot_pcx ? WritePCXfile : WriteBMPfile)
            (lbmname,linear, v_width, v_height,pal)))
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
  S_StartSound(NULL, !success ? doom_printf(errno ? strerror(errno) :
                                            FC_ERROR"Could not take screenshot"), sfx_oof :
               gameModeInfo->c_BellSound);  // just tink, no radio
                                            // tink is in doom2 too
}

// haleyjd: portable strupr function
char *M_Strupr(char *string)
{
   char *s = string;

   while(*s)
   {
      char c = *s;
      *s++ = toupper(c);
   }

   return string;
}

// haleyjd: portable strlwr function
char *M_Strlwr(char *string)
{
   char *s = string;

   while(*s)
   {
      char c = *s;
      *s++ = tolower(c);
   }

   return string;
}

// haleyjd: portable itoa, from DJGPP libc

/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */

char *M_Itoa(int value, char *string, int radix)
{
#ifdef HAVE_ITOA
   return ITOA_NAME(value, string, radix);
#else
   char tmp[33];
   char *tp = tmp;
   int i;
   unsigned int v;
   int sign;
   char *sp;

   if(radix > 36 || radix <= 1)
   {
      errno = EDOM;
      return 0;
   }

   sign = (radix == 10 && value < 0);

   if(sign)
      v = -value;
   else
      v = (unsigned int)value;

   while(v || tp == tmp)
   {
      i = v % radix;
      v = v / radix;

      if(i < 10)
         *tp++ = i + '0';
      else
         *tp++ = i + 'a' - 10;
   }

   if(string == 0)
      string = (char *)(malloc)((tp-tmp)+sign+1);
   sp = string;

   if(sign)
      *sp++ = '-';

   while(tp > tmp)
      *sp++ = *--tp;
   *sp = 0;

   return string;
#endif
}

// haleyjd: general file path name extraction
void M_GetFilePath(const char *fn, char *base, size_t len)
{
   char *p;

   memset(base, 0, len);

   // haleyjd 01/11/04: gads, this was writing one off the
   // end of the array, messing up EDF file loading from GFS!
   p = base + len - 1;

   strncpy(base, fn, len);
   
   while(p >= base)
   {
      if(*p == '/' || *p == '\\')
      {
         *p = '\0';
         break;
      }
      *p = '\0';
      p--;
   }

   // haleyjd: do not return "" when there is no directory,
   // instead return "." for current working directory
   if(*base == '\0')
      *base = '.';
}


//----------------------------------------------------------------------------
//
// $Log: m_misc.c,v $
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

