/*
** thingdef.cpp
**
** Actor definitions
**
**---------------------------------------------------------------------------
** Copyright 2002-2005 Christoph Oelckers
** Copyright 2004-2005 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <stdlib.h>
#include <string.h>
#include "tarray.h"
#include "scanner.h"
#include "thingdef.h"
extern "C"
{
#include "doomdef.h"
#include "p_enemy.h"
#include "m_misc.h"
#include "d_think.h"
#include "p_pspr.h"
}

void A_SetSolid(mobj_t * self);
void A_UnsetSolid(mobj_t * self);
void A_SetFloat(mobj_t * self);
void A_UnsetFloat(mobj_t * self);
void A_PlaySoundParms(mobj_t * self);
void A_SeekerMissile(mobj_t * self);
void A_BulletAttack (mobj_t *self);
void A_Jump(mobj_t * self);
void A_JumpIfHealthLower(mobj_t * self);
void A_JumpIfCloser(mobj_t * self);
void A_ExplodeParms (mobj_t *self);
void A_CustomMissile(mobj_t * self);
void A_CustomBulletAttack (mobj_t *self);
void A_ChangeFlag(mobj_t *self);

// all state parameters
extern TArray<int> StateParameters;


//==========================================================================
//
// Extra info maintained while defining an actor. The original
// implementation stored these in a CustomActor class. They have all been
// moved into action function parameters so that no special CustomActor
// class is necessary.
//
//==========================================================================

struct Baggage
{
	mobjinfo_t *Info;
	bool DropItemSet;
	bool StateSet;
	int CurrentState;
	FDropItem *DropItems;
};

//==========================================================================
//
// Sets the default values with which an actor definition starts
//
//==========================================================================

static void ResetBaggage(Baggage *bag)
{
	bag->DropItemSet = false;
	bag->CurrentState = 0;
	bag->StateSet = false;
	bag->DropItems = NULL;
}

//==========================================================================
//
// List of all flags
//
//==========================================================================

#define DEFINE_FLAG(name) { MF_##name, #name }
#define DEFINE_FLAG2(symbol, name) { MF_##symbol, #name }
#define DEFINE_NO_FLAG(name) { 0, #name }

struct flagdef
{
	uint_64_t flagbit;
	const char * name;
};


static flagdef ActorFlags[]=
{
	DEFINE_FLAG(SOLID),
	DEFINE_FLAG(SHOOTABLE),
	DEFINE_FLAG(NOSECTOR),
	DEFINE_FLAG(NOBLOCKMAP),
	DEFINE_FLAG(AMBUSH),
	DEFINE_FLAG(JUSTHIT),
	DEFINE_FLAG(JUSTATTACKED),
	DEFINE_FLAG(SPAWNCEILING),
	DEFINE_FLAG(NOGRAVITY),
	DEFINE_FLAG(DROPOFF),
	DEFINE_FLAG(NOCLIP),
	DEFINE_FLAG(FLOAT),
	DEFINE_FLAG(TELEPORT),
	DEFINE_FLAG(MISSILE),
	DEFINE_FLAG(DROPPED),
	DEFINE_FLAG(SHADOW),
	DEFINE_FLAG(NOBLOOD),
	DEFINE_FLAG(CORPSE),
	DEFINE_FLAG(INFLOAT),
	DEFINE_FLAG(COUNTKILL),
	DEFINE_FLAG(COUNTITEM),
	DEFINE_FLAG(SKULLFLY),
	DEFINE_FLAG(NOTDMATCH),
	DEFINE_FLAG(TOUCHY),
	// BOUNCES is intentionally omitted because Killough's hacky implementation is basically non-portable and contains so many special cases that it cannot be made fully compatible with ZDoom.
	DEFINE_FLAG2(FRIEND, FRIENDLY),	// uses ZDoom's name for compatibility.
	
	// Previously hardcoded traits turned into flag, names are the same as for ZDoom's DECORATE.
	DEFINE_FLAG(NOTARGET),
	DEFINE_FLAG(NOTARGET),
	DEFINE_FLAG(MISSILEMORE),
	DEFINE_FLAG(FULLVOLSIGHT),
	DEFINE_FLAG(FULLVOLDEATH),
	DEFINE_FLAG(NORADIUSDMG),
	DEFINE_FLAG(QUICKTORETALIATE),
	DEFINE_FLAG(ISMONSTER),
	DEFINE_FLAG(DONTFALL),
	DEFINE_FLAG(SEEKERMISSILE),
	
	// PrBoom has no use for these but other ports might depend on them so allow these names as no-ops.
	DEFINE_NO_FLAG(CANPASS),
	DEFINE_NO_FLAG(FLOORCLIP),
};

//==========================================================================
//
// Find a flag by name using a binary search
//
//==========================================================================
static int flagcmp(const void *a, const void *b)
{
	return stricmp( ((flagdef*)a)->name, ((flagdef*)b)->name);
}

static flagdef *FindFlag(char *string)
{
	static bool flagsorted=false;

	if (!flagsorted) 
	{
		qsort(ActorFlags, sizeof(ActorFlags)/sizeof(ActorFlags[0]), sizeof(ActorFlags[0]), flagcmp);
		flagsorted=true;
	}
	M_Strupr(string);

	int min = 0, max = sizeof(ActorFlags)/sizeof(ActorFlags[0])-1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = strcmp (string, ActorFlags[mid].name);
		if (lexval == 0)
		{
			return &ActorFlags[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
// Action functions
//
//==========================================================================


struct AFuncDesc
{
	const char * Name;
	actionf_p1 Function;
	const char * parameters;
};

#define FUNC(name, parm) { #name, (actionf_p1)name, parm },
// Declare the code pointer table
AFuncDesc AFTable[]=
{
	// most of the functions available in Dehacked
	FUNC(A_BFGSpray, NULL)
	FUNC(A_Pain, NULL)
	{"A_NoBlocking", (actionf_p1)A_Fall, NULL},		// Allow ZDoom's name, too.
	FUNC(A_Fall, NULL)
	FUNC(A_XScream, NULL)
	FUNC(A_Look, NULL)
	FUNC(A_Chase, NULL)
	FUNC(A_FaceTarget, NULL)
	FUNC(A_PosAttack, NULL)
	FUNC(A_Scream, NULL)
	FUNC(A_SPosAttack, NULL)
	FUNC(A_VileChase, NULL)
	FUNC(A_VileStart, NULL)
	FUNC(A_VileTarget, NULL)
	FUNC(A_VileAttack, NULL)
	FUNC(A_StartFire, NULL)
	FUNC(A_Fire, NULL)
	FUNC(A_FireCrackle, NULL)
	FUNC(A_Tracer, NULL)
	FUNC(A_SkelWhoosh, NULL)
	FUNC(A_SkelFist, NULL)
	FUNC(A_SkelMissile, NULL)
	FUNC(A_FatRaise, NULL)
	FUNC(A_FatAttack1, "m")
	FUNC(A_FatAttack2, "m")
	FUNC(A_FatAttack3, "m")
	FUNC(A_BossDeath, NULL)
	FUNC(A_CPosAttack, NULL)
	FUNC(A_CPosRefire, NULL)
	FUNC(A_TroopAttack, NULL)
	FUNC(A_SargAttack, NULL)
	FUNC(A_HeadAttack, NULL)
	FUNC(A_BruisAttack, NULL)
	FUNC(A_SkullAttack, NULL)
	FUNC(A_Metal, NULL)
	FUNC(A_SpidRefire, NULL)
	FUNC(A_BabyMetal, NULL)
	FUNC(A_BspiAttack, NULL)
	FUNC(A_Hoof, NULL)
	FUNC(A_CyberAttack, NULL)
	FUNC(A_PainAttack, NULL)
	FUNC(A_PainDie, NULL)
	FUNC(A_KeenDie, NULL)
	FUNC(A_BrainPain, NULL)
	FUNC(A_BrainScream, NULL)
	FUNC(A_BrainDie, NULL)
	FUNC(A_BrainAwake, NULL)
	FUNC(A_BrainSpit, NULL)
	FUNC(A_SpawnSound, NULL)
	FUNC(A_SpawnFly, NULL)
	FUNC(A_BrainExplode, NULL)
	FUNC(A_Die, NULL)
	FUNC(A_Detonate, NULL)
	FUNC(A_Mushroom, NULL)

	FUNC(A_SetSolid, NULL)
	FUNC(A_UnsetSolid, NULL)
	FUNC(A_SetFloat, NULL)
	FUNC(A_UnsetFloat, NULL)

	// DECORATE specific functions
	FUNC(A_BulletAttack, NULL)
	{"A_Explode", (actionf_p1)A_ExplodeParms, "i"},	// Do not call the original function for this.
	{"A_PlaySound", (actionf_p1)A_PlaySoundParms, "S"},	// Do not call the original function for this.
	FUNC(A_SeekerMissile, "AA" )
	FUNC(A_Jump, "IL" )
	FUNC(A_CustomMissile, "MFFai" )
	FUNC(A_CustomBulletAttack, "AAII" )
	FUNC(A_JumpIfHealthLower, "IL" )
	FUNC(A_JumpIfCloser, "IL" )
	FUNC(A_ChangeFlag, "GI")
};

//==========================================================================
//
// Find a function by name using a binary search
//
//==========================================================================
static int funccmp(const void * a, const void * b)
{
	return stricmp( ((AFuncDesc*)a)->Name, ((AFuncDesc*)b)->Name);
}

static AFuncDesc * FindFunction(char * string)
{
	static bool funcsorted=false;

	if (!funcsorted) 
	{
		qsort(AFTable, sizeof(AFTable)/sizeof(AFTable[0]), sizeof(AFTable[0]), funccmp);
		funcsorted=true;
	}

	int min = 0, max = sizeof(AFTable)/sizeof(AFTable[0])-1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = stricmp (string, AFTable[mid].Name);
		if (lexval == 0)
		{
			return &AFTable[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
//
//
//==========================================================================
struct StateLabel
{
	const char *name;
	int index;
};

typedef TArray<StateLabel> LabelList;

TArray<LabelList*> GlobalLabels;

static void CompileStateList()
{
	StateLabel sl;
	
	// Get all the original actors' state label associations for inheritance
	for(int i=0;i<NUMMOBJTYPES;i++)
	{
		LabelList *list = new LabelList;
		mobjinfo_t *info = &mobjinfo[i];
		if (info->spawnstate) 
		{
			sl.name = "spawn";
			sl.index = info->spawnstate;
			list->Push(sl);
		}
		if (info->seestate) 
		{
			sl.name = "see";
			sl.index = info->seestate;
			list->Push(sl);
		}
		if (info->spawnstate) 
		{
			sl.name = "melee";
			sl.index = info->meleestate;
			list->Push(sl);
		}
		if (info->spawnstate) 
		{
			sl.name = "missile";
			sl.index = info->missilestate;
			list->Push(sl);
		}
		if (info->spawnstate) 
		{
			sl.name = "pain";
			sl.index = info->painstate;
			list->Push(sl);
		}
		if (info->deathstate) 
		{
			sl.name = "death";
			sl.index = info->deathstate;
			list->Push(sl);
		}
		if (info->xdeathstate) 
		{
			sl.name = "xdeath";
			sl.index = info->xdeathstate;
			list->Push(sl);
		}
		if (info->raisestate) 
		{
			sl.name = "raise";
			sl.index = info->deathstate;
			list->Push(sl);
		}
		if (info->healstate) 
		{
			sl.name = "heal";
			sl.index = info->healstate;
			list->Push(sl);
		}
		if (info->crushstate) 
		{
			sl.name = "crush";
			sl.index = info->crushstate;
			list->Push(sl);
		}
		GlobalLabels.Push(list);
	}
}


//==========================================================================
//
// Property parsers
//
//==========================================================================

typedef void(*ActorPropFunction) (Scanner &sc, mobjinfo_t *defaults, Baggage &bag);
struct ActorProps { const char *name; ActorPropFunction Handler; };

// placeholders
int FindSound(const char *name)
{
	return 0;
}

mobjtype_t FindType(const char *type)
{
	return (mobjtype_t)-1;
}
//==========================================================================
//
//==========================================================================
static void ActorHealth(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	defaults->spawnhealth = sc.number;
}

//==========================================================================
//
//==========================================================================
static void ActorReactionTime(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	defaults->reactiontime = sc.number;
}

//==========================================================================
//
//==========================================================================
static void ActorPainChance(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	defaults->painchance = sc.number;
}

//==========================================================================
//
//==========================================================================
static void ActorDamage(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	defaults->damage = sc.number;
}

//==========================================================================
//
//==========================================================================
static void ActorSpeed(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	defaults->speed = sc.number;	// needs to be multiplied with FRACUNIT for missiles
}

//==========================================================================
//
//==========================================================================
static void ActorRadius(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetFloat();
	defaults->radius = fixed_t(sc.decimal*FRACUNIT);
}

//==========================================================================
//
//==========================================================================
static void ActorHeight(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetFloat();
	defaults->height = fixed_t(sc.decimal*FRACUNIT);
}

//==========================================================================
//
//==========================================================================
static void ActorMass(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	defaults->mass = sc.number;
}

//==========================================================================
//
//==========================================================================
static void ActorSeeSound(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetToken(TK_StringConst);
	defaults->seesound = FindSound(sc.string);
}

//==========================================================================
//
//==========================================================================
static void ActorAttackSound(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetToken(TK_StringConst);
	defaults->attacksound = FindSound(sc.string);
}

//==========================================================================
//
//==========================================================================
static void ActorPainSound(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetToken(TK_StringConst);
	defaults->painsound = FindSound(sc.string);
}

//==========================================================================
//
//==========================================================================
static void ActorDeathSound(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetToken(TK_StringConst);
	defaults->deathsound = FindSound(sc.string);
}

//==========================================================================
//
//==========================================================================
static void ActorActiveSound(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetToken(TK_StringConst);
	defaults->activesound = FindSound(sc.string);
}

//==========================================================================
//
//==========================================================================
static void ActorDropItem(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	// create a linked list of dropitems
	if (!bag.DropItemSet)
	{
		bag.DropItemSet = true;
		bag.DropItems = NULL;
	}

	FDropItem * di = new FDropItem;

	sc.MustGetToken(TK_StringConst);
	di->mobjtype = FindType(sc.string);
	di->Next = bag.DropItems;
	bag.DropItems = di;
}

//==========================================================================
//
//==========================================================================
static void ActorObituary(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetToken(TK_StringConst);
	// Property left in the spec as a courtesy to ports which define obituaries
}

//==========================================================================
//
//==========================================================================
static void ActorTranslation(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	if (sc.number < 0 || sc.number > 2)
	{
		sc.ErrorF("Translation must be in the range [0,2]");
	}
	defaults->flags &= MF_TRANSLATION;
	defaults->flags |= sc.number << MF_TRANSSHIFT;
}

//==========================================================================
//
//==========================================================================
static void ActorMonster(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	// sets the standard flag for a monster
	defaults->flags |= MF_SHOOTABLE | MF_COUNTKILL | MF_SOLID | MF_ISMONSTER;
}

//==========================================================================
//
//==========================================================================
static void ActorProjectile(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	// sets the standard flags for a projectile
	defaults->flags |= MF_NOBLOCKMAP | MF_NOGRAVITY | MF_DROPOFF | MF_MISSILE;
}

//==========================================================================
//
//==========================================================================
static void ActorMMChance(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetInteger();
	defaults->minmissilechance = sc.number;
}

//==========================================================================
//
//==========================================================================
static void ActorMeleeThreshold(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetFloat();
	defaults->meleethreshold = fixed_t(sc.decimal*FRACUNIT);
}

//==========================================================================
//
//==========================================================================
static void ActorMaxAttackRange(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	sc.MustGetFloat();
	defaults->maxattackrange = fixed_t(sc.decimal*FRACUNIT);
}

//==========================================================================
//
//==========================================================================
static void ActorFlagSetOrReset(Scanner &sc, mobjinfo_t *defaults, Baggage &bag)
{
	char mod = sc.string[0];
	flagdef * fd;

	sc.MustGetToken(TK_StringConst);

	if (fd = FindFlag(sc.string))
	{
		if (mod == '+') defaults->flags |= fd->flagbit;
		else defaults->flags &= ~fd->flagbit;
	}
	else sc.ErrorF("\"%s\" is an unknown flag\n", sc.string);
}

//==========================================================================
//
//==========================================================================
static const ActorProps *APropSearch(const char *str, const ActorProps *props, int numprops)
{
	int min = 0, max = numprops - 1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		int lexval = strcmp(str, props[mid].name);
		if (lexval == 0)
		{
			return &props[mid];
		}
		else if (lexval > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return NULL;
}

//==========================================================================
//
// all actor properties
//
//==========================================================================
#define apf ActorPropFunction
static const ActorProps *is_actorprop(const char *str)
{
	static const ActorProps props[] =
	{
		{ "+",							ActorFlagSetOrReset },
		{ "-",							ActorFlagSetOrReset },
		{ "activesound",				ActorActiveSound },
		{ "attacksound",				ActorAttackSound },
		{ "damage",						ActorDamage },
		{ "deathsound",					ActorDeathSound },
		{ "dropitem",					ActorDropItem },
		{ "health",						ActorHealth },
		{ "height",						ActorHeight },
		{ "hitobituary",				ActorObituary },
		{ "mass",						ActorMass },
		{ "maxattackrange",				ActorMaxAttackRange },
		{ "meleethreshold",				ActorMeleeThreshold },
		{ "minmissilechance",			ActorMMChance },
		{ "monster",					ActorMonster },
		{ "obituary",					ActorObituary },
		{ "painchance",					ActorPainChance },
		{ "painsound",					ActorPainSound },
		{ "projectile",					ActorProjectile },
		{ "radius",						ActorRadius },
		{ "reactiontime",				ActorReactionTime },
		{ "seesound",					ActorSeeSound },
		{ "speed",						ActorSpeed },
		//{ "states",						ActorStates },
	};
	return APropSearch(str, props, sizeof(props) / sizeof(ActorProps));
}

