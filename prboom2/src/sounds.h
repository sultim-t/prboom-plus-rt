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
 *      Created by the sound utility written by Dave Taylor.
 *      Kept as a sample, DOOM2 sounds. Frozen.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __SOUNDS__
#define __SOUNDS__

//
// SoundFX struct.
//

struct sfxinfo_struct;

typedef struct sfxinfo_struct sfxinfo_t;

struct sfxinfo_struct {

  // up to 6-character name
  const char *name; // CPhipps - const

  // Sfx singularity (only one at a time)
  int singularity;

  // Sfx priority
  int priority;

  // referenced sound if a link
  sfxinfo_t *link;

  // pitch if a link
  int pitch;

  // volume if a link
  int volume;

  // sound data
  void *data;

  // this field was related to caching (now meaningless)
  int usefulness;

  // lump number of sfx
  int lumpnum;
};

//
// MusicInfo struct.
//

typedef struct {
  // up to 6-character name
  const char *name; // CPhipps - const

  // lump number of music
  int lumpnum;

  /* music data - cphipps 4/11 made const void* */
  const void *data;

  // music handle once registered
  int handle;
} musicinfo_t;

// the complete set of sound effects
extern sfxinfo_t    S_sfx[];

// the complete set of music
extern musicinfo_t  S_music[];

//
// Identifiers for all music in game.
//

typedef enum {
  mus_None,
  mus_e1m1,
  mus_e1m2,
  mus_e1m3,
  mus_e1m4,
  mus_e1m5,
  mus_e1m6,
  mus_e1m7,
  mus_e1m8,
  mus_e1m9,
  mus_e2m1,
  mus_e2m2,
  mus_e2m3,
  mus_e2m4,
  mus_e2m5,
  mus_e2m6,
  mus_e2m7,
  mus_e2m8,
  mus_e2m9,
  mus_e3m1,
  mus_e3m2,
  mus_e3m3,
  mus_e3m4,
  mus_e3m5,
  mus_e3m6,
  mus_e3m7,
  mus_e3m8,
  mus_e3m9,
  mus_inter,
  mus_intro,
  mus_bunny,
  mus_victor,
  mus_introa,
  mus_runnin,
  mus_stalks,
  mus_countd,
  mus_betwee,
  mus_doom,
  mus_the_da,
  mus_shawn,
  mus_ddtblu,
  mus_in_cit,
  mus_dead,
  mus_stlks2,
  mus_theda2,
  mus_doom2,
  mus_ddtbl2,
  mus_runni2,
  mus_dead2,
  mus_stlks3,
  mus_romero,
  mus_shawn2,
  mus_messag,
  mus_count2,
  mus_ddtbl3,
  mus_ampie,
  mus_theda3,
  mus_adrian,
  mus_messg2,
  mus_romer2,
  mus_tense,
  mus_shawn3,
  mus_openin,
  mus_evil,
  mus_ultima,
  mus_read_m,
  mus_dm2ttl,
  mus_dm2int,
  NUMMUSIC
} musicenum_t;

//
// Identifiers for all sfx in game.
//

typedef enum {
  sfx_None,
  sfx_pistol,
  sfx_shotgn,
  sfx_sgcock,
  sfx_dshtgn,
  sfx_dbopn,
  sfx_dbcls,
  sfx_dbload,
  sfx_plasma,
  sfx_bfg,
  sfx_sawup,
  sfx_sawidl,
  sfx_sawful,
  sfx_sawhit,
  sfx_rlaunc,
  sfx_rxplod,
  sfx_firsht,
  sfx_firxpl,
  sfx_pstart,
  sfx_pstop,
  sfx_doropn,
  sfx_dorcls,
  sfx_stnmov,
  sfx_swtchn,
  sfx_swtchx,
  sfx_plpain,
  sfx_dmpain,
  sfx_popain,
  sfx_vipain,
  sfx_mnpain,
  sfx_pepain,
  sfx_slop,
  sfx_itemup,
  sfx_wpnup,
  sfx_oof,
  sfx_telept,
  sfx_posit1,
  sfx_posit2,
  sfx_posit3,
  sfx_bgsit1,
  sfx_bgsit2,
  sfx_sgtsit,
  sfx_cacsit,
  sfx_brssit,
  sfx_cybsit,
  sfx_spisit,
  sfx_bspsit,
  sfx_kntsit,
  sfx_vilsit,
  sfx_mansit,
  sfx_pesit,
  sfx_sklatk,
  sfx_sgtatk,
  sfx_skepch,
  sfx_vilatk,
  sfx_claw,
  sfx_skeswg,
  sfx_pldeth,
  sfx_pdiehi,
  sfx_podth1,
  sfx_podth2,
  sfx_podth3,
  sfx_bgdth1,
  sfx_bgdth2,
  sfx_sgtdth,
  sfx_cacdth,
  sfx_skldth,
  sfx_brsdth,
  sfx_cybdth,
  sfx_spidth,
  sfx_bspdth,
  sfx_vildth,
  sfx_kntdth,
  sfx_pedth,
  sfx_skedth,
  sfx_posact,
  sfx_bgact,
  sfx_dmact,
  sfx_bspact,
  sfx_bspwlk,
  sfx_vilact,
  sfx_noway,
  sfx_barexp,
  sfx_punch,
  sfx_hoof,
  sfx_metal,
  sfx_chgun,
  sfx_tink,
  sfx_bdopn,
  sfx_bdcls,
  sfx_itmbk,
  sfx_flame,
  sfx_flamst,
  sfx_getpow,
  sfx_bospit,
  sfx_boscub,
  sfx_bossit,
  sfx_bospn,
  sfx_bosdth,
  sfx_manatk,
  sfx_mandth,
  sfx_sssit,
  sfx_ssdth,
  sfx_keenpn,
  sfx_keendt,
  sfx_skeact,
  sfx_skesit,
  sfx_skeatk,
  sfx_radio,

  /* killough 11/98: dog sounds */
  sfx_dgsit,
  sfx_dgatk,
  sfx_dgact,
  sfx_dgdth,
  sfx_dgpain,

  //e6y
  sfx_secret,
  sfx_gibdth,
  // Everything from here to 500 is reserved

  /* Free sound effect slots for DEHEXTRA. Offset agreed upon with Eternity devs. -SH */
  sfx_fre000 = 500,
  sfx_fre001,
  sfx_fre002,
  sfx_fre003,
  sfx_fre004,
  sfx_fre005,
  sfx_fre006,
  sfx_fre007,
  sfx_fre008,
  sfx_fre009,
  sfx_fre010,
  sfx_fre011,
  sfx_fre012,
  sfx_fre013,
  sfx_fre014,
  sfx_fre015,
  sfx_fre016,
  sfx_fre017,
  sfx_fre018,
  sfx_fre019,
  sfx_fre020,
  sfx_fre021,
  sfx_fre022,
  sfx_fre023,
  sfx_fre024,
  sfx_fre025,
  sfx_fre026,
  sfx_fre027,
  sfx_fre028,
  sfx_fre029,
  sfx_fre030,
  sfx_fre031,
  sfx_fre032,
  sfx_fre033,
  sfx_fre034,
  sfx_fre035,
  sfx_fre036,
  sfx_fre037,
  sfx_fre038,
  sfx_fre039,
  sfx_fre040,
  sfx_fre041,
  sfx_fre042,
  sfx_fre043,
  sfx_fre044,
  sfx_fre045,
  sfx_fre046,
  sfx_fre047,
  sfx_fre048,
  sfx_fre049,
  sfx_fre050,
  sfx_fre051,
  sfx_fre052,
  sfx_fre053,
  sfx_fre054,
  sfx_fre055,
  sfx_fre056,
  sfx_fre057,
  sfx_fre058,
  sfx_fre059,
  sfx_fre060,
  sfx_fre061,
  sfx_fre062,
  sfx_fre063,
  sfx_fre064,
  sfx_fre065,
  sfx_fre066,
  sfx_fre067,
  sfx_fre068,
  sfx_fre069,
  sfx_fre070,
  sfx_fre071,
  sfx_fre072,
  sfx_fre073,
  sfx_fre074,
  sfx_fre075,
  sfx_fre076,
  sfx_fre077,
  sfx_fre078,
  sfx_fre079,
  sfx_fre080,
  sfx_fre081,
  sfx_fre082,
  sfx_fre083,
  sfx_fre084,
  sfx_fre085,
  sfx_fre086,
  sfx_fre087,
  sfx_fre088,
  sfx_fre089,
  sfx_fre090,
  sfx_fre091,
  sfx_fre092,
  sfx_fre093,
  sfx_fre094,
  sfx_fre095,
  sfx_fre096,
  sfx_fre097,
  sfx_fre098,
  sfx_fre099,
  sfx_fre100,
  sfx_fre101,
  sfx_fre102,
  sfx_fre103,
  sfx_fre104,
  sfx_fre105,
  sfx_fre106,
  sfx_fre107,
  sfx_fre108,
  sfx_fre109,
  sfx_fre110,
  sfx_fre111,
  sfx_fre112,
  sfx_fre113,
  sfx_fre114,
  sfx_fre115,
  sfx_fre116,
  sfx_fre117,
  sfx_fre118,
  sfx_fre119,
  sfx_fre120,
  sfx_fre121,
  sfx_fre122,
  sfx_fre123,
  sfx_fre124,
  sfx_fre125,
  sfx_fre126,
  sfx_fre127,
  sfx_fre128,
  sfx_fre129,
  sfx_fre130,
  sfx_fre131,
  sfx_fre132,
  sfx_fre133,
  sfx_fre134,
  sfx_fre135,
  sfx_fre136,
  sfx_fre137,
  sfx_fre138,
  sfx_fre139,
  sfx_fre140,
  sfx_fre141,
  sfx_fre142,
  sfx_fre143,
  sfx_fre144,
  sfx_fre145,
  sfx_fre146,
  sfx_fre147,
  sfx_fre148,
  sfx_fre149,
  sfx_fre150,
  sfx_fre151,
  sfx_fre152,
  sfx_fre153,
  sfx_fre154,
  sfx_fre155,
  sfx_fre156,
  sfx_fre157,
  sfx_fre158,
  sfx_fre159,
  sfx_fre160,
  sfx_fre161,
  sfx_fre162,
  sfx_fre163,
  sfx_fre164,
  sfx_fre165,
  sfx_fre166,
  sfx_fre167,
  sfx_fre168,
  sfx_fre169,
  sfx_fre170,
  sfx_fre171,
  sfx_fre172,
  sfx_fre173,
  sfx_fre174,
  sfx_fre175,
  sfx_fre176,
  sfx_fre177,
  sfx_fre178,
  sfx_fre179,
  sfx_fre180,
  sfx_fre181,
  sfx_fre182,
  sfx_fre183,
  sfx_fre184,
  sfx_fre185,
  sfx_fre186,
  sfx_fre187,
  sfx_fre188,
  sfx_fre189,
  sfx_fre190,
  sfx_fre191,
  sfx_fre192,
  sfx_fre193,
  sfx_fre194,
  sfx_fre195,
  sfx_fre196,
  sfx_fre197,
  sfx_fre198,
  sfx_fre199,

  NUMSFX
} sfxenum_t;

#endif
