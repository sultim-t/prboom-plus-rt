// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// ExtraData
//
// The be-all, end-all extension to the DOOM map format. Uses the
// libConfuse library like EDF.
// 
// ExtraData can extend mapthings, lines, and sectors with an
// arbitrary number of fields, with data provided in more or less
// any format. The use of a textual input language will forever
// remove any future problems caused by binary format limitations.
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "m_qstr.h"
#include "doomdef.h"
#include "d_io.h"
#include "p_info.h"
#include "p_mobj.h"
#include "p_spec.h"
#include "w_wad.h"
#include "i_system.h"
#include "d_dehtbl.h" // for dehflags parsing
#include "e_exdata.h"
#include "e_things.h"

#include "Confuse/confuse.h"

#include "e_lib.h"

// statics

static mapthingext_t *EDThings;
static unsigned int numEDMapThings;

#define NUMMTCHAINS 1021
static unsigned int mapthing_chains[NUMMTCHAINS];

static maplinedefext_t *EDLines;
static unsigned int numEDLines;

#define NUMLDCHAINS 1021
static unsigned int linedef_chains[NUMLDCHAINS];

// ExtraData section names
#define SEC_MAPTHING "mapthing"
#define SEC_LINEDEF  "linedef"

// ExtraData field names
// mapthing fields:
#define FIELD_NUM     "recordnum"
#define FIELD_TID     "tid"
#define FIELD_TYPE    "type"
#define FIELD_OPTIONS "options"
#define FIELD_ARGS    "args"

// linedef fields
#define FIELD_LINE_NUM       "recordnum"
#define FIELD_LINE_SPECIAL   "special"
#define FIELD_LINE_TAG       "tag"
#define FIELD_LINE_EXTFLAGS  "extflags"
#define FIELD_LINE_ARGS      "args"

// mapthing options and related data structures

static cfg_opt_t mapthing_opts[] =
{
   CFG_INT(FIELD_NUM,     0,  CFGF_NONE),
   CFG_INT(FIELD_TID,     0,  CFGF_NONE),
   CFG_STR(FIELD_TYPE,    "", CFGF_NONE),
   CFG_STR(FIELD_OPTIONS, "", CFGF_NONE),
   CFG_STR(FIELD_ARGS,    0,  CFGF_LIST),
   CFG_END()
};

// mapthing flag values and mnemonics

static dehflags_t mapthingflags[] =
{
   { "EASY",      MTF_EASY },
   { "NORMAL",    MTF_NORMAL },
   { "HARD",      MTF_HARD },
   { "AMBUSH",    MTF_AMBUSH },
   { "NOTSINGLE", MTF_NOTSINGLE },
   { "NOTDM",     MTF_NOTDM },
   { "NOTCOOP",   MTF_NOTCOOP },
   { "FRIEND",    MTF_FRIEND },
   { "DORMANT",   MTF_DORMANT },
   { NULL,        0 }
};

static dehflagset_t mt_flagset =
{
   mapthingflags, // flaglist
   0,             // mode
};

// linedef options and related data structures

// line special callback
static int E_LineSpecCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                        void *result);

static cfg_opt_t linedef_opts[] =
{
   CFG_INT(FIELD_LINE_NUM,        0,  CFGF_NONE),
   CFG_INT_CB(FIELD_LINE_SPECIAL, 0,  CFGF_NONE, E_LineSpecCB),
   CFG_INT(FIELD_LINE_TAG,        0,  CFGF_NONE),
   CFG_STR(FIELD_LINE_EXTFLAGS,   "", CFGF_NONE),
   CFG_STR(FIELD_LINE_ARGS,       0,  CFGF_LIST),
   CFG_END()
};

// linedef extended flag values and mnemonics

static dehflags_t extlineflags[] =
{
   { "CROSS",   EX_ML_CROSS },
   { "USE",     EX_ML_USE },
   { "IMPACT",  EX_ML_IMPACT },
   { "PUSH",    EX_ML_PUSH },
   { "PLAYER",  EX_ML_PLAYER },
   { "MONSTER", EX_ML_MONSTER },
   { "MISSILE", EX_ML_MISSILE },
   { "REPEAT",  EX_ML_REPEAT },
   { "1SONLY",  EX_ML_1SONLY },
   { NULL,      0 }
};

static dehflagset_t ld_flagset =
{
   extlineflags, // flaglist
   0,            // mode
};

//
// Line Special Information
//

static struct exlinespec
{
   short special;     // line special number
   const char *name;  // descriptive name
   unsigned int next; // for hashing
} exlinespecs[] =
{
   // Normal DOOM/BOOM extended specials.
   // Most of these have horrible names, but they won't be
   // used much via ExtraData, so it doesn't matter.
   {   0, "None" }, 
   {   1, "DR_RaiseDoor_Slow_Mon" }, 
   {   2, "W1_OpenDoor_Slow" },      
   {   3, "W1_CloseDoor_Slow" },
   {   4, "W1_RaiseDoor_Slow" },
   {   5, "W1_Floor_UpLnC_Slow" },
   {   6, "W1_StartCrusher_Fast" },
   {   7, "S1_Stairs_Up8_Slow" },
   {   8, "W1_Stairs_Up8_Slow" },
   {   9, "S1_Donut" },
   {  10, "W1_Plat_Lift_Slow" },
   {  11, "S1_ExitLevel" },
   {  12, "W1_Light_MaxNeighbor" },
   {  13, "W1_Light_Set255" },
   {  14, "S1_Plat_Up32_c0t_Slow" },
   {  15, "S1_Plat_Up24_cTt_Slow" },
   {  16, "W1_Door_CloseWait30" },
   {  17, "W1_Light_Blink" },
   {  18, "S1_Floor_UpNnF_Slow" },
   {  19, "W1_Floor_DnHnF_Slow" },
   {  20, "S1_Plat_UpNnF_c0t_Slow" },
   {  21, "S1_Plat_Lift_Slow" },
   {  22, "W1_Plat_UpNnF_c0t_Slow" },
   {  23, "S1_Floor_DnLnF_Slow" },
   {  24, "G1_Floor_UpLnC_Slow" },
   {  25, "W1_StartCrusher_Slow" },
   {  26, "DR_RaiseLockedDoor_Blue_Slow" },
   {  27, "DR_RaiseLockedDoor_Yellow_Slow" },
   {  28, "DR_RaiseLockedDoor_Red_Slow" },
   {  29, "S1_RaiseDoor_Slow" },
   {  30, "W1_Floor_UpsT_Slow" },
   {  31, "D1_OpenDoor_Slow" },
   {  32, "D1_OpenLockedDoor_Blue_Slow" },
   {  33, "D1_OpenLockedDoor_Red_Slow" },
   {  34, "D1_OpenLockedDoor_Yellow_Slow" },
   {  35, "W1_Light_Set35" },
   {  36, "W1_Floor_DnHnFp8_Fast" },
   {  37, "W1_Floor_DnLnF_cSn_Slow" },
   {  38, "W1_Floor_DnLnF_Slow" },
   {  39, "W1_TeleportToSpot" },
   {  40, "W1_Ceiling_UpHnC_Slow" },
   {  41, "S1_Ceiling_DnF_Fast" },
   {  42, "SR_CloseDoor_Slow" },
   {  43, "SR_Ceiling_DnF_Fast" },
   {  44, "W1_Ceiling_DnFp8_Slow" },
   {  45, "SR_Floor_DnHnF_Slow" },
   {  46, "GR_OpenDoor_Slow" },
   {  47, "G1_Plat_UpNnF_c0t_Slow" },
   {  48, "ScrollLeft" },
   {  49, "S1_StartCrusher_Slow" },
   {  50, "S1_CloseDoor_Slow" },
   {  51, "S1_ExitSecret" },
   {  52, "W1_ExitLevel" },
   {  53, "W1_Plat_LoHiPerpetual" },
   {  54, "W1_StopPlat" },
   {  55, "S1_Floor_UpLnCm8_Slow_Crush" },
   {  56, "W1_Floor_UpLnCm8_Slow_Crush" },
   {  57, "W1_StopCrusher" },
   {  58, "W1_Floor_Up24_Slow" },
   {  59, "W1_Floor_Up24_cSt_Slow" },
   {  60, "SR_Floor_DnLnF_Slow" },
   {  61, "SR_OpenDoor_Slow" },
   {  62, "SR_Plat_Lift_Slow" },
   {  63, "SR_RaiseDoor_Slow" },
   {  64, "SR_Floor_UpLnC_Slow" },
   {  65, "SR_Floor_UpLnCm8_Slow_Crush" },
   {  66, "SR_Plat_Up24_cTt_Slow" },
   {  67, "SR_Plat_Up32_c0t_Slow" },
   {  68, "SR_Plat_UpNnF_c0t_Slow" },
   {  69, "SR_Floor_UpNnF_Slow" },
   {  70, "SR_Floor_DnHnFp8_Fast" },
   {  71, "S1_Floor_DnHnFp8_Fast" },
   {  72, "WR_Ceiling_DnFp8_Slow" },
   {  73, "WR_StartCrusher_Slow" },
   {  74, "WR_StopCrusher" },
   {  75, "WR_CloseDoor_Slow" },
   {  76, "WR_Door_CloseWait30" },
   {  77, "WR_StartCrusher_Fast" },
   {  78, "SR_Floor_cSn" },
   {  79, "WR_Light_Set35" },
   {  80, "WR_Light_MaxNeighbor" },
   {  81, "WR_Light_Set255" },
   {  82, "WR_Floor_DnLnF_Slow" },
   {  83, "WR_Floor_DnHnF_Slow" },
   {  84, "WR_Floor_DnLnF_cSn_Slow" },
   {  85, "ScrollRight" },
   {  86, "WR_OpenDoor_Slow" },
   {  87, "WR_Plat_LoHiPerpetual" },
   {  88, "WR_Plat_Lift_Slow" },
   {  89, "WR_StopPlat" },
   {  90, "WR_RaiseDoor_Slow" },
   {  91, "WR_Floor_UpLnC_Slow" },
   {  92, "WR_Floor_Up24_Slow" },
   {  93, "WR_Floor_Up24_cSt_Slow" },
   {  94, "WR_Floor_UpLnCm8_Slow_Crush" },
   {  95, "WR_Plat_UpNnF_c0t_Slow" },
   {  96, "WR_Floor_UpsT_Slow" },
   {  97, "WR_TeleportToSpot" },
   {  98, "WR_Floor_DnHnFp8_Fast" },
   {  99, "WR_OpenLockedDoor_Blue_Fast" },
   { 100, "W1_Stairs_Up16_Fast" },
   { 101, "S1_Floor_UpLnC_Slow" },
   { 102, "S1_Floor_DnHnF_Slow" },
   { 103, "S1_OpenDoor_Slow" },
   { 104, "W1_Light_MinNeighbor" },
   { 105, "WR_RaiseDoor_Fast" },
   { 106, "WR_OpenDoor_Fast" },
   { 107, "WR_CloseDoor_Fast" },
   { 108, "W1_RaiseDoor_Fast" },
   { 109, "W1_OpenDoor_Fast" },
   { 110, "W1_CloseDoor_Fast" },
   { 111, "S1_RaiseDoor_Fast" },
   { 112, "S1_OpenDoor_Fast" },
   { 113, "S1_CloseDoor_Fast" },
   { 114, "SR_RaiseDoor_Fast" },
   { 115, "SR_OpenDoor_Fast" },
   { 116, "SR_CloseDoor_Fast" },
   { 117, "DR_RaiseDoor_Fast" },
   { 118, "D1_OpenDoor_Fast" },
   { 119, "W1_Floor_UpNnF_Slow" },
   { 120, "WR_Plat_Lift_Fast" },
   { 121, "W1_Plat_Lift_Fast" },
   { 122, "S1_Plat_Lift_Fast" },
   { 123, "SR_Plat_Lift_Fast" },
   { 124, "W1_ExitSecret" },
   { 125, "W1_TeleportToSpot_MonOnly" },
   { 126, "WR_TeleportToSpot_MonOnly" },
   { 127, "S1_Stairs_Up16_Fast" },
   { 128, "WR_Floor_UpNnF_Slow" },
   { 129, "WR_Floor_UpNnF_Fast" },
   { 130, "W1_Floor_UpNnF_Fast" },
   { 131, "S1_Floor_UpNnF_Fast" },
   { 132, "SR_Floor_UpNnF_Fast" },
   { 133, "S1_OpenLockedDoor_Blue_Fast" },
   { 134, "SR_OpenLockedDoor_Red_Fast" },
   { 135, "S1_OpenLockedDoor_Red_Fast" },
   { 136, "SR_OpenLockedDoor_Yellow_Fast" },
   { 137, "S1_OpenLockedDoor_Yellow_Fast" },
   { 138, "SR_Light_Set255" },
   { 139, "SR_Light_Set35" },
   { 140, "S1_Floor_Up512_Slow" },
   { 141, "W1_StartCrusher_Silent_Slow" },
   { 142, "W1_Floor_Up512_Slow" },
   { 143, "W1_Plat_Up24_cTt_Slow" },
   { 144, "W1_Plat_Up32_c0t_Slow" },
   { 145, "W1_Ceiling_DnF_Fast" },
   { 146, "W1_Donut" },
   { 147, "WR_Floor_Up512_Slow" },
   { 148, "WR_Plat_Up24_cTt_Slow" },
   { 149, "WR_Plat_Up32_c0t_Slow" },
   { 150, "WR_StartCrusher_Silent_Slow" },
   { 151, "WR_Ceiling_UpHnC_Slow" },
   { 152, "WR_Ceiling_DnF_Fast" },
   { 153, "W1_Floor_cSt" },
   { 154, "WR_Floor_cSt" },
   { 155, "WR_Donut" },
   { 156, "WR_Light_Blink" },
   { 157, "WR_Light_MinNeighbor" },
   { 158, "S1_Floor_UpsT_Slow" },
   { 159, "S1_Floor_DnLnF_cSn_Slow" },
   { 160, "S1_Floor_Up24_cSt_Slow" },
   { 161, "S1_Floor_Up24_Slow" },
   { 162, "S1_Plat_LoHiPerpetual" },
   { 163, "S1_StopPlat" },
   { 164, "S1_StartCrusher_Fast" },
   { 165, "S1_StartCrusher_Silent_Slow" },
   { 166, "S1_Ceiling_UpHnC_Slow" },
   { 167, "S1_Ceiling_DnFp8_Slow" },
   { 168, "S1_StopCrusher" },
   { 169, "S1_Light_MaxNeighbor" },
   { 170, "S1_Light_Set35" },
   { 171, "S1_Light_Set255" },
   { 172, "S1_Light_Blink" },
   { 173, "S1_Light_MinNeighbor" },
   { 174, "S1_TeleportToSpot" },
   { 175, "S1_Door_CloseWait30" },
   { 176, "SR_Floor_UpsT_Slow" },
   { 177, "SR_Floor_DnLnF_cSn_Slow" },
   { 178, "SR_Floor_Up512_Slow" },
   { 179, "SR_Floor_Up24_cSt_Slow" },
   { 180, "SR_Floor_Up24_Slow" },
   { 181, "SR_Plat_LoHiPerpetual" },
   { 182, "SR_StopPlat" },
   { 183, "SR_StartCrusher_Fast" },
   { 184, "SR_StartCrusher_Slow" },
   { 185, "SR_StartCrusher_Silent_Slow" },
   { 186, "SR_Ceiling_UpHnC_Slow" },
   { 187, "SR_Ceiling_DnFp8_Slow" },
   { 188, "SR_StopCrusher" },
   { 189, "S1_Floor_cSt" },
   { 190, "SR_Floor_cSt" },
   { 191, "SR_Donut" },
   { 192, "SR_Light_MaxNeighbor" },
   { 193, "SR_Light_Blink" },
   { 194, "SR_Light_MinNeighbor" },
   { 195, "SR_TeleportToSpot" },
   { 196, "SR_Door_CloseWait30" },
   { 197, "G1_ExitLevel" },
   { 198, "G1_ExitSecret" },
   { 199, "W1_Ceiling_DnLnC_Slow" },
   { 200, "W1_Ceiling_DnHnF_Slow" },
   { 201, "WR_Ceiling_DnLnC_Slow" },
   { 202, "WR_Ceiling_DnHnF_Slow" },
   { 203, "S1_Ceiling_DnLnC_Slow" },
   { 204, "S1_Ceiling_DnHnF_Slow" },
   { 205, "SR_Ceiling_DnLnC_Slow" },
   { 206, "SR_Ceiling_DnHnF_Slow" },
   { 207, "W1_TeleportToSpot_Orient_Silent" },
   { 208, "WR_TeleportToSpot_Orient_Silent" },
   { 209, "S1_TeleportToSpot_Orient_Silent" },
   { 210, "SR_TeleportToSpot_Orient_Silent" },
   { 211, "SR_Plat_CeilingToggle_Instant" },
   { 212, "WR_Plat_CeilingToggle_Instant" },
   { 213, "TransferLight_Floor" },
   { 214, "ScrollCeiling_Accel" },
   { 215, "ScrollFloor_Accel" },
   { 216, "CarryObjects_Accel" },
   { 217, "ScrollFloorCarryObjects_Accel" },
   { 218, "ScrollWallWithFlat_Accel" },
   { 219, "W1_Floor_DnNnF_Slow" },
   { 220, "WR_Floor_DnNnF_Slow" },
   { 221, "S1_Floor_DnNnF_Slow" },
   { 222, "SR_Floor_DnNnF_Slow" },
   { 223, "TransferFriction" },
   { 224, "TransferWind" },
   { 225, "TransferCurrent" },
   { 226, "TransferPointForce" },
   { 227, "W1_Elevator_NextHi" },
   { 228, "WR_Elevator_NextHi" },
   { 229, "S1_Elevator_NextHi" },
   { 230, "SR_Elevator_NextHi" },
   { 231, "W1_Elevator_NextLo" },
   { 232, "WR_Elevator_NextLo" },
   { 233, "S1_Elevator_NextLo" },
   { 234, "SR_Elevator_NextLo" },
   { 235, "W1_Elevator_Current" },
   { 236, "WR_Elevator_Current" },
   { 237, "S1_Elevator_Current" },
   { 238, "SR_Elevator_Current" },
   { 239, "W1_Floor_cSn" },
   { 240, "WR_Floor_cSn" },
   { 241, "S1_Floor_cSn" },
   { 242, "TransferHeights" },
   { 243, "W1_TeleportToLine" },
   { 244, "WR_TeleportToLine" },
   { 245, "ScrollCeiling_ByHeight" },
   { 246, "ScrollFloor_ByHeight" },
   { 247, "CarryObjects_ByHeight" },
   { 248, "ScrollFloorCarryObjects_ByHeight" },
   { 249, "ScrollWallWithFlat_ByHeight" },
   { 250, "ScrollCeiling" },
   { 251, "ScrollFloor" },
   { 252, "CarryObjects" },
   { 253, "ScrollFloorCarryObjects" },
   { 254, "ScrollWallWithFlat" },
   { 255, "ScrollWallByOffsets" },
   { 256, "WR_Stairs_Up8_Slow" },
   { 257, "WR_Stairs_Up16_Fast" },
   { 258, "SR_Stairs_Up8_Slow" },
   { 259, "SR_Stairs_Up16_Fast" },
   { 260, "Translucent" },
   { 261, "TransferLight_Ceiling" },
   { 262, "W1_TeleportToLine_Reverse" },
   { 263, "WR_TeleportToLine_Reverse" },
   { 264, "W1_TeleportToLine_Reverse_MonOnly" },
   { 265, "WR_TeleportToLine_Reverse_MonOnly" },
   { 266, "W1_TeleportToLine_MonOnly" },
   { 267, "WR_TeleportToLine_MonOnly" },
   { 268, "W1_TeleportToSpot_Silent_MonOnly" },
   { 269, "WR_TeleportToSpot_Silent_MonOnly" },
   { 270, "ExtraDataSpecial" },
   { 271, "TransferSky" },
   { 272, "TransferSkyFlipped" },
   { 273, "WR_StartScript_1S" },
   { 274, "W1_StartScript" },
   { 275, "W1_StartScript_1S" },
   { 276, "SR_StartScript" },
   { 277, "S1_StartScript" },
   { 278, "GR_StartScript" },
   { 279, "G1_StartScript" },
   { 280, "WR_StartScript" },
   { 281, "3DMidTex_MoveWithFloor" },
   { 282, "3DMidTex_MoveWithCeiling" },
   { 283, "Portal_PlaneCeiling" },
   { 284, "Portal_PlaneFloor" },
   { 285, "Portal_PlaneFloorCeiling" },
   { 286, "Portal_HorizonCeiling" },
   { 287, "Portal_HorizonFloor" },
   { 288, "Portal_HorizonFloorCeiling" },
   { 289, "Portal_LineTransfer" },
   { 290, "Portal_SkyboxCeiling" },
   { 291, "Portal_SkyboxFloor" },
   { 292, "Portal_SkyboxFloorCeiling" },
   { 293, "TransferHereticWind" },
   { 294, "TransferHereticCurrent" },
   { 295, "Portal_AnchoredCeiling" },
   { 296, "Portal_AnchoredFloor" },
   { 297, "Portal_AnchoredFloorCeiling" },
   { 298, "Portal_AnchorLine" },
   { 299, "Portal_AnchorLineFloor" },

   // Parameterized specials.
   // These have nice names because their specifics come from ExtraData 
   // line arguments, ala Hexen, instead of being hardcoded.
   { 300, "Door_Raise" },
   { 301, "Door_Open" },
   { 302, "Door_Close" },
   { 303, "Door_CloseWaitOpen" },
   { 304, "Door_WaitRaise" },
   { 305, "Door_WaitClose" },
   { 306, "Floor_RaiseToHighest" },
   { 307, "Floor_LowerToHighest" },
   { 308, "Floor_RaiseToLowest" },
   { 309, "Floor_LowerToLowest" },
   { 310, "Floor_RaiseToNearest" },
   { 311, "Floor_LowerToNearest" },
   { 312, "Floor_RaiseToLowestCeiling" },
   { 313, "Floor_LowerToLowestCeiling" },
   { 314, "Floor_RaiseToCeiling" },
   { 315, "Floor_RaiseByTexture" },
   { 316, "Floor_LowerByTexture" },
   { 317, "Floor_RaiseByValue" },
   { 318, "Floor_LowerByValue" },
   { 319, "Floor_MoveToValue" },
   { 320, "Floor_RaiseInstant" },
   { 321, "Floor_LowerInstant" },
};

#define NUMLINESPECS (sizeof(exlinespecs) / sizeof(struct exlinespec))

// primary ExtraData options table

static cfg_opt_t ed_opts[] =
{
   CFG_SEC(SEC_MAPTHING, mapthing_opts, CFGF_MULTI|CFGF_NOCASE),
   CFG_SEC(SEC_LINEDEF,  linedef_opts,  CFGF_MULTI|CFGF_NOCASE),
   CFG_END()
};

//
// Shared Processing Routines
//

//
// E_ParseArg
//
// Parses a single element of an args list.
//
static void E_ParseArg(const char *str, long *dest)
{
   // currently only integers are supported
   *dest = strtol(str, NULL, 0);
}

//
// Begin Mapthing Routines
//

//
// E_EDThingForRecordNum
//
// Returns an index into EDThings for the given record number.
// Returns numEDMapThings if no such record exists.
//
static unsigned int E_EDThingForRecordNum(int recnum)
{
   unsigned int num;
   int key = recnum % NUMMTCHAINS;

   num = mapthing_chains[key];
   while(num != numEDMapThings && EDThings[num].recordnum != recnum)
   {
      num = EDThings[num].next;
   }

   return num;
}

//
// E_ParseTypeField
//
// Parses thing type fields in ExtraData. Allows resolving of
// EDF thingtype mnemonics to their corresponding doomednums.
//
static int E_ParseTypeField(char *value)
{
   int i;
   char prefix[16];
   char *colonloc, *strval;

   memset(prefix, 0, 16);

   colonloc = E_ExtractPrefix(value, prefix, 16);

   if(colonloc)
   {
      strval = colonloc + 1;

      if(!strcasecmp(prefix, "thing"))
      {
         // translate from EDF mnemonic to doomednum
         int type = E_SafeThingName(strval);
         int num = mobjinfo[type].doomednum;
         
         // don't return -1, use no-op zero in that case
         // (this'll work even if somebody messed with 'Unknown')
         return (num >= 0 ? num : 0);
      }
 
      // invalid prefix
      I_Error("E_ParseTypeField: invalid prefix %s\n", prefix);
      return 0;
   }
   else
   {
      // integer value
      i = strtol(value, NULL, 0);
      return (i >= 0 ? i : 0);
   }
}

//
// E_ParseThingArgs
//
// Parses the mapthing args list.
//
static void E_ParseThingArgs(mapthingext_t *mte, cfg_t *sec)
{
   unsigned int i, numargs;

   // count number of args given in list
   numargs = cfg_size(sec, FIELD_ARGS);
   
   // init all args to 0
   for(i = 0; i < 5; ++i) // hard-coded args max
      mte->args[i] = 0;
   
   // parse the given args values
   for(i = 0; i < numargs && i < 5; ++i) // hard-coded args max
   {
      const char *argstr = cfg_getnstr(sec, FIELD_ARGS, i);
      E_ParseArg(argstr, &(mte->args[i]));
   }
}

//
// E_ProcessEDThings
//
// Allocates and processes ExtraData mapthing records.
//
static void E_ProcessEDThings(cfg_t *cfg)
{
   unsigned int i;

   // get the number of mapthing records
   numEDMapThings = cfg_size(cfg, SEC_MAPTHING);

   // if none, we're done
   if(!numEDMapThings)
      return;

   // allocate the mapthingext_t structures
   EDThings = Z_Malloc(numEDMapThings * sizeof(mapthingext_t),
                       PU_LEVEL, NULL);

   // initialize the hash chains
   for(i = 0; i < NUMMTCHAINS; i++)
      mapthing_chains[i] = numEDMapThings;

   // read fields
   for(i = 0; i < numEDMapThings; i++)
   {
      cfg_t *thingsec;
      char *tempstr;
      int tempint;

      thingsec = cfg_getnsec(cfg, SEC_MAPTHING, i);

      // get the record number
      tempint = EDThings[i].recordnum = cfg_getint(thingsec, FIELD_NUM);

      // guard against duplicate record numbers
      if(E_EDThingForRecordNum(tempint) != numEDMapThings)
         I_Error("E_ProcessEDThings: duplicate record number %d\n", tempint);

      // hash this ExtraData mapthing record by its recordnum field
      tempint = EDThings[i].recordnum % NUMMTCHAINS;
      EDThings[i].next = mapthing_chains[tempint];
      mapthing_chains[tempint] = i;

      // standard fields

      // type
      tempstr = cfg_getstr(thingsec, FIELD_TYPE);
      EDThings[i].stdfields.type = (short)(E_ParseTypeField(tempstr));

      // it is not allowed to spawn an ExtraData control object via
      // ExtraData, but the error is tolerated by changing it to an 
      // "Unknown" thing
      if(EDThings[i].stdfields.type == ED_CTRL_DOOMEDNUM)
         EDThings[i].stdfields.type = mobjinfo[UnknownThingType].doomednum;

      // options
      tempstr = cfg_getstr(thingsec, FIELD_OPTIONS);
      if(*tempstr == '\0')
         EDThings[i].stdfields.options = 0;
      else
         EDThings[i].stdfields.options = (short)(E_ParseFlags(tempstr, &mt_flagset));

      // extended fields

      // get TID field
      EDThings[i].tid = (unsigned short)cfg_getint(thingsec, FIELD_TID);

      // get args
      E_ParseThingArgs(&EDThings[i], thingsec);

      // TODO: any other new fields
   }
}

//
// Begin Linedef Routines
//

//
// E_EDLineForRecordNum
//
// Returns an index into EDLines for the given record number.
// Returns numEDLines if no such record exists.
//
static unsigned int E_EDLineForRecordNum(int recnum)
{
   unsigned int num;
   int key = recnum % NUMLDCHAINS;

   num = linedef_chains[key];
   while(num != numEDLines && EDLines[num].recordnum != recnum)
   {
      num = EDLines[num].next;
   }

   return num;
}

#define NUMLSPECCHAINS 307
static unsigned int linespec_chains[NUMLSPECCHAINS];

//
// E_InitLineSpecHash
//
// Sets up a threaded hash table through the exlinespecs array to
// avoid untimely name -> number resolution.
//
static void E_InitLineSpecHash(void)
{
   unsigned int i;

   // init all chains
   for(i = 0; i < NUMLSPECCHAINS; ++i)
      linespec_chains[i] = NUMLINESPECS;

   // add all specials to hash table
   for(i = 0; i < NUMLINESPECS; ++i)
   {
      unsigned int key = D_HashTableKey(exlinespecs[i].name) % NUMLSPECCHAINS;
      exlinespecs[i].next = linespec_chains[key];
      linespec_chains[key] = i;
   }
}

//
// E_LineSpecForName
//
// Gets a line special for a name.
//
static short E_LineSpecForName(const char *name)
{
   unsigned int key = D_HashTableKey(name) % NUMLSPECCHAINS;
   unsigned int i   = linespec_chains[key];

   while(i != NUMLINESPECS && strcasecmp(name, exlinespecs[i].name))
      i = exlinespecs[i].next;

   return (i != NUMLINESPECS ? exlinespecs[i].special : 0);
}

//
// E_GenTypeForName
//
// Returns a base generalized line type for a name.
//
static short E_GenTypeForName(const char *name)
{
   short i = 0;
   static const char *names[] =
   {
      "GenFloor", "GenCeiling", "GenDoor", "GenLockedDoor",
      "GenLift", "GenStairs", "GenCrusher", NULL
   };
   static short bases[] =
   {
      GenFloorBase, GenCeilingBase, GenDoorBase, GenLockedBase,
      GenLiftBase, GenStairsBase, GenCrusherBase, 0
   };

   while(names[i] && strcasecmp(name, names[i]))
      ++i;

   return bases[i];
}

//
// E_GenTokenizer
//
// A lexer function for generalized specials.
//
static const char *E_GenTokenizer(const char *text, int *index, qstring_t *token)
{
   char c;
   int state = 0;

   // if we're already at the end, return NULL
   if(text[*index] == '\0')
      return NULL;

   M_QStrClear(token);

   while((c = text[*index]) != '\0')
   {
      *index += 1;
      switch(state)
      {
      case 0: // default state
         switch(c)
         {
         case ')':     // ignore closing bracket
         case ' ':
         case '\t':    // ignore whitespace
            continue;
         case '"':
            state = 1; // enter quoted part
            continue;
         case '\'':
            state = 2; // enter quoted part (single quote support)
            continue;
         case '(':     // end of current token
         case ',':
            return M_QStrBuffer(token);
         default:      // everything else == part of value
            M_QStrPutc(token, c);
            continue;
         }
      case 1: // in quoted area (double quotes)
         if(c == '"') // end of quoted area
            state = 0;
         else
            M_QStrPutc(token, c); // everything inside is literal
         continue;
      case 2: // in quoted area (single quotes)
         if(c == '\'') // end of quoted area
            state = 0;
         else
            M_QStrPutc(token, c); // everything inside is literal
         continue;
      default:
         I_Error("E_GenTokenizer: internal error - undefined lexer state\n");
      }
   }

   // return final token, next call will return NULL
   return M_QStrBuffer(token);
}

//
// E_BooleanArg
// 
// Parses a yes/no generalized type argument.
//
static boolean E_BooleanArg(const char *str)
{
   if(!strcasecmp(str, "yes"))
      return true;
   
   return false;
}

//
// E_SpeedArg
//
// Parses a generalized speed argument.
//
static short E_SpeedArg(const char *str)
{
   short i = 0;
   static const char *speeds[] =
   {
      "slow", "normal", "fast", "turbo", NULL
   };

   while(speeds[i] && strcasecmp(str, speeds[i]))
      ++i;

   return (speeds[i] ? i : 0);
}

//
// E_ChangeArg
//
// Parses a floor/ceiling changer type argument.
//
static short E_ChangeArg(const char *str)
{
   short i = 0;
   static const char *changes[] =
   {
      "none", "zero", "texture", "texturetype", NULL
   };

   while(changes[i] && strcasecmp(str, changes[i]))
      ++i;

   return (changes[i] ? i : 0);
}

//
// E_ModelArg
//
// Parses a floor/ceiling changer model argument.
//
static short E_ModelArg(const char *str)
{
   if(!strcasecmp(str, "numeric"))
      return 1;

   // trigger = 0
   return 0;
}

//
// E_FloorTarget
//
// Parses the target argument for a generalized floor type.
//
static short E_FloorTarget(const char *str)
{
   short i = 0;
   static const char *targs[] =
   {
      "HnF", "LnF", "NnF", "LnC", "C", "sT", "24", "32", NULL
   };

   while(targs[i] && strcasecmp(str, targs[i]))
      ++i;

   return (targs[i] ? i : 0);
}

//
// E_CeilingTarget
//
// Parses the target argument for a generalized ceiling type.
//
static short E_CeilingTarget(const char *str)
{
   short i = 0;
   static const char *targs[] =
   {
      "HnC", "LnC", "NnC", "HnF", "F", "sT", "24", "32", NULL
   };

   while(targs[i] && strcasecmp(str, targs[i]))
      ++i;

   return (targs[i] ? i : 0);
}

//
// E_DirArg
//
// Parses a direction argument.
//
static short E_DirArg(const char *str)
{
   if(!strcasecmp(str, "up"))
      return 1;

   // down = 0
   return 0;
}

//
// E_DoorTypeArg
//
// Parses a generalized door type argument.
//
static short E_DoorTypeArg(const char *str)
{
   if(!strcasecmp(str, "Open"))
      return 1;
   else if(!strcasecmp(str, "CloseWaitOpen"))
      return 2;
   else if(!strcasecmp(str, "Close"))
      return 3;

   // OpenWaitClose = 0
   return 0;
}

//
// E_DoorDelay
//
// Parses a generalized door delay argument.
//
static short E_DoorDelay(const char *str)
{
   if(*str == '4')
      return 1;
   else if(*str == '9')
      return 2;
   else if(!strcasecmp(str, "30"))
      return 3;

   // "1" = 0
   return 0;
}

//
// E_LockedType
//
// Parses a generalized locked door type argument.
//
static short E_LockedType(const char *str)
{
   if(!strcasecmp(str, "Open"))
      return 1;

   // OpenWaitClose = 0
   return 0;
}

//
// E_LockedKey
//
// Parses the key argument for a locked door.
//
static short E_LockedKey(const char *str)
{
   short i = 0;
   static const char *keys[] =
   {
      "Any", "RedCard", "BlueCard", "YellowCard", "RedSkull", "BlueSkull",
      "YellowSkull", "All", NULL
   };

   while(keys[i] && strcasecmp(str, keys[i]))
      ++i;

   return (keys[i] ? i : 0);
}

//
// E_LiftTarget
//
// Parses the target argument for a generalized lift.
//
static short E_LiftTarget(const char *str)
{
   if(!strcasecmp(str, "NnF"))
      return 1;
   else if(!strcasecmp(str, "LnC"))
      return 2;
   else if(!strcasecmp(str, "Perpetual"))
      return 3;

   // LnF = 0
   return 0;
}

//
// E_LiftDelay
//
// Parses the delay argument for a generalized lift.
//
static short E_LiftDelay(const char *str)
{
   if(*str == '3')
      return 1;
   else if(*str == '5')
      return 2;
   else if(!strcasecmp(str, "10"))
      return 3;

   // "1" = 0
   return 0;
}

//
// E_StepSize
//
// Parses the step size argument for generalized stairs.
//
static short E_StepSize(const char *str)
{
   if(*str == '8')
      return 1;
   else if(!strcasecmp(str, "16"))
      return 2;
   else if(!strcasecmp(str, "24"))
      return 3;

   // "4" = 0
   return 0;
}

//
// E_GenTrigger
//
// Parses the trigger type of a generalized line special.
//
static short E_GenTrigger(const char *str)
{
   short i = 0;
   static const char *trigs[] =
   {
      "W1", "WR", "S1", "SR", "G1", "GR", "D1", "DR", NULL
   };

   while(trigs[i] && strcasecmp(str, trigs[i]))
      ++i;

   return (trigs[i] ? i : 0);
}

// macro for E_ProcessGenSpec

// NEXTTOKEN: calls E_GenTokenizer to get the next token

#define NEXTTOKEN() \
   curtoken = E_GenTokenizer(value, &tok_index, &buffer); \
   if(!curtoken) \
      I_Error("E_ProcessGenSpec: bad generalized line special\n")

//
// E_ProcessGenSpec
//
// Parses a generalized line special string, with the following format:
//
// 'Gen'<type>'('<arg>+')'
//
// <type> = Floor|Ceiling|Door|LockedDoor|Lift|Stairs|Crusher
// <arg>  = <char>+','
//
// Whitespace between args is ignored.
//
// The number and type of arguments are fixed for a given special
// type, and all must be present. This is provided only for completeness,
// and I do not anticipate that it will actually be used, since
// new parameterized specials more or less obsolete the BOOM generalized
// line system. Hence, not a lot of time has been spent making it
// fast or clean.
//
static short E_ProcessGenSpec(const char *value)
{
   qstring_t buffer;
   const char *curtoken = NULL;
   int t, forc = 0, tok_index = 0;
   short trigger;

   // first things first, we have to initialize the qstring
   M_QStrInitCreate(&buffer);

   // get special name (starts at beginning, ends at '[')
   // and convert to base trigger type
   NEXTTOKEN();
   trigger = E_GenTypeForName(curtoken);

   switch(trigger)
   {
   case GenFloorBase:
      forc = 1;
      // fall through
   case GenCeilingBase:
      NEXTTOKEN(); // get crushing value
      trigger += (E_BooleanArg(curtoken) << FloorCrushShift);
      NEXTTOKEN(); // get speed
      trigger += (E_SpeedArg(curtoken) << FloorSpeedShift);
      NEXTTOKEN(); // get change type
      trigger += ((t = E_ChangeArg(curtoken)) << FloorChangeShift);
      NEXTTOKEN(); // get change type or monster allowed
      if(t)
         trigger += (E_ModelArg(curtoken) << FloorModelShift);
      else
         trigger += (E_BooleanArg(curtoken) << FloorModelShift);
      NEXTTOKEN(); // get target
      if(forc)
         trigger += (E_FloorTarget(curtoken) << FloorTargetShift);
      else
         trigger += (E_CeilingTarget(curtoken) << FloorTargetShift);
      NEXTTOKEN(); // get direction
      trigger += (E_DirArg(curtoken) << FloorDirectionShift);
      break;
   case GenDoorBase:
      NEXTTOKEN(); // get kind
      trigger += (E_DoorTypeArg(curtoken) << DoorKindShift);
      NEXTTOKEN(); // get speed
      trigger += (E_SpeedArg(curtoken) << DoorSpeedShift);
      NEXTTOKEN(); // get monster
      trigger += (E_BooleanArg(curtoken) << DoorMonsterShift);
      NEXTTOKEN(); // get delay
      trigger += (E_DoorDelay(curtoken) << DoorDelayShift);
      break;
   case GenLockedBase:
      NEXTTOKEN(); // get kind
      trigger += (E_LockedType(curtoken) << LockedKindShift);
      NEXTTOKEN(); // get speed
      trigger += (E_SpeedArg(curtoken) << LockedSpeedShift);
      NEXTTOKEN(); // get key
      trigger += (E_LockedKey(curtoken) << LockedKeyShift);
      NEXTTOKEN(); // get skull/key same
      trigger += (E_BooleanArg(curtoken) << LockedNKeysShift);
      break;
   case GenLiftBase:
      NEXTTOKEN(); // get target
      trigger += (E_LiftTarget(curtoken) << LiftTargetShift);
      NEXTTOKEN(); // get delay
      trigger += (E_LiftDelay(curtoken) << LiftDelayShift);
      NEXTTOKEN(); // get monster
      trigger += (E_BooleanArg(curtoken) << LiftMonsterShift);
      NEXTTOKEN(); // get speed
      trigger += (E_SpeedArg(curtoken) << LiftSpeedShift);
      break;
   case GenStairsBase:
      NEXTTOKEN(); // get direction
      trigger += (E_DirArg(curtoken) << StairDirectionShift);
      NEXTTOKEN(); // get step size
      trigger += (E_StepSize(curtoken) << StairStepShift);
      NEXTTOKEN(); // get ignore texture
      trigger += (E_BooleanArg(curtoken) << StairIgnoreShift);
      NEXTTOKEN(); // get monster
      trigger += (E_BooleanArg(curtoken) << StairMonsterShift);
      NEXTTOKEN(); // get speed
      trigger += (E_SpeedArg(curtoken) << StairSpeedShift);
      break;
   case GenCrusherBase:
      NEXTTOKEN(); // get silent
      trigger += (E_BooleanArg(curtoken) << CrusherSilentShift);
      NEXTTOKEN(); // get monster
      trigger += (E_BooleanArg(curtoken) << CrusherMonsterShift);
      NEXTTOKEN(); // get speed
      trigger += (E_SpeedArg(curtoken) << CrusherSpeedShift);
      break;
   default:
      break;
   }

   // for all: get trigger type
   NEXTTOKEN();
   trigger += (E_GenTrigger(curtoken) << TriggerTypeShift);

   // free the qstring
   M_QStrFree(&buffer);

   return trigger;
}

#undef NEXTTOKEN

//
// E_LineSpecCB
//
// libConfuse value-parsing callback function for the linedef special
// field. This function can resolve normal and generalized special
// names and accepts special numbers, too.
//
// Normal and parameterized specials are stored in the exlinespecs
// hash table and are resolved via name lookup.
//
// Generalized specials use a functional syntax which is parsed by
// E_ProcessGenSpec and E_GenTokenizer above. It is rather complicated
// and probably not that useful, but is provided for completeness.
//
// Raw special numbers are not checked for validity, but invalid
// names are turned into zero specials. Malformed generalized specials
// will cause errors, but bad argument values are zeroed.
//
static int E_LineSpecCB(cfg_t *cfg, cfg_opt_t *opt, const char *value,
                        void *result)
{
   long num;
   char *endptr;

   num = strtol(value, &endptr, 0);

   // check if value is a number or not
   if(*endptr != '\0')
   {
      // value is a special name
      static boolean hash_init = false;
      char *bracket_loc = strchr(value, '(');

      // if it has a parenthesis, it's a generalized type
      if(bracket_loc)
         *(long *)result = E_ProcessGenSpec(value);
      else
      {
         // initialize line specials hash first time
         if(!hash_init)
         {
            E_InitLineSpecHash();
            hash_init = true;
         }
         
         *(long *)result = (long)(E_LineSpecForName(value));
      }
   }
   else
   {
      // value is a number
      if(errno == ERANGE)
      {
         if(cfg)
         {
            cfg_error(cfg,
                      "integer value for option '%s' is out of range",
                      opt->name);
         }
         return -1;
      }

      *(long *)result = num;
   }

   return 0;
}

//
// E_ParseLineArgs
//
// Parses the linedef args list.
//
static void E_ParseLineArgs(maplinedefext_t *mlde, cfg_t *sec)
{
   unsigned int i, numargs;

   // count number of args given in list
   numargs = cfg_size(sec, FIELD_LINE_ARGS);
   
   // init all args to 0
   for(i = 0; i < 5; ++i) // hard-coded args max
      mlde->args[i] = 0;
   
   // parse the given args values
   for(i = 0; i < numargs && i < 5; ++i) // hard-coded args max
   {
      const char *argstr = cfg_getnstr(sec, FIELD_LINE_ARGS, i);
      E_ParseArg(argstr, &(mlde->args[i]));
   }
}

//
// E_ProcessEDLines
//
// Allocates and processes ExtraData linedef records.
//
static void E_ProcessEDLines(cfg_t *cfg)
{
   unsigned int i;

   // get the number of mapthing records
   numEDLines = cfg_size(cfg, SEC_LINEDEF);

   // if none, we're done
   if(!numEDLines)
      return;

   // allocate the maplinedefext_t structures
   EDLines = Z_Malloc(numEDLines * sizeof(maplinedefext_t),
                      PU_LEVEL, NULL);

   // initialize the hash chains
   for(i = 0; i < NUMLDCHAINS; ++i)
      linedef_chains[i] = numEDLines;

   // read fields
   for(i = 0; i < numEDLines; ++i)
   {
      cfg_t *linesec;
      const char *tempstr;
      int tempint;

      linesec = cfg_getnsec(cfg, SEC_LINEDEF, i);

      // get the record number
      tempint = EDLines[i].recordnum = cfg_getint(linesec, FIELD_LINE_NUM);

      // guard against duplicate record numbers
      if(E_EDLineForRecordNum(tempint) != numEDLines)
         I_Error("E_ProcessEDLines: duplicate record number %d\n", tempint);

      // hash this ExtraData linedef record by its recordnum field
      tempint = EDLines[i].recordnum % NUMLDCHAINS;
      EDLines[i].next = linedef_chains[tempint];
      linedef_chains[tempint] = i;

      // standard fields

      // special
      EDLines[i].stdfields.special = (short)cfg_getint(linesec, FIELD_LINE_SPECIAL);

      // tag
      EDLines[i].stdfields.tag = (short)cfg_getint(linesec, FIELD_LINE_TAG);

      // extflags
      tempstr = cfg_getstr(linesec, FIELD_LINE_EXTFLAGS);
      if(*tempstr == '\0')
         EDLines[i].extflags = 0;
      else
         EDLines[i].extflags = E_ParseFlags(tempstr, &ld_flagset);

      // args
      E_ParseLineArgs(&EDLines[i], linesec);

      // TODO: any other new fields
   }
}

//
// Global Routines
//

//
// E_LoadExtraData
//
// Loads the ExtraData lump for the level, if it has one
//
void E_LoadExtraData(void)
{
   cfg_t *cfg;

   // reset ExtraData variables (allocations are at PU_LEVEL
   // cache level, so anything from any earlier level has been
   // freed)
   
   EDThings = NULL;
   numEDMapThings = 0;
   
   EDLines = NULL;
   numEDLines = 0;

   // check to see if the ExtraData lump is defined by MapInfo
   if(!LevelInfo.extraData)
      return;

   // initialize the cfg
   cfg = cfg_init(ed_opts, CFGF_NOCASE);
   cfg_set_error_function(cfg, E_ErrorCB);

   // load and parse the ED lump
   if(cfg_parselump(cfg, LevelInfo.extraData))
      I_Error("E_LoadExtraData: Error finding or parsing lump\n");

   // processing

   // load mapthings
   E_ProcessEDThings(cfg);

   // load lines
   E_ProcessEDLines(cfg);

   // TODO: more processing

   // free the cfg
   cfg_free(cfg);
}

//
// E_SpawnMapThingExt
//
// Called by P_SpawnMapThing when an ExtraData control point
// (doomednum 5004) is encountered.  This function recursively
// calls P_SpawnMapThing with the new basic mapthing data from
// the corresponding ExtraData record, and then sets ExtraData 
// fields on the returned mobj_t.
//
mobj_t *E_SpawnMapThingExt(mapthing_t *mt)
{
   unsigned int edThingIdx;
   mapthingext_t *edthing;
   mobj_t *mo;

   // The record number is stored in the control thing's options field.
   // Check to see if the record exists, and that ExtraData is loaded.
   if(!LevelInfo.extraData || numEDMapThings == 0 ||
      (edThingIdx = E_EDThingForRecordNum((unsigned short)(mt->options))) == numEDMapThings)
   {
      // spawn an Unknown thing
      mo = P_SpawnMobj(mt->x << FRACBITS, mt->y << FRACBITS, ONFLOORZ,
                       UnknownThingType);
      return mo;
   }

   // get a pointer to the proper ExtraData mapthing record
   edthing = &(EDThings[edThingIdx]);

   // propagate the control object's x, y, and angle fields to the
   // mapthing_t inside the record
   edthing->stdfields.x = mt->x;
   edthing->stdfields.y = mt->y;
   edthing->stdfields.angle = mt->angle;

   // spawn the thing normally
   mo = P_SpawnMapThing(&(edthing->stdfields));

   // set extended fields in mo from the record
   if(mo)
   {
      // 02/02/04: TID -- numeric id used for scripting
      P_AddThingTID(mo, edthing->tid);

      // 08/16/04: args values
      memcpy(mo->args, edthing->args, 5 * sizeof(long));
   }

   // return the spawned object back through P_SpawnMapThing
   return mo;
}

//
// E_LoadLineDefExt
//
// Called from P_LoadLines for lines with the ED_LINE_SPECIAL after they
// have been initialized normally. Normal fields will be altered and
// extended fields will be set in the linedef.
//
void E_LoadLineDefExt(line_t *line)
{
   unsigned int edLineIdx;
   maplinedefext_t *edline;

   // ExtraData record number is stored in line tag
   if(!LevelInfo.extraData || numEDLines == 0 ||
      (edLineIdx = E_EDLineForRecordNum((unsigned short)(line->tag))) == numEDLines)
   {
      // if no ExtraData or no such record, zero special and tag,
      // and we're finished here.
      line->special = 0;
      line->tag = 0;
      return;
   }

   // get a pointer to the proper ExtraData line record
   edline = &(EDLines[edLineIdx]);

   // apply standard fields to the line
   line->special = edline->stdfields.special;
   line->tag     = edline->stdfields.tag;

   // apply extended fields to the line

   // extended special flags
   line->extflags = edline->extflags;

   // args
   memcpy(line->args, edline->args, 5*sizeof(long));
}

//
// E_IsParamSpecial
//
// Tests if a given line special is parameterized.
//
boolean E_IsParamSpecial(short special)
{
   switch(special)
   {
   case 300: // Door_Raise
   case 301: // Door_Open
   case 302: // Door_Close
   case 303: // Door_CloseWaitOpen
   case 304: // Door_WaitRaise
   case 305: // Door_WaitClose
   case 306:
   case 307:
   case 308:
   case 309:
   case 310:
   case 311:
   case 312:
   case 313:
   case 314:
   case 315:
   case 316:
   case 317:
   case 318:
   case 319:
   case 320:
   case 321:
      return true;
   default:
      return false;
   }
}

void E_GetEDMapThings(mapthingext_t **things, int *numthings)
{
   *things = EDThings;
   *numthings = numEDMapThings;
}

void E_GetEDLines(maplinedefext_t **lines, int *numlines)
{
   *lines = EDLines;
   *numlines = numEDLines;
}

// EOF

