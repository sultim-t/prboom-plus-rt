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
//      Created by a sound utility.
//      Kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: sounds.c,v 1.3 1998/05/03 22:44:25 killough Exp $";

// killough 5/3/98: reformatted

#include "doomtype.h"
#include "sounds.h"
#include "p_skin.h"

//
// Information about all the music
//

musicinfo_t S_music[] = {
  { 0 },
  { "e1m1", 0 },
  { "e1m2", 0 },
  { "e1m3", 0 },
  { "e1m4", 0 },
  { "e1m5", 0 },
  { "e1m6", 0 },
  { "e1m7", 0 },
  { "e1m8", 0 },
  { "e1m9", 0 },
  { "e2m1", 0 },
  { "e2m2", 0 },
  { "e2m3", 0 },
  { "e2m4", 0 },
  { "e2m5", 0 },
  { "e2m6", 0 },
  { "e2m7", 0 },
  { "e2m8", 0 },
  { "e2m9", 0 },
  { "e3m1", 0 },
  { "e3m2", 0 },
  { "e3m3", 0 },
  { "e3m4", 0 },
  { "e3m5", 0 },
  { "e3m6", 0 },
  { "e3m7", 0 },
  { "e3m8", 0 },
  { "e3m9", 0 },
  { "inter", 0 },
  { "intro", 0 },
  { "bunny", 0 },
  { "victor", 0 },
  { "introa", 0 },
  { "runnin", 0 },
  { "stalks", 0 },
  { "countd", 0 },
  { "betwee", 0 },
  { "doom", 0 },
  { "the_da", 0 },
  { "shawn", 0 },
  { "ddtblu", 0 },
  { "in_cit", 0 },
  { "dead", 0 },
  { "stlks2", 0 },
  { "theda2", 0 },
  { "doom2", 0 },
  { "ddtbl2", 0 },
  { "runni2", 0 },
  { "dead2", 0 },
  { "stlks3", 0 },
  { "romero", 0 },
  { "shawn2", 0 },
  { "messag", 0 },
  { "count2", 0 },
  { "ddtbl3", 0 },
  { "ampie", 0 },
  { "theda3", 0 },
  { "adrian", 0 },
  { "messg2", 0 },
  { "romer2", 0 },
  { "tense", 0 },
  { "shawn3", 0 },
  { "openin", 0 },
  { "evil", 0 },
  { "ultima", 0 },
  { "read_m", 0 },
  { "dm2ttl", 0 },
  { "dm2int", 0 },
  { "newmus", 0 },
};

musicinfo_t H_music[] =
{
   { 0 },
   { "e1m1", 0 }, // 1
   { "e1m2", 0 }, // 2
   { "e1m3", 0 }, // 3
   { "e1m4", 0 }, // 4
   { "e1m5", 0 }, // 5
   { "e1m6", 0 }, // 6
   { "e1m7", 0 }, // 7
   { "e1m8", 0 }, // 8
   { "e1m9", 0 }, // 9
   { "e2m1", 0 }, // 10
   { "e2m2", 0 }, // 11
   { "e2m3", 0 }, // 12
   { "e2m4", 0 }, // 13
   { "e2m6", 0 }, // 14
   { "e2m7", 0 }, // 15
   { "e2m8", 0 }, // 16
   { "e2m9", 0 }, // 17
   { "e3m2", 0 }, // 18
   { "e3m3", 0 }, // 19
   { "titl", 0 }, // 20
   { "intr", 0 }, // 21
   { "cptd", 0 }, // 22
};

// haleyjd: a much more clever way of getting an index to the music
// above than what was used in Heretic
int H_Mus_Matrix[6][9] =
{
   {  1,  2,  3,  4,  5,  6,  7,  8,  9 }, // episode 1
   { 10, 11, 12, 13,  4, 14, 15, 16, 17 }, // episode 2
   {  1, 18, 19,  6,  3,  2,  5,  9, 14 }, // episode 3
   {  6,  2,  3,  4,  5,  1,  7,  8,  9 }, // episode 4 (sosr)
   { 10, 11, 12, 13,  4, 14, 15, 16, 17 }, // episode 5 (sosr)
   { 18, 19,  6, 20, 21, 22,  1,  1,  1 }, // hidden levels
};

//
// Information about all the sfx
//
// killough 12/98: 
// Reimplemented 'singularity' flag, adjusting many sounds below

// haleyjd 11/05/03: made dynamic with EDF
sfxinfo_t *S_sfx = NULL;
int NUMSFX = 0;

/*
sfxinfo_t S_sfx[] = {
  { "none" },  // S_sfx[0] needs to be a dummy for odd reasons.

//  name      prefix singl.    pri lnk vol pt dat skin
  { "pistol", true,  sg_none,   64, 0, -1, -1, 0 },
  { "shotgn", true,  sg_none,   64, 0, -1, -1, 0 },
  { "sgcock", true,  sg_none,   64, 0, -1, -1, 0 },
  { "dshtgn", true,  sg_none,   64, 0, -1, -1, 0 },
  { "dbopn",  true,  sg_none,   64, 0, -1, -1, 0 },
  { "dbcls",  true,  sg_none,   64, 0, -1, -1, 0 },
  { "dbload", true,  sg_none,   64, 0, -1, -1, 0 },
  { "plasma", true,  sg_none,   64, 0, -1, -1, 0 },
  { "bfg",    true,  sg_none,   64, 0, -1, -1, 0 },
  { "sawup",  true,  sg_none,   64, 0, -1, -1, 0 },
  { "sawidl", true,  sg_none,  118, 0, -1, -1, 0 },
  { "sawful", true,  sg_none,   64, 0, -1, -1, 0 },
  { "sawhit", true,  sg_none,   64, 0, -1, -1, 0 },
  { "rlaunc", true,  sg_none,   64, 0, -1, -1, 0 },
  { "rxplod", true,  sg_none,   70, 0, -1, -1, 0 },
  { "firsht", true,  sg_none,   70, 0, -1, -1, 0 },
  { "firxpl", true,  sg_none,   70, 0, -1, -1, 0 },
  { "pstart", true,  sg_none,  100, 0, -1, -1, 0 },
  { "pstop",  true,  sg_none,  100, 0, -1, -1, 0 },
  { "doropn", true,  sg_none,  100, 0, -1, -1, 0 },
  { "dorcls", true,  sg_none,  100, 0, -1, -1, 0 },
  { "stnmov", true,  sg_none,  119, 0, -1, -1, 0 },
  { "swtchn", true,  sg_none,   78, 0, -1, -1, 0 },
  { "swtchx", true,  sg_none,   78, 0, -1, -1, 0 },
  { "plpain", true,  sg_none,   96, 0, -1, -1, 0, sk_plpain+1 },
  { "dmpain", true,  sg_none,   96, 0, -1, -1, 0 },
  { "popain", true,  sg_none,   96, 0, -1, -1, 0 },
  { "vipain", true,  sg_none,   96, 0, -1, -1, 0 },
  { "mnpain", true,  sg_none,   96, 0, -1, -1, 0 },
  { "pepain", true,  sg_none,   96, 0, -1, -1, 0 },
  { "slop",   true,  sg_none,   78, 0, -1, -1, 0, sk_slop+1 },
  { "itemup", true,  sg_itemup, 78, 0, -1, -1, 0 },
  { "wpnup",  true,  sg_wpnup,  78, 0, -1, -1, 0 },
  { "oof",    true,  sg_oof,    96, 0, -1, -1, 0, sk_oof+1 },
  { "telept", true,  sg_none,   32, 0, -1, -1, 0 },
  { "posit1", true,  sg_none,   98, 0, -1, -1, 0 },
  { "posit2", true,  sg_none,   98, 0, -1, -1, 0 },
  { "posit3", true,  sg_none,   98, 0, -1, -1, 0 },
  { "bgsit1", true,  sg_none,   98, 0, -1, -1, 0 },
  { "bgsit2", true,  sg_none,   98, 0, -1, -1, 0 },
  { "sgtsit", true,  sg_none,   98, 0, -1, -1, 0 },
  { "cacsit", true,  sg_none,   98, 0, -1, -1, 0 },
  { "brssit", true,  sg_none,   94, 0, -1, -1, 0 },
  { "cybsit", true,  sg_none,   92, 0, -1, -1, 0 },
  { "spisit", true,  sg_none,   90, 0, -1, -1, 0 },
  { "bspsit", true,  sg_none,   90, 0, -1, -1, 0 },
  { "kntsit", true,  sg_none,   90, 0, -1, -1, 0 },
  { "vilsit", true,  sg_none,   90, 0, -1, -1, 0 },
  { "mansit", true,  sg_none,   90, 0, -1, -1, 0 },
  { "pesit",  true,  sg_none,   90, 0, -1, -1, 0 },
  { "sklatk", true,  sg_none,   70, 0, -1, -1, 0 },
  { "sgtatk", true,  sg_none,   70, 0, -1, -1, 0 },
  { "skepch", true,  sg_none,   70, 0, -1, -1, 0 },
  { "vilatk", true,  sg_none,   70, 0, -1, -1, 0 },
  { "claw",   true,  sg_none,   70, 0, -1, -1, 0 },
  { "skeswg", true,  sg_none,   70, 0, -1, -1, 0 },
  { "pldeth", true,  sg_none,   32, 0, -1, -1, 0, sk_pldeth+1 },
  { "pdiehi", true,  sg_none,   32, 0, -1, -1, 0, sk_pdiehi+1 },
  { "podth1", true,  sg_none,   70, 0, -1, -1, 0 },
  { "podth2", true,  sg_none,   70, 0, -1, -1, 0 },
  { "podth3", true,  sg_none,   70, 0, -1, -1, 0 },
  { "bgdth1", true,  sg_none,   70, 0, -1, -1, 0 },
  { "bgdth2", true,  sg_none,   70, 0, -1, -1, 0 },
  { "sgtdth", true,  sg_none,   70, 0, -1, -1, 0 },
  { "cacdth", true,  sg_none,   70, 0, -1, -1, 0 },
  { "skldth", true,  sg_none,   70, 0, -1, -1, 0 },
  { "brsdth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "cybdth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "spidth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "bspdth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "vildth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "kntdth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "pedth",  true,  sg_none,   32, 0, -1, -1, 0 },
  { "skedth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "posact", true,  sg_none,  120, 0, -1, -1, 0 },
  { "bgact",  true,  sg_none,  120, 0, -1, -1, 0 },
  { "dmact",  true,  sg_none,  120, 0, -1, -1, 0 },
  { "bspact", true,  sg_none,  100, 0, -1, -1, 0 },
  { "bspwlk", true,  sg_none,  100, 0, -1, -1, 0 },
  { "vilact", true,  sg_none,  100, 0, -1, -1, 0 },
  { "noway",  true,  sg_oof,    78, 0, -1, -1, 0, sk_oof+1  },
  { "barexp", true,  sg_none,   60, 0, -1, -1, 0 },
  { "punch",  true,  sg_none,   64, 0, -1, -1, 0, sk_punch+1 },
  { "hoof",   true,  sg_none,   70, 0, -1, -1, 0 },
  { "metal",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "chgun",  true,  sg_none,   64, &S_sfx[sfx_pistol], 128, 0, 0 },
  { "tink",   true,  sg_none,   60, 0, -1, -1, 0 },
  { "bdopn",  true,  sg_none,  100, 0, -1, -1, 0 },
  { "bdcls",  true,  sg_none,  100, 0, -1, -1, 0 },
  { "itmbk",  true,  sg_none,  100, 0, -1, -1, 0 },
  { "flame",  true,  sg_none,   32, 0, -1, -1, 0 },
  { "flamst", true,  sg_none,   32, 0, -1, -1, 0 },
  { "getpow", true,  sg_getpow, 60, 0, -1, -1, 0 },
  { "bospit", true,  sg_none,   70, 0, -1, -1, 0 },
  { "boscub", true,  sg_none,   70, 0, -1, -1, 0 },
  { "bossit", true,  sg_none,   70, 0, -1, -1, 0 },
  { "bospn",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "bosdth", true,  sg_none,   70, 0, -1, -1, 0 },
  { "manatk", true,  sg_none,   70, 0, -1, -1, 0 },
  { "mandth", true,  sg_none,   70, 0, -1, -1, 0 },
  { "sssit",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "ssdth",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "keenpn", true,  sg_none,   70, 0, -1, -1, 0 },
  { "keendt", true,  sg_none,   70, 0, -1, -1, 0 },
  { "skeact", true,  sg_none,   70, 0, -1, -1, 0 },
  { "skesit", true,  sg_none,   70, 0, -1, -1, 0 },
  { "skeatk", true,  sg_none,   70, 0, -1, -1, 0 },
  { "radio",  true,  sg_none,   60, 0, -1, -1, 0 },

  // killough 11/98: dog sounds
  { "dgsit",  true,  sg_none,   98, 0, -1, -1, 0 },
  { "dgatk",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "dgact",  true,  sg_none,  120, 0, -1, -1, 0 },
  { "dgdth",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "dgpain", true,  sg_none,   96, 0, -1, -1, 0 },

  // haleyjd 10/08/02: heretic sounds                mnemonic
  { "gldhit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_gldhit" },
  { "telept", false, sg_none,   50, 0, -1, -1, 0, 0, "ht_telept" },
  { "chat",   false, sg_none,  100, 0, -1, -1, 0, 0, "ht_chat"  },
  { "keyup",  false, sg_none,   50, 0, -1, -1, 0, 0, "ht_keyup" },
  { "itemup", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_itemup" },
  { "mumsit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_mumsit" },
  { "mumat1", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_mumat1" },
  { "mumat2", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_mumat2" },
  { "mumpai", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_mumpai" },
  { "mumdth", false, sg_none,   80, 0, -1, -1, 0, 0, "ht_mumdth" },
  { "mumhed", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_mumhed" },
  { "clksit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_clksit" },
  { "clkatk", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_clkatk" },
  { "clkpai", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_clkpai" },
  { "clkdth", false, sg_none,   80, 0, -1, -1, 0, 0, "ht_clkdth" },
  { "clkact", false, sg_none,   20, 0, -1, -1, 0, 0, "ht_clkact" },
  { "wizsit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_wizsit" },
  { "wizatk", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_wizatk" },
  { "wizdth", false, sg_none,   80, 0, -1, -1, 0, 0, "ht_wizdth" },
  { "wizact", false, sg_none,   20, 0, -1, -1, 0, 0, "ht_wizact" },
  { "wizpai", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_wizpai" },
  { "sorzap", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_sorzap" },
  { "sorrise",false, sg_none,   32, 0, -1, -1, 0, 0, "ht_sorrise" },
  { "sorsit", false, sg_none,  200, 0, -1, -1, 0, 0, "ht_sorsit" },
  { "soratk", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_soratk" },
  { "sorpai", false, sg_none,  200, 0, -1, -1, 0, 0, "ht_sorpai" },
  { "soract", false, sg_none,  200, 0, -1, -1, 0, 0, "ht_soract" },
  { "sordsph",false, sg_none,  200, 0, -1, -1, 0, 0, "ht_sordsph" },
  { "sordexp",false, sg_none,  200, 0, -1, -1, 0, 0, "ht_sordexp" },
  { "sordbon",false, sg_none,  200, 0, -1, -1, 0, 0, "ht_sordbon" },
  { "wind",   false, sg_none,   16, 0, -1, -1, 0, 0, "ht_wind" },
  { "waterfl",false, sg_none,   16, 0, -1, -1, 0, 0, "ht_waterfl" },
  { "podexp", false, sg_none,   40, 0, -1, -1, 0, 0, "ht_podexp" },
  { "newpod", false, sg_none,   16, 0, -1, -1, 0, 0, "ht_newpod" },
  { "kgtsit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_kgtsit" },
  { "kgtatk", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_kgtatk" },
  { "kgtat2", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_kgtat2" },
  { "kgtdth", false, sg_none,   80, 0, -1, -1, 0, 0, "ht_kgtdth" },
  { "kgtpai", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_kgtpai" },
  { "hrnhit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_hrnhit" },
  { "bstsit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_bstsit" },
  { "bstatk", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_bstatk" },
  { "bstpai", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_bstpai" },
  { "bstdth", false, sg_none,   80, 0, -1, -1, 0, 0, "ht_bstdth" },
  { "bstact", false, sg_none,   20, 0, -1, -1, 0, 0, "ht_bstact" },
  { "snksit", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_snksit" },
  { "snkatk", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_snkatk" },
  { "snkpai", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_snkpai" },
  { "snkdth", false, sg_none,   80, 0, -1, -1, 0, 0, "ht_snkdth" },
  { "snkact", false, sg_none,   20, 0, -1, -1, 0, 0, "ht_snkact" },
  { "doropn", false, sg_none,   40, 0, -1, -1, 0, 0, "ht_doropn" },
  { "dorcls", false, sg_none,   40, 0, -1, -1, 0, 0, "ht_dorcls" },
  { "switch", false, sg_none,   40, 0, -1, -1, 0, 0, "ht_switch" },
  { "pstart", false, sg_none,   40, 0, -1, -1, 0, 0, "ht_pstart" },
  { "pstop",  false, sg_none,   40, 0, -1, -1, 0, 0, "ht_pstop" },
  { "stnmov", false, sg_none,   40, 0, -1, -1, 0, 0, "ht_stnmov" },
  { "sbtpai", false, sg_none,   32, 0, -1, -1, 0, 0, "ht_sbtpai" },
  { "sbtdth", false, sg_none,   80, 0, -1, -1, 0, 0, "ht_sbtdth" },
  { "sbtact", false, sg_none,   20, 0, -1, -1, 0, 0, "ht_sbtact" },

  // Start Eternity TC New Sounds
  { "minsit", true,  sg_none,   92, 0, -1, -1, 0 },
  { "minat1", true,  sg_none,   98, 0, -1, -1, 0 },
  { "minat2", true,  sg_none,   98, 0, -1, -1, 0 },
  { "minat3", true,  sg_none,   98, 0, -1, -1, 0 },
  { "minpai", true,  sg_none,   96, 0, -1, -1, 0 },
  { "mindth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "minact", true,  sg_none,   94, 0, -1, -1, 0 },
  { "stfpow", true,  sg_none,   92, 0, -1, -1, 0 },
  { "phohit", true,  sg_none,   60, 0, -1, -1, 0 },
  { "clratk", true,  sg_none,  120, 0, -1, -1, 0 },
  { "clrpn",  true,  sg_none,   96, 0, -1, -1, 0 },
  { "clrdth", true,  sg_none,   32, 0, -1, -1, 0 },
  { "cblsht", true,  sg_none,   70, 0, -1, -1, 0 },
  { "cblexp", true,  sg_none,   70, 0, -1, -1, 0 },
  { "gloop",  true,  sg_none,   32, 0, -1, -1, 0 },
  { "thundr", true,  sg_none,   96, 0, -1, -1, 0 },
  { "muck",   true,  sg_none,   32, 0, -1, -1, 0 },
  { "clrdef", true,  sg_none,   60, 0, -1, -1, 0 }, 
  { "dfdth",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "dwrfpn", true,  sg_none,   96, 0, -1, -1, 0 },
  { "dfsit1", true,  sg_none,   98, 0, -1, -1, 0 },
  { "dfsit2", true,  sg_none,   98, 0, -1, -1, 0 },
  { "fdsit",  true,  sg_none,   98, 0, -1, -1, 0 },
  { "fdpn",   true,  sg_none,   96, 0, -1, -1, 0 },
  { "fddth",  true,  sg_none,   70, 0, -1, -1, 0 },
  { "heal",   true,  sg_none,   60, 0, -1, -1, 0 },
  { "harp",   true,  sg_none,   60, 0, -1, -1, 0 },
  { "wofp",   true,  sg_none,   60, 0, -1, -1, 0 },
  { "nspdth", true,  sg_none,   90, 0, -1, -1, 0 },
  { "burn",   true,  sg_none,   32, 0, -1, -1, 0 },
  { "cone3",  true,  sg_none,   60, 0, -1, -1, 0 },
  { "icedth", true,  sg_none,   96, 0, -1, -1, 0 },
  { "plfeet", true,  sg_oof,    96, 0, -1, -1, 0, sk_plfeet+1 },
  { "plfall", true,  sg_none,   96, 0, -1, -1, 0, sk_plfall+1 },
  { "fallht", true,  sg_none,   96, 0, -1, -1, 0, sk_fallht+1 },
  { "clsit1", true,  sg_none,	92, 0, -1, -1, 0 },
  { "clsit2", true,  sg_none,	92, 0, -1, -1, 0 },
  { "clsit3", true,  sg_none,	92, 0, -1, -1, 0 },
  { "clsit4", true,  sg_none,	92, 0, -1, -1, 0 },
  // End Eternity TC New Sounds
};

*/

//----------------------------------------------------------------------------
//
// $Log: sounds.c,v $
// Revision 1.3  1998/05/03  22:44:25  killough
// beautification
//
// Revision 1.2  1998/01/26  19:24:54  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
