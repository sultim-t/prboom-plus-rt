#ifndef __THINGDEF_H
#define __THINGDEF_H

extern "C"
{
#include "doomtype.h"
#include "info.h"
}

// move me!
struct FDropItem
{
	mobjtype_t mobjtype;
	FDropItem * Next;
};

extern TArray<FDropItem *> DropItemList;
extern TArray<int> StateParameters;

#define STATEPARAM_ID 0x12345678


#endif
