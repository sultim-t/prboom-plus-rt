//-----------------------------------------------------------------------------
//
// Copyright 2017 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "umapinfo.h"
#include "scanner.h"

extern "C"
{
#include "m_misc.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

void M_AddEpisode(const char *map, char *def);

MapList Maps;
}


//==========================================================================
//
// The Doom actors in their original order
// Names are the same as in ZDoom.
//
//==========================================================================

static const char * const ActorNames[] =
{
	"DoomPlayer",
	"ZombieMan",
	"ShotgunGuy",
	"Archvile",
	"ArchvileFire",
	"Revenant",
	"RevenantTracer",
	"RevenantTracerSmoke",
	"Fatso",
	"FatShot",
	"ChaingunGuy",
	"DoomImp",
	"Demon",
	"Spectre",
	"Cacodemon",
	"BaronOfHell",
	"BaronBall",
	"HellKnight",
	"LostSoul",
	"SpiderMastermind",
	"Arachnotron",
	"Cyberdemon",
	"PainElemental",
	"WolfensteinSS",
	"CommanderKeen",
	"BossBrain",
	"BossEye",
	"BossTarget",
	"SpawnShot",
	"SpawnFire",
	"ExplosiveBarrel",
	"DoomImpBall",
	"CacodemonBall",
	"Rocket",
	"PlasmaBall",
	"BFGBall",
	"ArachnotronPlasma",
	"BulletPuff",
	"Blood",
	"TeleportFog",
	"ItemFog",
	"TeleportDest",
	"BFGExtra",
	"GreenArmor",
	"BlueArmor",
	"HealthBonus",
	"ArmorBonus",
	"BlueCard",
	"RedCard",
	"YellowCard",
	"YellowSkull",
	"RedSkull",
	"BlueSkull",
	"Stimpack",
	"Medikit",
	"Soulsphere",
	"InvulnerabilitySphere",
	"Berserk",
	"BlurSphere",
	"RadSuit",
	"Allmap",
	"Infrared",
	"Megasphere",
	"Clip",
	"ClipBox",
	"RocketAmmo",
	"RocketBox",
	"Cell",
	"CellPack",
	"Shell",
	"ShellBox",
	"Backpack",
	"BFG9000",
	"Chaingun",
	"Chainsaw",
	"RocketLauncher",
	"PlasmaRifle",
	"Shotgun",
	"SuperShotgun",
	"TechLamp",
	"TechLamp2",
	"Column",
	"TallGreenColumn",
	"ShortGreenColumn",
	"TallRedColumn",
	"ShortRedColumn",
	"SkullColumn",
	"HeartColumn",
	"EvilEye",
	"FloatingSkull",
	"TorchTree",
	"BlueTorch",
	"GreenTorch",
	"RedTorch",
	"ShortBlueTorch",
	"ShortGreenTorch",
	"ShortRedTorch",
	"Stalagtite",
	"TechPillar",
	"CandleStick",
	"Candelabra",
	"BloodyTwitch",
	"Meat2",
	"Meat3",
	"Meat4",
	"Meat5",
	"NonsolidMeat2",
	"NonsolidMeat4",
	"NonsolidMeat3",
	"NonsolidMeat5",
	"NonsolidTwitch",
	"DeadCacodemon",
	"DeadMarine",
	"DeadZombieMan",
	"DeadDemon",
	"DeadLostSoul",
	"DeadDoomImp",
	"DeadShotgunGuy",
	"GibbedMarine",
	"GibbedMarineExtra",
	"HeadsOnAStick",
	"Gibs",
	"HeadOnAStick",
	"HeadCandles",
	"DeadStick",
	"LiveStick",
	"BigTree",
	"BurningBarrel",
	"HangNoGuts",
	"HangBNoBrain",
	"HangTLookingDown",
	"HangTSkull",
	"HangTLookingUp",
	"HangTNoBrain",
	"ColonGibs",
	"SmallBloodPool",
	"BrainStem",
	//Boom/MBF additions
	"PointPusher",
	"PointPuller",
	"MBFHelperDog",
	"PlasmaBall1",
	"PlasmaBall2",
	"EvilSceptre",
	"UnholyBible",
	"MusicChanger",
	"Deh_Actor_145",
	"Deh_Actor_146",
	"Deh_Actor_147",
	"Deh_Actor_148",
	"Deh_Actor_149",
	// DEHEXTRA Actors start here
	"Deh_Actor_150", // Extra thing 0
	"Deh_Actor_151", // Extra thing 1
	"Deh_Actor_152", // Extra thing 2
	"Deh_Actor_153", // Extra thing 3
	"Deh_Actor_154", // Extra thing 4
	"Deh_Actor_155", // Extra thing 5
	"Deh_Actor_156", // Extra thing 6
	"Deh_Actor_157", // Extra thing 7
	"Deh_Actor_158", // Extra thing 8
	"Deh_Actor_159", // Extra thing 9
	"Deh_Actor_160", // Extra thing 10
	"Deh_Actor_161", // Extra thing 11
	"Deh_Actor_162", // Extra thing 12
	"Deh_Actor_163", // Extra thing 13
	"Deh_Actor_164", // Extra thing 14
	"Deh_Actor_165", // Extra thing 15
	"Deh_Actor_166", // Extra thing 16
	"Deh_Actor_167", // Extra thing 17
	"Deh_Actor_168", // Extra thing 18
	"Deh_Actor_169", // Extra thing 19
	"Deh_Actor_170", // Extra thing 20
	"Deh_Actor_171", // Extra thing 21
	"Deh_Actor_172", // Extra thing 22
	"Deh_Actor_173", // Extra thing 23
	"Deh_Actor_174", // Extra thing 24
	"Deh_Actor_175", // Extra thing 25
	"Deh_Actor_176", // Extra thing 26
	"Deh_Actor_177", // Extra thing 27
	"Deh_Actor_178", // Extra thing 28
	"Deh_Actor_179", // Extra thing 29
	"Deh_Actor_180", // Extra thing 30
	"Deh_Actor_181", // Extra thing 31
	"Deh_Actor_182", // Extra thing 32
	"Deh_Actor_183", // Extra thing 33
	"Deh_Actor_184", // Extra thing 34
	"Deh_Actor_185", // Extra thing 35
	"Deh_Actor_186", // Extra thing 36
	"Deh_Actor_187", // Extra thing 37
	"Deh_Actor_188", // Extra thing 38
	"Deh_Actor_189", // Extra thing 39
	"Deh_Actor_190", // Extra thing 40
	"Deh_Actor_191", // Extra thing 41
	"Deh_Actor_192", // Extra thing 42
	"Deh_Actor_193", // Extra thing 43
	"Deh_Actor_194", // Extra thing 44
	"Deh_Actor_195", // Extra thing 45
	"Deh_Actor_196", // Extra thing 46
	"Deh_Actor_197", // Extra thing 47
	"Deh_Actor_198", // Extra thing 48
	"Deh_Actor_199", // Extra thing 49
	"Deh_Actor_200", // Extra thing 50
	"Deh_Actor_201", // Extra thing 51
	"Deh_Actor_202", // Extra thing 52
	"Deh_Actor_203", // Extra thing 53
	"Deh_Actor_204", // Extra thing 54
	"Deh_Actor_205", // Extra thing 55
	"Deh_Actor_206", // Extra thing 56
	"Deh_Actor_207", // Extra thing 57
	"Deh_Actor_208", // Extra thing 58
	"Deh_Actor_209", // Extra thing 59
	"Deh_Actor_210", // Extra thing 60
	"Deh_Actor_211", // Extra thing 61
	"Deh_Actor_212", // Extra thing 62
	"Deh_Actor_213", // Extra thing 63
	"Deh_Actor_214", // Extra thing 64
	"Deh_Actor_215", // Extra thing 65
	"Deh_Actor_216", // Extra thing 66
	"Deh_Actor_217", // Extra thing 67
	"Deh_Actor_218", // Extra thing 68
	"Deh_Actor_219", // Extra thing 69
	"Deh_Actor_220", // Extra thing 70
	"Deh_Actor_221", // Extra thing 71
	"Deh_Actor_222", // Extra thing 72
	"Deh_Actor_223", // Extra thing 73
	"Deh_Actor_224", // Extra thing 74
	"Deh_Actor_225", // Extra thing 75
	"Deh_Actor_226", // Extra thing 76
	"Deh_Actor_227", // Extra thing 77
	"Deh_Actor_228", // Extra thing 78
	"Deh_Actor_229", // Extra thing 79
	"Deh_Actor_230", // Extra thing 80
	"Deh_Actor_231", // Extra thing 81
	"Deh_Actor_232", // Extra thing 82
	"Deh_Actor_233", // Extra thing 83
	"Deh_Actor_234", // Extra thing 84
	"Deh_Actor_235", // Extra thing 85
	"Deh_Actor_236", // Extra thing 86
	"Deh_Actor_237", // Extra thing 87
	"Deh_Actor_238", // Extra thing 88
	"Deh_Actor_239", // Extra thing 89
	"Deh_Actor_240", // Extra thing 90
	"Deh_Actor_241", // Extra thing 91
	"Deh_Actor_242", // Extra thing 92
	"Deh_Actor_243", // Extra thing 93
	"Deh_Actor_244", // Extra thing 94
	"Deh_Actor_245", // Extra thing 95
	"Deh_Actor_246", // Extra thing 96
	"Deh_Actor_247", // Extra thing 97
	"Deh_Actor_248", // Extra thing 98
	"Deh_Actor_249", // Extra thing 99
	NULL
};


// -----------------------------------------------
//
//
// -----------------------------------------------

static void FreeMap(MapEntry *mape)
{
	if (mape->mapname) free(mape->mapname);
	if (mape->levelname) free(mape->levelname);
	if (mape->intertext) free(mape->intertext);
	if (mape->intertextsecret) free(mape->intertextsecret);
	if (mape->properties) free(mape->properties);
	if (mape->bossactions) free(mape->bossactions);
	mape->propertycount = 0;
	mape->mapname = NULL;
	mape->properties = NULL;
}


void FreeMapList()
{
	unsigned i;
	
	for(i = 0; i < Maps.mapcount; i++)
	{
		FreeMap(&Maps.maps[i]);
	}
	free(Maps.maps);
	Maps.maps = NULL;
	Maps.mapcount = 0;
}


void ReplaceString(char **pptr, const char *newstring)
{
	if (*pptr != NULL) free(*pptr);
	*pptr = strdup(newstring);
}

// -----------------------------------------------
//
// Parses a set of string and concatenates them
//
// -----------------------------------------------

static char *ParseMultiString(Scanner &scanner, int error)
{
	char *build = NULL;
	
	if (scanner.CheckToken(TK_Identifier))
	{
		if (!stricmp(scanner.string, "clear"))
		{
			return strdup("-");	// this was explicitly deleted to override the default.
		}
		else
		{
			scanner.ErrorF("Either 'clear' or string constant expected");
		}
	}
	
	do
	{
		scanner.MustGetToken(TK_StringConst);
		if (build == NULL) build = strdup(scanner.string);
		else
		{
			size_t newlen = strlen(build) + strlen(scanner.string) + 2; // strlen for both the existing text and the new line, plus room for one \n and one \0
			build = (char*)realloc(build, newlen); // Prepare the destination memory for the below strcats
			strcat(build, "\n"); // Replace the existing text's \0 terminator with a \n
			strcat(build, scanner.string); // Concatenate the new line onto the existing text
		}
	} while (scanner.CheckToken(','));
	return build;
}

// -----------------------------------------------
//
// Parses a lump name. The buffer must be at least 9 characters.
//
// -----------------------------------------------

static int ParseLumpName(Scanner &scanner, char *buffer)
{
	scanner.MustGetToken(TK_StringConst);
	if (strlen(scanner.string) > 8)
	{
		scanner.ErrorF("String too long. Maximum size is 8 characters.");
		return 0;
	}
	strncpy(buffer, scanner.string, 8);
	buffer[8] = 0;
	M_Strupr(buffer);
	return 1;
}

// -----------------------------------------------
//
// Parses a standard property that is already known
// These do not get stored in the property list
// but in dedicated struct member variables.
//
// -----------------------------------------------

static int ParseStandardProperty(Scanner &scanner, MapEntry *mape)
{
	// find the next line with content.
	// this line is no property.
	
	scanner.MustGetToken(TK_Identifier);
	char *pname = strdup(scanner.string);
	scanner.MustGetToken('=');

	if (!stricmp(pname, "levelname"))
	{
		scanner.MustGetToken(TK_StringConst);
		ReplaceString(&mape->levelname, scanner.string);
	}
	else if (!stricmp(pname, "next"))
	{
		ParseLumpName(scanner, mape->nextmap);
		if (!G_ValidateMapName(mape->nextmap, NULL, NULL))
		{
			scanner.ErrorF("Invalid map name %s.", mape->nextmap);
			return 0;
		}
	}
	else if (!stricmp(pname, "nextsecret"))
	{
		ParseLumpName(scanner, mape->nextsecret);
		if (!G_ValidateMapName(mape->nextsecret, NULL, NULL))
		{
			scanner.ErrorF("Invalid map name %s", mape->nextmap);
			return 0;
		}
	}
	else if (!stricmp(pname, "levelpic"))
	{
		ParseLumpName(scanner, mape->levelpic);
	}
	else if (!stricmp(pname, "skytexture"))
	{
		ParseLumpName(scanner, mape->skytexture);
	}
	else if (!stricmp(pname, "music"))
	{
		ParseLumpName(scanner, mape->music);
	}
	else if (!stricmp(pname, "endpic"))
	{
		ParseLumpName(scanner, mape->endpic);
	}
	else if (!stricmp(pname, "endcast"))
	{
		scanner.MustGetToken(TK_BoolConst);
		if (scanner.boolean) strcpy(mape->endpic, "$CAST");
		else strcpy(mape->endpic, "-");
	}
	else if (!stricmp(pname, "endbunny"))
	{
		scanner.MustGetToken(TK_BoolConst);
		if (scanner.boolean) strcpy(mape->endpic, "$BUNNY");
		else strcpy(mape->endpic, "-");
	}
	else if (!stricmp(pname, "endgame"))
	{
		scanner.MustGetToken(TK_BoolConst);
		if (scanner.boolean) strcpy(mape->endpic, "!");
		else strcpy(mape->endpic, "-");
	}
	else if (!stricmp(pname, "exitpic"))
	{
		ParseLumpName(scanner, mape->exitpic);
	}
	else if (!stricmp(pname, "enterpic"))
	{
		ParseLumpName(scanner, mape->enterpic);
	}
	else if (!stricmp(pname, "nointermission"))
	{
		scanner.MustGetToken(TK_BoolConst);
		mape->nointermission = scanner.boolean;
	}
	else if (!stricmp(pname, "partime"))
	{
		scanner.MustGetInteger();
		mape->partime = TICRATE * scanner.number;
	}
	else if (!stricmp(pname, "intertext"))
	{
		char *lname = ParseMultiString(scanner, 1);
		if (!lname) return 0;
		if (mape->intertext != NULL) free(mape->intertext);
		mape->intertext = lname;
	}
	else if (!stricmp(pname, "intertextsecret"))
	{
		char *lname = ParseMultiString(scanner, 1);
		if (!lname) return 0;
		if (mape->intertextsecret != NULL) free(mape->intertextsecret);
		mape->intertextsecret = lname;
	}
	else if (!stricmp(pname, "interbackdrop"))
	{
		ParseLumpName(scanner, mape->interbackdrop);
	}
	else if (!stricmp(pname, "intermusic"))
	{
		ParseLumpName(scanner, mape->intermusic);
	}
	else if (!stricmp(pname, "episode"))
	{
		char *lname = ParseMultiString(scanner, 1);
		if (!lname) return 0;
		M_AddEpisode(mape->mapname, lname);
	}
	else if (!stricmp(pname, "bossaction"))
	{
		scanner.MustGetToken(TK_Identifier);
		int classnum, special, tag;
		if (!stricmp(scanner.string, "clear"))
		{
			// mark level free of boss actions
			classnum = special = tag = -1;
			if (mape->bossactions) free(mape->bossactions);
			mape->bossactions = NULL;
			mape->numbossactions = -1;
		}
		else
		{
			int i;
			for (i = 0; ActorNames[i]; i++)
			{
				if (!stricmp(scanner.string, ActorNames[i])) break;
			}
			if (ActorNames[i] == NULL)
			{
				scanner.ErrorF("Unknown thing type %s", scanner.string);
				return 0;
			}

			scanner.MustGetToken(',');
			scanner.MustGetInteger();
			special = scanner.number;
			scanner.MustGetToken(',');
			scanner.MustGetInteger();
			tag = scanner.number;
			// allow no 0-tag specials here, unless a level exit.
			if (tag != 0 || special == 11 || special == 51 || special == 52 || special == 124)
			{
				if (mape->numbossactions == -1) mape->numbossactions = 1;
				else mape->numbossactions++;
				mape->bossactions = (struct BossAction *)realloc(mape->bossactions, sizeof(struct BossAction) * mape->numbossactions);
				mape->bossactions[mape->numbossactions - 1].type = i;
				mape->bossactions[mape->numbossactions - 1].special = special;
				mape->bossactions[mape->numbossactions - 1].tag = tag;
			}

		}
	}
	else
	{
		do
		{
			if (!scanner.CheckFloat()) scanner.GetNextToken();
			if (scanner.token > TK_BoolConst) 
			{
				scanner.Error(TK_Identifier);
			}
			
		} while (scanner.CheckToken(','));
	}
	free(pname);
	return 1;
}

// -----------------------------------------------
//
// Parses a complete map entry
//
// -----------------------------------------------

static int ParseMapEntry(Scanner &scanner, MapEntry *val)
{
	val->mapname = NULL;
	val->propertycount = 0;
	val->properties = NULL;

	scanner.MustGetIdentifier("map");
	scanner.MustGetToken(TK_Identifier);
	if (!G_ValidateMapName(scanner.string, NULL, NULL))
	{
		scanner.ErrorF("Invalid map name %s", scanner.string);
		return 0;
	}

	ReplaceString(&val->mapname, scanner.string);
	scanner.MustGetToken('{');
	while(!scanner.CheckToken('}'))
	{
		ParseStandardProperty(scanner, val);
	}
	return 1;
}

// -----------------------------------------------
//
// Parses a complete UMAPINFO lump
//
// -----------------------------------------------

int ParseUMapInfo(const unsigned char *buffer, size_t length, umapinfo_errorfunc err)
{
	Scanner scanner((const char*)buffer, length);
	unsigned int i;

	scanner.SetErrorCallback(err);


	while (scanner.TokensLeft())
	{
		MapEntry parsed = { 0 };
		ParseMapEntry(scanner, &parsed);

		// Set default level progression here to simplify the checks elsewhere. Doing this lets us skip all normal code for this if nothing has been defined.
		if (parsed.endpic[0] && (strcmp(parsed.endpic, "-") != 0))
		{
			parsed.nextmap[0] = parsed.nextsecret[0] = 0;
		}
		else if (!parsed.nextmap[0] && !parsed.endpic[0])
		{
			if (!stricmp(parsed.mapname, "MAP30")) strcpy(parsed.endpic, "$CAST");
			else if (!stricmp(parsed.mapname, "E1M8"))  strcpy(parsed.endpic, gamemode == retail? "CREDIT" : "HELP2");
			else if (!stricmp(parsed.mapname, "E2M8"))  strcpy(parsed.endpic, "VICTORY");
			else if (!stricmp(parsed.mapname, "E3M8"))  strcpy(parsed.endpic, "$BUNNY");
			else if (!stricmp(parsed.mapname, "E4M8"))  strcpy(parsed.endpic, "ENDPIC");
			else if (gamemission == chex && !stricmp(parsed.mapname, "E1M5"))  strcpy(parsed.endpic, "CREDIT");
			else
			{
				int ep, map;
				G_ValidateMapName(parsed.mapname, &ep, &map);
				map++;
				if (gamemode == commercial)
					sprintf(parsed.nextmap, "MAP%02d", map);
				else
					sprintf(parsed.nextmap, "E%dM%d", ep, map);
			}
		}

		// Does this property already exist? If yes, replace it.
		for(i = 0; i < Maps.mapcount; i++)
		{
			if (!strcmp(parsed.mapname, Maps.maps[i].mapname))
			{
				FreeMap(&Maps.maps[i]);
				Maps.maps[i] = parsed;
				break;
			}
		}
		// Not found so create a new one.
		if (i == Maps.mapcount)
		{
			Maps.mapcount++;
			Maps.maps = (MapEntry*)realloc(Maps.maps, sizeof(MapEntry)*Maps.mapcount);
			Maps.maps[Maps.mapcount-1] = parsed;
		}
		
	}
	return 1;
}


MapProperty *FindProperty(MapEntry *map, const char *name)
{
	return NULL;
}
