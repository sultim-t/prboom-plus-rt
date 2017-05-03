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

#ifdef __cplusplus
extern "C"
{
#endif

struct BossAction
{
	int type;
	int special;
	int tag;
};

struct MapEntry
{
	char *mapname;
	char *levelname;
	char *intertext;
	char *intertextsecret;
	char levelpic[9];
	char nextmap[9];
	char nextsecret[9];
	char music[9];
	char skytexture[9];
	char endpic[9];
	char exitpic[9];
	char enterpic[9];
	char interbackdrop[9];
	char intermusic[9];
	int partime;
	int nointermission;
	int numbossactions;

	unsigned int propertycount;
	struct MapProperty *properties;
	struct BossAction *bossactions;
};

struct MapList
{
	unsigned int mapcount;
	struct MapEntry *maps;
};

typedef void (*umapinfo_errorfunc)(const char *fmt, ...);	// this must not return!

extern struct MapList Maps;

int ParseUMapInfo(const unsigned char *buffer, size_t length, umapinfo_errorfunc err);
void FreeMapList();
struct MapProperty *FindProperty(struct MapEntry *map, const char *name);

#ifdef __cplusplus
}
#endif

#endif
