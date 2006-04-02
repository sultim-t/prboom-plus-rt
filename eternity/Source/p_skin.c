//--------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
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
//                Copyright(C) 1999 Simon Howard 'Fraggle'
//
// Skins (Doom Legacy)
//
// Skins are a set of sprites which replace the normal player sprites, so
// in multiplayer the players can look like whatever they want.
//
// FIXME: rewrite needed for Heretic-specific stuff
//
//--------------------------------------------------------------------------

#include "z_zone.h"
#include "c_runcmd.h"
#include "c_io.h"
#include "c_net.h"
#include "d_player.h"
#include "doomstat.h"
#include "d_main.h"
#include "info.h"
#include "p_info.h"
#include "p_skin.h"
#include "r_things.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "d_io.h" // SoM 3/13/02: Get rid of strncasecmp warnings in VC++
#include "e_edf.h"
#include "e_sound.h"

skin_t marine = 
{
   SKIN_PLAYER, // haleyjd 09/26/04: skin type
   "PLAY",
   "marine",
   0,           // haleyjd 05/11/03: player sprite number now set by EDF
   {
      NULL
   }, 
   "STF", 
   default_faces
};

int numskins = 0;      // haleyjd 03/22/03
int numskinsalloc = 0; // haleyjd 03/22/03
skin_t **skins = NULL;

static skin_t **monster_skins = NULL; // haleyjd 09/26/04

char **spritelist = NULL;
char *default_skin = NULL;     // name of currently selected skin

char *skinsoundnames[NUMSKINSOUNDS] =
{
   "dsplpain",
   "dspdiehi",
   "dsoof",
   "dsslop",
   "dspunch",
   "dsradio",
   "dspldeth",
   "dsplfall",
   "dsplfeet",
   "dsfallht",
};

// forward prototypes
static void P_AddSkin(skin_t *newskin);
static void P_CreateMarine(void);
static void P_CacheFaces(skin_t *skin);
static void P_InitMonsterSkins(void);

//
// P_InitSkins
//
// Allocates the combined sprite list and creates the default
// skin for the current game mode
//
// haleyjd 03/22/03: significant rewriting, safety increased
// haleyjd 09/26/04: initialize the monster skins list here
//
void P_InitSkins(void)
{
   int i;
   char **currentsprite;

   // haleyjd 09/26/04: initialize monster skins list
   P_InitMonsterSkins();

   // create default gamemode skin -- TODO: heretic support
   P_CreateMarine();

   // allocate spritelist
   if(spritelist)
      Z_Free(spritelist);
   
   // haleyjd 05/12/03: don't count the marine skin
   spritelist = Z_Malloc(((numskins-1)+NUMSPRITES+1)*sizeof(char *),
                         PU_STATIC, 0);

   // add the normal sprites
   currentsprite = spritelist;
   for(i = 0; i < NUMSPRITES + 1; i++)
   {
      if(!sprnames[i])
         break;
      *currentsprite = sprnames[i];
      currentsprite++;
   }

   // add skin sprites
   for(i = 0; i < numskins; i++)
   {
      if(skins[i] == &marine) // do not add the marine sprite again
         continue;
      *currentsprite   = skins[i]->spritename;
      skins[i]->sprite = currentsprite - spritelist;
      P_CacheFaces(skins[i]);
      currentsprite++;
   }

   *currentsprite = NULL;     // end in null
}

//
// GetDefSound
//
// This function gets the EDF sound mnemonic for a given sound DeHackEd
// number. This keeps skins more compatible than they were previously.
//
static void GetDefSound(char **var, int dehnum)
{
   sfxinfo_t *sfx = E_SoundForDEHNum(dehnum);

   *var = sfx ? sfx->mnemonic : "none";
}

//
// P_CreateMarine
//
// Initialize the default DOOM marine skin
//
// haleyjd 03/22/03: partially rewritten
//
static void P_CreateMarine(void)
{
   static boolean marine_created = false;
   
   if(marine_created) 
      return;      // dont make twice

   marine.sprite = playerSpriteNum;

   // initialize sound names
   GetDefSound(&(marine.sounds[sk_plpain]), sfx_plpain);
   GetDefSound(&(marine.sounds[sk_pdiehi]), sfx_pdiehi);
   GetDefSound(&(marine.sounds[sk_oof]),    sfx_oof);
   GetDefSound(&(marine.sounds[sk_slop]),   sfx_slop);
   GetDefSound(&(marine.sounds[sk_punch]),  sfx_punch);
   GetDefSound(&(marine.sounds[sk_radio]),  sfx_radio);
   GetDefSound(&(marine.sounds[sk_pldeth]), sfx_pldeth);
   GetDefSound(&(marine.sounds[sk_plfall]), sfx_plfall);
   GetDefSound(&(marine.sounds[sk_plfeet]), sfx_plfeet);
   GetDefSound(&(marine.sounds[sk_fallht]), sfx_fallht);
   
   if(default_skin == NULL) 
      default_skin = Z_Strdup("marine", PU_STATIC, 0);
   
   P_AddSkin(&marine);
   
   marine_created = true;
}

//
// P_AddSkin
//
// Add a new skin to the skins list
//
// haleyjd 03/22/03: rewritten
//
static void P_AddSkin(skin_t *newskin)
{
   if(numskins >= numskinsalloc)
   {
      numskinsalloc = numskinsalloc ? numskinsalloc*2 : 32;

      skins = realloc(skins, numskinsalloc*sizeof(skin_t *));
   }
   
   skins[numskins] = newskin;
   numskins++;
}

static void P_AddSpriteLumps(const char *named)
{
   int i, n = strlen(named);
   
   for(i = 0; i < numlumps; i++)
   {
      if(!strncasecmp(lumpinfo[i]->name, named, n))
      {
         // mark as sprites so that W_CoalesceMarkedResource
         // will group them as sprites
         lumpinfo[i]->li_namespace = ns_sprites;
      }
   }
}

static skin_t *newskin;

static void P_ParseSkinCmd(char *line)
{
   int i;
   
   while(*line==' ') 
      line++;
   if(!*line) 
      return;      // maybe nothing left now
   
   if(!strncasecmp(line, "name", 4))
   {
      char *skinname = line+4;
      while(*skinname == ' ') 
         skinname++;
      newskin->skinname = strdup(skinname);
   }
   if(!strncasecmp(line, "sprite", 6))
   {
      char *spritename = line+6;
      while(*spritename == ' ') spritename++;
      strncpy(newskin->spritename, spritename, 4);
      newskin->spritename[4] = 0;
   }
   if(!strncasecmp(line, "face", 4))
   {
      char *facename = line+4;
      while(*facename == ' ') facename++;
      newskin->facename = strdup(facename);
      newskin->facename[3] = 0;
   }

   // is it a sound?
   
   for(i = 0; i < NUMSKINSOUNDS; i++)
   {
      if(!strncasecmp(line, skinsoundnames[i], 
                      strlen(skinsoundnames[i])))
      {                    // yes!
         char *newsoundname = line + strlen(skinsoundnames[i]);
         while(*newsoundname == ' ')
            newsoundname++;
         
         newsoundname += 2;        // ds
         
         newskin->sounds[i] = strdup(newsoundname);
      }
   }
}

void P_ParseSkin(int lumpnum)
{
   char *lump;
   char *rover;
   char inputline[256];
   boolean comment;

   memset(inputline, 0, 256);
      
   newskin = Z_Malloc(sizeof(skin_t), PU_STATIC, 0);
   newskin->spritename = Z_Malloc(5, PU_STATIC, 0);
   strncpy(newskin->spritename, lumpinfo[lumpnum+1]->name, 4);
   newskin->spritename[4] = 0;
   newskin->facename = "STF";      // default status bar face
   newskin->faces = 0;

   newskin->type = SKIN_PLAYER; // haleyjd: it's a player skin

   // set sounds to defaults
   GetDefSound(&(newskin->sounds[sk_plpain]), sfx_plpain);
   GetDefSound(&(newskin->sounds[sk_pdiehi]), sfx_pdiehi);
   GetDefSound(&(newskin->sounds[sk_oof]),    sfx_oof);
   GetDefSound(&(newskin->sounds[sk_slop]),   sfx_slop);
   GetDefSound(&(newskin->sounds[sk_punch]),  sfx_punch);
   GetDefSound(&(newskin->sounds[sk_radio]),  sfx_radio);
   GetDefSound(&(newskin->sounds[sk_pldeth]), sfx_pldeth);
   GetDefSound(&(newskin->sounds[sk_plfall]), sfx_plfall);
   GetDefSound(&(newskin->sounds[sk_plfeet]), sfx_plfeet);
   GetDefSound(&(newskin->sounds[sk_fallht]), sfx_fallht);

   lump = W_CacheLumpNum(lumpnum, PU_STATIC);  // get the lump
   
   rover = lump; 
   comment = false;

   while(rover < lump+lumpinfo[lumpnum]->size)
   {
      if((*rover=='/' && *(rover+1)=='/') ||        // '//'
         (*rover==';') || (*rover=='#') )           // ';', '#'
         comment = true;
      if(*rover>31 && !comment)
      {
         psnprintf(inputline, sizeof(inputline), "%s%c", 
                   inputline, (*rover == '=') ? ' ' : *rover);
      }
      if(*rover=='\n') // end of line
      {
         P_ParseSkinCmd(inputline);    // parse the line
         memset(inputline, 0, 256);
         comment = false;
      }
      rover++;
   }
   P_ParseSkinCmd(inputline);    // parse the last line
   
   Z_ChangeTag(lump, PU_CACHE); // mark lump purgable
   
   P_AddSkin(newskin);
   P_AddSpriteLumps(newskin->spritename);
}

static void P_CacheFaces(skin_t *skin)
{
   if(skin->faces) 
      return; // already cached
   
   if(!strcasecmp(skin->facename,"STF"))
   {
      skin->faces = default_faces;
   }
   else
   {
      skin->faces =
         Z_Malloc(ST_NUMFACES*sizeof(patch_t*),PU_STATIC,0);
      ST_CacheFaces(skin->faces, skin->facename);
   }
}

// this could be done with a hash table for speed.
// i cant be bothered tho, its not something likely to be
// being done constantly, only now and again

static skin_t *P_SkinForName(char *s)
{
   int i;

   while(*s==' ')
      s++;
   
   if(!skins)
      return NULL;

   for(i = 0; i < numskins; i++)
   {
      if(!strcasecmp(s, skins[i]->skinname))
         return skins[i];
   }

   return NULL;
}

void P_SetSkin(skin_t *skin, int playernum)
{
   if(!playeringame[playernum])
      return;
   
   players[playernum].skin = skin;
   if(gamestate == GS_LEVEL)
   {
      players[playernum].mo->skin = skin;
      players[playernum].mo->sprite = skin->sprite;
   }
   
   if(playernum == consoleplayer) 
      default_skin = skin->skinname;
}

        // change to previous skin
static skin_t *P_PrevSkin(int player)
{
   int skinnum;
      
   // find the skin in the list first
   for(skinnum = 0; skinnum < numskins; skinnum++)
   {
      if(players[player].skin == skins[skinnum])
         break;
   }
         
   if(skinnum == numskins)
      return NULL;         // not found (?)

   --skinnum;      // previous skin
   
   if(skinnum < 0) 
      skinnum = numskins-1;   // loop around
   
   return skins[skinnum];
}

        // change to next skin
static skin_t *P_NextSkin(int player)
{
   int skinnum;
      
   // find the skin in the list first

   for(skinnum = 0; skinnum < numskins; skinnum++)
   {
      if(players[player].skin == skins[skinnum]) 
         break;
   }

   if(skinnum == numskins)
      return NULL;         // not found (?)
   
   ++skinnum;      // next skin
   
   if(skinnum >= numskins) skinnum = 0;    // loop around
   
   return skins[skinnum];
}

//
// P_InitMonsterSkins
//
// haleyjd 09/26/04:
// Allocates the skin_t pointer array for monster skins with size
// NUMSPRITES (only one monster skin is needed at max for each sprite).
// Must be called after EDF and before first use of P_GetMonsterSkin.
//
static void P_InitMonsterSkins(void)
{
   if(!monster_skins)
   {
      monster_skins = Z_Malloc(NUMSPRITES * sizeof(skin_t *), PU_STATIC, 0);
      memset(monster_skins, 0, NUMSPRITES * sizeof(skin_t *));
   }
}

//
// P_GetMonsterSkin
//
// haleyjd 09/26/04:
// If a monster skin doesn't exist for the requested sprite, one will
// be created. Otherwise, the existing skin is returned.
//
skin_t *P_GetMonsterSkin(spritenum_t sprnum)
{
#ifdef RANGECHECK
   if(sprnum < 0 || sprnum >= NUMSPRITES)
      I_Error("P_GetMonsterSkin: sprite %d out of range\n", sprnum);
#endif

   if(!monster_skins[sprnum])
   {
      monster_skins[sprnum] = Z_Malloc(sizeof(skin_t), PU_STATIC, 0);
      memset(monster_skins[sprnum], 0, sizeof(skin_t));

      monster_skins[sprnum]->type = SKIN_MONSTER;
      monster_skins[sprnum]->sprite = sprnum;
   }

   return monster_skins[sprnum];
}

/**** console stuff ******/

CONSOLE_COMMAND(listskins, 0)
{
   int i;

   for(i = 0; i < numskins; i++)
   {      
      C_Printf("%s\n", skins[i]->skinname);
   }
}

//      helper macro to ensure grammatical correctness :)

#define isvowel(c)              \
        ( (c)=='a' || (c)=='e' || (c)=='i' || (c)=='o' || (c)=='u' )

VARIABLE_STRING(default_skin, NULL, 50);

        // player skin
CONSOLE_NETVAR(skin, default_skin, cf_handlerset, netcmd_skin)
{
   skin_t *skin;
   
   if(!c_argc)
   {
      if(consoleplayer == cmdsrc)
         C_Printf("%s is %s %s\n", players[cmdsrc].name,
                  isvowel(players[cmdsrc].skin->skinname[0]) ? "an" : "a",
                  players[cmdsrc].skin->skinname);
      return;
   }

   if(!strcmp(c_argv[0], "+"))
      skin = P_NextSkin(cmdsrc);
   else if(!strcmp(c_argv[0], "-"))
      skin = P_PrevSkin(cmdsrc);
   else if(!(skin = P_SkinForName(c_argv[0])))
   {
      if(consoleplayer == cmdsrc)
         C_Printf("skin not found: '%s'\n", c_argv[0]);
      return;
   }

   P_SetSkin(skin, cmdsrc);
   // wake up status bar for new face
   redrawsbar = true;
}

void P_Skin_AddCommands(void)
{
   C_AddCommand(skin);
   C_AddCommand(listskins);
}

// EOF
