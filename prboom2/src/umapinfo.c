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
#include "m_misc.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

/*
UMAOINFO uses an INI-like format,
[MAPxx]
property = value
...

property is a case insensitive identifier, beginning with a letter and may contain [a-z0-9_]
value is either an identifier, a number (only doubles are stored) or a string literal.
Comments must be in C++-form, i.e. from '//' until the end of the line.
*/

void M_AddEpisode(const char *map, char *def);

struct MapList Maps;

struct ParseState
{
	const unsigned char * end;
	const unsigned char * position;
	unsigned int line;
	int error;
	umapinfo_errorfunc ErrorFunction;
};


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
	"Slalagtite",
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
	NULL
};


// -----------------------------------------------
//
//
// -----------------------------------------------

static void FreeProperty(struct MapProperty *prop)
{
	unsigned i;
	if (prop->propertyname) free(prop->propertyname);
	for(i = 0; i < prop->valuecount; i++)
	{
		if (prop->values[i].type == TYPE_STRING || prop->values[i].type == TYPE_IDENTIFIER)
		{
			free (prop->values[i].v.string);
		}
	}
	if (prop->values) free(prop->values);
	prop->valuecount = 0;
	prop->propertyname = NULL;
	prop->values = NULL;
}

// -----------------------------------------------
//
//
// -----------------------------------------------

static void FreeMap(struct MapEntry *mape)
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

// -----------------------------------------------
//
// Parses an argument. This can either be a string literal,
// an identifier or a number that strtod can parse into a double
//
// -----------------------------------------------

static int SkipWhitespace(struct ParseState *state, int ignorelines)
{
again:
	while (*state->position != '\n' && isspace(*state->position))
	{
		state->position++;
		if (state->position == state->end) return 1;
	}
	if (*state->position == '\n')
	{
		state->position++;
		state->line++;
		if (ignorelines) goto again;
		return 1;
	}
	if (*state->position == '/' && state->position[1] == '/')
	{
		// skip the rest of this line.
		while (*state->position != '\n' && state->position < state->end) state->position++;
		state->line++;
		if (ignorelines) goto again;
		return 1;
	}
	return 0;
}

// -----------------------------------------------
//
// Parses an identifier
// Identifiers are case insensitve,  begin with a letter 
// and may contain [a-z0-9_]. 
//
// -----------------------------------------------

static char *ParseIdentifier(struct ParseState *state, int error)
{
	if (isalpha(*state->position))
	{
		size_t i, size;
		char *copiedstring;
		const unsigned char *startpos = state->position;
		while (isalnum(*state->position) || *state->position == '_')
		{
			state->position++;
			if (state->position == state->end) break;
		}
		size = state->position - startpos;
		copiedstring = (char*)malloc(size + 1);
		assert(copiedstring != NULL);
		for(i = 0; i < size; i++)
		{
			int c = startpos[i];
			if (!isalpha(c)) copiedstring[i] = (char)c;
			else copiedstring[i] = (char)(c | 32);	// make lowercase;
		}
		copiedstring[state->position - startpos] = 0;
		return copiedstring;
	}
	if (error)
	{
		state->error = 1;
		state->ErrorFunction("Identifier expected in line %u", state->line);
	}
	return NULL;
}

// -----------------------------------------------
//
// Parses a string literal
//
// -----------------------------------------------

static char *ParseString(struct ParseState *state, int error)
{
	int firstchar = *state->position;
	if (firstchar == '"')
	{
		size_t size;
		char *copiedstring;
		const unsigned char *startpos = ++state->position;
		while (*state->position != '"')
		{
			if (*state->position == '\\')
			{
				if (state->position == state->end - 1 || state->position[1] == '\n')
				{
					state->position++;
					state->line++;
					state->error = 1;
					state->ErrorFunction("Unterminated string constant in line %u", state->line);
					return NULL;	// reached the end of the line.
				}
				state->position += 2;
			}
			else state->position++;
			if (state->position >= state->end || *state->position == '\n')
			{
				state->error = 1;
				state->ErrorFunction("Unterminated string constant in line %u", state->line);
				return NULL;	// reached the end of the line.
			}
		}
		size = state->position - startpos;
		copiedstring = (char*)malloc(size + 1);
		assert(copiedstring != NULL);

		memcpy(copiedstring, startpos, size);
		copiedstring[size] = 0;
		/* is this really needed? PrBoom can only modify the string table but not add to it so it'a a relatively pointless feature.
#ifndef DYN_STRING_TABLE
		// Ports that have multilanguage string tables that can change at run time should disable this part and look up the string when being used.
		if (*copiedstring == '$')
		{
			const char *lookupstring = deh_LookupString(copiedstring+1);
			free(copiedstring);
			copiedstring = strdup(lookupstring);
		}
#endif
*/
		state->position++;
		return copiedstring;
	}
	if (error)
	{
		state->error = 1;
		state->ErrorFunction("String expected in line %u", state->line);
	}
	return NULL;
}

// -----------------------------------------------
//
// Parses a set of string and concatenates them
//
// -----------------------------------------------

static char *ParseMultiString(struct ParseState *state, int error)
{
	char *build = NULL;
	if (*state->position == 'c' && state->end - state->position > 5)
	{
		char cmp[6];
		memcpy(cmp, state->position, 5);
		cmp[5] = 0;
		if (!stricmp(cmp, "clear"))
		{
			state->position += 5;
			return strdup("-");	// this was explicitly deleted to override the default.
		}
	}

	for (;;)
	{
		char *str = ParseString(state, error);
		if (str == NULL)
		{
			if (build) free(build);
			return NULL;
		}
		if (build == NULL) build = str;
		else
		{
			size_t oldlen = strlen(build);
			size_t newlen = oldlen + strlen(str) + 1;

			build = realloc(build, newlen);
			build[oldlen] = '\n';
			strcpy(build + oldlen + 1, str);
			build[newlen] = 0;
		}
		SkipWhitespace(state, false);
		if (*state->position != ',') return build;
		state->position++;
		SkipWhitespace(state, true);
	}
}


// -----------------------------------------------
//
// Parses a lump name. The buffer must be at least 9 characters.
//
// -----------------------------------------------

static int ParseLumpName(struct ParseState *state, char *buffer, int error)
{
	int firstchar = *state->position;
	if (firstchar == '"')
	{
		size_t size;
		const unsigned char *startpos = ++state->position;
		while (*state->position != '"')
		{
			// No filters allowed in lump names. Use proper names!
			state->position++;
			if (state->position >= state->end || *state->position == '\n')
			{
				state->error = 1;
				state->ErrorFunction("Unterminated string constant in line %u", state->line);
				return 0;	// reached the end of the line.
			}
		}
		size = state->position - startpos;
		if (size > 8)
		{
			state->error = 1;
			state->ErrorFunction("String too long. Maximum size is 8 characters in line %u", state->line);
			return 0;
		}
		strncpy(buffer, startpos, size);
		M_Strupr(buffer);

		state->position++;
		return 1;
	}
	if (error)
	{
		state->error = 1;
		state->ErrorFunction("String expected in line %u", state->line);
	}
	return 0;
}

// -----------------------------------------------
//
// Parses a floating point number
//
// -----------------------------------------------

static double ParseFloat(struct ParseState *state)
{
	const unsigned char *newpos;
	double value = strtod((char*)state->position, (char**)&newpos);
	if (newpos == state->position)
	{
		state->error = 1;
		state->ErrorFunction("Cannot find a property value in line %u", state->line);
		return 0;
	}
	else if (isalnum(*newpos) || *newpos == '_')
	{
		state->error = 1;
		state->ErrorFunction("Syntax error in line %u: numeric constant followed by invalid characters", state->line);
		return 0;
	}
	state->position = newpos;
	return value;
}

// -----------------------------------------------
//
// Parses an integer number
//
// -----------------------------------------------

static long ParseInt(struct ParseState *state, int allowbool)
{
	const unsigned char *newpos;
	long value;
	if (allowbool && (tolower(*state->position) == 't' || tolower(*state->position) == 'f'))
	{
		char *id = ParseIdentifier(state, 1);
		if (id)
		{
			if (!stricmp(id, "true"))
			{
				free(id);
				return 1;
			}
			if (!stricmp(id, "false"))
			{
				free(id);
				return 0;
			}
		}
		else
		{
			state->error = 1;
			state->ErrorFunction("Cannot find a property value in line %u", state->line);
			return 0;
		}
	}
	value = strtol((char*)state->position, (char**)&newpos, 0);
	if (newpos == state->position)
	{
		state->error = 1;
		state->ErrorFunction("Cannot find a property value in line %u", state->line);
		return 0;
	}
	else if (isalnum(*newpos) || *newpos == '_')
	{
		state->error = 1;
		state->ErrorFunction("Syntax error in line %u: numeric constant followed by invalid characters", state->line);
		return 0;
	}
	state->position = newpos;
	return value;
}

// -----------------------------------------------
//
// Parses an argument. This can either be a string literal,
// an identifier or a number that strtod can parse into a double
//
// -----------------------------------------------

static int ParseArgument(struct ParseState *state, struct MapPropertyValue *val)
{
	int firstchar = *state->position;
	if (firstchar == '"')
	{
		char *copiedstring = ParseString(state, 0);
		if(copiedstring == NULL) return 0;
		
		// todo: Filter '\\'
		val->type = TYPE_STRING;
		val->v.string = copiedstring;
		return 1;
	}
	else if (isalpha(firstchar)) 
	{
		// This case cannot error out because we got at least one valid letter.
		char *copiedstring = ParseIdentifier(state, 0);
		val->type = TYPE_IDENTIFIER;
		val->v.string = copiedstring;
		return 1;
	}
	else
	{
		val->type = TYPE_NUMBER;
		val->v.number = ParseFloat(state);
		return !state->error;
	}
}

// -----------------------------------------------
//
// Parses an assignment operator
//
// -----------------------------------------------

static int ParseAssign(struct ParseState *state)
{
	if (SkipWhitespace(state, false))
	{
		state->error = 1;
		state->ErrorFunction("'=' expected in line %u", state->line);
		return 0;
	}
	if (*state->position != '=')
	{
		state->error = 1;
		state->ErrorFunction("'=' expected in line %u", state->line);
		return 0;
	}
	state->position++;
	if (SkipWhitespace(state, false ))
	{
		state->error = 1;
		state->ErrorFunction("Unexpected end of file %u", state->line);
		return 0;
	}
	return 1;
}

// -----------------------------------------------
//
// Parses an assignment operator
//
// -----------------------------------------------

static int ParseComma(struct ParseState *state)
{
	if (SkipWhitespace(state, false))
	{
		state->error = 1;
		state->ErrorFunction("',' expected in line %u", state->line);
		return 0;
	}
	if (*state->position != ',')
	{
		state->error = 1;
		state->ErrorFunction("',' expected in line %u", state->line);
		return 0;
	}
	state->position++;
	if (SkipWhitespace(state, false))
	{
		state->error = 1;
		state->ErrorFunction("Unexpected end of file %u", state->line);
		return 0;
	}
	return 1;
}

// -----------------------------------------------
//
// Parses a map property of the form 
// 'property = value1 [, value2...]'
//
// -----------------------------------------------

static int ParseMapProperty(struct ParseState *state, struct MapProperty *val)
{
	char *pname;
	// find the next line with content.
	while (state->position < state->end && SkipWhitespace(state, false));
	// this line is no property.
	if (*state->position == '[' || state->position >= state->end) return 0;
	pname = ParseIdentifier(state, 1);
	val->propertyname = pname;

	if (pname == NULL)
	{
		return 0;
	}
	if (!ParseAssign(state)) return 0;

	while(1)
	{
		struct MapPropertyValue propval = { 0 };
		if (!ParseArgument(state, &propval)) return 0;
		val->valuecount++;
		val->values = (struct MapPropertyValue*)realloc(val->values, sizeof(struct MapPropertyValue) * val->valuecount);
		
		if (SkipWhitespace(state, false)) return 1;
		if (*state->position != ',')
		{
			state->error = 1;
			state->ErrorFunction("',' expected in line %u", state->line);
			return 0;
		}
		state->position++;
		if (SkipWhitespace(state, false))
		{
			state->error = 1;
			state->ErrorFunction("Unexpected end of file in line %u", state->line);
			return 0;
		}
	}
}

// -----------------------------------------------
//
// Parses a standard property that is already known
// These do not get stored in the property list
// but in dedicated struct member variables.
//
// -----------------------------------------------

static int ParseStandardProperty(struct ParseState *state, struct MapEntry *mape)
{
	const unsigned char *savedpos;
	char *pname;

	// find the next line with content.
	while (state->position < state->end && SkipWhitespace(state, false));
	// this line is no property.
	if (*state->position == '[' || state->position >= state->end) return 0;
	
	savedpos = state->position;
	pname = ParseIdentifier(state, 0);
	if (pname == 0) return 0;
	if (!ParseAssign(state)) return 0;
	if (!stricmp(pname, "levelname"))
	{
		char *lname = ParseString(state, 1);
		if (!lname) return 0;
		if (mape->levelname != NULL) free(mape->levelname);
		mape->levelname = lname;
	}
	else if (!stricmp(pname, "next"))
	{
		ParseLumpName(state, mape->nextmap, 1);
		if (!G_ValidateMapName(mape->nextmap, NULL, NULL))
		{
			state->error = 1;
			state->ErrorFunction("Invalid map name %s in file %u", mape->nextmap, state->line);
			return 0;
		}
	}
	else if (!stricmp(pname, "nextsecret"))
	{
		ParseLumpName(state, mape->nextsecret, 1);
		if (!G_ValidateMapName(mape->nextsecret, NULL, NULL))
		{
			state->error = 1;
			state->ErrorFunction("Invalid map name %s in file %u", mape->nextmap, state->line);
			return 0;
		}
	}
	else if (!stricmp(pname, "levelpic"))
	{
		ParseLumpName(state, mape->levelpic, 1);
	}
	else if (!stricmp(pname, "skytexture"))
	{
		ParseLumpName(state, mape->skytexture, 1);
	}
	else if (!stricmp(pname, "music"))
	{
		ParseLumpName(state, mape->music, 1);
	}
	else if (!stricmp(pname, "endpic"))
	{
		ParseLumpName(state, mape->endpic, 1);
	}
	else if (!stricmp(pname, "endcast"))
	{
		if (ParseInt(state, true)) strcpy(mape->endpic, "$CAST");
		else strcpy(mape->endpic, "-");
	}
	else if (!stricmp(pname, "endbunny"))
	{
		if (ParseInt(state, true)) strcpy(mape->endpic, "$BUNNY");
		else strcpy(mape->endpic, "-");
	}
	else if (!stricmp(pname, "endgame"))
	{
		if (ParseInt(state, true)) strcpy(mape->endpic, "!");
		else strcpy(mape->endpic, "-");
	}
	else if (!stricmp(pname, "exitpic"))
	{
		ParseLumpName(state, mape->exitpic, 1);
	}
	else if (!stricmp(pname, "enterpic"))
	{
		ParseLumpName(state, mape->enterpic, 1);
	}
	else if (!stricmp(pname, "nointermission"))
	{
		mape->nointermission = ParseInt(state, true);
	}
	else if (!stricmp(pname, "partime"))
	{
		mape->partime = TICRATE * ParseInt(state, false);
	}
	else if (!stricmp(pname, "intertext"))
	{
		char *lname = ParseMultiString(state, 1);
		if (!lname) return 0;
		if (mape->intertext != NULL) free(mape->intertext);
		mape->intertext = lname;
	}
	else if (!stricmp(pname, "intertextsecret"))
	{
		char *lname = ParseMultiString(state, 1);
		if (!lname) return 0;
		if (mape->intertextsecret != NULL) free(mape->intertextsecret);
		mape->intertextsecret = lname;
	}
	else if (!stricmp(pname, "interbackdrop"))
	{
		ParseLumpName(state, mape->interbackdrop, 1);
	}
	else if (!stricmp(pname, "intermusic"))
	{
		ParseLumpName(state, mape->intermusic, 1);
	}
	else if (!stricmp(pname, "episode"))
	{
		char *lname = ParseMultiString(state, 1);
		if (!lname) return 0;
		M_AddEpisode(mape->mapname, lname);
	}
	else if (!stricmp(pname, "bossaction"))
	{
		char * classname = ParseIdentifier(state, true);
		int classnum, special, tag;
		if (!stricmp(classname, "clear"))
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
				if (!stricmp(classname, ActorNames[i])) break;
			}
			if (ActorNames[i] == NULL)
			{
				state->error = 1;
				state->ErrorFunction("Unknown thing type %s in line %d", classname, state->line);
				return 0;
			}

			ParseComma(state);
			special = ParseInt(state, false);
			ParseComma(state);
			tag = ParseInt(state, false);
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
		free(classname);
	}
	else
	{
		state->position = savedpos;
		return 0;
	}

	free(pname);
	return !state->error;
}

// -----------------------------------------------
//
// Parses a complete map entry
//
// -----------------------------------------------

static int ParseMapEntry(struct ParseState *state, struct MapEntry *val)
{
	char *pname;

	val->mapname = NULL;
	val->propertycount = 0;
	val->properties = NULL;

	// find the next line with content.
	while (state->position < state->end && SkipWhitespace(state, false));
	if (*state->position != '[')
	{
		state->error = 1;
		state->ErrorFunction("'[' expected in line %u", state->line);
		return 0;
	}
	state->position++;
	pname = ParseIdentifier(state, 1);
	val->mapname = pname;
	if (pname == NULL)
	{
		return 0;
	}
	if (*state->position != ']')
	{
		state->error = 1;
		state->ErrorFunction("']' expected in line %u", state->line);
		return 0;
	}
	state->position++;
	if (!SkipWhitespace(state, false))
	{
		state->error = 1;
		state->ErrorFunction("Unexpected content in line %u", state->line);
		return 0;
	}
	while(1)
	{
		unsigned i;
		struct MapProperty prop = { 0 };
		if (!ParseStandardProperty(state, val) && !state->error)
		{
			if (!ParseMapProperty(state, &prop)) return !state->error;	// If we get here, it's either that no more properties were found or an error occured, so we need to check the error flag.

			// Does this property already exist? If yes, replace it.
			for (i = 0; i < val->propertycount; i++)
			{
				if (!strcmp(prop.propertyname, val->properties[i].propertyname))
				{
					FreeProperty(&val->properties[i]);
					val->properties[i] = prop;
					break;
				}
			}
			// Not found so create a new one.
			if (i == val->propertycount)
			{
				val->propertycount++;
				val->properties = (struct MapProperty*)realloc(val->properties, sizeof(struct MapProperty)*val->propertycount);
				val->properties[val->propertycount - 1] = prop;
			}
		}
		else if (state->error)
		{
			return 0;
		}
	}
}

// -----------------------------------------------
//
// Parses a complete UMAPINFO lump
//
// -----------------------------------------------

int ParseUMapInfo(const unsigned char *buffer, size_t length, umapinfo_errorfunc err)
{
	struct ParseState parse;
	unsigned int i;

	// must reallocate to append a 0 for strtod to work
	unsigned char *newbuffer = (unsigned char*)malloc(length+1);
	assert(newbuffer != NULL);
	memcpy(newbuffer, buffer, length);
	newbuffer[length] = 0;

	parse.position = newbuffer;
	parse.end = newbuffer + strlen((char*)newbuffer);
	parse.line = 1;
	parse.error = 0;
	parse.ErrorFunction = err;
	
	while (parse.position < parse.end)
	{
		struct MapEntry parsed = { 0 };
		if (!ParseMapEntry(&parse, &parsed))
		{
			// we should never get here, but if we do, let's clean out the data.
			FreeMapList();
			free(newbuffer);
			return 0;
		}

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
			Maps.maps = (struct MapEntry*)realloc(Maps.maps, sizeof(struct MapEntry)*Maps.mapcount);
			Maps.maps[Maps.mapcount-1] = parsed;
		}
		
	}
	free(newbuffer);
	return 1;
}


struct MapProperty *FindProperty(struct MapEntry *map, const char *name)
{
	return NULL;
}
