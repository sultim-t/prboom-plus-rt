// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2002 James Haley
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
// DeHackEd information tables / hashing
//
// Separated out from d_deh.c to avoid clutter and speed up the
// editing of that file. Routines for internal table hash chaining
// and initialization are also here.
//
//--------------------------------------------------------------------------

#include "z_zone.h"
#include "m_argv.h"
#include "m_fixed.h"
#include "d_io.h"
#include "doomdef.h"
#include "doomtype.h"
#include "dstrings.h"  // to get initial text values
#include "dhticstr.h"  // haleyjd
#include "d_dehtbl.h"
#include "m_queue.h"
#include "d_mod.h"

//
// Mod Strings: haleyjd -- stuck here because there's no better
// place for them ;)
//

const char *MODNames[NUM_MOD_TYPES] =
{
   "UNKNOWN",
   "FIST",
   "PISTOL",
   "SHOTGUN",
   "CHAINGUN",
   "ROCKET",
   "R_SPLASH",
   "PLASMA",
   "BFG",
   "BFG_SPLASH",
   "CHAINSAW",
   "SSHOTGUN",
   "SLIME",
   "LAVA",
   "CRUSH",
   "TELEFRAG",
   "FALLING",
   "SUICIDE",
   "BARREL",
   "SPLASH",
   "HIT",
   "BFG11K_SPLASH",
   "BETABFG",
   "BFGBURST",
   "PLAYERMISC",
   "GRENADE",
};

//
// Text Replacement
//

// #include "d_deh.h" -- we don't do that here but we declare the
// variables.  This externalizes everything that there is a string
// set for in the language files.  See d_deh.h for detailed comments,
// original English values etc.  These are set to the macro values,
// which are set by D_ENGLSH.H or D_FRENCH.H(etc).  BEX files are a
// better way of changing these strings globally by language.

// ============================================================
// Any of these can be changed using the bex extensions

char *s_D_DEVSTR    = D_DEVSTR;
char *s_D_CDROM     = D_CDROM;
char *s_PRESSKEY    = PRESSKEY;
char *s_PRESSYN     = PRESSYN;
char *s_QUITMSG     = "";    // sf: optional quitmsg replacement
char *s_LOADNET     = LOADNET;   // PRESSKEY; // killough 4/4/98:
char *s_QLOADNET    = QLOADNET;  // PRESSKEY;
char *s_QSAVESPOT   = QSAVESPOT; // PRESSKEY;
char *s_SAVEDEAD    = SAVEDEAD;  // PRESSKEY; // remove duplicate y/n
char *s_QSPROMPT    = QSPROMPT;  // PRESSYN;
char *s_QLPROMPT    = QLPROMPT;  // PRESSYN;
char *s_NEWGAME     = NEWGAME;   // PRESSKEY;
char *s_NIGHTMARE   = NIGHTMARE; // PRESSYN;
char *s_SWSTRING    = SWSTRING;  // PRESSKEY;
char *s_MSGOFF      = MSGOFF;
char *s_MSGON       = MSGON;
char *s_NETEND      = NETEND;    // PRESSKEY;
char *s_ENDGAME     = ENDGAME;   // PRESSYN; // killough 4/4/98: end
char *s_DOSY        = DOSY;
// joel - detail level strings ditched
char *s_GAMMALVL0   = GAMMALVL0;
char *s_GAMMALVL1   = GAMMALVL1;
char *s_GAMMALVL2   = GAMMALVL2;
char *s_GAMMALVL3   = GAMMALVL3;
char *s_GAMMALVL4   = GAMMALVL4;
char *s_EMPTYSTRING = EMPTYSTRING;
char *s_GOTARMOR    = GOTARMOR;
char *s_GOTMEGA     = GOTMEGA;
char *s_GOTHTHBONUS = GOTHTHBONUS;
char *s_GOTARMBONUS = GOTARMBONUS;
char *s_GOTSTIM     = GOTSTIM;
char *s_GOTMEDINEED = GOTMEDINEED;
char *s_GOTMEDIKIT  = GOTMEDIKIT;
char *s_GOTSUPER    = GOTSUPER;
char *s_GOTBLUECARD = GOTBLUECARD;
char *s_GOTYELWCARD = GOTYELWCARD;
char *s_GOTREDCARD  = GOTREDCARD;
char *s_GOTBLUESKUL = GOTBLUESKUL;
char *s_GOTYELWSKUL = GOTYELWSKUL;
char *s_GOTREDSKULL = GOTREDSKULL;
char *s_GOTINVUL    = GOTINVUL;
char *s_GOTBERSERK  = GOTBERSERK;
char *s_GOTINVIS    = GOTINVIS;
char *s_GOTSUIT     = GOTSUIT;
char *s_GOTMAP      = GOTMAP;
char *s_GOTVISOR    = GOTVISOR;
char *s_GOTMSPHERE  = GOTMSPHERE;
char *s_GOTCLIP     = GOTCLIP;
char *s_GOTCLIPBOX  = GOTCLIPBOX;
char *s_GOTROCKET   = GOTROCKET;
char *s_GOTROCKBOX  = GOTROCKBOX;
char *s_GOTCELL     = GOTCELL;
char *s_GOTCELLBOX  = GOTCELLBOX;
char *s_GOTSHELLS   = GOTSHELLS;
char *s_GOTSHELLBOX = GOTSHELLBOX;
char *s_GOTBACKPACK = GOTBACKPACK;
char *s_GOTBFG9000  = GOTBFG9000;
char *s_GOTCHAINGUN = GOTCHAINGUN;
char *s_GOTCHAINSAW = GOTCHAINSAW;
char *s_GOTLAUNCHER = GOTLAUNCHER;
char *s_GOTPLASMA   = GOTPLASMA;
char *s_GOTSHOTGUN  = GOTSHOTGUN;
char *s_GOTSHOTGUN2 = GOTSHOTGUN2;
char *s_PD_BLUEO    = PD_BLUEO;
char *s_PD_REDO     = PD_REDO;
char *s_PD_YELLOWO  = PD_YELLOWO;
char *s_PD_BLUEK    = PD_BLUEK;
char *s_PD_REDK     = PD_REDK;
char *s_PD_YELLOWK  = PD_YELLOWK;
char *s_PD_BLUEC    = PD_BLUEC;
char *s_PD_REDC     = PD_REDC;
char *s_PD_YELLOWC  = PD_YELLOWC;
char *s_PD_BLUES    = PD_BLUES;
char *s_PD_REDS     = PD_REDS;
char *s_PD_YELLOWS  = PD_YELLOWS;
char *s_PD_ANY      = PD_ANY;
char *s_PD_ALL3     = PD_ALL3;
char *s_PD_ALL6     = PD_ALL6;
char *s_GGSAVED     = GGSAVED;
char *s_HUSTR_MSGU  = HUSTR_MSGU;
char *s_HUSTR_E1M1  = HUSTR_E1M1;
char *s_HUSTR_E1M2  = HUSTR_E1M2;
char *s_HUSTR_E1M3  = HUSTR_E1M3;
char *s_HUSTR_E1M4  = HUSTR_E1M4;
char *s_HUSTR_E1M5  = HUSTR_E1M5;
char *s_HUSTR_E1M6  = HUSTR_E1M6;
char *s_HUSTR_E1M7  = HUSTR_E1M7;
char *s_HUSTR_E1M8  = HUSTR_E1M8;
char *s_HUSTR_E1M9  = HUSTR_E1M9;
char *s_HUSTR_E2M1  = HUSTR_E2M1;
char *s_HUSTR_E2M2  = HUSTR_E2M2;
char *s_HUSTR_E2M3  = HUSTR_E2M3;
char *s_HUSTR_E2M4  = HUSTR_E2M4;
char *s_HUSTR_E2M5  = HUSTR_E2M5;
char *s_HUSTR_E2M6  = HUSTR_E2M6;
char *s_HUSTR_E2M7  = HUSTR_E2M7;
char *s_HUSTR_E2M8  = HUSTR_E2M8;
char *s_HUSTR_E2M9  = HUSTR_E2M9;
char *s_HUSTR_E3M1  = HUSTR_E3M1;
char *s_HUSTR_E3M2  = HUSTR_E3M2;
char *s_HUSTR_E3M3  = HUSTR_E3M3;
char *s_HUSTR_E3M4  = HUSTR_E3M4;
char *s_HUSTR_E3M5  = HUSTR_E3M5;
char *s_HUSTR_E3M6  = HUSTR_E3M6;
char *s_HUSTR_E3M7  = HUSTR_E3M7;
char *s_HUSTR_E3M8  = HUSTR_E3M8;
char *s_HUSTR_E3M9  = HUSTR_E3M9;
char *s_HUSTR_E4M1  = HUSTR_E4M1;
char *s_HUSTR_E4M2  = HUSTR_E4M2;
char *s_HUSTR_E4M3  = HUSTR_E4M3;
char *s_HUSTR_E4M4  = HUSTR_E4M4;
char *s_HUSTR_E4M5  = HUSTR_E4M5;
char *s_HUSTR_E4M6  = HUSTR_E4M6;
char *s_HUSTR_E4M7  = HUSTR_E4M7;
char *s_HUSTR_E4M8  = HUSTR_E4M8;
char *s_HUSTR_E4M9  = HUSTR_E4M9;
char *s_HUSTR_1     = HUSTR_1;
char *s_HUSTR_2     = HUSTR_2;
char *s_HUSTR_3     = HUSTR_3;
char *s_HUSTR_4     = HUSTR_4;
char *s_HUSTR_5     = HUSTR_5;
char *s_HUSTR_6     = HUSTR_6;
char *s_HUSTR_7     = HUSTR_7;
char *s_HUSTR_8     = HUSTR_8;
char *s_HUSTR_9     = HUSTR_9;
char *s_HUSTR_10    = HUSTR_10;
char *s_HUSTR_11    = HUSTR_11;
char *s_HUSTR_12    = HUSTR_12;
char *s_HUSTR_13    = HUSTR_13;
char *s_HUSTR_14    = HUSTR_14;
char *s_HUSTR_15    = HUSTR_15;
char *s_HUSTR_16    = HUSTR_16;
char *s_HUSTR_17    = HUSTR_17;
char *s_HUSTR_18    = HUSTR_18;
char *s_HUSTR_19    = HUSTR_19;
char *s_HUSTR_20    = HUSTR_20;
char *s_HUSTR_21    = HUSTR_21;
char *s_HUSTR_22    = HUSTR_22;
char *s_HUSTR_23    = HUSTR_23;
char *s_HUSTR_24    = HUSTR_24;
char *s_HUSTR_25    = HUSTR_25;
char *s_HUSTR_26    = HUSTR_26;
char *s_HUSTR_27    = HUSTR_27;
char *s_HUSTR_28    = HUSTR_28;
char *s_HUSTR_29    = HUSTR_29;
char *s_HUSTR_30    = HUSTR_30;
char *s_HUSTR_31    = HUSTR_31;
char *s_HUSTR_32    = HUSTR_32;
char *s_PHUSTR_1    = PHUSTR_1;
char *s_PHUSTR_2    = PHUSTR_2;
char *s_PHUSTR_3    = PHUSTR_3;
char *s_PHUSTR_4    = PHUSTR_4;
char *s_PHUSTR_5    = PHUSTR_5;
char *s_PHUSTR_6    = PHUSTR_6;
char *s_PHUSTR_7    = PHUSTR_7;
char *s_PHUSTR_8    = PHUSTR_8;
char *s_PHUSTR_9    = PHUSTR_9;
char *s_PHUSTR_10   = PHUSTR_10;
char *s_PHUSTR_11   = PHUSTR_11;
char *s_PHUSTR_12   = PHUSTR_12;
char *s_PHUSTR_13   = PHUSTR_13;
char *s_PHUSTR_14   = PHUSTR_14;
char *s_PHUSTR_15   = PHUSTR_15;
char *s_PHUSTR_16   = PHUSTR_16;
char *s_PHUSTR_17   = PHUSTR_17;
char *s_PHUSTR_18   = PHUSTR_18;
char *s_PHUSTR_19   = PHUSTR_19;
char *s_PHUSTR_20   = PHUSTR_20;
char *s_PHUSTR_21   = PHUSTR_21;
char *s_PHUSTR_22   = PHUSTR_22;
char *s_PHUSTR_23   = PHUSTR_23;
char *s_PHUSTR_24   = PHUSTR_24;
char *s_PHUSTR_25   = PHUSTR_25;
char *s_PHUSTR_26   = PHUSTR_26;
char *s_PHUSTR_27   = PHUSTR_27;
char *s_PHUSTR_28   = PHUSTR_28;
char *s_PHUSTR_29   = PHUSTR_29;
char *s_PHUSTR_30   = PHUSTR_30;
char *s_PHUSTR_31   = PHUSTR_31;
char *s_PHUSTR_32   = PHUSTR_32;
char *s_THUSTR_1    = THUSTR_1;
char *s_THUSTR_2    = THUSTR_2;
char *s_THUSTR_3    = THUSTR_3;
char *s_THUSTR_4    = THUSTR_4;
char *s_THUSTR_5    = THUSTR_5;
char *s_THUSTR_6    = THUSTR_6;
char *s_THUSTR_7    = THUSTR_7;
char *s_THUSTR_8    = THUSTR_8;
char *s_THUSTR_9    = THUSTR_9;
char *s_THUSTR_10   = THUSTR_10;
char *s_THUSTR_11   = THUSTR_11;
char *s_THUSTR_12   = THUSTR_12;
char *s_THUSTR_13   = THUSTR_13;
char *s_THUSTR_14   = THUSTR_14;
char *s_THUSTR_15   = THUSTR_15;
char *s_THUSTR_16   = THUSTR_16;
char *s_THUSTR_17   = THUSTR_17;
char *s_THUSTR_18   = THUSTR_18;
char *s_THUSTR_19   = THUSTR_19;
char *s_THUSTR_20   = THUSTR_20;
char *s_THUSTR_21   = THUSTR_21;
char *s_THUSTR_22   = THUSTR_22;
char *s_THUSTR_23   = THUSTR_23;
char *s_THUSTR_24   = THUSTR_24;
char *s_THUSTR_25   = THUSTR_25;
char *s_THUSTR_26   = THUSTR_26;
char *s_THUSTR_27   = THUSTR_27;
char *s_THUSTR_28   = THUSTR_28;
char *s_THUSTR_29   = THUSTR_29;
char *s_THUSTR_30   = THUSTR_30;
char *s_THUSTR_31   = THUSTR_31;
char *s_THUSTR_32   = THUSTR_32;
char *s_HUSTR_CHATMACRO1   = HUSTR_CHATMACRO1;
char *s_HUSTR_CHATMACRO2   = HUSTR_CHATMACRO2;
char *s_HUSTR_CHATMACRO3   = HUSTR_CHATMACRO3;
char *s_HUSTR_CHATMACRO4   = HUSTR_CHATMACRO4;
char *s_HUSTR_CHATMACRO5   = HUSTR_CHATMACRO5;
char *s_HUSTR_CHATMACRO6   = HUSTR_CHATMACRO6;
char *s_HUSTR_CHATMACRO7   = HUSTR_CHATMACRO7;
char *s_HUSTR_CHATMACRO8   = HUSTR_CHATMACRO8;
char *s_HUSTR_CHATMACRO9   = HUSTR_CHATMACRO9;
char *s_HUSTR_CHATMACRO0   = HUSTR_CHATMACRO0;
char *s_HUSTR_TALKTOSELF1  = HUSTR_TALKTOSELF1;
char *s_HUSTR_TALKTOSELF2  = HUSTR_TALKTOSELF2;
char *s_HUSTR_TALKTOSELF3  = HUSTR_TALKTOSELF3;
char *s_HUSTR_TALKTOSELF4  = HUSTR_TALKTOSELF4;
char *s_HUSTR_TALKTOSELF5  = HUSTR_TALKTOSELF5;
char *s_HUSTR_MESSAGESENT  = HUSTR_MESSAGESENT;
char *s_HUSTR_PLRGREEN     = HUSTR_PLRGREEN;
char *s_HUSTR_PLRINDIGO    = HUSTR_PLRINDIGO;
char *s_HUSTR_PLRBROWN     = HUSTR_PLRBROWN;
char *s_HUSTR_PLRRED       = HUSTR_PLRRED;
char *s_AMSTR_FOLLOWON     = AMSTR_FOLLOWON;
char *s_AMSTR_FOLLOWOFF    = AMSTR_FOLLOWOFF;
char *s_AMSTR_GRIDON       = AMSTR_GRIDON;
char *s_AMSTR_GRIDOFF      = AMSTR_GRIDOFF;
char *s_AMSTR_MARKEDSPOT   = AMSTR_MARKEDSPOT;
char *s_AMSTR_MARKSCLEARED = AMSTR_MARKSCLEARED;
char *s_STSTR_MUS          = STSTR_MUS;
char *s_STSTR_NOMUS        = STSTR_NOMUS;
char *s_STSTR_DQDON        = STSTR_DQDON;
char *s_STSTR_DQDOFF       = STSTR_DQDOFF;
char *s_STSTR_KFAADDED     = STSTR_KFAADDED;
char *s_STSTR_FAADDED      = STSTR_FAADDED;
char *s_STSTR_NCON         = STSTR_NCON;
char *s_STSTR_NCOFF        = STSTR_NCOFF;
char *s_STSTR_BEHOLD       = STSTR_BEHOLD;
char *s_STSTR_BEHOLDX      = STSTR_BEHOLDX;
char *s_STSTR_CHOPPERS     = STSTR_CHOPPERS;
char *s_STSTR_CLEV         = STSTR_CLEV;
char *s_STSTR_COMPON       = STSTR_COMPON;
char *s_STSTR_COMPOFF      = STSTR_COMPOFF;
char *s_E1TEXT     = E1TEXT;
char *s_E2TEXT     = E2TEXT;
char *s_E3TEXT     = E3TEXT;
char *s_E4TEXT     = E4TEXT;
char *s_C1TEXT     = C1TEXT;
char *s_C2TEXT     = C2TEXT;
char *s_C3TEXT     = C3TEXT;
char *s_C4TEXT     = C4TEXT;
char *s_C5TEXT     = C5TEXT;
char *s_C6TEXT     = C6TEXT;
char *s_P1TEXT     = P1TEXT;
char *s_P2TEXT     = P2TEXT;
char *s_P3TEXT     = P3TEXT;
char *s_P4TEXT     = P4TEXT;
char *s_P5TEXT     = P5TEXT;
char *s_P6TEXT     = P6TEXT;
char *s_T1TEXT     = T1TEXT;
char *s_T2TEXT     = T2TEXT;
char *s_T3TEXT     = T3TEXT;
char *s_T4TEXT     = T4TEXT;
char *s_T5TEXT     = T5TEXT;
char *s_T6TEXT     = T6TEXT;
char *s_CC_ZOMBIE  = CC_ZOMBIE;
char *s_CC_SHOTGUN = CC_SHOTGUN;
char *s_CC_HEAVY   = CC_HEAVY;
char *s_CC_IMP     = CC_IMP;
char *s_CC_DEMON   = CC_DEMON;
char *s_CC_LOST    = CC_LOST;
char *s_CC_CACO    = CC_CACO;
char *s_CC_HELL    = CC_HELL;
char *s_CC_BARON   = CC_BARON;
char *s_CC_ARACH   = CC_ARACH;
char *s_CC_PAIN    = CC_PAIN;
char *s_CC_REVEN   = CC_REVEN;
char *s_CC_MANCU   = CC_MANCU;
char *s_CC_ARCH    = CC_ARCH;
char *s_CC_SPIDER  = CC_SPIDER;
char *s_CC_CYBER   = CC_CYBER;
char *s_CC_HERO    = CC_HERO;
// Ty 03/30/98 - new substitutions for background textures
char *bgflatE1     = "FLOOR4_8"; // end of DOOM Episode 1
char *bgflatE2     = "SFLR6_1";  // end of DOOM Episode 2
char *bgflatE3     = "MFLR8_4";  // end of DOOM Episode 3
char *bgflatE4     = "MFLR8_3";  // end of DOOM Episode 4
char *bgflat06     = "SLIME16";  // DOOM2 after MAP06
char *bgflat11     = "RROCK14";  // DOOM2 after MAP11
char *bgflat20     = "RROCK07";  // DOOM2 after MAP20
char *bgflat30     = "RROCK17";  // DOOM2 after MAP30
char *bgflat15     = "RROCK13";  // DOOM2 going MAP15 to MAP31
char *bgflat31     = "RROCK19";  // DOOM2 going MAP31 to MAP32
char *bgcastcall   = "BOSSBACK"; // Panel behind cast call
char *bgflathE1    = "FLOOR25";  // haleyjd: Heretic episode 1
char *bgflathE2    = "FLATHUH1"; // Heretic episode 2
char *bgflathE3    = "FLTWAWA2"; // Heretic episode 3
char *bgflathE4    = "FLOOR28";  // Heretic episode 4
char *bgflathE5    = "FLOOR08";  // Heretic episode 5

char *startup1     = "";  // blank lines are default and are not
char *startup2     = "";  // printed
char *startup3     = "";
char *startup4     = "";
char *startup5     = "";

// haleyjd: heretic strings
char *s_HHUSTR_E1M1 = HHUSTR_E1M1;
char *s_HHUSTR_E1M2 = HHUSTR_E1M2;
char *s_HHUSTR_E1M3 = HHUSTR_E1M3;
char *s_HHUSTR_E1M4 = HHUSTR_E1M4;
char *s_HHUSTR_E1M5 = HHUSTR_E1M5;
char *s_HHUSTR_E1M6 = HHUSTR_E1M6;
char *s_HHUSTR_E1M7 = HHUSTR_E1M7;
char *s_HHUSTR_E1M8 = HHUSTR_E1M8;
char *s_HHUSTR_E1M9 = HHUSTR_E1M9;
char *s_HHUSTR_E2M1 = HHUSTR_E2M1;
char *s_HHUSTR_E2M2 = HHUSTR_E2M2;
char *s_HHUSTR_E2M3 = HHUSTR_E2M3;
char *s_HHUSTR_E2M4 = HHUSTR_E2M4;
char *s_HHUSTR_E2M5 = HHUSTR_E2M5;
char *s_HHUSTR_E2M6 = HHUSTR_E2M6;
char *s_HHUSTR_E2M7 = HHUSTR_E2M7;
char *s_HHUSTR_E2M8 = HHUSTR_E2M8;
char *s_HHUSTR_E2M9 = HHUSTR_E2M9;
char *s_HHUSTR_E3M1 = HHUSTR_E3M1;
char *s_HHUSTR_E3M2 = HHUSTR_E3M2;
char *s_HHUSTR_E3M3 = HHUSTR_E3M3;
char *s_HHUSTR_E3M4 = HHUSTR_E3M4;
char *s_HHUSTR_E3M5 = HHUSTR_E3M5;
char *s_HHUSTR_E3M6 = HHUSTR_E3M6;
char *s_HHUSTR_E3M7 = HHUSTR_E3M7;
char *s_HHUSTR_E3M8 = HHUSTR_E3M8;
char *s_HHUSTR_E3M9 = HHUSTR_E3M9;
char *s_HHUSTR_E4M1 = HHUSTR_E4M1;
char *s_HHUSTR_E4M2 = HHUSTR_E4M2;
char *s_HHUSTR_E4M3 = HHUSTR_E4M3;
char *s_HHUSTR_E4M4 = HHUSTR_E4M4;
char *s_HHUSTR_E4M5 = HHUSTR_E4M5;
char *s_HHUSTR_E4M6 = HHUSTR_E4M6;
char *s_HHUSTR_E4M7 = HHUSTR_E4M7;
char *s_HHUSTR_E4M8 = HHUSTR_E4M8;
char *s_HHUSTR_E4M9 = HHUSTR_E4M9;
char *s_HHUSTR_E5M1 = HHUSTR_E5M1;
char *s_HHUSTR_E5M2 = HHUSTR_E5M2;
char *s_HHUSTR_E5M3 = HHUSTR_E5M3;
char *s_HHUSTR_E5M4 = HHUSTR_E5M4;
char *s_HHUSTR_E5M5 = HHUSTR_E5M5;
char *s_HHUSTR_E5M6 = HHUSTR_E5M6;
char *s_HHUSTR_E5M7 = HHUSTR_E5M7;
char *s_HHUSTR_E5M8 = HHUSTR_E5M8;
char *s_HHUSTR_E5M9 = HHUSTR_E5M9;
char *s_H1TEXT      = H1TEXT;
char *s_H2TEXT      = H2TEXT;
char *s_H3TEXT      = H3TEXT;
char *s_H4TEXT      = H4TEXT;
char *s_H5TEXT      = H5TEXT;
char *s_HGOTBLUEKEY = HGOTBLUEKEY;
char *s_HGOTYELLOWKEY     = HGOTYELLOWKEY;
char *s_HGOTGREENKEY      = HGOTGREENKEY;
char *s_HITEMHEALTH       = HITEMHEALTH;
char *s_HITEMSHIELD1      = HITEMSHIELD1;
char *s_HITEMSHIELD2      = HITEMSHIELD2;
char *s_HITEMBAGOFHOLDING = HITEMBAGOFHOLDING;
char *s_HITEMSUPERMAP     = HITEMSUPERMAP;
char *s_HPD_GREENO  = HPD_GREENO;
char *s_HPD_GREENK  = HPD_GREENK;
char *s_HAMMOGOLDWAND1 = HAMMOGOLDWAND1;
char *s_HAMMOGOLDWAND2 = HAMMOGOLDWAND2;
char *s_HAMMOMACE1     = HAMMOMACE1;
char *s_HAMMOMACE2     = HAMMOMACE2;
char *s_HAMMOCROSSBOW1 = HAMMOCROSSBOW1;
char *s_HAMMOCROSSBOW2 = HAMMOCROSSBOW2;
char *s_HAMMOBLASTER1  = HAMMOBLASTER1;
char *s_HAMMOBLASTER2  = HAMMOBLASTER2;
char *s_HAMMOSKULLROD1 = HAMMOSKULLROD1;
char *s_HAMMOSKULLROD2 = HAMMOSKULLROD2;
char *s_HAMMOPHOENIXROD1 = HAMMOPHOENIXROD1;
char *s_HAMMOPHOENIXROD2 = HAMMOPHOENIXROD2;

// obituaries
char *s_OB_SUICIDE = OB_SUICIDE;
char *s_OB_FALLING = OB_FALLING;
char *s_OB_CRUSH   = OB_CRUSH;
char *s_OB_LAVA    = OB_LAVA;
char *s_OB_SLIME   = OB_SLIME;
char *s_OB_BARREL  = OB_BARREL;
char *s_OB_SPLASH  = OB_SPLASH;
char *s_OB_COOP    = OB_COOP;
char *s_OB_DEFAULT = OB_DEFAULT;
char *s_OB_R_SPLASH_SELF = OB_R_SPLASH_SELF;
char *s_OB_ROCKET_SELF = OB_ROCKET_SELF;
char *s_OB_BFG11K_SELF = OB_BFG11K_SELF;
char *s_OB_FIST = OB_FIST;
char *s_OB_CHAINSAW = OB_CHAINSAW;
char *s_OB_PISTOL = OB_PISTOL;
char *s_OB_SHOTGUN = OB_SHOTGUN;
char *s_OB_SSHOTGUN = OB_SSHOTGUN;
char *s_OB_CHAINGUN = OB_CHAINGUN;
char *s_OB_ROCKET = OB_ROCKET;
char *s_OB_R_SPLASH = OB_R_SPLASH;
char *s_OB_PLASMA = OB_PLASMA;
char *s_OB_BFG = OB_BFG;
char *s_OB_BFG_SPLASH = OB_BFG_SPLASH;
char *s_OB_BETABFG = OB_BETABFG;
char *s_OB_BFGBURST = OB_BFGBURST;
char *s_OB_GRENADE_SELF = OB_GRENADE_SELF;
char *s_OB_GRENADE = OB_GRENADE;

// Ty 05/03/98 - externalized
char *savegamename;

// end d_deh.h variable declarations
// ============================================================

// Do this for a lookup--the pointer (loaded above) is 
// cross-referenced to a string key that is the same as the define 
// above.  We will use strdups to set these new values that we read 
// from the file, orphaning the original value set above.

deh_strs deh_strlookup[] = 
{
   {&s_D_DEVSTR,"D_DEVSTR"},
   {&s_D_CDROM,"D_CDROM"},
   {&s_PRESSKEY,"PRESSKEY"},
   {&s_PRESSYN,"PRESSYN"},
   {&s_QUITMSG,"QUITMSG"},
   {&s_LOADNET,"LOADNET"},
   {&s_QLOADNET,"QLOADNET"},
   {&s_QSAVESPOT,"QSAVESPOT"},
   {&s_SAVEDEAD,"SAVEDEAD"},
   // cph - disabled to prevent format string attacks
   // {&s_QSPROMPT,"QSPROMPT"},
   // {&s_QLPROMPT,"QLPROMPT"},
   {&s_NEWGAME,"NEWGAME"},
   {&s_NIGHTMARE,"NIGHTMARE"},
   {&s_SWSTRING,"SWSTRING"},
   {&s_MSGOFF,"MSGOFF"},
   {&s_MSGON,"MSGON"},
   {&s_NETEND,"NETEND"},
   {&s_ENDGAME,"ENDGAME"},
   {&s_DOSY,"DOSY"},
   // joel - detail level strings ditched
   {&s_GAMMALVL0,"GAMMALVL0"},
   {&s_GAMMALVL1,"GAMMALVL1"},
   {&s_GAMMALVL2,"GAMMALVL2"},
   {&s_GAMMALVL3,"GAMMALVL3"},
   {&s_GAMMALVL4,"GAMMALVL4"},
   {&s_EMPTYSTRING,"EMPTYSTRING"},
   {&s_GOTARMOR,"GOTARMOR"},
   {&s_GOTMEGA,"GOTMEGA"},
   {&s_GOTHTHBONUS,"GOTHTHBONUS"},
   {&s_GOTARMBONUS,"GOTARMBONUS"},
   {&s_GOTSTIM,"GOTSTIM"},
   {&s_GOTMEDINEED,"GOTMEDINEED"},
   {&s_GOTMEDIKIT,"GOTMEDIKIT"},
   {&s_GOTSUPER,"GOTSUPER"},
   {&s_GOTBLUECARD,"GOTBLUECARD"},
   {&s_GOTYELWCARD,"GOTYELWCARD"},
   {&s_GOTREDCARD,"GOTREDCARD"},
   {&s_GOTBLUESKUL,"GOTBLUESKUL"},
   {&s_GOTYELWSKUL,"GOTYELWSKUL"},
   {&s_GOTREDSKULL,"GOTREDSKULL"},
   {&s_GOTINVUL,"GOTINVUL"},
   {&s_GOTBERSERK,"GOTBERSERK"},
   {&s_GOTINVIS,"GOTINVIS"},
   {&s_GOTSUIT,"GOTSUIT"},
   {&s_GOTMAP,"GOTMAP"},
   {&s_GOTVISOR,"GOTVISOR"},
   {&s_GOTMSPHERE,"GOTMSPHERE"},
   {&s_GOTCLIP,"GOTCLIP"},
   {&s_GOTCLIPBOX,"GOTCLIPBOX"},
   {&s_GOTROCKET,"GOTROCKET"},
   {&s_GOTROCKBOX,"GOTROCKBOX"},
   {&s_GOTCELL,"GOTCELL"},
   {&s_GOTCELLBOX,"GOTCELLBOX"},
   {&s_GOTSHELLS,"GOTSHELLS"},
   {&s_GOTSHELLBOX,"GOTSHELLBOX"},
   {&s_GOTBACKPACK,"GOTBACKPACK"},
   {&s_GOTBFG9000,"GOTBFG9000"},
   {&s_GOTCHAINGUN,"GOTCHAINGUN"},
   {&s_GOTCHAINSAW,"GOTCHAINSAW"},
   {&s_GOTLAUNCHER,"GOTLAUNCHER"},
   {&s_GOTPLASMA,"GOTPLASMA"},
   {&s_GOTSHOTGUN,"GOTSHOTGUN"},
   {&s_GOTSHOTGUN2,"GOTSHOTGUN2"},
   {&s_PD_BLUEO,"PD_BLUEO"},
   {&s_PD_REDO,"PD_REDO"},
   {&s_PD_YELLOWO,"PD_YELLOWO"},
   {&s_PD_BLUEK,"PD_BLUEK"},
   {&s_PD_REDK,"PD_REDK"},
   {&s_PD_YELLOWK,"PD_YELLOWK"},
   {&s_PD_BLUEC,"PD_BLUEC"},
   {&s_PD_REDC,"PD_REDC"},
   {&s_PD_YELLOWC,"PD_YELLOWC"},
   {&s_PD_BLUES,"PD_BLUES"},
   {&s_PD_REDS,"PD_REDS"},
   {&s_PD_YELLOWS,"PD_YELLOWS"},
   {&s_PD_ANY,"PD_ANY"},
   {&s_PD_ALL3,"PD_ALL3"},
   {&s_PD_ALL6,"PD_ALL6"},
   {&s_GGSAVED,"GGSAVED"},
   {&s_HUSTR_MSGU,"HUSTR_MSGU"},
   {&s_HUSTR_E1M1,"HUSTR_E1M1"},
   {&s_HUSTR_E1M2,"HUSTR_E1M2"},
   {&s_HUSTR_E1M3,"HUSTR_E1M3"},
   {&s_HUSTR_E1M4,"HUSTR_E1M4"},
   {&s_HUSTR_E1M5,"HUSTR_E1M5"},
   {&s_HUSTR_E1M6,"HUSTR_E1M6"},
   {&s_HUSTR_E1M7,"HUSTR_E1M7"},
   {&s_HUSTR_E1M8,"HUSTR_E1M8"},
   {&s_HUSTR_E1M9,"HUSTR_E1M9"},
   {&s_HUSTR_E2M1,"HUSTR_E2M1"},
   {&s_HUSTR_E2M2,"HUSTR_E2M2"},
   {&s_HUSTR_E2M3,"HUSTR_E2M3"},
   {&s_HUSTR_E2M4,"HUSTR_E2M4"},
   {&s_HUSTR_E2M5,"HUSTR_E2M5"},
   {&s_HUSTR_E2M6,"HUSTR_E2M6"},
   {&s_HUSTR_E2M7,"HUSTR_E2M7"},
   {&s_HUSTR_E2M8,"HUSTR_E2M8"},
   {&s_HUSTR_E2M9,"HUSTR_E2M9"},
   {&s_HUSTR_E3M1,"HUSTR_E3M1"},
   {&s_HUSTR_E3M2,"HUSTR_E3M2"},
   {&s_HUSTR_E3M3,"HUSTR_E3M3"},
   {&s_HUSTR_E3M4,"HUSTR_E3M4"},
   {&s_HUSTR_E3M5,"HUSTR_E3M5"},
   {&s_HUSTR_E3M6,"HUSTR_E3M6"},
   {&s_HUSTR_E3M7,"HUSTR_E3M7"},
   {&s_HUSTR_E3M8,"HUSTR_E3M8"},
   {&s_HUSTR_E3M9,"HUSTR_E3M9"},
   {&s_HUSTR_E4M1,"HUSTR_E4M1"},
   {&s_HUSTR_E4M2,"HUSTR_E4M2"},
   {&s_HUSTR_E4M3,"HUSTR_E4M3"},
   {&s_HUSTR_E4M4,"HUSTR_E4M4"},
   {&s_HUSTR_E4M5,"HUSTR_E4M5"},
   {&s_HUSTR_E4M6,"HUSTR_E4M6"},
   {&s_HUSTR_E4M7,"HUSTR_E4M7"},
   {&s_HUSTR_E4M8,"HUSTR_E4M8"},
   {&s_HUSTR_E4M9,"HUSTR_E4M9"},
   {&s_HUSTR_1,"HUSTR_1"},
   {&s_HUSTR_2,"HUSTR_2"},
   {&s_HUSTR_3,"HUSTR_3"},
   {&s_HUSTR_4,"HUSTR_4"},
   {&s_HUSTR_5,"HUSTR_5"},
   {&s_HUSTR_6,"HUSTR_6"},
   {&s_HUSTR_7,"HUSTR_7"},
   {&s_HUSTR_8,"HUSTR_8"},
   {&s_HUSTR_9,"HUSTR_9"},
   {&s_HUSTR_10,"HUSTR_10"},
   {&s_HUSTR_11,"HUSTR_11"},
   {&s_HUSTR_12,"HUSTR_12"},
   {&s_HUSTR_13,"HUSTR_13"},
   {&s_HUSTR_14,"HUSTR_14"},
   {&s_HUSTR_15,"HUSTR_15"},
   {&s_HUSTR_16,"HUSTR_16"},
   {&s_HUSTR_17,"HUSTR_17"},
   {&s_HUSTR_18,"HUSTR_18"},
   {&s_HUSTR_19,"HUSTR_19"},
   {&s_HUSTR_20,"HUSTR_20"},
   {&s_HUSTR_21,"HUSTR_21"},
   {&s_HUSTR_22,"HUSTR_22"},
   {&s_HUSTR_23,"HUSTR_23"},
   {&s_HUSTR_24,"HUSTR_24"},
   {&s_HUSTR_25,"HUSTR_25"},
   {&s_HUSTR_26,"HUSTR_26"},
   {&s_HUSTR_27,"HUSTR_27"},
   {&s_HUSTR_28,"HUSTR_28"},
   {&s_HUSTR_29,"HUSTR_29"},
   {&s_HUSTR_30,"HUSTR_30"},
   {&s_HUSTR_31,"HUSTR_31"},
   {&s_HUSTR_32,"HUSTR_32"},
   {&s_PHUSTR_1,"PHUSTR_1"},
   {&s_PHUSTR_2,"PHUSTR_2"},
   {&s_PHUSTR_3,"PHUSTR_3"},
   {&s_PHUSTR_4,"PHUSTR_4"},
   {&s_PHUSTR_5,"PHUSTR_5"},
   {&s_PHUSTR_6,"PHUSTR_6"},
   {&s_PHUSTR_7,"PHUSTR_7"},
   {&s_PHUSTR_8,"PHUSTR_8"},
   {&s_PHUSTR_9,"PHUSTR_9"},
   {&s_PHUSTR_10,"PHUSTR_10"},
   {&s_PHUSTR_11,"PHUSTR_11"},
   {&s_PHUSTR_12,"PHUSTR_12"},
   {&s_PHUSTR_13,"PHUSTR_13"},
   {&s_PHUSTR_14,"PHUSTR_14"},
   {&s_PHUSTR_15,"PHUSTR_15"},
   {&s_PHUSTR_16,"PHUSTR_16"},
   {&s_PHUSTR_17,"PHUSTR_17"},
   {&s_PHUSTR_18,"PHUSTR_18"},
   {&s_PHUSTR_19,"PHUSTR_19"},
   {&s_PHUSTR_20,"PHUSTR_20"},
   {&s_PHUSTR_21,"PHUSTR_21"},
   {&s_PHUSTR_22,"PHUSTR_22"},
   {&s_PHUSTR_23,"PHUSTR_23"},
   {&s_PHUSTR_24,"PHUSTR_24"},
   {&s_PHUSTR_25,"PHUSTR_25"},
   {&s_PHUSTR_26,"PHUSTR_26"},
   {&s_PHUSTR_27,"PHUSTR_27"},
   {&s_PHUSTR_28,"PHUSTR_28"},
   {&s_PHUSTR_29,"PHUSTR_29"},
   {&s_PHUSTR_30,"PHUSTR_30"},
   {&s_PHUSTR_31,"PHUSTR_31"},
   {&s_PHUSTR_32,"PHUSTR_32"},
   {&s_THUSTR_1,"THUSTR_1"},
   {&s_THUSTR_2,"THUSTR_2"},
   {&s_THUSTR_3,"THUSTR_3"},
   {&s_THUSTR_4,"THUSTR_4"},
   {&s_THUSTR_5,"THUSTR_5"},
   {&s_THUSTR_6,"THUSTR_6"},
   {&s_THUSTR_7,"THUSTR_7"},
   {&s_THUSTR_8,"THUSTR_8"},
   {&s_THUSTR_9,"THUSTR_9"},
   {&s_THUSTR_10,"THUSTR_10"},
   {&s_THUSTR_11,"THUSTR_11"},
   {&s_THUSTR_12,"THUSTR_12"},
   {&s_THUSTR_13,"THUSTR_13"},
   {&s_THUSTR_14,"THUSTR_14"},
   {&s_THUSTR_15,"THUSTR_15"},
   {&s_THUSTR_16,"THUSTR_16"},
   {&s_THUSTR_17,"THUSTR_17"},
   {&s_THUSTR_18,"THUSTR_18"},
   {&s_THUSTR_19,"THUSTR_19"},
   {&s_THUSTR_20,"THUSTR_20"},
   {&s_THUSTR_21,"THUSTR_21"},
   {&s_THUSTR_22,"THUSTR_22"},
   {&s_THUSTR_23,"THUSTR_23"},
   {&s_THUSTR_24,"THUSTR_24"},
   {&s_THUSTR_25,"THUSTR_25"},
   {&s_THUSTR_26,"THUSTR_26"},
   {&s_THUSTR_27,"THUSTR_27"},
   {&s_THUSTR_28,"THUSTR_28"},
   {&s_THUSTR_29,"THUSTR_29"},
   {&s_THUSTR_30,"THUSTR_30"},
   {&s_THUSTR_31,"THUSTR_31"},
   {&s_THUSTR_32,"THUSTR_32"},
   {&s_HUSTR_CHATMACRO1,"HUSTR_CHATMACRO1"},
   {&s_HUSTR_CHATMACRO2,"HUSTR_CHATMACRO2"},
   {&s_HUSTR_CHATMACRO3,"HUSTR_CHATMACRO3"},
   {&s_HUSTR_CHATMACRO4,"HUSTR_CHATMACRO4"},
   {&s_HUSTR_CHATMACRO5,"HUSTR_CHATMACRO5"},
   {&s_HUSTR_CHATMACRO6,"HUSTR_CHATMACRO6"},
   {&s_HUSTR_CHATMACRO7,"HUSTR_CHATMACRO7"},
   {&s_HUSTR_CHATMACRO8,"HUSTR_CHATMACRO8"},
   {&s_HUSTR_CHATMACRO9,"HUSTR_CHATMACRO9"},
   {&s_HUSTR_CHATMACRO0,"HUSTR_CHATMACRO0"},
   {&s_HUSTR_TALKTOSELF1,"HUSTR_TALKTOSELF1"},
   {&s_HUSTR_TALKTOSELF2,"HUSTR_TALKTOSELF2"},
   {&s_HUSTR_TALKTOSELF3,"HUSTR_TALKTOSELF3"},
   {&s_HUSTR_TALKTOSELF4,"HUSTR_TALKTOSELF4"},
   {&s_HUSTR_TALKTOSELF5,"HUSTR_TALKTOSELF5"},
   {&s_HUSTR_MESSAGESENT,"HUSTR_MESSAGESENT"},
   {&s_HUSTR_PLRGREEN,"HUSTR_PLRGREEN"},
   {&s_HUSTR_PLRINDIGO,"HUSTR_PLRINDIGO"},
   {&s_HUSTR_PLRBROWN,"HUSTR_PLRBROWN"},
   {&s_HUSTR_PLRRED,"HUSTR_PLRRED"},
   // player chat keys removed
   {&s_AMSTR_FOLLOWON,"AMSTR_FOLLOWON"},
   {&s_AMSTR_FOLLOWOFF,"AMSTR_FOLLOWOFF"},
   {&s_AMSTR_GRIDON,"AMSTR_GRIDON"},
   {&s_AMSTR_GRIDOFF,"AMSTR_GRIDOFF"},
   {&s_AMSTR_MARKEDSPOT,"AMSTR_MARKEDSPOT"},
   {&s_AMSTR_MARKSCLEARED,"AMSTR_MARKSCLEARED"},
   {&s_STSTR_MUS,"STSTR_MUS"},
   {&s_STSTR_NOMUS,"STSTR_NOMUS"},
   {&s_STSTR_DQDON,"STSTR_DQDON"},
   {&s_STSTR_DQDOFF,"STSTR_DQDOFF"},
   {&s_STSTR_KFAADDED,"STSTR_KFAADDED"},
   {&s_STSTR_FAADDED,"STSTR_FAADDED"},
   {&s_STSTR_NCON,"STSTR_NCON"},
   {&s_STSTR_NCOFF,"STSTR_NCOFF"},
   {&s_STSTR_BEHOLD,"STSTR_BEHOLD"},
   {&s_STSTR_BEHOLDX,"STSTR_BEHOLDX"},
   {&s_STSTR_CHOPPERS,"STSTR_CHOPPERS"},
   {&s_STSTR_CLEV,"STSTR_CLEV"},
   {&s_STSTR_COMPON,"STSTR_COMPON"},
   {&s_STSTR_COMPOFF,"STSTR_COMPOFF"},
   {&s_E1TEXT,"E1TEXT"},
   {&s_E2TEXT,"E2TEXT"},
   {&s_E3TEXT,"E3TEXT"},
   {&s_E4TEXT,"E4TEXT"},
   {&s_C1TEXT,"C1TEXT"},
   {&s_C2TEXT,"C2TEXT"},
   {&s_C3TEXT,"C3TEXT"},
   {&s_C4TEXT,"C4TEXT"},
   {&s_C5TEXT,"C5TEXT"},
   {&s_C6TEXT,"C6TEXT"},
   {&s_P1TEXT,"P1TEXT"},
   {&s_P2TEXT,"P2TEXT"},
   {&s_P3TEXT,"P3TEXT"},
   {&s_P4TEXT,"P4TEXT"},
   {&s_P5TEXT,"P5TEXT"},
   {&s_P6TEXT,"P6TEXT"},
   {&s_T1TEXT,"T1TEXT"},
   {&s_T2TEXT,"T2TEXT"},
   {&s_T3TEXT,"T3TEXT"},
   {&s_T4TEXT,"T4TEXT"},
   {&s_T5TEXT,"T5TEXT"},
   {&s_T6TEXT,"T6TEXT"},
   {&s_CC_ZOMBIE,"CC_ZOMBIE"},
   {&s_CC_SHOTGUN,"CC_SHOTGUN"},
   {&s_CC_HEAVY,"CC_HEAVY"},
   {&s_CC_IMP,"CC_IMP"},
   {&s_CC_DEMON,"CC_DEMON"},
   {&s_CC_LOST,"CC_LOST"},
   {&s_CC_CACO,"CC_CACO"},
   {&s_CC_HELL,"CC_HELL"},
   {&s_CC_BARON,"CC_BARON"},
   {&s_CC_ARACH,"CC_ARACH"},
   {&s_CC_PAIN,"CC_PAIN"},
   {&s_CC_REVEN,"CC_REVEN"},
   {&s_CC_MANCU,"CC_MANCU"},
   {&s_CC_ARCH,"CC_ARCH"},
   {&s_CC_SPIDER,"CC_SPIDER"},
   {&s_CC_CYBER,"CC_CYBER"},
   {&s_CC_HERO,"CC_HERO"},
   {&bgflatE1,"BGFLATE1"},
   {&bgflatE2,"BGFLATE2"},
   {&bgflatE3,"BGFLATE3"},
   {&bgflatE4,"BGFLATE4"},
   {&bgflat06,"BGFLAT06"},
   {&bgflat11,"BGFLAT11"},
   {&bgflat20,"BGFLAT20"},
   {&bgflat30,"BGFLAT30"},
   {&bgflat15,"BGFLAT15"},
   {&bgflat31,"BGFLAT31"},
   {&bgcastcall,"BGCASTCALL"},
   // Ty 04/08/98 - added 5 general purpose startup announcement
   // strings for hacker use.
   {&startup1,"STARTUP1"},
   {&startup2,"STARTUP2"},
   {&startup3,"STARTUP3"},
   {&startup4,"STARTUP4"},
   {&startup5,"STARTUP5"},
   {&savegamename,"SAVEGAMENAME"},  // Ty 05/03/98
   // haleyjd: Eternity and Heretic strings follow
   {&bgflathE1,"BGFLATHE1"},
   {&bgflathE2,"BGFLATHE2"},
   {&bgflathE3,"BGFLATHE3"},
   {&bgflathE4,"BGFLATHE4"},
   {&bgflathE5,"BGFLATHE5"},
   {&s_HHUSTR_E1M1,"HHUSTR_E1M1"},
   {&s_HHUSTR_E1M2,"HHUSTR_E1M2"},
   {&s_HHUSTR_E1M3,"HHUSTR_E1M3"},
   {&s_HHUSTR_E1M4,"HHUSTR_E1M4"},
   {&s_HHUSTR_E1M5,"HHUSTR_E1M5"},
   {&s_HHUSTR_E1M6,"HHUSTR_E1M6"},
   {&s_HHUSTR_E1M7,"HHUSTR_E1M7"},
   {&s_HHUSTR_E1M8,"HHUSTR_E1M8"},
   {&s_HHUSTR_E1M9,"HHUSTR_E1M9"},
   {&s_HHUSTR_E2M1,"HHUSTR_E2M1"},
   {&s_HHUSTR_E2M2,"HHUSTR_E2M2"},
   {&s_HHUSTR_E2M3,"HHUSTR_E2M3"},
   {&s_HHUSTR_E2M4,"HHUSTR_E2M4"},
   {&s_HHUSTR_E2M5,"HHUSTR_E2M5"},
   {&s_HHUSTR_E2M6,"HHUSTR_E2M6"},
   {&s_HHUSTR_E2M7,"HHUSTR_E2M7"},
   {&s_HHUSTR_E2M8,"HHUSTR_E2M8"},
   {&s_HHUSTR_E2M9,"HHUSTR_E2M9"},
   {&s_HHUSTR_E3M1,"HHUSTR_E3M1"},
   {&s_HHUSTR_E3M2,"HHUSTR_E3M2"},
   {&s_HHUSTR_E3M3,"HHUSTR_E3M3"},
   {&s_HHUSTR_E3M4,"HHUSTR_E3M4"},
   {&s_HHUSTR_E3M5,"HHUSTR_E3M5"},
   {&s_HHUSTR_E3M6,"HHUSTR_E3M6"},
   {&s_HHUSTR_E3M7,"HHUSTR_E3M7"},
   {&s_HHUSTR_E3M8,"HHUSTR_E3M8"},
   {&s_HHUSTR_E3M9,"HHUSTR_E3M9"},
   {&s_HHUSTR_E4M1,"HHUSTR_E4M1"},
   {&s_HHUSTR_E4M2,"HHUSTR_E4M2"},
   {&s_HHUSTR_E4M3,"HHUSTR_E4M3"},
   {&s_HHUSTR_E4M4,"HHUSTR_E4M4"},
   {&s_HHUSTR_E4M5,"HHUSTR_E4M5"},
   {&s_HHUSTR_E4M6,"HHUSTR_E4M6"},
   {&s_HHUSTR_E4M7,"HHUSTR_E4M7"},
   {&s_HHUSTR_E4M8,"HHUSTR_E4M8"},
   {&s_HHUSTR_E4M9,"HHUSTR_E4M9"},
   {&s_HHUSTR_E5M1,"HHUSTR_E5M1"},
   {&s_HHUSTR_E5M2,"HHUSTR_E5M2"},
   {&s_HHUSTR_E5M3,"HHUSTR_E5M3"},
   {&s_HHUSTR_E5M4,"HHUSTR_E5M4"},
   {&s_HHUSTR_E5M5,"HHUSTR_E5M5"},
   {&s_HHUSTR_E5M6,"HHUSTR_E5M6"},
   {&s_HHUSTR_E5M7,"HHUSTR_E5M7"},
   {&s_HHUSTR_E5M8,"HHUSTR_E5M8"},
   {&s_HHUSTR_E5M9,"HHUSTR_E5M9"},
   {&s_H1TEXT,"H1TEXT"},
   {&s_H2TEXT,"H2TEXT"},
   {&s_H3TEXT,"H3TEXT"},
   {&s_H4TEXT,"H4TEXT"},
   {&s_H5TEXT,"H5TEXT"},
   {&s_HGOTBLUEKEY,"HGOTBLUEKEY"},
   {&s_HGOTYELLOWKEY,"HGOTYELLOWKEY"},
   {&s_HGOTGREENKEY,"HGOTGREENKEY"},
   {&s_HITEMHEALTH,"HITEMHEALTH"},
   {&s_HITEMSHIELD1,"HITEMSHIELD1"},
   {&s_HITEMSHIELD2,"HITEMSHIELD2"},
   {&s_HITEMBAGOFHOLDING,"HITEMBAG"},
   {&s_HITEMSUPERMAP,"HITEMSUPERMAP"},
   {&s_HAMMOGOLDWAND1,"HAMMOGOLDWAND1"},
   {&s_HAMMOGOLDWAND2,"HAMMOGOLDWAND2"},
   {&s_HAMMOMACE1,"HAMMOMACE1"},
   {&s_HAMMOMACE2,"HAMMOMACE2"},
   {&s_HAMMOCROSSBOW1,"HAMMOCROSSBOW1"},
   {&s_HAMMOCROSSBOW2,"HAMMOCROSSBOW2"},
   {&s_HAMMOBLASTER1,"HAMMOBLASTER1"},
   {&s_HAMMOBLASTER2,"HAMMOBLASTER2"},
   {&s_HAMMOSKULLROD1,"HAMMOSKULLROD1"},
   {&s_HAMMOSKULLROD2,"HAMMOSKULLROD2"},
   {&s_HAMMOPHOENIXROD1,"HAMMOPHOENIXROD1"},
   {&s_HAMMOPHOENIXROD2,"HAMMOPHOENIXROD2"},
   {&s_HPD_GREENO,"HPD_GREENO"},
   {&s_HPD_GREENK,"HPD_GREENK"},
   {&s_OB_SUICIDE,"OB_SUICIDE"},
   {&s_OB_FALLING,"OB_FALLING"},
   {&s_OB_CRUSH,"OB_CRUSH"},
   {&s_OB_LAVA, "OB_LAVA"},
   {&s_OB_SLIME,"OB_SLIME"},
   {&s_OB_BARREL,"OB_BARREL"},
   {&s_OB_SPLASH,"OB_SPLASH"},
   {&s_OB_COOP,"OB_COOP"},
   {&s_OB_DEFAULT,"OB_DEFAULT"},
   {&s_OB_R_SPLASH_SELF,"OB_R_SPLASH_SELF"},
   {&s_OB_ROCKET_SELF,"OB_ROCKET_SELF"},
   {&s_OB_BFG11K_SELF,"OB_BFG11K_SELF"},
   {&s_OB_FIST,"OB_FIST"},
   {&s_OB_CHAINSAW,"OB_CHAINSAW"},
   {&s_OB_PISTOL,"OB_PISTOL"},
   {&s_OB_SHOTGUN,"OB_SHOTGUN"},
   {&s_OB_SSHOTGUN,"OB_SSHOTGUN"},
   {&s_OB_CHAINGUN,"OB_CHAINGUN"},
   {&s_OB_ROCKET,"OB_ROCKET"},
   {&s_OB_R_SPLASH,"OB_R_SPLASH"},
   {&s_OB_PLASMA,"OB_PLASMA"},
   {&s_OB_BFG,"OB_BFG"},
   {&s_OB_BFG_SPLASH,"OB_BFG_SPLASH"},
   {&s_OB_BETABFG,"OB_BETABFG"},
   {&s_OB_BFGBURST,"OB_BFGBURST"},
   {&s_OB_GRENADE_SELF,"OB_GRENADE_SELF"},
   {&s_OB_GRENADE,"OB_GRENADE"},
};

static int deh_numstrlookup = sizeof(deh_strlookup)/sizeof(deh_strs);

// level name tables

char *deh_newlevel = "NEWLEVEL";

char **mapnames[] =  // DOOM shareware/registered/retail (Ultimate) names.
{
  &s_HUSTR_E1M1,
  &s_HUSTR_E1M2,
  &s_HUSTR_E1M3,
  &s_HUSTR_E1M4,
  &s_HUSTR_E1M5,
  &s_HUSTR_E1M6,
  &s_HUSTR_E1M7,
  &s_HUSTR_E1M8,
  &s_HUSTR_E1M9,

  &s_HUSTR_E2M1,
  &s_HUSTR_E2M2,
  &s_HUSTR_E2M3,
  &s_HUSTR_E2M4,
  &s_HUSTR_E2M5,
  &s_HUSTR_E2M6,
  &s_HUSTR_E2M7,
  &s_HUSTR_E2M8,
  &s_HUSTR_E2M9,

  &s_HUSTR_E3M1,
  &s_HUSTR_E3M2,
  &s_HUSTR_E3M3,
  &s_HUSTR_E3M4,
  &s_HUSTR_E3M5,
  &s_HUSTR_E3M6,
  &s_HUSTR_E3M7,
  &s_HUSTR_E3M8,
  &s_HUSTR_E3M9,

  &s_HUSTR_E4M1,
  &s_HUSTR_E4M2,
  &s_HUSTR_E4M3,
  &s_HUSTR_E4M4,
  &s_HUSTR_E4M5,
  &s_HUSTR_E4M6,
  &s_HUSTR_E4M7,
  &s_HUSTR_E4M8,
  &s_HUSTR_E4M9,

  &deh_newlevel,  // spares?  Unused.
  &deh_newlevel,
  &deh_newlevel,
  &deh_newlevel,
  &deh_newlevel,
  &deh_newlevel,
  &deh_newlevel,
  &deh_newlevel,
  &deh_newlevel
};

char **mapnames2[] = // DOOM 2 map names.
{
  &s_HUSTR_1,
  &s_HUSTR_2,
  &s_HUSTR_3,
  &s_HUSTR_4,
  &s_HUSTR_5,
  &s_HUSTR_6,
  &s_HUSTR_7,
  &s_HUSTR_8,
  &s_HUSTR_9,
  &s_HUSTR_10,
  &s_HUSTR_11,

  &s_HUSTR_12,
  &s_HUSTR_13,
  &s_HUSTR_14,
  &s_HUSTR_15,
  &s_HUSTR_16,
  &s_HUSTR_17,
  &s_HUSTR_18,
  &s_HUSTR_19,
  &s_HUSTR_20,

  &s_HUSTR_21,
  &s_HUSTR_22,
  &s_HUSTR_23,
  &s_HUSTR_24,
  &s_HUSTR_25,
  &s_HUSTR_26,
  &s_HUSTR_27,
  &s_HUSTR_28,
  &s_HUSTR_29,
  &s_HUSTR_30,
  &s_HUSTR_31,
  &s_HUSTR_32,
};

char **mapnamesp[] = // Plutonia WAD map names.
{
  &s_PHUSTR_1,
  &s_PHUSTR_2,
  &s_PHUSTR_3,
  &s_PHUSTR_4,
  &s_PHUSTR_5,
  &s_PHUSTR_6,
  &s_PHUSTR_7,
  &s_PHUSTR_8,
  &s_PHUSTR_9,
  &s_PHUSTR_10,
  &s_PHUSTR_11,

  &s_PHUSTR_12,
  &s_PHUSTR_13,
  &s_PHUSTR_14,
  &s_PHUSTR_15,
  &s_PHUSTR_16,
  &s_PHUSTR_17,
  &s_PHUSTR_18,
  &s_PHUSTR_19,
  &s_PHUSTR_20,

  &s_PHUSTR_21,
  &s_PHUSTR_22,
  &s_PHUSTR_23,
  &s_PHUSTR_24,
  &s_PHUSTR_25,
  &s_PHUSTR_26,
  &s_PHUSTR_27,
  &s_PHUSTR_28,
  &s_PHUSTR_29,
  &s_PHUSTR_30,
  &s_PHUSTR_31,
  &s_PHUSTR_32,
};

char **mapnamest[] = // TNT WAD map names.
{
  &s_THUSTR_1,
  &s_THUSTR_2,
  &s_THUSTR_3,
  &s_THUSTR_4,
  &s_THUSTR_5,
  &s_THUSTR_6,
  &s_THUSTR_7,
  &s_THUSTR_8,
  &s_THUSTR_9,
  &s_THUSTR_10,
  &s_THUSTR_11,

  &s_THUSTR_12,
  &s_THUSTR_13,
  &s_THUSTR_14,
  &s_THUSTR_15,
  &s_THUSTR_16,
  &s_THUSTR_17,
  &s_THUSTR_18,
  &s_THUSTR_19,
  &s_THUSTR_20,

  &s_THUSTR_21,
  &s_THUSTR_22,
  &s_THUSTR_23,
  &s_THUSTR_24,
  &s_THUSTR_25,
  &s_THUSTR_26,
  &s_THUSTR_27,
  &s_THUSTR_28,
  &s_THUSTR_29,
  &s_THUSTR_30,
  &s_THUSTR_31,
  &s_THUSTR_32,
};

char **mapnamesh[] = // haleyjd: heretic map names
{
   &s_HHUSTR_E1M1,
   &s_HHUSTR_E1M2,
   &s_HHUSTR_E1M3,
   &s_HHUSTR_E1M4,
   &s_HHUSTR_E1M5,
   &s_HHUSTR_E1M6,
   &s_HHUSTR_E1M7,
   &s_HHUSTR_E1M8,
   &s_HHUSTR_E1M9,

   &s_HHUSTR_E2M1,
   &s_HHUSTR_E2M2,
   &s_HHUSTR_E2M3,
   &s_HHUSTR_E2M4,
   &s_HHUSTR_E2M5,
   &s_HHUSTR_E2M6,
   &s_HHUSTR_E2M7,
   &s_HHUSTR_E2M8,
   &s_HHUSTR_E2M9,

   &s_HHUSTR_E3M1,
   &s_HHUSTR_E3M2,
   &s_HHUSTR_E3M3,
   &s_HHUSTR_E3M4,
   &s_HHUSTR_E3M5,
   &s_HHUSTR_E3M6,
   &s_HHUSTR_E3M7,
   &s_HHUSTR_E3M8,
   &s_HHUSTR_E3M9,

   &s_HHUSTR_E4M1,
   &s_HHUSTR_E4M2,
   &s_HHUSTR_E4M3,
   &s_HHUSTR_E4M4,
   &s_HHUSTR_E4M5,
   &s_HHUSTR_E4M6,
   &s_HHUSTR_E4M7,
   &s_HHUSTR_E4M8,
   &s_HHUSTR_E4M9,

   &s_HHUSTR_E5M1,
   &s_HHUSTR_E5M2,
   &s_HHUSTR_E5M3,
   &s_HHUSTR_E5M4,
   &s_HHUSTR_E5M5,
   &s_HHUSTR_E5M6,
   &s_HHUSTR_E5M7,
   &s_HHUSTR_E5M8,
   &s_HHUSTR_E5M9,
};


//
// BEX Codepointers
//

// External references to action functions scattered about the code

extern void A_Light0();
extern void A_WeaponReady();
extern void A_Lower();
extern void A_Raise();
extern void A_Punch();
extern void A_ReFire();
extern void A_FirePistol();
extern void A_Light1();
extern void A_FireShotgun();
extern void A_Light2();
extern void A_FireShotgun2();
extern void A_CheckReload();
extern void A_OpenShotgun2();
extern void A_LoadShotgun2();
extern void A_CloseShotgun2();
extern void A_FireCGun();
extern void A_GunFlash();
extern void A_FireMissile();
extern void A_Saw();
extern void A_FirePlasma();
extern void A_BFGsound();
extern void A_FireBFG();
extern void A_BFGSpray();
extern void A_Explode();
extern void A_Pain();
extern void A_PlayerScream();
extern void A_Fall();
extern void A_XScream();
extern void A_Look();
extern void A_Chase();
extern void A_FaceTarget();
extern void A_PosAttack();
extern void A_Scream();
extern void A_SPosAttack();
extern void A_VileChase();
extern void A_VileStart();
extern void A_VileTarget();
extern void A_VileAttack();
extern void A_StartFire();
extern void A_Fire();
extern void A_FireCrackle();
extern void A_Tracer();
extern void A_SkelWhoosh();
extern void A_SkelFist();
extern void A_SkelMissile();
extern void A_FatRaise();
extern void A_FatAttack1();
extern void A_FatAttack2();
extern void A_FatAttack3();
extern void A_BossDeath();
extern void A_CPosAttack();
extern void A_CPosRefire();
extern void A_TroopAttack();
extern void A_SargAttack();
extern void A_HeadAttack();
extern void A_BruisAttack();
extern void A_SkullAttack();
extern void A_Metal();
extern void A_SpidRefire();
extern void A_BabyMetal();
extern void A_BspiAttack();
extern void A_Hoof();
extern void A_CyberAttack();
extern void A_PainAttack();
extern void A_PainDie();
extern void A_KeenDie();
extern void A_BrainPain();
extern void A_BrainScream();
extern void A_BrainDie();
extern void A_BrainAwake();
extern void A_BrainSpit();
extern void A_SpawnSound();
extern void A_SpawnFly();
extern void A_BrainExplode();
extern void A_Detonate();        // killough 8/9/98
extern void A_Mushroom();        // killough 10/98
extern void A_Die();             // killough 11/98
extern void A_Spawn();           // killough 11/98
extern void A_Turn();            // killough 11/98
extern void A_Face();            // killough 11/98
extern void A_Scratch();         // killough 11/98
extern void A_PlaySound();       // killough 11/98
extern void A_RandomJump();      // killough 11/98
extern void A_LineEffect();      // killough 11/98
extern void A_Nailbomb();

// haleyjd: start new eternity action functions
extern void A_SetFlags();
extern void A_UnSetFlags();
extern void A_BetaSkullAttack();
extern void A_StartScript();        // haleyjd 1/25/00: Script wrapper
extern void A_PlayerStartScript();
extern void A_FireGrenade();
extern void A_FireCustomBullets();
extern void A_FirePlayerMissile();
extern void A_CustomPlayerMelee();
extern void A_GenTracer();
extern void A_BFG11KHit();
extern void A_BouncingBFG();
extern void A_BFGBurst();
extern void A_FireOldBFG();
extern void A_KeepChasing();
extern void A_Stop();
extern void A_PlayerThunk();
extern void A_MissileAttack();
extern void A_MissileSpread();
extern void A_BulletAttack();
extern void A_HealthJump();
extern void A_CounterJump();
extern void A_CounterSwitch();
extern void A_SetCounter();
extern void A_CopyCounter();
extern void A_CounterOp();
extern void A_SetTics();
extern void A_AproxDistance();
extern void A_ShowMessage();
extern void A_RandomWalk();
extern void A_TargetJump();
extern void A_ThingSummon();
extern void A_KillChildren();

// haleyjd 10/12/02: Heretic pointers
extern void A_SpawnGlitter();
extern void A_AccelGlitter();
extern void A_SpawnAbove();
extern void A_MummyAttack();
extern void A_MummyAttack2();
extern void A_MummySoul();
extern void A_HticDrop();
extern void A_HticTracer();
extern void A_ClinkAttack();
extern void A_WizardAtk1();
extern void A_WizardAtk2();
extern void A_WizardAtk3();
extern void A_Srcr2Decide();
extern void A_Srcr2Attack();
extern void A_BlueSpark();
extern void A_GenWizard();
extern void A_Sor2DthInit();
extern void A_Sor2DthLoop();
extern void A_HticExplode();
extern void A_HticBossDeath();
extern void A_PodPain();
extern void A_RemovePod();
extern void A_MakePod();
extern void A_KnightAttack();
extern void A_DripBlood();
extern void A_BeastAttack();
extern void A_BeastPuff();
extern void A_SnakeAttack();
extern void A_SnakeAttack2();
extern void A_Sor1Chase();
extern void A_Sor1Pain();
extern void A_Srcr1Attack();
extern void A_SorcererRise();
extern void A_VolcanoBlast();
extern void A_VolcBallImpact();
extern void A_MinotaurAtk1();
extern void A_MinotaurDecide();
extern void A_MinotaurAtk2();
extern void A_MinotaurAtk3();
extern void A_MinotaurCharge();
extern void A_MntrFloorFire();
extern void A_LichFire();
extern void A_LichWhirlwind();
extern void A_LichAttack();
extern void A_WhirlwindSeek();
extern void A_LichIceImpact();
extern void A_LichFireGrow();
extern void A_ImpChargeAtk();
extern void A_ImpMeleeAtk();
extern void A_ImpMissileAtk();
extern void A_ImpDeath();
extern void A_ImpXDeath1();
extern void A_ImpXDeath2();
extern void A_ImpExplode();

// eternity tc ptrs: TODO: remove these?
extern void A_ClericAtk();
extern void A_FogSpawn();
extern void A_FogMove();
extern void A_Cleric2Chase();
extern void A_Cleric2Decide();
extern void A_Cleric2Attack();
extern void A_ClericBreak();
extern void A_DwarfDie();
extern void A_CyberGuardSigh();
extern void A_DwarfLDOCMagic();
extern void A_DwarfFWAEMagic();
extern void A_DwarfAlterEgoChase();
extern void A_DwarfAlterEgoAttack();
extern void A_PhoenixTracer();

// haleyjd 07/13/03: special death actions for killem cheat
extern void A_PainNukeSpec();
extern void A_SorcNukeSpec();

// haleyjd 03/14/03: moved here, added hashing, eliminated useless
// A_ prefixes on mnemonics

deh_bexptr deh_bexptrs[] =
{
  {A_Light0,         "Light0"},
  {A_WeaponReady,    "WeaponReady"},
  {A_Lower,          "Lower"},
  {A_Raise,          "Raise"},
  {A_Punch,          "Punch"},
  {A_ReFire,         "ReFire"},
  {A_FirePistol,     "FirePistol"},
  {A_Light1,         "Light1"},
  {A_FireShotgun,    "FireShotgun"},
  {A_Light2,         "Light2"},
  {A_FireShotgun2,   "FireShotgun2"},
  {A_CheckReload,    "CheckReload"},
  {A_OpenShotgun2,   "OpenShotgun2"},
  {A_LoadShotgun2,   "LoadShotgun2"},
  {A_CloseShotgun2,  "CloseShotgun2"},
  {A_FireCGun,       "FireCGun"},
  {A_GunFlash,       "GunFlash"},
  {A_FireMissile,    "FireMissile"},
  {A_Saw,            "Saw"},
  {A_FirePlasma,     "FirePlasma"},
  {A_BFGsound,       "BFGsound"},
  {A_FireBFG,        "FireBFG"},
  {A_BFGSpray,       "BFGSpray"},
  {A_Explode,        "Explode",    BPF_PTHUNK},
  {A_Pain,           "Pain",       BPF_PTHUNK},
  {A_PlayerScream,   "PlayerScream", BPF_PTHUNK},
  {A_Fall,           "Fall",       BPF_PTHUNK},
  {A_XScream,        "XScream",    BPF_PTHUNK},
  {A_Look,           "Look"},
  {A_Chase,          "Chase"},
  {A_FaceTarget,     "FaceTarget", BPF_PTHUNK},
  {A_PosAttack,      "PosAttack",  BPF_PTHUNK},
  {A_Scream,         "Scream",     BPF_PTHUNK},
  {A_SPosAttack,     "SPosAttack", BPF_PTHUNK},
  {A_VileChase,      "VileChase"},
  {A_VileStart,      "VileStart",  BPF_PTHUNK},
  {A_VileTarget,     "VileTarget"},
  {A_VileAttack,     "VileAttack", BPF_PTHUNK},
  {A_StartFire,      "StartFire"},
  {A_Fire,           "Fire"},
  {A_FireCrackle,    "FireCrackle"},
  {A_Tracer,         "Tracer"},
  {A_SkelWhoosh,     "SkelWhoosh", BPF_PTHUNK},
  {A_SkelFist,       "SkelFist",   BPF_PTHUNK},
  {A_SkelMissile,    "SkelMissile",BPF_PTHUNK},
  {A_FatRaise,       "FatRaise",   BPF_PTHUNK},
  {A_FatAttack1,     "FatAttack1", BPF_PTHUNK},
  {A_FatAttack2,     "FatAttack2", BPF_PTHUNK},
  {A_FatAttack3,     "FatAttack3", BPF_PTHUNK},
  {A_BossDeath,      "BossDeath"},
  {A_CPosAttack,     "CPosAttack", BPF_PTHUNK},
  {A_CPosRefire,     "CPosRefire", BPF_PTHUNK},
  {A_TroopAttack,    "TroopAttack",BPF_PTHUNK},
  {A_SargAttack,     "SargAttack", BPF_PTHUNK},
  {A_HeadAttack,     "HeadAttack", BPF_PTHUNK},
  {A_BruisAttack,    "BruisAttack",BPF_PTHUNK},
  {A_SkullAttack,    "SkullAttack",BPF_PTHUNK},
  {A_Metal,          "Metal"},
  {A_SpidRefire,     "SpidRefire", BPF_PTHUNK},
  {A_BabyMetal,      "BabyMetal"},
  {A_BspiAttack,     "BspiAttack", BPF_PTHUNK},
  {A_Hoof,           "Hoof"},
  {A_CyberAttack,    "CyberAttack",BPF_PTHUNK},
  {A_PainAttack,     "PainAttack", BPF_PTHUNK},
  {A_PainDie,        "PainDie",    BPF_PTHUNK},
  {A_KeenDie,        "KeenDie"},
  {A_BrainPain,      "BrainPain",  BPF_PTHUNK},
  {A_BrainScream,    "BrainScream",BPF_PTHUNK},
  {A_BrainDie,       "BrainDie",   BPF_PTHUNK},
  {A_BrainAwake,     "BrainAwake", BPF_PTHUNK},
  {A_BrainSpit,      "BrainSpit",  BPF_PTHUNK},
  {A_SpawnSound,     "SpawnSound"},
  {A_SpawnFly,       "SpawnFly"},
  {A_BrainExplode,   "BrainExplode", BPF_PTHUNK},
  {A_Detonate,       "Detonate", BPF_PTHUNK}, // killough 8/9/98
  {A_Mushroom,       "Mushroom", BPF_PTHUNK}, // killough 10/98
  {A_Die,            "Die",      BPF_PTHUNK}, // killough 11/98
  {A_Spawn,          "Spawn",    BPF_PTHUNK}, // killough 11/98
  {A_Turn,           "Turn",     BPF_PTHUNK}, // killough 11/98
  {A_Face,           "Face",     BPF_PTHUNK}, // killough 11/98
  {A_Scratch,        "Scratch",  BPF_PTHUNK}, // killough 11/98
  {A_PlaySound,      "PlaySound",BPF_PTHUNK}, // killough 11/98
  {A_RandomJump,     "RandomJump"},           // killough 11/98
  {A_LineEffect,     "LineEffect"},           // killough 11/98
  {A_Nailbomb,       "Nailbomb", BPF_PTHUNK}, //sf
  // haleyjd: start new eternity codeptrs
  {A_StartScript,	"StartScript"},
  {A_PlayerStartScript, "PlayerStartScript"},
  {A_SetFlags,          "SetFlags",       BPF_PTHUNK},
  {A_UnSetFlags,        "UnSetFlags",     BPF_PTHUNK},
  {A_BetaSkullAttack,   "BetaSkullAttack",BPF_PTHUNK}, // haleyjd: MBF comp.
  {A_FireGrenade,	"FireGrenade"},
  {A_FireCustomBullets, "FireCustomBullets"},
  {A_FirePlayerMissile, "FirePlayerMissile"},
  {A_CustomPlayerMelee, "CustomPlayerMelee"},
  {A_GenTracer,         "GenTracer"},
  {A_BFG11KHit,         "BFG11KHit"},
  {A_BouncingBFG,       "BouncingBFG"},
  {A_BFGBurst,          "BFGBurst"},
  {A_FireOldBFG,        "FireOldBFG"},  // haleyjd: added for EDF
  {A_KeepChasing,       "KeepChasing"}, // haleyjd: lost pointer!
  {A_Stop,              "Stop",         BPF_PTHUNK}, // haleyjd: ditto
  {A_PlayerThunk,       "PlayerThunk"},
  {A_MissileAttack,     "MissileAttack",BPF_PTHUNK},
  {A_MissileSpread,     "MissileSpread",BPF_PTHUNK},
  {A_BulletAttack,      "BulletAttack", BPF_PTHUNK},
  {A_HealthJump,        "HealthJump"},
  {A_CounterJump,       "CounterJump"},
  {A_CounterSwitch,     "CounterSwitch"},
  {A_SetCounter,        "SetCounter",   BPF_PTHUNK},
  {A_CopyCounter,       "CopyCounter",  BPF_PTHUNK},
  {A_CounterOp,         "CounterOp",    BPF_PTHUNK},
  {A_SetTics,           "SetTics",      BPF_PTHUNK},
  {A_AproxDistance,     "AproxDistance",BPF_PTHUNK},
  {A_ShowMessage,       "ShowMessage",  BPF_PTHUNK},
  {A_RandomWalk,        "RandomWalk"},
  {A_TargetJump,        "TargetJump"},
  {A_ThingSummon,       "ThingSummon",  BPF_PTHUNK},
  {A_KillChildren,      "KillChildren", BPF_PTHUNK},
  // haleyjd: Heretic pointers
  {A_SpawnGlitter,      "SpawnGlitter", BPF_PTHUNK},
  {A_AccelGlitter,      "AccelGlitter", BPF_PTHUNK},
  {A_SpawnAbove,        "SpawnAbove",   BPF_PTHUNK},
  {A_MummyAttack,	"MummyAttack",  BPF_PTHUNK},
  {A_MummyAttack2,      "MummyAttack2", BPF_PTHUNK},
  {A_MummySoul,         "MummySoul",    BPF_PTHUNK},
  {A_HticDrop,          "HticDrop",     BPF_PTHUNK},
  {A_HticTracer,        "HticTracer"},
  {A_ClinkAttack,       "ClinkAttack",  BPF_PTHUNK},
  {A_WizardAtk1,        "WizardAtk1",   BPF_PTHUNK},
  {A_WizardAtk2,        "WizardAtk2",   BPF_PTHUNK},
  {A_WizardAtk3,        "WizardAtk3",   BPF_PTHUNK},
  {A_Srcr2Decide,       "Srcr2Decide"},
  {A_Srcr2Attack,       "Srcr2Attack",  BPF_PTHUNK},
  {A_BlueSpark,         "BlueSpark",    BPF_PTHUNK},
  {A_GenWizard,         "GenWizard"},
  {A_Sor2DthInit,       "Sor2DthInit",  BPF_PTHUNK},
  {A_Sor2DthLoop,       "Sor2DthLoop"},
  {A_HticExplode,       "HticExplode",  BPF_PTHUNK},
  {A_HticBossDeath,     "HticBossDeath"},
  {A_PodPain,           "PodPain",      BPF_PTHUNK},
  {A_RemovePod,         "RemovePod",    BPF_PTHUNK},
  {A_MakePod,           "MakePod",      BPF_PTHUNK},
  {A_KnightAttack,      "KnightAttack", BPF_PTHUNK},
  {A_DripBlood,         "DripBlood",    BPF_PTHUNK},
  {A_BeastAttack,       "BeastAttack",  BPF_PTHUNK},
  {A_BeastPuff,         "BeastPuff",    BPF_PTHUNK},
  {A_SnakeAttack,       "SnakeAttack"},
  {A_SnakeAttack2,      "SnakeAttack2"},
  {A_Sor1Chase,         "Sor1Chase"},
  {A_Sor1Pain,          "Sor1Pain",     BPF_PTHUNK},
  {A_Srcr1Attack,       "Srcr1Attack"},
  {A_SorcererRise,      "SorcererRise", BPF_PTHUNK},
  {A_VolcanoBlast,      "VolcanoBlast", BPF_PTHUNK},
  {A_VolcBallImpact,    "VolcBallImpact"},
  {A_MinotaurAtk1,	"MinotaurAtk1", BPF_PTHUNK},
  {A_MinotaurDecide,	"MinotaurDecide"},
  {A_MinotaurAtk2,	"MinotaurAtk2", BPF_PTHUNK},
  {A_MinotaurAtk3,	"MinotaurAtk3"},
  {A_MinotaurCharge,	"MinotaurCharge"},
  {A_MntrFloorFire,	"MntrFloorFire"},
  {A_LichFire,          "LichFire",     BPF_PTHUNK},
  {A_LichWhirlwind,     "LichWhirlwind",BPF_PTHUNK},
  {A_LichAttack,        "LichAttack",   BPF_PTHUNK},
  {A_WhirlwindSeek,     "WhirlwindSeek" },
  {A_LichIceImpact,     "LichIceImpact",BPF_PTHUNK},
  {A_LichFireGrow,      "LichFireGrow" },
  {A_ImpChargeAtk,      "ImpChargeAtk" },
  {A_ImpMeleeAtk,       "ImpMeleeAtk",  BPF_PTHUNK},
  {A_ImpMissileAtk,     "ImpMissileAtk",BPF_PTHUNK},
  {A_ImpDeath,          "ImpDeath"},
  {A_ImpXDeath1,        "ImpXDeath1"},
  {A_ImpXDeath2,        "ImpXDeath2"},
  {A_ImpExplode,        "ImpExplode"},
    // haleyjd 07/13/03: nuke specials
  {A_PainNukeSpec,      "PainNukeSpec"},
  {A_SorcNukeSpec,      "SorcNukeSpec"},
  // ETERNITY TC ptrs -- TODO: maybe eliminate these
  {A_ClericAtk,		"ClericAtk"},
  {A_FogSpawn,		"FogSpawn"},
  {A_FogMove,		"FogMove"},
  {A_Cleric2Chase,	"Cleric2Chase"},
  {A_Cleric2Decide,	"Cleric2Decide"},
  {A_Cleric2Attack,	"Cleric2Attack"},
  {A_ClericBreak,	"ClericBreak"},
  {A_DwarfDie,		"DwarfDie"},
  {A_CyberGuardSigh,	"CyberGuardSigh"},
  {A_DwarfLDOCMagic,	"DwarfLDOCMagic"},
  {A_DwarfFWAEMagic,	"DwarfFWAEMagic"},
  {A_DwarfAlterEgoChase,  "DwarfAlterEgoChase"},
  {A_DwarfAlterEgoAttack, "DwarfAlterEgoAttack"},
  {A_PhoenixTracer,	"PhoenixTracer"},
  // This NULL entry must be the last in the list
  {NULL,             "NULL"},  // Ty 05/16/98
};

// haleyjd 03/14/03: Just because its null-terminated doesn't mean 
// we can index it however the hell we want! See deh_procPointer in
// d_deh.c for a big bug fix.

int num_bexptrs = sizeof(deh_bexptrs) / sizeof(*deh_bexptrs);

//
// Shared Hash functions
//

//
// D_HashTableKey
//
// Fairly standard key computation -- this is used for multiple 
// tables so there's not much use trying to make it perfect. It'll 
// save time anyways.
// 08/28/03: vastly simplified, is now similar to SGI's STL hash
//
unsigned int D_HashTableKey(const char *str)
{
   const char *c = str;
   unsigned int h = 0;

   if(!str)
      I_Error("D_HashTableKey: cannot hash NULL string!\n");

   // note: this needs to be case insensitive for EDF mnemonics
   while(*c)
   {
      h = 5 * h + toupper(*c);
      ++c;
   }

   return h;
}

//
// BEX String Hash Table
//
// The deh_strs table has two independent hash chains through it,
// one for lookup by mnemonic (BEX style), and one for lookup by
// value (DEH style).
//

// number of strings = 416, 419 = closest greater prime
#define NUMSTRCHAINS 419
static int bexstrhashchains[NUMSTRCHAINS];
static int dehstrhashchains[NUMSTRCHAINS];

static void D_DEHStrHashInit(void)
{
   int i;

   memset(bexstrhashchains, -1, NUMSTRCHAINS*sizeof(int));
   memset(dehstrhashchains, -1, NUMSTRCHAINS*sizeof(int));

   for(i = 0; i < deh_numstrlookup; ++i)
   {
      unsigned int bkey, dkey;

      // key for bex mnemonic
      bkey = D_HashTableKey(deh_strlookup[i].lookup) % NUMSTRCHAINS;

      // key for actual string value
      dkey = D_HashTableKey(*(deh_strlookup[i].ppstr)) % NUMSTRCHAINS;

      deh_strlookup[i].bnext = bexstrhashchains[bkey];      
      bexstrhashchains[bkey] = i;

      deh_strlookup[i].dnext = dehstrhashchains[dkey];
      dehstrhashchains[dkey] = i;
   }
}

//
// D_GetBEXStr
//
// Finds the entry in the table above given a BEX mnemonic to
// look for
//
deh_strs *D_GetBEXStr(const char *string)
{
   unsigned int key;
   deh_strs *dehstr;

   // compute key for string
   key = D_HashTableKey(string) % NUMSTRCHAINS;

   // hash chain empty -- not found
   if(bexstrhashchains[key] == -1)
      return NULL;

   dehstr = &deh_strlookup[bexstrhashchains[key]];

   // while string doesn't match lookup, go down hash chain
   while(strcasecmp(string, dehstr->lookup))
   {
      // end of hash chain -- not found
      if(dehstr->bnext == -1)
         return NULL;
      else
         dehstr = &deh_strlookup[dehstr->bnext];
   }

   return dehstr;
}

//
// D_GetDEHStr
//
// Finds the entry in the table above given the actual string
// value to look for
//
deh_strs *D_GetDEHStr(const char *string)
{
   unsigned int key;
   deh_strs *dehstr;

   // compute key for string
   key = D_HashTableKey(string) % NUMSTRCHAINS;

   // hash chain empty -- not found
   if(dehstrhashchains[key] == -1)
      return NULL;

   dehstr = &deh_strlookup[dehstrhashchains[key]];

   // while string doesn't match lookup, go down hash chain
   while(stricmp(*(dehstr->ppstr), string))
   {
      // end of hash chain -- not found
      if(dehstr->dnext == -1)
         return NULL;
      else
         dehstr = &deh_strlookup[dehstr->dnext];
   }

   return dehstr;
}

//
// BEX Code Pointer Hash Table
//

#define NUMCPTRCHAINS 257

static int bexcpchains[NUMCPTRCHAINS];

static void D_BEXPtrHashInit(void)
{
   int i;

   memset(bexcpchains, -1, NUMCPTRCHAINS*sizeof(int));

   for(i = 0; i < num_bexptrs; i++)
   {
      unsigned int key;
      
      key = D_HashTableKey(deh_bexptrs[i].lookup) % NUMCPTRCHAINS;

      deh_bexptrs[i].next = bexcpchains[key];
      bexcpchains[key] = i;
   }
}

deh_bexptr *D_GetBexPtr(const char *mnemonic)
{
   unsigned int key;
   deh_bexptr *bexptr;

   // calculate key for mnemonic
   key = D_HashTableKey(mnemonic) % NUMCPTRCHAINS;

   // is chain empty?
   if(bexcpchains[key] == -1)
      return NULL; // doesn't exist

   bexptr = &deh_bexptrs[bexcpchains[key]];

   while(stricmp(mnemonic, bexptr->lookup))
   {
      // end of hash chain?
      if(bexptr->next == -1)
         return NULL; // doesn't exist
      else
         bexptr = &deh_bexptrs[bexptr->next];
   }

   return bexptr;
}

//
// D_BuildBEXHashChains
//
// haleyjd 03/14/03: with the addition of EDF, it has become necessary
// to separate internal table hash chain building from the dynamic
// sprites/music/sounds lookup table building below. This function
// must be called before E_ProcessEDF, whereas the one below must
// be called afterward.
//
void D_BuildBEXHashChains(void)
{
   // build string hash chains
   D_DEHStrHashInit();

   // bex codepointer table
   D_BEXPtrHashInit();
}

// haleyjd: support for BEX SPRITES, SOUNDS, and MUSIC
char **deh_spritenames;
char **deh_musicnames;

//
// D_BuildBEXTables
//
// strdup's string values in the sprites and music
// tables for use as unchanging mnemonics. Sounds now
// store their own mnemonics and thus a table for them
// is unnecessary.
//
void D_BuildBEXTables(void)
{
   char *spritestr;
   char *musicstr;
   int i;

   // build sprites, music lookups

   // haleyjd 03/11/03: must be dynamic now
   // 10/17/03: allocate all the names through a single pointer
   spritestr = Z_Malloc(5 * NUMSPRITES, PU_STATIC, 0);
   memset(spritestr, 0, 5 * NUMSPRITES);

   deh_spritenames = Z_Malloc((NUMSPRITES+1)*sizeof(char *),PU_STATIC,0);

   for(i = 0; i < NUMSPRITES; ++i)
   {
      deh_spritenames[i] = spritestr + i * 5;
      strncpy(deh_spritenames[i], sprnames[i], 4);
   }
   deh_spritenames[NUMSPRITES] = NULL;

   // 09/07/05: allocate all music names through one pointer
   musicstr = Z_Malloc(7 * NUMMUSIC, PU_STATIC, 0);
   memset(musicstr, 0, 7 * NUMMUSIC);

   deh_musicnames = Z_Malloc((NUMMUSIC+1)*sizeof(char *), PU_STATIC, 0);

   for(i = 1; i < NUMMUSIC; ++i)
   {
      deh_musicnames[i] = musicstr + i * 7;
      strncpy(deh_musicnames[i], S_music[i].name, 6);
   }
   deh_musicnames[0] = deh_musicnames[NUMMUSIC] = NULL;
}

//
// DeHackEd File Queue
//

// haleyjd: All DeHackEd files and lumps are now queued as they
// are encountered, and then processed in the appropriate order
// all at once. This allows EDF to be extended for loading from
// wad files.

typedef struct dehqueueitem_s
{
   mqueueitem_t mqitem; // this must be first

   char name[PATH_MAX+1];
   int  lumpnum;   
} dehqueueitem_t;

static mqueue_t dehqueue;

void D_DEHQueueInit(void)
{
   M_QueueInit(&dehqueue);
}

//
// D_QueueDEH
//
// Adds a dehqueue_t to the DeHackEd file/lump queue, so that all
// DeHackEd files/lumps can be parsed at once in the proper order.
//
void D_QueueDEH(const char *filename, int lumpnum)
{
   // allocate a new dehqueue_t
   dehqueueitem_t *newdq = malloc(sizeof(dehqueueitem_t));
   memset(newdq, 0, sizeof(dehqueueitem_t));

   // if filename is valid, this is a file DEH
   if(filename)
   {
      strncpy(newdq->name, filename, PATH_MAX+1);
      newdq->lumpnum = -1;
   }
   else
   {
      // otherwise, this is a wad dehacked lump
      newdq->lumpnum = lumpnum;
   }

   M_QueueInsert(&(newdq->mqitem), &dehqueue);
}

// DeHackEd support - Ty 03/09/97
// killough 10/98:
// Add lump number as third argument, for use when filename==NULL
void ProcessDehFile(char *filename, const char *outfilename, int lump);

// killough 10/98: support -dehout filename
// cph - made const, don't cache results
// haleyjd 09/11/03: moved here from d_main.c
static const char *D_dehout(void)
{
   int p = M_CheckParm("-dehout");

   if(!p)
      p = M_CheckParm("-bexout");
   
   return (p && ++p < myargc) ? myargv[p] : NULL;
}

//
// D_ProcessDEHQueue
//
// Processes all the DeHackEd/BEX files queued during startup, including
// command-line DEHs, GFS DEHs, preincluded DEHs, and in-wad DEHs.
//
void D_ProcessDEHQueue(void)
{
   // Start at the head node and process each DeHackEd -- the queue
   // has preserved the proper processing order.

   mqueueitem_t *rover;

   while((rover = M_QueueIterator(&dehqueue)))
   {
      dehqueueitem_t *dqitem = (dehqueueitem_t *)rover;

      // if lumpnum != -1, this is a wad dehacked lump, otherwise 
      // it's a file
      if(dqitem->lumpnum != -1)
      {
         ProcessDehFile(NULL, D_dehout(), dqitem->lumpnum);
      }
      else
      {
         ProcessDehFile(dqitem->name, D_dehout(), 0);
      }
   }

   // free the elements and reset the queue
   M_QueueFree(&dehqueue);
}

// EOF


