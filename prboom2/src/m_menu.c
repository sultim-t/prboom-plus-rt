/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_menu.c,v 1.5 2000/05/09 21:45:38 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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
 *  DOOM selection menu, options, episode etc. (aka Big Font menus)
 *  Sliders and icons. Kinda widget stuff.
 *  Setup Menus.
 *  Extended HELP screens.
 *  Dynamic HELP screen.
 *
 *-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: m_menu.c,v 1.5 2000/05/09 21:45:38 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _MSC_VER
#include <io.h>
#endif
#include <fcntl.h>

#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"
#include "d_main.h"
#include "i_system.h"
#include "v_video.h"
#include "w_wad.h"
#include "r_main.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_menu.h"
#include "d_deh.h"
#include "m_misc.h"
#include "lprintf.h"
#include "am_map.h"

extern patchnum_t hu_font[HU_FONTSIZE];
extern boolean  message_dontfuckwithme;
          
extern boolean chat_on;          // in heads-up code
extern int     hud_active;       // in heads-up code
extern int     hud_displayed;    // in heads-up code
extern int     hud_distributed;  // in heads-up code
extern int     HU_MoveHud(void); // jff 3/9/98 avoid glitch in HUD display

//
// defaulted values
//

int mouseSensitivity_horiz; // has default   //  killough
int mouseSensitivity_vert;  // has default

int showMessages;    // Show messages has default, 0 = off, 1 = on
  
int traditional_menu;

int hide_setup=1; // killough 5/15/98

// Blocky mode, has default, 0 = high, 1 = normal
//int     detailLevel;    obsolete -- killough
int screenblocks;    // has default

int screenSize;      // temp for screenblocks (0-9)    

int quickSaveSlot;   // -1 = no quicksave slot picked!          

int messageToPrint;  // 1 = message to be printed

// CPhipps - static const
static const char* messageString; // ...and here is the message string! 

// message x & y
int     messx;      
int     messy;
int     messageLastMenuActive;

boolean messageNeedsInput; // timed message = no input from user     

void (*messageRoutine)(int response);

#define SAVESTRINGSIZE  24

/* cphipps - M_DrawBackground renamed and moved to v_video.c */
#define M_DrawBackground V_DrawBackground

// we are going to be entering a savegame string

int saveStringEnter;              
int saveSlot;        // which slot to save in
int saveCharIndex;   // which char we're editing
// old save description before edit
char saveOldString[SAVESTRINGSIZE];  

boolean inhelpscreens; // indicates we are in or just left a help screen

boolean menuactive;    // The menus are up

#define SKULLXOFF  -32
#define LINEHEIGHT  16

char savegamestrings[10][SAVESTRINGSIZE];

char endstring[160];

// CPhipps - unused: extern boolean sendpause;
extern skill_t startskill; //jff 3/24/98 make startskill from D_MAIN accessible


//
// MENU TYPEDEFS
//

typedef struct
{
  short status; // 0 = no cursor here, 1 = ok, 2 = arrows ok
  char  name[10];
    
  // choice = menu item #.
  // if status = 2,
  //   choice=0:leftarrow,1:rightarrow
  void  (*routine)(int choice);
  char  alphaKey; // hotkey in menu     
} menuitem_t;

typedef struct menu_s
{                           
  short           numitems;     // # of menu items
  struct menu_s*  prevMenu;     // previous menu
  menuitem_t*     menuitems;    // menu items
  void            (*routine)(); // draw routine
  short           x;
  short           y;            // x,y of menu
  short           lastOn;       // last item user was on in menu
} menu_t;

short itemOn;           // menu item skull is on (for Big Font menus)
short skullAnimCounter; // skull animation counter
short whichSkull;       // which skull to draw (he blinks)

// graphic name of skulls
// warning: initializer-string for array of chars is too long

char skullName[2][/*8*/9] = {"M_SKULL1","M_SKULL2"};

menu_t* currentMenu; // current menudef                          

// jff 3/24/98
extern int defaultskill; // config file specified skill

// killough 3/6/98: preserve autorun across games
extern int autorun;      // always running?                         // phares

// phares 3/30/98
// externs added for setup menus

extern int destination_keys[MAXPLAYERS];
extern int mousebfire;                                   
extern int mousebstrafe;                               
extern int mousebforward;
extern int joybfire;
extern int joybstrafe;                               
extern int joybuse;                                   
extern int joybspeed;                                     
extern int default_weapon_recoil;   // weapon recoil        
extern int weapon_recoil;           // weapon recoil           
extern int default_player_bobbing;  // whether player bobs or not         
extern int player_bobbing;          // whether player bobs or not       
extern int weapon_preferences[2][NUMWEAPONS+1];                   
extern int health_red;    // health amount less than which status is red
extern int health_yellow; // health amount less than which status is yellow
extern int health_green;  // health amount above is blue, below is green
extern int armor_red;     // armor amount less than which status is red
extern int armor_yellow;  // armor amount less than which status is yellow
extern int armor_green;   // armor amount above is blue, below is green
extern int ammo_red;      // ammo percent less than which status is red
extern int ammo_yellow;   // ammo percent less is yellow more green
extern int sts_always_red;// status numbers do not change colors
extern int sts_pct_always_gray;// status percents do not change colors
extern int hud_nosecrets; // status does not list secrets/items/kills
extern int sts_traditional_keys;  // display keys the traditional way
extern int hud_list_bgon; // solid window background for list of messages
extern int hud_msg_lines; // number of message lines in window up to 16
extern int default_monsters_remember;                     
extern int monsters_remember;                            
extern int hudcolor_titl; // color range of automap level title
extern int hudcolor_xyco; // color range of new coords on automap
extern int hudcolor_mesg; // color range of scrolling messages
extern int hudcolor_chat; // color range of chat lines
extern int hudcolor_list; // color of list of past messages

extern char* chat_macros[];  // chat macros
extern const char* shiftxform;
extern int map_secret_after; //secrets do not appear til after bagged
extern default_t defaults[];
extern int numdefaults;

int mapcolor_me; // The player colour of the console player

// end of externs added for setup menus

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
void M_ChangeSensitivity(int choice);
void M_SfxVol(int choice);
void M_MusicVol(int choice);
/* void M_ChangeDetail(int choice);  unused -- killough */
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_Mouse(int choice, int *sens);      /* killough */
void M_MouseVert(int choice);
void M_MouseHoriz(int choice);
void M_ChangeSensitivity(int choice);
void M_DrawMouse(void);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawLoad(void);
void M_DrawSave(void);
void M_DrawSetup(void);                                     // phares 3/21/98
void M_DrawHelp (void);                                     // phares 5/04/98

void M_DrawSaveLoadBorder(int x,int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(int x,int y,int thermWidth,int thermDot);
void M_DrawEmptyCell(menu_t *menu,int item);
void M_DrawSelCell(menu_t *menu,int item);
void M_WriteText(int x, int y, const char *string);
int  M_StringWidth(const char *string);
int  M_StringHeight(const char *string);
void M_StartControlPanel(void);
void M_StartMessage(const char *string, void *routine, boolean input);
void M_StopMessage(void);
void M_ClearMenus (void);

// phares 3/30/98
// prototypes added to support Setup Menus and Extended HELP screens

int  M_GetKeyString(int,int);
void M_Setup(int choice);                               
void M_KeyBindings(int choice);                        
void M_Weapons(int);
void M_StatusBar(int);
void M_Automap(int);
void M_Enemy(int);
void M_Messages(int);
void M_ChatStrings(int);
void M_InitExtendedHelp(void);
void M_ExtHelpNextScreen(int);
void M_ExtHelp(int);
int  M_GetPixelWidth(char*);
void M_DrawKeybnd(void);
void M_DrawWeapons(void);
void M_DrawMenuString(int,int,int);                    
void M_DrawStatusHUD(void);
void M_DrawExtHelp(void);
void M_DrawAutoMap(void);
void M_DrawEnemy(void);
void M_DrawMessages(void);
void M_DrawChatStrings(void);

menu_t NewDef;                                              // phares 5/04/98

// end of prototypes added to support Setup Menus and Extended HELP screens

/////////////////////////////////////////////////////////////////////////////
//
// DOOM MENUS
//

/////////////////////////////
//
// MAIN MENU
//

// main_e provides numerical values for which Big Font screen you're on

enum
{
  newgame = 0,
  loadgame,
  savegame,
  options,
  readthis,
  quitdoom,
  main_end
} main_e;

//
// MainMenu is the definition of what the main menu Screen should look
// like. Each entry shows that the cursor can land on each item (1), the
// built-in graphic lump (i.e. "M_NGAME") that should be displayed,
// the program which takes over when an item is selected, and the hotkey
// associated with the item.
//

menuitem_t MainMenu[]=
{
  {1,"M_NGAME", M_NewGame, 'n'},
  {1,"M_LOADG", M_LoadGame,'l'},
  {1,"M_SAVEG", M_SaveGame,'s'},
  {1,"M_OPTION",M_Options, 'o'},
  // Another hickup with Special edition.
  {1,"M_RDTHIS",M_ReadThis,'r'},
  {1,"M_QUITG", M_QuitDOOM,'q'}
};

menu_t MainDef =
{
  main_end,       // number of menu items
  NULL,           // previous menu screen
  MainMenu,       // table that defines menu items
  M_DrawMainMenu, // drawing routine
  97,64,          // initial cursor position
  0               // last menu item the user was on
};

//
// M_DrawMainMenu
//

void M_DrawMainMenu(void)
{
  // CPhipps - patch drawing updated
  V_DrawNamePatch(94, 2, 0, "M_DOOM", CR_DEFAULT, VPT_STRETCH);
}

/////////////////////////////
//
// Read This! MENU 1 & 2
//

// There are no menu items on the Read This! screens, so read_e just
// provides a placeholder to maintain structure.

enum
{
  rdthsempty1,
  read1_end
} read_e;

enum
{
  rdthsempty2,
  read2_end
} read_e2;


// The definitions of the Read This! screens

menuitem_t ReadMenu1[] =
{
  {1,"",M_ReadThis2,0}
};

menuitem_t ReadMenu2[]=
{
  {1,"",M_FinishReadThis,0}
};

menu_t ReadDef1 =
{
  read1_end,
  &MainDef,
  ReadMenu1,
  M_DrawReadThis1,
  330,175,
//280,185,              // killough 2/21/98: fix help screens
  0
};

menu_t ReadDef2 =
{
  read2_end,
  &ReadDef1,
  ReadMenu2,
  M_DrawReadThis2,
  330,175,
  0
};


//
// M_ReadThis
//

void M_ReadThis(int choice)
  {
  choice = 0;
  M_SetupNextMenu(&ReadDef1);
  }

void M_ReadThis2(int choice)
  {
  choice = 0;
  M_SetupNextMenu(&ReadDef2);
  }

void M_FinishReadThis(int choice)
  {
  choice = 0;
  M_SetupNextMenu(&MainDef);
  }

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//

void M_DrawReadThis1(void)
  {
  inhelpscreens = true;
  switch(gamemode)
      {
    case commercial:
      M_DrawHelp();
      break;
    case shareware:
    case registered:
      // CPhipps - patch drawing updated
      V_DrawNamePatch(0, 0, 0, "HELP2", CR_DEFAULT, VPT_STRETCH);
      break;
    case retail:   // killough 2/21/98: Fix Ultimate Doom help screen:
      // CPhipps - patch drawing updated
      V_DrawNamePatch(0, 0, 0, "CREDIT", CR_DEFAULT, VPT_STRETCH);
      break;
    default:
      break;
      }
  return;
  }

//
// Read This Menus - optional second page.
//

void M_DrawReadThis2(void)
  {
  inhelpscreens = true;
  switch(gamemode)
      {
    case commercial:

      // This hack keeps us from having to change menus.

      // CPhipps - patch drawing updated
      V_DrawNamePatch(0, 0, 0, "CREDIT", CR_DEFAULT, VPT_STRETCH);
      break;
    case shareware:
    case registered:
    case retail:
      M_DrawHelp();
      break;
    default:
      break;
      }
  return;
  }

/////////////////////////////
//
// EPISODE SELECT
//

//
// episodes_e provides numbers for the episode menu items. The default is
// 4, to accomodate Ultimate Doom. If the user is running anything else,
// this is accounted for in the code.
//

enum
{
  ep1,
  ep2,
  ep3,
  ep4,
  ep_end
} episodes_e;

// The definitions of the Episodes menu

menuitem_t EpisodeMenu[]=
{
  {1,"M_EPI1", M_Episode,'k'},
  {1,"M_EPI2", M_Episode,'t'},
  {1,"M_EPI3", M_Episode,'i'},
  {1,"M_EPI4", M_Episode,'t'}
};

menu_t EpiDef =
{
  ep_end,        // # of menu items
  &MainDef,      // previous menu
  EpisodeMenu,   // menuitem_t ->
  M_DrawEpisode, // drawing routine ->
  48,63,         // x,y
  ep1            // lastOn
};

//
//    M_Episode
//
int epi;

void M_DrawEpisode(void)
{
  // CPhipps - patch drawing updated
  V_DrawNamePatch(54, 38, 0, "M_EPISOD", CR_DEFAULT, VPT_STRETCH);
}

void M_Episode(int choice)
  {
  if ( (gamemode == shareware) && choice)
    {
    M_StartMessage(s_SWSTRING,NULL,false); // Ty 03/27/98 - externalized
    M_SetupNextMenu(&ReadDef1);
    return;
    }

  // Yet another hack...
  if ( (gamemode == registered) && (choice > 2))
    {
    lprintf( LO_WARN, 
     "M_Episode: 4th episode requires UltimateDOOM\n");
    choice = 0;
    }
   
  epi = choice;
  M_SetupNextMenu(&NewDef);
  }

/////////////////////////////
//
// NEW GAME
//

// numerical values for the New Game menu items

// cph - fix enum decl (?!)
enum newgame_e
{
  killthings,
  toorough,
  hurtme,
  violence,
  nightmare,
  newg_end
};

// The definitions of the New Game menu

menuitem_t NewGameMenu[]=
{
  {1,"M_JKILL", M_ChooseSkill, 'i'},
  {1,"M_ROUGH", M_ChooseSkill, 'h'},
  {1,"M_HURT",  M_ChooseSkill, 'h'},
  {1,"M_ULTRA", M_ChooseSkill, 'u'},
  {1,"M_NMARE", M_ChooseSkill, 'n'}
};

menu_t NewDef =
{
  newg_end,       // # of menu items
  &EpiDef,        // previous menu
  NewGameMenu,    // menuitem_t ->
  M_DrawNewGame,  // drawing routine ->
  48,63,          // x,y
  hurtme          // lastOn
};

//
// M_NewGame
//

void M_DrawNewGame(void)
{
  // CPhipps - patch drawing updated
  V_DrawNamePatch(96, 14, 0, "M_NEWG", CR_DEFAULT, VPT_STRETCH);
  V_DrawNamePatch(54, 38, 0, "M_SKILL",CR_DEFAULT, VPT_STRETCH);
}

// CPhipps - make `New Game' restart the level in a netgame
static void M_RestartLevelResponse(int ch)
{
  if (ch != 'y')
    return;

  if (demorecording)
    exit(0);

  currentMenu->lastOn = itemOn;
  M_ClearMenus ();
  G_RestartLevel ();
}

void M_NewGame(int choice)
  {
  if (netgame && !demoplayback) {
    if (compatibility_level < lxdoom_1_compatibility)
      M_StartMessage(s_NEWGAME,NULL,false); // Ty 03/27/98 - externalized
    else // CPhipps - query restarting the level
      M_StartMessage(s_RESTARTLEVEL,M_RestartLevelResponse,true);
    return;
  }
  
  if (demorecording)   // killough 5/26/98: exclude during demo recordings
    {
    M_StartMessage("you can't start a new game\n"
                   "while recording a demo!\n\n"PRESSKEY,
                   NULL, false); // killough 5/26/98: not externalized
    return;
    }
  
  if ( gamemode == commercial )
    M_SetupNextMenu(&NewDef);
  else
    M_SetupNextMenu(&EpiDef);
  }

// CPhipps - static
static void M_VerifyNightmare(int ch)
{
  if (ch != 'y')
  return;
  
  G_DeferedInitNew(nightmare,epi+1,1);
  M_ClearMenus ();
}

void M_ChooseSkill(int choice)
  {
  if (choice == nightmare)
    {
    M_StartMessage(s_NIGHTMARE,M_VerifyNightmare,true); // Ty 03/27/98 - externalized
    return;
    }
  
  G_DeferedInitNew(choice,epi+1,1);
  M_ClearMenus ();
  }

/////////////////////////////
//
// LOAD GAME MENU
//

// numerical values for the Load Game slots

enum
{
  load1,
  load2,
  load3,
  load4,
  load5,
  load6,
  load7, //jff 3/15/98 extend number of slots
  load8,
  load_end
} load_e;

// The definitions of the Load Game screen

menuitem_t LoadMenu[]=
  {
  {1,"", M_LoadSelect,'1'},
  {1,"", M_LoadSelect,'2'},
  {1,"", M_LoadSelect,'3'},
  {1,"", M_LoadSelect,'4'},
  {1,"", M_LoadSelect,'5'},
  {1,"", M_LoadSelect,'6'},
  {1,"", M_LoadSelect,'7'}, //jff 3/15/98 extend number of slots
  {1,"", M_LoadSelect,'8'},
  };

menu_t LoadDef =
  {
  load_end,
  &MainDef,
  LoadMenu,
  M_DrawLoad,
  80,34, //jff 3/15/98 move menu up
  0
  };

#define LOADGRAPHIC_Y 8

//
// M_LoadGame & Cie.
//

void M_DrawLoad(void)
  {
  int i;

  //jff 3/15/98 use symbolic load position
  // CPhipps - patch drawing updated
  V_DrawNamePatch(72 ,LOADGRAPHIC_Y, 0, "M_LOADG", CR_DEFAULT, VPT_STRETCH);
  for (i = 0 ; i < load_end ; i++)
    {
    M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
    M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }
  }

//
// Draw border for the savegame description
//
// CPhipps - patch drawing updated

void M_DrawSaveLoadBorder(int x,int y)
  {
  int i;
  
  V_DrawNamePatch(x-8, y+7, 0, "M_LSLEFT", CR_DEFAULT, VPT_STRETCH);
  
  for (i = 0 ; i < 24 ; i++)
    {
      V_DrawNamePatch(x, y+7, 0, "M_LSCNTR", CR_DEFAULT, VPT_STRETCH);
      x += 8;
    }

  V_DrawNamePatch(x, y+7, 0, "M_LSRGHT", CR_DEFAULT, VPT_STRETCH);
  }

//
// User wants to load this game
//

void M_LoadSelect(int choice)
{
  // CPhipps - Modified so savegame filename is worked out only internal 
  //  to g_game.c, this only passes the slot.
  
  G_LoadGame(choice, false); // killough 3/16/98, 5/15/98: add slot, cmd

  M_ClearMenus ();
}

//
// killough 5/15/98: add forced loadgames
//
// CPhipps - must strdup() and free() this pointer without casts if possible
static char* forced_load_str; 

static void M_VerifyForcedLoadGame(int ch)
{
  if (ch=='y')
    G_ForcedLoadGame();

  if (forced_load_str) {
    free(forced_load_str);       // free the message strdup()'ed below
    forced_load_str = NULL;
  }

  M_ClearMenus();
}

void M_ForcedLoadGame(const char *msg)
{
  M_StartMessage(forced_load_str = strdup(msg), M_VerifyForcedLoadGame, true);
}

//
// Selected from DOOM menu
//

void M_LoadGame (int choice)
  {
#if 0
    if (netgame && !demoplayback)     // killough 5/26/98: add !demoplayback
    {
    M_StartMessage(s_LOADNET,NULL,false); // Ty 03/27/98 - externalized
    return;
    }
#endif

    if (demorecording)   // killough 5/26/98: exclude during demo recordings
    {
    M_StartMessage("you can't load a game\n"
                   "while recording a demo!\n\n"PRESSKEY,
                   NULL, false); // killough 5/26/98: not externalized
    return;
    }
  
  M_SetupNextMenu(&LoadDef);
  M_ReadSaveStrings();
  }

/////////////////////////////
//
// SAVE GAME MENU
//

// The definitions of the Save Game screen

menuitem_t SaveMenu[]=
  {
  {1,"", M_SaveSelect,'1'},
  {1,"", M_SaveSelect,'2'},
  {1,"", M_SaveSelect,'3'},
  {1,"", M_SaveSelect,'4'},
  {1,"", M_SaveSelect,'5'},
  {1,"", M_SaveSelect,'6'},
  {1,"", M_SaveSelect,'7'}, //jff 3/15/98 extend number of slots
  {1,"", M_SaveSelect,'8'},
  };

menu_t SaveDef =
  {
  load_end, // same number of slots as the Load Game screen
  &MainDef,
  SaveMenu,
  M_DrawSave,
  80,34, //jff 3/15/98 move menu up
  0
  };

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(void)
  {
  int i;

  for (i = 0 ; i < load_end ; i++)
    {
    int handle;
    char name[PATH_MAX+1];    // killough 3/22/98

    G_SaveGameName(name,sizeof(name),i);    // killough 3/22/98
    handle = open (name, O_RDONLY | 0, 0666);
    if (handle == -1)
      {
      strcpy(&savegamestrings[i][0],s_EMPTYSTRING); // Ty 03/27/98 - externalized
      LoadMenu[i].status = 0;
      continue;
      }
    read (handle, &savegamestrings[i], SAVESTRINGSIZE);
    close (handle);
    LoadMenu[i].status = 1;
    }
  }

//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
  {
  int i;

  //jff 3/15/98 use symbolic load position
  // CPhipps - patch drawing updated
  V_DrawNamePatch(72, LOADGRAPHIC_Y, 0, "M_SAVEG", CR_DEFAULT, VPT_STRETCH);
  for (i = 0 ; i < load_end ; i++)
    {
    M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
    M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }
  
  if (saveStringEnter)
    {
    i = M_StringWidth(savegamestrings[saveSlot]);
    M_WriteText(LoadDef.x + i,LoadDef.y+LINEHEIGHT*saveSlot,"_");
    }
  }

//
// M_Responder calls this when user is finished
//
static void M_DoSave(int slot) // CPhipps - static
{
  G_SaveGame (slot,savegamestrings[slot]);
  M_ClearMenus ();

  // PICK QUICKSAVE SLOT YET?
  if (quickSaveSlot == -2)
    quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
  {
  // we are going to be intercepting all chars
  saveStringEnter = 1;
  
  saveSlot = choice;
  strcpy(saveOldString,savegamestrings[choice]);
  if (!strcmp(savegamestrings[choice],s_EMPTYSTRING)) // Ty 03/27/98 - externalized
    savegamestrings[choice][0] = 0;
  saveCharIndex = strlen(savegamestrings[choice]);
  }

//
// Selected from DOOM menu
//
void M_SaveGame (int choice)
  {
  if (!usergame)
    {
    M_StartMessage(s_SAVEDEAD,NULL,false); // Ty 03/27/98 - externalized
    return;
    }
  
  if (gamestate != GS_LEVEL)
    return;
  
  M_SetupNextMenu(&SaveDef);
  M_ReadSaveStrings();
  }

/////////////////////////////
//
// OPTIONS MENU
//

// numerical values for the Options menu items

enum
{
  // killough 4/6/98: move setup to be a sub-menu of OPTIONs
  setup,                                                    // phares 3/21/98
  endgame,
  messages,
  /*    detail, obsolete -- killough */
  scrnsize,
  option_empty1,
  mousesens,
  /* option_empty2, submenu now -- killough */
  soundvol,
  opt_end
} options_e;

// The definitions of the Options menu

menuitem_t OptionsMenu[]=
{
  // killough 4/6/98: move setup to be a sub-menu of OPTIONs
  {1,"M_SETUP",  M_Setup,   's'},                          // phares 3/21/98
  {1,"M_ENDGAM", M_EndGame,'e'},
  {1,"M_MESSG",  M_ChangeMessages,'m'},
  /*    {1,"M_DETAIL",  M_ChangeDetail,'g'},  unused -- killough */  
  {2,"M_SCRNSZ", M_SizeDisplay,'s'},
  {-1,"",0},
  {1,"M_MSENS",  M_ChangeSensitivity,'m'},
  /* {-1,"",0},  replaced with submenu -- killough */ 
  {1,"M_SVOL",   M_Sound,'s'}
};

menu_t OptionsDef =
{
  opt_end,
  &MainDef,
  OptionsMenu,
  M_DrawOptions,
  60,37,
  0
};

//
// M_Options
//
char detailNames[2][9] = {"M_GDHIGH","M_GDLOW"};
char msgNames[2][9]  = {"M_MSGOFF","M_MSGON"};


void M_DrawOptions(void)
{
  // CPhipps - patch drawing updated
  // proff/nicolas 09/20/98 -- changed for hi-res
  V_DrawNamePatch(108, 15, 0, "M_OPTTTL", CR_DEFAULT, VPT_STRETCH);

  V_DrawNamePatch(OptionsDef.x + 120, OptionsDef.y+LINEHEIGHT*messages, 0,
		  msgNames[showMessages], CR_DEFAULT, VPT_STRETCH);

  M_DrawThermo(OptionsDef.x,OptionsDef.y+LINEHEIGHT*(scrnsize+1),
   9,screenSize);
}

void M_Options(int choice)
  {
  M_SetupNextMenu(&OptionsDef);
  }

/////////////////////////////
//
// M_QuitDOOM
//
int quitsounds[8] =
  {
  sfx_pldeth,
  sfx_dmpain,
  sfx_popain,
  sfx_slop,
  sfx_telept,
  sfx_posit1,
  sfx_posit3,
  sfx_sgtatk
  };

int quitsounds2[8] =
  {
  sfx_vilact,
  sfx_getpow,
  sfx_boscub,
  sfx_slop,
  sfx_skeswg,
  sfx_kntdth,
  sfx_bspact,
  sfx_sgtatk
  };

static void M_QuitResponse(int ch)
{
  if (ch != 'y')
    return;
  if (!netgame)
    {
    if (gamemode == commercial)
      S_StartSound(NULL,quitsounds2[(gametic>>2)&7]);
    else
      S_StartSound(NULL,quitsounds[(gametic>>2)&7]);

    I_uSleep(3000000); // cph - 3 s
    }
  exit(0); // killough
}

void M_QuitDOOM(int choice)
  {
  // We pick index 0 which is language sensitive,
  // or one at random, between 1 and maximum number.
  // Ty 03/27/98 - externalized DOSY as a string s_DOSY that's in the sprintf
  if (language != english)
    sprintf(endstring,"%s\n\n%s",s_DOSY, endmsg[0] );
  else         // killough 1/18/98: fix endgame message calculation:
    // killough 3/28/98: Fix incorrect order of s_DOSY:
    sprintf(endstring,"%s\n\n%s", endmsg[gametic%(NUM_QUITMESSAGES-1) + 1], s_DOSY);
  
  M_StartMessage(endstring,M_QuitResponse,true);
  }

/////////////////////////////
//
// SOUND VOLUME MENU
//

// numerical values for the Sound Volume menu items
// The 'empty' slots are where the sliding scales appear.

enum
{
  sfx_vol,
  sfx_empty1,
  music_vol,
  sfx_empty2,
  sound_end
} sound_e;

// The definitions of the Sound Volume menu

menuitem_t SoundMenu[]=
{
  {2,"M_SFXVOL",M_SfxVol,'s'},
  {-1,"",0},
  {2,"M_MUSVOL",M_MusicVol,'m'},
  {-1,"",0}
};

menu_t SoundDef =
{
  sound_end,
  &OptionsDef,
  SoundMenu,
  M_DrawSound,
  80,64,
  0
};

//
// Change Sfx & Music volumes
//

void M_DrawSound(void)
  {
  // CPhipps - patch drawing updated
  V_DrawNamePatch(60, 38, 0, "M_SVOL", CR_DEFAULT, VPT_STRETCH);

  M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_vol+1),16,snd_SfxVolume);

  M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(music_vol+1),16,snd_MusicVolume);
  }

void M_Sound(int choice)
  {
  M_SetupNextMenu(&SoundDef);
  }

void M_SfxVol(int choice)
  {
  switch(choice)
      {
    case 0:
      if (snd_SfxVolume)
        snd_SfxVolume--;
      break;
    case 1:
      if (snd_SfxVolume < 15)
        snd_SfxVolume++;
      break;
      }
  
  S_SetSfxVolume(snd_SfxVolume /* *8 */);
  }

void M_MusicVol(int choice)
  {
  switch(choice)
      {
    case 0:
      if (snd_MusicVolume)
        snd_MusicVolume--;
      break;
    case 1:
      if (snd_MusicVolume < 15)
        snd_MusicVolume++;
      break;
      }
  
  S_SetMusicVolume(snd_MusicVolume /* *8 */);
  }

/////////////////////////////
//
// MOUSE SENSITIVITY MENU -- killough
//

// numerical values for the Mouse Sensitivity menu items
// The 'empty' slots are where the sliding scales appear.

enum
{
  mouse_horiz,
  mouse_empty1,
  mouse_vert,
  mouse_empty2,
  mouse_end
} mouse_e;

// The definitions of the Mouse Sensitivity menu

menuitem_t MouseMenu[]=
{
  {2,"M_HORSEN",M_MouseHoriz,'h'},
  {-1,"",0},
  {2,"M_VERSEN",M_MouseVert,'v'},
  {-1,"",0}
};

menu_t MouseDef =
{
  mouse_end,
  &OptionsDef,
  MouseMenu,
  M_DrawMouse,
  60,64,
  0
};


// I'm using a scale of 100 since I don't know what's normal -- killough.

#define MOUSE_SENS_MAX 100

//
// Change Mouse Sensitivities -- killough
//

void M_DrawMouse(void)
  {
  int mhmx,mvmx; /* jff 4/3/98 clamp drawn position    99max mead */

  // CPhipps - patch drawing updated
  V_DrawNamePatch(60, 38, 0, "M_MSENS", CR_DEFAULT, VPT_STRETCH);

  //jff 4/3/98 clamp horizontal sensitivity display
  mhmx = mouseSensitivity_horiz>99? 99 : mouseSensitivity_horiz; /*mead*/
  M_DrawThermo(MouseDef.x,MouseDef.y+LINEHEIGHT*(mouse_horiz+1),100,mhmx);
  //jff 4/3/98 clamp vertical sensitivity display
  mvmx = mouseSensitivity_vert>99? 99 : mouseSensitivity_vert; /*mead*/
  M_DrawThermo(MouseDef.x,MouseDef.y+LINEHEIGHT*(mouse_vert+1),100,mvmx);
  }

void M_ChangeSensitivity(int choice)
  {
  M_SetupNextMenu(&MouseDef);      // killough

  //  switch(choice)
  //      {
  //    case 0:
  //      if (mouseSensitivity)
  //        mouseSensitivity--;
  //      break;
  //    case 1:
  //      if (mouseSensitivity < 9)
  //        mouseSensitivity++;
  //      break;
  //      }
  }

void M_MouseHoriz(int choice)
  {
  M_Mouse(choice, &mouseSensitivity_horiz);
  }

void M_MouseVert(int choice)
  {
  M_Mouse(choice, &mouseSensitivity_vert);
  }

void M_Mouse(int choice, int *sens)
  {
  switch(choice)
      {
    case 0:
      if (*sens)
        --*sens;
      break;
    case 1:
      if (*sens < 99) 
        ++*sens;              /*mead*/
      break;
      }
  }

/////////////////////////////
//
//    M_QuickSave
//

char tempstring[80];

static void M_QuickSaveResponse(int ch)
{
  if (ch == 'y')  {
    M_DoSave(quickSaveSlot);
    S_StartSound(NULL,sfx_swtchx);
  }
}

void M_QuickSave(void)
{
  if (!usergame) {
    S_StartSound(NULL,sfx_oof);
    return;
  }
  
  if (gamestate != GS_LEVEL)
    return;
  
  if (quickSaveSlot < 0) {
    M_StartControlPanel();
    M_ReadSaveStrings();
    M_SetupNextMenu(&SaveDef);
    quickSaveSlot = -2; // means to pick a slot now
    return;
  }
  sprintf(tempstring,s_QSPROMPT,savegamestrings[quickSaveSlot]); // Ty 03/27/98 - externalized
  M_StartMessage(tempstring,M_QuickSaveResponse,true);
}

/////////////////////////////
//
// M_QuickLoad
//

static void M_QuickLoadResponse(int ch)
{
  if (ch == 'y') {
    M_LoadSelect(quickSaveSlot);
    S_StartSound(NULL,sfx_swtchx);
  }
}

void M_QuickLoad(void)
{
  // cph - removed restriction against quickload in a netgame

  if (demorecording) {  // killough 5/26/98: exclude during demo recordings 
    M_StartMessage("you can't quickload\n"
                   "while recording a demo!\n\n"PRESSKEY,
                   NULL, false); // killough 5/26/98: not externalized
    return;
  }
  
  if (quickSaveSlot < 0) {
    M_StartMessage(s_QSAVESPOT,NULL,false); // Ty 03/27/98 - externalized
    return;
  }
  sprintf(tempstring,s_QLPROMPT,savegamestrings[quickSaveSlot]); // Ty 03/27/98 - externalized
  M_StartMessage(tempstring,M_QuickLoadResponse,true);
}

/////////////////////////////
//
// M_EndGame
//

static void M_EndGameResponse(int ch)
{
  if (ch != 'y')
    return;

  if (demorecording) // killough 5/26/98: make endgame quit if recording demo
    exit(0);

  currentMenu->lastOn = itemOn;
  M_ClearMenus ();
  D_StartTitle ();
}

void M_EndGame(int choice)
  {
  //  choice = 0;
  if (!usergame)
    {
    S_StartSound(NULL,sfx_oof);
    return;
    }
  
  if (netgame)
    {
    M_StartMessage(s_NETEND,NULL,false); // Ty 03/27/98 - externalized
    return;
    }

  M_StartMessage(s_ENDGAME,M_EndGameResponse,true); // Ty 03/27/98 - externalized
  }

/////////////////////////////
//
//    Toggle messages on/off
//

void M_ChangeMessages(int choice)
  {
  // warning: unused parameter `int choice'
  choice = 0;
  showMessages = 1 - showMessages;
  
  if (!showMessages)
    players[consoleplayer].message = s_MSGOFF; // Ty 03/27/98 - externalized
  else
    players[consoleplayer].message = s_MSGON ; // Ty 03/27/98 - externalized

  message_dontfuckwithme = true;
  }

/////////////////////////////
//
// CHANGE DISPLAY SIZE
//
// jff 2/23/98 restored to pre-HUD state
// hud_active controlled soley by F5=key_detail (key_hud)
// hud_displayed is toggled by + or = in fullscreen
// hud_displayed is cleared by -

void M_SizeDisplay(int choice)
  {
  switch(choice)
    {
  case 0:
    if (screenSize > 0)
      {
      screenblocks--;
      screenSize--;
      hud_displayed = 0;
      }
    break;
  case 1:
    if (screenSize < 8)
      {
      screenblocks++;
      screenSize++;
      }
    else
      hud_displayed = !hud_displayed;
    break;
    }
  R_SetViewSize (screenblocks /*, detailLevel obsolete -- killough */);
  }

//
// End of Original Menus
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// SETUP MENU (phares)
//
// We've added a set of Setup Screens from which you can configure a number
// of variables w/o having to restart the game. There are 7 screens:
//
//    Key Bindings
//    Weapons
//    Status Bar / HUD
//    Automap
//    Enemies
//    Messages
//    Chat Strings
//

/////////////////////////////
//
// booleans for setup screens
// these tell you what state the setup screens are in, and whether any of
// the overlay screens (automap colors, reset button message) should be
// displayed

boolean setup_active      = false; // in one of the setup screens
boolean set_keybnd_active = false; // in key binding setup screens
boolean set_weapon_active = false; // in weapons setup screen
boolean set_status_active = false; // in status bar/hud setup screen
boolean set_auto_active   = false; // in automap setup screen
boolean set_enemy_active  = false; // in enemies setup screen
boolean set_mess_active   = false; // in messages setup screen
boolean set_chat_active   = false; // in chat string setup screen
boolean setup_select      = false; // changing an item
boolean setup_gather      = false; // gathering keys for value
boolean colorbox_active   = false; // color palette being shown
boolean default_verify    = false; // verify reset defaults decision

/////////////////////////////
//
// The following #defines are for the m_flags field of each item on every
// Setup Screen. They can be OR'ed together where appropriate

#define S_HILITE     0x1 // Cursor is sitting on this item
#define S_SELECT     0x2 // We're changing this item
#define S_TITLE      0x4 // Title item
#define S_YESNO      0x8 // Yes or No item
#define S_CRITEM    0x10 // Message color
#define S_COLOR     0x20 // Automap color
#define S_CHAT      0x40 // Chat String
#define S_RESET     0x80 // Reset to Defaults Button
#define S_PREV     0x100 // Previous menu exists
#define S_NEXT     0x200 // Next menu exists
#define S_KEY      0x400 // Key Binding
#define S_WEAP     0x800 // Weapon #
#define S_NUM     0x1000 // Numerical item
#define S_SKIP    0x2000 // Cursor can't land here
#define S_KEEP    0x4000 // Don't swap key out
#define S_END     0x8000 // Last item in list (dummy)

// S_SHOWDESC = the set of items whose description should be displayed
// S_SHOWSET  = the set of items whose setting should be displayed

#define S_SHOWDESC (S_TITLE|S_YESNO|S_CRITEM|S_COLOR|S_CHAT|S_RESET|S_PREV|S_NEXT|S_KEY|S_WEAP|S_NUM)
#define S_SHOWSET  (S_YESNO|S_CRITEM|S_COLOR|S_CHAT|S_KEY|S_WEAP|S_NUM)

/////////////////////////////
//
// The setup_group enum is used to show which 'groups' keys fall into so
// that you can bind a key differently in each 'group'.

typedef enum {
  m_null,       // Has no meaning; not applicable
  m_scrn,       // A key can not be assigned to more than one action
  m_map,        // in the same group. A key can be assigned to one
  m_menu,       // action in one group, and another action in another.
  } setup_group;

/////////////////////////////
//
// phares 4/17/98:
// State definition for each item.
// This is the definition of the structure for each setup item. Not all
// fields are used by all items.
//
// A setup screen is defined by an array of these items specific to
// that screen.
//
// Future: don't include the m_low and m_high fields here. Go back to
// the defaults config table in m_misc.c to get those values.

// CPhipps - make stuct members const where possible
typedef struct
{
  const char* m_text;   // text to display
  int         m_flags;  // phares 4/17/98: flag bits S_* (defined above)
  setup_group m_group;  // Group
  short       m_x;      // screen x position (left is 0)
  short       m_y;      // screen y position (top is 0)
  int*        m_key;    // key value, or 0 if not shown
  int*        m_mouse;  // mouse button value, or 0 if not shown
  int*        m_joy;    // joystick button value, or 0 if not shown
  int*        m_var1;   // value or default setting
  int*        m_var2;   // possible non-default setting
  int         m_low;    // lowest value for numerical settings
  int         m_high;   // highest value for numerical settings
} setup_menu_t;

/////////////////////////////
//
// set_menu_itemon is an index that starts at zero, and tells you which
// item on the current screen the cursor is sitting on.
//
// current_setup_menu is a pointer to the current setup menu table.

static int set_menu_itemon; // which setup item is selected?
setup_menu_t* current_setup_menu; // points to current setup menu table

/////////////////////////////
//
// The menu_buffer is used to construct strings for display on the screen.

static char menu_buffer[64];

/////////////////////////////
//
// The setup_e enum is used to provide a unique number for each group of Setup
// Screens.

enum
  {
  set_key_bindings,                                     
  set_weapons,                                           
  set_statbar,                                           
  set_automap,
  set_enemy,
  set_messages,
  set_chatstrings,
  set_setup_end
  } setup_e;

int setup_screen; // the current setup screen. takes values from setup_e 

/////////////////////////////
//
// SetupMenu is the definition of what the main Setup Screen should look
// like. Each entry shows that the cursor can land on each item (1), the
// built-in graphic lump (i.e. "M_KEYBND") that should be displayed,
// the program which takes over when an item is selected, and the hotkey
// associated with the item.

menuitem_t SetupMenu[]=
  {
  {1,"M_KEYBND",M_KeyBindings,'k'},
  {1,"M_WEAP"  ,M_Weapons,    'w'},
  {1,"M_STAT"  ,M_StatusBar,  's'},               
  {1,"M_AUTO"  ,M_Automap,    'a'},                    
  {1,"M_ENEM"  ,M_Enemy,      'e'},                     
  {1,"M_MESS"  ,M_Messages,   'm'},                     
  {1,"M_CHAT"  ,M_ChatStrings,'c'},                     
  };

/////////////////////////////
//
// M_DoNothing does just that: nothing. Just a placeholder.
// CPhipps - so make it static
static void M_DoNothing(int choice)
{
}

/////////////////////////////
//
// Items needed to satisfy the 'Big Font' menu structures:
//
// the generic_setup_e enum mimics the 'Big Font' menu structures, but
// means nothing to the Setup Menus.

enum
  {
  generic_setupempty1,
  generic_setup_end
  } generic_setup_e;

// Generic_Setup is a do-nothing definition that the mainstream Menu code
// can understand, while the Setup Menu code is working. Another placeholder.

menuitem_t Generic_Setup[] =
  {
  {1,"",M_DoNothing,0}
  };

/////////////////////////////
//
// SetupDef is the menu definition that the mainstream Menu code understands.
// This is used by M_Setup (below) to define what is drawn and what is done
// with the main Setup screen.

menu_t  SetupDef =
  {
  set_setup_end, // number of Setup Menu items (Key Bindings, etc.)
  &MainDef,      // menu to return to when BACKSPACE is hit on this menu
  SetupMenu,     // definition of items to show on the Setup Screen
  M_DrawSetup,   // program that draws the Setup Screen
  59,37,         // x,y position of the skull (modified when the skull is
                 // drawn). The skull is parked on the upper-left corner
                 // of the Setup screens, since it isn't needed as a cursor
  0              // last item the user was on for this menu
  };

/////////////////////////////
//
// Here are the definitions of the individual Setup Menu screens. They
// follow the format of the 'Big Font' menu structures. See the comments
// for SetupDef (above) to help understand what each of these says.

menu_t KeybndDef =
  {
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawKeybnd,
  34,5,      // skull drawn here
  0
  };

menu_t WeaponDef =
  {
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawWeapons,
  34,5,      // skull drawn here
  0
  };

menu_t StatusHUDDef =
  {
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawStatusHUD,
  34,5,      // skull drawn here
  0
  };

menu_t AutoMapDef =
  {
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawAutoMap,
  34,5,      // skull drawn here
  0
  };

menu_t EnemyDef =                                           // phares 4/08/98
  {
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawEnemy,
  34,5,      // skull drawn here
  0
  };

menu_t MessageDef =                                         // phares 4/08/98
  {
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawMessages,
  34,5,      // skull drawn here
  0
  };

menu_t ChatStrDef =                                         // phares 4/10/98
  {
  generic_setup_end,
  &SetupDef,
  Generic_Setup,
  M_DrawChatStrings,
  34,5,      // skull drawn here
  0
  };

/////////////////////////////
//
// Draws the Title for the main Setup screen

void M_DrawSetup(void)
  {
  // CPhipps - patch drawing updated
  V_DrawNamePatch(124, 15, 0, "M_SETUP", CR_DEFAULT, VPT_STRETCH);
  }

/////////////////////////////
//
// Uses the SetupDef structure to draw the menu items for the main
// Setup screen

void M_Setup(int choice)
  {
      M_SetupNextMenu(&SetupDef);
  }

/////////////////////////////
//
// Data that's used by the Setup screen code
//
// Establish the message colors to be used

#define CR_TITLE  CR_GOLD
#define CR_SET    CR_GREEN
#define CR_ITEM   CR_RED
#define CR_HILITE CR_ORANGE
#define CR_SELECT CR_GRAY

// Data used by the Automap color selection code

#define CHIP_SIZE 7 // size of color block for colored items

#define COLORPALXORIG ((320 - 16*(CHIP_SIZE+1))/2)
#define COLORPALYORIG ((200 - 16*(CHIP_SIZE+1))/2)

#define PAL_BLACK   0
#define PAL_WHITE   4

static byte colorblock[(CHIP_SIZE+4)*(CHIP_SIZE+4)];

// Data used by the Chat String editing code

#define CHAT_STRING_BFR_SIZE 128
#define MAXCHATWIDTH         287 // chat strings must fit in this screen space

int   chat_index;
char* chat_string_buffer; // points to new chat strings while editing

/////////////////////////////
//
// phares 4/17/98:
// Added 'Reset to Defaults' Button to Setup Menu screens
// This is a small button that sits in the upper-right-hand corner of
// the first screen for each group. It blinks when selected, thus the
// two patches, which it toggles back and forth.

char ResetButtonName[2][8] = {"M_BUTT1","M_BUTT2"};

/////////////////////////////
//
// phares 4/18/98:
// Consolidate Item drawing code
//
// M_DrawItem draws the description of the provided item (the left-hand
// part). A different color is used for the text depending on whether the
// item is selected or not, or whether it's about to change.

// CPhipps - static, hanging else removed, const parameter
static void M_DrawItem(const setup_menu_t* s)
{
  int color,flags;
  int x,y;

  x = s->m_x;
  y = s->m_y;
  flags = s->m_flags;
  if (flags & S_RESET) {
    // This item is the reset button
    // Draw the 'off' version if this isn't the current menu item
    // Draw the blinking version in tune with the blinking skull otherwise

    // proff/nicolas 09/20/98 -- changed for hi-res
    // CPhipps - Patch drawing updated, reformatted

    V_DrawNamePatch(x, y, 0, ResetButtonName[(flags & (S_HILITE|S_SELECT)) ? whichSkull : 0], 
		    CR_DEFAULT, VPT_STRETCH);

  // Draw the item string

  } else {
    if (flags & S_SELECT)
      color = CR_SELECT;
    else if (flags & S_HILITE)
      color = CR_HILITE;
    else if (flags & (S_TITLE|S_NEXT|S_PREV))
      color = CR_TITLE;
    else
      color = CR_ITEM;
    strcpy(menu_buffer,s->m_text);
    M_DrawMenuString(x-M_GetPixelWidth(menu_buffer)-4,y,color);
  }
}

/////////////////////////////
//
// phares 4/18/98:
// Consolidate Item Setting drawing code
//
// M_DrawSetting draws the setting of the provided item (the right-hand
// part. It determines the text color based on whether the item is
// selected or being changed. Then, depending on the type of item, it
// displays the appropriate setting value: yes/no, a key binding, a number,
// a paint chip, etc.

// CPhipps - static, formatting, const parameter
static void M_DrawSetting(const setup_menu_t* s)
{
  int*  key;
  int   weapon,flags,i,cursor_start,char_width,len,x,y,color;
  byte  ch,*ptr;
  char* text;
  char  c[2];

  x = s->m_x;
  y = s->m_y;
  flags = s->m_flags;

  // Determine color of the text. This may or may not be used later,
  // depending on whether the item is a text string or not.

  if (flags & S_SELECT)
    color = CR_SELECT;
  else if (flags & S_HILITE)
    color = CR_HILITE;
  else
    color = CR_SET;

  // Is the item a YES/NO item?

  if (flags & S_YESNO) {
    strcpy(menu_buffer,*(s->m_var1) ? "YES" : "NO");
    M_DrawMenuString(x,y,color);
  }

  // Is the item a simple number?

  else if (flags & S_NUM) {
    sprintf(menu_buffer,"%d",*(s->m_var1));
    M_DrawMenuString(x,y,color);
  }

  // Is the item a key binding?

  else if (flags & S_KEY) { // Key Binding
    // Draw the key bound to the action
    
    key = s->m_key;
    if (key) {
      M_GetKeyString(*key,0); // string to display
      if (key == &key_use) {
      
        // For the 'use' key, you have to build the string
      
        if (s->m_mouse)
          sprintf(&menu_buffer[strlen(menu_buffer)],"/DBL-CLK MB%d",*(s->m_mouse)+1);
        if (s->m_joy)
          sprintf(&menu_buffer[strlen(menu_buffer)],"/JSB%d",*(s->m_joy)+1);
      }
      else if ((key == &key_up)   || (key == &key_speed) ||
               (key == &key_fire) || (key == &key_strafe))
        {
	  if (s->m_mouse)
	    sprintf(&menu_buffer[strlen(menu_buffer)],"/MB%d",*(s->m_mouse)+1);
	  if (s->m_joy)
	    sprintf(&menu_buffer[strlen(menu_buffer)],"/JSB%d",*(s->m_joy)+1);
        }
      M_DrawMenuString(x,y,color);
    }
  }
  
  // Is the item a weapon number?
  // If so, special processing is needed so as not to display weapons that
  // didn't exist in earlier versions of the game.

  else if (flags & S_WEAP) { // weapon number
  
    weapon = *(s->m_var1) - 1;
    if (((weapon == wp_plasma) || (weapon == wp_bfg)) &&
	(gamemode == shareware))
      return;
    if ((weapon == wp_supershotgun) && (gamemode != commercial))
      return;
    sprintf(menu_buffer,"%d",weapon + 1);
    M_DrawMenuString(x,y,color);
  }
  
  // Is the item a paint chip?

  else if (flags & S_COLOR) { // Automap paint chip
      
    // draw the border of the paint chip
       
    ptr = colorblock;
    for (i = 0 ; i < (CHIP_SIZE+2)*(CHIP_SIZE+2) ; i++)
      *ptr++ = PAL_BLACK;
    // CPhipps - used stretched block drawing, block drawing updated
    V_DrawBlock(x, y-1, 0, CHIP_SIZE+2, CHIP_SIZE+2, colorblock, VPT_STRETCH);
      
    // draw the paint chip
       
    ch = (byte) *(s->m_var1);
    if (ch == 0) // don't show this item in automap mode
      // CPhipps - patch drawing updated
      V_DrawNamePatch(x+1, y, 0, "M_PALNO", CR_DEFAULT, VPT_STRETCH);
    else {
      ptr = colorblock;
      for (i = 0 ; i < CHIP_SIZE*CHIP_SIZE ; i++)
        *ptr++ = ch;

      // CPhipps - block drawing updated, used stretched block drawing
      V_DrawBlock(x+1,y,0,CHIP_SIZE,CHIP_SIZE,colorblock, VPT_STRETCH);
    }
  }

  // Is the item a colored text string from the Automap?

  else if (flags & S_CRITEM) {
    sprintf(menu_buffer,"%d",*(s->m_var1));
    M_DrawMenuString(x,y,*(s->m_var1));
  }

  // Is the item a chat string?

  else if (flags & S_CHAT) {
    text = *((char**)(s->m_var1));

    // Are we editing this string? If so, display a cursor under
    // the correct character.

    if (setup_select && (s->m_flags & (S_HILITE|S_SELECT))) {

      // If the string is too wide for the screen, trim it back,
      // one char at a time until it fits. This should only occur
      // while you're editing the string.

      while (M_GetPixelWidth(text) >= MAXCHATWIDTH) {
        len = strlen(text); 
        *(text+len-1) = 0;
        len--;
        if (chat_index > len)
          chat_index--;
      }

      // Find the distance from the beginning of the string to
      // where the cursor should be drawn, plus the width of
      // the char the cursor is under..

      c[0] = *(text + chat_index); // hold temporarily
      c[1] = 0;
      char_width = M_GetPixelWidth(c);
      if (char_width == 1)
        char_width = 7; // default for end of line
      *(text + chat_index) = 0; // NULL to get cursor position
      cursor_start = M_GetPixelWidth(text);
      *(text + chat_index) = c[0]; // replace stored char

      // Now draw the cursor

      for (i = 0 ; i < char_width ; i++)
        colorblock[i] = PAL_BLACK;

      // CPhipps - used stretched block drawing, block drawing updated
      V_DrawBlock(x+cursor_start-1, y+7, 0, char_width, 1, colorblock, VPT_STRETCH);
    }

    // Draw the setting for the item

    strcpy(menu_buffer,text);
    M_DrawMenuString(x,y,color);
  }
}

/////////////////////////////
//
// M_DrawScreenItems takes the data for each menu item and gives it to
// the drawing routines above.

// CPhipps - static, const parameter, formatting
static void M_DrawScreenItems(const setup_menu_t* src)
{
  while (!(src->m_flags & S_END)) {

    // See if we're to draw the item description (left-hand part)
    
    if (src->m_flags & S_SHOWDESC)
      M_DrawItem(src);

    // See if we're to draw the setting (right-hand part)

    if (src->m_flags & S_SHOWSET)
      M_DrawSetting(src);
    src++;
  }
}

/////////////////////////////
//
// Data used to draw the "are you sure?" dialogue box when resetting
// to defaults.

#define VERIFYBOXXORG 66
#define VERIFYBOXYORG 88
#define PAL_GRAY1  91
#define PAL_GRAY2  98
#define PAL_GRAY3 105

// And the routine to draw it.
// CPhipps - use stretched block drawing
//         - use memset instead of loops (!)
//         - static void
//         - rewrite to draw whole block before blitting, so stretching works
//
static void M_DrawDefVerify(void)
{
#define BOXW 187
#define BOXH 23
  byte* bigblock = malloc(BOXW*BOXH);
  int i;
  
  // CPhipps - draw one big block instead of line-by-line, otherwise it doesn't 
  //  stretch properly
  memset(bigblock, PAL_BLACK, BOXW*BOXH);
#define colset(orig,val,len) for (i=0; i<len; i++) (orig)[i*BOXW] = val

  memset(bigblock, PAL_GRAY1, BOXW);
  memset(bigblock+2+(BOXH-3)*BOXW, PAL_GRAY1, BOXW-4);
  colset(bigblock, PAL_GRAY1, BOXH);
  colset(bigblock+(BOXW-3)+2*BOXW, PAL_GRAY1, BOXH-4);

  memset(bigblock+1+BOXW, PAL_GRAY2, BOXW-2);
  memset(bigblock+1+BOXW*(BOXH-2), PAL_GRAY2, BOXW-2);
  colset(bigblock+1+BOXW, PAL_GRAY2, BOXH-2);
  colset(bigblock+(BOXW-2)+BOXW, PAL_GRAY2, BOXH-2);

  memset(bigblock+2+2*BOXW, PAL_GRAY3, BOXW-4);
  memset(bigblock+BOXW*(BOXH-1), PAL_GRAY3, BOXW);
  colset(bigblock+2+2*BOXW, PAL_GRAY3, BOXH-4);
  colset(bigblock+BOXW-1, PAL_GRAY3, BOXH);
#undef colset

  // CPhipps - block drawing updated
  V_DrawBlock(VERIFYBOXXORG+3,VERIFYBOXYORG+3,0,BOXW,BOXH,bigblock, VPT_STRETCH);
  free(bigblock);

  // The blinking messages is keyed off of the blinking of the
  // cursor skull.

  if (whichSkull) { // blink the text
    strcpy(menu_buffer,"Reset to defaults? (Y or N)");
    M_DrawMenuString(VERIFYBOXXORG+8,VERIFYBOXYORG+8,CR_RED);
  }
}


/////////////////////////////
//
// phares 4/18/98:
// M_DrawInstructions writes the instruction text just below the screen title

// CPhipps - static, reformatted, hanging else removed
static void M_DrawInstructions()
{
  int flags = (current_setup_menu + set_menu_itemon)->m_flags;
  int x = 0;
  int color;

  // There are different instruction messages depending on whether you
  // are changing an item or just sitting on it.

  if (setup_select) {
    color = CR_SELECT; 
    if (flags & S_KEY) {

      // See if a joystick or mouse button setting is allowed for
      // this item.

      if ((current_setup_menu + set_menu_itemon)->m_mouse ||
        (current_setup_menu + set_menu_itemon)->m_joy)
        {
        strcpy(menu_buffer,"Press key or button for this action");
        x = 49;
        }
      else
        {
        strcpy(menu_buffer,"Press key for this action");
        x = 84;
        }
    } else if (flags & S_YESNO) {
      strcpy(menu_buffer,"Press ENTER key to toggle");
      x = 78;
    } else if (flags & S_WEAP) {
      strcpy(menu_buffer,"Enter weapon number");
      x = 97;
    } else if (flags & S_NUM) {
      strcpy(menu_buffer,"Enter value. Press ENTER when finished.");
      x = 37;
    } else if (flags & S_COLOR) {
      strcpy(menu_buffer,"Select color and press enter");
      x = 70;
    } else if (flags & S_CRITEM) {
      strcpy(menu_buffer,"Enter value");
      x = 125;
    } else if (flags & S_CHAT) {
      strcpy(menu_buffer,"Type/edit chat string and Press ENTER");
      x = 43;
    } else if (flags & S_RESET) {
      strcpy(menu_buffer,"");
      x = 43;
    }
  } else { // when you're changing something
    color = CR_HILITE;
    if (flags & S_RESET) {
      strcpy(menu_buffer,"Press ENTER key to reset to defaults");
      x = 43;
    } else {
      strcpy(menu_buffer,"Press Enter to Change");
      x = 91;
    }
  }
  M_DrawMenuString(x,20,color);
}

/////////////////////////////
//
// The Key Binding Screen tables.

#define KB_X  160
#define KB_PREV  57
#define KB_NEXT 310
#define KB_Y   31

// phares 4/16/98:
// X,Y position of reset button. This is the same for every screen, and is
// only defined once here.

#define X_BUTTON 301
#define Y_BUTTON   3

// Definitions of the (in this case) four key binding screens.

setup_menu_t keys_settings1[];       
setup_menu_t keys_settings2[];       
setup_menu_t keys_settings3[];       
setup_menu_t keys_settings4[];       
setup_menu_t keys_settings5[];       

// The table which gets you from one screen table to the next.

setup_menu_t* keys_settings[] =
{
  keys_settings1,
  keys_settings2,
  keys_settings3,
  keys_settings4,
  keys_settings5,
  NULL
};

int mult_screens_index; // the index of the current screen in a set

// Here's an example from this first screen, with explanations.
//
//  {
//  "STRAFE",      // The description of the item ('strafe' key)
//  S_KEY,         // This is a key binding item
//  m_scrn,        // It belongs to the m_scrn group. Its key cannot be
//                 // bound to two items in this group.
//  KB_X,          // The X offset of the start of the right-hand side
//  KB_Y+ 8*8,     // The Y offset of the start of the right-hand side.
//                 // Always given in multiples off a baseline.
//  &key_strafe,   // The variable that holds the key value bound to this
//  &mousebstrafe, // The variable that holds the mouse button bound to
                   // this. If zero, no mouse button can be bound here.
//  &joybstrafe,   // The variable that holds the joystick button bound to
                   // this. If zero, no mouse button can be bound here.
//  0,             // Where a pointer to a numerical setting would be.
//  0,             // where a pointer to another numerical setting would be
//  0,             // upper limit if this was a number that had one
//  0              // lower limit if this was a number that had one
//  }

// The first Key Binding screen table.
// Note that the Y values are ascending. If you need to add something to
// this table, (well, this one's not a good example, because it's full)
// you need to make sure the Y values still make sense so everything gets
// displayed.
//
// Note also that the first screen of each set has a line for the reset
// button. If there is more than one screen in a set, the others don't get
// the reset button.
//
// Note also that this screen has a "NEXT ->" line. This acts like an
// item, in that 'activating' it moves you along to the next screen. If
// there's a "<- PREV" item on a screen, it behaves similarly, moving you
// to the previous screen. If you leave these off, you can't move from
// screen to screen.

setup_menu_t keys_settings1[] =  // Key Binding screen strings       
{
  {"MOVEMENT"    ,(S_SKIP|S_TITLE),m_null,KB_X,KB_Y ,0                  ,0,0,0,0,0,0},
  {"FORWARD"     ,S_KEY       ,m_scrn,KB_X,KB_Y+ 1*8,&key_up            ,&mousebforward,0,0,0,0,0},
  {"BACKWARD"    ,S_KEY       ,m_scrn,KB_X,KB_Y+ 2*8,&key_down          ,0,0,0,0,0,0},
  {"TURN LEFT"   ,S_KEY       ,m_scrn,KB_X,KB_Y+ 3*8,&key_left          ,0,0,0,0,0,0},
  {"TURN RIGHT"  ,S_KEY       ,m_scrn,KB_X,KB_Y+ 4*8,&key_right         ,0,0,0,0,0,0},
  {"RUN"         ,S_KEY       ,m_scrn,KB_X,KB_Y+ 5*8,&key_speed         ,0,&joybspeed,0,0,0,0},
  {"STRAFE LEFT" ,S_KEY       ,m_scrn,KB_X,KB_Y+ 6*8,&key_strafeleft    ,0,0,0,0,0,0},
  {"STRAFE RIGHT",S_KEY       ,m_scrn,KB_X,KB_Y+ 7*8,&key_straferight   ,0,0,0,0,0,0},
  {"STRAFE"      ,S_KEY       ,m_scrn,KB_X,KB_Y+ 8*8,&key_strafe        ,&mousebstrafe,&joybstrafe,0,0,0,0},
  {"AUTORUN"     ,S_KEY       ,m_scrn,KB_X,KB_Y+ 9*8,&key_autorun       ,0,0,0,0,0,0},
  {"180 TURN"    ,S_KEY       ,m_scrn,KB_X,KB_Y+10*8,&key_reverse       ,0,0,0,0,0,0},
  {"USE"         ,S_KEY       ,m_scrn,KB_X,KB_Y+11*8,&key_use           ,&mousebforward,&joybuse,0,0,0,0},

  {"MENUS"       ,(S_SKIP|S_TITLE),m_null,KB_X,KB_Y+12*8,0              ,0,0,0,0,0,0},
  {"NEXT ITEM"   ,S_KEY       ,m_menu,KB_X,KB_Y+13*8,&key_menu_down     ,0,0,0,0,0,0},
  {"PREV ITEM"   ,S_KEY       ,m_menu,KB_X,KB_Y+14*8,&key_menu_up       ,0,0,0,0,0,0},
  {"LEFT"        ,S_KEY       ,m_menu,KB_X,KB_Y+15*8,&key_menu_left     ,0,0,0,0,0,0},
  {"RIGHT"       ,S_KEY       ,m_menu,KB_X,KB_Y+16*8,&key_menu_right    ,0,0,0,0,0,0},
  {"BACKSPACE"   ,S_KEY       ,m_menu,KB_X,KB_Y+17*8,&key_menu_backspace,0,0,0,0,0,0},
  {"SELECT ITEM" ,S_KEY       ,m_menu,KB_X,KB_Y+18*8,&key_menu_enter    ,0,0,0,0,0,0},
  {"EXIT"        ,S_KEY       ,m_menu,KB_X,KB_Y+19*8,&key_menu_escape   ,0,0,0,0,0,0},

  // Button for resetting to defaults

  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON,0,0,0,0,0,0,0},

  {"NEXT ->",(S_SKIP|S_NEXT),m_null,KB_NEXT,KB_Y+20*8,0,0,0,(int *)keys_settings2,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};

setup_menu_t keys_settings2[] =  // Key Binding screen strings       
{
  {"SCREEN"      ,(S_SKIP|S_TITLE),m_null,KB_X,KB_Y   ,0         ,0,0,0,0,0,0},

// phares 4/13/98:
// key_help and key_escape can no longer be rebound. This keeps the
// player from getting themselves in a bind where they can't remember how
// to get to the menus, and can't remember how to get to the help screen
// to give them a clue as to how to get to the menus. :)

// Also, the keys assigned to these functions cannot be bound to other
// functions. Introduce an S_KEEP flag to show that you cannot swap this
// key with other keys in the same 'group'. (m_scrn, etc.)

  {"HELP"        ,(S_SKIP|S_KEEP) ,m_scrn,0   ,0    ,&key_help         ,0,0,0,0,0,0},
  {"MENU"        ,(S_SKIP|S_KEEP) ,m_scrn,0   ,0    ,&key_escape       ,0,0,0,0,0,0},

  {"PAUSE"       ,S_KEY       ,m_scrn,KB_X,KB_Y+ 1*8,&key_pause        ,0,0,0,0,0,0},
  {"AUTOMAP"     ,S_KEY       ,m_scrn,KB_X,KB_Y+ 2*8,&key_map          ,0,0,0,0,0,0},
  {"VOLUME"      ,S_KEY       ,m_scrn,KB_X,KB_Y+ 3*8,&key_soundvolume  ,0,0,0,0,0,0},
  {"HUD"         ,S_KEY       ,m_scrn,KB_X,KB_Y+ 4*8,&key_hud          ,0,0,0,0,0,0},
  {"MESSAGES"    ,S_KEY       ,m_scrn,KB_X,KB_Y+ 5*8,&key_messages     ,0,0,0,0,0,0},
  {"GAMMA FIX"   ,S_KEY       ,m_scrn,KB_X,KB_Y+ 6*8,&key_gamma        ,0,0,0,0,0,0},
  {"SPY"         ,S_KEY       ,m_scrn,KB_X,KB_Y+ 7*8,&key_spy          ,0,0,0,0,0,0},
  {"LARGER VIEW" ,S_KEY       ,m_scrn,KB_X,KB_Y+ 8*8,&key_zoomin       ,0,0,0,0,0,0}, 
  {"SMALLER VIEW",S_KEY       ,m_scrn,KB_X,KB_Y+ 9*8,&key_zoomout      ,0,0,0,0,0,0},
  {"SCREENSHOT"  ,S_KEY       ,m_scrn,KB_X,KB_Y+10*8,&key_screenshot   ,0,0,0,0,0,0},

  {"GAME"        ,(S_SKIP|S_TITLE),m_null,KB_X,KB_Y+12*8,0             ,0,0,0,0,0,0},
  {"SAVE"        ,S_KEY       ,m_scrn,KB_X,KB_Y+13*8,&key_savegame     ,0,0,0,0,0,0},
  {"LOAD"        ,S_KEY       ,m_scrn,KB_X,KB_Y+14*8,&key_loadgame     ,0,0,0,0,0,0},
  {"QUICKSAVE"   ,S_KEY       ,m_scrn,KB_X,KB_Y+15*8,&key_quicksave    ,0,0,0,0,0,0},
  {"QUICKLOAD"   ,S_KEY       ,m_scrn,KB_X,KB_Y+16*8,&key_quickload    ,0,0,0,0,0,0},
  {"END GAME"    ,S_KEY       ,m_scrn,KB_X,KB_Y+17*8,&key_endgame      ,0,0,0,0,0,0},
  {"QUIT"        ,S_KEY       ,m_scrn,KB_X,KB_Y+18*8,&key_quit         ,0,0,0,0,0,0},

  {"<- PREV"     ,(S_SKIP|S_PREV),m_null,KB_PREV,KB_Y+20*8,0,0,0,(int *)keys_settings1,0,0,0},
  {"NEXT ->"     ,(S_SKIP|S_NEXT),m_null,KB_NEXT,KB_Y+20*8,0,0,0,(int *)keys_settings3,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};

setup_menu_t keys_settings3[] =  // Key Binding screen strings       
{
  {"WEAPONS" ,(S_SKIP|S_TITLE),m_null,KB_X,KB_Y   ,0              ,0,0,0,0,0,0},
  {"FIST"    ,S_KEY       ,m_scrn,KB_X,KB_Y+ 1*8,&key_weapon1     ,0,0,0,0,0,0},
  {"PISTOL"  ,S_KEY       ,m_scrn,KB_X,KB_Y+ 2*8,&key_weapon2     ,0,0,0,0,0,0},
  {"SHOTGUN" ,S_KEY       ,m_scrn,KB_X,KB_Y+ 3*8,&key_weapon3     ,0,0,0,0,0,0},
  {"CHAINGUN",S_KEY       ,m_scrn,KB_X,KB_Y+ 4*8,&key_weapon4     ,0,0,0,0,0,0},
  {"ROCKET"  ,S_KEY       ,m_scrn,KB_X,KB_Y+ 5*8,&key_weapon5     ,0,0,0,0,0,0},
  {"PLASMA"  ,S_KEY       ,m_scrn,KB_X,KB_Y+ 6*8,&key_weapon6     ,0,0,0,0,0,0},
  {"BFG 9000",S_KEY       ,m_scrn,KB_X,KB_Y+ 7*8,&key_weapon7     ,0,0,0,0,0,0},
  {"CHAINSAW",S_KEY       ,m_scrn,KB_X,KB_Y+ 8*8,&key_weapon8     ,0,0,0,0,0,0},
  {"SSG"     ,S_KEY       ,m_scrn,KB_X,KB_Y+ 9*8,&key_weapon9     ,0,0,0,0,0,0},
  {"BEST"    ,S_KEY       ,m_scrn,KB_X,KB_Y+10*8,&key_weapontoggle,0,0,0,0,0,0},
  {"FIRE"    ,S_KEY       ,m_scrn,KB_X,KB_Y+11*8,&key_fire        ,&mousebfire,&joybfire,0,0,0,0},

  {"<- PREV",(S_SKIP|S_PREV),m_null,KB_PREV,KB_Y+20*8,0,0,0,(int *)keys_settings2,0,0,0},
  {"NEXT ->",(S_SKIP|S_NEXT),m_null,KB_NEXT,KB_Y+20*8,0,0,0,(int *)keys_settings4,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};

setup_menu_t keys_settings4[] =  // Key Binding screen strings       
{
  {"AUTOMAP"    ,(S_SKIP|S_TITLE),m_null,KB_X,KB_Y   ,0                 ,0,0,0,0,0,0},
  {"FOLLOW"     ,S_KEY       ,m_map ,KB_X,KB_Y+ 1*8,&key_map_follow     ,0,0,0,0,0,0},
  {"ZOOM IN"    ,S_KEY       ,m_map ,KB_X,KB_Y+ 2*8,&key_map_zoomin     ,0,0,0,0,0,0},
  {"ZOOM OUT"   ,S_KEY       ,m_map ,KB_X,KB_Y+ 3*8,&key_map_zoomout    ,0,0,0,0,0,0},
  {"SHIFT UP"   ,S_KEY       ,m_map ,KB_X,KB_Y+ 4*8,&key_map_up         ,0,0,0,0,0,0},
  {"SHIFT DOWN" ,S_KEY       ,m_map ,KB_X,KB_Y+ 5*8,&key_map_down       ,0,0,0,0,0,0},
  {"SHIFT LEFT" ,S_KEY       ,m_map ,KB_X,KB_Y+ 6*8,&key_map_left       ,0,0,0,0,0,0},
  {"SHIFT RIGHT",S_KEY       ,m_map ,KB_X,KB_Y+ 7*8,&key_map_right      ,0,0,0,0,0,0},
  {"MARK PLACE" ,S_KEY       ,m_map ,KB_X,KB_Y+ 8*8,&key_map_mark       ,0,0,0,0,0,0},
  {"CLEAR MARKS",S_KEY       ,m_map ,KB_X,KB_Y+ 9*8,&key_map_clear      ,0,0,0,0,0,0},
  {"FULL/ZOOM"  ,S_KEY       ,m_map ,KB_X,KB_Y+10*8,&key_map_gobig      ,0,0,0,0,0,0},
  {"GRID"       ,S_KEY       ,m_map ,KB_X,KB_Y+11*8,&key_map_grid       ,0,0,0,0,0,0},
  {"ROTATE"     ,S_KEY       ,m_map ,KB_X,KB_Y+12*8,&key_map_rotate     ,0,0,0,0,0,0},
  {"OVERLAY"    ,S_KEY       ,m_map ,KB_X,KB_Y+13*8,&key_map_overlay    ,0,0,0,0,0,0},

  {"<- PREV",(S_SKIP|S_PREV),m_null,KB_PREV,KB_Y+20*8,0,0,0,(int *)keys_settings3,0,0,0},
  {"NEXT ->",(S_SKIP|S_NEXT),m_null,KB_NEXT,KB_Y+20*8,0,0,0,(int *)keys_settings5,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}
};

setup_menu_t keys_settings5[] = // Chat keys setup screen
{

  {"CHATTING"   ,(S_SKIP|S_TITLE),m_null,KB_X,KB_Y,0                   ,0,0,0,0,0,0},
  {"BEGIN CHAT" ,S_KEY       ,m_scrn,KB_X,KB_Y+1*8,&key_chat           ,0,0,0,0,0,0},
  {"PLAYER 1"   ,S_KEY       ,m_scrn,KB_X,KB_Y+2*8,&destination_keys[0],0,0,0,0,0,0},
  {"PLAYER 2"   ,S_KEY       ,m_scrn,KB_X,KB_Y+3*8,&destination_keys[1],0,0,0,0,0,0},
  {"PLAYER 3"   ,S_KEY       ,m_scrn,KB_X,KB_Y+4*8,&destination_keys[2],0,0,0,0,0,0},
  {"PLAYER 4"   ,S_KEY       ,m_scrn,KB_X,KB_Y+5*8,&destination_keys[3],0,0,0,0,0,0},
  {"BACKSPACE"  ,S_KEY       ,m_scrn,KB_X,KB_Y+6*8,&key_backspace      ,0,0,0,0,0,0},
  {"ENTER"      ,S_KEY       ,m_scrn,KB_X,KB_Y+7*8,&key_enter          ,0,0,0,0,0,0},

  {"<- PREV" ,(S_SKIP|S_PREV),m_null,KB_PREV,KB_Y+20*8,0,0,0,(int *)keys_settings4,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};

// Setting up for the Key Binding screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_KeyBindings(int choice)
  {
  M_SetupNextMenu(&KeybndDef);

  setup_active = true;
  setup_screen = ss_keys;
  set_keybnd_active = true;
  setup_select = false;
  default_verify = false;
  mult_screens_index = 0;
  current_setup_menu = keys_settings[0];
  set_menu_itemon = 0;
  while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
    set_menu_itemon++;
  (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
  }

// The drawing part of the Key Bindings Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawKeybnd(void)

  {
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  // Set up the Key Binding screen 

  M_DrawBackground("FLOOR4_6"); // Draw background
  // proff/nicolas 09/20/98 -- changed for hi-res
  V_DrawNamePatch(84, 2, 0, "M_KEYBND", CR_DEFAULT, VPT_STRETCH);
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
  }

/////////////////////////////
//
// The Weapon Screen tables.

#define WP_X 203
#define WP_Y  31

// There's only one weapon settings screen (for now). But since we're
// trying to fit a common description for screens, it gets a setup_menu_t,
// which only has one screen definition in it.
//
// Note that this screen has no PREV or NEXT items, since there are no
// neighboring screens.

setup_menu_t weap_settings1[];

setup_menu_t* weap_settings[] =
  {
  weap_settings1,
  NULL
  };

setup_menu_t weap_settings1[] =  // Weapons Settings screen       
{
  {"ENABLE RECOIL"    ,S_YESNO,m_null,WP_X,WP_Y+ 1*8,0,0,0,&default_weapon_recoil ,&weapon_recoil,0,0},
  {"ENABLE BOBBING"   ,S_YESNO,m_null,WP_X,WP_Y+ 2*8,0,0,0,&default_player_bobbing,&player_bobbing,0,0},

  {"1ST CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+ 4*8,0,0,0,&weapon_preferences[0][0],0,0,0},
  {"2nd CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+ 5*8,0,0,0,&weapon_preferences[0][1],0,0,0},
  {"3rd CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+ 6*8,0,0,0,&weapon_preferences[0][2],0,0,0},
  {"4th CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+ 7*8,0,0,0,&weapon_preferences[0][3],0,0,0},
  {"5th CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+ 8*8,0,0,0,&weapon_preferences[0][4],0,0,0},
  {"6th CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+ 9*8,0,0,0,&weapon_preferences[0][5],0,0,0},
  {"7th CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+10*8,0,0,0,&weapon_preferences[0][6],0,0,0},
  {"8th CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+11*8,0,0,0,&weapon_preferences[0][7],0,0,0},
  {"9th CHOICE WEAPON",S_WEAP ,m_null,WP_X,WP_Y+12*8,0,0,0,&weapon_preferences[0][8],0,0,0},

  // Button for resetting to defaults

  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON,0,0,0,0,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};


// Setting up for the Weapons screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Weapons(int choice)
  {
  M_SetupNextMenu(&WeaponDef);

  setup_active = true;
  setup_screen = ss_weap;
  set_weapon_active = true;
  setup_select = false;
  default_verify = false;
  mult_screens_index = 0;
  current_setup_menu = weap_settings[0];
  set_menu_itemon = 0;
  while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
    set_menu_itemon++;
  (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
  }


// The drawing part of the Weapons Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawWeapons(void)

  {
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  M_DrawBackground("FLOOR4_6"); // Draw background
  // proff/nicolas 09/20/98 -- changed for hi-res
  V_DrawNamePatch(109, 2, 0, "M_WEAP", CR_DEFAULT, VPT_STRETCH);
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
  }

/////////////////////////////
//
// The Status Bar / HUD tables.

#define ST_X 203
#define ST_Y  31

// If a number item is being changed, allow up to 3 keystrokes to 'gather'
// the value. Gather_count tells you how many you have so far. The legality
// of what is gathered is determined by the low/high settings for the item.

#define MAXGATHER 3
int  gather_count;

// Screen table definitions

setup_menu_t stat_settings1[];

setup_menu_t* stat_settings[] =
  {
  stat_settings1,
  NULL
  };

setup_menu_t stat_settings1[] =  // Status Bar and HUD Settings screen       
{
  {"STATUS BAR"        ,(S_SKIP|S_TITLE),m_null,ST_X,ST_Y+ 1*8,0,0,0,0                ,0,0,0  },
  {"USE RED NUMBERS"   ,S_YESNO     ,m_null,ST_X,ST_Y+ 2*8,0,0,0,&sts_always_red      ,0,0,0  },
  {"GRAY %"            ,S_YESNO     ,m_null,ST_X,ST_Y+ 3*8,0,0,0,&sts_pct_always_gray ,0,0,0  },
  {"SINGLE KEY DISPLAY",S_YESNO     ,m_null,ST_X,ST_Y+ 4*8,0,0,0,&sts_traditional_keys,0,0,0  },

  {"HEADS-UP DISPLAY"  ,(S_SKIP|S_TITLE),m_null,ST_X,ST_Y+ 6*8,0,0,0,0                ,0,0,0  },
  {"HIDE SECRETS"      ,S_YESNO     ,m_null,ST_X,ST_Y+ 7*8,0,0,0,&hud_nosecrets       ,0,0,0  },
  {"HEALTH LOW/OK"     ,S_NUM       ,m_null,ST_X,ST_Y+ 8*8,0,0,0,&health_red          ,0,0,200},
  {"HEALTH OK/GOOD"    ,S_NUM       ,m_null,ST_X,ST_Y+ 9*8,0,0,0,&health_yellow       ,0,0,200},
  {"HEALTH GOOD/EXTRA" ,S_NUM       ,m_null,ST_X,ST_Y+10*8,0,0,0,&health_green        ,0,0,200},
  {"ARMOR LOW/OK"      ,S_NUM       ,m_null,ST_X,ST_Y+11*8,0,0,0,&armor_red           ,0,0,200},
  {"ARMOR OK/GOOD"     ,S_NUM       ,m_null,ST_X,ST_Y+12*8,0,0,0,&armor_yellow        ,0,0,200},
  {"ARMOR GOOD/EXTRA"  ,S_NUM       ,m_null,ST_X,ST_Y+13*8,0,0,0,&armor_green         ,0,0,200},
  {"AMMO LOW/OK"       ,S_NUM       ,m_null,ST_X,ST_Y+14*8,0,0,0,&ammo_red            ,0,0,100},
  {"AMMO OK/GOOD"      ,S_NUM       ,m_null,ST_X,ST_Y+15*8,0,0,0,&ammo_yellow         ,0,0,100},

  // Button for resetting to defaults

  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON,0,0,0,0,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};

// Setting up for the Status Bar / HUD screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_StatusBar(int choice)
  {
  M_SetupNextMenu(&StatusHUDDef);

  setup_active = true;
  setup_screen = ss_stat;
  set_status_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  gather_count = 0;
  mult_screens_index = 0;
  current_setup_menu = stat_settings[0];
  set_menu_itemon = 0;
  while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
    set_menu_itemon++;
  (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
  }


// The drawing part of the Status Bar / HUD Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawStatusHUD(void)

  {
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  M_DrawBackground("FLOOR4_6"); // Draw background
  // proff/nicolas 09/20/98 -- changed for hi-res
  V_DrawNamePatch(59, 2, 0, "M_STAT", CR_DEFAULT, VPT_STRETCH);
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
  }


/////////////////////////////
//
// The Automap tables.

#define AU_X    250
#define AU_Y     31
#define AU_PREV KB_PREV
#define AU_NEXT KB_NEXT

setup_menu_t auto_settings1[];       
setup_menu_t auto_settings2[];

setup_menu_t* auto_settings[] =
  {
  auto_settings1,
  auto_settings2,
  NULL
  };

setup_menu_t auto_settings1[] =  // 1st AutoMap Settings screen       
{
  {"background"                         ,S_COLOR,m_null,AU_X,AU_Y     ,0,0,0,&mapcolor_back,0,0,0},
  {"grid lines"                         ,S_COLOR,m_null,AU_X,AU_Y+ 1*8,0,0,0,&mapcolor_grid,0,0,0},
  {"normal 1s wall"                     ,S_COLOR,m_null,AU_X,AU_Y+ 2*8,0,0,0,&mapcolor_wall,0,0,0},
  {"line at floor height change"        ,S_COLOR,m_null,AU_X,AU_Y+ 3*8,0,0,0,&mapcolor_fchg,0,0,0},
  {"line at ceiling height change"      ,S_COLOR,m_null,AU_X,AU_Y+ 4*8,0,0,0,&mapcolor_cchg,0,0,0},
  {"line at sector with floor = ceiling",S_COLOR,m_null,AU_X,AU_Y+ 5*8,0,0,0,&mapcolor_clsd,0,0,0},
  {"red key"                            ,S_COLOR,m_null,AU_X,AU_Y+ 6*8,0,0,0,&mapcolor_rkey,0,0,0},
  {"blue key"                           ,S_COLOR,m_null,AU_X,AU_Y+ 7*8,0,0,0,&mapcolor_bkey,0,0,0},
  {"yellow key"                         ,S_COLOR,m_null,AU_X,AU_Y+ 8*8,0,0,0,&mapcolor_ykey,0,0,0},
  {"red door"                           ,S_COLOR,m_null,AU_X,AU_Y+ 9*8,0,0,0,&mapcolor_rdor,0,0,0},
  {"blue door"                          ,S_COLOR,m_null,AU_X,AU_Y+10*8,0,0,0,&mapcolor_bdor,0,0,0},
  {"yellow door"                        ,S_COLOR,m_null,AU_X,AU_Y+11*8,0,0,0,&mapcolor_ydor,0,0,0},

  // Button for resetting to defaults

  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON,0,0,0,0,0,0,0},

  {"NEXT ->",(S_SKIP|S_NEXT),m_null,AU_NEXT,AU_Y+20*8,0,0,0,(int *)auto_settings2,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};

setup_menu_t auto_settings2[] =  // 2nd AutoMap Settings screen
{
  {"teleporter line"                ,S_COLOR ,m_null,AU_X,AU_Y     ,0,0,0,&mapcolor_tele   ,0,0,0},
  {"secret sector boundary"         ,S_COLOR ,m_null,AU_X,AU_Y+ 1*8,0,0,0,&mapcolor_secr   ,0,0,0},
  //jff 4/23/98 add exit line to automap
  {"exit line"                      ,S_COLOR ,m_null,AU_X,AU_Y+ 2*8,0,0,0,&mapcolor_exit   ,0,0,0},
  {"computer map unseen line"       ,S_COLOR ,m_null,AU_X,AU_Y+ 3*8,0,0,0,&mapcolor_unsn   ,0,0,0},
  {"line w/no floor/ceiling changes",S_COLOR ,m_null,AU_X,AU_Y+ 4*8,0,0,0,&mapcolor_flat   ,0,0,0}, 
  {"general sprite"                 ,S_COLOR ,m_null,AU_X,AU_Y+ 5*8,0,0,0,&mapcolor_sprt   ,0,0,0},
  {"crosshair"                      ,S_COLOR ,m_null,AU_X,AU_Y+ 6*8,0,0,0,&mapcolor_hair   ,0,0,0},
  {"single player arrow"            ,S_COLOR ,m_null,AU_X,AU_Y+ 7*8,0,0,0,&mapcolor_sngl   ,0,0,0},
  {"your colour in multiplayer"     ,S_COLOR ,m_null,AU_X,AU_Y+ 8*8,0,0,0,&mapcolor_me,0,0,0},
  {"AUTOMAP LEVEL TITLE COLOR"      ,S_CRITEM,m_null,AU_X,AU_Y+13*8,0,0,0,&hudcolor_titl   ,0,0,9},
  {"AUTOMAP COORDINATES COLOR"      ,S_CRITEM,m_null,AU_X,AU_Y+14*8,0,0,0,&hudcolor_xyco   ,0,0,9},

  {"Show Secrets only after entering",S_YESNO,m_null,AU_X,AU_Y+16*8,0,0,0,&map_secret_after,0,0,0},

  {"<- PREV",(S_SKIP|S_PREV),m_null,AU_PREV,AU_Y+20*8,0,0,0,(int *)auto_settings1,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};


// Setting up for the Automap screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Automap(int choice)
  {
  M_SetupNextMenu(&AutoMapDef);

  setup_active = true;
  setup_screen = ss_auto;
  set_auto_active = true;
  setup_select = false;
  colorbox_active = false;
  default_verify = false;
  set_menu_itemon = 0;
  mult_screens_index = 0;
  current_setup_menu = auto_settings[0];
  while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
    set_menu_itemon++;
  (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
  }

// Data used by the color palette that is displayed for the player to
// select colors.

int color_palette_x; // X position of the cursor on the color palette
int color_palette_y; // Y position of the cursor on the color palette
byte palette_background[16*(CHIP_SIZE+1)+8];

// M_DrawColPal() draws the color palette when the user needs to select a
// color.

// phares 4/1/98: now uses a single lump for the palette instead of
// building the image out of individual paint chips.

static void M_DrawColPal(void) // CPhipps - static, void, formatting
{
  int cpx,cpy;

  // Draw a background, border, and paint chips

  // proff/nicolas 09/20/98 -- changed for hi-res
  // CPhipps - patch drawing updated
  V_DrawNamePatch(COLORPALXORIG-5, COLORPALYORIG-5, 0, "M_COLORS", CR_DEFAULT, VPT_STRETCH);

  // Draw the cursor around the paint chip
  // (cpx,cpy) is the upper left-hand corner of the paint chip

  // CPhipps - use memset
  memset(colorblock, PAL_WHITE, CHIP_SIZE+2);

  cpx = COLORPALXORIG+color_palette_x*(CHIP_SIZE+1)-1;
  cpy = COLORPALYORIG+color_palette_y*(CHIP_SIZE+1)-1;
  // CPhipps - block drawing updated, stretch for hi-res
  V_DrawBlock(cpx,             cpy, 0, CHIP_SIZE+2, 1, colorblock, VPT_STRETCH);
  V_DrawBlock(cpx+CHIP_SIZE+1, cpy, 0, 1, CHIP_SIZE+2, colorblock, VPT_STRETCH);
  V_DrawBlock(cpx, cpy+CHIP_SIZE+1, 0, CHIP_SIZE+2, 1, colorblock, VPT_STRETCH);
  V_DrawBlock(cpx,             cpy, 0, 1, CHIP_SIZE+2, colorblock, VPT_STRETCH);
}

// The drawing part of the Automap Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawAutoMap(void)

  {
  inhelpscreens = true;    // killough 4/6/98: Force status bar redraw

  M_DrawBackground("FLOOR4_6"); // Draw background
  // CPhipps - patch drawing updated
  V_DrawNamePatch(109, 2, 0, "M_AUTO", CR_DEFAULT, VPT_STRETCH);
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If a color is being selected, need to show color paint chips

  if (colorbox_active)
    M_DrawColPal();

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  else if (default_verify)
    M_DrawDefVerify();
  }


/////////////////////////////
//
// The Enemies table.

#define E_X 227
#define E_Y  31

setup_menu_t enem_settings1[];

setup_menu_t* enem_settings[] =
  {
  enem_settings1,
  NULL
  };

setup_menu_t enem_settings1[] =  // Enemy Settings screen       
{
  {"REMEMBER PREVIOUS TARGET",S_YESNO,m_null,E_X,E_Y+ 1*8,0,0,0,&default_monsters_remember ,&monsters_remember,0,0},

  // Button for resetting to defaults

  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON,0,0,0,0,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};


// Setting up for the Enemies screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Enemy(int choice)
  {
  M_SetupNextMenu(&EnemyDef);

  setup_active = true;
  setup_screen = ss_enem;
  set_enemy_active = true;
  setup_select = false;
  default_verify = false;
  mult_screens_index = 0;
  current_setup_menu = enem_settings[0];
  set_menu_itemon = 0;
  while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
    set_menu_itemon++;
  (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
  }


// The drawing part of the Enemies Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawEnemy(void)

  {
  inhelpscreens = true;

  M_DrawBackground("FLOOR4_6"); // Draw background
  // proff/nicolas 09/20/98 -- changed for hi-res
  V_DrawNamePatch(114, 2, 0, "M_ENEM", CR_DEFAULT, VPT_STRETCH);
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
  }


/////////////////////////////
//
// The Messages table.

#define M_X 238
#define M_Y  31

setup_menu_t mess_settings1[];

setup_menu_t* mess_settings[] =
  {
  mess_settings1,
  NULL
  };

setup_menu_t mess_settings1[] =  // Messages screen       
{
  {"MESSAGE BACKGROUND"       ,S_YESNO ,m_null,M_X,M_Y+ 1*8,0,0,0,&hud_list_bgon ,0,0,0 },
  {"# MESSAGE LINES"          ,S_NUM   ,m_null,M_X,M_Y+ 2*8,0,0,0,&hud_msg_lines ,0,1,16},
  {"MESSAGE COLOR DURING PLAY",S_CRITEM,m_null,M_X,M_Y+ 3*8,0,0,0,&hudcolor_mesg ,0,0,9 },
  {"MESSAGE REVIEW COLOR"     ,S_CRITEM,m_null,M_X,M_Y+ 4*8,0,0,0,&hudcolor_list ,0,0,9 },
  {"CHAT MESSAGE COLOR"       ,S_CRITEM,m_null,M_X,M_Y+ 5*8,0,0,0,&hudcolor_chat ,0,0,9 },

  // Button for resetting to defaults

  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON,0,0,0,0,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};


// Setting up for the Messages screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_Messages(int choice)
  {
  M_SetupNextMenu(&MessageDef);

  setup_active = true;
  setup_screen = ss_mess;
  set_mess_active = true;
  setup_select = false;
  default_verify = false;
  setup_gather = false;
  gather_count = 0;
  mult_screens_index = 0;
  current_setup_menu = mess_settings[0];
  set_menu_itemon = 0;
  while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
    set_menu_itemon++;
  (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
  }


// The drawing part of the Messages Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawMessages(void)

  {
  inhelpscreens = true;
  M_DrawBackground("FLOOR4_6"); // Draw background
  // CPhipps - patch drawing updated
  V_DrawNamePatch(103, 2, 0, "M_MESS", CR_DEFAULT, VPT_STRETCH);
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);
  if (default_verify)
    M_DrawDefVerify();
  }


/////////////////////////////
//
// The Chat Strings table.

#define CS_X 20
#define CS_Y (31+8)

setup_menu_t chat_settings1[];

setup_menu_t* chat_settings[] =
  {
  chat_settings1,
  NULL
  };

setup_menu_t chat_settings1[] =  // Chat Strings screen       
{
  {"1",S_CHAT,m_null,CS_X,CS_Y+ 1*8,0,0,0,(int *)&chat_macros[1],0,0,0},
  {"2",S_CHAT,m_null,CS_X,CS_Y+ 2*8,0,0,0,(int *)&chat_macros[2],0,0,0},
  {"3",S_CHAT,m_null,CS_X,CS_Y+ 3*8,0,0,0,(int *)&chat_macros[3],0,0,0},
  {"4",S_CHAT,m_null,CS_X,CS_Y+ 4*8,0,0,0,(int *)&chat_macros[4],0,0,0},
  {"5",S_CHAT,m_null,CS_X,CS_Y+ 5*8,0,0,0,(int *)&chat_macros[5],0,0,0},
  {"6",S_CHAT,m_null,CS_X,CS_Y+ 6*8,0,0,0,(int *)&chat_macros[6],0,0,0},
  {"7",S_CHAT,m_null,CS_X,CS_Y+ 7*8,0,0,0,(int *)&chat_macros[7],0,0,0},
  {"8",S_CHAT,m_null,CS_X,CS_Y+ 8*8,0,0,0,(int *)&chat_macros[8],0,0,0},
  {"9",S_CHAT,m_null,CS_X,CS_Y+ 9*8,0,0,0,(int *)&chat_macros[9],0,0,0},
  {"0",S_CHAT,m_null,CS_X,CS_Y+10*8,0,0,0,(int *)&chat_macros[0],0,0,0},

  // Button for resetting to defaults

  {0,S_RESET,m_null,X_BUTTON,Y_BUTTON,0,0,0,0,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}

};

// Setting up for the Chat Strings screen. Turn on flags, set pointers,
// locate the first item on the screen where the cursor is allowed to
// land.

void M_ChatStrings(int choice)
  {
  M_SetupNextMenu(&ChatStrDef);
  setup_active = true;
  setup_screen = ss_chat;
  set_chat_active = true;
  setup_select = false;
  default_verify = false;
  mult_screens_index = 0;
  current_setup_menu = chat_settings[0];
  set_menu_itemon = 0;
  while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
    set_menu_itemon++;
  (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
  }

// The drawing part of the Chat Strings Setup initialization. Draw the
// background, title, instruction line, and items.

void M_DrawChatStrings(void)

  {
  inhelpscreens = true;
  M_DrawBackground("FLOOR4_6"); // Draw background
  // CPhipps - patch drawing updated
  V_DrawNamePatch(83, 2, 0, "M_CHAT", CR_DEFAULT, VPT_STRETCH);
  M_DrawInstructions();
  M_DrawScreenItems(current_setup_menu);

  // If the Reset Button has been selected, an "Are you sure?" message
  // is overlayed across everything else.

  if (default_verify)
    M_DrawDefVerify();
  }

/////////////////////////////
//
// General routines used by the Setup screens.
//

static boolean shiftdown = false; // phares 4/10/98: SHIFT key down or not

// phares 4/17/98:
// M_SelectDone() gets called when you have finished entering your
// Setup Menu item change.

static void M_SelectDone(setup_menu_t* ptr) // CPhipps - static
{
  ptr->m_flags &= ~S_SELECT;
  ptr->m_flags |= S_HILITE;
  S_StartSound(NULL,sfx_itemup);
  setup_select = false;
  colorbox_active = false;
}

// phares 4/21/98:
// Array of setup screens used by M_ResetDefaults()

setup_menu_t** setup_screens[] =
  {
  0,
  keys_settings,
  weap_settings,
  stat_settings,
  auto_settings,
  enem_settings,
  mess_settings,
  chat_settings
  };

// phares 4/19/98:
// M_ResetDefaults() resets all values for a setup screen to default values

// CPhipps - static, void, formatting
static void M_ResetDefaults(void)
{
  int i;
  default_t* d;
  setup_menu_t** ptr1;
  setup_menu_t*  ptr2;

  // Look through the defaults table and reset every variable that
  // belongs to the group we're interested in.

  d = defaults;
  for (i = 0 ; i < numdefaults ; i++, d++)
    if (d->setupscreen == setup_screen) {
      if (IS_STRING(*d)) {
        free((void*)(*(d->location.ppsz)));
        *(d->location.ppsz) = strdup(d->defaultvalue.psz);
      } else
        *(d->location.pi) = d->defaultvalue.i;
    }

  // Look through the set of screens for any items that have default
  // fields.

  if (!demoplayback && !demorecording && !netgame) {
    ptr1 = setup_screens[setup_screen];
    for (i = 0 ; ptr1[i] ; i++)
      for (ptr2 = ptr1[i] ; !(ptr2->m_flags & S_END) ; ptr2++)
        if (ptr2->m_var2)
          *(ptr2->m_var2) = *(ptr2->m_var1);
  }
}

//
// End of Setup Screens.
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Start of Extended HELP screens               // phares 3/30/98
//
// The wad designer can define a set of extended HELP screens for their own
// information display. These screens should be 320x200 graphic lumps
// defined in a separate wad. They should be named "HELP01" through "HELP99".
// "HELP01" is shown after the regular BOOM Dynamic HELP screen, and ENTER
// and BACKSPACE keys move the player through the HELP set.
//
// Rather than define a set of menu definitions for each of the possible
// HELP screens, one definition is used, and is altered on the fly
// depending on what HELPnn lumps the game finds.

// phares 3/30/98:
// Extended Help Screen variables

int extended_help_count;   // number of user-defined help screens found
int extended_help_index;   // index of current extended help screen

menuitem_t ExtHelpMenu[] =
{
  {1,"",M_ExtHelpNextScreen,0}
};

menu_t ExtHelpDef =
  {
  1,             // # of menu items
  &ReadDef1,     // previous menu
  ExtHelpMenu,   // menuitem_t ->
  M_DrawExtHelp, // drawing routine ->
  330,181,       // x,y
  0              // lastOn
  };

// M_ExtHelpNextScreen establishes the number of the next HELP screen in
// the series.

void M_ExtHelpNextScreen(int choice)
  {
  choice = 0;
  if (++extended_help_index > extended_help_count)
    {

    // when finished with extended help screens, return to Main Menu

    extended_help_index = 1;
    M_SetupNextMenu(&MainDef);
    }
  }

// phares 3/30/98:
// Routine to look for HELPnn screens and create a menu
// definition structure that defines extended help screens.

void M_InitExtendedHelp(void)
{
  int index,i;
  char namebfr[10] = { "HELPnn" }; // CPhipps - make local & writable

  extended_help_count = 0;
  for (index = 1 ; index < 100 ; index++) {
    namebfr[4] = index/10 + 0x30;
    namebfr[5] = index%10 + 0x30;
    i = W_CheckNumForName(namebfr);
    if (i == -1) {
      if (extended_help_count) {
        if (gamemode == commercial) {
          ExtHelpDef.prevMenu  = &ReadDef1; // previous menu
          ReadMenu1[0].routine = M_ExtHelp;
	} else {
          ExtHelpDef.prevMenu  = &ReadDef2; // previous menu
          ReadMenu2[0].routine = M_ExtHelp;
	}
      }
      return;
    }
    extended_help_count++;
  }
}

// Initialization for the extended HELP screens.

void M_ExtHelp(int choice)
{
  choice = 0;
  extended_help_index = 1; // Start with first extended help screen
  M_SetupNextMenu(&ExtHelpDef);
}

// Initialize the drawing part of the extended HELP screens.

void M_DrawExtHelp(void)
{
  char namebfr[10] = { "HELPnn" }; // CPhipps - make it local & writable

  inhelpscreens = true;              // killough 5/1/98
  namebfr[4] = extended_help_index/10 + 0x30;
  namebfr[5] = extended_help_index%10 + 0x30;
  // CPhipps - patch drawing updated
  V_DrawNamePatch(0, 0, 0, namebfr, CR_DEFAULT, VPT_STRETCH);
}

//
// End of Extended HELP screens               // phares 3/30/98
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// Dynamic HELP screen                     // phares 3/2/98
//
// Rather than providing the static HELP screens from DOOM and its versions,
// BOOM provides the player with a dynamic HELP screen that displays the
// current settings of major key bindings.
//
// The Dynamic HELP screen is defined in a manner similar to that used for
// the Setup Screens above.
//
// M_GetKeyString finds the correct string to represent the key binding
// for the current item being drawn.

int M_GetKeyString(int c,int offset)
{
  const char* s = NULL; // CPhipps - CONST please

  if (c >= 33 && c <= 126) {

    // The '=', ',', and '.' keys originally meant the shifted
    // versions of those keys, but w/o having to shift them in
    // the game. Any actions that are mapped to these keys will
    // still mean their shifted versions. Could be changed later
    // if someone can come up with a better way to deal with them.

    if (c == '=')      // probably means the '+' key?
      c = '+';
    else if (c == ',') // probably means the '<' key?
      c = '<';
    else if (c == '.') // probably means the '>' key?
      c = '>';
    menu_buffer[offset++] = c; // Just insert the ascii key
    menu_buffer[offset] = 0;

  } else {

    // Retrieve 4-letter (max) string representing the key

    // cph - Keypad keys, general code reorganisation to 
    //  make this smaller and neater.
    if ((0x100 <= c) && (c < 0x200)) {
      if (c == KEYD_KEYPADENTER)
	s = "PADE";
      else {
	strcpy(&menu_buffer[offset], "PAD");
 	offset+=4;
	menu_buffer[offset-1] = c & 0xff;
	menu_buffer[offset] = 0;
      }
    } else if ((KEYD_F1 <= c) && (c < KEYD_F10)) {
      menu_buffer[offset++] = 'F';
      menu_buffer[offset++] = '1' + c - KEYD_F1;
      menu_buffer[offset]   = 0;
    } else {
      switch(c) {
      case KEYD_TAB:	    s = "TAB";  break;
      case KEYD_ENTER:	    s = "ENTR"; break;
      case KEYD_ESCAPE:	    s = "ESC";  break;
      case KEYD_SPACEBAR:   s = "SPAC"; break;
      case KEYD_BACKSPACE:  s = "BACK"; break;
      case KEYD_RCTRL:	    s = "CTRL"; break;
      case KEYD_LEFTARROW:  s = "LARR"; break;
      case KEYD_UPARROW:    s = "UARR"; break;
      case KEYD_RIGHTARROW: s = "RARR"; break;
      case KEYD_DOWNARROW:  s = "DARR"; break;
      case KEYD_RSHIFT:	    s = "SHFT"; break;
      case KEYD_RALT:       s = "ALT";  break;
      case KEYD_CAPSLOCK:   s = "CAPS"; break;
      case KEYD_SCROLLLOCK: s = "SCRL"; break;
      case KEYD_HOME:       s = "HOME"; break;
      case KEYD_PAGEUP:	    s = "PGUP"; break;
      case KEYD_END:        s = "END";  break;
      case KEYD_PAGEDOWN:   s = "PGDN"; break;
      case KEYD_INSERT:	    s = "INST"; break;
      case KEYD_F10:        s = "F10";  break;
      case KEYD_F11:        s = "F11";  break;
      case KEYD_F12:        s = "F12";  break;
      case KEYD_PAUSE:      s = "PAUS"; break;
      default:              s = "JUNK"; break;
      }

      if (s) { // cph - Slight code change
	strcpy(&menu_buffer[offset],s); // string to display
	offset += strlen(s);
      }
    }
  }
  return offset;
  }

//
// The Dynamic HELP screen table.

#define KT_X1 283
#define KT_X2 172
#define KT_X3  87

#define KT_Y1   2
#define KT_Y2 110
#define KT_Y3 102

setup_menu_t helpstrings[] =  // HELP screen strings       
{
  {"SCREEN"      ,(S_SKIP|S_TITLE),m_null,KT_X1,KT_Y1   ,0                ,0,0,0,0,0,0},
  {"HELP"        ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 1*8,&key_help        ,0,0,0,0,0,0},
  {"MENU"        ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 2*8,&key_escape      ,0,0,0,0,0,0},
  {"PAUSE"       ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 3*8,&key_pause       ,0,0,0,0,0,0},
  {"AUTOMAP"     ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 4*8,&key_map         ,0,0,0,0,0,0},
  {"SOUND VOLUME",(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 5*8,&key_soundvolume ,0,0,0,0,0,0},
  {"HUD"         ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 6*8,&key_hud         ,0,0,0,0,0,0},
  {"MESSAGES"    ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 7*8,&key_messages    ,0,0,0,0,0,0},
  {"GAMMA FIX"   ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 8*8,&key_gamma       ,0,0,0,0,0,0},
  {"SPY"         ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+ 9*8,&key_spy         ,0,0,0,0,0,0},
  {"LARGER VIEW" ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+10*8,&key_zoomin      ,0,0,0,0,0,0}, 
  {"SMALLER VIEW",(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+11*8,&key_zoomout     ,0,0,0,0,0,0},
  {"SCREENSHOT"  ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y1+12*8,&key_screenshot  ,0,0,0,0,0,0},

  {"AUTOMAP"     ,(S_SKIP|S_TITLE),m_null,KT_X1,KT_Y2   ,0                ,0,0,0,0,0,0},
  {"FOLLOW MODE" ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y2+ 1*8,&key_map_follow  ,0,0,0,0,0,0},
  {"ZOOM IN"     ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y2+ 2*8,&key_map_zoomin  ,0,0,0,0,0,0},
  {"ZOOM OUT"    ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y2+ 3*8,&key_map_zoomout ,0,0,0,0,0,0},
  {"MARK PLACE"  ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y2+ 4*8,&key_map_mark    ,0,0,0,0,0,0},
  {"CLEAR MARKS" ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y2+ 5*8,&key_map_clear   ,0,0,0,0,0,0},
  {"FULL/ZOOM"   ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y2+ 6*8,&key_map_gobig   ,0,0,0,0,0,0},
  {"GRID"        ,(S_SKIP|S_KEY),m_null,KT_X1,KT_Y2+ 7*8,&key_map_grid    ,0,0,0,0,0,0},

  {"WEAPONS"     ,(S_SKIP|S_TITLE),m_null,KT_X3,KT_Y1   ,0                ,0,0,0,0,0,0},
  {"FIST"        ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 1*8,&key_weapon1     ,0,0,0,0,0,0},
  {"PISTOL"      ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 2*8,&key_weapon2     ,0,0,0,0,0,0},
  {"SHOTGUN"     ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 3*8,&key_weapon3     ,0,0,0,0,0,0},
  {"CHAINGUN"    ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 4*8,&key_weapon4     ,0,0,0,0,0,0},
  {"ROCKET"      ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 5*8,&key_weapon5     ,0,0,0,0,0,0},
  {"PLASMA"      ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 6*8,&key_weapon6     ,0,0,0,0,0,0},
  {"BFG 9000"    ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 7*8,&key_weapon7     ,0,0,0,0,0,0},
  {"CHAINSAW"    ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 8*8,&key_weapon8     ,0,0,0,0,0,0},
  {"SSG"         ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+ 9*8,&key_weapon9     ,0,0,0,0,0,0},
  {"BEST"        ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+10*8,&key_weapontoggle,0,0,0,0,0,0},
  {"FIRE"        ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y1+11*8,&key_fire        ,&mousebfire,&joybfire,0,0,0,0},

  {"MOVEMENT"    ,(S_SKIP|S_TITLE),m_null,KT_X3,KT_Y3   ,0                ,0,0,0,0,0,0},
  {"FORWARD"     ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 1*8,&key_up          ,&mousebforward,0,0,0,0,0},
  {"BACKWARD"    ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 2*8,&key_down        ,0,0,0,0,0,0},
  {"TURN LEFT"   ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 3*8,&key_left        ,0,0,0,0,0,0},
  {"TURN RIGHT"  ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 4*8,&key_right       ,0,0,0,0,0,0},
  {"RUN"         ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 5*8,&key_speed       ,0,&joybspeed,0,0,0,0},
  {"STRAFE LEFT" ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 6*8,&key_strafeleft  ,0,0,0,0,0,0},
  {"STRAFE RIGHT",(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 7*8,&key_straferight ,0,0,0,0,0,0},
  {"STRAFE"      ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 8*8,&key_strafe      ,&mousebstrafe,&joybstrafe,0,0,0,0},
  {"AUTORUN"     ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+ 9*8,&key_autorun     ,0,0,0,0,0,0},
  {"180 TURN"    ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+10*8,&key_reverse     ,0,0,0,0,0,0},
  {"USE"         ,(S_SKIP|S_KEY),m_null,KT_X3,KT_Y3+11*8,&key_use         ,&mousebforward,&joybuse,0,0,0,0},

  {"GAME"        ,(S_SKIP|S_TITLE),m_null,KT_X2,KT_Y1   ,0                ,0,0,0,0,0,0},
  {"SAVE"        ,(S_SKIP|S_KEY),m_null,KT_X2,KT_Y1+ 1*8,&key_savegame    ,0,0,0,0,0,0},
  {"LOAD"        ,(S_SKIP|S_KEY),m_null,KT_X2,KT_Y1+ 2*8,&key_loadgame    ,0,0,0,0,0,0},
  {"QUICKSAVE"   ,(S_SKIP|S_KEY),m_null,KT_X2,KT_Y1+ 3*8,&key_quicksave   ,0,0,0,0,0,0},
  {"END GAME"    ,(S_SKIP|S_KEY),m_null,KT_X2,KT_Y1+ 4*8,&key_endgame     ,0,0,0,0,0,0},
  {"QUICKLOAD"   ,(S_SKIP|S_KEY),m_null,KT_X2,KT_Y1+ 5*8,&key_quickload   ,0,0,0,0,0,0},
  {"QUIT"        ,(S_SKIP|S_KEY),m_null,KT_X2,KT_Y1+ 6*8,&key_quit        ,0,0,0,0,0,0},

  // Final entry

  {0,(S_SKIP|S_END),m_null,0,0,0,0,0,0,0,0,0}
};

#define SPACEWIDTH 4

// M_DrawMenuString() draws the string in menu_buffer[]

void M_DrawMenuString(int cx, int cy, int color)
  {
  int   w;
  int   c;
  char* ch = menu_buffer;

  while (*ch)
    {
    c = *ch++;         // get next char
    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
      {
      cx += SPACEWIDTH;    // space
      continue;
      }
    w = SHORT (hu_font[c].width);
    if (cx + w > SCREENWIDTH)
      break;
    
    // V_DrawpatchTranslated() will draw the string in the
    // desired color, colrngs[color]
    
    // CPhipps - patch drawing updated
    V_DrawNumPatch(cx, cy, 0, hu_font[c].lumpnum, color, VPT_STRETCH | VPT_TRANS); 
    cx += w - 1; // The screen is cramped, so trim one unit from each
                 // character so they butt up against each other.
    }
  }

// M_GetPixelWidth() returns the number of pixels in the width of
// the string, NOT the number of chars in the string.

int M_GetPixelWidth(char* ch)
  {
  int len = 0;
  int c;

  while (*ch)
    {
    c = *ch++;    // pick up next char
    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c > HU_FONTSIZE)
      {
      len += SPACEWIDTH;   // space
      continue;
      }
    len += SHORT (hu_font[c].width);
    len--; // adjust so everything fits
    }
  len++; // replace what you took away on the last char only
  return len;
  }


//
// M_DrawHelp
//
// This displays the help screen

void M_DrawHelp (void)
  {
  M_DrawBackground("FLOOR4_6");

  V_MarkRect (0,0,SCREENWIDTH,SCREENHEIGHT);

  M_DrawScreenItems(helpstrings);
  }
  
//
// End of Dynamic HELP screen                // phares 3/2/98
//
////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// M_Responder
//
// Examines incoming keystrokes and button pushes and determines some
// action based on the state of the system.
//
// CPhipps - remove the tons of I_GetTime() references here, not needed
boolean M_Responder (event_t* ev)
  {
  int    ch;
  int    i;
  static int joywait   = 0;
  static int mousewait = 0;
  static int mousey    = 0;
  static int lasty     = 0;
  static int mousex    = 0;
  static int lastx     = 0;
  
  ch = -1; // will be changed to a legit char if we're going to use it here

  // Process joystick input

  if (ev->type == ev_joystick && joywait < gametic)
    {
    if (ev->data3 == -1)
      {
      ch = key_menu_up;                                      // phares 3/7/98
      joywait = gametic + 5;
      }
    else if (ev->data3 == 1)
      {
      ch = key_menu_down;                                    // phares 3/7/98
      joywait = gametic + 5;
      }
  
    if (ev->data2 == -1)
      {
      ch = key_menu_left;                                    // phares 3/7/98
      joywait = gametic + 2;
      }
    else if (ev->data2 == 1)
      {
      ch = key_menu_right;                                   // phares 3/7/98
      joywait = gametic + 2;
      }
  
    if (ev->data1&1)
      {
      ch = key_menu_enter;                                   // phares 3/7/98
      joywait = gametic + 5;
      }

    if (ev->data1&2)
      {
      ch = key_menu_backspace;                               // phares 3/7/98
      joywait = gametic + 5;
      }

    // phares 4/4/98:
    // Handle joystick buttons 3 and 4, and allow them to pass down
    // to where key binding can eat them.

    if (setup_active && set_keybnd_active)
      {
      if (ev->data1&4)
        {
        ch = 0; // meaningless, just to get you past the check for -1
        joywait = gametic + 5;
        }
      if (ev->data1&8)
        {
        ch = 0; // meaningless, just to get you past the check for -1
        joywait = gametic + 5;
        }
      }

    }
  else
    {

    // Process mouse input

    if (ev->type == ev_mouse && mousewait < gametic)
      {
      mousey += ev->data3;
      if (mousey < lasty-30)
        {
        ch = key_menu_down;                                  // phares 3/7/98
        mousewait = gametic + 5;
        mousey = lasty -= 30;
        }
      else if (mousey > lasty+30)
        {
        ch = key_menu_up;                                    // phares 3/7/98
        mousewait = gametic + 5;
        mousey = lasty += 30;
        }
  
      mousex += ev->data2;
      if (mousex < lastx-30)
        {
        ch = key_menu_left;                                  // phares 3/7/98
        mousewait = gametic + 5;
        mousex = lastx -= 30;
        }
      else if (mousex > lastx+30)
        {
        ch = key_menu_right;                                 // phares 3/7/98
        mousewait = gametic + 5;
        mousex = lastx += 30;
        }
  
      if (ev->data1&1)
        {
        ch = key_menu_enter;                                 // phares 3/7/98
        mousewait = gametic + 15;
        }
    
      if (ev->data1&2)
        {
        ch = key_menu_backspace;                             // phares 3/7/98
        mousewait = gametic + 15;
        }

      // phares 4/4/98:
      // Handle mouse button 3, and allow it to pass down
      // to where key binding can eat it.

      if (setup_active && set_keybnd_active)
        if (ev->data1&4)
          {
          ch = 0; // meaningless, just to get you past the check for -1
          mousewait = gametic + 15;
          }
      }
    else
        
      // Process keyboard input

      if (ev->type == ev_keydown)
        {
        ch = ev->data1;               // phares 4/11/98:
        if (ch == KEYD_RSHIFT)        // For chat string processing, need
          shiftdown = true;           // to know when shift key is up or
        }                             // down so you can get at the !,#,
      else if (ev->type == ev_keyup)  // etc. keys. Keydowns are allowed
        if (ev->data1 == KEYD_RSHIFT) // past this point, but keyups aren't
          shiftdown = false;          // so we need to note the difference
    }                                 // here using the 'shiftdown' boolean.
  
  if (ch == -1)
    return false; // we can't use the event here

  // Save Game string input

  if (saveStringEnter)
    {
    if (ch == key_menu_backspace)                            // phares 3/7/98
      {
      if (saveCharIndex > 0)
        {
        saveCharIndex--;
        savegamestrings[saveSlot][saveCharIndex] = 0;
        }
      }

    else if (ch == key_menu_escape)                          // phares 3/7/98
      {
      saveStringEnter = 0;
      strcpy(&savegamestrings[saveSlot][0],saveOldString);
      }
    
    else if (ch == key_menu_enter)                           // phares 3/7/98
      {
      saveStringEnter = 0;
      if (savegamestrings[saveSlot][0])
        M_DoSave(saveSlot);
      }
    
    else
      {
      ch = toupper(ch);
      if (ch != 32)
        if (ch-HU_FONTSTART < 0 || ch-HU_FONTSTART >= HU_FONTSIZE)
          ; // if true, do nothing
      if (ch >= 32 && ch <= 127 &&
          saveCharIndex < SAVESTRINGSIZE-1 &&
          M_StringWidth(savegamestrings[saveSlot]) < (SAVESTRINGSIZE-2)*8)
        {
        savegamestrings[saveSlot][saveCharIndex++] = ch;
        savegamestrings[saveSlot][saveCharIndex] = 0;
        }
      }
    return true;
    }
  
  // Take care of any messages that need input

  if (messageToPrint)
    {
    if (messageNeedsInput == true &&
        !(ch == ' ' || ch == 'n' || ch == 'y' || ch == key_escape)) // phares
      return false;
  
    menuactive = messageLastMenuActive;
    messageToPrint = 0;
    if (messageRoutine)
      messageRoutine(ch);
    
    menuactive = false;
    S_StartSound(NULL,sfx_swtchx);
    return true;
    }

  // killough 2/22/98: add support for screenshot key:

  if ((devparm && ch == key_help) || ch == key_screenshot)
    {
    G_ScreenShot ();
    return true;
    }

  // If there is no active menu displayed...

  if (!menuactive)                                                  // phares
    {                                                               //  |
    if (ch == key_autorun)      // Autorun                          //  V
      {
      autorun = !autorun;
      return true;
      }

    if (ch == key_help)      // Help key
      {
      M_StartControlPanel ();

      // killough 3/7/98:
      //
      // Here's where you're supposed to switch the
      // order in which the help menus are entered:

      if ( gamemode != commercial )   // killough 3/7/98
        currentMenu = &ReadDef2;
      else
        currentMenu = &ReadDef1;
      itemOn = 0;
      S_StartSound(NULL,sfx_swtchn);
      return true;
      }

    if (ch == key_savegame)     // Save Game
      {                             
      M_StartControlPanel();
      S_StartSound(NULL,sfx_swtchn);
      M_SaveGame(0);
      return true;
      }

    if (ch == key_loadgame)     // Load Game
      {
      M_StartControlPanel();
      S_StartSound(NULL,sfx_swtchn);
      M_LoadGame(0);
      return true;
      }

    if (ch == key_soundvolume)      // Sound Volume
      {
      M_StartControlPanel ();
      currentMenu = &SoundDef;
      itemOn = sfx_vol;
      S_StartSound(NULL,sfx_swtchn);
      return true;
      }

//jff        if (ch == key_hud)      // Used to be 'detail'; now HUD
//2/24/98      return false;
//no longer      /*    M_ChangeDetail(0);    obsolete -- killough
//used        S_StartSound(NULL,sfx_swtchn);
//            return true; */

    if (ch == key_quicksave)      // Quicksave
      {
      S_StartSound(NULL,sfx_swtchn);
      M_QuickSave();
      return true;
      }
    
    if (ch == key_endgame)      // End game
      {
      S_StartSound(NULL,sfx_swtchn);
      M_EndGame(0);
      return true;
      }
    
    if (ch == key_messages)      // Toggle messages
      {
      M_ChangeMessages(0);
      S_StartSound(NULL,sfx_swtchn);
      return true;
      }
    
    if (ch == key_quickload)      // Quickload
      {
      S_StartSound(NULL,sfx_swtchn);
      M_QuickLoad();
      return true;
      }
    
    if (ch == key_quit)       // Quit DOOM
      {
      S_StartSound(NULL,sfx_swtchn);
      M_QuitDOOM(0);
      return true;
      }
    
    if (ch == key_gamma)       // gamma toggle
      {
      usegamma++;
      if (usegamma > 4)
        usegamma = 0;
      players[consoleplayer].message =
        usegamma == 0 ? s_GAMMALVL0 :
        usegamma == 1 ? s_GAMMALVL1 :
        usegamma == 2 ? s_GAMMALVL2 :
        usegamma == 3 ? s_GAMMALVL3 :
        s_GAMMALVL4;
      V_SetPalette(0);
      return true;                      
      }


    if (ch == key_zoomout)     // zoom out
      {
      if ((automapmode & am_active) || chat_on)
        return false;
      M_SizeDisplay(0);
      S_StartSound(NULL,sfx_stnmov);
      return true;
      }
    
    if (ch == key_zoomin)               // zoom in
      {                                 // jff 2/23/98
      if ((automapmode & am_active) || chat_on)     // allow 
        return false;                   // key_hud==key_zoomin
      M_SizeDisplay(1);                                             //  ^
      S_StartSound(NULL,sfx_stnmov);                                //  |
      return true;                                                  // phares
      }
                                  
    if (ch == key_hud)   // heads-up mode       
      {                    
      if ((automapmode & am_active) || chat_on)    // jff 2/22/98
        return false;                  // HUD mode control
      if (screenSize<8)                // function on default F5
        while (screenSize<8 || !hud_displayed) // make hud visible
          M_SizeDisplay(1);            // when configuring it
      else
        {
        hud_displayed = 1;               //jff 3/3/98 turn hud on
        hud_active = (hud_active+1)%3;   // cycle hud_active
        if (!hud_active)                 //jff 3/4/98 add distributed
          {
          hud_distributed = !hud_distributed; // to cycle
          HU_MoveHud(); //jff 3/9/98 move it now to avoid glitch
          }
        }
      return true;
      }
    }                               
  
  // Pop-up Main menu?

  if (!menuactive)
    {
    if (ch == key_escape)                                           // phares
      {
      M_StartControlPanel ();
      S_StartSound(NULL,sfx_swtchn);
      return true;
      }
    return false;
    }

  // phares 3/26/98 - 4/11/98:
  // Setup screen key processing

  if (setup_active)
    {
    setup_menu_t* ptr1= current_setup_menu + set_menu_itemon;
    setup_menu_t* ptr2 = NULL;

    // phares 4/19/98:
    // Catch the response to the 'reset to default?' verification
    // screen

    if (default_verify)
      {
      if (toupper(ch) == 'Y')
        {
        M_ResetDefaults();
        default_verify = false;
        M_SelectDone(ptr1);
        }
      else if (toupper(ch) == 'N')
        {
        default_verify = false;
        M_SelectDone(ptr1);
        }
      return true;
      }

    // Common processing for some items

    if (setup_select) // changing an entry
      {
      if (ch == key_menu_escape) // Exit key = no change
        {
        M_SelectDone(ptr1);                                 // phares 4/17/98
        return true;
        }

      if (ptr1->m_flags & S_YESNO) // yes or no setting?
        {
        if (ch == key_menu_enter)
          {
          if (*(ptr1->m_var1))
            *(ptr1->m_var1) = 0; // turn off
          else
            *(ptr1->m_var1) = 1; // turn on

          // phares 4/14/98:
          // If not in demoplayback, demorecording, or netgame,
          // and there's a second variable in var2, set that
          // as well

          if (!demoplayback && !demorecording &&
            !netgame && ptr1->m_var2)
            *(ptr1->m_var2) = *(ptr1->m_var1);
          }
        M_SelectDone(ptr1);                                 // phares 4/17/98
        return true;
        }

      if (ptr1->m_flags & S_CRITEM)
        {
        if (ch != key_menu_enter)
          {
          ch -= 0x30; // out of ascii
          if (ch < 0 || ch > 9)
            return true; // ignore
          *(ptr1->m_var1) = ch;
          }
        M_SelectDone(ptr1);                                 // phares 4/17/98
        return true;
        }

      if (ptr1->m_flags & S_NUM) // number?
        {
        if (setup_gather) // gathering keys for a value?
          {
          int value;
    
          if (ch == key_menu_enter)
            {
            M_SelectDone(ptr1);     // phares 4/17/98
            setup_gather = false; // finished gathering keys
            return true;
            }
          if (ch < 0x30 || ch > 0x39)
            return true; // ignore
          if (gather_count == MAXGATHER)
            return true; // ignore
          value = 10*(*(ptr1->m_var1)) + ch - 0x30;
          if (value < ptr1->m_low || value > ptr1->m_high)
            return true; // ignore
          gather_count++;
          *(ptr1->m_var1) = value;
          }
        return true;
        }
      }

    // Key Bindings

    if (set_keybnd_active) // on a key binding setup screen
      if (setup_select)    // incoming key or button gets bound
        {
        if (ev->type == ev_joystick)
          {
          int i,oldbutton,group;
          boolean search = true;
      
          if (ptr1->m_joy == NULL)
            return true; // not a legal action here (yet)
      
          // see if the button is already bound elsewhere. if so, you
          // have to swap bindings so the action where it's currently
          // bound doesn't go dead. Since there is more than one
          // keybinding screen, you have to search all of them for
          // any duplicates. You're only interested in the items
          // that belong to the same group as the one you're changing.
      
          oldbutton = *(ptr1->m_joy);
          group  = ptr1->m_group;
          if (ev->data1 & 1)
            ch = 0;
          else if (ev->data1 & 2)
            ch = 1;
          else if (ev->data1 & 4)
            ch = 2;
          else if (ev->data1 & 8)
            ch = 3;
          else
            return true;
          for (i = 0 ; keys_settings[i] && search ; i++)
            for (ptr2 = keys_settings[i] ; !(ptr2->m_flags & S_END) ; ptr2++)
              if ((ptr2->m_group == group) && (ptr1 != ptr2))
                if ((ptr2->m_flags & S_KEY) && (ptr2->m_joy))
                  if (*(ptr2->m_joy) == ch)
                    {
                    *(ptr2->m_joy) = oldbutton;
                    search = false;
                    break;
                    }
          *(ptr1->m_joy) = ch;
          }
        else if (ev->type == ev_mouse)
          {
          int i,oldbutton,group;
          boolean search = true;

          if (ptr1->m_mouse == NULL)
            return true; // not a legal action here (yet)

          // see if the button is already bound elsewhere. if so, you
          // have to swap bindings so the action where it's currently
          // bound doesn't go dead. Since there is more than one
          // keybinding screen, you have to search all of them for
          // any duplicates. You're only interested in the items
          // that belong to the same group as the one you're changing.

          oldbutton = *(ptr1->m_mouse);
          group  = ptr1->m_group;
          if (ev->data1 & 1)
            ch = 0;
          else if (ev->data1 & 2)
            ch = 1;
          else if (ev->data1 & 4)
            ch = 2;
          else
            return true;
          for (i = 0 ; keys_settings[i] && search ; i++)
            for (ptr2 = keys_settings[i] ; !(ptr2->m_flags & S_END) ; ptr2++)
              if ((ptr2->m_group == group) && (ptr1 != ptr2))
                if ((ptr2->m_flags & S_KEY) && (ptr2->m_mouse))
                  if (*(ptr2->m_mouse) == ch)
                    {
                    *(ptr2->m_mouse) = oldbutton;
                    search = false;
                    break;
                    }
          *(ptr1->m_mouse) = ch;
          }
        else  // keyboard key
          {
          int i,oldkey,group;
          boolean search = true;
        
          // see if 'ch' is already bound elsewhere. if so, you have
          // to swap bindings so the action where it's currently
          // bound doesn't go dead. Since there is more than one
          // keybinding screen, you have to search all of them for
          // any duplicates. You're only interested in the items
          // that belong to the same group as the one you're changing.

          // if you find that you're trying to swap with an action
          // that has S_KEEP set, you can't bind ch; it's already
          // bound to that S_KEEP action, and that action has to
          // keep that key.

          oldkey = *(ptr1->m_key);
          group  = ptr1->m_group;
          for (i = 0 ; keys_settings[i] && search ; i++)
            for (ptr2 = keys_settings[i] ; !(ptr2->m_flags & S_END) ; ptr2++)
              if ((ptr2->m_flags & (S_KEY|S_KEEP)) &&
                (ptr2->m_group == group)  && 
                (ptr1 != ptr2))
                if (*(ptr2->m_key) == ch)
                  {
                  if (ptr2->m_flags & S_KEEP)
                    return true; // can't have it!
                  *(ptr2->m_key) = oldkey;
                  search = false;
                  break;
                  }
          *(ptr1->m_key) = ch;
          }

        M_SelectDone(ptr1);             // phares 4/17/98
        return true;
        }

    // Weapons

    if (set_weapon_active) // on the weapons setup screen
      if (setup_select) // changing an entry
        {
        if (ch != key_menu_enter)
          {
          int oldass;
          boolean search = true;
      
          ch -= 0x30; // out of ascii
          if (ch < 1 || ch > 9)
            return true; // ignore
              
          // Plasma and BFG don't exist in shareware
              
          if (gamemode == shareware)
            if ((ch == (wp_plasma+1)) || (ch == (wp_bfg+1))) 
              return true;
            
          // SSG doesn't exist in shareware, registered, or
          // retail DOOM 1
            
          if (gamemode != commercial)
            if (ch == (wp_supershotgun+1))
              return true;
            
          // see if 'ch' is already assigned elsewhere. if so,
          // you have to swap assignments.
            
          oldass = *(ptr1->m_var1);
          for (i = 0 ; weap_settings[i] && search ; i++)
            for (ptr2 = weap_settings[i] ; !(ptr2->m_flags & S_END) ; ptr2++)
              if ((ptr2->m_flags & S_WEAP) && (ptr1 != ptr2))
                if (*(ptr2->m_var1) == ch)
                  {
                  *(ptr2->m_var1) = oldass;
                  search = false;
                  break;
                  }
          *(ptr1->m_var1) = ch;
          }

        M_SelectDone(ptr1);             // phares 4/17/98
        return true;
        }

    // Status Bar and Heads-Up Display

    if (set_status_active) // on the status bar & hud setup screen
      if (setup_select) // changing an entry
        {

        // Here's where status/hud-specific setup items go.

        M_SelectDone(ptr1);             // phares 4/17/98
        return true;
        }

    // Automap

    if (set_auto_active) // on the automap setup screen
      if (setup_select) // incoming key
        {
        if (ch == key_menu_down)                
          {
          if (++color_palette_y == 16)
            color_palette_y = 0;
          S_StartSound(NULL,sfx_itemup);
          return true;
          }

        if (ch == key_menu_up)                
          {
          if (--color_palette_y < 0)
            color_palette_y = 15;
          S_StartSound(NULL,sfx_itemup);
          return true;
          }

        if (ch == key_menu_left)                
          {
          if (--color_palette_x < 0)
            color_palette_x = 15;
          S_StartSound(NULL,sfx_itemup);
          return true;
          }

        if (ch == key_menu_right)                
          {
          if (++color_palette_x == 16)
            color_palette_x = 0;
          S_StartSound(NULL,sfx_itemup);
          return true;
          }

        if (ch == key_menu_enter)               
          {
          *(ptr1->m_var1) = color_palette_x + 16*color_palette_y;
          M_SelectDone(ptr1);                               // phares 4/17/98
          colorbox_active = false;
          return true;
          }
        }

    // Enemies

    if (set_enemy_active) // on the enemy setup screen
      if (setup_select) // changing an entry
        {

        // Here's where enemy-specific setup items go.
        // A good place for the Phase II AI changes, if any.

        M_SelectDone(ptr1);                                 // phares 4/17/98
        return true;
        }

    // Messages

    if (set_mess_active) // on the messages setup screen
      if (setup_select)  // changing an entry
        {

        // Here's where message-specific setup items go.

        M_SelectDone(ptr1);                                 // phares 4/17/98
        return true;
        }

    // Chat Strings

    if (set_chat_active)  // on the Chat Strings setup screen
      if (setup_select) // changing an entry
        {
        if (ptr1->m_flags & S_CHAT) // creating/editing a string?
          {
          if (ch == key_menu_backspace) // backspace and DEL
            {
            if (chat_string_buffer[chat_index] == 0)
              {
              if (chat_index > 0)
                chat_string_buffer[--chat_index] = 0;
              }
             // shift the remainder of the text one char left
            else
              strcpy(&chat_string_buffer[chat_index],
                   &chat_string_buffer[chat_index+1]);
            }
          else if (ch == key_menu_left) // move cursor left
            {
            if (chat_index > 0)
              chat_index--;
            }
          else if (ch == key_menu_right) // move cursor right
            {
            if (chat_string_buffer[chat_index] != 0)
              chat_index++;
            }
          else if ((ch == key_menu_enter) ||
               (ch == key_menu_escape))
            {
            *((char**)(ptr1->m_var1)) = chat_string_buffer;
            M_SelectDone(ptr1);         // phares 4/17/98
            }

          // Adding a char to the text. Has to be a printable
          // char, and you can't overrun the buffer. If the
          // chat string gets larger than what the screen can hold,
          // it is dealt with when the string is drawn (above).

          else if ((ch >= 32) && (ch <= 126))
            if ((chat_index+1) < CHAT_STRING_BFR_SIZE)
              {
              if (shiftdown)
                ch = shiftxform[ch];
              if (chat_string_buffer[chat_index] == 0) 
                {
                chat_string_buffer[chat_index++] = ch;
                chat_string_buffer[chat_index] = 0;
                }
              else
                chat_string_buffer[chat_index++] = ch;
              }
          return true;
          }

        M_SelectDone(ptr1);             // phares 4/17/98
        return true;
        }

    // Not changing any items on the Setup screens. See if we're
    // navigating the Setup menus or selecting an item to change.

    if (ch == key_menu_down)
      {
      ptr1->m_flags &= ~S_HILITE;           // phares 4/17/98
      do
        if (ptr1->m_flags & S_END)
          {
          set_menu_itemon = 0;
          ptr1 = current_setup_menu;
          }
        else
          {
          set_menu_itemon++;
          ptr1++;
          }
      while (ptr1->m_flags & S_SKIP);
      M_SelectDone(ptr1);               // phares 4/17/98
      return true;
      }
  
    if (ch == key_menu_up)                 
      {
      ptr1->m_flags &= ~S_HILITE;           // phares 4/17/98
      do
        {
        if (set_menu_itemon == 0)
          do
            set_menu_itemon++;
          while(!((current_setup_menu + set_menu_itemon)->m_flags & S_END));
        set_menu_itemon--;
        }
      while((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP);
      M_SelectDone(current_setup_menu + set_menu_itemon);               // phares 4/17/98
      return true;
      }

    if (ch == key_menu_enter)               
      {
      int flags = ptr1->m_flags;

      // You've selected an item to change. Highlight it, post a new
      // message about what to do, and get ready to process the
      // change.

      if (flags & S_NUM)
        {
        setup_gather = true;
        *(ptr1->m_var1) = 0; // clear so you can rebuild it
        gather_count = 0;
        }

      else if (flags & S_COLOR)
        {
        int color = *(ptr1->m_var1);
        
        if (color < 0 || color > 255) // range check the value
          color = 0;        // 'no show' if invalid
        color_palette_x = (*(ptr1->m_var1))&15;
        color_palette_y = (*(ptr1->m_var1))>>4;
        colorbox_active = true;
        }

      else if (flags & S_CHAT)
        {

        // copy chat string into working buffer; trim if needed.
        // free the old chat string memory and replace it with
        // the (possibly larger) new memory for editing purposes

        chat_string_buffer = malloc(CHAT_STRING_BFR_SIZE);
        strncpy(chat_string_buffer,*((char**)(ptr1->m_var1)),CHAT_STRING_BFR_SIZE);
        chat_string_buffer[CHAT_STRING_BFR_SIZE-1] = 0; // gurantee null delimiter

        // set chat table pointer to working buffer
        // and free old string's memory.

        free(*((char**)(ptr1->m_var1)));
        *((char**)(ptr1->m_var1)) = chat_string_buffer;
        chat_index = 0; // current cursor position in chat_string_buffer
        }

      else if (flags & S_RESET)
        default_verify = true;

      ptr1->m_flags |= S_SELECT;
      setup_select = true;
      S_StartSound(NULL,sfx_itemup);
      return true;
      }

    if ((ch == key_menu_escape) || (ch == key_menu_backspace))
      {
      if (ch == key_menu_escape) // Clear all menus
        M_ClearMenus();
      else // key_menu_backspace = return to Setup Menu
        if (currentMenu->prevMenu)
          {
          currentMenu = currentMenu->prevMenu;
          itemOn = currentMenu->lastOn;
          S_StartSound(NULL,sfx_swtchn);
          }
      ptr1->m_flags &= ~(S_HILITE|S_SELECT);      // phares 4/19/98
      setup_active = false;
      set_keybnd_active = false;
      set_weapon_active = false;
      set_status_active = false;
      set_auto_active = false;
      set_enemy_active = false;
      set_mess_active = false;
      set_chat_active = false;
      colorbox_active = false;
      default_verify = false;             // phares 4/19/98
      HU_Start();    // catch any message changes // phares 4/19/98
      S_StartSound(NULL,sfx_swtchx);
      return true;
      }

    // Some setup screens may have multiple screens.
    // When there are multiple screens, m_prev and m_next items need to
    // be placed on the appropriate screen tables so the user can
    // move among the screens using the left and right arrow keys.
    // The m_var1 field contains a pointer to the appropriate screen
    // to move to.

    if (ch == key_menu_left)
      {
      ptr2 = ptr1;
      do
        {
        ptr2++;
        if (ptr2->m_flags & S_PREV)
          {
          ptr1->m_flags &= ~S_HILITE;
          mult_screens_index--;
          current_setup_menu = (setup_menu_t *) ptr2->m_var1;
          set_menu_itemon = 0;
          while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
            set_menu_itemon++;
          (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
          S_StartSound(NULL,sfx_swtchx);
          return true;
          }
        }
      while (!(ptr2->m_flags & S_END));
      }

    if (ch == key_menu_right)                
      {
      ptr2 = ptr1;
      do
        {
        ptr2++;
        if (ptr2->m_flags & S_NEXT)
          {
          ptr1->m_flags &= ~S_HILITE;
          mult_screens_index++;
          current_setup_menu = (setup_menu_t *) ptr2->m_var1;
          set_menu_itemon = 0;
          while ((current_setup_menu + set_menu_itemon)->m_flags & S_SKIP)
            set_menu_itemon++;
          (current_setup_menu + set_menu_itemon)->m_flags |= S_HILITE;
          S_StartSound(NULL,sfx_swtchx);
          return true;
          }
        }
      while (!(ptr2->m_flags & S_END));
      }

    } // End of Setup Screen processing

  // From here on, these navigation keys are used on the BIG FONT menus
  // like the Main Menu.

  if (ch == key_menu_down)                                   // phares 3/7/98
    {
    do
      {
      if (itemOn+1 > currentMenu->numitems-1)
        itemOn = 0;
      else
        itemOn++;
      S_StartSound(NULL,sfx_pstop);
      }
    while(currentMenu->menuitems[itemOn].status==-1);
    return true;
    }
  
  if (ch == key_menu_up)                                     // phares 3/7/98
    {
    do
      {
      if (!itemOn)
        itemOn = currentMenu->numitems-1;
      else
        itemOn--;
      S_StartSound(NULL,sfx_pstop);
      }
    while(currentMenu->menuitems[itemOn].status==-1);
    return true;
    }

  if (ch == key_menu_left)                                   // phares 3/7/98
    {
    if (currentMenu->menuitems[itemOn].routine &&
      currentMenu->menuitems[itemOn].status == 2)
      {
      S_StartSound(NULL,sfx_stnmov);
      currentMenu->menuitems[itemOn].routine(0);
      }
    return true;
    }
  
  if (ch == key_menu_right)                                  // phares 3/7/98
    {
    if (currentMenu->menuitems[itemOn].routine &&
      currentMenu->menuitems[itemOn].status == 2)
      {
      S_StartSound(NULL,sfx_stnmov);
      currentMenu->menuitems[itemOn].routine(1);
      }
    return true;
    }

  if (ch == key_menu_enter)                                  // phares 3/7/98
    {
    if (currentMenu->menuitems[itemOn].routine &&
        currentMenu->menuitems[itemOn].status)
      {
      currentMenu->lastOn = itemOn;
      if (currentMenu->menuitems[itemOn].status == 2)
        {
        currentMenu->menuitems[itemOn].routine(1);   // right arrow
        S_StartSound(NULL,sfx_stnmov);
        }
      else
        {
        currentMenu->menuitems[itemOn].routine(itemOn);
        S_StartSound(NULL,sfx_pistol);
        }
      }
    if (currentMenu==&NewDef) //jff 3/24/98 remember last skill selected
      defaultskill = currentMenu->lastOn+1;
    return true;
    }
  
  if (ch == key_menu_escape)                                 // phares 3/7/98  
    {
    currentMenu->lastOn = itemOn;
    M_ClearMenus ();
    S_StartSound(NULL,sfx_swtchx);
    return true;
    }
  
  if (ch == key_menu_backspace)                              // phares 3/7/98
    {
    currentMenu->lastOn = itemOn;

    // phares 3/30/98:
    // add checks to see if you're in the extended help screens
    // if so, stay with the same menu definition, but bump the
    // index back one. if the index bumps back far enough ( == 0)
    // then you can return to the Read_Thisn menu definitions

    if (currentMenu->prevMenu)
      {
      if (currentMenu == &ExtHelpDef)
        {
        if (--extended_help_index == 0)
          {
          currentMenu = currentMenu->prevMenu;
          extended_help_index = 1; // reset
          }
        }
      else
        currentMenu = currentMenu->prevMenu;
      itemOn = currentMenu->lastOn;
      S_StartSound(NULL,sfx_swtchn);
      }
    return true;
    }
  
  else
    {
    for (i = itemOn+1;i < currentMenu->numitems;i++)
      if (currentMenu->menuitems[i].alphaKey == ch)
        {
        itemOn = i;
        S_StartSound(NULL,sfx_pstop);
        return true;
        }
    for (i = 0;i <= itemOn;i++)
      if (currentMenu->menuitems[i].alphaKey == ch)
        {
        itemOn = i;
        S_StartSound(NULL,sfx_pstop);
        return true;
        }
    }
  return false;
  }

//
// End of M_Responder
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// General Routines
//
// This displays the Main menu and gets the menu screens rolling.
// Plus a variety of routines that control the Big Font menu display.
// Plus some initialization for game-dependant situations.

void M_StartControlPanel (void)
  {
  // intro might call this repeatedly

  if (menuactive)
    return;

  //jff 3/24/98 make default skill menu choice follow -skill or defaultskill
  //from command line or config file

  if (startskill != sk_none)
    NewDef.lastOn = (enum newgame_e)startskill;
  
  menuactive = 1;
  currentMenu = &MainDef;         // JDC
  itemOn = currentMenu->lastOn;   // JDC
  }

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//

void M_Drawer (void)
  {
  static short  x;
  static short  y;
  short  i;
  short  max;
#define M_LINE_LEN 50
  char   string[M_LINE_LEN];
  int    start;

  inhelpscreens = false;

  
  // Horiz. & Vertically center string and print it.
  if (messageToPrint)
    {
    start = 0;
    y = 100 - M_StringHeight(messageString)/2;
    while(*(messageString+start))
      {
      for (i = 0;(size_t)i < strlen(messageString+start);i++)
        if ((messageString[start+i] == '\n') || (i == M_LINE_LEN-1))
          {
          memset(string,0,40);
          strncpy(string,messageString+start,i);
          start += i;
	  if (messageString[start]=='\n') start++;
          break;
          }
    
      if ((size_t)i == strlen(messageString+start))
        {
        strcpy(string,messageString+start);
        start += i;
        }
    
      x = 160 - M_StringWidth(string)/2;
      M_WriteText(x,y,string);
      y += SHORT(hu_font[0].height);
      }
    return;
    }

  if (!menuactive)
    return;

  if (currentMenu->routine)
    currentMenu->routine();     // call Draw routine
  
  // DRAW MENU

  x = currentMenu->x;
  y = currentMenu->y;
  max = currentMenu->numitems;

  for (i=0;i<max;i++)
    {
    if (currentMenu->menuitems[i].name[0])
      // CPhipps - patch drawing updated
      V_DrawNamePatch(x, y, 0, currentMenu->menuitems[i].name, CR_DEFAULT, VPT_STRETCH);
    y += LINEHEIGHT;
    }
  
  // DRAW SKULL

  // CPhipps - patch drawing updated
  V_DrawNamePatch(x + SKULLXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT,0,
		  skullName[whichSkull], CR_DEFAULT, VPT_STRETCH);
  }

//
// M_ClearMenus
//
// Called when leaving the menu screens for the real world

void M_ClearMenus (void)
  {
  menuactive = 0;
  // if (!netgame && usergame && paused)
  //     sendpause = true;
  }

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
  {
  currentMenu = menudef;
  itemOn = currentMenu->lastOn;
  }

/////////////////////////////
//
// M_Ticker
//
void M_Ticker (void)
  {
  if (--skullAnimCounter <= 0)
    {
    whichSkull ^= 1;
    skullAnimCounter = 8;
    }
  }

/////////////////////////////
//
// Message Routines
//

void M_StartMessage (const char* string,void* routine,boolean input)
{
  messageLastMenuActive = menuactive;
  messageToPrint = 1;
  messageString = string;
  messageRoutine = routine;
  messageNeedsInput = input;
  menuactive = true;
  return;
}

void M_StopMessage(void)
  {
  menuactive = messageLastMenuActive;
  messageToPrint = 0;
  }

/////////////////////////////
//
// Thermometer Routines
//

//
// M_DrawThermo draws the thermometer graphic for Mouse Sensitivity,
// Sound Volume, etc.
//
// proff/nicolas 09/20/98 -- changed for hi-res
// CPhipps - patch drawing updated
//
void M_DrawThermo(int x,int y,int thermWidth,int thermDot )
  {
  int          xx;
  int           i;
  /*
   * Modification By Barry Mead to allow the Thermometer to have vastly 
   * larger ranges. (the thermWidth parameter can now have a value as
   * large as 200.      Modified 1-9-2000  Originally I used it to make
   * the sensitivity range for the mouse better. It could however also
   * be used to improve the dynamic range of music and sound affect 
   * volume controls for example.
   */
  int horizScaler; //Used to allow more thermo range for mouse sensitivity. 
  thermWidth = (thermWidth > 200) ? 200 : thermWidth; //Clamp to 200 max
  horizScaler = (thermWidth > 23) ? (200 / thermWidth) : 8; //Dynamic range
  xx = x;
  V_DrawNamePatch(xx, y, 0, "M_THERML", CR_DEFAULT, VPT_STRETCH);
  xx += 8;
  for (i=0;i<thermWidth;i++)
    {
    V_DrawNamePatch(xx, y, 0, "M_THERMM", CR_DEFAULT, VPT_STRETCH);
    xx += horizScaler;
    }

  xx += (8 - horizScaler); 	/* make the right end look even */

  V_DrawNamePatch(xx, y, 0, "M_THERMR", CR_DEFAULT, VPT_STRETCH);
  V_DrawNamePatch((x+8)+thermDot*horizScaler,y,0,"M_THERMO",CR_DEFAULT,VPT_STRETCH);
  }

//
// Draw an empty cell in the thermometer
//

void M_DrawEmptyCell (menu_t* menu,int item)
  {
  // CPhipps - patch drawing updated
  V_DrawNamePatch(menu->x - 10, menu->y+item*LINEHEIGHT - 1, 0,
		  "M_CELL1", CR_DEFAULT, VPT_STRETCH);
  }

//
// Draw a full cell in the thermometer
//

void M_DrawSelCell (menu_t* menu,int item)
  {
  // CPhipps - patch drawing updated
  V_DrawNamePatch(menu->x - 10, menu->y+item*LINEHEIGHT - 1, 0,
		  "M_CELL2", CR_DEFAULT, VPT_STRETCH);
  }

/////////////////////////////
//
// String-drawing Routines
//

//
// Find string width from hu_font chars
//
// CPhipps - const string
int M_StringWidth(const char* string)
{
  int i;
  int w = 0;
  int c;
  
  for (i = 0;(size_t)i < strlen(string);i++) {
    c = toupper(string[i]) - HU_FONTSTART;
    if (c < 0 || c >= HU_FONTSIZE)
      w += 4;
    else
      w += SHORT (hu_font[c].width);
  }
  
  return w;
}

//
//    Find string height from hu_font chars
//
int M_StringHeight(const char* string)
{
  int i;
  int h;
  int height = SHORT(hu_font[0].height);
  
  h = height;
  for (i = 0;string[i];i++)            // killough 1/31/98
    //  for (i = 0;i < strlen(string);i++)     // slow old code
    if (string[i] == '\n')
      h += height;
  
  return h;
}

//
//    Write a string using the hu_font
//
// CPhipps - const on string parameter, formatting
void M_WriteText (int x, int y, const char* string)
{
  int   w;
  const char* ch = string;
  int   c;
  int   cx = x;
  int   cy = y;
  
  while(1) {
    c = *ch++;
    if (!c)
      break;
    if (c == '\n') {
      cx = x;
      cy += 12;
      continue;
    }
    
    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c>= HU_FONTSIZE) {
      cx += 4;
      continue;
    }
    
    w = SHORT (hu_font[c].width);
    if (cx+w > SCREENWIDTH)
      break;
    // proff/nicolas 09/20/98 -- changed for hi-res
    // CPhipps - patch drawing updated
    V_DrawNumPatch(cx, cy, 0, hu_font[c].lumpnum, CR_DEFAULT, VPT_STRETCH);
    cx+=w;
  }
}

/////////////////////////////
//
// Initialization Routines to take care of one-time setup
//

// phares 4/08/98:
// M_InitHelpScreen() clears the weapons from the HELP
// screen that don't exist in this version of the game.

static void M_InitHelpScreen() // CPhipps - static
{
  setup_menu_t* src;

  src = helpstrings;
  while (!(src->m_flags & S_END)) {

    if ((strncmp(src->m_text,"PLASMA",6) == 0) && (gamemode == shareware))
      src->m_flags = S_SKIP; // Don't show setting or item
    if ((strncmp(src->m_text,"BFG",3) == 0) && (gamemode == shareware))
      src->m_flags = S_SKIP; // Don't show setting or item
    if ((strncmp(src->m_text,"SSG",3) == 0) && (gamemode != commercial))
      src->m_flags = S_SKIP; // Don't show setting or item
    src++;
  }
}

//
// M_Init
//
void M_Init (void)
  {
  currentMenu = &MainDef;
  menuactive = 0;
  itemOn = currentMenu->lastOn;
  whichSkull = 0;
  skullAnimCounter = 10;
  screenSize = screenblocks - 3;
  messageToPrint = 0;
  messageString = NULL;
  messageLastMenuActive = menuactive;
  quickSaveSlot = -1;

  // Here we could catch other version dependencies,
  //  like HELP1/2, and four episodes.

  switch(gamemode)
      {
    case commercial:
      // This is used because DOOM 2 had only one HELP
      //  page. I use CREDIT as second page now, but
      //  kept this hack for educational purposes.
      MainMenu[readthis] = MainMenu[quitdoom];
      MainDef.numitems--;
      MainDef.y += 8;
      NewDef.prevMenu = &MainDef;
      ReadDef1.routine = M_DrawReadThis1;
      ReadDef1.x = 330;
      ReadDef1.y = 165;
      ReadMenu1[0].routine = M_FinishReadThis;
      break;
    case registered:
      // Episode 2 and 3 are handled,
      //  branching to an ad screen.

      // killough 2/21/98: Fix registered Doom help screen
      ReadDef1.x = 280;
      ReadDef1.y = 185;

    case shareware:
      // We need to remove the fourth episode.
      EpiDef.numitems--;
      break;
    case retail:
    // We are fine.
    default:
      break;
      }

  // killough 4/17/98:
  // Doom traditional menu, for arch-conservatives like yours truly

  if (traditional_menu)
    {
    menuitem_t t       = MainMenu[loadgame];
    MainMenu[loadgame] = MainMenu[options];
    MainMenu[options]  = MainMenu[savegame];
    MainMenu[savegame] = t;
    }

  M_InitHelpScreen();   // init the help screen             // phares 4/08/98
  M_InitExtendedHelp(); // init extended help screens       // phares 3/30/98
  }

//
// End of General Routines
//
/////////////////////////////////////////////////////////////////////////////
