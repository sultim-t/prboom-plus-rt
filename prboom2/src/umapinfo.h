//-----------------------------------------------------------------------------
//
// Copyright 2017 Christoph Oelckers, Charles Gunyon
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

typedef struct BossAction {
    int type;
    int special;
    int tag;
} UMIBossAction;

struct UMIMapEntryStruct {
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
    UMIBossAction *bossactions;
};

typedef struct UMIMapEntryStruct UMIMapEntry;

typedef struct {
    size_t len;
    UMIMapEntry *maps;
} UMIMapList;

extern UMIMapList umi_maps;

void UMI_Parse(const char *buffer, size_t length);
UMIMapEntry* UMI_LookupMapInfo(const char *map_name);

#endif
