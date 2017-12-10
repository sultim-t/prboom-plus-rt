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
#include "tarray.h"

extern "C"
{
#include "m_misc.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

void M_AddEpisode(const char *map, char *def);

MapList Maps;
}

extern TArray<const char *> ClassNames;

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
			size_t oldlen = strlen(build);
			size_t newlen = oldlen + strlen(scanner.string) + 2;

			build = (char*)realloc(build, newlen);
			build[oldlen] = '\n';
			strcpy(build + oldlen + 1, scanner.string);
			build[newlen] = 0;
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
			unsigned int i;
			for (i = 0; i < ClassNames.Size(); i++)
			{
				if (!stricmp(scanner.string, ClassNames[i])) break;
			}
			if (ClassNames[i] == NULL)
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
		if (parsed.endpic[0])
		{
			parsed.nextmap[0] = parsed.nextsecret[0] = 0;
			if (parsed.endpic[0] == '!') parsed.endpic[0] = 0;
		}
		if (!parsed.nextmap[0] && !parsed.endpic[0])
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
