/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  Copyright (C) 2022 by
 *  Sultim Tsyrendashiev
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

#include <math.h>

#include "doomstat.h"
#include "e6y.h"
#include "lprintf.h"
#include "rt_main.h"
#include "r_fps.h"
#include "r_main.h"
#include "r_patch.h"
#include "r_things.h"


#define SET_RGVEC2(vec, x, y)    (vec).data[0]=x;(vec).data[1]=y
#define SET_RGVEC3(vec, x, y, z) (vec).data[0]=x;(vec).data[1]=y;(vec).data[2]=z


typedef struct
{
  int cm;
  float x, y, z;
  float vt, vb;
  float ul, ur;
  float x1, y1;
  float x2, y2;
  float light;
  // float fogdensity;
  fixed_t scale;
  const rt_texture_t *td;
  uint_64_t flags;
  // int index;
  // int xy;
  // fixed_t fx, fy;

  dboolean has_rotation_frames;
} rt_sprite_t;


extern float RT_CalcLightLevel(int lightlevel);


static RgFloat3D GetNormalUp(void);
static RgFloat3D GetNormalTowardsCamera(void);
static RgFloat3D GetNormalForSprite(void);


static RgFloat3D GetCenter(const RgPrimitiveVertex vs[6])
{
  RgFloat3D center = { 0,0,0 };
  for (int i = 0; i < 6; i++)
  {
    center.data[0] += vs[i].position[0];
    center.data[1] += vs[i].position[1];
    center.data[2] += vs[i].position[2];
  }
  center.data[0] /= 6; center.data[1] /= 6; center.data[2] /= 6;

  return center;
}
static float GetMaxY(const RgPrimitiveVertex vs[6])
{
  float maxy = vs[0].position[1];
  for (int i = 1; i < 6; i++)
  {
    maxy = i_max(maxy, vs[i].position[1]);
  }
  return maxy;
}


#define Vec3Normalize(x)                                                      \
    do                                                                        \
    {                                                                         \
        float l = sqrtf((x)[0] * (x)[0] + (x)[1] * (x)[1] + (x)[2] * (x)[2]); \
        (x)[0] /= l;                                                          \
        (x)[1] /= l;                                                          \
        (x)[2] /= l;                                                          \
    } while (0)


static RgFloat3D GetWorldDirection(const mobj_t* thing)
{
  fixed_t forward[] = {
      finecosine[thing->angle >> ANGLETOFINESHIFT],
      finesine[thing->angle >> ANGLETOFINESHIFT],
  };

  RgFloat3D r = {
      -(float)forward[0],
      0,
      (float)forward[1],
  };
  Vec3Normalize(r.data);
  return r;
}


static void DrawSprite(const mobj_t *thing, const rt_sprite_t *sprite, int sectornum)
{
  dboolean is_partial_invisibility  = !!(sprite->flags & MF_SHADOW);
  // dboolean is_translucent           = !!(sprite->flags & MF_TRANSLUCENT);
  dboolean add_lightsource = (sprite->td->flags & RT_TEXTURE_FLAG_WITH_LIGHTSOURCE_BIT)        && sprite->td->metainfo != NULL;
  dboolean add_conelight   = (sprite->td->flags & RT_TEXTURE_FLAG_WITH_VERTICAL_CONELIGHT_BIT) && sprite->td->metainfo != NULL;
  
  dboolean can_offset = sprite->has_rotation_frames;
  dboolean noshadows  = (add_lightsource && !can_offset);

  dboolean is_local_player = thing->type == MT_PLAYER && thing == players[displayplayer].mo;

  // don't add lights to local player sprites
  if (is_local_player)
  {
    add_lightsource = false;
    add_conelight   = false;
  }
  
  RgPrimitiveVertex vertices[ 6 ];


  RgFloat3D normal = is_partial_invisibility ? GetNormalTowardsCamera() : GetNormalForSprite();
  // TODO RT: tangent
  RgFloat4D tangent = { 1, 0, 0, 1 };

  #define RG_UNPACK_3(v) { (v).data[0], (v).data[1], (v).data[2] }
  #define RG_UNPACK_4(v) { (v).data[0], (v).data[1], (v).data[2], (v).data[3] }

  float yaw = 270.0f - (float)(viewangle >> ANGLETOFINESHIFT) * 360.0f / FINEANGLES;
  float inv_yaw = 180.0f - yaw;
  float cos_inv_yaw = cosf(inv_yaw * (float)M_PI / 180.f);
  float sin_inv_yaw = sinf(inv_yaw * (float)M_PI / 180.f);

  RgColor4DPacked32 lightcolor =
      rgUtilPackColorFloat4D(sprite->light, sprite->light, sprite->light, 1.0f);

  if (sprite->flags & (MF_SOLID | MF_SPAWNCEILING))
  {
    float x1 = +(sprite->x1 * cos_inv_yaw) + sprite->x;
    float x2 = +(sprite->x2 * cos_inv_yaw) + sprite->x;

    float y1 = sprite->y + sprite->y1;
    float y2 = sprite->y + sprite->y2;

    float z2 = -(sprite->x1 * sin_inv_yaw) + sprite->z;
    float z1 = -(sprite->x2 * sin_inv_yaw) + sprite->z;

    // clang-format off
    RgPrimitiveVertex v0 = { .position = { x1, y1, z2 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ul, sprite->vt }, .color = lightcolor };
    RgPrimitiveVertex v1 = { .position = { x2, y1, z1 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ur, sprite->vt }, .color = lightcolor };
    RgPrimitiveVertex v2 = { .position = { x1, y2, z2 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ul, sprite->vb }, .color = lightcolor };
    RgPrimitiveVertex v3 = { .position = { x2, y2, z1 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ur, sprite->vb }, .color = lightcolor };
    // clang-format on

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    vertices[3] = v1;
    vertices[4] = v3;
    vertices[5] = v2;
  }
  else
  {
    const float cos_paperitems_pitch = cosf(0);
    const float sin_paperitems_pitch = sinf(0);

    float ycenter = fabsf(sprite->y1 - sprite->y2) * 0.5f;
    float y1c = sprite->y1 - ycenter;
    float y2c = sprite->y2 - ycenter;
    float cy = sprite->y + ycenter;

    float y1z2_y = -(y1c * sin_paperitems_pitch);
    float y2z2_y = -(y2c * sin_paperitems_pitch);

    float x1 = +(sprite->x1 * cos_inv_yaw - y1z2_y * sin_inv_yaw) + sprite->x;
    float x2 = +(sprite->x2 * cos_inv_yaw - y1z2_y * sin_inv_yaw) + sprite->x;
    float x3 = +(sprite->x1 * cos_inv_yaw - y2z2_y * sin_inv_yaw) + sprite->x;
    float x4 = +(sprite->x2 * cos_inv_yaw - y2z2_y * sin_inv_yaw) + sprite->x;

    float y1 = +(y1c * cos_paperitems_pitch) + cy;
    float y2 = +(y2c * cos_paperitems_pitch) + cy;

    float z1 = -(sprite->x1 * sin_inv_yaw + y1z2_y * cos_inv_yaw) + sprite->z;
    float z2 = -(sprite->x2 * sin_inv_yaw + y1z2_y * cos_inv_yaw) + sprite->z;
    float z3 = -(sprite->x1 * sin_inv_yaw + y2z2_y * cos_inv_yaw) + sprite->z;
    float z4 = -(sprite->x2 * sin_inv_yaw + y2z2_y * cos_inv_yaw) + sprite->z;

    // clang-format off
    RgPrimitiveVertex v0 = { .position = { x1, y1, z1 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ul, sprite->vt }, .color = lightcolor };
    RgPrimitiveVertex v1 = { .position = { x2, y1, z2 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ur, sprite->vt }, .color = lightcolor };
    RgPrimitiveVertex v2 = { .position = { x3, y2, z3 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ul, sprite->vb }, .color = lightcolor };
    RgPrimitiveVertex v3 = { .position = { x4, y2, z4 }, .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = { sprite->ur, sprite->vb }, .color = lightcolor };
    // clang-format on

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    vertices[3] = v1;
    vertices[4] = v3;
    vertices[5] = v2;
  }

  #undef RG_UNPACK_3
  #undef RG_UNPACK_4


  RgMeshPrimitiveInfo prim = {
      .sType = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_INFO,
      .pNext = NULL,
      .flags = RG_MESH_PRIMITIVE_ALPHA_TESTED | RG_MESH_PRIMITIVE_DONT_GENERATE_NORMALS |
               (is_partial_invisibility ? RG_MESH_PRIMITIVE_WATER : 0) |
               (is_local_player ? RG_MESH_PRIMITIVE_FIRST_PERSON_VIEWER : 0),
      .pPrimitiveNameInMesh = NULL,
      .primitiveIndexInMesh = 0,
      .pVertices            = vertices,
      .vertexCount          = RG_ARRAY_SIZE(vertices),
      .pIndices             = NULL,
      .indexCount           = 0,
      .pTextureName         = sprite->td->name,
      .color                = RG_PACKED_COLOR_WHITE,
      .emissive             = RT_TEXTURE_EMISSION(sprite->td),
  };

  RgMeshInfo info = {
      .sType          = RG_STRUCTURE_TYPE_MESH_INFO,
      .pNext          = NULL,
      .uniqueObjectID = RT_GetUniqueID_Thing(thing),
      .pMeshName      = NULL,
      .transform      = RG_TRANSFORM_IDENTITY,
      .isExportable   = false,
      .animationName  = NULL,
      .animationTime  = 0.0f,
  };

  RgResult r = rgUploadMeshPrimitive(rtmain.instance, &info, &prim);
  RG_CHECK(r);


  if (add_lightsource)
  {
    const rt_texture_metainfo_t *mt = sprite->td->metainfo;

    RgFloat3D pos = GetCenter(vertices);
    if (can_offset)
    {
        RgFloat3D dir    = GetWorldDirection(thing);
        float     offset = 0.3f;

        pos.data[0] += offset * dir.data[0];
        pos.data[1] += offset * dir.data[1];
        pos.data[2] += offset * dir.data[2];
    }

    float falloff = 7 * mt->falloff_multiplier;

    RgLightSphericalEXT ext = {
        .sType     = RG_STRUCTURE_TYPE_LIGHT_SPHERICAL_EXT,
        .pNext     = NULL,
        .color     = mt->light_color,
        .intensity = falloff * RG_LIGHT_INTENSITY_MULT,
        .position  = pos,
        .radius    = 0.01f,
    };

    RgLightInfo lt = {
        .sType        = RG_STRUCTURE_TYPE_LIGHT_INFO,
        .pNext        = &ext,
        .uniqueID     = RT_GetUniqueID_Thing(thing),
        .isExportable = false,
    };

    r = rgUploadLight(rtmain.instance, &lt);
    RG_CHECK(r);
  }
  else if (add_conelight)
  {
    float x_radius = (sprite->x2 - sprite->x1) * 0.5f;

    if (x_radius > 0)
    {
        const rt_texture_metainfo_t *mt = sprite->td->metainfo;

        RgFloat3D pos = GetCenter(vertices);
        pos.data[1]   = GetMaxY(vertices);

        RgLightSpotEXT ext = {
            .sType      = RG_STRUCTURE_TYPE_LIGHT_SPOT_EXT,
            .pNext      = NULL,
            .color      = mt->light_color,
            .intensity  = mt->falloff_multiplier * RG_LIGHT_INTENSITY_MULT,
            .position   = pos,
            .direction  = { 0, 1, 0 },
            .radius     = x_radius,
            .angleOuter = DEG2RAD(85),
            .angleInner = DEG2RAD(50),
        };

        RgLightInfo lt = {
            .sType        = RG_STRUCTURE_TYPE_LIGHT_INFO,
            .pNext        = &ext,
            .uniqueID     = RT_GetUniqueID_Thing(thing),
            .isExportable = false,
        };

        r = rgUploadLight(rtmain.instance, &lt);
        RG_CHECK(r);
    }
  }
}


void RT_AddSprite(int sectornum, mobj_t* thing, int lightlevel)
{
  if (sectornum < 0 || sectornum >= numsectors)
  {
    assert(0);
    return;
  }


  spritedef_t *sprdef;
  spriteframe_t *sprframe;
  int lump;
  dboolean flip;

  fixed_t fx, fy, fz;

  rt_sprite_t sprite = { 0 };

  // int mlook = HaveMouseLook() || (render_fov > FOV90);

  if (!paused && movement_smooth)
  {
    fx = thing->PrevX + FixedMul(tic_vars.frac, thing->x - thing->PrevX);
    fy = thing->PrevY + FixedMul(tic_vars.frac, thing->y - thing->PrevY);
    fz = thing->PrevZ + FixedMul(tic_vars.frac, thing->z - thing->PrevZ);
  }
  else
  {
    fx = thing->x;
    fy = thing->y;
    fz = thing->z;
  }

  fixed_t tr_x = fx - viewx;
  fixed_t tr_y = fy - viewy;

  fixed_t gxt = FixedMul(tr_x, viewcos);
  fixed_t gyt = -FixedMul(tr_y, viewsin);

  fixed_t tz = gxt - gyt;

  // thing is behind view plane?
  //if (tz < r_near_clip_plane)
  //  return;

  //gxt = -FixedMul(tr_x, viewsin);
  //gyt =  FixedMul(tr_y, viewcos);
  //fixed_t tx = -(gyt + gxt);

  //e6y
  //if (!render_paperitems && mlook)
  //{
  //  if (tz >= MINZ && (D_abs(tx) >> 5) > tz)
  //    return;
  //}
  //else
  //{
  //  // too far off the side?
  //  if (D_abs(tx) > (tz << 2))
  //    return;
  //}

  // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
  if ((unsigned)thing->sprite >= (unsigned)numsprites)
    I_Error("RT_ProjectSprite: Invalid sprite number %i", thing->sprite);
#endif

  sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
  if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
    I_Error("RT_ProjectSprite: Invalid sprite frame %i : %i", thing->sprite, thing->frame);
#endif

  if (!sprdef->spriteframes)
    I_Error("RT_ProjectSprite: Missing spriteframes %i : %i", thing->sprite, thing->frame);

  sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

  if (sprframe->rotate)
  {
    // choose a different rotation based on player view
    angle_t rot;
    angle_t ang = R_PointToAngle2(viewx, viewy, fx, fy);
    if (sprframe->lump[0] == sprframe->lump[1])
    {
      rot = (ang - thing->angle + (angle_t)(ANG45 / 2) * 9) >> 28;
    }
    else
    {
      rot = (ang - thing->angle + (angle_t)(ANG45 / 2) * 9 -
             (angle_t)(ANG180 / 16)) >> 28;
    }
    lump = sprframe->lump[rot];
    flip = (dboolean)(sprframe->flip & (1 << rot));
  }
  else
  {
    // use single rotation for all views
    lump = sprframe->lump[0];
    flip = (dboolean)(sprframe->flip & 1);
  }
  lump += firstspritelump;

  const rpatch_t *patch = R_CachePatchNum(lump);
  thing->patch_width = patch->width;

  //// killough 4/9/98: clip things which are out of view due to height
  //if (!mlook)
  //{
  //  int x1, x2;
  //  fixed_t xscale = FixedDiv(projection, tz);
  //  /* calculate edges of the shape
  //  * cph 2003/08/1 - fraggle points out that this offset must be flipped
  //  * if the sprite is flipped; e.g. FreeDoom imp is messed up by this. */
  //  if (flip)
  //    tx -= (patch->width - patch->leftoffset) << FRACBITS;
  //  else
  //    tx -= patch->leftoffset << FRACBITS;

  //  x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;
  //  tx += patch->width << FRACBITS;
  //  x2 = ((centerxfrac + FixedMul(tx, xscale) - FRACUNIT / 2) >> FRACBITS);

  //  // off the side?
  //  if (x1 > viewwidth || x2 < 0)
  //    goto unlock_patch;
  //}

  // killough 3/27/98: exclude things totally separated
  // from the viewer, by either water or fake ceilings
  // killough 4/11/98: improve sprite clipping for underwater/fake ceilings

  //heightsec = thing->subsector->sector->heightsec;
  //if (heightsec != -1)   // only clip things which are in special sectors
  //{
  //  int phs = viewplayer->mo->subsector->sector->heightsec;
  //  fixed_t gzt = fz + (patch->topoffset << FRACBITS);
  //  if (phs != -1 && viewz < sectors[phs].floorheight ?
  //      fz >= sectors[heightsec].floorheight :
  //      gzt < sectors[heightsec].floorheight)
  //    goto unlock_patch;
  //  if (phs != -1 && viewz > sectors[phs].ceilingheight ?
  //      gzt < sectors[heightsec].ceilingheight && viewz >= sectors[heightsec].ceilingheight :
  //      fz >= sectors[heightsec].ceilingheight)
  //    goto unlock_patch;
  //}

  // RT: don't ignore local player model
  // if (thing == players[displayplayer].mo && walkcamera.type != 2) // e6y FIXME!!!
  //   goto unlock_patch;

  sprite.x = -(float)fx / MAP_SCALE;
  sprite.y =  (float)fz / MAP_SCALE;
  sprite.z =  (float)fy / MAP_SCALE;

  // Bring items up out of floor by configurable amount times .01 Mead 8/13/03
  sprite.y += gl_sprite_offset;

  sprite.x2 = (float)patch->leftoffset / MAP_COEFF;
  sprite.x1 = sprite.x2 - ((float)patch->width / MAP_COEFF);
  sprite.y1 = (float)patch->topoffset / MAP_COEFF;
  sprite.y2 = sprite.y1 - ((float)patch->height / MAP_COEFF);

  // e6y
  // if the sprite is below the floor, and it's not a hanger/floater/missile, 
  // and it's not a fully dead corpse, move it up
  if ((gl_spriteclip != spriteclip_const) &&
      (sprite.y2 < 0) && (sprite.y2 >= (float)(-gl_spriteclip_threshold_f)) &&
      !(thing->flags & (MF_SPAWNCEILING | MF_FLOAT | MF_MISSILE | MF_NOGRAVITY)) &&
      ((gl_spriteclip == spriteclip_always) || !((thing->flags & MF_CORPSE) && thing->tics == -1)))
  {
    sprite.y1 -= sprite.y2;
    sprite.y2 = 0.0f;
  }

  //if (frustum_culling)
  //{
  //  if (!gld_SphereInFrustum(
  //    sprite.x + cos_inv_yaw * (sprite.x1 + sprite.x2) / 2.0f,
  //    sprite.y + (sprite.y1 + sprite.y2) / 2.0f,
  //    sprite.z - sin_inv_yaw * (sprite.x1 + sprite.x2) / 2.0f,
  //    //1.5 == sqrt(2) + small delta for MF_FOREGROUND
  //    (float)(MAX(patch->width, patch->height)) / MAP_COEFF / 2.0f * 1.5f))
  //  {
  //    goto unlock_patch;
  //  }
  //}

  sprite.scale = FixedDiv(projectiony, tz);
  if ((thing->frame & FF_FULLBRIGHT) || show_alive)
  {
    //sprite.fogdensity = 0.0f;
    sprite.light = 1.0f;
  }
  else
  {
    sprite.light = RT_CalcLightLevel(lightlevel + (extralight << 5));
    //sprite.fogdensity = CalcFogDensity(thing->subsector->sector, lightlevel, GLDIT_SPRITE);
  }
  sprite.cm = CR_LIMIT + (int)((thing->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT));
  // [FG] colored blood and gibs
  if (thing->flags & MF_COLOREDBLOOD)
  {
    sprite.cm = thing->bloodcolor;
  }
  sprite.td = RT_Texture_GetFromPatchLump(lump); // sprite.cm
  if (!sprite.td)
    goto unlock_patch;
  sprite.flags = thing->flags;

  //if (thing->flags & MF_FOREGROUND)
  //  scene_has_overlapped_sprites = true;

  //sprite.index = gl_spriteindex++;
  //sprite.xy = thing->x + (thing->y >> 16);
  //sprite.fx = thing->x;
  //sprite.fy = thing->y;

  sprite.vt = 0.0f;
  sprite.vb = 1.0f;
  if (flip)
  {
    sprite.ul = 0.0f;
    sprite.ur = 1.0f;
  }
  else
  {
    sprite.ul = 1.0f;
    sprite.ur = 0.0f;
  }

  sprite.has_rotation_frames = sprframe->rotate;

  DrawSprite(thing, &sprite, sectornum);

unlock_patch:
  R_UnlockPatchNum(lump);
}


static RgFloat4D ApplyMat44ToVec4(const float column_mat[4][4], const float v[4])
{
  RgFloat4D r;
  for (int i = 0; i < 4; i++)
  {
    r.data[i] = column_mat[0][i] * v[0] + column_mat[1][i] * v[1] + column_mat[2][i] * v[2] + column_mat[3][i] * v[3];
  }
  return r;
}


static RgFloat3D ApplyMat44ToDirection(const float column_mat[4][4], const float dir[3])
{
  RgFloat3D r;
  for (int i = 0; i < 3; i++)
  {
    r.data[i] = column_mat[0][i] * dir[0] + column_mat[1][i] * dir[1] + column_mat[2][i] * dir[2];
  }
  return r;
}


static RgFloat3D FromHomogeneous(const RgFloat4D v)
{
  RgFloat3D r = { v.data[0] / v.data[3],v.data[1] / v.data[3],v.data[2] / v.data[3] };
  return r;
}


static RgFloat3D GetNormalUp(void)
{
  RgFloat3D r = { 0, 1, 0 };
  return r;
}


static RgFloat3D GetNormalTowardsCamera(void)
{
  const float f[3] = { 0, 0, 1 };
  return ApplyMat44ToDirection(rtmain.mat_view_inverse, f);
}


static RgFloat3D GetNormalForSprite(void)
{
  const float f[3] = { 0,0,1 };
  RgFloat3D t = ApplyMat44ToDirection(rtmain.mat_view_inverse, f);

  // let up-down axis be some const, and then normalize the vector,
  // so flashlight (for which normals towards camera preferable)
  // and other illumination (normals up look better),
  // both look consistent enough
  t.data[1] = 1.0f;
  Vec3Normalize(t.data);

  return t;
}


void RT_AddWeaponSprite(int weaponlump, const vissprite_t* vis, float zoffset, int lightlevel)
{
  dboolean is_partial_invisibility = false;

  const rt_texture_t *td = RT_Texture_GetFromPatchLump(firstspritelump + weaponlump);
  if (!td)
    return;

  float fU1 = 0;
  float fV1 = 0;
  float fU2 = 1;
  float fV2 = 1;

  int ix1 = viewwindowx + vis->x1;
  int ix2 = ix1 + (int)((float)td->width * pspritexscale_f);
  int iy1 = viewwindowy + centery - (int)(((float)vis->texturemid / (float)FRACUNIT) * pspriteyscale_f);
  int iy2 = iy1 + (int)((float)td->height * pspriteyscale_f) + 1;

  float x1 = (float)ix1 / (float)SCREENWIDTH;
  float x2 = (float)ix2 / (float)SCREENWIDTH;
  float y1 = (float)iy1 / (float)SCREENHEIGHT;
  float y2 = (float)iy2 / (float)SCREENHEIGHT;

  RgFloat2D v0_screen = { x1, y1 };
  RgFloat2D v1_screen = { x1, y2 };
  RgFloat2D v2_screen = { x2, y1 };
  RgFloat2D v3_screen = { x2, y2 };

  float z = 0.1f + zoffset;
  RgFloat4D v0_ndc = { v0_screen.data[0] * 2 - 1, v0_screen.data[1] * 2 - 1, z, 1.0f };
  RgFloat4D v1_ndc = { v1_screen.data[0] * 2 - 1, v1_screen.data[1] * 2 - 1, z, 1.0f };
  RgFloat4D v2_ndc = { v2_screen.data[0] * 2 - 1, v2_screen.data[1] * 2 - 1, z, 1.0f };
  RgFloat4D v3_ndc = { v3_screen.data[0] * 2 - 1, v3_screen.data[1] * 2 - 1, z, 1.0f };

  // assume *_ndc are the same as clip space,
  // so apply inverse projection to get view space coords
  RgFloat4D v0_view = ApplyMat44ToVec4(rtmain.mat_projectionvk_inverse, v0_ndc.data);
  RgFloat4D v1_view = ApplyMat44ToVec4(rtmain.mat_projectionvk_inverse, v1_ndc.data);
  RgFloat4D v2_view = ApplyMat44ToVec4(rtmain.mat_projectionvk_inverse, v2_ndc.data);
  RgFloat4D v3_view = ApplyMat44ToVec4(rtmain.mat_projectionvk_inverse, v3_ndc.data);

  RgFloat3D v0_world = FromHomogeneous(ApplyMat44ToVec4(rtmain.mat_view_inverse, v0_view.data));
  RgFloat3D v1_world = FromHomogeneous(ApplyMat44ToVec4(rtmain.mat_view_inverse, v1_view.data));
  RgFloat3D v2_world = FromHomogeneous(ApplyMat44ToVec4(rtmain.mat_view_inverse, v2_view.data));
  RgFloat3D v3_world = FromHomogeneous(ApplyMat44ToVec4(rtmain.mat_view_inverse, v3_view.data));


  if (/*(viewplayer->mo->flags & MF_SHADOW) && */!vis->colormap)
  {
    is_partial_invisibility = true;
  }
  else
  {
    // dboolean is_translucent = viewplayer->mo->flags & MF_TRANSLUCENT;
  }

  RgFloat2D t0 = { fU1, fV1 };
  RgFloat2D t1 = { fU1, fV2 };
  RgFloat2D t2 = { fU2, fV1 };
  RgFloat2D t3 = { fU2, fV2 };
  
  RgFloat3D normal = is_partial_invisibility ? GetNormalTowardsCamera() : GetNormalUp();
  // TODO RT: tangent
  RgFloat4D tangent = { 1, 0, 0, 1 };

  float ll = RT_CalcLightLevel(lightlevel + (extralight << 5));
  RgColor4DPacked32 lightcolor = rgUtilPackColorFloat4D(ll, ll, ll, 1.0f);

  // clang-format off
  #define RG_UNPACK_2( v ) { (v).data[0], (v).data[1] }
  #define RG_UNPACK_3( v ) { (v).data[0], (v).data[1], (v).data[2] }
  #define RG_UNPACK_4( v ) { (v).data[0], (v).data[1], (v).data[2], (v).data[3] }
  RgPrimitiveVertex vertices[] = {
      { .position = RG_UNPACK_3(v0_world), .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = RG_UNPACK_2(t0), .color = lightcolor },
      { .position = RG_UNPACK_3(v1_world), .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = RG_UNPACK_2(t1), .color = lightcolor },
      { .position = RG_UNPACK_3(v2_world), .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = RG_UNPACK_2(t2), .color = lightcolor },
      { .position = RG_UNPACK_3(v1_world), .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = RG_UNPACK_2(t1), .color = lightcolor },
      { .position = RG_UNPACK_3(v3_world), .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = RG_UNPACK_2(t3), .color = lightcolor },
      { .position = RG_UNPACK_3(v2_world), .normal = RG_UNPACK_3(normal), .tangent = RG_UNPACK_4(tangent), .texCoord = RG_UNPACK_2(t2), .color = lightcolor },
  };
  #undef RG_UNPACK_2
  #undef RG_UNPACK_3
  #undef RG_UNPACK_4
  // clang-format on

  RgMeshPrimitiveInfo prim = {
      .sType = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_INFO,
      .pNext = NULL,
      .flags = RG_MESH_PRIMITIVE_ALPHA_TESTED | RG_MESH_PRIMITIVE_DONT_GENERATE_NORMALS |
               (is_partial_invisibility ? RG_MESH_PRIMITIVE_WATER : 0) |
               RG_MESH_PRIMITIVE_FIRST_PERSON,
      .pPrimitiveNameInMesh = NULL,
      .primitiveIndexInMesh = 0,
      .pVertices            = vertices,
      .vertexCount          = RG_ARRAY_SIZE(vertices),
      .pIndices             = NULL,
      .indexCount           = 0,
      .pTextureName         = td->name,
      .color                = RG_PACKED_COLOR_WHITE,
      .emissive             = RT_TEXTURE_EMISSION(td),
  };

  RgMeshInfo info = {
      .sType          = RG_STRUCTURE_TYPE_MESH_INFO,
      .pNext          = NULL,
      .uniqueObjectID = RT_GetUniqueID_FirstPersonWeapon(weaponlump),
      .pMeshName      = NULL,
      .transform      = RG_TRANSFORM_IDENTITY,
      .isExportable   = false,
      .animationName  = NULL,
      .animationTime  = 0.0f,
  };

  RgResult r = rgUploadMeshPrimitive(rtmain.instance, &info, &prim);
  RG_CHECK(r);
}


#include <i_video.h>


static RgFloat3D GetCameraDirection(void)
{
  const float f[4] = { 0,0,-1,0 };
  RgFloat4D cam_dir = ApplyMat44ToVec4(rtmain.mat_view_inverse, f);
  const float *d = cam_dir.data;

  float len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
  RgFloat3D r;
  if (len > 0.0001f)
  {
    r.data[0] = d[0] / len;
    r.data[1] = d[1] / len;
    r.data[2] = d[2] / len;
  }
  else
  {
    //assert(0);
    r.data[0] = 1;
    r.data[1] = 0;
    r.data[2] = 0;
  }
  return r;
}


static RgFloat3D GetCameraPosition(void)
{
  const float *cam_pos = rtmain.mat_view_inverse[3];
  RgFloat3D r = { cam_pos[0], cam_pos[1], cam_pos[2] };
  return r;
}


static RgFloat3D GetOffsetFromCameraPosition(const RgFloat3D dir, float x, float y, float z)
{
  const static RgFloat3D up = { 0,1,0 };

  // cross
  const RgFloat3D right = {
    dir.data[1] * up.data[2] - dir.data[2] * up.data[1],
    dir.data[2] * up.data[0] - dir.data[0] * up.data[2],
    dir.data[0] * up.data[1] - dir.data[1] * up.data[0]
  };

  RgFloat3D r = GetCameraPosition();

  for (int i = 0; i < 3; i++)
  {
    r.data[i] += right.data[i] * x;
    r.data[i] +=    up.data[i] * y;
    r.data[i] +=   dir.data[i] * z;
  }

  return r;
}


// muzzlelight is extralight,
// extralight is 1 or 2 when muzzle flash is active
void AddMuzzleFlashLight(int muzzlelight, float flash_z_offset)
{
  if (muzzlelight <= 0)
  {
    return;
  }

  float flash_intensity = muzzlelight >= 2 ? 1.0f : 0.5f;
  float flash_y_offset = -0.15f;

  if (!rt_settings.muzzleflash_enable)
  {
    return;
  }

  RgFloat3D flash_pos = GetOffsetFromCameraPosition(GetCameraDirection(), 0, flash_y_offset, flash_z_offset);

  float falloff = 6.5f;

  RgLightSphericalEXT ext = {
      .sType     = RG_STRUCTURE_TYPE_LIGHT_SPHERICAL_EXT,
      .pNext     = NULL,
      .color     = rgUtilPackColorByte4D(255, 222, 148, 255),
      .intensity = flash_intensity * falloff * RG_LIGHT_INTENSITY_MULT,
      .position  = flash_pos,
      .radius    = 0.05f,
  };

  RgLightInfo info = {
      .sType        = RG_STRUCTURE_TYPE_LIGHT_INFO,
      .pNext        = &ext,
      .uniqueID     = RT_GetUniqueID_FirstPersonWeapon(0),
      .isExportable = false,
  };

  RgResult r = rgUploadLight(rtmain.instance, &info);
  RG_CHECK(r);
}


static void AddFlashlight(float to_left_offset, float y_multiplier)
{
  assert(!(rtmain.powerupflags & RT_POWERUP_FLAG_MORELIGHT_BIT));

  float x = -to_left_offset;
  float y = -0.1f * y_multiplier;

  RgFloat3D cam_dir = GetCameraDirection();
  RgFloat3D pos = GetOffsetFromCameraPosition(cam_dir, x, y, 0);

  float falloff = 7;

  RgLightSpotEXT ext = {
      .sType      = RG_STRUCTURE_TYPE_LIGHT_SPOT_EXT,
      .pNext      = NULL,
      .position   = pos,
      .direction  = cam_dir,
      .color      = rgUtilPackColorByte4D(204, 204, 255, 255),
      .intensity  = falloff * RG_LIGHT_INTENSITY_MULT,
      .radius     = 0.01f,
      .angleOuter = DEG2RAD(25),
      .angleInner = DEG2RAD(5),
  };

  RgLightInfo info = {
      .sType        = RG_STRUCTURE_TYPE_LIGHT_INFO,
      .pNext        = &ext,
      .uniqueID     = RT_GetUniqueID_FirstPersonWeapon(2),
      .isExportable = false,
  };

  RgResult r = rgUploadLight(rtmain.instance, &info);
  RG_CHECK(r);
}


static void AddClassicPlayerLight(void)
{
  const float intensity = 0.07f;
  const float falloff = 4;

  const float x = 0;
  const float y = -0.12f;
  const float z = 0.07f;

  RgFloat3D dir = GetCameraDirection();
  dir.data[1]   = 0;
  Vec3Normalize(dir.data);
  
  RgFloat3D pos = GetOffsetFromCameraPosition(dir, x, y, z);

  RgLightSphericalEXT ext = {
      .sType     = RG_STRUCTURE_TYPE_LIGHT_SPHERICAL_EXT,
      .pNext     = NULL,
      .color     = rgUtilPackColorByte4D(255, 222, 255, 255),
      .intensity = intensity * falloff * RG_LIGHT_INTENSITY_MULT,
      .position  = pos,
      .radius    = 0.01f,
  };

  RgLightInfo info = {
      .sType        = RG_STRUCTURE_TYPE_LIGHT_INFO,
      .pNext        = &ext,
      .uniqueID     = RT_GetUniqueID_FirstPersonWeapon(1),
      .isExportable = false,
  };

  RgResult r = rgUploadLight(rtmain.instance, &info);
  RG_CHECK(r);
}


#include "p_maputl.h"
dboolean PTR_NoWayTraverse_RT(intercept_t *in)
{
  if (in->isaline)
  {
    // found any line => assume that it's an obstacle
    return false;
  }
  else
  {
    const mobj_t *thing = in->d.thing;

    // if it's an enemy
    if ((thing->flags & MF_COUNTKILL) && !(thing->flags & MF_CORPSE))
    {
      // it's an obstacle
      return false;
    }
  }

  return true;
}


static dboolean AreNoObstacles(const fixed_t src[2], const fixed_t dst[2])
{
  // RT: based on P_UseLines
  return P_PathTraverse(src[0], src[1], dst[0], dst[1], PT_ADDLINES | PT_ADDTHINGS, PTR_NoWayTraverse_RT);
}


#define Fixed2_AddMultiplied(p, dir, range) \
  { (p)[0] + (((range) * FRACUNIT) >> FRACBITS) * (dir)[0], \
    (p)[1] + (((range) * FRACUNIT) >> FRACBITS) * (dir)[1] } 


#define Lerp(a,b,t) ((a)*(1.0f-(t)) + (b)*(t))


#define MUZZLEFLASH_MAX_ZOFFSET 0.75f
#define MUZZLEFLASH_OBSTACLE_CHECKRANGE 64

#define FLASHLIGHT_OBSTACLE_CHECKRANGE 32


// Called for local player
void RT_ProcessPlayer(const player_t *player)
{
  fixed_t position[] = { player->mo->x, player->mo->y };

  int angle = player->mo->angle >> ANGLETOFINESHIFT;

  fixed_t forward[] = { finecosine[angle], finesine[angle] }; // angle
  fixed_t left[] = { -finesine[angle], finecosine[angle] };   // angle + pi/2


  // RT: not important timer to lerp z offset
  static float last_time = 0;

  float cur_time = (float)RT_GetCurrentTime();
  float delta_time = i_max(cur_time - last_time, 0.001f);
  last_time = cur_time;


  {
    static float muzzleflash_z_offset = 0;
    fixed_t p_dst[] = Fixed2_AddMultiplied(position, forward, MUZZLEFLASH_OBSTACLE_CHECKRANGE);

    muzzleflash_z_offset = AreNoObstacles(position, p_dst) ?
      Lerp(muzzleflash_z_offset, MUZZLEFLASH_MAX_ZOFFSET, 2 * delta_time) :
      Lerp(muzzleflash_z_offset, 0, 20 * delta_time);

    AddMuzzleFlashLight(player->extralight, muzzleflash_z_offset);
  }


  static float flashlight_to_left_offset = 0;
  const float offset_bound = 0.3f;
  if (rtmain.request_flashlight && !(rtmain.powerupflags & RT_POWERUP_FLAG_MORELIGHT_BIT) && !rt_settings.classic_flashlight)
  {
    fixed_t dst_fwd[]  = Fixed2_AddMultiplied(position, forward, FLASHLIGHT_OBSTACLE_CHECKRANGE * 3);
    fixed_t dst_left[] = Fixed2_AddMultiplied(position, left, FLASHLIGHT_OBSTACLE_CHECKRANGE);

    float target_offset, speed;
    
    if (AreNoObstacles(position, dst_left))
    {
      if (AreNoObstacles(dst_left, dst_fwd))
      {
        // default
        target_offset = 0.2f;
        speed = 2;
      }
      else
      {
        target_offset = -0.02f;
        speed = 2;
      }
    }
    else
    {
      target_offset = -0.02f;
      speed = 20;
    }
    // check if not too much
    assert(fabs(target_offset) < fabs(offset_bound));
    flashlight_to_left_offset = BETWEEN(-offset_bound, offset_bound, flashlight_to_left_offset);
    
    flashlight_to_left_offset = Lerp(flashlight_to_left_offset, target_offset, speed * delta_time);

    float dy;
    {
      fixed_t y_min = 6 * FRACUNIT; // death cam, look p_user.c
      fixed_t y_max = VIEWHEIGHT;
      dy = (float)(player->viewheight - y_min) / (float)(y_max - y_min);
      dy = BETWEEN(0.0f, 1.0f, dy);
    }

    AddFlashlight(flashlight_to_left_offset, dy);
  }
  else
  {
    // when flashlight is disabled, instantly move it to default position,
    // for some animation on enabling a flashlight
    flashlight_to_left_offset = 0;
  }

  if (rtmain.request_flashlight && !(rtmain.powerupflags & RT_POWERUP_FLAG_MORELIGHT_BIT) && rt_settings.classic_flashlight)
  {
    AddClassicPlayerLight();
  }
}


void RT_SetPowerupPalette(uint32_t powerupflags)
{
  rtmain.powerupflags = 0;

  if (powerupflags & RT_POWERUP_FLAG_BERSERK_BIT)
  {
    rtmain.powerupflags |= RT_POWERUP_FLAG_BERSERK_BIT;
  }

  if (powerupflags & RT_POWERUP_FLAG_DAMAGE_BIT)
  {
    rtmain.powerupflags |= RT_POWERUP_FLAG_DAMAGE_BIT;
  }

  if (powerupflags & RT_POWERUP_FLAG_RADIATIONSUIT_BIT)
  {
    rtmain.powerupflags |= RT_POWERUP_FLAG_RADIATIONSUIT_BIT;
  }

  if (powerupflags & RT_POWERUP_FLAG_BONUS_BIT)
  {
    rtmain.powerupflags |= RT_POWERUP_FLAG_BONUS_BIT;
  }

  if (players[displayplayer].fixedcolormap == INVERSECOLORMAP)
  {
    rtmain.powerupflags |= RT_POWERUP_FLAG_INVUNERABILITY_BIT;
  }
  else if (players[displayplayer].fixedcolormap > 0)
  {
    rtmain.powerupflags |= RT_POWERUP_FLAG_MORELIGHT_BIT;
  }
}
