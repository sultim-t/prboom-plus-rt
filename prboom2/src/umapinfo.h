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

#ifndef __UMAPINFO_H
#define __UMAPINFO_H

enum
{
	TYPE_NUMBER,
	TYPE_STRING,
	TYPE_IDENTIFIER
};

struct MapPropertyValue
{
	int type;
	union 
	{
		double number;
		char *string;
	} v;
};

struct MapProperty
{
	char *propertyname;
	unsigned int valuecount;
	struct MapPropertyValue *values;
};

struct MapEntry
{
	char *mapname;
	unsigned int propertycount;
	struct MapProperty *properties;
};

struct MapList
{
	unsigned int mapcount;
	struct MapEntry *maps;
};

typedef void (*umapinfo_errorfunc)(const char *fmt, ...);	// this must not return!

extern struct MapList Maps;

int ParseUMapInfo(const unsigned char *buffer, size_t length, umapinfo_errorfunc err);
void FreeMapData();

#endif
