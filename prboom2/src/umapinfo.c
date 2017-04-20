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

/*
UMAOINFO uses an INI-like format,
[MAPxx]
property = value
...

property is a case insensitive identifier, beginning with a letter and may contain [a-z0-9_]
value is either an identifier, a number (only doubles are stored) or a string literal.
Comments must be in C++-form, i.e. from '//' until the end of the line.
*/


struct MapList Maps;

struct ParseState
{
	const unsigned char * end;
	const unsigned char * position;
	unsigned int line;
	int error;
	umapinfo_errorfunc ErrorFunction;
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
	if (mape->properties) free(mape->properties);
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
// Parses an identifier
// Identifiers are case insensitve,  begin with a letter 
// and may contain [a-z0-9_]. 
//
// -----------------------------------------------

static char *ParseIdentifier(struct ParseState *state)
{
	if (isalpha(*state->position))
	{
		const unsigned char *startpos = state->position;
		while (isalnum(*state->position) || *state->position == '_')
		{
			state->position++;
			if (state->position == state->end) break;
		}
		size_t size = state->position - startpos;
		char *copiedstring = (char*)malloc(size + 1);
		assert(copiedstring != NULL);
		for(size_t i = 0; i < size; i++)
		{
			int c = startpos[i];
			if (!isalpha(c)) copiedstring[i] = (char)c;
			else copiedstring[i] = (char)(c | 32);	// make lowercase;
		}
		copiedstring[state->position - startpos] = 0;
		return copiedstring;
	}
	return NULL;
}

// -----------------------------------------------
//
// Parses a string literal
//
// -----------------------------------------------

static char *ParseString(struct ParseState *state)
{
	int firstchar = *state->position;
	if (firstchar == '"')
	{
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
		size_t size = state->position - startpos;
		char *copiedstring = (char*)malloc(size + 1);
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
	return NULL;
}

// -----------------------------------------------
//
// Parses a string literal
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
// Parses a string literal
//
// -----------------------------------------------

static long ParseInt(struct ParseState *state)
{
	const unsigned char *newpos;
	long value = strtol((char*)state->position, (char**)&newpos, 0);
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
		char *copiedstring = ParseString(state);
		if(copiedstring == NULL) return 0;
		
		// todo: Filter '\\'
		val->type = TYPE_STRING;
		val->v.string = copiedstring;
		return 1;
	}
	else if (isalpha(firstchar)) 
	{
		// This case cannot error out because we got at least one valid letter.
		char *copiedstring = ParseIdentifier(state);
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
// Parses an argument. This can either be a string literal,
// an identifier or a number that strtod can parse into a double
//
// -----------------------------------------------

static int SkipWhitespace(struct ParseState *state)
{
	while (*state->position != '\n' && isspace(*state->position))
	{
		state->position++;
		if (state->position == state->end) return 1;
	}
	if (*state->position == '\n')
	{
		state->position++;
		state->line++;
		return 1;
	}
	if (*state->position == '/' && state->position[1] == '/')
	{
		// skip the rest of this line.
		while (*state->position != '\n' && state->position < state->end) state->position++;
		state->line++;
		return 1;
	}
	return 0;
}

// -----------------------------------------------
//
// Parses an assignment operator
//
// -----------------------------------------------

static int ParseAssign(struct ParseState *state)
{
	if (SkipWhitespace(state))
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
	if (SkipWhitespace(state))
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
	// find the next line with content.
	while (state->position < state->end && SkipWhitespace(state));
	// this line is no property.
	if (*state->position == '[' || state->position >= state->end) return 0;
	char *pname = ParseIdentifier(state);
	val->propertyname = pname;

	if (pname == NULL)
	{
		state->error = 1;
		state->ErrorFunction("Identifier expected in line %u", state->line);
		return 0;
	}
	if (!ParseAssign(state)) return 0;

	while(1)
	{
		struct MapPropertyValue propval = { 0 };
		if (!ParseArgument(state, &propval)) return 0;
		val->valuecount++;
		val->values = (struct MapPropertyValue*)realloc(val->values, sizeof(struct MapPropertyValue) * val->valuecount);
		
		if (SkipWhitespace(state)) return 1;
		if (*state->position != ',')
		{
			state->error = 1;
			state->ErrorFunction("',' expected in line %u", state->line);
			return 0;
		}
		state->position++;
		if (SkipWhitespace(state))
		{
			state->error = 1;
			state->ErrorFunction("Unexpected end of file %u", state->line);
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
	// find the next line with content.
	while (state->position < state->end && SkipWhitespace(state));
	// this line is no property.
	if (*state->position == '[' || state->position >= state->end) return 0;
	
	const unsigned char *savedpos = state->position;
	char *pname = ParseIdentifier(state);
	if (pname == 0) return 0;
	if (!ParseAssign(state)) return 0;
	if (!stricmp(pname, "levelname"))
	{
		char *lname = ParseString(state);
		if (!lname) return 0;
		if (mape->levelname != NULL) free(mape->levelname);
		mape->levelname = lname;
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
	val->mapname = NULL;
	val->propertycount = 0;
	val->properties = NULL;

	// find the next line with content.
	while (state->position < state->end && SkipWhitespace(state));
	if (*state->position != '[')
	{
		state->error = 1;
		state->ErrorFunction("'[' expected in line %u", state->line);
		return 0;
	}
	state->position++;
	char *pname = ParseIdentifier(state);
	val->mapname = pname;
	if (pname == NULL)
	{
		state->error = 1;
		state->ErrorFunction("Identifier expected in line %u", state->line);
		return 0;
	}
	if (*state->position != ']')
	{
		state->error = 1;
		state->ErrorFunction("']' expected in line %u", state->line);
		return 0;
	}
	state->position++;
	if (!SkipWhitespace(state))
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
