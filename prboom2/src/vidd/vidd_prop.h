#ifdef COMPILE_VIDD

#ifndef VIDD_PROP_H
#define VIDD_PROP_H

#include "../../ViddSys/ViddSys.h"

//---------------------------------------------------------------------------
// Element property codes. These must be bound to actual codes using 
// VIDD_attachQuickCodes()
//---------------------------------------------------------------------------
typedef struct {
  int quickCodeVersion;
  
  TVIDDElementType
  
  TYPE_MOBJ,
  TYPE_SECTOR,
  TYPE_PLAYER,
  TYPE_SIDE,
  TYPE_GLOBAL;
  
  TVIDDElementProperty
    
  MOBJ_TYPE,
  MOBJ_POS,
  MOBJ_SPRITE,
  MOBJ_STATE,
  MOBJ_ANGLE,
  
  SECTOR_FLOORHEIGHT,
  SECTOR_CEILINGHEIGHT,
  SECTOR_LIGHTLEVEL,
  SECTOR_FLOORPIC,
  SECTOR_CEILINGPIC,
  
  SIDE_TOPTEX,
  SIDE_BOTTOMTEX,
  SIDE_MIDTEX,

  PLAYER_VIEWZ,
  PLAYER_HEALTH,
  PLAYER_ARMORPOINTS,
  PLAYER_ARMORTYPE,
  PLAYER_BACKPACK,
  PLAYER_WEAPONS,
  PLAYER_CARDS,
  PLAYER_POWER_BIOSUIT,
  PLAYER_POWER_INVULN,
  PLAYER_POWER_INVIS,
  PLAYER_POWER_ALLMAP,
  PLAYER_POWER_GOGGLES,
  PLAYER_POWER_BERSERK,      
  PLAYER_KILLCOUNT,
  PLAYER_ITEMCOUNT,
  PLAYER_SECRETCOUNT,
  PLAYER_DAMAGECOUNT,
  PLAYER_BONUSCOUNT,
  PLAYER_EXTRALIGHT,
  PLAYER_FIXEDCOLORMAP,
  PLAYER_COLORMAP,
  PLAYER_AMMO_CLIP,
  PLAYER_AMMO_SHELL,
  PLAYER_AMMO_MISSLE,
  PLAYER_AMMO_CELL, 
  PLAYER_READYWEAPON,        
  PLAYER_SCREENSPRITE0_STATE,
  PLAYER_SCREENSPRITE0_SX,
  PLAYER_SCREENSPRITE0_SY,  
  PLAYER_SCREENSPRITE1_STATE,
  PLAYER_SCREENSPRITE1_SX,
  PLAYER_SCREENSPRITE1_SY,
  PLAYER_ATTACKERID,
  PLAYER_MESSAGE,
  
  GLOBAL_TOTALSECRETS,
  GLOBAL_TOTALITEMS,
  GLOBAL_TOTALENEMIES;
  
} TQuickCodeBinding;

//---------------------------------------------------------------------------
void VIDD_attachQuickCodes(TQuickCodeBinding *qcb, int forPlay);

//---------------------------------------------------------------------------
#endif

#endif // #ifdef COMPILE_VIDD
