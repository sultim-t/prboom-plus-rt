#ifdef COMPILE_VIDD

#include "vidd_prop.h"
#include "vidd_util.h"
#include "vidd.h"

//---------------------------------------------------------------------------
void bindToDeprecated(TQuickCodeBinding *qcb);

//---------------------------------------------------------------------------
#define ATTACHPROP(var, name) \
  if (forPlay) var = (TVIDDElementProperty)getStringAsInt( viddSys_getAttribute(VAT_GLOBAL, name) ); \
  else { \
    var = prop++; \
    viddSys_setAttribute(VAT_GLOBAL, name, getIntAsString((int)var)); \
  }

//---------------------------------------------------------------------------
#define ATTACHTYPE(var, name) \
  if (forPlay) var = (TVIDDElementType)getStringAsInt( viddSys_getAttribute(VAT_GLOBAL, name) ); \
  else { \
    var = type++; \
    viddSys_setAttribute(VAT_GLOBAL, name, getIntAsString((int)var)); \
  }
  
//---------------------------------------------------------------------------
void VIDD_attachQuickCodes(TQuickCodeBinding *qcb, int forPlay) {
  // the ViddSys library no longer has built-in enums for any element types
  // or properties. This allows customization of both, which are effected
  // using a string to describe the property or type, bound to an integer.
  // These bindings are stored in the global attribute table of the VIDD,
  // and can be recalled, matched, and assigned here. This has the upside
  // of creating human-readable properties and types so other applications
  // can better interpret what's stored in the VIDD. It also allows demos to
  // contain information for elements that aren't strictly necessary.
  TVIDDElementProperty prop = VEP_USER;
  TVIDDElementType type = VET_USER;
  
  if (!qcb) return;

  // latest quickCode version
  qcb->quickCodeVersion = 1;
  
  if (forPlay) {
    // see what version of the quickCode mapping this VIDD uses
    qcb->quickCodeVersion = getStringAsInt(viddSys_getAttribute(VAT_GLOBAL, "qc_version"));
    if (!qcb->quickCodeVersion) {
      // old VIDD that relied on the static (deprecated) ViddSys enums
      bindToDeprecated(qcb);
      return;
    }    
  } else {
    // recording, use the latest quickCode version
    viddSys_setAttribute(VAT_GLOBAL, "qc_version", getIntAsString(qcb->quickCodeVersion));
  }
  
  // bind the string description of each property/type to an integer
  // that facilitates much faster lookup when querying the ViddSys
  // about an element's status
  ATTACHTYPE( qcb->TYPE_MOBJ, "qc_type_mobj" )
  ATTACHTYPE( qcb->TYPE_SECTOR, "qc_type_sector" )
  ATTACHTYPE( qcb->TYPE_PLAYER, "qc_type_player" )
  ATTACHTYPE( qcb->TYPE_SIDE, "qc_type_side" )
  ATTACHTYPE( qcb->TYPE_GLOBAL, "qc_type_global" )

  ATTACHPROP( qcb->MOBJ_TYPE, "qc_mobj_type" )
  ATTACHPROP( qcb->MOBJ_POS, "qc_mobj_pos" )
  ATTACHPROP( qcb->MOBJ_SPRITE, "qc_mobj_sprite" )
  ATTACHPROP( qcb->MOBJ_STATE, "qc_mobj_state" )
  ATTACHPROP( qcb->MOBJ_ANGLE, "qc_mobj_angle" )
  
  ATTACHPROP( qcb->SECTOR_FLOORHEIGHT, "qc_sector_floorheight" )
  ATTACHPROP( qcb->SECTOR_CEILINGHEIGHT, "qc_sector_ceilingheight" )
  ATTACHPROP( qcb->SECTOR_LIGHTLEVEL, "qc_sector_lightlevel" )
  ATTACHPROP( qcb->SECTOR_FLOORPIC, "qc_sector_floorpic" )
  ATTACHPROP( qcb->SECTOR_CEILINGPIC, "qc_sector_ceilingpic" )

  ATTACHPROP( qcb->SIDE_TOPTEX, "qc_side_toptex" )
  ATTACHPROP( qcb->SIDE_BOTTOMTEX, "qc_side_bottomtex" )
  ATTACHPROP( qcb->SIDE_MIDTEX, "qc_side_midtex" )

  ATTACHPROP( qcb->PLAYER_VIEWZ, "qc_player_viewz" )
  ATTACHPROP( qcb->PLAYER_HEALTH, "qc_player_health" )
  ATTACHPROP( qcb->PLAYER_ARMORPOINTS, "qc_player_armorpoints" )
  ATTACHPROP( qcb->PLAYER_ARMORTYPE, "qc_player_armortype" )
  ATTACHPROP( qcb->PLAYER_BACKPACK, "qc_player_backpack" )
  ATTACHPROP( qcb->PLAYER_WEAPONS, "qc_player_weapons" )
  ATTACHPROP( qcb->PLAYER_CARDS, "qc_player_cards" )
  ATTACHPROP( qcb->PLAYER_POWER_BIOSUIT, "qc_player_power_biosuit" )
  ATTACHPROP( qcb->PLAYER_POWER_INVULN, "qc_player_power_invuln" )
  ATTACHPROP( qcb->PLAYER_POWER_INVIS, "qc_player_power_invis" )
  ATTACHPROP( qcb->PLAYER_POWER_ALLMAP, "qc_player_power_allmap" )
  ATTACHPROP( qcb->PLAYER_POWER_GOGGLES, "qc_player_power_goggles" )
  ATTACHPROP( qcb->PLAYER_POWER_BERSERK, "qc_player_power_berserk" )
  ATTACHPROP( qcb->PLAYER_KILLCOUNT, "qc_player_killcount" )
  ATTACHPROP( qcb->PLAYER_ITEMCOUNT, "qc_player_itemcount" )
  ATTACHPROP( qcb->PLAYER_SECRETCOUNT, "qc_player_secretcount" )
  ATTACHPROP( qcb->PLAYER_DAMAGECOUNT, "qc_player_damagecount" )
  ATTACHPROP( qcb->PLAYER_BONUSCOUNT, "qc_player_bonuscount" )
  ATTACHPROP( qcb->PLAYER_EXTRALIGHT, "qc_player_extralight" )
  ATTACHPROP( qcb->PLAYER_FIXEDCOLORMAP, "qc_player_fixedcolormap" )
  ATTACHPROP( qcb->PLAYER_COLORMAP, "qc_player_colormap" )
  ATTACHPROP( qcb->PLAYER_AMMO_CLIP, "qc_player_ammo_clip" )
  ATTACHPROP( qcb->PLAYER_AMMO_SHELL, "qc_player_ammo_shell" )
  ATTACHPROP( qcb->PLAYER_AMMO_MISSLE, "qc_player_ammo_missile" )
  ATTACHPROP( qcb->PLAYER_AMMO_CELL, "qc_player_ammo_cell" )
  ATTACHPROP( qcb->PLAYER_READYWEAPON, "qc_player_readyweapon" )
  ATTACHPROP( qcb->PLAYER_SCREENSPRITE0_STATE, "qc_player_screensprite0_state" )
  ATTACHPROP( qcb->PLAYER_SCREENSPRITE0_SX, "qc_player_screensprite0_sx" )
  ATTACHPROP( qcb->PLAYER_SCREENSPRITE0_SY, "qc_player_screensprite0_sy" )
  ATTACHPROP( qcb->PLAYER_SCREENSPRITE1_STATE, "qc_player_screensprite1_state" )
  ATTACHPROP( qcb->PLAYER_SCREENSPRITE1_SX, "qc_player_screensprite1_sx" )
  ATTACHPROP( qcb->PLAYER_SCREENSPRITE1_SY, "qc_player_screensprite1_sy" )
  ATTACHPROP( qcb->PLAYER_ATTACKERID, "qc_player_attackerid" )
  ATTACHPROP( qcb->PLAYER_MESSAGE, "qc_player_message" )
  
  ATTACHPROP( qcb->GLOBAL_TOTALSECRETS, "qc_global_totalsecrets" )
  ATTACHPROP( qcb->GLOBAL_TOTALITEMS, "qc_global_totalitems" )
  ATTACHPROP( qcb->GLOBAL_TOTALENEMIES, "qc_global_totalenemies")
}

//---------------------------------------------------------------------------
void bindToDeprecated(TQuickCodeBinding *qcb) {
  // old TVIDDElementProperty and TVIDDElementType enum values
  qcb->TYPE_MOBJ    = 1;
  qcb->TYPE_SECTOR  = 2;
  qcb->TYPE_PLAYER  = 3;
  qcb->TYPE_SIDE    = 4;
  qcb->TYPE_GLOBAL  = 5;
  
  qcb->MOBJ_TYPE = 100;
  qcb->MOBJ_POS = 101;
  qcb->MOBJ_SPRITE = 102;
  qcb->MOBJ_STATE = 103;
  qcb->MOBJ_ANGLE = 104;
  
  qcb->SECTOR_FLOORHEIGHT = 200;
  qcb->SECTOR_CEILINGHEIGHT = 201;
  qcb->SECTOR_LIGHTLEVEL = 202;
  qcb->SECTOR_FLOORPIC = 203;
  qcb->SECTOR_CEILINGPIC = 204;
  
  qcb->SIDE_TOPTEX = 300;
  qcb->SIDE_BOTTOMTEX = 301;
  qcb->SIDE_MIDTEX = 302;

  qcb->PLAYER_VIEWZ = 400;
  qcb->PLAYER_HEALTH = 401;
  qcb->PLAYER_ARMORPOINTS = 402;
  qcb->PLAYER_ARMORTYPE = 403;
  qcb->PLAYER_BACKPACK = 404;
  qcb->PLAYER_WEAPONS = 405;
  qcb->PLAYER_CARDS = 406;
  qcb->PLAYER_POWER_BIOSUIT = 407;
  qcb->PLAYER_POWER_INVULN = 408;
  qcb->PLAYER_POWER_INVIS = 409;
  qcb->PLAYER_POWER_ALLMAP = 410;
  qcb->PLAYER_POWER_GOGGLES = 411;
  qcb->PLAYER_POWER_BERSERK = 412;      
  qcb->PLAYER_KILLCOUNT = 413;
  qcb->PLAYER_ITEMCOUNT = 414;
  qcb->PLAYER_SECRETCOUNT = 415;
  qcb->PLAYER_DAMAGECOUNT = 416;
  qcb->PLAYER_BONUSCOUNT = 417;
  qcb->PLAYER_EXTRALIGHT = 418;
  qcb->PLAYER_FIXEDCOLORMAP = 419;
  qcb->PLAYER_COLORMAP = 420;
  qcb->PLAYER_AMMO_CLIP = 421;
  qcb->PLAYER_AMMO_SHELL = 422;
  qcb->PLAYER_AMMO_MISSLE = 423;
  qcb->PLAYER_AMMO_CELL = 424; 
  qcb->PLAYER_READYWEAPON = 425;        
  qcb->PLAYER_SCREENSPRITE0_STATE = 426;
  qcb->PLAYER_SCREENSPRITE0_SX = 427;
  qcb->PLAYER_SCREENSPRITE0_SY = 428;  
  qcb->PLAYER_SCREENSPRITE1_STATE = 429;
  qcb->PLAYER_SCREENSPRITE1_SX = 430;
  qcb->PLAYER_SCREENSPRITE1_SY = 431;
  qcb->PLAYER_ATTACKERID = 432;
  qcb->PLAYER_MESSAGE = 433;
  
  qcb->GLOBAL_TOTALSECRETS = 500;
  qcb->GLOBAL_TOTALITEMS = 501;
  qcb->GLOBAL_TOTALENEMIES = 502;
}


#endif // #ifdef COMPILE_VIDD
