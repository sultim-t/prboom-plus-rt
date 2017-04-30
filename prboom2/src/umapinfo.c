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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "ini.h"

#include "doomstat.h"
#include "m_misc.h"
#include "g_game.h"
#include "doomdef.h"
#include "lprintf.h"
#include "umapinfo.h"

static UMIMapEntry *current_map_entry = NULL;

static char* ini_reader_func(char *str, int num, void *stream) {
    return (char *)stream;
}

void M_AddEpisode(const char *map, char *def);

UMIMapList umi_maps;

//==========================================================================
//
// The Doom actors in their original order
// Names are the same as in ZDoom.
//
//==========================================================================

static const char* ActorNames[] = {
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

/* 307 lines for ini.c and ini.h */

static UMIMapEntry* add_map_entry(const char *map_name) {
    UMIMapEntry *map_entry = NULL;

    umi_maps.len++;
    umi_maps.maps = realloc(umi_maps.maps, sizeof(UMIMapEntry) * umi_maps.len);

    if (!umi_maps.maps) {
        I_Error("Error expanding UMI.maps");
    }

    map_entry = &umi_maps.maps[umi_maps.len - 1];
    map_entry->mapname = strdup(map_name);

    if (!map_entry->mapname) {
        I_Error("Error copying UMI map name");
    }

    return map_entry;
}

static UMIMapEntry* find_map_entry(const char *map_name) {
    size_t i;

    for (i = 0; i < umi_maps.len; i++) {
        UMIMapEntry *map_entry = &umi_maps.maps[i];

        if (stricmp(map_name, map_entry->mapname) == 0) {
            return map_entry;
        }
    }

    return NULL;
}

static void assert_property_value_max_length(const char *property_value,
                                             const char *s,
                                             size_t maxlen) {
    if (strlen(s) > maxlen) {
        I_Error(
            "Value for \"%s\" is too long; maximum size is %zu\n",
            property_value,
            maxlen
        );
    }
}

static void assert_valid_lump_name(const char *property_value,
                                   const char *lump_name) {
    assert_property_value_max_length(property_value, lump_name, 8);
}

static void assert_valid_map_name(const char *property_value,
                                  const char *map_name) {
    assert_property_value_max_length(property_value, map_name, 8);

    if (!G_ValidateMapName(map_name, NULL, NULL)) {
        I_Error("Invalid map name %s", map_name);
    }
}

static void assign_string(char **slot, const char *s) {
    if (*slot) {
        free(*slot);
    }

    *slot = strdup(s);

    if (!slot) {
        I_Error("Error assigning UMAPINFO string");
    }
}

static double parse_num(const char *property_name,
                        const char *property_value,
                        char **endptr) {
    char *temp_endptr = NULL;
    double out = 0.0;
    
    if (!endptr) {
        endptr = &temp_endptr;
    }

    out = strtod(property_value, endptr);

    errno = 0;

    if ((out == 0.0) && (*endptr == property_value)) {
        I_Error(
            "Invalid number value for %s: \"%s\"",
            property_name,
            property_value
        );
    }

    if (errno == ERANGE) {
        if (out == HUGE_VAL) {
            I_Error(
                "Number value for %s would cause overflow",
                property_name
            );
        }
        else {
            I_Error(
                "Number value for %s would cause underflow",
                property_name
            );
        }
    }

    return out;
}

static int parse_int(const char *property_name, const char *property_value) {
    double doubleval1 = parse_num(property_name, property_value, NULL);
    int intval = (int)doubleval1;
    double doubleval2 = (double)intval;

    if (doubleval1 != doubleval2) {
        I_Error("Expected integer value for %s, got float", property_name);
    }

    return intval;
}

static int parse_bool(const char *property_name,
                      const char *property_value) {
    if (!stricmp(property_value, "true")) {
        return 1;
    }

    if (!stricmp(property_value, "false")) {
        return 0;
    }

    I_Error("Expected true/false for %s", property_name);

    return -1; /* Make compiler happy here */
}

static int convert_bossaction_int(const char *property_name, char *startptr,
                                                             char **endptr) {
    long longval = 0L;
    
    errno = 0;
    strtol(startptr, endptr, 10);

    if (errno == ERANGE) {
        if (longval == LONG_MIN) {
            I_Error(
                "%s for bossaction property in UMAPINFO causes underflow",
                property_name
            );
        }
        else {
            I_Error(
                "%s for bossaction property in UMAPINFO causes overflow",
                property_name
            );
        }
    }

    if (longval > INT_MAX) {
        I_Error(
            "Line special for bossaction property in UMAPINFO is too large"
        );
    }

    if (longval < INT_MIN) {
        /* [CG] Not the most accurate error, but fuck it */
        I_Error(
            "Line special for bossaction property in UMAPINFO is too small"
        );
    }

    return (int)longval;
}

static void parse_boss_action(const char *boss_action, int *type,
                                                       int *line_special,
                                                       int *tag) {
    size_t i;
    size_t max = strlen(boss_action);
    char *startptr = (char *)boss_action;
    char *endptr = NULL;

    for (; (startptr - boss_action) < max; startptr++) {
        if (*startptr == ' ') {
            break;
        }

    }

    for (i = 0; ActorNames[i]; i++) {
        if (!strnicmp(ActorNames[i], boss_action, startptr - boss_action)) {
            *type = i;
            break;
        }
    }

    if (!ActorNames[i]) {
        I_Error(
            "Unknown thing type for bossaction property in UMAPINFO (%s)",
            boss_action
        );
    }

    if ((startptr - boss_action) >= max) {
        I_Error("Invalid bossaction property in UMAPINFO (%s)", boss_action);
    }

    startptr++;

    if ((startptr - boss_action) >= max) {
        I_Error(
            "No line special or tag for bossaction property in UMAPINFO (%s)",
            boss_action
        );
    }

    *line_special = convert_bossaction_int("Line special", startptr, &endptr);

    if (*endptr != ' ') {
        I_Error(
            "Line special and tag must be separated by a space in UMAPINFO "
            "bossaction properties (%s)",
            boss_action
        );
    }

    startptr = endptr + 1;

    if ((startptr - boss_action) >= max) {
        I_Error(
            "No tag for bossaction property in UMAPINFO (%s)",
            boss_action
        );
    }

    *tag = convert_bossaction_int("Tag", startptr, &endptr);
}

static int ini_value_handler(void *user, const char *map_name,
                                         const char *property_name,
                                         const char *property_value) {
    if (!current_map_entry) {
        current_map_entry = add_map_entry(map_name);
    }
    else if (!stricmp(map_name, current_map_entry->mapname)) {
        current_map_entry = find_map_entry(map_name);

        if (!current_map_entry) {
            current_map_entry = add_map_entry(map_name);
        }
    }

    if (!stricmp(property_name, "levelname")) {
        assign_string((char **)&current_map_entry->levelname, property_value);
    }
    else if (!stricmp(property_name, "next")) {
        assert_valid_map_name(property_name, property_value);
        assign_string((char **)&current_map_entry->nextmap, property_value);
    }
    else if (!stricmp(property_name, "nextsecret")) {
        assert_valid_map_name(property_name, property_value);
        assign_string((char **)&current_map_entry->nextsecret, property_value);
    }
    else if (!stricmp(property_name, "levelpic")) {
        assign_string((char **)&current_map_entry->levelpic, property_value);
    }
    else if (!stricmp(property_name, "skytexture")) {
        assign_string((char **)&current_map_entry->skytexture, property_value);
    }
    else if (!stricmp(property_name, "music")) {
        assign_string((char **)&current_map_entry->music, property_value);
    }
    else if (!stricmp(property_name, "endpic")) {
        assign_string((char **)&current_map_entry->endpic, property_value);
    }
    else if (!stricmp(property_name, "endcast")) {
        if (parse_bool(property_name, property_value)) {
            strcpy(current_map_entry->endpic, "$CAST");
        }
        else {
            strcpy(current_map_entry->endpic, "-");
        }
    }
    else if (!stricmp(property_name, "endbunny")) {
        if (parse_bool(property_name, property_value)) {
            strcpy(current_map_entry->endpic, "$BUNNY");
        }
        else {
            strcpy(current_map_entry->endpic, "-");
        }
    }
    else if (!stricmp(property_name, "endgame")) {
        if (parse_bool(property_name, property_value)) {
            strcpy(current_map_entry->endpic, "!");
        }
        else {
            strcpy(current_map_entry->endpic, "-");
        }
    }
    else if (!stricmp(property_name, "exitpic")) {
        assert_valid_lump_name(property_name, property_value);
        strcpy(current_map_entry->exitpic, property_value);
    }
    else if (!stricmp(property_name, "enterpic")) {
        assert_valid_lump_name(property_name, property_value);
        strcpy(current_map_entry->enterpic, property_value);
    }
    else if (!stricmp(property_name, "nointermission")) {
        current_map_entry->nointermission = parse_bool(
            property_name,
            property_value
        );
    }
    else if (!stricmp(property_name, "partime")) {
        current_map_entry->partime = TICRATE * parse_int(
            property_name,
            property_value
        );
    }
    else if (!stricmp(property_name, "intertext")) {
        assign_string((char **)&current_map_entry->intertext, property_value);
    }
    else if (!stricmp(property_name, "intertextsecret")) {
        assign_string(
            (char **)&current_map_entry->intertextsecret,
            property_value
        );
    }
    else if (!stricmp(property_name, "interbackdrop")) {
        assert_valid_lump_name(property_name, property_value);
        strcpy(current_map_entry->interbackdrop, property_value);
    }
    else if (!stricmp(property_name, "intermusic")) {
        assert_valid_lump_name(property_name, property_value);
        strcpy(current_map_entry->intermusic, property_value);
    }
    else if (!stricmp(property_name, "episode")) {
        M_AddEpisode(current_map_entry->mapname, (char *)property_value);
    }
    else if (!stricmp(property_name, "bossaction")) {
        if (!stricmp(property_value, "clear")) {
            if (current_map_entry->bossactions) {
                free(current_map_entry->bossactions);
            }

            current_map_entry->bossactions = NULL;
            current_map_entry->numbossactions = -1;
        }
        else {
            int type;
            int special;
            int tag;

            parse_boss_action(property_value, &type, &special, &tag);

            // allow no 0-tag specials here, unless a level exit.
            if (tag != 0 || special == 11 || special == 51 || special == 52
                                                           || special == 124) {
                UMIBossAction *boss_action = NULL;

                if (current_map_entry->numbossactions == -1) {
                    current_map_entry->numbossactions = 1;
                }
                else {
                    current_map_entry->numbossactions++;
                }

                current_map_entry->bossactions = realloc(
                    current_map_entry->bossactions,
                    sizeof(UMIBossAction) * current_map_entry->numbossactions
                );

                boss_action = &current_map_entry->bossactions[
                    current_map_entry->numbossactions - 1
                ];

                boss_action->type = type;
                boss_action->special = special;
                boss_action->tag = tag;
            }
        }
    }

    return 1;
}

// -----------------------------------------------
//
// Parses a complete UMAPINFO lump
//
// -----------------------------------------------

int ParseUMapInfo(const char *buffer, size_t length) {
    size_t i;

    // must reallocate to append a 0 for strtod to work
    char *newbuffer = malloc(length + 1);

    if (!newbuffer) {
        I_Error("Error allocating UMAPINFO buffer");
    }

    memcpy(newbuffer, buffer, length);
    newbuffer[length] = 0;

    if (ini_parse_stream((ini_reader)ini_reader_func, newbuffer,
                                                      ini_value_handler,
                                                      NULL) < 0) {
        I_Error("Parse error in UMAPINFO");
    }

    for (i = 0; i < umi_maps.len; i++) {
        UMIMapEntry *map_entry = &umi_maps.maps[i];

        // Set default level progression here to simplify the checks elsewhere.
        // Doing this lets us skip all normal code for this if nothing has been
        // defined.

        if (map_entry->endpic[0]) {
            map_entry->nextmap[0] = map_entry->nextsecret[0] = 0;
            if (map_entry->endpic[0] == '!') {
                map_entry->endpic[0] = 0;
            }
        }

        if (!map_entry->nextmap[0] && !map_entry->endpic[0]) {
            if (!stricmp(map_entry->mapname, "MAP30")) {
                strcpy(map_entry->endpic, "$CAST");
            }
            else if (!stricmp(map_entry->mapname, "E1M8")) {
                strcpy(map_entry->endpic, gamemode == retail? "CREDIT" : "HELP2");
            }
            else if (!stricmp(map_entry->mapname, "E2M8")) {
                strcpy(map_entry->endpic, "VICTORY");
            }
            else if (!stricmp(map_entry->mapname, "E3M8")) {
                strcpy(map_entry->endpic, "$BUNNY");
            }
            else if (!stricmp(map_entry->mapname, "E4M8")) {
                strcpy(map_entry->endpic, "ENDPIC");
            }
            else if (gamemission == chex && !stricmp(map_entry->mapname, "E1M5")) {
                strcpy(map_entry->endpic, "CREDIT");
            }
            else {
                int ep;
                int map;

                G_ValidateMapName(map_entry->mapname, &ep, &map);

                map++;

                if (gamemode == commercial) {
                    sprintf(map_entry->nextmap, "MAP%02d", map);
                }
                else {
                    sprintf(map_entry->nextmap, "E%dM%d", ep, map);
                }
            }
        }
    }

    free(newbuffer);

    return 1;
}

UMIMapEntry* UMI_LookupMapInfo(const char *map_name) {
    size_t i;

    for (size_t i = 0; i < umi_maps.len; i++) {
        UMIMapEntry *map_entry = &umi_maps.maps[i];

        if (!stricmp(map_entry->mapname, map_name)) {
            return map_entry;
        }
    }

    return NULL;
}
